#ifndef VPYTHON_PYTHON_CURVE_HPP
#define VPYTHON_PYTHON_CURVE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/displaylist.hpp"
#include "python/num_util.hpp"

#include <boost/python/list.hpp>

namespace cvisual { namespace python {

using boost::python::list;
using boost::python::numeric::array;

class curve : public renderable
{
 private:
	// The pos and color arrays are always overallocated to make appends
	// faster.  Whenever they are read from Python, we return a slice into the
	// array that starts at its beginning and runs up to the last used position
	// in the array.  This is simmilar to many implementations of std::vector<>.
	array pos;
	array color;
	bool antialias;
	double radius;
	// the space allocated for storage so far
	size_t preallocated_size;
	// the number of vectors currently occupying the allocated storage.
	// index( , count+1) is the last element in the arrays.
	// index( , 1) is the first element in the array
	// index( , 0) is used as the before-the-first element when rendering with
	//   gle.
	// index( , count+2) is used as the after-the-last point when rendering with
	//    gle.
	size_t count;

	// A type used to cache (in OpenGL memory) a displaylist for a chunk of the
	// curve.
	struct c_cache 
	{
		static const size_t items = 256;
		displaylist gl_cache;
		long checksum;
		c_cache() : checksum(0) {}
	};
	std::vector<c_cache> cache;
	typedef std::vector<c_cache>::iterator cache_iterator;

	// Returns true if the object is single-colored.
	bool monochrome( size_t begin, size_t end);
	// Returns true if the object is degenarate and should not be rendered.
 	bool degenerate() const;
	// Returns true if the cuve follows a closed path.
 	bool closed_path() const;
	// Compute a checksum over the elements of the specified [begin,end) range.
	long checksum( size_t begin, size_t end);

 public:
	
	curve();
	curve( const curve& other);

	void append_rgb( vector, float r=-1, float g=-1, float b=-1);
	void append( vector _pos, rgb _color); // Append a single position with new color.
	void append( vector _pos); // Append a single position element, extend color.
	
	boost::python::object get_pos(void);
	boost::python::object get_color(void);
	
	inline bool get_antialias( void) { return false; }
	inline double get_radius( void) { return radius; }

	void set_pos( array pos); // An Nx3 array
	void set_pos_l( const list& pos); // A list of vector
	void set_pos_v( const vector& pos); // Interpreted as an initial append().
	void set_color( array color); // An Nx3 array of color
	void set_color_l( const list& color); // A list of vectors
	void set_color_t( const rgb& color); // A single tuple
	
	void set_antialias( bool);
	void set_radius( const double& r);
	void set_red( const array& red);
	void set_red_l( const list& red);
	void set_red_d( const double red);
	void set_blue( const array& blue);
	void set_blue_l( const list& blue);
	void set_blue_d( const double blue);
	void set_green( const array& green);
	void set_green_l( const list& green);
	void set_green_d( const double green);
	void set_x( const array& x);
	void set_x_l( const list& x);
	void set_x_d( const double x);
	void set_y( const array& y);
	void set_y_l( const list& y);
	void set_y_d( const double y);
	void set_z( const array& z);
	void set_z_l( const list& z);
	void set_z_d( const double z);
	
 private:
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
	
	void thinline( const view&, size_t begin, size_t end);
	void thickline( const view&, size_t begin, size_t end);

	// Verify that the pos and color arrays have room for the requested length
	// if not, they are grown as required and the old data is copied over.
	void set_length( size_t new_length);
};

void curve_init_type();

} } // !namespace cvisual::python

#endif // !VPYTHON_PYTHON_CURVE_HPP
