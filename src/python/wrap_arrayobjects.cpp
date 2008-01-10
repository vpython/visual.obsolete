// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

// This file currently requires 144 MB to compile (optimizing).

#include "python/curve.hpp"
#include "python/faces.hpp"
#include "python/convex.hpp"
#include "python/points.hpp"

#include <boost/python/class.hpp>
#include <boost/python/args.hpp>
#include <boost/python/overloads.hpp>


#
namespace cvisual {

namespace {
using namespace boost::python;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( faces_smooth_shade, 
	python::faces::smooth_shade, 0, 1)
}

void
wrap_arrayobjects()
{
	using namespace boost::python;
	using python::curve;

	void (curve::*append_v_r)( vector, rgb) = &curve::append;
	void (curve::*append_v)( vector) = &curve::append;
	
	class_<curve, bases<renderable> >( "curve")
		.def( init<const curve&>())
		.add_property( "radius", &curve::get_radius, &curve::set_radius)  // AKA thickness.
		.add_property( "antialias", &curve::get_antialias, &curve::set_antialias)
		.add_property( "retain", &curve::get_retain, &curve::set_retain)
		.def( "get_color", &curve::get_color)
		// The order of set_color specifications matters.
		.def( "set_color", &curve::set_color_t)
		.def( "set_color", &curve::set_color_l)
		.def( "set_color", &curve::set_color)
		.def( "set_red", &curve::set_red_l)
		.def( "set_red", &curve::set_red_d)
		.def( "set_red", &curve::set_red)
		.def( "set_green", &curve::set_green_l)
		.def( "set_green", &curve::set_green_d)
		.def( "set_green", &curve::set_green)
		.def( "set_blue", &curve::set_blue_l)
		.def( "set_blue", &curve::set_blue_d)
		.def( "set_blue", &curve::set_blue)
		.def( "set_pos", &curve::set_pos_v)
		.def( "get_pos", &curve::get_pos)
		.def( "set_pos", &curve::set_pos_l)
		.def( "set_pos", &curve::set_pos)
		.def( "set_x", &curve::set_x_l)
		.def( "set_x", &curve::set_x_d)
		.def( "set_x", &curve::set_x)
		.def( "set_y", &curve::set_y_l)
		.def( "set_y", &curve::set_y_d)
		.def( "set_y", &curve::set_y) 
		.def( "set_z", &curve::set_z_l)
		.def( "set_z", &curve::set_z_d)
		.def( "set_z", &curve::set_z)
		.def( "append", append_v_r, args( "pos", "color"))
		.def( "append", &curve::append_rgb,
			(args("pos"), args("r")=-1, args("g")=-1, args("b")=-1))
		.def( "append", append_v, args("pos"))
		;

	using python::faces;

	void (faces::* append_all_vectors)(vector, vector, rgb) = &faces::append;
	void (faces::* append_default_color)( vector, vector) = &faces::append;

	class_<faces, bases<renderable> >("faces")
		.def( init<const faces&>())
		.def( "append", append_default_color, args( "pos", "normal"))
		.def( "append", append_all_vectors, args("pos", "normal", "color"))
		.def( "get_pos", &faces::get_pos)
		.def( "set_pos", &faces::set_pos)
		.def( "set_pos", &faces::set_pos_l)
		.def( "get_normal", &faces::get_normal)
		.def( "set_normal", &faces::set_normal_l)
		.def( "set_normal", &faces::set_normal_v)
		.def( "set_normal", &faces::set_normal)
		.def( "get_color", &faces::get_color)
		.def( "set_color", &faces::set_color_l)
		.def( "set_color", &faces::set_color)
		.def( "set_color", &faces::set_color_t)
		.def( "smooth_shade", &faces::smooth_shade, 
			faces_smooth_shade( args("doublesided"),
			"Average normal vectors at coincident vertexes."))
		;
	
	using python::convex;
	class_<convex, bases<renderable> >( "convex")
		.def( init<const convex&>())
		.def( "append", &convex::append, args("pos"),
		 	"Append a point to the surface in O(n) time.")
		.add_property( "color", &convex::get_color, &convex::set_color)
		.def( "set_pos", &convex::set_pos)
		.def( "set_pos", &convex::set_pos_l)
		.def( "get_pos", &convex::get_pos)
		;  

	using python::points;

	void (points::*pappend_v_r)( vector, rgba) = &points::append;
	void (points::*pappend_v)( vector) = &points::append;
	
	class_<points, bases<renderable> >( "points")
		.def( init<const points&>())
		.add_property( "size", &points::get_size, &points::set_size)  // AKA thickness.
		.add_property( "antialias", &points::get_antialias, &points::set_antialias)
		.add_property( "shape", &points::get_points_shape, &points::set_points_shape)
		.add_property( "type", &points::get_size_type, &points::set_size_type)
		.def( "get_color", &points::get_color)
		// The order of set_color specifications matters.
		.def( "set_color", &points::set_color_t)
		.def( "set_color", &points::set_color_l)
		.def( "set_color", &points::set_color)
		.def( "set_red", &points::set_red_l)
		.def( "set_red", &points::set_red_d)
		.def( "set_red", &points::set_red)
		.def( "set_green", &points::set_green_l)
		.def( "set_green", &points::set_green_d)
		.def( "set_green", &points::set_green)
		.def( "set_blue", &points::set_blue_l)
		.def( "set_blue", &points::set_blue_d)
		.def( "set_blue", &points::set_blue)
		.def( "set_opacit", &points::set_opacity_l)
		.def( "set_opacity", &points::set_opacity_d)
		.def( "set_opacity", &points::set_opacity)
		.def( "set_pos", &points::set_pos_v)
		.def( "get_pos", &points::get_pos)
		.def( "set_pos", &points::set_pos_l)
		.def( "set_pos", &points::set_pos)
		.def( "set_x", &points::set_x_l)
		.def( "set_x", &points::set_x_d)
		.def( "set_x", &points::set_x)
		.def( "set_y", &points::set_y_l)
		.def( "set_y", &points::set_y_d)
		.def( "set_y", &points::set_y) 
		.def( "set_z", &points::set_z_l)
		.def( "set_z", &points::set_z_d)
		.def( "set_z", &points::set_z)
		.def( "append", pappend_v_r, args( "pos", "color"))
		.def( "append", &points::append_rgba,
			(args("pos"), args("r")=-1, args("g")=-1, args("b")=-1, args("a")=-1))
		.def( "append", pappend_v, args("pos"))
		;

}

} // !namespace cvisual
