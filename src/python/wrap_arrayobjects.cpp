// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

// This file currently requires 122 MB to compile (optimizing).

#include "python/curve.hpp"

#include <boost/python/class.hpp>
#include <boost/python/args.hpp>

namespace cvisual {

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
		.def( "get_color", &curve::get_color)
		.def( "set_color", &curve::set_color_l)
		.def( "set_color", &curve::set_color)
		.def( "set_color", &curve::set_color_t)
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
}

}
