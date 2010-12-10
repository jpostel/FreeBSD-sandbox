/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sys_generic.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_compat.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysproto.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/socketvar.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/resourcevar.h>
#include <sys/selinfo.h>
#include <sys/sleepqueue.h>
#include <sys/syscallsubr.h>
#include <sys/sysctl.h>
#include <sys/sysent.h>
#include <sys/vnode.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <security/audit/audit.h>

static MALLOC_DEFINE(M_IOCTLOPS, "ioctlops", "ioctl data buffer");
static MALLOC_DEFINE(M_SELECT, "select", "select() buffer");
MALLOC_DEFINE(M_IOV, "iov", "large iov's");

static void	doselwakeup(struct selinfo *, int);

/*
 * One seltd per-thread allocated on demand as needed.
 *
 *	t - protected by st_mtx
 * 	k - Only accessed by curthread or read-only
 */
struct seltd {
	STAILQ_HEAD(, selfd)	st_selq;	/* (k) List of selfds. */
	struct selfd		*st_free1;	/* (k) free fd for read set. */
	struct selfd		*st_free2;	/* (k) free fd for write set. */
	struct mtx		st_mtx;		/* Protects struct seltd */
	struct cv		st_wait;	/* (t) Wait channel. */
	int			st_flags;	/* (t) SELTD_ flags. */
};

#define	SELTD_PENDING	0x0001			/* We have pending events. */
#define	SELTD_RESCAN	0x0002			/* Doing a rescan. */

/*
 * One selfd allocated per-thread per-file-descriptor.
 *	f - protected by sf_mtx
 */
struct selfd {
	STAILQ_ENTRY(selfd)	sf_link;	/* (k) fds owned by this td. */
	TAILQ_ENTRY(selfd)	sf_threads;	/* (f) fds on this selinfo. */
	struct selinfo		*sf_si;		/* (f) selinfo when linked. */
	struct mtx		*sf_mtx;	/* Pointer to selinfo mtx. */
	struct seltd		*sf_td;		/* (k) owning seltd. */
	void			*sf_cookie;	/* (k) fd or pollfd. */
};

#if 0
static uma_zone_t selfd_zone;
#endif
static struct mtx_pool *mtxpool_select;

/*
 * Record a select request.
 */
void
selrecord(selector, sip)
	struct thread *selector;
	struct selinfo *sip;
{
	struct selfd *sfp;
	struct seltd *stp;
	struct mtx *mtxp;

	stp = selector->td_sel;
	/*
	 * Don't record when doing a rescan.
	 */
	if (stp->st_flags & SELTD_RESCAN)
		return;
	/*
	 * Grab one of the preallocated descriptors.
	 */
	sfp = NULL;
	if ((sfp = stp->st_free1) != NULL)
		stp->st_free1 = NULL;
	else if ((sfp = stp->st_free2) != NULL)
		stp->st_free2 = NULL;
	else
		panic("selrecord: No free selfd on selq");
	mtxp = sip->si_mtx;
	if (mtxp == NULL)
		mtxp = mtx_pool_find(mtxpool_select, sip);
	/*
	 * Initialize the sfp and queue it in the thread.
	 */
	sfp->sf_si = sip;
	sfp->sf_mtx = mtxp;
	STAILQ_INSERT_TAIL(&stp->st_selq, sfp, sf_link);
	/*
	 * Now that we've locked the sip, check for initialization.
	 */
	mtx_lock(mtxp);
	if (sip->si_mtx == NULL) {
		sip->si_mtx = mtxp;
		TAILQ_INIT(&sip->si_tdlist);
	}
	/*
	 * Add this thread to the list of selfds listening on this selinfo.
	 */
	TAILQ_INSERT_TAIL(&sip->si_tdlist, sfp, sf_threads);
	mtx_unlock(sip->si_mtx);
}

/* Wake up a selecting thread, and set its priority. */
void
selwakeuppri(sip, pri)
	struct selinfo *sip;
	int pri;
{
	doselwakeup(sip, pri);
}

/*
 * Do a wakeup when a selectable event occurs.
 */
static void
doselwakeup(sip, pri)
	struct selinfo *sip;
	int pri;
{
	struct selfd *sfp;
	struct selfd *sfn;
	struct seltd *stp;

	/* If it's not initialized there can't be any waiters. */
	if (sip->si_mtx == NULL)
		return;
	/*
	 * Locking the selinfo locks all selfds associated with it.
	 */
	mtx_lock(sip->si_mtx);
	TAILQ_FOREACH_SAFE(sfp, &sip->si_tdlist, sf_threads, sfn) {
		/*
		 * Once we remove this sfp from the list and clear the
		 * sf_si seltdclear will know to ignore this si.
		 */
		TAILQ_REMOVE(&sip->si_tdlist, sfp, sf_threads);
		sfp->sf_si = NULL;
		stp = sfp->sf_td;
		mtx_lock(&stp->st_mtx);
		stp->st_flags |= SELTD_PENDING;
		cv_broadcastpri(&stp->st_wait, pri);
		mtx_unlock(&stp->st_mtx);
	}
	mtx_unlock(sip->si_mtx);
}
