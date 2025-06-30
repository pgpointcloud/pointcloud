
all install uninstall noop clean distclean:
	$(MAKE) -C lib $@
	$(MAKE) -C pgsql $@
	$(MAKE) -C pgsql_postgis $@

check:
	$(MAKE) -C lib $@

installcheck:
	$(MAKE) -C pgsql $@

maintainer-clean: clean
	rm -f config.log config.mk config.status lib/pc_config.h configure
	rm -rf autom4te.cache build
