/*	$OpenBSD: linux_syscallargs.h,v 1.21 2000/04/04 05:32:17 jasoni Exp $	*/

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.21 2000/04/04 05:31:50 jasoni Exp 
 */

#define	syscallarg(x)	union { x datum; register_t pad; }

struct linux_sys_open_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct linux_sys_waitpid_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
};

struct linux_sys_creat_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct linux_sys_unlink_args {
	syscallarg(char *) path;
};

struct linux_sys_execve_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct linux_sys_chdir_args {
	syscallarg(char *) path;
};

struct linux_sys_time_args {
	syscallarg(linux_time_t *) t;
};

struct linux_sys_mknod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct linux_sys_chmod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct linux_sys_lchown_args {
	syscallarg(char *) path;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_break_args {
	syscallarg(char *) nsize;
};

struct linux_sys_mount_args {
	syscallarg(char *) specialfile;
	syscallarg(char *) dir;
	syscallarg(char *) filesystemtype;
	syscallarg(long) rwflag;
	syscallarg(void *) data;
};

struct linux_sys_umount_args {
	syscallarg(char *) specialfile;
};

struct linux_sys_stime_args {
	syscallarg(linux_time_t *) t;
};

struct linux_sys_alarm_args {
	syscallarg(unsigned int) secs;
};

struct linux_sys_utime_args {
	syscallarg(char *) path;
	syscallarg(struct linux_utimbuf *) times;
};

struct linux_sys_access_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
};

struct linux_sys_nice_args {
	syscallarg(int) incr;
};

struct linux_sys_kill_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
};

struct linux_sys_rename_args {
	syscallarg(char *) from;
	syscallarg(char *) to;
};

struct linux_sys_mkdir_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct linux_sys_rmdir_args {
	syscallarg(char *) path;
};

struct linux_sys_pipe_args {
	syscallarg(int *) pfds;
};

struct linux_sys_times_args {
	syscallarg(struct times *) tms;
};

struct linux_sys_brk_args {
	syscallarg(char *) nsize;
};

struct linux_sys_signal_args {
	syscallarg(int) sig;
	syscallarg(linux_handler_t) handler;
};

struct linux_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(u_long) com;
	syscallarg(caddr_t) data;
};

struct linux_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct linux_sys_oldolduname_args {
	syscallarg(struct linux_oldold_utsname *) up;
};

struct linux_sys_sigaction_args {
	syscallarg(int) signum;
	syscallarg(struct linux_old_sigaction *) nsa;
	syscallarg(struct linux_old_sigaction *) osa;
};

struct linux_sys_sigsetmask_args {
	syscallarg(linux_old_sigset_t) mask;
};

struct linux_sys_setreuid_args {
	syscallarg(int) ruid;
	syscallarg(int) euid;
};

struct linux_sys_setregid_args {
	syscallarg(int) rgid;
	syscallarg(int) egid;
};

struct linux_sys_sigsuspend_args {
	syscallarg(caddr_t) restart;
	syscallarg(int) oldmask;
	syscallarg(int) mask;
};

struct linux_sys_sigpending_args {
	syscallarg(linux_old_sigset_t *) mask;
};

struct linux_sys_oldselect_args {
	syscallarg(struct linux_select *) lsp;
};

struct linux_sys_symlink_args {
	syscallarg(char *) path;
	syscallarg(char *) to;
};

struct linux_sys_readlink_args {
	syscallarg(char *) name;
	syscallarg(char *) buf;
	syscallarg(int) count;
};

struct linux_sys_uselib_args {
	syscallarg(char *) path;
};

struct linux_sys_readdir_args {
	syscallarg(int) fd;
	syscallarg(caddr_t) dent;
	syscallarg(unsigned int) count;
};

struct linux_sys_mmap_args {
	syscallarg(struct linux_mmap *) lmp;
};

struct linux_sys_truncate_args {
	syscallarg(char *) path;
	syscallarg(long) length;
};

