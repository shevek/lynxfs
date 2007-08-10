#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lynxfs, 1.0.0, shevek@anarres.org)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(lynxfs, $PACKAGE_VERSION)
AC_CONFIG_SRCDIR([src/lynxfs.c])
AM_CONFIG_HEADER(src/lfs_config.h)

# remember the version info for later

KV_MAJOR=`echo $PACKAGE_VERSION | sed 's/^\([[0-9]]*\)\.[[0-9]]*\.[[0-9]]*/\1/'`
KV_MINOR=`echo $PACKAGE_VERSION | sed 's/^[[0-9]]*\.\([[0-9]]*\)\.[[0-9]]*/\1/'`
KV_PATCH=`echo $PACKAGE_VERSION | sed 's/^[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)/\1/'`
AC_SUBST([KV_MAJOR])
AC_SUBST([KV_MINOR])
AC_SUBST([KV_PATCH])


# Checks for programs.
AC_PROG_CC
if test "X$ac_cv_prog_gcc" = "Xyes"; then
	gcc_ver=`gcc -v 2>&1 | grep version | sed 's/.*gcc version \([[0-9]][[0-9]]*\)\..*/\1/'`
#        echo "GCC version: $gcc_ver"
	if test "X$gcc_ver" = "X3"; then
		CFLAGS="$CFLAGS -W -Wall -Wmissing-prototypes -Wwrite-strings"
#        	CFLAGS="$CFLAGS -std=c89 -D_BSD_SOURCE -W -Wall -Wmissing-prototypes -Wwrite-strings"
#        	CFLAGS="$CFLAGS -std=c99 -W -Wall -Wmissing-prototypes -Wwrite-strings"
#        	CFLAGS="$CFLAGS -std=gnu99 -W -Wall -Wmissing-prototypes -Wwrite-strings"
	else
		CFLAGS="$CFLAGS -Wall"
	fi
fi
AC_PROG_CPP
AC_SUBST(SUBDIRS)
AC_PROG_MAKE_SET
AC_SUBST(LIBPROGS)
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
dnl AC_PROG_RANLIB

# Check if the linker accepts --rpath (for Darwin)
AC_MSG_CHECKING([if ld accepts --rpath])
SAVEDLDFLAGS=$LDFLAGS
LDFLAGS=$LDFLAGS" -Wl,--rpath=/"
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
    [rpath="--rpath="; ldrpath=yes], [rpath="-L"; ldrpath=no])
LDFLAGS=$SAVEDLDFLAGS
AC_MSG_RESULT([$ldrpath])

# Checks for libraries.
AC_CHECK_LIB(nsl, inet_pton)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(intl, gettext)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

AC_CHECK_HEADERS([arpa/inet.h arpa/nameser.h ctype.h errno.h])
AC_CHECK_HEADERS([fcntl.h getopt.h grp.h inttypes.h])
AC_CHECK_HEADERS([libgen.h libintl.h malloc.h netdb.h])
AC_CHECK_HEADERS([netinet/in.h nmemory.h pwd.h]) 
AC_CHECK_HEADERS([stdarg.h stddef.h stdio.h stdlib.h])
AC_CHECK_HEADERS([string.h strings.h syslog.h sys/select.h])
AC_CHECK_HEADERS([sys/socket.h sys/time.h sys/types.h time.h])
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_VOLATILE
AC_C_BIGENDIAN
AC_C_INLINE
AC_C_CHAR_UNSIGNED
AC_CHECK_TYPES([u_char, u_int8_t, u_int16_t, u_int32_t, uint8_t, uint16_t, uint32_t])
AC_CHECK_TYPES([ns_type], [], [],
	[[
	#ifdef HAVE_SYS_SOCKET_H
	# include <sys/socket.h>		/* inet_ functions / structs */
	#endif
	#ifdef HAVE_NETINET_IN_H
	# include <netinet/in.h>		/* inet_ functions / structs */
	#endif
	#ifdef HAVE_ARPA_NAMESER_H
	#  include <arpa/nameser.h> /* DNS HEADER struct */
	#endif
	]])
AC_CHECK_TYPES([struct in6_addr], [], [],
	[[
	#ifdef HAVE_NETINET_IN_H
	#  include <netinet/in.h>   /* inet_ functions / structs */
	#endif
	]])

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL


# It is not clear if BIND8 and/or BIND9 require the use of the
# res_ninit() and res_nquery() functions when threading, but it is
# safest to use them.

AC_CHECK_DECLS([res_ninit], [], [], [[
	#ifdef HAVE_SYS_SOCKET_H
	# include <sys/socket.h>		/* inet_ functions / structs */
	#endif
	#ifdef HAVE_NETINET_IN_H
	# include <netinet/in.h>		/* inet_ functions / structs */
	#endif
	#ifdef HAVE_ARPA_NAMESER_H
	#  include <arpa/nameser.h> /* DNS HEADER struct */
	#endif
	#include <resolv.h>
	]])

AC_CHECK_FUNCS([getopt_long ftruncate gethostname memmove memset socket strcasecmp strchr strcspn strdup strerror strncasecmp strspn strtoul])


CFLAGS="$CFLAGS -D_FILE_OFFSET_BITS=64"
CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
AC_CHECK_HEADER(fuse.h, , [
	echo "FUSE is required to build this program."
	exit 1;
])
AC_CHECK_LIB(fuse, fuse_main, , [
	echo "FUSE is required to build this program."
	exit 1;
])


AC_CONFIG_FILES([Makefile Doxyfile src/Makefile])
AC_OUTPUT
