/*	$OpenBSD: sunos_syscallargs.h,v 1.19 2004/07/09 23:56:33 millert Exp $	*/

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.16 2004/07/09 23:52:02 millert Exp 
 */

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register_t pad;						\
		struct { x datum; } le;					\
		struct {						\
			int8_t pad[ (sizeof (register_t) < sizeof (x))	\
				? 0					\
				: sizeof (register_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct sunos_sys_open_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct sunos_sys_wait4_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
};

struct sunos_sys_creat_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct sunos_sys_execv_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
};

struct sunos_sys_mknod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct sunos_sys_ostime_args {
	syscallarg(int) time;
};

struct sunos_sys_ptrace_args {
	syscallarg(int) req;
	syscallarg(pid_t) pid;
	syscallarg(caddr_t) addr;
	syscallarg(int) data;
	syscallarg(char *) addr2;
};

struct sunos_sys_access_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
};

struct sunos_sys_stat_args {
	syscallarg(char *) path;
	syscallarg(struct stat43 *) ub;
};

struct sunos_sys_lstat_args {
	syscallarg(char *) path;
	syscallarg(struct stat43 *) ub;
};

struct sunos_sys_otimes_args {
	syscallarg(struct tms *) tp;
};

struct sunos_sys_mctl_args {
	syscallarg(caddr_t) addr;
	syscallarg(int) len;
	syscallarg(int) func;
	syscallarg(void *) arg;
};

struct sunos_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(u_long) com;
	syscallarg(caddr_t) data;
};

struct sunos_sys_reboot_args {
	syscallarg(int) howto;
	syscallarg(char *) bootstr;
};

struct sunos_sys_execve_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct sunos_sys_mmap_args {
	syscallarg(caddr_t) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(long) pos;
};

struct sunos_sys_setpgrp_args {
	syscallarg(int) pid;
	syscallarg(int) pgid;
};

struct sunos_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct sunos_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) name;
	syscallarg(caddr_t) val;
	syscallarg(int) valsize;
};

struct sunos_sys_sigvec_args {
	syscallarg(int) signum;
	syscallarg(struct sigvec *) nsv;
	syscallarg(struct sigvec *) osv;
};

struct sunos_sys_sigreturn_args {
	syscallarg(struct sigcontext *) sigcntxp;
};

struct sunos_sys_getrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct orlimit *) rlp;
};

struct sunos_sys_setrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct orlimit *) rlp;
};

struct sunos_sys_nfssvc_args {
	syscallarg(int) fd;
};

struct sunos_sys_statfs_args {
	syscallarg(char *) path;
	syscallarg(struct sunos_statfs *) buf;
};

struct sunos_sys_fstatfs_args {
	syscallarg(int) fd;
	syscallarg(struct sunos_statfs *) buf;
};

struct sunos_sys_unmount_args {
	syscallarg(char *) path;
};

struct sunos_sys_quotactl_args {
	syscallarg(int) cmd;
	syscallarg(char *) special;
	syscallarg(int) uid;
	syscallarg(caddr_t) addr;
};

struct sunos_sys_exportfs_args {
	syscallarg(char *) path;
	syscallarg(char *) ex;
};

struct sunos_sys_mount_args {
	syscallarg(char *) type;
	syscallarg(char *) dir;
	syscallarg(int) flags;
	syscallarg(caddr_t) data;
};

struct sunos_sys_ustat_args {
	syscallarg(int) dev;
	syscallarg(struct sunos_ustat *) buf;
};

struct sunos_sys_auditsys_args {
	syscallarg(char *) record;
};

struct sunos_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(int) nbytes;
};

struct sunos_sys_fchroot_args {
	syscallarg(int) fd;
};

struct sunos_sys_sigpending_args {
	syscallarg(int *) mask;
};

struct sunos_sys_sysconf_args {
	syscallarg(int) name;
};

struct sunos_sys_uname_args {
	syscallarg(struct sunos_utsname *) name;
};

/*
 * System call prototypes.
 */

