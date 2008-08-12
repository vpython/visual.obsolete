#ifndef VPYTHON_UTIL_CLIPPING_PLANE_HPP
#define VPYTHON_UTIL_CLIPPING_PLANE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/vector.hpp"

namespace cvisual {

class clipping_plane
{
 private:
	// Represents an equation in the form Ax + By + Cz + D = 0 (the format that
	// OpenGL requires).
	double equation[4];
	size_t id;

 public:
	// Construct the clipping plane equation from a point in view space and a
	// normal vector.  normal must be of unit length.  The normal vector points
	// in the direction of polygons that pass the clipping test.
	clipping_plane( vector point, vector normal);
	~clipping_plane();
 
	// TODO: perform testing to see how expensive this is.  Answer: better
	// performance when texturing is required, slightly worse when not.  (for
	// spheres anyway).
	void gl_enable();
	void gl_disable();

};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_CLIPPING_PLANE_HPP
