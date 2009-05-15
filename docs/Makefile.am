## Process this file with automake to produce Makefile.in
# Makefile to distribute the documentation

AUTOMAKE_OPTIONS = foreign

srcdir = @srcdir@
VPATH = @srcdir@

if BUILD_DOCS
docfiles = index.html \
	visual/arrow.html visual/axes.gif visual/box.html visual/color.html \
	visual/cone.html visual/controls.html visual/convex.html visual/curve.html \
	visual/cylinder.html visual/defaults.html visual/delete.html visual/display.html \
	visual/ellipsoid.html visual/faces.html visual/factorial.html \
	visual/files.html visual/float.html \
	visual/frame.html visual/graph.html visual/helix.html visual/index.html \
	visual/keyboard.html visual/label.html visual/license.txt visual/lights.html \
	visual/materials.html visual/mouse.html visual/mouse_click.html \
	visual/mouse_drag.html visual/new_features.html visual/options.html \
	visual/points.html visual/primitives.html \
	visual/pyramid.html visual/rate.html visual/ring.gif visual/ring.html \
	visual/rotation.html visual/sphere.html visual/spherebox.gif visual/vector.html \
	visual/VisualIntro.html visual/VisualRef.css \
	visual/VisualRef-1.gif visual/VisualRef-2.gif visual/VisualRef-3.gif \
	visual/VisualRef-4.gif visual/VisualRef-6.gif visual/VisualRef-7.gif \
	visual/VPython_Intro.pdf \
	visual/images/arrow.jpg visual/images/box.jpg visual/images/cone.jpg visual/images/controls.jpg \
	visual/images/convex.jpg visual/images/curve.jpg visual/images/cylinder.jpg visual/images/ellipsoid.jpg \
	visual/images/faces.jpg visual/images/filedialog.jpg visual/images/graph.jpg visual/images/helix.jpg \
	visual/images/label.jpg visual/images/material_etc.jpg visual/images/points.jpg \
	visual/images/pyramid.jpg visual/images/ring.jpg visual/images/sphere.jpg
else
docfiles = 
endif

pkgdatadir = @visualdocdir@

nobase_dist_pkgdata_DATA = $(docfiles)
