/*	$OpenBSD: freebsd_sysent.c,v 1.34 2007/11/27 18:05:59 art Exp $	*/

/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.31 2007/04/05 15:33:42 tedu Exp 
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <compat/freebsd/freebsd_signal.h>
#include <compat/freebsd/freebsd_syscallargs.h>
#include <machine/freebsd_machdep.h>

#define	s(type)	sizeof(type)

struct sysent freebsd_sysent[] = {
	{ 0, 0, 0,
	    sys_nosys },			/* 0 = syscall */
	{ 1, s(struct sys_exit_args), 0,
	    sys_exit },				/* 1 = exit */
	{ 0, 0, 0,
	    sys_fork },				/* 2 = fork */
	{ 3, s(struct sys_read_args), 0,
	    sys_read },				/* 3 = read */
	{ 3, s(struct sys_write_args), 0,
	    sys_write },			/* 4 = write */
	{ 3, s(struct freebsd_sys_open_args), 0,
	    freebsd_sys_open },			/* 5 = open */
	{ 1, s(struct sys_close_args), 0,
	    sys_close },			/* 6 = close */
	{ 4, s(struct sys_wait4_args), 0,
	    sys_wait4 },			/* 7 = wait4 */
	{ 2, s(struct compat_43_freebsd_sys_creat_args), 0,
	    compat_43_freebsd_sys_creat },	/* 8 = ocreat */
	{ 2, s(struct freebsd_sys_link_args), 0,
	    freebsd_sys_link },			/* 9 = link */
	{ 1, s(struct freebsd_sys_unlink_args), 0,
	    freebsd_sys_unlink },		/* 10 = unlink */
	{ 0, 0, 0,
	    sys_nosys },			/* 11 = obsolete execv */
	{ 1, s(struct freebsd_sys_chdir_args), 0,
	    freebsd_sys_chdir },		/* 12 = chdir */
	{ 1, s(struct sys_fchdir_args), 0,
	    sys_fchdir },			/* 13 = fchdir */
	{ 3, s(struct freebsd_sys_mknod_args), 0,
	    freebsd_sys_mknod },		/* 14 = mknod */
	{ 2, s(struct freebsd_sys_chmod_args), 0,
	    freebsd_sys_chmod },		/* 15 = chmod */
	{ 3, s(struct freebsd_sys_chown_args), 0,
	    freebsd_sys_chown },		/* 16 = chown */
	{ 1, s(struct sys_obreak_args), 0,
	    sys_obreak },			/* 17 = break */
	{ 3, s(struct freebsd_sys_getfsstat_args), 0,
	    freebsd_sys_getfsstat },		/* 18 = getfsstat */
	{ 3, s(struct compat_43_sys_lseek_args), 0,
	    compat_43_sys_lseek },		/* 19 = olseek */
	{ 0, 0, 0,
	    sys_getpid },			/* 20 = getpid */
	{ 4, s(struct freebsd_sys_mount_args), 0,
	    freebsd_sys_mount },		/* 21 = mount */
	{ 2, s(struct freebsd_sys_unmount_args), 0,
	    freebsd_sys_unmount },		/* 22 = unmount */
	{ 1, s(struct sys_setuid_args), 0,
	    sys_setuid },			/* 23 = setuid */
	{ 0, 0, 0,
	    sys_getuid },			/* 24 = getuid */
	{ 0, 0, 0,
	    sys_geteuid },			/* 25 = geteuid */
#ifdef PTRACE
	{ 4, s(struct freebsd_sys_ptrace_args), 0,
	    freebsd_sys_ptrace },		/* 26 = ptrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 26 = unimplemented ptrace */
#endif
	{ 3, s(struct sys_recvmsg_args), 0,
	    sys_recvmsg },			/* 27 = recvmsg */
	{ 3, s(struct sys_sendmsg_args), 0,
	    sys_sendmsg },			/* 28 = sendmsg */
	{ 6, s(struct sys_recvfrom_args), 0,
	    sys_recvfrom },			/* 29 = recvfrom */
	{ 3, s(struct sys_accept_args), 0,
	    sys_accept },			/* 30 = accept */
	{ 3, s(struct sys_getpeername_args), 0,
	    sys_getpeername },			/* 31 = getpeername */
	{ 3, s(struct sys_getsockname_args), 0,
	    sys_getsockname },			/* 32 = getsockname */
	{ 2, s(struct freebsd_sys_access_args), 0,
	    freebsd_sys_access },		/* 33 = access */
	{ 2, s(struct freebsd_sys_chflags_args), 0,
	    freebsd_sys_chflags },		/* 34 = chflags */
	{ 2, s(struct sys_fchflags_args), 0,
	    sys_fchflags },			/* 35 = fchflags */
	{ 0, 0, 0,
	    sys_sync },				/* 36 = sync */
	{ 2, s(struct sys_kill_args), 0,
	    sys_kill },				/* 37 = kill */
	{ 2, s(struct compat_43_freebsd_sys_stat_args), 0,
	    compat_43_freebsd_sys_stat },	/* 38 = stat43 */
	{ 0, 0, 0,
	    sys_getppid },			/* 39 = getppid */
	{ 2, s(struct compat_43_freebsd_sys_lstat_args), 0,
	    compat_43_freebsd_sys_lstat },	/* 40 = lstat43 */
	{ 1, s(struct sys_dup_args), 0,
	    sys_dup },				/* 41 = dup */
	{ 0, 0, 0,
	    sys_opipe },			/* 42 = opipe */
	{ 0, 0, 0,
	    sys_getegid },			/* 43 = getegid */
	{ 4, s(struct sys_profil_args), 0,
	    sys_profil },			/* 44 = profil */
#ifdef KTRACE
	{ 4, s(struct sys_ktrace_args), 0,
	    sys_ktrace },			/* 45 = ktrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 45 = unimplemented ktrace */
#endif
	{ 3, s(struct sys_sigaction_args), 0,
	    sys_sigaction },			/* 46 = sigaction */
	{ 0, 0, 0,
	    sys_getgid },			/* 47 = getgid */
	{ 2, s(struct sys_sigprocmask_args), 0,
	    sys_sigprocmask },			/* 48 = sigprocmask */
	{ 2, s(struct sys_getlogin_args), 0,
	    sys_getlogin },			/* 49 = getlogin */
	{ 1, s(struct sys_setlogin_args), 0,
	    sys_setlogin },			/* 50 = setlogin */
#ifdef ACCOUNTING
	{ 1, s(struct sys_acct_args), 0,
	    sys_acct },				/* 51 = acct */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 51 = unimplemented acct */
#endif
	{ 0, 0, 0,
	    sys_sigpending },			/* 52 = sigpending */
	{ 2, s(struct sys_sigaltstack_args), 0,
	    sys_sigaltstack },			/* 53 = sigaltstack */
	{ 3, s(struct freebsd_sys_ioctl_args), 0,
	    freebsd_sys_ioctl },		/* 54 = ioctl */
	{ 1, s(struct sys_reboot_args), 0,
	    sys_reboot },			/* 55 = reboot */
	{ 1, s(struct freebsd_sys_revoke_args), 0,
	    freebsd_sys_revoke },		/* 56 = revoke */
	{ 2, s(struct freebsd_sys_symlink_args), 0,
	    freebsd_sys_symlink },		/* 57 = symlink */
	{ 3, s(struct freebsd_sys_readlink_args), 0,
	    freebsd_sys_readlink },		/* 58 = readlink */
	{ 3, s(struct freebsd_sys_execve_args), 0,
	    freebsd_sys_execve },		/* 59 = execve */
	{ 1, s(struct sys_umask_args), 0,
	    sys_umask },			/* 60 = umask */
	{ 1, s(struct freebsd_sys_chroot_args), 0,
	    freebsd_sys_chroot },		/* 61 = chroot */
	{ 2, s(struct compat_43_sys_fstat_args), 0,
	    compat_43_sys_fstat },		/* 62 = fstat43 */
	{ 4, s(struct compat_43_sys_getkerninfo_args), 0,
	    compat_43_sys_getkerninfo },	/* 63 = ogetkerninfo */
	{ 0, 0, 0,
	    compat_43_sys_getpagesize },	/* 64 = ogetpagesize */
	{ 3, s(struct sys_msync_args), 0,
	    sys_msync },			/* 65 = msync */
	{ 0, 0, 0,
	    sys_vfork },			/* 66 = vfork */
	{ 0, 0, 0,
	    sys_nosys },			/* 67 = obsolete vread */
	{ 0, 0, 0,
	    sys_nosys },			/* 68 = obsolete vwrite */
	{ 1, s(struct sys_sbrk_args), 0,
	    sys_sbrk },				/* 69 = sbrk */
	{ 1, s(struct sys_sstk_args), 0,
	    sys_sstk },				/* 70 = sstk */
	{ 6, s(struct compat_43_sys_mmap_args), 0,
	    compat_43_sys_mmap },		/* 71 = ommap */
	{ 1, s(struct sys_ovadvise_args), 0,
	    sys_ovadvise },			/* 72 = vadvise */
	{ 2, s(struct sys_munmap_args), 0,
	    sys_munmap },			/* 73 = munmap */
	{ 3, s(struct sys_mprotect_args), 0,
	    sys_mprotect },			/* 74 = mprotect */
	{ 3, s(struct freebsd_sys_madvise_args), 0,
	    freebsd_sys_madvise },		/* 75 = madvise */
	{ 0, 0, 0,
	    sys_nosys },			/* 76 = obsolete vhangup */
	{ 0, 0, 0,
	    sys_nosys },			/* 77 = obsolete vlimit */
	{ 3, s(struct sys_mincore_args), 0,
	    sys_mincore },			/* 78 = mincore */
	{ 2, s(struct sys_getgroups_args), 0,
	    sys_getgroups },			/* 79 = getgroups */
	{ 2, s(struct sys_setgroups_args), 0,
	    sys_setgroups },			/* 80 = setgroups */
	{ 0, 0, 0,
	    sys_getpgrp },			/* 81 = getpgrp */
	{ 2, s(struct sys_setpgid_args), 0,
	    sys_setpgid },			/* 82 = setpgid */
	{ 3, s(struct sys_setitimer_args), 0,
	    sys_setitimer },			/* 83 = setitimer */
	{ 0, 0, 0,
	    compat_43_sys_wait },		/* 84 = owait */
	{ 1, s(struct compat_25_sys_swapon_args), 0,
	    compat_25_sys_swapon },		/* 85 = swapon */
	{ 2, s(struct sys_getitimer_args), 0,
	    sys_getitimer },			/* 86 = getitimer */
	{ 2, s(struct compat_43_sys_gethostname_args), 0,
	    compat_43_sys_gethostname },	/* 87 = ogethostname */
	{ 2, s(struct compat_43_sys_sethostname_args), 0,
	    compat_43_sys_sethostname },	/* 88 = osethostname */
	{ 0, 0, 0,
	    compat_43_sys_getdtablesize },	/* 89 = ogetdtablesize */
	{ 2, s(struct sys_dup2_args), 0,
	    sys_dup2 },				/* 90 = dup2 */
	{ 0, 0, 0,
	    sys_nosys },			/* 91 = unimplemented getdopt */
	{ 3, s(struct freebsd_sys_fcntl_args), 0,
	    freebsd_sys_fcntl },		/* 92 = fcntl */
	{ 5, s(struct sys_select_args), 0,
	    sys_select },			/* 93 = select */
	{ 0, 0, 0,
	    sys_nosys },			/* 94 = unimplemented setdopt */
	{ 1, s(struct sys_fsync_args), 0,
	    sys_fsync },			/* 95 = fsync */
	{ 3, s(struct sys_setpriority_args), 0,
	    sys_setpriority },			/* 96 = setpriority */
	{ 3, s(struct sys_socket_args), 0,
	    sys_socket },			/* 97 = socket */
	{ 3, s(struct sys_connect_args), 0,
	    sys_connect },			/* 98 = connect */
	{ 3, s(struct compat_43_sys_accept_args), 0,
	    compat_43_sys_accept },		/* 99 = oaccept */
	{ 2, s(struct sys_getpriority_args), 0,
	    sys_getpriority },			/* 100 = getpriority */
	{ 4, s(struct compat_43_sys_send_args), 0,
	    compat_43_sys_send },		/* 101 = osend */
	{ 4, s(struct compat_43_sys_recv_args), 0,
	    compat_43_sys_recv },		/* 102 = orecv */
	{ 1, s(struct freebsd_sys_sigreturn_args), 0,
	    freebsd_sys_sigreturn },		/* 103 = sigreturn */
	{ 3, s(struct sys_bind_args), 0,
	    sys_bind },				/* 104 = bind */
	{ 5, s(struct sys_setsockopt_args), 0,
	    sys_setsockopt },			/* 105 = setsockopt */
	{ 2, s(struct sys_listen_args), 0,
	    sys_listen },			/* 106 = listen */
	{ 0, 0, 0,
	    sys_nosys },			/* 107 = obsolete vtimes */
	{ 3, s(struct compat_43_sys_sigvec_args), 0,
	    compat_43_sys_sigvec },		/* 108 = osigvec */
	{ 1, s(struct compat_43_sys_sigblock_args), 0,
	    compat_43_sys_sigblock },		/* 109 = osigblock */
	{ 1, s(struct compat_43_sys_sigsetmask_args), 0,
	    compat_43_sys_sigsetmask },		/* 110 = osigsetmask */
	{ 1, s(struct sys_sigsuspend_args), 0,
	    sys_sigsuspend },			/* 111 = sigsuspend */
	{ 2, s(struct compat_43_sys_sigstack_args), 0,
	    compat_43_sys_sigstack },		/* 112 = osigstack */
	{ 3, s(struct compat_43_sys_recvmsg_args), 0,
	    compat_43_sys_recvmsg },		/* 113 = orecvmsg */
	{ 3, s(struct compat_43_sys_sendmsg_args), 0,
	    compat_43_sys_sendmsg },		/* 114 = osendmsg */
#ifdef TRACE
	{ 2, s(struct sys_vtrace_args), 0,
	    sys_vtrace },			/* 115 = vtrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 115 = obsolete vtrace */
#endif
	{ 2, s(struct sys_gettimeofday_args), 0,
	    sys_gettimeofday },			/* 116 = gettimeofday */
	{ 2, s(struct sys_getrusage_args), 0,
	    sys_getrusage },			/* 117 = getrusage */
	{ 5, s(struct sys_getsockopt_args), 0,
	    sys_getsockopt },			/* 118 = getsockopt */
	{ 0, 0, 0,
	    sys_nosys },			/* 119 = obsolete resuba */
	{ 3, s(struct sys_readv_args), 0,
	    sys_readv },			/* 120 = readv */
	{ 3, s(struct sys_writev_args), 0,
	    sys_writev },			/* 121 = writev */
	{ 2, s(struct sys_settimeofday_args), 0,
	    sys_settimeofday },			/* 122 = settimeofday */
	{ 3, s(struct sys_fchown_args), 0,
	    sys_fchown },			/* 123 = fchown */
	{ 2, s(struct sys_fchmod_args), 0,
	    sys_fchmod },			/* 124 = fchmod */
	{ 6, s(struct compat_43_sys_recvfrom_args), 0,
	    compat_43_sys_recvfrom },		/* 125 = orecvfrom */
	{ 2, s(struct sys_setreuid_args), 0,
	    sys_setreuid },			/* 126 = setreuid */
	{ 2, s(struct sys_setregid_args), 0,
	    sys_setregid },			/* 127 = setregid */
	{ 2, s(struct freebsd_sys_rename_args), 0,
	    freebsd_sys_rename },		/* 128 = rename */
	{ 2, s(struct compat_43_freebsd_sys_truncate_args), 0,
	    compat_43_freebsd_sys_truncate },	/* 129 = otruncate */
	{ 2, s(struct compat_43_sys_ftruncate_args), 0,
	    compat_43_sys_ftruncate },		/* 130 = oftruncate */
	{ 2, s(struct sys_flock_args), 0,
	    sys_flock },			/* 131 = flock */
	{ 2, s(struct freebsd_sys_mkfifo_args), 0,
	    freebsd_sys_mkfifo },		/* 132 = mkfifo */
	{ 6, s(struct sys_sendto_args), 0,
	    sys_sendto },			/* 133 = sendto */
	{ 2, s(struct sys_shutdown_args), 0,
	    sys_shutdown },			/* 134 = shutdown */
	{ 4, s(struct sys_socketpair_args), 0,
	    sys_socketpair },			/* 135 = socketpair */
	{ 2, s(struct freebsd_sys_mkdir_args), 0,
	    freebsd_sys_mkdir },		/* 136 = mkdir */
	{ 1, s(struct freebsd_sys_rmdir_args), 0,
	    freebsd_sys_rmdir },		/* 137 = rmdir */
	{ 2, s(struct sys_utimes_args), 0,
	    sys_utimes },			/* 138 = utimes */
	{ 0, 0, 0,
	    sys_nosys },			/* 139 = obsolete 4.2 sigreturn */
	{ 2, s(struct sys_adjtime_args), 0,
	    sys_adjtime },			/* 140 = adjtime */
	{ 3, s(struct compat_43_sys_getpeername_args), 0,
	    compat_43_sys_getpeername },	/* 141 = ogetpeername */
	{ 0, 0, 0,
	    compat_43_sys_gethostid },		/* 142 = ogethostid */
	{ 1, s(struct compat_43_sys_sethostid_args), 0,
	    compat_43_sys_sethostid },		/* 143 = osethostid */
	{ 2, s(struct compat_43_sys_getrlimit_args), 0,
	    compat_43_sys_getrlimit },		/* 144 = ogetrlimit */
	{ 2, s(struct compat_43_sys_setrlimit_args), 0,
	    compat_43_sys_setrlimit },		/* 145 = osetrlimit */
	{ 2, s(struct compat_43_sys_killpg_args), 0,
	    compat_43_sys_killpg },		/* 146 = okillpg */
	{ 0, 0, 0,
	    sys_setsid },			/* 147 = setsid */
	{ 4, s(struct sys_quotactl_args), 0,
	    sys_quotactl },			/* 148 = quotactl */
	{ 0, 0, 0,
	    compat_43_sys_quota },		/* 149 = oquota */
	{ 3, s(struct compat_43_sys_getsockname_args), 0,
	    compat_43_sys_getsockname },	/* 150 = ogetsockname */
	{ 0, 0, 0,
	    sys_nosys },			/* 151 = unimplemented sem_lock */
	{ 0, 0, 0,
	    sys_nosys },			/* 152 = unimplemented sem_wakeup */
	{ 0, 0, 0,
	    sys_nosys },			/* 153 = unimplemented asyncdaemon */
	{ 0, 0, 0,
	    sys_nosys },			/* 154 = unimplemented */
#if defined(NFSCLIENT) || defined(NFSSERVER)
	{ 2, s(struct sys_nfssvc_args), 0,
	    sys_nfssvc },			/* 155 = nfssvc */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 155 = unimplemented */
#endif
	{ 4, s(struct compat_43_sys_getdirentries_args), 0,
	    compat_43_sys_getdirentries },	/* 156 = ogetdirentries */
	{ 2, s(struct freebsd_sys_statfs_args), 0,
	    freebsd_sys_statfs },		/* 157 = statfs */
	{ 2, s(struct freebsd_sys_fstatfs_args), 0,
	    freebsd_sys_fstatfs },		/* 158 = fstatfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 159 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 160 = unimplemented */
#ifdef NFSCLIENT
	{ 2, s(struct freebsd_sys_getfh_args), 0,
	    freebsd_sys_getfh },		/* 161 = getfh */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 161 = unimplemented getfh */
#endif
	{ 2, s(struct compat_09_sys_getdomainname_args), 0,
	    compat_09_sys_getdomainname },	/* 162 = getdomainname */
	{ 2, s(struct compat_09_sys_setdomainname_args), 0,
	    compat_09_sys_setdomainname },	/* 163 = setdomainname */
	{ 1, s(struct compat_09_sys_uname_args), 0,
	    compat_09_sys_uname },		/* 164 = uname */
	{ 2, s(struct sys_sysarch_args), 0,
	    sys_sysarch },			/* 165 = sysarch */
	{ 3, s(struct freebsd_sys_rtprio_args), 0,
	    freebsd_sys_rtprio },		/* 166 = rtprio */
	{ 0, 0, 0,
	    sys_nosys },			/* 167 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 168 = unimplemented */
#if defined(SYSVSEM) && !defined(alpha)
	{ 5, s(struct compat_10_sys_semsys_args), 0,
	    compat_10_sys_semsys },		/* 169 = semsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 169 = unimplemented 1.0 semsys */
#endif
#if defined(SYSVMSG) && !defined(alpha)
	{ 6, s(struct compat_10_sys_msgsys_args), 0,
	    compat_10_sys_msgsys },		/* 170 = msgsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 170 = unimplemented 1.0 msgsys */
#endif
#if defined(SYSVSHM) && !defined(alpha)
	{ 4, s(struct compat_10_sys_shmsys_args), 0,
	    compat_10_sys_shmsys },		/* 171 = shmsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 171 = unimplemented 1.0 shmsys */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 172 = unimplemented */
	{ 5, s(struct sys_pread_args), 0,
	    sys_pread },			/* 173 = pread */
	{ 5, s(struct sys_pwrite_args), 0,
	    sys_pwrite },			/* 174 = pwrite */
	{ 0, 0, 0,
	    sys_nosys },			/* 175 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 176 = unimplemented ntp_adjtime */
	{ 0, 0, 0,
	    sys_nosys },			/* 177 = unimplemented sfork */
	{ 0, 0, 0,
	    sys_nosys },			/* 178 = unimplemented getdescriptor */
	{ 0, 0, 0,
	    sys_nosys },			/* 179 = unimplemented setdescriptor */
	{ 0, 0, 0,
	    sys_nosys },			/* 180 = unimplemented */
	{ 1, s(struct sys_setgid_args), 0,
	    sys_setgid },			/* 181 = setgid */
	{ 1, s(struct sys_setegid_args), 0,
	    sys_setegid },			/* 182 = setegid */
	{ 1, s(struct sys_seteuid_args), 0,
	    sys_seteuid },			/* 183 = seteuid */
	{ 0, 0, 0,
	    sys_nosys },			/* 184 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 185 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 186 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 187 = unimplemented */
	{ 2, s(struct freebsd_sys_stat_args), 0,
	    freebsd_sys_stat },			/* 188 = stat */
	{ 2, s(struct compat_35_sys_fstat_args), 0,
	    compat_35_sys_fstat },		/* 189 = fstat */
	{ 2, s(struct freebsd_sys_lstat_args), 0,
	    freebsd_sys_lstat },		/* 190 = lstat */
	{ 2, s(struct freebsd_sys_pathconf_args), 0,
	    freebsd_sys_pathconf },		/* 191 = pathconf */
	{ 2, s(struct sys_fpathconf_args), 0,
	    sys_fpathconf },			/* 192 = fpathconf */
	{ 0, 0, 0,
	    sys_nosys },			/* 193 = unimplemented */
	{ 2, s(struct sys_getrlimit_args), 0,
	    sys_getrlimit },			/* 194 = getrlimit */
	{ 2, s(struct sys_setrlimit_args), 0,
	    sys_setrlimit },			/* 195 = setrlimit */
	{ 4, s(struct sys_getdirentries_args), 0,
	    sys_getdirentries },		/* 196 = getdirentries */
	{ 7, s(struct freebsd_sys_mmap_args), 0,
	    freebsd_sys_mmap },			/* 197 = mmap */
	{ 0, 0, 0,
	    sys_nosys },			/* 198 = __syscall */
	{ 4, s(struct sys_lseek_args), 0,
	    sys_lseek },			/* 199 = lseek */
	{ 3, s(struct freebsd_sys_truncate_args), 0,
	    freebsd_sys_truncate },		/* 200 = truncate */
	{ 3, s(struct sys_ftruncate_args), 0,
	    sys_ftruncate },			/* 201 = ftruncate */
	{ 6, s(struct sys___sysctl_args), 0,
	    sys___sysctl },			/* 202 = __sysctl */
	{ 2, s(struct sys_mlock_args), 0,
	    sys_mlock },			/* 203 = mlock */
	{ 2, s(struct sys_munlock_args), 0,
	    sys_munlock },			/* 204 = munlock */
	{ 0, 0, 0,
	    sys_nosys },			/* 205 = unimplemented sys_undelete */
	{ 0, 0, 0,
	    sys_nosys },			/* 206 = unimplemented futimes */
	{ 1, s(struct sys_getpgid_args), 0,
	    sys_getpgid },			/* 207 = getpgid */
	{ 0, 0, 0,
	    sys_nosys },			/* 208 = unimplemented reboot */
	{ 3, s(struct sys_poll_args), 0,
	    sys_poll },				/* 209 = poll */
	{ 0, 0, 0,
	    sys_nosys },			/* 210 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 211 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 212 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 213 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 214 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 215 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 216 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 217 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 218 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 219 = unimplemented */
#ifdef SYSVSEM
	{ 4, s(struct sys___semctl_args), 0,
	    sys___semctl },			/* 220 = __semctl */
	{ 3, s(struct sys_semget_args), 0,
	    sys_semget },			/* 221 = semget */
	{ 3, s(struct sys_semop_args), 0,
	    sys_semop },			/* 222 = semop */
	{ 0, 0, 0,
	    sys_nosys },			/* 223 = obsolete sys_semconfig */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 220 = unimplemented sys___semctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 221 = unimplemented sys_semget */
	{ 0, 0, 0,
	    sys_nosys },			/* 222 = unimplemented sys_semop */
	{ 0, 0, 0,
	    sys_nosys },			/* 223 = unimplemented sys_semconfig */
#endif
#ifdef SYSVMSG
	{ 3, s(struct sys_msgctl_args), 0,
	    sys_msgctl },			/* 224 = msgctl */
	{ 2, s(struct sys_msgget_args), 0,
	    sys_msgget },			/* 225 = msgget */
	{ 4, s(struct sys_msgsnd_args), 0,
	    sys_msgsnd },			/* 226 = msgsnd */
	{ 5, s(struct sys_msgrcv_args), 0,
	    sys_msgrcv },			/* 227 = msgrcv */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 224 = unimplemented sys_msgctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 225 = unimplemented sys_msgget */
	{ 0, 0, 0,
	    sys_nosys },			/* 226 = unimplemented sys_msgsnd */
	{ 0, 0, 0,
	    sys_nosys },			/* 227 = unimplemented sys_msgrcv */
#endif
#ifdef SYSVSHM
	{ 3, s(struct sys_shmat_args), 0,
	    sys_shmat },			/* 228 = shmat */
	{ 3, s(struct sys_shmctl_args), 0,
	    sys_shmctl },			/* 229 = shmctl */
	{ 1, s(struct sys_shmdt_args), 0,
	    sys_shmdt },			/* 230 = shmdt */
	{ 3, s(struct sys_shmget_args), 0,
	    sys_shmget },			/* 231 = shmget */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 228 = unimplemented sys_shmat */
	{ 0, 0, 0,
	    sys_nosys },			/* 229 = unimplemented sys_shmctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 230 = unimplemented sys_shmdt */
	{ 0, 0, 0,
	    sys_nosys },			/* 231 = unimplemented sys_shmget */
#endif
	{ 2, s(struct sys_clock_gettime_args), 0,
	    sys_clock_gettime },		/* 232 = clock_gettime */
	{ 0, 0, 0,
	    sys_nosys },			/* 233 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 234 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 235 = unimplemented timer_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 236 = unimplemented timer_delete */
	{ 0, 0, 0,
	    sys_nosys },			/* 237 = unimplemented timer_settime */
	{ 0, 0, 0,
	    sys_nosys },			/* 238 = unimplemented timer_gettime */
	{ 0, 0, 0,
	    sys_nosys },			/* 239 = unimplemented timer_getoverrun */
	{ 2, s(struct sys_nanosleep_args), 0,
	    sys_nanosleep },			/* 240 = nanosleep */
	{ 0, 0, 0,
	    sys_nosys },			/* 241 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 242 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 243 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 244 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 245 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 246 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 247 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 248 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 249 = unimplemented */
	{ 3, s(struct sys_minherit_args), 0,
	    sys_minherit },			/* 250 = minherit */
	{ 1, s(struct sys_rfork_args), 0,
	    sys_rfork },			/* 251 = rfork */
	{ 3, s(struct freebsd_sys_poll2_args), 0,
	    freebsd_sys_poll2 },		/* 252 = poll2 */
	{ 0, 0, 0,
	    sys_issetugid },			/* 253 = issetugid */
	{ 3, s(struct sys_lchown_args), 0,
	    sys_lchown },			/* 254 = lchown */
	{ 0, 0, 0,
	    sys_nosys },			/* 255 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 256 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 257 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 258 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 259 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 260 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 261 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 262 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 263 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 264 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 265 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 266 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 267 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 268 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 269 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 270 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 271 = unimplemented */
	{ 3, s(struct freebsd_sys_getdents_args), 0,
	    freebsd_sys_getdents },		/* 272 = getdents */
	{ 0, 0, 0,
	    sys_nosys },			/* 273 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 274 = unimplemented lchmod */
	{ 0, 0, 0,
	    sys_nosys },			/* 275 = unimplemented lchown */
	{ 0, 0, 0,
	    sys_nosys },			/* 276 = unimplemented lutimes */
	{ 0, 0, 0,
	    sys_nosys },			/* 277 = unimplemented msync */
	{ 0, 0, 0,
	    sys_nosys },			/* 278 = unimplemented stat */
	{ 0, 0, 0,
	    sys_nosys },			/* 279 = unimplemented fstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 280 = unimplemented lstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 281 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 282 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 283 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 284 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 285 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 286 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 287 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 288 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 289 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 290 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 291 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 292 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 293 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 294 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 295 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 296 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 297 = unimplemented fhstatfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 298 = unimplemented fhopen */
	{ 0, 0, 0,
	    sys_nosys },			/* 299 = unimplemented fhstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 300 = unimplemented modnext */
	{ 0, 0, 0,
	    sys_nosys },			/* 301 = unimplemented modstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 302 = unimplemented modfnext */
	{ 0, 0, 0,
	    sys_nosys },			/* 303 = unimplemented modfind */
	{ 0, 0, 0,
	    sys_nosys },			/* 304 = unimplemented kldload */
	{ 0, 0, 0,
	    sys_nosys },			/* 305 = unimplemented kldunload */
	{ 0, 0, 0,
	    sys_nosys },			/* 306 = unimplemented kldfind */
	{ 0, 0, 0,
	    sys_nosys },			/* 307 = unimplemented kldnext */
	{ 0, 0, 0,
	    sys_nosys },			/* 308 = unimplemented kldstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 309 = unimplemented kldfirstmod */
	{ 0, 0, 0,
	    sys_nosys },			/* 310 = unimplemented getsid */
	{ 3, s(struct sys_setresuid_args), 0,
	    sys_setresuid },			/* 311 = setresuid */
	{ 3, s(struct sys_setresgid_args), 0,
	    sys_setresgid },			/* 312 = setresgid */
	{ 0, 0, 0,
	    sys_nosys },			/* 313 = unimplemented signanosleep */
	{ 0, 0, 0,
	    sys_nosys },			/* 314 = unimplemented aio_return */
	{ 0, 0, 0,
	    sys_nosys },			/* 315 = unimplemented aio_suspend */
	{ 0, 0, 0,
	    sys_nosys },			/* 316 = unimplemented aio_cancel */
	{ 0, 0, 0,
	    sys_nosys },			/* 317 = unimplemented aio_error */
	{ 0, 0, 0,
	    sys_nosys },			/* 318 = unimplemented aio_read */
	{ 0, 0, 0,
	    sys_nosys },			/* 319 = unimplemented aio_write */
	{ 0, 0, 0,
	    sys_nosys },			/* 320 = unimplemented lio_listio */
	{ 0, 0, 0,
	    sys_nosys },			/* 321 = unimplemented yield */
	{ 0, 0, 0,
	    sys_nosys },			/* 322 = unimplemented thr_sleep */
	{ 0, 0, 0,
	    sys_nosys },			/* 323 = unimplemented thr_wakeup */
	{ 0, 0, 0,
	    sys_nosys },			/* 324 = unimplemented mlockall */
	{ 0, 0, 0,
	    sys_nosys },			/* 325 = unimplemented munlockall */
	{ 0, 0, 0,
	    sys_nosys },			/* 326 = unimplemented __getcwd */
	{ 0, 0, 0,
	    sys_nosys },			/* 327 = unimplemented sched_setparam */
	{ 0, 0, 0,
	    sys_nosys },			/* 328 = unimplemented sched_getparam */
	{ 0, 0, 0,
	    sys_nosys },			/* 329 = unimplemented sched_setscheduler */
	{ 0, 0, 0,
	    sys_nosys },			/* 330 = unimplemented sched_getscheduler */
	{ 0, 0, 0,
	    sys_nosys },			/* 331 = unimplemented sched_yield */
	{ 0, 0, 0,
	    sys_nosys },			/* 332 = unimplemented sched_get_priority_max */
	{ 0, 0, 0,
	    sys_nosys },			/* 333 = unimplemented sched_get_priority_min */
	{ 0, 0, 0,
	    sys_nosys },			/* 334 = unimplemented sched_rr_get_interval */
	{ 0, 0, 0,
	    sys_nosys },			/* 335 = unimplemented utrace */
	{ 0, 0, 0,
	    sys_nosys },			/* 336 = unimplemented sendfile */
	{ 0, 0, 0,
	    sys_nosys },			/* 337 = unimplemented kldsym */
	{ 0, 0, 0,
	    sys_nosys },			/* 338 = unimplemented jail */
	{ 0, 0, 0,
	    sys_nosys },			/* 339 = unimplemented pioctl */
	{ 3, s(struct freebsd_sys_sigprocmask40_args), 0,
	    freebsd_sys_sigprocmask40 },	/* 340 = sigprocmask40 */
	{ 1, s(struct freebsd_sys_sigsuspend40_args), 0,
	    freebsd_sys_sigsuspend40 },		/* 341 = sigsuspend40 */
	{ 3, s(struct freebsd_sys_sigaction40_args), 0,
	    freebsd_sys_sigaction40 },		/* 342 = sigaction40 */
	{ 1, s(struct freebsd_sys_sigpending40_args), 0,
	    freebsd_sys_sigpending40 },		/* 343 = sigpending40 */
	{ 0, 0, 0,
	    sys_nosys },			/* 344 = unimplemented 4.0 sigreturn */
	{ 0, 0, 0,
	    sys_nosys },			/* 345 = unimplemented sigtimedwait */
	{ 0, 0, 0,
	    sys_nosys },			/* 346 = unimplemented sigwaitinfo */
	{ 0, 0, 0,
	    sys_nosys },			/* 347 = unimplemented __acl_get_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 348 = unimplemented __acl_set_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 349 = unimplemented __acl_get_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 350 = unimplemented __acl_set_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 351 = unimplemented __acl_delete_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 352 = unimplemented __acl_delete_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 353 = unimplemented __acl_aclcheck_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 354 = unimplemented __acl_aclcheck_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 355 = unimplemented extattrctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 356 = unimplemented extattr_set_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 357 = unimplemented extattr_get_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 358 = unimplemented extattr_delete_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 359 = unimplemented aio_waitcomplete */
	{ 0, 0, 0,
	    sys_nosys },			/* 360 = unimplemented getresuid */
	{ 0, 0, 0,
	    sys_nosys },			/* 361 = unimplemented getresgid */
	{ 0, 0, 0,
	    sys_kqueue },			/* 362 = kqueue */
	{ 6, s(struct sys_kevent_args), 0,
	    sys_kevent },			/* 363 = kevent */
	{ 0, 0, 0,
	    sys_nosys },			/* 364 = unimplemented __cap_get_proc */
	{ 0, 0, 0,
	    sys_nosys },			/* 365 = unimplemented __cap_set_proc */
	{ 0, 0, 0,
	    sys_nosys },			/* 366 = unimplemented __cap_get_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 367 = unimplemented __cap_get_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 368 = unimplemented __cap_set_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 369 = unimplemented __cap_set_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 370 = unimplemented lkmressym */
	{ 0, 0, 0,
	    sys_nosys },			/* 371 = unimplemented extattr_set_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 372 = unimplemented extattr_get_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 373 = unimplemented extattr_delete_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 374 = unimplemented __setugid */
	{ 0, 0, 0,
	    sys_nosys },			/* 375 = unimplemented nfsclnt */
	{ 0, 0, 0,
	    sys_nosys },			/* 376 = unimplemented eaccess */
	{ 0, 0, 0,
	    sys_nosys },			/* 377 = unimplemented afs_syscall */
	{ 0, 0, 0,
	    sys_nosys },			/* 378 = unimplemented nmount */
	{ 0, 0, 0,
	    sys_nosys },			/* 379 = unimplemented kse_exit */
	{ 0, 0, 0,
	    sys_nosys },			/* 380 = unimplemented kse_wakeup */
	{ 0, 0, 0,
	    sys_nosys },			/* 381 = unimplemented kse_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 382 = unimplemented kse_thr_interrupt */
	{ 0, 0, 0,
	    sys_nosys },			/* 383 = unimplemented kse_release */
	{ 0, 0, 0,
	    sys_nosys },			/* 384 = unimplemented __mac_get_proc */
	{ 0, 0, 0,
	    sys_nosys },			/* 385 = unimplemented __mac_set_proc */
	{ 0, 0, 0,
	    sys_nosys },			/* 386 = unimplemented __mac_get_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 387 = unimplemented __mac_get_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 388 = unimplemented __mac_set_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 389 = unimplemented __mac_set_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 390 = unimplemented kenv */
	{ 0, 0, 0,
	    sys_nosys },			/* 391 = unimplemented lchflags */
	{ 0, 0, 0,
	    sys_nosys },			/* 392 = unimplemented uuidgen */
	{ 0, 0, 0,
	    sys_nosys },			/* 393 = unimplemented sendfile */
	{ 0, 0, 0,
	    sys_nosys },			/* 394 = unimplemented mac_syscall */
	{ 0, 0, 0,
	    sys_nosys },			/* 395 = unimplemented getfsstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 396 = unimplemented statfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 397 = unimplemented fsstatfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 398 = unimplemented fhstatfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 399 = unimplemented nosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 400 = unimplemented ksem_close */
	{ 0, 0, 0,
	    sys_nosys },			/* 401 = unimplemented ksem_post */
	{ 0, 0, 0,
	    sys_nosys },			/* 402 = unimplemented ksem_wait */
	{ 0, 0, 0,
	    sys_nosys },			/* 403 = unimplemented ksem_trywait */
	{ 0, 0, 0,
	    sys_nosys },			/* 404 = unimplemented ksem_init */
	{ 0, 0, 0,
	    sys_nosys },			/* 405 = unimplemented ksem_open */
	{ 0, 0, 0,
	    sys_nosys },			/* 406 = unimplemented ksem_unlink */
	{ 0, 0, 0,
	    sys_nosys },			/* 407 = unimplemented ksem_getvalue */
	{ 0, 0, 0,
	    sys_nosys },			/* 408 = unimplemented ksem_destroy */
	{ 0, 0, 0,
	    sys_nosys },			/* 409 = unimplemented __mac_get_pid */
	{ 0, 0, 0,
	    sys_nosys },			/* 410 = unimplemented __mac_get_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 411 = unimplemented __mac_set_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 412 = unimplemented extattr_set_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 413 = unimplemented extattr_get_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 414 = unimplemented extattr_delete_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 415 = unimplemented __mac_execve */
	{ 0, 0, 0,
	    sys_nosys },			/* 416 = unimplemented sigaction */
	{ 0, 0, 0,
	    sys_nosys },			/* 417 = unimplemented sigreturn */
	{ 0, 0, 0,
	    sys_nosys },			/* 418 = unimplemented __xstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 419 = unimplemented __xfstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 420 = unimplemented __xlstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 421 = unimplemented getcontext */
	{ 0, 0, 0,
	    sys_nosys },			/* 422 = unimplemented setcontext */
	{ 0, 0, 0,
	    sys_nosys },			/* 423 = unimplemented swapcontext */
	{ 0, 0, 0,
	    sys_nosys },			/* 424 = unimplemented swapoff */
	{ 0, 0, 0,
	    sys_nosys },			/* 425 = unimplemented __acl_get_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 426 = unimplemented __acl_set_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 427 = unimplemented __acl_delete_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 428 = unimplemented __acl_aclcheck_link */
	{ 0, 0, 0,
	    sys_nosys },			/* 429 = unimplemented sigwait */
	{ 0, 0, 0,
	    sys_nosys },			/* 430 = unimplemented thr_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 431 = unimplemented thr_exit */
	{ 0, 0, 0,
	    sys_nosys },			/* 432 = unimplemented thr_self */
	{ 0, 0, 0,
	    sys_nosys },			/* 433 = unimplemented thr_kill */
	{ 0, 0, 0,
	    sys_nosys },			/* 434 = unimplemented _umtx_lock */
	{ 0, 0, 0,
	    sys_nosys },			/* 435 = unimplemented _umtx_unlock */
	{ 0, 0, 0,
	    sys_nosys },			/* 436 = unimplemented jail_attach */
	{ 0, 0, 0,
	    sys_nosys },			/* 437 = unimplemented extattr_list_fd */
	{ 0, 0, 0,
	    sys_nosys },			/* 438 = unimplemented extattr_list_file */
	{ 0, 0, 0,
	    sys_nosys },			/* 439 = unimplemented extattr_list_link */
};

