## Process this file with automake to produce Makefile.in
# Makefile to distribute the example programs

AUTOMAKE_OPTIONS = foreign
srcdir = @srcdir@
VPATH = @srcdir@

if BUILD_EXAMPLES
examplefiles = bounce.py bounce2.py boxlighttest.py colorsliders.py \
	controlstest.py \
	convex.py crossproduct.py crystal.py dipole.py doublependulum.py \
	drape.py faces_cone.py faces_heightfield.py gas.py glinfo.py \
	graphtest.py gyro.py gyro2.py hanoi.py labels.py lathe.py \
	lorenz.py mandelbrot.py material_test.py orbit.py planar.py randombox.py stars.py \
	stonehenge.py texttest.py texturetest.py tictac.py tictacdat.py \
	texture_and_lighting.py flower128.tga \
	toroid_drag.py toroid.py wave.py
else
examplefiles =
endif

pkgdatadir = @visualexampledir@

dist_pkgdata_DATA = $(examplefiles)
