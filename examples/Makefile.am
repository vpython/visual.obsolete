## Process this file with automake to produce Makefile.in
# Makefile to distribute the example programs

AUTOMAKE_OPTIONS = foreign
srcdir = @srcdir@
VPATH = @srcdir@

if BUILD_EXAMPLES
examplefiles = bounce.py bounce2.py colorsliders.py controlstest.py \
	convex.py crossproduct.py crystal.py dipole.py doublependulum.py \
	drape.py faces_cone.py faces_heightfield.py gas.py glinfo.py \
	graphtest.py gyro.py gyro2.py hanoi.py labels.py lathe.py \
	lorenz.py orbit.py planar.py randombox.py stars.py \
	stonehenge.py texttest.py texturetest.py tictac.py tictacdat.py \
	texture_and_lighting.py wood256lum.vpt flower128.vpt \
	Tk-Visual.py toroid_drag.py toroid.py wave.py
else
examplefiles =
endif

pkgdatadir = @visualexampledir@

dist_pkgdata_DATA = $(examplefiles)