struct linux_sys_fchown_args {
	syscallarg(int) fd;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_statfs_args {
	syscallarg(char *) path;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_fstatfs_args {
	syscallarg(int) fd;
	syscallarg(struct linux_statfs *) sp;
};

struct linux_sys_ioperm_args {
	syscallarg(unsigned int) lo;
	syscallarg(unsigned int) hi;
	syscallarg(int) val;
};

struct linux_sys_socketcall_args {
	syscallarg(int) what;
	syscallarg(void *) args;
};

struct linux_sys_stat_args {
	syscallarg(char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_lstat_args {
	syscallarg(char *) path;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(struct linux_stat *) sp;
};

struct linux_sys_olduname_args {
	syscallarg(struct linux_old_utsname *) up;
};

struct linux_sys_iopl_args {
	syscallarg(int) level;
};

struct linux_sys_wait4_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
};

struct linux_sys_ipc_args {
	syscallarg(int) what;
	syscallarg(int) a1;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(caddr_t) ptr;
};

struct linux_sys_sigreturn_args {
	syscallarg(struct linux_sigcontext *) scp;
};

struct linux_sys_uname_args {
	syscallarg(struct linux_utsname *) up;
};

struct linux_sys_modify_ldt_args {
	syscallarg(int) func;
	syscallarg(void *) ptr;
	syscallarg(size_t) bytecount;
};

struct linux_sys_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(linux_old_sigset_t *) set;
	syscallarg(linux_old_sigset_t *) oset;
};

struct linux_sys_getpgid_args {
	syscallarg(int) pid;
};

struct linux_sys_personality_args {
	syscallarg(int) per;
};

struct linux_sys_llseek_args {
	syscallarg(int) fd;
	syscallarg(u_int32_t) ohigh;
	syscallarg(u_int32_t) olow;
	syscallarg(caddr_t) res;
	syscallarg(int) whence;
};

struct linux_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(void *) dirent;
	syscallarg(unsigned) count;
};

struct linux_sys_select_args {
	syscallarg(int) nfds;
	syscallarg(fd_set *) readfds;
	syscallarg(fd_set *) writefds;
	syscallarg(fd_set *) exceptfds;
	syscallarg(struct timeval *) timeout;
};

struct linux_sys_getsid_args {
	syscallarg(int) pid;
};

struct linux_sys_fdatasync_args {
	syscallarg(int) fd;
};

struct linux_sys___sysctl_args {
	syscallarg(struct linux___sysctl *) lsp;
};

struct linux_sys_mremap_args {
	syscallarg(void *) old_address;
	syscallarg(size_t) old_size;
	syscallarg(size_t) new_size;
	syscallarg(long) flags;
};

struct linux_sys_setresuid_args {
	syscallarg(uid_t) ruid;
	syscallarg(uid_t) euid;
	syscallarg(uid_t) suid;
};

struct linux_sys_getresuid_args {
	syscallarg(uid_t *) ruid;
	syscallarg(uid_t *) euid;
	syscallarg(uid_t *) suid;
};

struct linux_sys_setresgid_args {
	syscallarg(gid_t) rgid;
	syscallarg(gid_t) egid;
	syscallarg(gid_t) sgid;
};

struct linux_sys_getresgid_args {
	syscallarg(gid_t *) rgid;
	syscallarg(gid_t *) egid;
	syscallarg(gid_t *) sgid;
};

struct linux_sys_rt_sigreturn_args {
	syscallarg(struct linux_rt_sigframe *) sfp;
};

struct linux_sys_rt_sigaction_args {
	syscallarg(int) signum;
	syscallarg(struct linux_sigaction *) nsa;
	syscallarg(struct linux_sigaction *) osa;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(const linux_sigset_t *) set;
	syscallarg(linux_sigset_t *) oset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigpending_args {
	syscallarg(linux_sigset_t *) set;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_rt_sigsuspend_args {
	syscallarg(linux_sigset_t *) unewset;
	syscallarg(size_t) sigsetsize;
};

struct linux_sys_pread_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_pwrite_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(linux_off_t) offset;
};

struct linux_sys_chown_args {
	syscallarg(char *) path;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct linux_sys_getcwd_args {
	syscallarg(char *) bufp;
	syscallarg(size_t) length;
};

/*
 * System call prototypes.
 */

int	sys_nosys	__P((struct proc *, void *, register_t *));
int	sys_exit	__P((struct proc *, void *, register_t *));
int	sys_fork	__P((struct proc *, void *, register_t *));
int	sys_read	__P((struct proc *, void *, register_t *));
int	sys_write	__P((struct proc *, void *, register_t *));
int	linux_sys_open	__P((struct proc *, void *, register_t *));
int	sys_close	__P((struct proc *, void *, register_t *));
int	linux_sys_waitpid	__P((struct proc *, void *, register_t *));
int	linux_sys_creat	__P((struct proc *, void *, register_t *));
int	sys_link	__P((struct proc *, void *, register_t *));
int	linux_sys_unlink	__P((struct proc *, void *, register_t *));
int	linux_sys_execve	__P((struct proc *, void *, register_t *));
int	linux_sys_chdir	__P((struct proc *, void *, register_t *));
int	linux_sys_time	__P((struct proc *, void *, register_t *));
int	linux_sys_mknod	__P((struct proc *, void *, register_t *));
int	linux_sys_chmod	__P((struct proc *, void *, register_t *));
int	linux_sys_lchown	__P((struct proc *, void *, register_t *));
int	linux_sys_break	__P((struct proc *, void *, register_t *));
int	compat_43_sys_lseek	__P((struct proc *, void *, register_t *));
int	sys_getpid	__P((struct proc *, void *, register_t *));
int	linux_sys_mount	__P((struct proc *, void *, register_t *));
int	linux_sys_umount	__P((struct proc *, void *, register_t *));
int	sys_setuid	__P((struct proc *, void *, register_t *));
int	sys_getuid	__P((struct proc *, void *, register_t *));
int	linux_sys_stime	__P((struct proc *, void *, register_t *));
int	linux_sys_alarm	__P((struct proc *, void *, register_t *));
int	linux_sys_pause	__P((struct proc *, void *, register_t *));
int	linux_sys_utime	__P((struct proc *, void *, register_t *));
int	linux_sys_access	__P((struct proc *, void *, register_t *));
int	linux_sys_nice	__P((struct proc *, void *, register_t *));
int	sys_sync	__P((struct proc *, void *, register_t *));
int	linux_sys_kill	__P((struct proc *, void *, register_t *));
int	linux_sys_rename	__P((struct proc *, void *, register_t *));
int	linux_sys_mkdir	__P((struct proc *, void *, register_t *));
int	linux_sys_rmdir	__P((struct proc *, void *, register_t *));
int	sys_dup	__P((struct proc *, void *, register_t *));
int	linux_sys_pipe	__P((struct proc *, void *, register_t *));
int	linux_sys_times	__P((struct proc *, void *, register_t *));
int	linux_sys_brk	__P((struct proc *, void *, register_t *));
int	sys_setgid	__P((struct proc *, void *, register_t *));
int	sys_getgid	__P((struct proc *, void *, register_t *));
int	linux_sys_signal	__P((struct proc *, void *, register_t *));
int	sys_geteuid	__P((struct proc *, void *, register_t *));
int	sys_getegid	__P((struct proc *, void *, register_t *));
int	sys_acct	__P((struct proc *, void *, register_t *));
int	linux_sys_ioctl	__P((struct proc *, void *, register_t *));
int	linux_sys_fcntl	__P((struct proc *, void *, register_t *));
int	sys_setpgid	__P((struct proc *, void *, register_t *));
int	linux_sys_oldolduname	__P((struct proc *, void *, register_t *));
int	sys_umask	__P((struct proc *, void *, register_t *));
int	sys_chroot	__P((struct proc *, void *, register_t *));
int	sys_dup2	__P((struct proc *, void *, register_t *));
int	sys_getppid	__P((struct proc *, void *, register_t *));
int	sys_getpgrp	__P((struct proc *, void *, register_t *));
int	sys_setsid	__P((struct proc *, void *, register_t *));
int	linux_sys_sigaction	__P((struct proc *, void *, register_t *));
int	linux_sys_siggetmask	__P((struct proc *, void *, register_t *));
int	linux_sys_sigsetmask	__P((struct proc *, void *, register_t *));
int	linux_sys_setreuid	__P((struct proc *, void *, register_t *));
int	linux_sys_setregid	__P((struct proc *, void *, register_t *));
int	linux_sys_sigsuspend	__P((struct proc *, void *, register_t *));
int	linux_sys_sigpending	__P((struct proc *, void *, register_t *));
int	compat_43_sys_sethostname	__P((struct proc *, void *, register_t *));
int	compat_43_sys_setrlimit	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getrlimit	__P((struct proc *, void *, register_t *));
int	sys_getrusage	__P((struct proc *, void *, register_t *));
int	sys_gettimeofday	__P((struct proc *, void *, register_t *));
int	sys_settimeofday	__P((struct proc *, void *, register_t *));
int	sys_getgroups	__P((struct proc *, void *, register_t *));
int	sys_setgroups	__P((struct proc *, void *, register_t *));
int	linux_sys_oldselect	__P((struct proc *, void *, register_t *));
int	linux_sys_symlink	__P((struct proc *, void *, register_t *));
int	compat_43_sys_lstat	__P((struct proc *, void *, register_t *));
int	linux_sys_readlink	__P((struct proc *, void *, register_t *));
int	linux_sys_uselib	__P((struct proc *, void *, register_t *));
int	sys_swapon	__P((struct proc *, void *, register_t *));
int	sys_reboot	__P((struct proc *, void *, register_t *));
int	linux_sys_readdir	__P((struct proc *, void *, register_t *));
int	linux_sys_mmap	__P((struct proc *, void *, register_t *));
int	sys_munmap	__P((struct proc *, void *, register_t *));
int	linux_sys_truncate	__P((struct proc *, void *, register_t *));
int	compat_43_sys_ftruncate	__P((struct proc *, void *, register_t *));
int	sys_fchmod	__P((struct proc *, void *, register_t *));
int	linux_sys_fchown	__P((struct proc *, void *, register_t *));
int	sys_getpriority	__P((struct proc *, void *, register_t *));
int	sys_setpriority	__P((struct proc *, void *, register_t *));
int	sys_profil	__P((struct proc *, void *, register_t *));
int	linux_sys_statfs	__P((struct proc *, void *, register_t *));
int	linux_sys_fstatfs	__P((struct proc *, void *, register_t *));
#ifdef __i386__
int	linux_sys_ioperm	__P((struct proc *, void *, register_t *));
#else
#endif
int	linux_sys_socketcall	__P((struct proc *, void *, register_t *));
int	sys_setitimer	__P((struct proc *, void *, register_t *));
int	sys_getitimer	__P((struct proc *, void *, register_t *));
int	linux_sys_stat	__P((struct proc *, void *, register_t *));
int	linux_sys_lstat	__P((struct proc *, void *, register_t *));
int	linux_sys_fstat	__P((struct proc *, void *, register_t *));
int	linux_sys_olduname	__P((struct proc *, void *, register_t *));
#ifdef __i386__
int	linux_sys_iopl	__P((struct proc *, void *, register_t *));
#else
#endif
int	linux_sys_wait4	__P((struct proc *, void *, register_t *));
int	linux_sys_ipc	__P((struct proc *, void *, register_t *));
int	sys_fsync	__P((struct proc *, void *, register_t *));
int	linux_sys_sigreturn	__P((struct proc *, void *, register_t *));
int	compat_09_sys_setdomainname	__P((struct proc *, void *, register_t *));
int	linux_sys_uname	__P((struct proc *, void *, register_t *));
#ifdef __i386__
int	linux_sys_modify_ldt	__P((struct proc *, void *, register_t *));
#else
#endif
int	sys_mprotect	__P((struct proc *, void *, register_t *));
int	linux_sys_sigprocmask	__P((struct proc *, void *, register_t *));
int	linux_sys_getpgid	__P((struct proc *, void *, register_t *));
int	sys_fchdir	__P((struct proc *, void *, register_t *));
int	linux_sys_personality	__P((struct proc *, void *, register_t *));
int	linux_sys_llseek	__P((struct proc *, void *, register_t *));
int	linux_sys_getdents	__P((struct proc *, void *, register_t *));
int	linux_sys_select	__P((struct proc *, void *, register_t *));
int	sys_flock	__P((struct proc *, void *, register_t *));
int	sys_msync	__P((struct proc *, void *, register_t *));
int	sys_readv	__P((struct proc *, void *, register_t *));
int	sys_writev	__P((struct proc *, void *, register_t *));
int	linux_sys_getsid	__P((struct proc *, void *, register_t *));
int	linux_sys_fdatasync	__P((struct proc *, void *, register_t *));
int	linux_sys___sysctl	__P((struct proc *, void *, register_t *));
int	sys_mlock	__P((struct proc *, void *, register_t *));
int	sys_munlock	__P((struct proc *, void *, register_t *));
int	sys_nanosleep	__P((struct proc *, void *, register_t *));
int	linux_sys_mremap	__P((struct proc *, void *, register_t *));
int	linux_sys_setresuid	__P((struct proc *, void *, register_t *));
int	linux_sys_getresuid	__P((struct proc *, void *, register_t *));
int	sys_poll	__P((struct proc *, void *, register_t *));
int	linux_sys_setresgid	__P((struct proc *, void *, register_t *));
int	linux_sys_getresgid	__P((struct proc *, void *, register_t *));
int	linux_sys_rt_sigreturn	__P((struct proc *, void *, register_t *));
int	linux_sys_rt_sigaction	__P((struct proc *, void *, register_t *));
int	linux_sys_rt_sigprocmask	__P((struct proc *, void *, register_t *));
int	linux_sys_rt_sigpending	__P((struct proc *, void *, register_t *));
int	linux_sys_rt_sigsuspend	__P((struct proc *, void *, register_t *));
int	linux_sys_pread	__P((struct proc *, void *, register_t *));
int	linux_sys_pwrite	__P((struct proc *, void *, register_t *));
int	linux_sys_chown	__P((struct proc *, void *, register_t *));
int	linux_sys_getcwd	__P((struct proc *, void *, register_t *));
int	sys_vfork	__P((struct proc *, void *, register_t *));
