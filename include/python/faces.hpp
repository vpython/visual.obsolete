#ifndef VPYTHON_PYTHON_FACES_HPP
#define VPYTHON_PYTHON_FACES_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "python/num_util.hpp"

#include <boost/python/object.hpp>
#include <boost/python/list.hpp>

namespace cvisual { namespace python {

class faces : public renderable
{
 private:
	array pos;    // An array of points defining the triangular faces
	array color;  // An array of colors for the faces
	array normal; // An array of normal vectors for the faces.

	int preallocated_size;
	int count;
	
	enum member { POS, COLOR, NORMAL };
	// Encapsulates the code to set any one of the three array members from a new
	// array.
	void set_array_member( member, const array&);
	void set_length( int);
	
	bool degenerate() const;
	virtual void gl_render( const view&);
	virtual void gl_pick_render( const view&);
	virtual vector get_center() const;
	virtual void grow_extent( extent&);

 public:
	faces();
	faces( const faces& other);
	// Add another vertex, normal, and color to the faces.
	void append( vector, vector, rgb);
	void append( vector, vector);
	// This routine was adapted from faces_heightfield.py.  It averages the normal
	// vectors at coincident verticies to smooth out boundaries between facets.  
	// No attempt is made to detect sharp edges.
	void smooth_shade(bool doublesided = true);
	
	// Getters.
	boost::python::object get_pos();
	boost::python::object get_color();
	boost::python::object get_normal();
	
	void set_pos( const array& pos);
	void set_pos_l( boost::python::list pos);
	void set_color( array color);
	void set_color_t( rgb color);
	void set_color_l( boost::python::list color);
	void set_normal( const array& normal);
	void set_normal_l( boost::python::list normals);
};

} } // !namespace cvisual::python

#endif // !defined VPYTHON_PYTHON_FACES_HPP
