SetOption( "implicit_cache", 1)

# Options common to all targets go in opt
opt = Environment( CCFLAGS=['-g', '-march=pentium3'])
opt.Append( CPPPATH=['include', '/usr/include/FTGL'])

# Options specific to libvpython-core.so
core = opt.Copy()
core.ParseConfig( 'pkg-config --cflags --libs sigc++-1.2 ftgl')
core.Append( LIBS=["GL", "GLU"])
vpython_core = core.SharedLibrary( 
	target = 'lib/vpython-core', 
	source = [ "src/core/arrow.cpp", 
	"src/core/util/displaylist.cpp",
	"src/core/util/errors.cpp",
	"src/core/util/extent.cpp",
	"src/core/util/lighting.cpp",
	"src/core/util/quadric.cpp",
	"src/core/util/random_device.cpp",
	"src/core/util/rgba.cpp",
	"src/core/util/texture.cpp",
	"src/core/util/timer.cpp",
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
	"src/core/pmap_sphere.cpp" ] )

gtk2 = opt.Copy()
gtk2.Append( LIBPATH='lib', LIBS='vpython-core')
gtk2.ParseConfig( 'pkg-config --cflags --libs gtkglextmm-1.0')
vpython_gtk2 = gtk2.SharedLibrary( 
	target = 'lib/vpython-gtk2',
	source = ['src/gtk2/file_texture.cpp', 
	'src/gtk2/render_surface.cpp' ])


################################################################################
# Build the test programs.
tests = opt.Copy()
# TODO: Find out why ParseConfig doesn't honor PKG_CONFIG_PATH.
tests['ENV']['PKG_CONFIG_PATH'] = './lib/pkgconfig/'
# tests.ParseConfig( 'pkg-config --cflags --libs vpython-3.0')
tests.ParseConfig( 'pkg-config --cflags --libs gtkglextmm-1.0')
tests.Append( LIBPATH='lib', LIBS=['vpython-core', 'vpython-gtk2'])
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
