// This file uses 195 MB to compile (optimizing)

// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "primitive.hpp"
#include "arrow.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "cone.hpp"
#include "ring.hpp"
#include "rectangular.hpp"
#include "box.hpp"
#include "ellipsoid.hpp"
#include "pyramid.hpp"
#include "label.hpp"
#include "frame.hpp"

#include <boost/python/class.hpp>

namespace cvisual {
using namespace boost::python;
using boost::noncopyable;

void
wrap_primitive()
{
	class_<renderable, boost::noncopyable>( "renderable", no_init);

	class_<primitive, bases<renderable>, noncopyable>( 
			"primitive", no_init)
		.add_property( "pos", 
			make_function(&primitive::get_pos, 
				return_internal_reference<>()), 
			&primitive::set_pos)
		.add_property( "x", &primitive::get_x, &primitive::set_x)
		.add_property( "y", &primitive::get_y, &primitive::set_y)
		.add_property( "z", &primitive::get_z, &primitive::set_z)
		.add_property( "axis", 
			make_function(&primitive::get_axis, 
				return_internal_reference<>()),
			&primitive::set_axis)
		.add_property( "up", 
			make_function(&primitive::get_up, 
				return_internal_reference<>()),
			&primitive::set_up)
		.add_property( "color", &primitive::get_color, &primitive::set_color)
		.add_property( "red", &primitive::get_red, &primitive::set_red)
		.add_property( "green", &primitive::get_green, &primitive::set_green)
		.add_property( "blue", &primitive::get_blue, &primitive::set_blue)
		.add_property( "alpha", &primitive::get_alpha, &primitive::set_alpha)		
		.add_property( "shininess", &primitive::get_shininess, &primitive::set_shininess)
		.add_property( "lit", &primitive::is_lit, &primitive::set_lit)
		.def( "rotate", &primitive::py_rotate1, args( "angle"))
		.def( "rotate", &primitive::py_rotate2, args( "angle", "axis"))
		.def( "rotate", &primitive::py_rotate3, args( "angle", "axis", "origin"))
		;
		
	class_<axial, bases<primitive>, noncopyable>( "axial", no_init)
		.add_property( "radius", &axial::get_radius, &axial::set_radius)
		;
	
	class_<rectangular, bases<primitive>, noncopyable>( "rectangular", no_init)
		.add_property( "length", &rectangular::get_length, &rectangular::set_length)
		.add_property( "width", &rectangular::get_width, &rectangular::set_width)
		.add_property( "height", &rectangular::get_height, &rectangular::set_height)
		.add_property( "size", &rectangular::get_size, &rectangular::set_size)
		;
		
	class_< arrow, bases<primitive>, noncopyable >("arrow")
		// .def( init<const visual::arrow&>())
		.add_property( "length", &arrow::get_length, &arrow::set_length)
		.add_property( "shaftwidth", &arrow::get_shaftwidth, &arrow::set_shaftwidth)
		.add_property( "headlength", &arrow::get_headlength, &arrow::set_headlength)
		.add_property( "headwidth", &arrow::get_headwidth, &arrow::set_headwidth)
		.add_property( "fixedwidth", &arrow::is_fixedwidth, &arrow::set_fixedwidth)
		;
	
	class_< sphere, bases<axial> >( "sphere")
		.def( init<const sphere&>())
		.add_property( "texture", &sphere::get_texture, &sphere::set_texture)
		;

	class_< cylinder, bases<axial> >( "cylinder")
		.def( init<const cylinder&>())
		;
	
	class_< cone, bases<axial> >( "cone")
		.def( init<const cone&>())
		;
		

	class_< ring, bases<primitive> >( "ring")
		.def( init<const ring&>())
		.add_property( "thickness", &ring::get_thickness, &ring::set_thickness)
		;

	class_< box, bases<rectangular> >( "box")
		.def( init<const box&>())
		;
	
	// Actually this inherits from sphere, but this avoids unwrapping the radius
	// member.
	class_< ellipsoid, bases<primitive> >( "ellipsoid")
		.def( init<const ellipsoid&>())
		.add_property( "width", &ellipsoid::get_width, &ellipsoid::set_width)
		.add_property( "height", &ellipsoid::get_height, &ellipsoid::set_height)
		.add_property( "length", &ellipsoid::get_length, &ellipsoid::set_length)
		.add_property( "size", &ellipsoid::get_size, &ellipsoid::set_size)
		;

	class_< pyramid, bases<rectangular> >( "pyramid")
		.def( init<const pyramid&>())
		;
		
	class_< label, bases<renderable> >( "label")
		.def( init<const label&>())
		.add_property( "color", &label::get_color, &label::set_color)
		.add_property( "red", &label::get_red, &label::set_red)
		.add_property( "green", &label::get_green, &label::set_green)
		.add_property( "blue", &label::get_blue, &label::set_blue)
		.add_property( "alpha", &label::get_alpha, &label::set_alpha)
		.add_property( "pos", 
			make_function(&label::get_pos, 
				return_internal_reference<>()), 
			&label::set_pos)
		.add_property( "x", &label::get_x, &label::set_x)
		.add_property( "y", &label::get_y, &label::set_y)
		.add_property( "z", &label::get_z, &label::set_z)
		.add_property( "height", &label::get_font_size, &label::set_font_size)
		.add_property( "xoffset", &label::get_xoffset, &label::set_xoffset)
		.add_property( "yoffset", &label::get_yoffset, &label::set_yoffset)
		.add_property( "opacity", &label::get_opacity, &label::set_opacity)
		.add_property( "border", &label::get_border, &label::set_border)
		.add_property( "box", &label::has_box, &label::render_box)
		.add_property( "line", &label::has_line, &label::render_line)
		.add_property( "linecolor", &label::get_linecolor, &label::set_linecolor)
		.add_property( "font", &label::get_font_family, &label::set_font_family)
		.add_property( "text", &label::get_text, &label::set_text)
		.add_property( "space", &label::get_space, &label::set_space)
		// .def( self_ns::str(self))
		;
		
	class_<frame, bases<renderable> >( "frame")
		.def( init<const frame&>())
		.add_property( "objects", &frame::get_objects)
		.add_property( "pos", 
			make_function(&frame::get_pos, 
				return_internal_reference<>()), 
			&frame::set_pos)
		.add_property( "x", &frame::get_x, &frame::set_x)
		.add_property( "y", &frame::get_y, &frame::set_y)
		.add_property( "z", &frame::get_z, &frame::set_z)
		.add_property( "axis", 
			make_function(&frame::get_axis, 
				return_internal_reference<>()),
			&frame::set_axis)
		.add_property( "up", 
			make_function(&frame::get_up, 
				return_internal_reference<>()),
			&frame::set_up)
		.def( "rotate", &frame::py_rotate1, args( "angle"))
		.def( "rotate", &frame::py_rotate2, args( "angle", "axis"))
		.def( "rotate", &frame::py_rotate3, args( "angle", "axis", "origin"))
		;
}	
	
} // !namespace cvisual
