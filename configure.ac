
# Process this file with Autoconf to produce a configure script

AC_INIT( [Visual Python], [3.2.9], [visualpython-users@lists.sourceforge.net])
AM_INIT_AUTOMAKE( visual, 3.2.9)

AC_CONFIG_SRCDIR(src/arrow.cpp)

# Compiling C and C++ code

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AM_MAINTAINER_MODE

# Determine if we are on MS Windows or Apple OSX.
VISUAL_CHECK_PLATFORM()

# Determine the location of Python and its directories
AM_PATH_PYTHON(2.2)

# Verify that Numeric prerequisites are available.
VISUAL_NUMERICLIBS()

AM_CHECK_PYTHON_HEADERS([], [
	AC_MSG_ERROR([A suitable python interpreter was found, but you do not have the header files required for building C/C++ extensions to python, or another problem was encoutered when compiling a program that uses Python.h.])])


if test $platform_win32 = yes; then
	# We are on Win32.
	AC_SUBST([GLOBJ], [wgl])
	AC_SUBST([PLATFORMOBJ], [platwin])
	AC_DEFINE([HAVE_WIN32_API],[1])
	# The current working directory, as a backslashified Win32 style path.
	AC_SUBST([WIN32_CWD], [`pwd -W | sed -e "s/\\//\\\\\/g"`])
	# The top-level source directory.
	AC_SUBST([WIN32_SRCDIR], [`cd $srcdir ; pwd -W | sed -e "s/\\//\\\\\/g"`])
else
	PKG_CHECK_MODULES( GTK, gtk+ >= 1.0, ,
		[AC_MSG_ERROR(Gtk+ is required on Unix-like systems)])
		AC_SUBST([GTK_LIBS])
		AC_SUBST([GTK_CFLAGS])
	PKG_CHECK_MODULES( GTHREAD, gthread >= 1.0, ,
		[AC_MSG_ERROR(GThread is required on Unix-like systems)])
	AC_SUBST([GTHREAD_CFLAGS])
	AC_SUBST([GTHREAD_LIBS])

	AM_PATH_GTKGL([
		AC_SUBST([PLATFORMOBJ], [platlinux])
		AC_SUBST([GLOBJ], [xgl])
		AC_DEFINE([HAVE_GTK_GL_AREA],[1])], 
		[AC_MSG_ERROR([gtkglarea is required on Unix-like systems])])
fi

# The major.minor version of Python, with the "." stripped out.
AC_SUBST([WIN32_PYTHON_VERSION], [`echo $PYTHON_VERSION | sed -e "s/\.//"`])

# Enable documentation and visualdocdir if desired
VISUAL_DOCS()
# Enable demos and visualdemodir if desired
VISUAL_EXAMPLES()

if test "x$PYTHONPATH" = "x" ; then
	python_path="\$PYTHONPATH"
else
	python_path="$PYTHONPATH:\$PYTHONPATH"
fi
AC_SUBST([PYTHONPATH], "$python_path")

AC_CONFIG_FILES( Makefile src/Makefile bin/vpython site-packages/visual/Makefile 
	docs/Makefile examples/Makefile MakeVPython.iss)
AM_CONFIG_HEADER( include/config.h)

AC_OUTPUT()
