// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "primitive.hpp"
#include "util/errors.hpp"

#include <typeinfo>
#include <cmath>

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

#include <sstream>

namespace cvisual {

tmatrix 
primitive::model_world_transform() const
{
	// Performs a scale, reorientation, and translation transform in that order.
	// ret = translation o reorientation o scale
	tmatrix ret;
	// A unit vector along the z_axis.
	vector z_axis = vector(0,0,1);
	if (std::fabs(axis.dot(up) / std::sqrt( up.mag2() * axis.mag2())) > 0.98) {
		// Then axis and up are in (nearly) the same direction: therefore,
		// try two other possible directions for the up vector.
		if (std::fabs(axis.norm().dot( vector(-1,0,0))) > 0.98)
			z_axis = axis.cross( vector(0,0,1)).norm();
		else
			z_axis = axis.cross( vector(-1,0,0)).norm();
	}
	else {
		z_axis = axis.cross( up).norm();
	}
	
	vector y_axis = z_axis.cross(axis).norm();
	vector x_axis = axis.norm();
	ret.x_column( x_axis);
	ret.y_column( y_axis);
	ret.z_column( z_axis);
	ret.w_column( /*pos*/);
	ret.w_row();
	return ret;
}

// TODO: reevaluate this function in the context of object selection code.
tmatrix
primitive::world_model_transform() const
{
	// this performs the inverse of the model_world_transform - 
	// translate backwards, reorinetation, and inverse scale.
	// ret = reorient^T o translate_backwards.
	
	tmatrix ret;
	vector z_axis( 0, 0, 1);
	if (std::fabs(axis.norm().dot( up.norm())) > 0.98) {
		if (std::fabs(axis.norm().dot( vector(-1,0,0))) > 0.98)
			z_axis = axis.cross( vector(0,0,1)).norm();
		else
			z_axis = axis.cross( vector(-1,0,0)).norm();
	}
	else {
		z_axis = axis.cross( up).norm();
	}
	vector y_axis = z_axis.cross(axis).norm();
	vector x_axis = axis.norm();
	// Since the reorientation matrix is orthogonal, its inverse is simply the
	// transpose.  Multiplying each of the rows above by the scaling factor
	// performs the left-multipy by the scaling matrix.
	ret.x_column( x_axis.x, y_axis.x, z_axis.x);
	ret.y_column( x_axis.y, y_axis.y, z_axis.y);
	ret.z_column( x_axis.z, y_axis.z, z_axis.z);
	ret.w_column();
	ret.w_row();
	
	// Perform an inverse translation - simply right-multipy ret by a translate
	// matrix that is the negative of the original.
	ret.w_column( 
		-x_axis.dot( pos),
		-y_axis.dot( pos),
		-z_axis.dot( pos));
	
	return ret;
}

// For oblong objects whose center is not at "pos".
vector
primitive::get_center() const
{
	return pos;
}

primitive::primitive()
	: axis(mtx, 1,0,0), up(mtx, 0,1,0), pos(mtx, 0,0,0)
{
}

primitive::primitive( const primitive& other)
	: renderable( other), axis(mtx, other.axis), up( mtx, other.up), 
		pos(mtx, other.pos)
{
}

primitive::~primitive()
{
}

void
primitive::rotate( double angle, const vector& _axis, const vector& origin)
{
	tmatrix R = rotation( angle, _axis, origin);
	vector fake_up = up;
	if (!axis.cross( fake_up)) {
		fake_up = vector( 1,0,0);
		if (!axis.cross( fake_up))
			fake_up = vector( 0,1,0);
	}
    {
        lock L(mtx);
    	pos.assign_locked( R * pos);
    	axis.assign_locked( R.times_v( axis));
    	up.assign_locked( R.times_v( fake_up));
    }
}

void 
primitive::set_pos( const vector& n_pos)
{
	model_damage();
	pos = n_pos;
}

shared_vector&
primitive::get_pos()
{
	return pos;
}

void
primitive::set_x( double x)
{
	pos.set_x( x);
}

double
primitive::get_x()
{
	return pos.x;
}

void
primitive::set_y( double y)
{
	pos.set_y( y);
}

double
primitive::get_y()
{
	return pos.y;
}

void
primitive::set_z( double z)
{
	pos.set_z( z);
}

double
primitive::get_z()
{
	return pos.z;
}

void 
primitive::set_axis( const vector& n_axis)
{
	model_damage();
	if (color.opacity != 1.0)
		if (n_axis.norm() != axis.norm()) {
			z_damage();
		}
	axis = n_axis;
}

shared_vector&
primitive::get_axis()
{
	return axis;
}

void 
primitive::set_up( const vector& n_up)
{
	model_damage();
	if (color.opacity != 1.0) {
		z_damage();
	}
	up = n_up;
}

shared_vector&
primitive::get_up()
{
	return up;
}

void 
primitive::set_color( const rgba& n_color)
{
	lock L(mtx);
	model_damage();
	color = n_color;
}

void
primitive::set_red( float r)
{
	lock L(mtx);
	color.red = r;
}

double
primitive::get_red()
{
	return color.red;
}

void
primitive::set_green( float g)
{
	lock L(mtx);
	color.green = g;
}

double
primitive::get_green()
{
	return color.green;
}

void
primitive::set_blue( float b)
{
	lock L(mtx);
	color.blue = b;
}

double
primitive::get_blue()
{
	return color.blue;
}

void
primitive::set_opacity( float a)
{
	lock L(mtx);
	color.opacity = a;
}

double
primitive::get_opacity()
{
	return color.opacity;
}

rgba
primitive::get_color()
{
	return color;
}

PRIMITIVE_TYPEINFO_IMPL(primitive)

} // !namespace cvisual