int	sys_nosys(struct proc *, void *, register_t *);
int	sys_exit(struct proc *, void *, register_t *);
int	sys_fork(struct proc *, void *, register_t *);
int	sys_read(struct proc *, void *, register_t *);
int	sys_write(struct proc *, void *, register_t *);
int	sunos_sys_open(struct proc *, void *, register_t *);
int	sys_close(struct proc *, void *, register_t *);
int	sunos_sys_wait4(struct proc *, void *, register_t *);
int	sunos_sys_creat(struct proc *, void *, register_t *);
int	sys_link(struct proc *, void *, register_t *);
int	sys_unlink(struct proc *, void *, register_t *);
int	sunos_sys_execv(struct proc *, void *, register_t *);
int	sys_chdir(struct proc *, void *, register_t *);
int	sunos_sys_mknod(struct proc *, void *, register_t *);
int	sys_chmod(struct proc *, void *, register_t *);
int	sys_lchown(struct proc *, void *, register_t *);
int	sys_obreak(struct proc *, void *, register_t *);
int	compat_43_sys_lseek(struct proc *, void *, register_t *);
int	sys_getpid(struct proc *, void *, register_t *);
int	sys_setuid(struct proc *, void *, register_t *);
int	sys_getuid(struct proc *, void *, register_t *);
int	sunos_sys_ostime(struct proc *, void *, register_t *);
#ifdef PTRACE
int	sunos_sys_ptrace(struct proc *, void *, register_t *);
#else
#endif
int	sunos_sys_access(struct proc *, void *, register_t *);
int	sys_sync(struct proc *, void *, register_t *);
int	sys_kill(struct proc *, void *, register_t *);
int	sunos_sys_stat(struct proc *, void *, register_t *);
int	sunos_sys_lstat(struct proc *, void *, register_t *);
int	sys_dup(struct proc *, void *, register_t *);
int	sys_opipe(struct proc *, void *, register_t *);
int	sunos_sys_otimes(struct proc *, void *, register_t *);
int	sys_profil(struct proc *, void *, register_t *);
int	sys_setgid(struct proc *, void *, register_t *);
int	sys_getgid(struct proc *, void *, register_t *);
#ifdef ACCOUNTING
int	sys_acct(struct proc *, void *, register_t *);
#else
#endif
int	sunos_sys_mctl(struct proc *, void *, register_t *);
int	sunos_sys_ioctl(struct proc *, void *, register_t *);
int	sunos_sys_reboot(struct proc *, void *, register_t *);
int	sys_symlink(struct proc *, void *, register_t *);
int	sys_readlink(struct proc *, void *, register_t *);
int	sunos_sys_execve(struct proc *, void *, register_t *);
int	sys_umask(struct proc *, void *, register_t *);
int	sys_chroot(struct proc *, void *, register_t *);
int	compat_43_sys_fstat(struct proc *, void *, register_t *);
int	compat_43_sys_getpagesize(struct proc *, void *, register_t *);
int	sys_msync(struct proc *, void *, register_t *);
int	sys_vfork(struct proc *, void *, register_t *);
int	sys_sbrk(struct proc *, void *, register_t *);
int	sys_sstk(struct proc *, void *, register_t *);
int	sunos_sys_mmap(struct proc *, void *, register_t *);
int	sys_ovadvise(struct proc *, void *, register_t *);
int	sys_munmap(struct proc *, void *, register_t *);
int	sys_mprotect(struct proc *, void *, register_t *);
int	sys_madvise(struct proc *, void *, register_t *);
int	sunos_sys_vhangup(struct proc *, void *, register_t *);
int	sys_mincore(struct proc *, void *, register_t *);
int	sys_getgroups(struct proc *, void *, register_t *);
int	sys_setgroups(struct proc *, void *, register_t *);
int	sys_getpgrp(struct proc *, void *, register_t *);
int	sunos_sys_setpgrp(struct proc *, void *, register_t *);
int	sys_setitimer(struct proc *, void *, register_t *);
int	compat_25_sys_swapon(struct proc *, void *, register_t *);
int	sys_getitimer(struct proc *, void *, register_t *);
int	compat_43_sys_gethostname(struct proc *, void *, register_t *);
int	compat_43_sys_sethostname(struct proc *, void *, register_t *);
int	compat_43_sys_getdtablesize(struct proc *, void *, register_t *);
int	sys_dup2(struct proc *, void *, register_t *);
int	sunos_sys_fcntl(struct proc *, void *, register_t *);
int	sys_select(struct proc *, void *, register_t *);
int	sys_fsync(struct proc *, void *, register_t *);
int	sys_setpriority(struct proc *, void *, register_t *);
int	sys_socket(struct proc *, void *, register_t *);
int	sys_connect(struct proc *, void *, register_t *);
int	compat_43_sys_accept(struct proc *, void *, register_t *);
int	sys_getpriority(struct proc *, void *, register_t *);
int	compat_43_sys_send(struct proc *, void *, register_t *);
int	compat_43_sys_recv(struct proc *, void *, register_t *);
int	sys_bind(struct proc *, void *, register_t *);
int	sunos_sys_setsockopt(struct proc *, void *, register_t *);
int	sys_listen(struct proc *, void *, register_t *);
int	sunos_sys_sigvec(struct proc *, void *, register_t *);
int	compat_43_sys_sigblock(struct proc *, void *, register_t *);
int	compat_43_sys_sigsetmask(struct proc *, void *, register_t *);
int	sys_sigsuspend(struct proc *, void *, register_t *);
int	compat_43_sys_sigstack(struct proc *, void *, register_t *);
int	compat_43_sys_recvmsg(struct proc *, void *, register_t *);
int	compat_43_sys_sendmsg(struct proc *, void *, register_t *);
int	sys_gettimeofday(struct proc *, void *, register_t *);
int	sys_getrusage(struct proc *, void *, register_t *);
int	sys_getsockopt(struct proc *, void *, register_t *);
int	sys_readv(struct proc *, void *, register_t *);
int	sys_writev(struct proc *, void *, register_t *);
int	sys_settimeofday(struct proc *, void *, register_t *);
int	sys_fchown(struct proc *, void *, register_t *);
int	sys_fchmod(struct proc *, void *, register_t *);
int	compat_43_sys_recvfrom(struct proc *, void *, register_t *);
int	sys_setreuid(struct proc *, void *, register_t *);
int	sys_setregid(struct proc *, void *, register_t *);
int	sys_rename(struct proc *, void *, register_t *);
int	compat_43_sys_truncate(struct proc *, void *, register_t *);
int	compat_43_sys_ftruncate(struct proc *, void *, register_t *);
int	sys_flock(struct proc *, void *, register_t *);
int	sys_sendto(struct proc *, void *, register_t *);
int	sys_shutdown(struct proc *, void *, register_t *);
int	sys_socketpair(struct proc *, void *, register_t *);
int	sys_mkdir(struct proc *, void *, register_t *);
int	sys_rmdir(struct proc *, void *, register_t *);
int	sys_utimes(struct proc *, void *, register_t *);
int	sunos_sys_sigreturn(struct proc *, void *, register_t *);
int	sys_adjtime(struct proc *, void *, register_t *);
int	compat_43_sys_getpeername(struct proc *, void *, register_t *);
int	compat_43_sys_gethostid(struct proc *, void *, register_t *);
int	sunos_sys_getrlimit(struct proc *, void *, register_t *);
int	sunos_sys_setrlimit(struct proc *, void *, register_t *);
int	compat_43_sys_killpg(struct proc *, void *, register_t *);
int	compat_43_sys_getsockname(struct proc *, void *, register_t *);
int	sys_poll(struct proc *, void *, register_t *);
#ifdef NFSSERVER
int	sunos_sys_nfssvc(struct proc *, void *, register_t *);
#else
#endif
int	sys_getdirentries(struct proc *, void *, register_t *);
int	sunos_sys_statfs(struct proc *, void *, register_t *);
int	sunos_sys_fstatfs(struct proc *, void *, register_t *);
int	sunos_sys_unmount(struct proc *, void *, register_t *);
#ifdef NFSCLIENT
int	async_daemon(struct proc *, void *, register_t *);
int	sys_getfh(struct proc *, void *, register_t *);
#else
#endif
int	compat_09_sys_getdomainname(struct proc *, void *, register_t *);
int	compat_09_sys_setdomainname(struct proc *, void *, register_t *);
int	sunos_sys_quotactl(struct proc *, void *, register_t *);
int	sunos_sys_exportfs(struct proc *, void *, register_t *);
int	sunos_sys_mount(struct proc *, void *, register_t *);
int	sunos_sys_ustat(struct proc *, void *, register_t *);
#ifdef SYSVSEM
int	compat_10_sys_semsys(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVMSG
int	compat_10_sys_msgsys(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVSHM
int	compat_10_sys_shmsys(struct proc *, void *, register_t *);
#else
#endif
int	sunos_sys_auditsys(struct proc *, void *, register_t *);
int	sunos_sys_getdents(struct proc *, void *, register_t *);
int	sys_setsid(struct proc *, void *, register_t *);
int	sys_fchdir(struct proc *, void *, register_t *);
int	sunos_sys_fchroot(struct proc *, void *, register_t *);
int	sunos_sys_sigpending(struct proc *, void *, register_t *);
int	sys_setpgid(struct proc *, void *, register_t *);
int	sys_pathconf(struct proc *, void *, register_t *);
int	sys_fpathconf(struct proc *, void *, register_t *);
int	sunos_sys_sysconf(struct proc *, void *, register_t *);
int	sunos_sys_uname(struct proc *, void *, register_t *);
