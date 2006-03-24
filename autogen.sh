#!/bin/sh
#
# autogen.sh 
#
# Requires: automake, autoconf, libtool

aclocal
libtoolize
automake --foreign --add-missing
autoconf

# Verify that everything was generated correctly.  Note that some of these
# may just be symlinks, which is OK.

if [ -e Makefile.in ] && [ -e aclocal.m4 ] && [ -e config.guess ] \
	&& [ -e config.sub ] && [ -e configure ] && [ -e install-sh ] \
	&& [ -e ltmain.sh ] && [ -e missing ] && [ -e py-compile ] \
	&& [ -e site-packages/visual/Makefile.in ] \
	&& [ -e examples/Makefile.in ] && [ -e docs/Makefile.in ] ; then
	echo "Completed successfully"
else
	echo "One or more generated files was not created properly."
	exit 1
fi

exit 0
