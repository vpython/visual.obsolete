// This file requires 182 MB to compile (optimizing).

// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "display.hpp"
#include "mouseobject.hpp"
#include "util/errors.hpp"
#include "python/gil.hpp"

#include <boost/bind.hpp>
#include <boost/python/class.hpp>
#include <boost/python/call_method.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/def.hpp>
#include <boost/python/manage_new_object.hpp>

namespace cvisual {

// The callback function that is invoked from the display class when
// shutting-down. Prior to Jan. 23, 2008, force_py_exit locked and posted
// a callback to exit, but this failed if the program was sitting
// at scene.mouse.getclick(). Simply calling exit directly seems
// to work fine.
static void
force_py_exit(void)
{
	VPYTHON_NOTE("Exiting");
	std::exit(0);
}

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

// This function automatically converts the std::list of light objects to a
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

// I must not know how to use Boost.Python yet, because I need this:
class py_base_display_kernel : public display_kernel {};

// A display implemented in python (e.g. to use PyOpenGL or PyObjC)
class py_display_kernel : public py_base_display_kernel
{
 public:
	PyObject* self;
	
	py_display_kernel( PyObject* self ) : self(self) {}
 
	// Delegates key display_kernel virtual methods to Python
	virtual void activate( bool active ) { boost::python::call_method<void>( self, "_activate", active ); }
	virtual EXTENSION_FUNCTION getProcAddress( const char* name ) { return (EXTENSION_FUNCTION)boost::python::call_method<intptr_t>( self, "_getProcAddress", name ); }
	intptr_t base_getProcAddress( const char* name ) { return (intptr_t)display_kernel::getProcAddress(name); }
 
	// Utility methods for Python subclasses
	void report_mouse_motion( float dx, float dy, std::string button)
	{
		if (button.empty())
			return;
		switch (button.at(0)) {
			case 'l':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::LEFT);
				break;
			case 'r':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::RIGHT);
				break;
			case 'm':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::MIDDLE);
				break;
		}
	}
};

