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
}	
	
} // !namespace cvisual
