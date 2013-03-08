dnl **********************************************************************
dnl * configure.ac
dnl *
dnl *  Pointclound build configuration.
dnl *
dnl *  PgSQL Pointcloud is free and open source software provided 
dnl *  by the Government of Canada
dnl *  Copyright (c) 2013 Natural Resources Canada
dnl *
dnl ***********************************************************************/

AC_INIT()
AC_CONFIG_MACRO_DIR([macros])

dnl
dnl Compilers
dnl
AC_PROG_CC

dnl
dnl Define executable suffix to use for utility programs
dnl
EXESUFFIX="$ac_cv_exeext"
AC_SUBST([EXESUFFIX])

dnl
dnl Search for flex/bison to build the parser
dnl
dnl AC_PROG_LEX
dnl AC_PROG_YACC
dnl AC_SUBST([LEX])
dnl AC_SUBST([YACC])


dnl ===========================================================================
dnl Detect ZLib if it is installed 
dnl ===========================================================================

ZLIB_LDFLAGS=""
AC_CHECK_HEADER([zlib.h], [
	CUNIT_CPPFLAGS="$CPPFLAGS"
	AC_CHECK_LIB([z], 
	  [inflate], 
	  [ZLIB_LDFLAGS="$LDFLAGS -lz"], 
	  [AC_MSG_ERROR([could not locate zlib])]
	  )
	],
	[
	AC_MSG_ERROR([could not locate zlib])
	])
	
AC_SUBST([ZLIB_CPPFLAGS])
AC_SUBST([ZLIB_LDFLAGS])


dnl ===========================================================================
dnl Detect CUnit if it is installed 
dnl ===========================================================================

CUNIT_LDFLAGS=""
AC_CHECK_HEADER([CUnit/CUnit.h], [
	CUNIT_CPPFLAGS="$CPPFLAGS"
	AC_CHECK_LIB([cunit], 
	  [CU_initialize_registry], 
	  [CUNIT_LDFLAGS="$LDFLAGS -lcunit"], 
	  [AC_MSG_ERROR([could not locate CUnit required for unit tests])]
	  )
	],
	[
	AC_MSG_ERROR([could not locate CUnit required for unit tests])
	])
	
AC_SUBST([CUNIT_CPPFLAGS])
AC_SUBST([CUNIT_LDFLAGS])


dnl ===========================================================================
dnl Detect the version of PostgreSQL installed on the system
dnl ===========================================================================

AC_ARG_WITH([pgconfig], 
	[AS_HELP_STRING([--with-pgconfig=FILE], [specify an alternative pg_config file])], 
	[PGCONFIG="$withval"], [PGCONFIG=""])

if test "x$PGCONFIG" = "x"; then
	dnl PGCONFIG was not specified, so search within the current path
	AC_PATH_PROG([PGCONFIG], [pg_config])

	dnl If we couldn't find pg_config, display an error
	if test "x$PGCONFIG" = "x"; then
		AC_MSG_ERROR([could not find pg_config within the current path. You may need to try re-running configure with a --with-pgconfig parameter.])
	fi
else
	dnl PGCONFIG was specified; display a message to the user
	if test "x$PGCONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-pgconfig, e.g. --with-pgconfig=/path/to/pg_config])
	else
		if test -f $PGCONFIG; then
			AC_MSG_RESULT([Using user-specified pg_config file: $PGCONFIG])
		else
			AC_MSG_ERROR([the user-specified pg_config file $PGCONFIG does not exist])
		fi
	fi
fi


dnl ===========================================================================
dnl Ensure that $PG_CONFIG --pgxs points to a valid file. This is because some 
dnl distributions such as Debian also include pg_config as part of libpq-dev 
dnl packages, but don't install the Makefile it points to unless
dnl the postgresql-server-dev packages are installed :)
dnl ===========================================================================

PGXS=`$PGCONFIG --pgxs`
if test ! -f $PGXS; then
	AC_MSG_ERROR([the PGXS Makefile $PGXS cannot be found. Please install the PostgreSQL server development packages and re-run configure.])
fi

AC_SUBST([PGXS])

