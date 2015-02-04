
all install uninstall noop clean distclean:
	$(MAKE) -C lib $@
	$(MAKE) -C pgsql $@
	$(MAKE) -C pgsql_postgis $@

check test:
	$(MAKE) -C lib $@

astyle:
	find . \
	  -name "*.c" \
	  -type f \
	  -or \
	  -name "*.h" \
	  -type f \
	  -exec astyle --style=ansi --indent=tab --suffix=none {} ';'

