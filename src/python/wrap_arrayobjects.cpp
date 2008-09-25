// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

// This file currently requires 144 MB to compile (optimizing).

#include "python/curve.hpp"
#include "python/faces.hpp"
#include "python/convex.hpp"
#include "python/points.hpp"

#include "python/num_util.hpp"
#include <boost/python/class.hpp>
#include <boost/python/args.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/detail/wrap_python.hpp>

#
namespace cvisual {

namespace {
using namespace boost::python;

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( faces_smooth_shade,
	python::faces::smooth_shade, 0, 1)
}

using python::double_array;

struct double_array_from_python {
	double_array_from_python() {
		boost::python::converter::registry::push_back(
				&convertible,
				&construct,
				boost::python::type_id< double_array >());
	}

	static void* convertible(PyObject* obj_ptr)
	{
		using namespace boost::python;

		// TODO: We are supposed to determine if construct will succeed.  But
		//   this is difficult and expensive for arbitrary sequences.  So we
		//   assume that anything that looks like a sequence will convert and
		//   throw an exception later.  This limits overload resolution, but
		//   most of our functions taking arrays have no overloads.
		// Legend has it that numpy arrays don't satisfy PySequence_Check so
		///  we check if x[0] succeeds.
		PyObject* p = PySequence_GetItem(obj_ptr, 0);
		if (!p) { PyErr_Clear(); return NULL; }
		Py_DECREF(p);

		return obj_ptr;
	}

	static void construct(
		PyObject* _obj,
		boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		using namespace boost::python;

		void* storage = (
			(boost::python::converter::rvalue_from_python_storage<double_array>*)
			data)->storage.bytes;

		Py_INCREF(_obj);
		PyObject* arr = PyArray_FromAny(_obj, PyArray_DescrFromType(NPY_DOUBLE), 0, 0, NPY_ENSUREARRAY|NPY_CONTIGUOUS|NPY_ALIGNED, NULL);
		if (!arr)
			throw std::invalid_argument("Object cannot be converted to array.");

		new (storage) double_array( handle<>(arr) );

		data->convertible = storage;
	}
};

void
wrap_arrayobjects()
{
	using namespace boost::python;
	using python::curve;

	double_array_from_python();

	void (curve::*append_v_rgb_retain)( vector, rgb, int) = &curve::append;
	void (curve::*append_v_rgb)( vector, rgb) = &curve::append;
	void (curve::*append_v_retain)( vector, int) = &curve::append;
	void (curve::*append_v)( vector) = &curve::append;

	class_<curve, bases<renderable> >( "curve")
		.def( init<const curve&>())
		.add_property( "radius", &curve::get_radius, &curve::set_radius)  // AKA thickness.
		.def( "get_color", &curve::get_color)
		// The order of set_color specifications matters.
		.def( "set_color", &curve::set_color_t)
		.def( "set_color", &curve::set_color)
		.def( "set_red", &curve::set_red_d)
		.def( "set_red", &curve::set_red)
		.def( "set_green", &curve::set_green_d)
		.def( "set_green", &curve::set_green)
		.def( "set_blue", &curve::set_blue_d)
		.def( "set_blue", &curve::set_blue)
		.def( "set_pos", &curve::set_pos_v)
		.def( "get_pos", &curve::get_pos)
		.def( "set_pos", &curve::set_pos)
		.def( "set_x", &curve::set_x_d)
		.def( "set_x", &curve::set_x)
		.def( "set_y", &curve::set_y_d)
		.def( "set_y", &curve::set_y)
		.def( "set_z", &curve::set_z_d)
		.def( "set_z", &curve::set_z)
		.def( "append", append_v_rgb_retain, args( "pos", "color", "retain"))
		.def( "append", append_v_rgb, args( "pos", "color"))
		.def( "append", append_v_retain, args( "pos", "retain"))
		.def( "append", &curve::append_rgb,
			(args("pos"), args("red")=-1, args("green")=-1, args("blue")=-1, args("retain")=-1))
		.def( "append", append_v, args("pos"))
		;

	using python::points;

	void (points::*pappend_v_r)( vector, rgb) = &points::append;
	void (points::*pappend_v)( vector) = &points::append;

	class_<points, bases<renderable> >( "points")
		.def( init<const points&>())
		.add_property( "size", &points::get_size, &points::set_size)  // AKA thickness.
		.add_property( "shape", &points::get_points_shape, &points::set_points_shape)
		.add_property( "size_units", &points::get_size_units, &points::set_size_units)
		.def( "get_color", &points::get_color)
		// The order of set_color specifications matters.
		.def( "set_color", &points::set_color_t)
		.def( "set_color", &points::set_color)
		.def( "set_red", &points::set_red_d)
		.def( "set_red", &points::set_red)
		.def( "set_green", &points::set_green_d)
		.def( "set_green", &points::set_green)
		.def( "set_blue", &points::set_blue_d)
		.def( "set_blue", &points::set_blue)
		.def( "set_pos", &points::set_pos_v)
		.def( "get_pos", &points::get_pos)
		.def( "set_pos", &points::set_pos)
		.def( "set_x", &points::set_x_d)
		.def( "set_x", &points::set_x)
		.def( "set_y", &points::set_y_d)
		.def( "set_y", &points::set_y)
		.def( "set_z", &points::set_z_d)
		.def( "set_z", &points::set_z)
		.def( "append", pappend_v_r, args( "pos", "color"))
		.def( "append", &points::append_rgb,
			(args("pos"), args("red")=-1, args("green")=-1, args("blue")=-1))
		.def( "append", pappend_v, args("pos"))
		;

	using python::faces;

	void (faces::* append_all_vectors)(vector, vector, rgb) = &faces::append;
	void (faces::* append_default_color)( vector, vector) = &faces::append;

	class_<faces, bases<renderable> >("faces")
		.def( init<const faces&>())
		.def( "get_pos", &faces::get_pos)
		.def( "set_pos", &faces::set_pos)
		.def( "get_normal", &faces::get_normal)
		.def( "set_normal", &faces::set_normal_v)
		.def( "set_normal", &faces::set_normal)
		.def( "get_color", &faces::get_color)
		.def( "set_color", &faces::set_color)
		.def( "set_color", &faces::set_color_t)
		.def( "smooth_shade", &faces::smooth_shade,
			faces_smooth_shade( args("doublesided"),
			"Average normal vectors at coincident vertexes."))
		.def( "append", &faces::append_rgb,
			(args("pos"), args("normal"), args("red")=-1, args("green")=-1, args("blue")=-1))
		.def( "append", append_default_color, args( "pos", "normal"))
		.def( "append", append_all_vectors, args("pos", "normal", "color"))
		;

	using python::convex;
	class_<convex, bases<renderable> >( "convex")
		.def( init<const convex&>())
		.def( "append", &convex::append, args("pos"),
		 	"Append a point to the surface in O(n) time.")
		.add_property( "color", &convex::get_color, &convex::set_color)
		.def( "set_pos", &convex::set_pos)
		.def( "get_pos", &convex::get_pos)
		;

}

} // !namespace cvisual