dnl Extract the version information from pg_config
dnl Note: we extract the major & minor separately, ensure they are numeric, and then combine to give
dnl the final version. This is to guard against user error... 
PGSQL_MAJOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f1 | sed 's/[[^0-9]]//g'`	
PGSQL_MINOR_VERSION=`$PGCONFIG --version | sed 's/[[A-Za-z ]]*//' | cut -d. -f2 | sed 's/[[^0-9]]//g'`	
PGSQL_FULL_VERSION=`$PGCONFIG --version`
PGSQL_VERSION="$PGSQL_MAJOR_VERSION$PGSQL_MINOR_VERSION"

PGSQL_PKGLIBDIR=`$PGCONFIG --pkglibdir`
PGSQL_LIBDIR=`$PGCONFIG --libdir`
PGSQL_SHAREDIR=`$PGCONFIG --sharedir`

AC_MSG_RESULT([checking PostgreSQL version... $PGSQL_FULL_VERSION])
dnl Ensure that we are using PostgreSQL >= 9.0
if test ! "$PGSQL_MAJOR_VERSION" -ge 9; then
	AC_MSG_ERROR([PointCloud requires PostgreSQL >= 9.0])
fi	

dnl Extract the linker and include flags for the frontend (for programs that use libpq)
PGSQL_FE_LDFLAGS=-L`$PGCONFIG --libdir`" -lpq"
PGSQL_FE_CPPFLAGS=-I`$PGCONFIG --includedir`

AC_SUBST([PGSQL_FE_LDFLAGS])
AC_SUBST([PGSQL_FE_CPPFLAGS])

dnl Ensure that we can parse libpq-fe.h
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PGSQL_FE_CPPFLAGS"
AC_CHECK_HEADER([libpq-fe.h], [], [AC_MSG_ERROR([could not find libpq-fe.h])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libpq
LIBS_SAVE="$LIBS"
LIBS="$PGSQL_FE_LDFLAGS"
AC_CHECK_LIB([pq], [PQserverVersion],
	[],
	[AC_MSG_ERROR([could not find libpq])],
	[])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([PGSQL_VERSION], [$PGSQL_VERSION], [PostgreSQL server version])	
AC_SUBST([PGSQL_VERSION])


dnl ===========================================================================
dnl Detect LibXML2 
dnl ===========================================================================

AC_ARG_WITH([xml2config], 
	[AS_HELP_STRING([--with-xml2config=FILE], [specify an alternative xml2-config file])], 
	[XML2CONFIG="$withval"], [XML2CONFIG=""])

if test "x$XML2CONFIG" = "x"; then
	dnl XML2CONFIG was not specified, so search within the current path
	AC_PATH_PROG([XML2CONFIG], [xml2-config])

	dnl If we couldn't find xml2-config, display a warning
	if test "x$XML2CONFIG" = "x"; then
		AC_MSG_ERROR([could not find xml2-config from libxml2 within the current path. You may need to try re-running configure with a --with-xml2config parameter.])
	fi
else
	dnl XML2CONFIG was specified; display a message to the user
	if test "x$XML2CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
	else
		if test -f $XML2CONFIG; then
			AC_MSG_RESULT([Using user-specified xml2-config file: $XML2CONFIG])
		else
			AC_MSG_ERROR([the user-specified xml2-config file $XML2CONFIG does not exist])
		fi	
	fi
fi


dnl Extract the linker and include flags 
XML2_LDFLAGS=`$XML2CONFIG --libs`
XML2_CPPFLAGS=`$XML2CONFIG --cflags`

dnl Extract the version
LIBXML2_VERSION=`$XML2CONFIG --version`

dnl Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XML2_CPPFLAGS"
AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h libxml/xpath.h libxml/xpathInternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxml2])])
CPPFLAGS="$CPPFLAGS_SAVE"

dnl Ensure we can link against libxml2
LIBS_SAVE="$LIBS"
LIBS="$XML2_LDFLAGS"
AC_CHECK_LIB([xml2], [xmlInitParser], [], [AC_MSG_ERROR([could not find libxml2])], [])
LIBS="$LIBS_SAVE"

AC_DEFINE_UNQUOTED([LIBXML2_VERSION], ["$LIBXML2_VERSION"], [PointCloud libxml2 version])
AC_SUBST([LIBXML2_VERSION])
AC_SUBST([XML2_LDFLAGS])
AC_SUBST([XML2_CPPFLAGS])


dnl ===========================================================================
dnl Output the relevant files
dnl ===========================================================================

AC_OUTPUT([ 
   config.mk
])
    
dnl ===========================================================================
dnl Display the configuration status information
dnl ===========================================================================

AC_MSG_RESULT()
AC_MSG_RESULT([  PointCloud is now configured for ${host}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Compiler Info ------------- ])
AC_MSG_RESULT([  C compiler:           ${CC} ${CFLAGS}])
AC_MSG_RESULT()
AC_MSG_RESULT([ -------------- Dependencies -------------- ])
AC_MSG_RESULT([  PostgreSQL config:    ${PGCONFIG}])
AC_MSG_RESULT([  PostgreSQL version:   ${PGSQL_FULL_VERSION}])
AC_MSG_RESULT([  Libxml2 config:       ${XML2CONFIG}])
AC_MSG_RESULT([  Libxml2 version:      ${LIBXML2_VERSION}])
AC_MSG_RESULT()
