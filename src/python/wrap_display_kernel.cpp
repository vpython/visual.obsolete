// This file requires 182 MB to compile (optimizing).

// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "display.hpp"
#include "mouseobject.hpp"
#include "util/errors.hpp"

#include <boost/python/class.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/def.hpp>
#include <boost/python/manage_new_object.hpp>
 
namespace cvisual {

// The function that is passed to Python to break out of an infinate loop.
static int
py_quit( void*)
{
	std::exit(0);
	return 0;
}

// The callback function that is invoked from the display class when 
// shutting-down.
static void
force_py_exit(void)
{
	VPYTHON_NOTE("Inserting the pending shutdown call...");
	PyGILState_STATE state = PyGILState_Ensure();
	Py_AddPendingCall( &py_quit, 0);
	PyGILState_Release(state);
	VPYTHON_NOTE( "The pending shutdown call has been entered.");
}

static void
wrap_shutdown(void)
{
	Py_BEGIN_ALLOW_THREADS
	gui_main::shutdown();
	Py_END_ALLOW_THREADS
}

static void
wrap_waitclosed(void)
{
	Py_BEGIN_ALLOW_THREADS
	gui_main::waitclosed();
	Py_END_ALLOW_THREADS
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

// The purpose of this class is to expose the signal-handling methods to Python.
class py_display_kernel : public display_kernel, public SigC::Object
{
 private:
	py::object gl_begin_cb;
	py::object gl_end_cb;
	py::object gl_swap_buffers_cb;

	void on_gl_begin()
	{
		if (gl_begin_cb.ptr() == Py_None) {
			PySys_WriteStderr( "***VPython CRITICAL ERROR***: "
				"No callback function registered for call to gl_begin().");
			return;
		}
		gl_begin_cb();
	}

	void on_gl_end()
	{
		if (gl_end_cb.ptr() == Py_None) {
			PySys_WriteStderr( "***VPython CRITICAL ERROR***: "
				"No callback function registered for call to gl_end().");
			return;
		}
		gl_end_cb();
	}
	void on_gl_swap_buffers()
	{
		if (gl_swap_buffers_cb.ptr() == Py_None) {
			PySys_WriteStderr( "***VPython CRITICAL ERROR***: "
				"No callback function registered for call to gl_swap_buffers().");
			return;
		}
		gl_swap_buffers_cb();
	}

	void verify_callable( py::object obj)
	{
		if (!PyCallable_Check( obj.ptr())) {
			PyErr_SetString( PyExc_TypeError, "Did not pass a callable object.");
			py::throw_error_already_set();
		}
	}

 public:
	py_display_kernel() 
		: display_kernel() 
	{
		gl_begin.connect( 
			SigC::slot( *this, &py_display_kernel::on_gl_begin));
		gl_end.connect( 
			SigC::slot( *this, &py_display_kernel::on_gl_end));
		gl_swap_buffers.connect( 
			SigC::slot( *this, &py_display_kernel::on_gl_swap_buffers));
	}
	
	void set_gl_begin_cb( py::object obj)
	{
		verify_callable( obj);
		gl_begin_cb = obj;
	}
	
	void set_gl_end_cb( py::object obj)
	{
		verify_callable( obj);
		gl_end_cb = obj;
	}
	
	void set_gl_swap_buffers_cb( py::object obj)
	{
		verify_callable( obj);
		gl_swap_buffers_cb = obj;
	}
	
	void report_mouse_motion( float dx, float dy, std::string button)
	{
		if (button.empty())
			return;
		switch (button.at(0)) {
			case 'l':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::LEFT);
				break;
			case 'r':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::LEFT);
				break;
			case 'm':
				display_kernel::report_mouse_motion( dx, dy, display_kernel::LEFT);
				break;
		}
	}
};

namespace {
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
		.add_property( "lights", &display_kernel::get_lights)
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
		.add_property( "show_renderspeed", 
			&display_kernel::is_showing_renderspeed,
			&display_kernel::set_show_renderspeed)
		.add_property( "renderspeed", &display_kernel::get_renderspeed) 
		.def( "_set_range", &display_kernel::set_range_d)
		.def( "_set_range", &display_kernel::set_range)
		.def( "_get_range", &display_kernel::get_range)
		;
		
	class_<py_display_kernel, bases<display_kernel>, noncopyable>
			( "display_kernel")
		.def( "set_gl_begin_cb", &py_display_kernel::set_gl_begin_cb)
		.def( "set_gl_end_cb", &py_display_kernel::set_gl_end_cb)
		.def( "set_gl_swap_buffers_cb", 
			&py_display_kernel::set_gl_swap_buffers_cb)
		// Functions for extending this type in Python.
		.def( "render_scene", &display_kernel::render_scene)
		.def( "report_realise", &display_kernel::report_realize)
		.def( "report_resize", &display_kernel::report_resize, 
			py::args( "width", "height"))
		.def( "report_mouse_motion", &py_display_kernel::report_mouse_motion)
		.def( "pick", &display_kernel::pick, pick_overloads( 
			py::args( "x", "y", "pixels")))
		;
	
	typedef atomic_queue<std::string> kb_object;
	py::class_< kb_object, noncopyable>( "kb_object", no_init)
		.def( "getkey", &kb_object::pop, "Returns the next key press value.")
		.add_property( "keys", &kb_object::size) 
		;
	
	py::class_<display, bases<display_kernel>, noncopyable>( "display")
		.add_property( "x", &display::get_x, &display::set_x)
		.add_property( "y", &display::get_y, &display::set_y)
		.add_property( "width", &display::get_width, &display::set_width)
		.add_property( "height", &display::get_height, &display::set_height)
		.add_property( "title", &display::get_title, &display::set_title)
		.add_property( "fullscreen", &display::is_fullscreen, 
			&display::set_fullscreen)
		.def( "set_selected", &display::set_selected)
		.staticmethod( "set_selected")
		.def( "get_selected", &display::get_selected)
		.staticmethod( "get_selected")
		.add_property( "visible", &display::get_visible, &display::set_visible)
		.add_property( "kb", py::make_function(
			&display::get_kb, py::return_internal_reference<>()))
		;
	
	py::to_python_converter<
		std::list<shared_ptr<renderable> >,
		renderable_objects_to_py_list>();
		
	py::to_python_converter<
		std::list<shared_ptr<light> >,
		lights_to_py_list>();
		
	// Free functions for exiting the system.  
	// These are undocumented at the moment, and are only used internally.
	def( "shutdown", wrap_shutdown, 
		"Close all Displays and shutdown visual.");
	def( "allclosed", gui_main::allclosed, 
		"Returns true if all of the Displays are closed.");
	def( "waitclose", wrap_waitclosed, 
		"Blocks until all of the Displays are closed by the user.");
		
	gui_main::on_shutdown.connect( SigC::slot( &force_py_exit));
	
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
		.add_property( "button", make_function(
			&mousebase::get_buttons, return_value_policy<manage_new_object>()))
		.add_property( "press",  make_function(
			&mousebase::get_press, return_value_policy<manage_new_object>()))
		.add_property( "release",  make_function(
			&mousebase::get_release, return_value_policy<manage_new_object>()))
		.add_property( "click",  make_function(
			&mousebase::get_click, return_value_policy<manage_new_object>()))
		.add_property( "drag",  make_function(
			&mousebase::get_drag, return_value_policy<manage_new_object>()))
		.add_property( "drop",  make_function(
			&mousebase::get_drop, return_value_policy<manage_new_object>()))
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
