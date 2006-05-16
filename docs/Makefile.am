## Process this file with automake to produce Makefile.in
# Makefile to distribute the documentation

AUTOMAKE_OPTIONS = foreign

srcdir = @srcdir@
VPATH = @srcdir@

if BUILD_DOCS
docfiles = visual/VisualRef-7.gif visual/delete.html visual/mouse_click.html visual/VisualRef.css \
	visual/display.html visual/mouse_drag.html visual/arrow.html visual/faces.html visual/options.html \
	visual/axes.gif visual/factorial.html visual/rate.html visual/VisualIntro.css visual/box.html \
	visual/float.html visual/ring.html visual/VisualIntro.html visual/color.html visual/frame.html \
	visual/rotation.html visual/VisualRef-1.gif visual/cone.html visual/graph.html visual/sphere.html \
	visual/pyramid.html visual/ellipsoid.html \
	visual/VisualRef-2.gif visual/controls.html index.html visual/index.html visual/spherebox.gif \
	visual/VisualRef-3.gif visual/convex.html visual/keyboard.html visual/vector.html \
	visual/VisualRef-4.gif visual/curve.html visual/label.html visual/VisualRef-5.gif \
	visual/cylinder.html visual/license.txt visual/VisualRef-6.gif visual/defaults.html visual/mouse.html \
	visual/helix.html \
	visual/icons/blank.gif visual/icons/contents.gif visual/icons/index.gif \
	visual/icons/modules.gif visual/icons/next.gif visual/icons/previous.gif \
	visual/icons/up.gif
else
docfiles = 
endif

pkgdatadir = @visualdocdir@

nobase_dist_pkgdata_DATA = $(docfiles)
