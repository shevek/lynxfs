# Makefile prototype

AUTOMAKE_OPTIONS = foreign

INCLUDES	= -I$(top_builddir) -I$(top_builddir)/src

noinst_HEADERS = lynxfs.h lfs_config.h

bin_PROGRAMS = lynxfs
noinst_PROGRAMS	= lynxfs_static

lynxfs_SOURCES	= lynxfs.c

lynxfs_static_SOURCES	= $(lynxfs_SOURCES)
lynxfs_static_LDFLAGS	= -static

# Copied from the libtool info file:
#
#    This flag accepts an argument of the form
# `CURRENT[:REVISION[:AGE]]'.  So, passing `-version-info 3:12:1' sets
# CURRENT to 3, REVISION to 12, and AGE to 1.
# 
#    If either REVISION or AGE are omitted, they default to 0.  Also note
# that AGE must be less than or equal to the CURRENT interface number.
# 
#    Here are a set of rules to help you update your library version
# information:
# 
#   1. Start with version information of `0:0:0' for each libtool library.
# 
#   2. Update the version information only immediately before a public
#      release of your software.  More frequent updates are unnecessary,
#      and only guarantee that the current interface number gets larger
#      faster.
# 
#   3. If the library source code has changed at all since the last
#      update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
# 
#   4. If any interfaces have been added, removed, or changed since the
#      last update, increment CURRENT, and set REVISION to 0.
# 
#   5. If any interfaces have been added since the last public release,
#      then increment AGE.
# 
#   6. If any interfaces have been removed since the last public release,
#      then set AGE to 0.
# 
#    *_Never_* try to set the interface numbers so that they correspond
# to the release number of your package.  This is an abuse that only
# fosters misunderstanding of the purpose of library versions.  Instead,
# use the `-release' flag (*note Release numbers::), but be warned that
# every release of your package will not be binary compatible with any
# other release.
# 