namespace {

boost::python::object
get_buttons( const mousebase* This)
{
	std::string* ret = This->get_buttons();
	if (ret) {
		boost::python::object py_ret( *ret);
		delete ret;
		return py_ret;
	}
	else {
		return boost::python::object();
	}
}

template <bool (mousebase:: *f)(void) const>
boost::python::object
test_state( const mousebase* This)
{
	if ((This->*f)()) {
		return get_buttons(This);
	}
	else
		return boost::python::object();
}

using namespace boost::python;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( pick_overloads, display_kernel::pick,
	2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( mousebase_project_partial_overloads,
	mousebase::project2, 1, 2)
} // !namespace (unnamed)

void
wrap_display_kernel(void)
{
	using boost::noncopyable;

	py::class_<display_kernel, noncopyable>( "_display_kernel", no_init)
		// Functions for the internal use of renderable and light classes.
		.def( "add_renderable", &display_kernel::add_renderable)
		.def( "remove_renderable", &display_kernel::remove_renderable)
		.add_property( "objects", &display_kernel::get_objects)
		.def( "add_light", &display_kernel::add_light)
		.def( "remove_light", &display_kernel::remove_light)
		.def( "_get_lights", &display_kernel::get_lights)
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
		.add_property( "scale",	&display_kernel::get_scale,
			&display_kernel::set_scale)
		.add_property( "center",
			py::make_function( &display_kernel::get_center,
				py::return_internal_reference<>()),
			&display_kernel::set_center)
		.add_property( "fov", &display_kernel::get_fov, &display_kernel::set_fov)
		.add_property( "lod", &display_kernel::get_lod, &display_kernel::set_lod)
		.add_property( "uniform", &display_kernel::is_uniform,
		 	&display_kernel::set_uniform)
		.add_property( "background", &display_kernel::get_background,
			&display_kernel::set_background)
		.add_property( "foreground", &display_kernel::get_forground,
			&display_kernel::set_forground)
		.add_property( "autoscale", &display_kernel::get_autoscale,
			&display_kernel::set_autoscale)
		.add_property( "autocenter", &display_kernel::get_autocenter,
			&display_kernel::set_autocenter)
		.add_property( "stereo", &display_kernel::get_stereomode,
			&display_kernel::set_stereomode)
		.add_property( "show_rendertime",
			&display_kernel::is_showing_rendertime,
			&display_kernel::set_show_rendertime)
		.add_property( "userspin", &display_kernel::spin_is_allowed,
			&display_kernel::allow_spin)
		.add_property( "userzoom", &display_kernel::zoom_is_allowed,
			&display_kernel::allow_zoom)
		.def( "info", &display_kernel::info)
		.add_property( "x", &display_kernel::get_x, &display_kernel::set_x)
		.add_property( "y", &display_kernel::get_y, &display_kernel::set_y)
		.add_property( "width", &display_kernel::get_width, &display_kernel::set_width)
		.add_property( "height", &display_kernel::get_height, &display_kernel::set_height)
		.add_property( "title", &display_kernel::get_title, &display_kernel::set_title)
		.add_property( "fullscreen", &display_kernel::is_fullscreen,
			&display_kernel::set_fullscreen)
		.add_property( "toolbar", &display_kernel::is_showing_toolbar,
			&display_kernel::set_show_toolbar)
		.add_property( "visible", &display_kernel::get_visible, &display_kernel::set_visible)
		.add_property( "exit", &display_kernel::get_exit, &display_kernel::set_exit)
		.add_property( "kb", py::make_function(
			&display_kernel::get_kb, py::return_internal_reference<>()))
		.add_property( "mouse", py::make_function(
			&display_kernel::get_mouse, py::return_internal_reference<>()))
		.def( "set_selected", &display_kernel::set_selected)
		.staticmethod( "set_selected")
		.def( "get_selected", &display_kernel::get_selected)
		.staticmethod( "get_selected")

		.def( "_set_range", &display_kernel::set_range_d)
		.def( "_set_range", &display_kernel::set_range)
		.def( "_get_range", &display_kernel::get_range)
		.add_property( "_shader", &display_kernel::get_shader, &display_kernel::set_shader)
		;

	class_<py_base_display_kernel, py_display_kernel, bases<display_kernel>, noncopyable>
			( "display_kernel")
		// Default implementations of key override methods
		.def( "_getProcAddress", &py_display_kernel::base_getProcAddress )
		// Functions for extending this type in Python.
		.def( "render_scene", &display_kernel::render_scene)
		.def( "report_resize", &display_kernel::report_resize)
		.def( "report_mouse_motion", &py_display_kernel::report_mouse_motion)
		.def( "pick", &display_kernel::pick, pick_overloads(
			py::args( "x", "y", "pixels")))
		;

	typedef atomic_queue<std::string> kb_object;
	py::class_< kb_object, noncopyable>( "kb_object", no_init)
		.def( "getkey", &kb_object::py_pop, "Returns the next key press value.")
		.add_property( "keys", &kb_object::size)
		;

	py::class_<display, bases<display_kernel>, noncopyable>( "display")
		;

	py::def( "_set_dataroot", &display::set_dataroot);

	py::to_python_converter<
		std::list<shared_ptr<renderable> >,
		renderable_objects_to_py_list>();

	py::to_python_converter<
		std::list<shared_ptr<light> >,
		lights_to_py_list>();

	// Free functions for exiting the system.
	// These are undocumented at the moment, and are only used internally.
	def( "waitclose", &display_kernel::waitWhileAnyDisplayVisible,
		"Blocks until all of the Displays are closed by the user.");

	gui_main::on_shutdown.connect( &force_py_exit );

	class_<mousebase>( "clickbase", no_init)
		.def( "project", &mousebase::project2,
			mousebase_project_partial_overloads( args("normal", "point")))
		.def( "project", &mousebase::project1, args("normal", "d"),
			"project the mouse pointer to the plane specified by the normal "
			"vector 'normal' that is either:\n-'d' distance away from the origin"
			" ( click.project( normal=vector, d=scalar)), or\n-includes the "
			"point 'point' ( click.project( normal=vector, point=vector))\n"
			"or passes through the origin ( click.project( normal=vector)).")
		.add_property( "pos", &mousebase::get_pos)
		.add_property( "pick", &mousebase::get_pick)
		.add_property( "pickpos", &mousebase::get_pickpos)
		.add_property( "camera", &mousebase::get_camera)
		.add_property( "ray", &mousebase::get_ray)
		.add_property( "button", &get_buttons)
		.add_property( "press", &test_state<&mousebase::is_press>)
		.add_property( "release", &test_state<&mousebase::is_release>)
		.add_property( "click", &test_state<&mousebase::is_click>)
		.add_property( "drag", &test_state<&mousebase::is_drag>)
		.add_property( "drop", &test_state<&mousebase::is_drop>)
		.add_property( "shift", &mousebase::is_shift)
		.add_property( "alt", &mousebase::is_alt)
		.add_property( "ctrl", &mousebase::is_ctrl)
		;


	class_< event, boost::shared_ptr<event>, bases<mousebase>, noncopyable>
		( "click_object", "An event generated from mouse actions.", no_init)
		;

	class_< mouse_t, boost::shared_ptr<mouse_t>, bases<mousebase>, noncopyable>
		( "mouse_object", "This class provides access to the mouse.", no_init)
		.def( "getclick", &mouse_t::pop_click)
		.add_property( "clicked", &mouse_t::num_clicks)
		.def( "getevent", &mouse_t::pop_event)
		.add_property( "events", &mouse_t::num_events, &mouse_t::clear_events)
		;

}

} // !namespace cvisual;
