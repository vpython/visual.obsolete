#ifndef VPYTHON_PYTYHON_POINTS_HPP
#define VPYTHON_PYTYHON_POINTS_HPP

#include "renderable.hpp"
#include "python/num_util.hpp"

#include <boost/python/list.hpp>

namespace cvisual { namespace python {

using boost::python::list;
using boost::python::numeric::array;

// TODO: This class shares everything but memory management and rendering with the
// curve object.  Perhaps they can share a (partial?) base class and use the 
// mix-in pattern?
class points : public renderable
{
 private:
	
	// The pos and color arrays are always overallocated to make appends
	// faster.  Whenever they are read from Python, we return a slice into the
	// array that starts at its beginning and runs up to the last used position
	// in the array.  This is simmilar to many implementations of std::vector<>.
	array pos;
	array color;	
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
	void set_length( size_t);
	
	// Specifies whether or not the size of the points should scale with the
	// world or with the screen.
	enum { WORLD, SCREEN } size_type;
	
	// Specifies the shape of the point. Future candidates are triangles,
	// diamonds, etc. 
	enum { ROUND, SQUARE } points_shape;

	// The size of the points
	float size;
	
	
	bool degenerate() const;
	
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
	
 public:
	points();
	points( const points& other);
	virtual ~points();
	
	void append_rgba( vector, float red=-1, float green=-1, float blue=-1, float opacity=-1);
	void append( vector _pos, rgba _color); // Append a single position with new color.
	void append( vector _pos); // Append a single position element, extend color.
	
	boost::python::object get_pos(void);
	boost::python::object get_color(void);
	
	void set_points_shape( const std::string& n_type);
	std::string get_points_shape( void);
	
	void set_size( float r);
	inline float get_size( void) { return size; }
	
	void set_size_type( const std::string& n_type);
	std::string get_size_type( void);
	
	void set_pos( array pos); // An Nx3 array
	void set_pos_l( const list& pos); // A list of vector
	void set_pos_v( const vector& pos); // Interpreted as an initial append().
	
	void set_color( array color); // An Nx3 array of color
	void set_color_l( const list& color); // A list of vectors
	void set_color_t( const rgba& color); // A single tuple
	
	void set_red( const array& red);
	void set_red_l( const list& red);
	void set_red_d( const float red);
	void set_blue( const array& blue);
	void set_blue_l( const list& blue);
	void set_blue_d( const float blue);
	void set_green( const array& green);
	void set_green_l( const list& green);
	void set_green_d( const float green);
	void set_opacity( const array& opacity);
	void set_opacity_l( const list& opacity);
	void set_opacity_d( const float opacity);
	void set_x( const array& x);
	void set_x_l( const list& x);
	void set_x_d( const double x);
	void set_y( const array& y);
	void set_y_l( const list& y);
	void set_y_d( const double y);
	void set_z( const array& z);
	void set_z_l( const list& z);
	void set_z_d( const double z);
	
};

} } // !namespace cvisual::python

#endif /*VPYTHON_PYTYHON_POINTS_HPP*/
