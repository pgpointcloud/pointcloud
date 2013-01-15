
all install uninstall noop clean distclean:
	$(MAKE) -C libpc $@
	$(MAKE) -C postgis $@

check:
	$(MAKE) -C libpc $@

astyle:
	find . \
	  -name "*.c" \
	  -type f \
	  -or \
	  -name "*.h" \
	  -type f \
	  -exec astyle --style=ansi --indent=tab --suffix=none {} ';'

