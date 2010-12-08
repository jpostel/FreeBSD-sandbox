/*-
 * Copyright (c) 2010 Jacob Postel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#undef _KERNEL
#define _WANT_UCRED
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/refcount.h>
#include <sys/ucred.h>
#include <sys/time.h>
#include <sys/proc.h>


#include <pthread.h>
struct malloc_type;

vm_offset_t kmem_malloc(void * map, int bytes, int wait);
void kmem_free(void *map, vm_offset_t addr, vm_size_t size);

vm_offset_t
kmem_malloc(void * map, int bytes, int wait)
{

	return ((vm_offset_t)mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_ANON, -1, 0));
}


void
kmem_free(void *map, vm_offset_t addr, vm_size_t size)
{

	munmap((void *)addr, size);
}

void *
plebnet_malloc(unsigned long size, struct malloc_type *type, int flags)
{

	return (malloc(size));
}

void
plebnet_free(void *addr, struct malloc_type *type)
{

	free(addr);
}

/*
 * Claim another reference to a ucred structure.
 */
struct ucred *
crhold(struct ucred *cr)
{

	refcount_acquire(&cr->cr_ref);
	return (cr);
}

/*
 * Free a cred structure.  Throws away space when ref count gets to 0.
 */
void
crfree(struct ucred *cr)
{
	if (refcount_release(&cr->cr_ref)) {
		free(cr);
	}
}       

void
panic(const char *fmt, ...)
{

	abort();
}

void
bintime(struct bintime *bt)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	timeval2bintime(&tv, bt);
}
	
void
getmicrouptime(struct timeval *tvp)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
}

void
getmicrotime(struct timeval *tvp)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
}

__thread struct thread *pcurthread;

struct pthread_start_args 
{
	struct thread *psa_td;
	void (*psa_start_routine)(void *);
	void *psa_arg;
};

static void *
pthread_start_routine(void *arg)
{
	struct pthread_start_args *psa = arg;

	pcurthread = psa->psa_td;
	psa->psa_start_routine(psa->psa_arg);
	free(psa);

	return (NULL);
}

int
kproc_kthread_add(void (*start_routine)(void *), void *arg,
    struct proc **p,  struct thread **td,
    int flags, int pages,
    char * procname, const char *str, ...)
{
	int error;
	pthread_t thread;
	pthread_attr_t attr;
	struct pthread_start_args *psa;

	*td = malloc(sizeof(struct thread));
	psa = malloc(sizeof(struct pthread_start_args));
	psa->psa_start_routine = start_routine;
	psa->psa_arg = arg;
	psa->psa_td = *td;
	
	pthread_attr_init(&attr); 
	error = pthread_create(&thread, &attr, pthread_start_routine, psa);

	return (error);
}
