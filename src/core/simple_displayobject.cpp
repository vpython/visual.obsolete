#include "simple_displayobject.hpp"
#include "util/errors.hpp"

#include <typeinfo>
#include <cmath>
#include <cxxabi.h>
#include <sstream>

tmatrix 
simple_displayobject::model_world_transform() const
{
	// Performs a scale, reorientation, and translation transform in that order.
	// ret = translation o reorientation o scale
	tmatrix ret;
	// A unit vector along the z_axis.
	vector z_axis = vector(0,0,1);
	if (std::fabs(axis.dot(up) / std::sqrt( up.mag2() * axis.mag2())) > 0.98) {
		// Then axis and up are in (nearly) the same direction: therefore,
		// try two other possible directions for the up vector.
		// But first, display an error message telling you of your mistake.
		std::ostringstream msg;
		msg << "axis and up are colinear for displayobject of type `";
		// Temporary variable to store the status code of the demangling routine.
		int status = 0;
		char* realname = abi::__cxa_demangle( this->get_typeid().name(), NULL, NULL, &status);
		msg << realname;
		free(realname);
		msg << "` at position " << pos << ".";
		VPYTHON_WARNING(msg.str());
		
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
simple_displayobject::world_model_transform() const
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
simple_displayobject::get_center() const
{
	return pos;
}

simple_displayobject::simple_displayobject()
	: axis(1,0,0), up(0,1,0), pos(0,0,0), shininess(1.0)
{
}

simple_displayobject::simple_displayobject( 
	const vector& n_pos, const vector& n_axis, const vector& n_up)
	: axis(n_axis), up(n_up), pos(n_pos), shininess( 1.0)
{
}

simple_displayobject::~simple_displayobject()
{
}

void 
simple_displayobject::set_pos( const vector& n_pos)
{
	model_damage();
	pos = n_pos;
}

void 
simple_displayobject::set_axis( const vector& n_axis)
{
	model_damage();
	if (color.alpha != 1.0)
		if (n_axis.norm() != axis.norm()) {
			z_damage();
		}
	axis = n_axis;
}

void 
simple_displayobject::set_up( const vector& n_up)
{
	model_damage();
	if (color.alpha != 1.0) {
		z_damage();
	}
	up = n_up;
}

void 
simple_displayobject::set_color( const rgba& n_color)
{
	model_damage();
	color = n_color;
}

void
simple_displayobject::set_shininess( const float s)
{
	model_damage();
	shininess = clamp( 0.0f, s, 1.0f);
}

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(simple_displayobject)
