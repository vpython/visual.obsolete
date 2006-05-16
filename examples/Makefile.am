## Process this file with automake to produce Makefile.in
# Makefile to distribute the example programs

AUTOMAKE_OPTIONS = foreign
srcdir = @srcdir@
VPATH = @srcdir@

if BUILD_EXAMPLES
examplefiles = crossproduct.py gyro.py stonehenge.py crystal.py \
	gyro2.py texttest.py dipole.py hanoi.py tictac.py \
	doublependulum.py tictacdat.py Tk-Visual.py drape.py \
	lathe.py toroid.py bounce.py faces_cone.py lorenz.py \
	toroid_drag.py bounce2.py faces_heightfield.py orbit.py \
	wave.py colorsliders.py gas.py planar.py controlstest.py \
	glinfo.py randombox.py convex.py graphtest.py stars.py
else
examplefiles =
endif

pkgdatadir = @visualexampledir@

dist_pkgdata_DATA = $(examplefiles)
