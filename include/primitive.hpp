#ifndef VPYTHON_SIMPLE_DISPLAYOBJECT_HPP
#define VPYTHON_SIMPLE_DISPLAYOBJECT_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/tmatrix.hpp"

#include <typeinfo>

namespace cvisual {

// All implementing base classes should use this pair of macros to help with standard
// error messages.  This allows functions to use the exact name of a virtual class.
#define PRIMITIVE_TYPEINFO_DECL virtual const std::type_info& get_typeid() const
#define PRIMITIVE_TYPEINFO_IMPL(base) \
	const std::type_info& \
	base::get_typeid() const \
	{ return typeid(*this); }

class primitive : public renderable
{
 protected:
	// The position and orientation of the body in World space.
	shared_vector axis;
	shared_vector up;
	shared_vector pos;
	float shininess;
	bool lit;

	// Returns a tmatrix that performs reorientation of the object from model
	// orientation to world (and view) orientation.
	tmatrix model_world_transform() const;
	// Returns a tmatrix that transforms coordinates from world space to
	// model space, for ray intersection calculations.
	tmatrix world_model_transform() const;
 
	// Generate a displayobject at the origin, with up pointing along +y and
	// an axis = vector(1, 0, 0).
	primitive();
	primitive( const vector& pos, const vector& axis, const vector& up);
	primitive( const primitive& other);
	
	// See above for PRIMITIVE_TYPEINFO_DECL/IMPL.
	virtual const std::type_info& get_typeid() const;
	
	// Used when obtaining the center of the body.
	virtual vector get_center() const;
	
	void rotate( double angle, const vector& axis, const vector& origin);

 public:
	virtual ~primitive();

	// Manually overload this member since the default arguments are variables.
	void py_rotate1( double angle);
	void py_rotate2( double angle, vector axis);
	void py_rotate3( double angle, vector axis, vector origin);
    void py_rotate4( double angle, vector origin);

	void set_pos( const vector& n_pos);
	shared_vector& get_pos();
	
	void set_x( double x);
	double get_x();
	
	void set_y( double y);
	double get_y();
	
	void set_z( double z);
	double get_z();
	
	void set_axis( const vector& n_axis);
	shared_vector& get_axis();
	
	void set_up( const vector& n_up);
	shared_vector& get_up();
	
	void set_color( const rgba& n_color);
	rgba get_color();
	
	void set_red( double x);
	double get_red();
	
	void set_green( double x);
	double get_green();
	
	void set_blue( double x);
	double get_blue();
	
	void set_alpha( double x);
	double get_alpha();
	
	void set_shininess( float);
	float get_shininess();
	
	void set_lit(bool);
	bool is_lit();
};

} // !namespace cvisual

#endif // !defined VPYTHON_SIMPLE_DISPLAYOBJECT_HPP
