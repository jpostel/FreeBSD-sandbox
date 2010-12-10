.include <bsd.init.mk>

LIB=	plebnet

PN_KERN_COMMON_OBJS +=		\
	kern_environment.o	\
	kern_fail.o		\
	kern_mbuf.o		\
	kern_mtxpool.o		\
	kern_sysctl.o		\
	md5c.o			\
	subr_eventhandler.o	\
	subr_kobj.o		\
	subr_lock.o		\
	subr_param.o		\
	subr_pcpu.o		\
	subr_sbuf.o		\
	sys_socket.o		\
	uipc_accf.o		\
	uipc_mbuf.o		\
	uipc_mbuf2.o		\
	uipc_domain.o		\
	uipc_sockbuf.o		\
	uipc_socket.o

PN_NET_COMMON_OBJS +=		\
	bpf.o			\
	if.o			\
	if_clone.o		\
	if_dead.o		\
	if_ethersubr.o		\
	if_loop.o		\
	if_llatbl.o		\
	netisr.o		\
	pfil.o			\
	radix.o			\
	raw_cb.o		\
	raw_usrreq.o		\
	route.o			\
	rtsock.o		

PN_NETINET_COMMON_OBJS +=	\
	if_ether.o		\
	igmp.o			\
	in.o			\
	in_cksum.o		\
	in_mcast.o		\
	in_pcb.o		\
	in_proto.o		\
	in_rmx.o		\
	ip_encap.o		\
	ip_fastfwd.o		\
	ip_icmp.o		\
	ip_id.o			\
	ip_input.o		\
	ip_options.o		\
	ip_output.o		\
	raw_ip.o		\
	tcp_debug.o		\
	tcp_hostcache.o		\
	tcp_input.o		\
	tcp_lro.o		\
	tcp_offload.o		\
	tcp_output.o		\
	tcp_reass.o		\
	tcp_sack.o		\
	tcp_subr.o		\
	tcp_syncache.o		\
	tcp_timer.o		\
	tcp_timewait.o		\
	tcp_usrreq.o		\
	udp_usrreq.o		\
	cc.o			\
	cc_newreno.o

PN_LIBKERN_COMMON_OBJS =	\
	arc4random.o		\
	bcd.o			\
	strlcpy.o

PN_OTHER_COMMON_OBJS =		\
	harvest.o		\
	uma_core.o

PN_SYS_COMMON_OBJS = 		\
	pn_compat.o		\
	pn_glue.o		\
	pn_in_cksum.o		\
	pn_init.o		\
	pn_init_main.o		\
	pn_kern_condvar.o	\
	pn_kern_descrip.o	\
	pn_kern_intr.o		\
	pn_kern_subr.o		\
	pn_kern_synch.o		\
	pn_kern_timeout.o	\
	pn_lock.o		\
	pn_private.o		\
	pn_subr_prf.o		\
	pn_subr_taskqueue.o	\
	pn_sys_generic.o


PN_MFILES = 			\
	bus_if.h 		\
	device_if.h

PN_OPT_FILES = 			\
	opt_atalk.h		\
	opt_bootp.h		\
	opt_bpf.h		\
	opt_carp.h		\
	opt_compat.h		\
	opt_device_polling.h	\
	opt_ddb.h		\
	opt_inet.h		\
	opt_inet6.h		\
	opt_ipfw.h		\
	opt_ipsec.h		\
	opt_ipstealth.h		\
	opt_ipx.h		\
	opt_ktrace.h		\
	opt_maxusers.h		\
	opt_mbuf_profiling.h	\
	opt_mbuf_stress_test.h	\
	opt_mpath.h		\
	opt_mprof.h		\
	opt_mrouting.h		\
	opt_netgraph.h		\
	opt_param.h		\
	opt_pf.h		\
	opt_route.h		\
	opt_sctp.h		\
	opt_tcpdebug.h		\
	opt_zero.h

# Search for kernel source tree in standard places.
.for _dir in ${.CURDIR}/../.. ${.CURDIR}/../../sys ${.CURDIR}/../../.. /sys /usr/src/sys
.if !defined(SYSDIR) && exists(${_dir}/kern/)
SYSDIR=	${_dir}
.endif
.endfor
.if !defined(SYSDIR) || !exists(${SYSDIR}/kern/)
.error "can't find kernel source tree"
.endif


