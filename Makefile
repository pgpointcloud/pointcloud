SUBDIRS = core postgis

all install uninstall noop clean distclean check:
	for s in $(SUBDIRS); do \
		$(MAKE) -C $${s} $@ || exit 1; \
	done;
	@if test x"$@" = xall; then \
		echo "PointCloud was built successfully. Ready to install."; \
	fi

astyle:
	find . \
	  -name "*.c" \
	  -type f \
	  -or \
	  -name "*.h" \
	  -type f \
	  -exec astyle --style=ansi --indent=tab --suffix=none {} ';'

