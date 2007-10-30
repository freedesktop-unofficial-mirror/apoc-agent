AC_INIT(apoc-desktop-agent, 1.0)

AM_INIT_AUTOMAKE
AC_JAVA_OPTIONS

AC_PROG_CC
AC_PROG_LIBTOOL

#Checking and including libXML2
PKG_CHECK_MODULES(LIBXML2, libxml-2.0)
CFLAGS="$CFLAGS $LIBXML2_CFLAGS"
LDFLAGS="$LDFLAGS $LIBXML2_LIBS"
LIBS="$LIBS $LIBXML2_LIBS"

# Setting APOC Daemon dir
AC_EXPAND(libdir,NONE,EXPANDED_LIBDIR)

APOCD_DIR="$EXPANDED_LIBDIR/apoc"

AC_ARG_WITH(daemon-dir,
AC_HELP_STRING( [--with-daemon-dir=DIR],
		[specify the APOC daemon installation path (default=LIBDIR/apoc)]),
[
	if test "$withval" != "no"
	then
		APOCD_DIR=$withval
	fi
])

#Java DB 4.2 library directory
DBJAVA_DIR="/usr"
AC_ARG_WITH(db-java-prefix,
AC_HELP_STRING( [--with-db-java-prefix=DIR],
		[specify the DB JAVA 4.2 installation prefix (default=/usr)]),
[
	if test "$withval" != "no"
	then
		BDBJAVA_DIR=$withval
	fi
])

DBJAVA_LIBDIR=$DBJAVA_DIR/lib
AC_ARG_WITH(db-java-libdir,
AC_HELP_STRING( [--with-db-java-libdir=DIR],
		[specify the BDB JAVA 4.2 library installation prefix (default=DBJAVA_DIR/usr)]),
[
	if test "$withval" != "no"
	then
		DBJAVA_LIBDIR=$withval
	fi
])

# Check only for the presence of the library, it's only needed
# in runtime by the java bindings. No need to add compiler flags.
TMP_LIBS=$LIBS
TMP_CFLAGS=$CFLAGS
TMP_LDFLAGS=$LDFLAGS
LDFLAGS=-L$DBJAVA_LIBDIR
AC_CHECK_LIB(db_java-4.2, DbEnv_get_version_major,
	[],
	AC_MSG_ERROR(DB JAVA 4.2 library not found. Check your DBJAVA prefix option.
If it is already installed please use --with-db-java-prefix=DIR or --with-db-java-libdir=DIR options to point to the DB JAVA 4.2 installation prefix or library dir.))
LDFLAGS=$TMP_LDFLAGS
CFLAGS=$CFLAGS
LIBS=$TMP_LIBS

# Checking for the GSSAPI
GSSAPI_DIR=/usr
GSSAPI_LIBDIR="$GSSAPI_DIR/lib"
GSSAPI_INCLUDEDIR="$GSSAPI_DIR/include"

AC_ARG_WITH(gss-prefix,
AC_HELP_STRING( [--with-gss-prefix=DIR],
		[specify the GSS API installation prefix (default=/usr)]),
[
	if test "$withval" != "no"
	then
		GSSAPI_DIR=$withval
	fi
])

AC_ARG_WITH(gss-libdir,
AC_HELP_STRING( [--with-gss-libdir=DIR],
		[specify the GSS API library path (default=GSS_PREFIX/lib)]),
[
	if test "$withval" != "no"
	then
		GSSAPI_LIBDIR=$withval
	fi
])

AC_ARG_WITH(gss-includedir,
AC_HELP_STRING( [--with-gss-includedir=DIR],
		[specify the GSS API include path (default=GSS_PREFIX/include)]),
[
	if test "$withval" != "no"
	then
		GSSAPI_INCLUDEDIR=$withval
	fi
])


CFLAGS="$CFLAGS -I$GSSAPI_INCLUDEDIR"
LDFLAGS="$LDFLAGS -L$GSSAPI_LIBDIR"

AC_CHECK_HEADERS(gssapi/gssapi.h,
	[GSSAPI_INCLUDES="-I$GSSAPI_INCLUDEDIR/gssapi"])

if test -z "$GSSAPI_INCLUDES"
then
AC_CHECK_HEADERS(gssapi.h,
	[GSSAPI_INCLUDES="-I$GSSAPI_INCLUDEDIR"])
fi

if test -z "$GSSAPI_INCLUDES"
then
	AC_MSG_ERROR(GSSAPI development headers not found.
If it is already installed please use --with-gss-prefix=DIR to point to the GSSAPI installation prefix.)
fi

#Checking the gssapi presence
AC_CHECK_LIB(gss, gss_init_sec_context,
	[GSSAPI_LIBS="-L$GSSAPI_LIBDIR -lgss"],) 

if test -z "$GSSAPI_LIBS"
then
AC_CHECK_LIB(gssapi, gss_init_sec_context,
	[GSSAPI_LIBS="-L$GSSAPI_LIBDIR -lgssapi"],)
fi 

if test -z "$GSSAPI_LIBS"
then
AC_CHECK_LIB(gssapi_krb5, gss_init_sec_context,
	[GSSAPI_LIBS="-L$GSSAPI_LIBDIR -lgssapi_krb5"],)
