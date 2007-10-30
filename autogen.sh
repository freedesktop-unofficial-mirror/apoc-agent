#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=AGENT
TEST_TYPE=-f
FILE=apocd/apocd.c

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
	DIE=1
}


if test -x "`which automake-1.9`"
then
    AUTOMAKE=`which automake-1.9`
    ACLOCAL=`which aclocal-1.9`
    ACLOCAL_FLAGS="-I m4"
elif test -x "`which automake-1.10`"
then
    AUTOMAKE=`which automake-1.10`
    ACLOCAL=`which aclocal-1.10`
    ACLOCAL_FLAGS="-I m4"
else
	echo
	echo "You must have automake 1.10.x or 1.9.x installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	DIE=1
fi

if test -x "`which libtoolize`"
then
	LIBTOOLIZE=`which libtoolize`
	LIBTOOLIZEFLAGS="--force"
else
	echo
	echo "You must have libtoolize installed to comopile $PROJECT."
	echo "Install the appropriate package for your operating system."
	DIE=1
fi


if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        if test -z "$*"; then
                echo "I am going to run ./configure with no arguments - if you wish "
                echo "to pass any to it, please specify them on the $0 command line."
        fi
fi

rm -rf autom4te.cache


$ACLOCAL $ACLOCAL_FLAGS || exit $?
$LIBTOOLIZE $LIBTOOLIZEFLAGS || exit $?
$AUTOMAKE --add-missing || exit $?
autoconf || exit $?
cd $ORIGDIR || exit $?

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        $srcdir/configure --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS "$@" || exit $?

        echo 
        echo "Now type 'make' to compile $PROJECT."
fi
