#ifndef VPYTHON_PYTHON_extrusion_HPP
#define VPYTHON_PYTHON_extrusion_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/displaylist.hpp"
#include "python/num_util.hpp"
#include "python/arrayprim.hpp"

namespace cvisual { namespace python {

using boost::python::list;
using boost::python::numeric::array;

class extrusion : public arrayprim_color
{
 protected:
	// The pos and color arrays are always overallocated to make appends
	// faster.  Whenever they are read from Python, we return a slice into the
	// array that starts at its beginning and runs up to the last used position
	// in the array.  This is similar to many implementations of std::vector<>.

	bool antialias;

	// contours are flattened N*2 arrays of points describing the 2D surface, one after another.
	// pcontours[0] is (numper of contours, 0)
	// pcontours[2*i+2] points to (length of ith contour, starting location of ith contour in contours).
	// strips are flattened N*2 arrays of points describing strips that span the "solid" part of the 2D surface.
	// pstrips[2*i] points to (length of ith strip, starting location of ith strip in strips).
	std::vector<double> contours, strips;
	std::vector<int> pcontours, pstrips;
	std::vector<int> indices;
	std::vector<double> normals2D;

	// Returns true if the object is single-colored.
	bool monochrome(float* tcolor, size_t pcount);

	virtual void outer_render( const view&);
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
	void get_material_matrix( const view& v, tmatrix& out );

	// Returns true if the object is degenarate and should not be rendered.
 	bool degenerate() const;

 public:
	extrusion();

	void set_contours( const array&, const array&, const array&, const array& );

	inline bool get_antialias( void) { return antialias; }

	void set_antialias( bool);

 private:
	bool adjust_colors( const view& scene, float* tcolor, size_t pcount);
	void extrude( const view&, double* spos, float* tcolor, size_t pcount);
};

} } // !namespace cvisual::python

#endif // !VPYTHON_PYTHON_extrusion_HPP
