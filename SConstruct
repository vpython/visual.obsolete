import os
import sys

SetOption( "implicit_cache", 1)

# TODO: Add support to build a gch for wrap_gl.hpp, render_surface.hpp,
# and simple_displayobject.hpp


core = Environment( CCFLAGS=['-pipe', '-g'],
	ENV = os.environ,
	CPPPATH='include')

# Workaround brain-dead behavior in FTGL's header file layout
if 'linux' in sys.platform:
	core.Append( CPPPATH=['/usr/include/FTGL'])

# Crank up the warnings
core.Append( CCFLAGS=['-Wall', '-W', '-Wsign-compare', '-Wconversion',
	'-Wdisabled-optimization', '-D_GLIBCPP_CONCEPT_CHECKS'] )

core.ParseConfig( 'pkg-config --cflags --libs sigc++-1.2')

srcs = [ "src/core/arrow.cpp", 
	"src/core/util/displaylist.cpp",
	"src/core/util/errors.cpp",
	"src/core/util/extent.cpp",
	"src/core/util/lighting.cpp",
	"src/core/util/quadric.cpp",
	"src/core/util/rgba.cpp",
	"src/core/util/texture.cpp",
	"src/core/util/vector.cpp",
	"src/core/util/clipping_plane.cpp",
	"src/core/util/tmatrix.cpp",
	"src/core/box.cpp",
	"src/core/cone.cpp",
	"src/core/cylinder.cpp",
	"src/core/ellipsoid.cpp",
	"src/core/pyramid.cpp",
	"src/core/renderable.cpp",
	"src/core/render_core.cpp",
	"src/core/ring.cpp",
	"src/core/simple_displayobject.cpp",
	"src/core/sphere.cpp",
	"src/core/pmap_sphere.cpp",
	"src/core/frame.cpp",
	"src/core/label.cpp" ]

if sys.platform == 'win32':
	srcs.remove( 'src/core/label.cpp')
	srcs.append( 'src/win32/render_surface.cpp')
	srcs.append( 'src/win32/timer.cpp')
	
	core.Append( LIBS=['opengl32', 'gdi32', 'glu32', 'comctl32'])
	core.Append( CPPPATH='include/win32')
	core.Append( LDFLAGS='-mwindows')
else:
	srcs.append( 'src/gtk2/render_surface.cpp')
	srcs.append( 'src/gtk2/font.cpp')
	srcs.append( 'src/gtk2/file_texture.cpp')
	srcs.append( 'src/gtk2/timer.cpp')
	srcs.append( 'src/gtk2/random_device.cpp')
	
	core.ParseConfig( 'pkg-config --cflags --libs gtkglextmm-1.0 ftgl fontconfig')
	core.Append( LIBS=["GL", "GLU"])
	core.Append( CPPPATH='include/gtk2')

# Options specific to libvpython-core.so
vpython_core = core.SharedLibrary( 
	target = 'lib/vpython-core', 
	source = srcs )


################################################################################
# Build the test programs.
tests = core.Copy()
# TODO: Find out why ParseConfig doesn't honor PKG_CONFIG_PATH.
tests['ENV']['PKG_CONFIG_PATH'] = './lib/pkgconfig/'
# tests.ParseConfig( 'pkg-config --cflags --libs vpython-3.0')
# tests.ParseConfig( 'pkg-config --cflags --libs gtkglextmm-1.0')
tests.Append( LIBPATH='lib', LIBS=['vpython-core'])

tests.Program( target='bin/sphere_lod_test', source='src/test/sphere_lod_test.cpp')
tests.Program( target='bin/arrow_transparent_test', source='src/test/arrow_transparent_test.cpp')
tests.Program( target='bin/object_zsort_bench', source='src/test/object_zsort_bench.cpp')
tests.Program( target='bin/model_zsort_bench',  source='src/test/model_zsort_bench.cpp')
tests.Program( target='bin/sincos_matrix_bench', source='src/test/sincos_matrix_bench.cpp')
tests.Program( target='bin/sphere_texture_test', source='src/test/sphere_texture_test.cpp')
tests.Program( target='bin/arrow_scaling_test', source='src/test/arrow_scaling_test.cpp')
tests.Program( target='bin/sphere_transparent_test', source='src/test/sphere_transparent_test.cpp')
tests.Program( target='bin/texture_sharing_test', source='src/test/texture_sharing_test.cpp')
tests.Program( target='bin/box_test', source='src/test/box_test.cpp')
tests.Program( target='bin/cone_test', source='src/test/cone_test.cpp')
tests.Program( target='bin/cylinder_test', source='src/test/cylinder_test.cpp')
tests.Program( target='bin/ring_test', source='src/test/ring_test.cpp')
tests.Program( target='bin/pyramid_test', source='src/test/pyramid_test.cpp')
tests.Program( target='bin/ellipsoid_test', source='src/test/ellipsoid_test.cpp')
tests.Program( target='bin/psphere_texture_test', source='src/test/psphere_texture_test.cpp')
tests.Program( target='bin/selection_test', source='src/test/selection_test.cpp')
tests.Program( target='bin/conference_demo', source='src/test/conference_demo.cpp')
if sys.platform != 'win32':
	tests.Program( target='bin/label_test', source='src/test/label_test.cpp')
	tests.Program( target='bin/gtk_style_test', source='src/test/gtk_style_test.cpp')