PREFIX= ${SYSDIR}
.PATH: ${PREFIX}/kern
.PATH: ${PREFIX}/net
.PATH: ${PREFIX}/netinet
.PATH: ${PREFIX}/netinet/cc
.PATH: ${PREFIX}/vm
.PATH: ${PREFIX}/libkern
.PATH: ${PREFIX}/dev/random
.PATH: plebnet_sys

PN_COMMON_OBJS =			\
	${PN_KERN_COMMON_OBJS}		\
	${PN_LIBKERN_COMMON_OBJS}	\
	${PN_NET_COMMON_OBJS}		\
	${PN_NETINET_COMMON_OBJS}	\
	${PN_OTHER_COMMON_OBJS}		\
	${PN_SYS_COMMON_OBJS}	

PN_COMMON_SRCS= ${PN_COMMON_OBJS:C/.o$/.c/}

SRCS=	${PN_COMMON_SRCS}
SRCS+=	${PN_MFILES}
SRCS+=	${PN_OPT_FILES}

CFLAGS+=	-I./include
CFLAGS+=	-I./include/opt
CFLAGS+=	-I${PREFIX}
CFLAGS+=	-I.
CFLAGS+=	-I${PREFIX}/contrib/altq
CFLAGS+=	-I${PREFIX}/contrib/pf
CFLAGS+=	-I/usr/include
CFLAGS+=	-D_KERNEL
CFLAGS+=	-DMUTEX_NOINLINE
CFLAGS+=	-DRWLOCK_NOINLINE
CFLAGS+=	-DSX_NOINLINE
CFLAGS+=	-DPLEBNET
CFLAGS+=	-DMAXUSERS=32
CFLAGS+=	-DINET=1
#CFLAGS+=	-fno-builtin
CFLAGS+=	-O0
CFLAGS+=	-g
CFLAGS+=	-nostdinc
NO_WERROR=	1
WARNS=2
CSTD="c99"
WFORMAT=0



AWK ?= awk

_ILINKS=@ machine
MACHINE_CPUARCH ?= ${MACHINE}
.if ${MACHINE} != ${MACHINE_CPUARCH}
_ILINKS+=${MACHINE_CPUARCH}
.endif

# newer versions of the kernel ...
.if ${MACHINE_CPUARCH} == "i386" || ${MACHINE_CPUARCH} == "amd64"
_ILINKS+=x86
.endif


all: ${_ILINKS}

beforedepend: ${_ILINKS}

# Ensure that the links exist without depending on it when it exists which
# causes all the modules to be rebuilt when the directory pointed to changes.
.for _link in ${_ILINKS}
.if !exists(${.OBJDIR}/${_link})
${OBJS}: ${_link}
.endif
.endfor

${_ILINKS}:
	@case ${.TARGET} in \
	machine) \
		path=${SYSDIR}/${MACHINE}/include ;; \
	@) \
		path=${SYSDIR} ;; \
	*) \
		path=${SYSDIR}/${.TARGET}/include ;; \
	esac ; \
	path=`(cd $$path && /bin/pwd)` ; \
	${ECHO} ${.TARGET} "->" $$path ; \
	ln -sf $$path ${.TARGET}


.if defined(KERNBUILDDIR)
.PATH: ${KERNBUILDDIR}
CFLAGS+=	-I${KERNBUILDDIR}
.for _src in ${SRCS:Mopt_*.h}
CLEANFILES+=	${_src}
.if !target(${_src})
${_src}:
	ln -sf ${KERNBUILDDIR}/${_src} ${.TARGET}
.endif
.endfor
.else
.for _src in ${SRCS:Mopt_*.h}
CLEANFILES+=	${_src}
.if !target(${_src})
${_src}:
	:> ${.TARGET}
.endif
.endfor
.endif

MFILES= kern/bus_if.m kern/device_if.m

.for _srcsrc in ${MFILES}
.for _ext in c h
.for _src in ${SRCS:M${_srcsrc:T:R}.${_ext}}
CLEANFILES+=	${_src}
.if !target(${_src})
.if !exists(@)
${_src}: @
.else
${_src}: @/tools/makeobjops.awk @/${_srcsrc}
.endif
	${AWK} -f @/tools/makeobjops.awk @/${_srcsrc} -${_ext}
.endif
.endfor # _src
.endfor # _ext
.endfor # _srcsrc

.include <bsd.dep.mk>

cleandepend: cleanilinks

cleanilinks:
	rm -f ${_ILINKS}

.if !exists(${.OBJDIR}/${DEPENDFILE})
${OBJS}: ${SRCS:M*.h}
.endif

.include <bsd.lib.mk>