import os
import sys

SetOption( "implicit_cache", 1)

# TODO: Add support to build a gch for wrap_gl.hpp, render_surface.hpp,
# and simple_displayobject.hpp


core = Environment( CCFLAGS=['-pipe', '-g'],
	ENV = os.environ,
	CPPPATH='include')

# Workaround brain-dead behavior in FTGL's header file layout
if sys.platform.find('linux') >= 0:
	core.Append( CPPPATH=['/usr/include/FTGL'])

# Crank up the warnings
core.Append( CCFLAGS=['-Wall', '-W', '-Wsign-compare', '-Wconversion',
	'-Wdisabled-optimization', '-D_GLIBCPP_CONCEPT_CHECKS'] )

# Add compiler flags for threading support.
if sys.platform == 'win32':
	core.Append( CCFLAGS='-mthreads')
else:
	core.Append( CCFLAGS='-pthread')

core.ParseConfig( 'pkg-config --cflags --libs sigc++-1.2')
core.Append( LIBS='gle')

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
	"src/core/util/checksum.cpp",
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
	"src/core/label.cpp",
	"src/core/curve.cpp",
	"src/core/convex.cpp",
	"src/core/faces.cpp" ]

if sys.platform == 'win32':
	srcs.append( 'src/win32/render_surface.cpp')
	srcs.append( 'src/win32/timer.cpp')
	srcs.append( 'src/win32/random_device.cpp')
	srcs.append( 'src/win32/font.cpp')
	# TODO: Write a file_texture.cpp implementation for Windows,
	
	core.Append( LIBS=['vpython-core', 'opengl32', 'gdi32', 'glu32', 'comctl32', 'crypt32'])
	core.Append( CPPPATH='include/win32')
	libname = 'bin/vpython-core'
else:
	srcs.append( 'src/gtk2/render_surface.cpp')
	srcs.append( 'src/gtk2/font.cpp')
	srcs.append( 'src/gtk2/file_texture.cpp')
	srcs.append( 'src/gtk2/timer.cpp')
	srcs.append( 'src/gtk2/random_device.cpp')
	
	core.ParseConfig( 'pkg-config --cflags --libs gtkglextmm-1.0 ftgl fontconfig')
	core.Append( LIBS=["GL", "GLU"])
	core.Append( CPPPATH='include/gtk2')
	libname='lib/vpython-core'

# Build libvpython-core.{so,dll}
vpython_core = core.SharedLibrary( 
	target = libname, 
	source = srcs )


################################################################################
# Build the test programs.
tests = core.Copy()
# TODO: Find out why ParseConfig doesn't honor PKG_CONFIG_PATH.
tests['ENV']['PKG_CONFIG_PATH'] = './lib/pkgconfig/'
# tests.ParseConfig( 'pkg-config --cflags --libs vpython-3.0')
tests.Append( LIBPATH='lib', LIBS=['vpython-core'])
if sys.platform == 'win32':
	tests.Append( LDFLAGS='-mwindows')
	main = 'src/win32/winmain.cpp'
else:
	main = 'src/gtk2/main.cpp'

def Test( name):
	tests.Program( target='bin/' + name, 
		source=['src/test/' + name + '.cpp', main])

Test('sphere_lod_test')
Test('arrow_transparent_test')
Test('sphere_texture_test')
Test('arrow_scaling_test')
Test('sphere_transparent_test')
Test('texture_sharing_test')
Test('box_test')
Test('cone_test')
Test('cylinder_test')
Test('ring_test')
Test('pyramid_test')
Test('ellipsoid_test')
Test('psphere_texture_test')
Test('selection_test')
# Test('conference_demo')
Test('curve_test')
Test('label_test')
Test('convex_test')
if sys.platform != 'win32':
	Test('gtk_style_test')
	
main = "src/gtk2/main.cpp"
Test('object_zsort_bench')
Test('model_zsort_bench')
Test('sincos_matrix_bench')
