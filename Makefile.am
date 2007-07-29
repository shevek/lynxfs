AUTOMAKE_OPTIONS = foreign

EXTRA_DIST		= LICENSE README config win32

SUBDIRS = src

dist-hook:
	@for file in $(EXTRA_DIST); do \
		find $(distdir)/$$file -name .svn | xargs rm -rf; \
	done; \
	rm -f $(distdir)/src/lfs_config.h
