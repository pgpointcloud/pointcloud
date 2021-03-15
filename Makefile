
all install uninstall noop clean distclean:
	$(MAKE) -C lib $@
	$(MAKE) -C pgsql $@
	$(MAKE) -C pgsql_postgis $@

check:
	$(MAKE) -C lib $@

installcheck:
	$(MAKE) -C pgsql $@
	$(MAKE) -C pgsql_postgis $@

astyle:
	find . \
	  -name "*.c" \
	  -type f \
	  -or \
	  -name "*.h" \
	  -type f \
	  -exec astyle --style=ansi --indent=tab --suffix=none {} ';'

maintainer-clean:
	rm -f config.log config.mk config.status lib/pc_config.h configure
	rm -rf autom4te.cache build
