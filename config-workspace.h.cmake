/* Define if you have DPMS support */
#cmakedefine HAVE_DPMS 1

/* Define if you have the DPMSCapable prototype in <X11/extensions/dpms.h> */
#cmakedefine HAVE_DPMSCAPABLE_PROTO 1

/* Define if you have the DPMSInfo prototype in <X11/extensions/dpms.h> */
#cmakedefine HAVE_DPMSINFO_PROTO 1

/* Define if you have gethostname */
#cmakedefine HAVE_GETHOSTNAME 1

/* Define if you have the gethostname prototype */
#cmakedefine HAVE_GETHOSTNAME_PROTO 1

/* Define to 1 if you have the `getpeereid' function. */
#cmakedefine HAVE_GETPEEREID 1

/* Defines if you have Solaris' libkstat */
/* #undef HAVE_KSTAT */

/* Define if you have long long as datatype */
#cmakedefine HAVE_LONG_LONG 1

/* Define to 1 if you have the `nice' function. */
#cmakedefine HAVE_NICE 1

/* Define to 1 if you have the <sasl.h> header file. */
#cmakedefine HAVE_SASL_H 1

/* Define to 1 if you have the <sasl/sasl.h> header file. */
#cmakedefine HAVE_SASL_SASL_H 1

/* Define to 1 if you have the `setpriority' function. */
#cmakedefine HAVE_SETPRIORITY 1

/* Define to 1 if you have the `sigaction' function. */
#cmakedefine HAVE_SIGACTION 1

/* Define to 1 if you have the `sigset' function. */
#cmakedefine HAVE_SIGSET 1

/* Define to 1 if you have statvfs */
#cmakedefine HAVE_STATVFS 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define if you have the struct ucred */
#cmakedefine HAVE_STRUCT_UCRED 1

/* Define to 1 if you have the <sys/loadavg.h> header file. */
#cmakedefine HAVE_SYS_LOADAVG_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#cmakedefine HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/statfs.h> header file. */
#cmakedefine HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#cmakedefine HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have statfs(). */
#cmakedefine HAVE_STATFS 1

/* Define to 1 if you have the <sys/select.h> header file. */
#cmakedefine HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#cmakedefine HAVE_SYS_VFS_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#cmakedefine HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <malloc.h> header file. */
#cmakedefine HAVE_MALLOC_H 1

/* Define if you have unsetenv */
#cmakedefine HAVE_UNSETENV 1

/* Define if you have the unsetenv prototype */
#cmakedefine HAVE_UNSETENV_PROTO 1

/* Define if you have usleep */
#cmakedefine HAVE_USLEEP 1

/* Define if you have the usleep prototype */
#cmakedefine HAVE_USLEEP_PROTO 1

/* Define to 1 if you have the `vsnprintf' function. */
#cmakedefine HAVE_VSNPRINTF 1

/* Define to 1 if you have the Wayland libraries. */
#cmakedefine WAYLAND_FOUND 1

/* KDE's default home directory */
#cmakedefine KDE_DEFAULT_HOME "${KDE_DEFAULT_HOME}"

/* KDE's configuration directory */
#define KDE_CONFDIR "${KDE_INSTALL_CONFDIR}"

/* KDE's static data directory */
#define KDE_DATADIR "${KDE_INSTALL_DATADIR}"

/* Define where your java executable is */
#undef PATH_JAVA

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME 1

/* xkb resources directory */
#cmakedefine XKBDIR "${XKBDIR}"

/* KWin binary name */
#define KWIN_BIN "${KWIN_BIN}"

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 if you have packagekit available. */
#cmakedefine HAVE_PACKAGEKIT 1

/*
 * On HP-UX, the declaration of vsnprintf() is needed every time !
 */

/* type to use in place of socklen_t if not defined */
#define kde_socklen_t socklen_t

#define WORKSPACE_VERSION_STRING "${PROJECT_VERSION}"