fi 

if test -z "$GSSAPI_LIBS"
then
AC_MSG_ERROR(GSSAPI library not found. Check your GSSAPI prefix option.
If it is already installed please use --with-gss-prefix=DIR to point to the GSSAPI installation prefix.)
fi


CFLAGS="$CFLAGS $GSSAPI_INCLUDES"
LDFLAGS="$LDFLAGS $GSSAPI_LIBS"
LIBS="$LIBS $GSSAPI_LIBS"


# Setting APOC class path
AC_EXPAND(datadir, NONE, EXPANDED_DATADIR)

APOCDIR=$EXPANDED_DATADIR'/lib/apoc'

AC_ARG_WITH(apoc-path,
AC_HELP_STRING( [--with-apoc-path=DIR],
		[specify a path to install the apoc libraries (DATADIR/lib/apoc)]),
[
	if test "$withval" != "no"
	then
		APOCDIR=$withval
	fi
])

SPIJDK=$APOCDIR/spi.jar

#Setting Oracle/Sleepycat's BerkeleyDB JDK
BDBJDK=$APOCDIR/db.jar

AC_ARG_WITH(bdb-path,
AC_HELP_STRING( [--with-bdb-path=DIR],
		[specify the Sleepycat's BerkeleyDB JDK path or jar file (APOCDIR/db.jar)]),
[
	if test "$withval" != "no"
	then
		BDBJDK=$withval
	fi
])

AC_ARG_ENABLE(debug,
AC_HELP_STRING( [--enable-debug],
		[enable debugging symbols]),
		[debug="yes"])

if test "$debug" == "yes"
then
	CFLAGS="$CFLAGS -g "
fi


_SAVE_CLASSPATH=$CLASSPATH
export CLASSPATH=$CLASPATH:$SPIJDK:$BDBJDK

AC_PROG_JAVAC
AC_PROG_JAVA
AC_PROG_JAR

AC_CHECK_CLASS(com.sleepycat.db.Db, [], [
echo
AC_MSG_ERROR(Sleepycat's BerkeleyDB JDK
If it is already installed please use --with-bdb-path=DIR to point to the path or jar file.)
])

AC_CHECK_CLASS(com.sun.apoc.spi.PolicyManager, [], [
echo
AC_MSG_ERROR(APOC SPI not found.
If it is already installed please use --with-apoc-path=DIR to point to the path wher the spi.jar file is stored.)
])

AC_JNI_INCLUDE_DIR

if test -z "$JNI_INCLUDE_DIRS"
then
	AC_MSG_ERROR(Java Native Interface (JNI) headers not found.)
fi

for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
do
	CFLAGS="$CFLAGS -I$JNI_INCLUDE_DIR"
done

AC_CHECK_PROG(ANT, ant, ant, no)
if test $ANT == "no"
then
	AC_MSG_ERROR(Apache's Ant was not found in path.)
fi

case "${host}" in
	*-*solaris*)	PLATFORM=UNX
			OS=SOLARIS
			CFLAGS="$CFLAGS -xarch=v8 -xspace -xO3"
			LIBS="$LIBS -lpthread -lnsl -lsocket -lm -lc -lrt -lscf"
			LDFLAGS="-mt -z combreloc -z text -Bdirect -z defs"
			;;
	*-*linux*) 	PLATFORM=UNX
			OS=LINUX
			;;
esac

CFLAGS="$CFLAGS -D$PLATFORM -D$OS"

AM_CONDITIONAL([SOLARIS], [test "$OS" = "SOLARIS"])

#APOC data dir
AC_EXPAND(localstatedir, NONE, EXPANDED_LOCALSTATEDIR)

APOC_DATADIR="$EXPANDED_LOCALSTATEDIR/apoc"

AC_ARG_WITH(apoc-datadir,
AC_HELP_STRING( [--with-apoc-data=DIR],
		[specify the APOC data dir (default=LOCALSTATEDIR/apoc)]),
[
	if test "$withval" != "no"
	then
		APOCD_DIR=$withval
	fi
])

AC_EXPAND(sysconfdir, NONE, EXPANDED_SYSCONFDIR)
AC_EXPAND(libdir, NONE, EXPANDED_LIBDIR)

AC_SUBST(APOCDIR)
AC_SUBST(SPIJDK)
AC_SUBST(BDBJDK)
AC_SUBST(APOCD_DIR)
AC_SUBST(APOC_DATADIR)
AC_SUBST(EXPANDED_SYSCONFDIR)
AC_SUBST(EXPANDED_LIBDIR)
AC_SUBST(DBJAVA_LIBDIR)

AC_OUTPUT([
Makefile
libapoc.pc
libapoc/Makefile
libapoc/config.h
libfileaccess/Makefile
apocd/Makefile
apocd/config.h
smf/apocd.xml
initd/apocd
src/Makefile
src/com/Makefile
src/com/sun/Makefile
src/com/sun/apoc/Makefile
src/com/sun/apoc/daemon/Makefile
src/com/sun/apoc/daemon/build.xml
src/com/sun/apoc/daemon/properties/os.properties
])
