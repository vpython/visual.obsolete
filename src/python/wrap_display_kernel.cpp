#include "display_kernel.hpp"
#include "display.hpp"

#include <boost/python/class.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_function.hpp>

namespace cvisual {
namespace py = boost::python;

// This function automatically converts the list of renderable objects to a 
// Python list.
struct renderable_objects_to_py_list
{
	static PyObject* convert( const std::list<shared_ptr<renderable> >& items)
	{
		py::list ret;
		for (std::list<shared_ptr<renderable> >::const_iterator i = items.begin();
				i != items.end(); ++i) {
			ret.append( py::object( *i));	
		}
		Py_INCREF(ret.ptr());
		return ret.ptr();
	}
};

// This function automatically converts the std::list of renderable objects to a
// Python list.
struct lights_to_py_list
{
	static PyObject* convert( const std::list<shared_ptr<light> >& lights)
	{
		py::list ret;
		for (std::list<shared_ptr<light> >::const_iterator i = lights.begin();
				i != lights.end(); ++i) {
			ret.append( py::object( *i));	
		}
		Py_INCREF( ret.ptr());
		return ret.ptr();
	}
};

namespace {
using namespace boost::python;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( pick_overloads, display_kernel::pick, 2,3)
}

void
wrap_display_kernel(void)
{
	// TODO: Wrap around SigC::Signal0<void>
	py::class_<display_kernel, boost::noncopyable>( "display_kernel")
		// Functions for the internal use of renderable and light classes.
		.def( "add_renderable", &display_kernel::add_renderable)
		.def( "add_renderable_screen", &display_kernel::add_renderable_screen)
		.def( "remove_renderable", &display_kernel::remove_renderable)
		.add_property( "objects", &display_kernel::get_objects)
		.def( "add_light", &display_kernel::add_light)
		.def( "remove_light", &display_kernel::remove_light)
		.add_property( "lights", &display_kernel::get_lights)
		// Functions for extending this type in Python.
		.def( "render_scene", &display_kernel::render_scene)
		.def( "report_realise", &display_kernel::report_realize)
		.def( "report_resize", &display_kernel::report_resize, py::args( "width", "height"))
		.def( "pick", &display_kernel::pick, pick_overloads( py::args( "x", "y", "pixels")))
		.def( "illuminate_default", &display_kernel::illuminate_default)
		.add_property( "ambient", &display_kernel::get_ambient, 
			&display_kernel::set_ambient)
		.add_property( "up",
			py::make_function( &display_kernel::get_up, 
				py::return_internal_reference<>()),
			&display_kernel::set_up)
		.add_property( "forward",
			py::make_function( &display_kernel::get_forward, 
				py::return_internal_reference<>()),
			&display_kernel::set_forward)
		.add_property( "scale",
			py::make_function( &display_kernel::get_scale, 
				py::return_internal_reference<>()),
			&display_kernel::set_scale)
		.add_property( "center",
			py::make_function( &display_kernel::get_center, 
				py::return_internal_reference<>()),
			&display_kernel::set_center)
		.add_property( "fov", &display_kernel::get_fov, &display_kernel::set_fov)
		.add_property( "uniform", &display_kernel::is_uniform, 
			&display_kernel::set_uniform)
		.add_property( "background", &display_kernel::get_background,
			&display_kernel::set_background)
		.add_property( "forground", &display_kernel::get_forground,
			&display_kernel::set_forground)
		.add_property( "autoscale", &display_kernel::get_autoscale, 
			&display_kernel::set_autoscale)
		.add_property( "autocenter", &display_kernel::get_autocenter,
			&display_kernel::set_autocenter)
		.add_property( "stereo", &display_kernel::get_stereomode,
			&display_kernel::set_stereomode)
		;
	
	py::class_<display, boost::noncopyable>( "display")
		.add_property( "x", &display::get_x, &display::set_x)
		.add_property( "y", &display::get_y, &display::set_y)
		.add_property( "width", &display::get_width, &display::set_width)
		.add_property( "height", &display::get_height, &display::set_height)
		.add_property( "title", &display::get_title, &display::set_title)
		// .add_property( "fullscreen", &display::is_fullscreen, &display::set_fullscreen)
		.def( "set_selected", &display::set_selected)
		.staticmethod( "set_selected")
		.def( "get_selected", &display::get_selected)
		.staticmethod( "get_selected")
		.add_property( "visible", &display::get_visible, &display::set_visible)
		;
	
	py::to_python_converter<
		std::list<shared_ptr<renderable> >,
		renderable_objects_to_py_list>();
		
	py::to_python_converter<
		std::list<shared_ptr<light> >,
		lights_to_py_list>();
};

} // !namespace cvisual;
