#ifndef VPYTHON_SIMPLE_DISPLAYOBJECT_HPP
#define VPYTHON_SIMPLE_DISPLAYOBJECT_HPP

#include "renderable.hpp"
#include "util/tmatrix.hpp"

#include <typeinfo>

// All implementing base classes should use this pair of macros to help with standard
// error messages.  This allows functions to use the exact name of a virtual class.
#define SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL virtual const std::type_info& get_typeid() const
#define SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(base) \
	const std::type_info& \
	base::get_typeid() const \
	{ return typeid(*this); }

class simple_displayobject : public renderable
{
 protected:
	// The position and orientation of the body in World space.
	vector axis;
	vector up;
	vector pos;
	float shininess;

	// Returns a tmatrix that performs reorientation of the object from model
	// orientation to world (and view) orientation.
	tmatrix model_world_transform() const;
	// Returns a tmatrix that transforms coordinates from world space to
	// model space, for ray intersection calculations.
	tmatrix world_model_transform() const;
 
	// Generate a displayobject at the origin, with up pointing along +y and
	// an axis = vector(1, 0, 0).
	simple_displayobject();
	simple_displayobject( const vector& pos, const vector& axis, const vector& up);
	// See above for SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL/IMPL.
	virtual const std::type_info& get_typeid() const;
	
	// Used when obtaining the center of the body.
	virtual vector get_center() const;

 public:
	virtual ~simple_displayobject();
	void set_pos( const vector& n_pos);
	void set_axis( const vector& n_axis);
	void set_up( const vector& n_up);
	void set_color( const rgba& n_color);
	void set_shininess( float);
};

#endif // !defined VPYTHON_SIMPLE_DISPLAYOBJECT_HPP
