#ifndef VPYTHON_CONE_HPP
#define VPYTHON_CONE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "primitive.hpp"

namespace cvisual {

class cone : public primitive
{
 private:
	// The radius of the base of the cone.
	double radius;
	static bool first;
	bool degenerate();
	
 public:
	cone();
	void set_radius( double r);
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	virtual void update_cache( const view&);
	virtual void grow_extent( extent&);
	virtual vector get_center() const;
	PRIMITIVE_TYPEINFO_DECL;
};

} // !namespace cvisual

#endif // !defined VPYTHON_CONE_HPP
