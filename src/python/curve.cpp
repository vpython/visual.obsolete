// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <boost/python/detail/wrap_python.hpp>
#include <boost/crc.hpp>

#include "util/errors.hpp"
#include "util/gl_enable.hpp"

#include "python/slice.hpp"
#include "python/curve.hpp"

#include <stdexcept>
#include <cassert>
#include <sstream>
#include <iostream>

using boost::python::make_tuple;
using boost::python::object;

// Recall that the default constructor for object() is a reference to None.

namespace cvisual { namespace python {

// Visual Studio chokes on this line in the link phase, but MinGW and Linux need it.
#ifndef _MSC_VER
const size_t curve::c_cache::items;
#endif

namespace {

// returns a pointer to the ith vector in the array.
double* index( const array& a, size_t i)
{
	// This is technically an unsafe cast since the alignment requirement
	// goes up for the cast.  It is made safe by padding actions within the Numeric
	// library itself, but the compiler doesn't know that, so I am just using a
	// raw cast vice a static_cast<>.
	return ((double*)data(a)) + (i+1) * 3;
}

float* findex( const array& a, size_t i)
{
	return ((float*)data(a)) + (i+1)*3;
}

} // !namespace visual::(unnamed)


curve::curve()
	: pos( 0), color( 0), antialias( true),
	radius(0.0),
	retain(0),
	preallocated_size(257),
	count(0), sides(4)
{
	// Perform initial allocation of storage space for the buggar.
	std::vector<npy_intp> dims(2);
	dims[0] = preallocated_size;
	dims[1] = 3;
	pos = makeNum( dims);
	color = makeNum( dims);
	double* pos_i = index( pos, 0);
	double* color_i = index( color, 0);
	pos_i[0] = 0;
	pos_i[1] = 0;
	pos_i[2] = 0;
	color_i[0] = 1;
	color_i[1] = 1;
	color_i[2] = 1;

	int curve_around = sides;

	for (int i=0; i<curve_around; i++) {
		curve_sc[i]  = (float) std::cos(i * 2 * M_PI / curve_around);
		curve_sc[i+curve_around] = (float) std::sin(i * 2 * M_PI / curve_around);
	}

	for (int i=0; i<128; i++) {
		curve_slice[i*2]       = i*curve_around;
		curve_slice[i*2+1]     = i*curve_around + 1;
		curve_slice[i*2 + 256] = i*curve_around + (curve_around - 1);
		curve_slice[i*2 + 257] = i*curve_around;
	}
}

curve::curve( const curve& other)
	: renderable( other), pos( other.pos), color( other.color),
	antialias( other.antialias), retain( other.retain),
	radius( other.radius), preallocated_size( other.preallocated_size),
	count( other.count)
{
	int curve_around = sides;

	for (int i=0; i<curve_around; i++) {
		curve_sc[i]  = (float) std::cos(i * 2 * M_PI / curve_around);
		curve_sc[i+curve_around] = (float) std::sin(i * 2 * M_PI / curve_around);
	}

	for (int i=0; i<128; i++) {
		curve_slice[i*2]       = i*curve_around;
		curve_slice[i*2+1]     = i*curve_around + 1;
		curve_slice[i*2 + 256] = i*curve_around + (curve_around - 1);
		curve_slice[i*2 + 257] = i*curve_around;
	}
}

curve::~curve()
{
}

object
curve::get_pos()
{
	return pos[slice(1, (int)count+1)];
}

object
curve::get_color()
{
	return color[slice(1, (int)count+1)];
}

void
curve::set_length( size_t length)
{
	// The number of points that are valid.
	size_t npoints = count;
	if (length < npoints) { // A shrink operation
		const size_t shift = (npoints-length);
		double* pos_i = index( pos, shift);
		double* pos_end = index( pos, npoints);
		while (pos_i < pos_end) {
			pos_i[0-3*shift] = pos_i[0];
			pos_i[1-3*shift] = pos_i[1];
			pos_i[2-3*shift] = pos_i[2];
			pos_i += 3;
		}

		double* color_i = index( color, shift);
		double* color_end = index( color, npoints);
		while (color_i < color_end) {
			color_i[0-3*shift] = color_i[0];
			color_i[1-3*shift] = color_i[1];
			color_i[2-3*shift] = color_i[2];
			color_i += 3;
		}
	}
	if (npoints == 0)
		// The first allocation.
		npoints = 1;

	if (length > preallocated_size-2) {
		VPYTHON_NOTE( "Reallocating buffers for a curve object.");
		std::vector<npy_intp> dims(2);
		dims[0] = 2*length + 2;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		array n_color = makeNum( dims);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * (npoints + 1));
		std::memcpy( data( n_color), data( color), sizeof(double) * 3 * (npoints + 1));
		pos = n_pos;
		color = n_color;
		preallocated_size = dims[0];
	}
	if (length > npoints) {
		// Copy the last good element to the new positions.
		const double* last_pos = index( pos, npoints-1);
		double* pos_i = index( pos, npoints);
		double* pos_end = index( pos, length);
		while (pos_i < pos_end) {
			pos_i[0] = last_pos[0];
			pos_i[1] = last_pos[1];
			pos_i[2] = last_pos[2];
			pos_i += 3;
		}

		const double* last_color = index( color, npoints-1);
		double* color_i = index( color, npoints);
		double* color_end = index( color, length);
		while (color_i < color_end) {
			color_i[0] = last_color[0];
			color_i[1] = last_color[1];
			color_i[2] = last_color[2];
			color_i += 3;
		}
	}
	count = length;
	int cache_length = (count+c_cache::items)/(c_cache::items-1) - cache.size();
	while (cache_length > 0) {
		cache.push_back( c_cache());
		cache_length--;
	}
}

void
curve::append_rgb( vector npos, double red, double blue, double green)
{
	lock L(mtx);
	if (retain > 0 and count >= retain) {
		set_length( retain-1); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	double* last_color = index( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	if (red != -1)
		last_color[0] = red;
	if (green != -1)
		last_color[1] = green;
	if (blue != -1)
		last_color[2] = blue;
}

void
curve::append( vector npos)
{
	lock L(mtx);
	if (retain > 0 and count >= retain) {
		set_length( retain-1); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
}

void
curve::append( vector npos, rgb ncolor)
{
	lock L(mtx);
	if (retain > 0 and count >= retain) {
		set_length( retain-1); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	double* last_color = index( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	last_color[0] = ncolor.red;
	last_color[1] = ncolor.green;
	last_color[2] = ncolor.blue;
}

void
curve::set_pos( array n_pos)
{
	NPY_TYPES t = type( n_pos);
	if (t != NPY_DOUBLE) {
		n_pos = astype(n_pos, NPY_DOUBLE);
	}
	std::vector<npy_intp> dims = shape( n_pos);
	if (dims.size() == 1) {
		if (!dims[0]) {
			lock L(mtx);
			set_length(0);
			return;
		}
		else {
			lock L(mtx);
			set_length( dims[0]);
			pos[make_tuple(slice(1, count+1), slice())] = n_pos;
			if (retain > 0 and count >= retain) set_length( retain);
			return;
		}
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
	if (dims[1] == 2) {
		lock L(mtx);
		set_length( dims[0]);
		pos[make_tuple(slice(1, count+1), slice(0,2))] = n_pos;
		pos[make_tuple(slice(1, count+1), 2)] = 0.0;
		if (retain > 0 and count >= retain) set_length( retain);
		return;
	}
	else if (dims[1] == 3) {
		lock L(mtx);
		set_length( dims[0]);
		pos[make_tuple(slice(1, count+1), slice())] = n_pos;
		if (retain > 0 and count >= retain) set_length( retain);
		return;
	}
	else {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
}

void
curve::set_pos_l( const list& pos)
{
	this->set_pos( array(pos));
}

// Interpreted as an initial append operation, with no color specified.
void
curve::set_pos_v( const vector& npos)
{
	using namespace boost::python;
	tuple t_pos = make_tuple( npos.x, npos.y, npos.z);
	set_pos( array(t_pos));
}

void
curve::set_color( array n_color)
{
	NPY_TYPES t = type(n_color);
	if (t != NPY_DOUBLE) {
		n_color = astype(n_color, NPY_DOUBLE);
	}
    std::vector<npy_intp> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		lock L(mtx);
		color[slice(1,npoints+1)] = n_color;
		if (retain > 0 and count >= retain) set_length( retain);
		return;
	}
	if (dims.size() == 2 && dims[1] == 3) {
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		lock L(mtx);
		color[slice(1, count+1)] = n_color;
		if (retain > 0 and count >= retain) set_length( retain);
		return;
	}
	throw std::invalid_argument( "color must be an Nx3 array");
}

void
curve::set_color_l( const list& color)
{
	this->set_color( array(color));
}

void
curve::set_color_t( const rgb& color)
{
	this->set_color( array( make_tuple(color.red, color.green, color.blue)));
}

void
curve::set_red( const array& red)
{
	lock L(mtx);
	set_length( shape(red).at(0));
	color[make_tuple(slice(1,count+1), 0)] = red;
}

void
curve::set_green( const array& green)
{
	lock L(mtx);
	set_length( shape(green).at(0));
	color[make_tuple(slice(1,count+1), 1)] = green;
}

void
curve::set_blue( const array& blue)
{
	lock L(mtx);
	set_length( shape(blue).at(0));
	color[make_tuple(slice(1,count+1), 2)] = blue;
}

void
curve::set_x( const array& x)
{
	lock L(mtx);
	set_length( shape(x).at(0));
	pos[make_tuple( slice(1, count+1), 0)] = x;
}

void
curve::set_y( const array& y)
{
	lock L(mtx);
	set_length( shape(y).at(0));
	pos[make_tuple( slice(1, count+1), 1)] = y;
}

void
curve::set_z( const array& z)
{
	lock L(mtx);
	set_length( shape(z).at(0));
	pos[make_tuple( slice(1, count+1), 2)] = z;
}

void
curve::set_red_l( const list& red)
{
	this->set_red( array(red));
}

void
curve::set_green_l( const list& green)
{
	this->set_green( array(green));
}

void
curve::set_blue_l( const list& blue)
{
	this->set_blue( array(blue));
}

void
curve::set_x_l( const list& x)
{
	set_x( array(x));
}

void
curve::set_y_l( const list& y)
{
	set_y( array(y));
}

void
curve::set_z_l( const list& z)
{
	set_z( array(z));
}

void
curve::set_x_d( const double x)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 0)] = x;
}

void
curve::set_y_d( const double y)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 1)] = y;
}

void
curve::set_z_d( const double z)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 2)] = z;
}

void
curve::set_red_d( const double red)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 0)] = red;
}

void
curve::set_green_d( const double green)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 1)] = green;
}

void
curve::set_blue_d( const double blue)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 2)] = blue;
}

void
curve::set_radius( const double& radius)
{
	lock L(mtx);
	this->radius = radius;
}

void
curve::set_antialias( bool aa)
{
	lock L(mtx);
	this->antialias = aa;
}

void
curve::set_retain( int retain)
{
	lock L(mtx);
	if (retain > 0 and count > retain) {
		set_length( retain); //move pos and color lists down
	}
	this->retain = retain;
}

bool
curve::degenerate() const
{
	return count < 2;
}


bool
curve::monochrome()
{
	const double* color_i = index( color, 0);
	const double* color_end = index( color, count);

	rgb first_color( (float) color_i[0], (float) color_i[1],(float) color_i[2]);
	color_i += 3;

	while (color_i < color_end) {
		if (color_i[0] != first_color.red)
			return false;
		if (color_i[1] != first_color.green)
			return false;
		if (color_i[2] != first_color.blue)
			return false;
		color_i += 3;
	}

	return true;
}

namespace {
// Determines if two values differ by more than frac of either one.
bool
eq_frac( double lhs, double rhs, double frac)
{
	if (lhs == rhs)
		return true;
	double diff = fabs(lhs - rhs);
	lhs = fabs(lhs);
	rhs = fabs(rhs);
	return lhs*frac > diff && rhs*frac > diff;
}
} // !namespace (unnamed)

bool
curve::closed_path() const
{
	// Determines if the curve follows a closed path or not.  The case where
	// this fails is when the beginning and end are:
	//   very close together.
	//   and very close to the origin.
	//   and the scope of the entire curve is large relative to the points'
	//     proximity to the origin.
	// In this case, it returns false when it should be true.
	vector begin( index(pos, 0));
	vector end( index(pos, count-1));
	return eq_frac(begin.x, end.x, 1e-5)
		&& eq_frac(begin.y, end.y, 1e-5)
		&& eq_frac(begin.z, end.z, 1e-5);
}

long
curve::checksum( size_t begin, size_t end)
{
	boost::crc_32_type engine;
	engine.process_block( &radius, &radius + sizeof(radius));
	engine.process_block( index( pos, begin-1), index( pos, end+1));
	engine.process_block( index( color, begin), index( color, end));
	return engine.checksum();
}

vector
curve::get_center() const
{
	// TODO: Optimize this by only recomputing the center when the checksum of
	// the entire object has changed.
	// TODO: Only add the "optimization" if the checksum is actually faster than
	// computing the average value every time...
	if (degenerate())
		return vector();
	vector ret;
	const double* pos_i = index( pos, 0);
	const double* pos_end = index( pos, count);
	while (pos_i < pos_end) {
		ret.x += pos_i[0];
		ret.y += pos_i[1];
		ret.z += pos_i[2];
		pos_i += 3;
	}
	ret /= count;
	return ret;
}

void
curve::gl_pick_render( const view& scene)
{
	// Aack, I can't think of any obvious optimizations here
	gl_render( scene);
}

void
curve::grow_extent( extent& world)
{
	if (degenerate())
		return;
	const double* pos_i = index(pos, 0);
	const double* pos_end = index( pos, count);
	if (radius == 0.0)
		for ( ; pos_i < pos_end; pos_i += 3)
			world.add_point( vector(pos_i));
	else
		for ( ; pos_i < pos_end; pos_i += 3)
			world.add_sphere( vector(pos_i), radius);
	world.add_body();
}

bool
curve::adjust_colors( const view& scene, float* tcolor, size_t pcount)
{
	rgb rendered_color;
	bool mono = monochrome();
	if (mono) {
		// We can get away without using a color array.
		rendered_color = rgb( tcolor[0], tcolor[1], tcolor[2]);
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				rendered_color.desaturate().gl_set();
			else
				rendered_color.grayscale().gl_set();
		}
		else
			rendered_color.gl_set();
	}
	else {
		glEnableClientState( GL_COLOR_ARRAY);
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.

			for (size_t i = 0; i < pcount; ++i) {
				rendered_color = rgb( tcolor[3*i], tcolor[3*i+1], tcolor[3*i+2]);
				if (scene.coloranaglyph)
					rendered_color = rendered_color.desaturate();
				else
					rendered_color = rendered_color.grayscale();
				tcolor[3*i] = rendered_color.red;
				tcolor[3*i+1] = rendered_color.green;
				tcolor[3*i+2] = rendered_color.blue;
			}
		}
	}
	return mono;
}

namespace {
template <typename T>
struct converter
{
	T data[3];
};
} // !namespace (anonymous)


void
curve::thickline( const view& scene, const float* spos, float* tcolor, size_t pcount, double scaled_radius)
{
	size_t curve_around = sides;
	std::vector<vector> projected;
	std::vector<vector> normals;
	std::vector<rgb> light;

	float *cost = curve_sc;
	float *sint = cost + curve_around;

	vector lastx(1, 0, 0), lasty(0, 1, 0);

	// pos and color iterators
	const float* v_i = spos;
	const float* c_i = tcolor;
	bool mono = adjust_colors( scene, tcolor, pcount);

	bool first = true;
	for(size_t corner=0; corner < count; ++corner, v_i += 3, c_i += 3 ) {
		// The vector to which v_i currently points towards.
		vector current( v_i[0], v_i[1], v_i[2] );

		/* To construct the circle of vertices around a point on the curve,
		 * I need an orthonormal basis of the plane of the circle.  A and B,
		 * normalized vectors pointing toward the next and from the previous
		 * points on the curve, are vectors in the plane of the *curve* at
		 * this point.  The cross product and the difference of these vectors
		 * are orthogonal and lie in the appropriate plane.
       * Then parallel transport the last frame to this plane
		 * a) vectors 'initial' and 'final' specify the normal to the plane at the beginning
		 * and end of the curve in which the surrounding tube terminates
		 * b) vector 'vecx' specifies the 'x'-direction of the frame in the beginning plane
		 *    This is useful to change the orientation of the surrounding box when sides=4
		 * c) int 'sides' specifies the number of sides for the tube 2 <= sides <= 20
		 * d)	rscale specifies how to define radius in the planes at the end of each segment
		 *		rscale = 0.0 defaults to projection of radius into plane without length scaling
		 *                 and without parallel transport
		 *    rscale > 0.0 scales the radius so that the tube is of constant diameter
		 *    rscale small may give sharp points at very acute corners
		 *    rscale = 1.0 gives same as default tube except uses parallel transport for frame
		 * e) array rvect = (x,y,z) defines radius in x and y frame directions, z coordinate is not used
		 *    this allows radius to vary along curve and should be defined for each point in curve
	Last change:  AWW  21 Aug 2004    1:12 pm

		Added to VPython-core2 16 May 2006
		 */

		vector next( v_i[3], v_i[4], v_i[5]); // The next vector in pos
		vector prev( v_i[-3], v_i[-2], v_i[-1]); // The previous vector in pos
		vector A = (next - current).norm();
		vector B = (current - prev).norm();
		vector x = (A - B).norm();
		vector y = A.cross(B).norm();
		if (first && !x) {
			x = A.cross( vector(0, 0, 1));
			if (!x)
				x = A.cross(vector( 1, 0, 0));
			y = A.cross(x);
		}
		if (first && !y) {
			y = A.cross(x);
		}
		first = false;
		if (!x)
			x = lastx;
		if (!y)
			y = lasty;

		if (!x || !y || x == y) {
			std::ostringstream msg;
			msg << "Degenerate curve case! please report the following "
				"information to visualpython-users@lists.sourceforge.net: ";
			msg << "current:" << current << " next:" << next << " prev:" << prev
		 		<< " A:" << A << " B:" << B << " x:" << x << " y:" << y
		 		<< " lastx:" << lastx << " lasty:" << lasty << " first:"
		 		<< first << std::endl;
		 	VPYTHON_WARNING( msg.str());
		}

		// remove twists
		if (x.dot(lastx) < 0)
			x = -x;
		if (y.dot(lasty) < 0)
			y = -y;

		// save last set of vectors
		lastx = x;
		lasty = y;

		// scale radii
		x *= scaled_radius;
		y *= scaled_radius;

		for (size_t a=0; a < curve_around; a++) {
			float c = cost[a];
			float s = sint[a];
			normals.push_back( (x*c + y*s).norm());
			projected.push_back( current + x*c + y*s);
			if (!mono) {
				light.push_back( rgb( c_i[0], c_i[1], c_i[2]) );
			}
		}
	}
	size_t npoints = normals.size() / curve_around;
	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	if (!mono) {
		glEnableClientState( GL_COLOR_ARRAY);
	}

	int *ind = curve_slice;
	for (size_t a=0; a < curve_around; a++) {
		size_t ai = a;
		if (a == curve_around-1) {
			ind += 256;
			ai = 0;
		}

		for (size_t i = 0; i < npoints; i += 127u) {
			glVertexPointer(3, GL_DOUBLE, sizeof( vector), &projected[i*curve_around + ai].x);
			if (!mono)
				glColorPointer(3, GL_FLOAT, sizeof( rgb), &light[(i*curve_around + ai)].red );
			glNormalPointer( GL_DOUBLE, sizeof(vector), &normals[i*curve_around + ai].x);
			if (npoints-i < 128)
				glDrawElements(GL_TRIANGLE_STRIP, 2*(npoints-i), GL_UNSIGNED_INT, ind);
			else
				glDrawElements(GL_TRIANGLE_STRIP, 256u, GL_UNSIGNED_INT, ind);
		}
	}
	if (!mono)
		glDisableClientState( GL_COLOR_ARRAY);
}

void
curve::gl_render( const view& scene)
{
	if (degenerate())
		return;
	const size_t true_size = count;
	// Set up the leading and trailing points for the joins.  See
	// glePolyCylinder() for details.  The intent is to create joins that are
	// perpendicular to the path at the last segment.  When the path appears
	// to be closed, it should be rendered that way on-screen.
	bool closed = closed_path();
	if (closed) {
		// Make the 0th element == last element.
		// Make the true_size+1 element == first element.
		double* pos_i = index( pos, 0)-3;
		double* pos_end = index( pos, true_size-1);
		pos_i[0] = pos_end[0];
		pos_i[1] = pos_end[1];
		pos_i[2] = pos_end[2];
		pos_i += 3;
		pos_end += 3;
		pos_end[0] = pos_i[0];
		pos_end[1] = pos_i[1];
		pos_end[2] = pos_i[2];
	}
	else {
		double* pos_begin = index( pos, 0)-3;
		pos_begin[0] = pos_begin[0+3] - (pos_begin[0+6]-pos_begin[0+3]);
		pos_begin[1] = pos_begin[1+3] - (pos_begin[1+6]-pos_begin[1+3]);
		pos_begin[2] = pos_begin[2+3] - (pos_begin[2+6]-pos_begin[2+3]);

		double* pos_end = index( pos, true_size);
		double* pos_last = index( pos, true_size-1);
		pos_end[0] = pos_last[0] + (pos_last[0] - pos_last[0-3]);
		pos_end[1] = pos_last[1] + (pos_last[1] - pos_last[1-3]);
		pos_end[2] = pos_last[2] + (pos_last[2] - pos_last[2-3]);
	}
	
	// The maximum number of points to display.
	const int LINE_LENGTH = 10000;
	// Data storage for the position and color data.
	float spos[3*LINE_LENGTH];
	float tcolor[3*LINE_LENGTH];
	float fstep = ( ((float) count)-1.0)/(LINE_LENGTH-1.0);
	if (fstep < 1.0) fstep = 1.0;
	size_t iptr=0, iptr3, pcount=0;

	const double* p_i = (double*)( data(this->pos));
	const double* c_i = (double*)( data(this->color));
	
	// Choose which points to display
	for (float fptr=0.0; iptr < count; fptr += fstep, iptr = (int) (fptr+.5), ++pcount) {
		iptr3 = 3*(iptr+1); // first real point is the second in the data array
		spos[3*pcount] = p_i[iptr3];
		spos[3*pcount+1] = p_i[iptr3+1];
		spos[3*pcount+2] = p_i[iptr3+2];
		tcolor[3*pcount] = c_i[iptr3];
		tcolor[3*pcount+1] = c_i[iptr3+1];
		tcolor[3*pcount+2] = c_i[iptr3+2];
	}
	
	// Do scaling if necessary
	double scaled_radius = radius;
	if (scene.gcf != 1.0 || (scene.gcfvec[0] != scene.gcfvec[1])) {
		scaled_radius = radius*scene.gcfvec[0];
		for (size_t i = 0; i < pcount; ++i) {
			spos[3*i] *= scene.gcfvec[0];
			spos[3*i+1] *= scene.gcfvec[1];
			spos[3*i+2] *= scene.gcfvec[2];
		}
	}
		
	clear_gl_error();

	size_t size = std::min(c_cache::items, true_size);
	size_t begin = 0;
	cache_iterator c = cache.begin();
	cache_iterator c_end = cache.end();
	const bool do_thinline = (radius == 0.0);
	if (do_thinline) {
		glEnableClientState( GL_VERTEX_ARRAY);
		glDisable( GL_LIGHTING);
		// Assume monochrome.
		if (antialias) {
			glEnable( GL_BLEND);
			glEnable( GL_LINE_SMOOTH);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	else {
		lighting_prepare();
		shiny_prepare();
	}
	assert( c != c_end);
	// TODO: Make a smarter caching algorithm.  Should only make a cache
	// if the checksum has been constant for some predetermined number of
	// rendering cycles.

	long check = checksum( begin, begin+size);
	if (check == c->checksum && !scene.gcf_changed) {
		c->gl_cache.gl_render();
	}
	else {
		c->gl_cache.gl_compile_begin();
		if (do_thinline) {
			glVertexPointer( 3, GL_FLOAT, 0, spos);
			bool mono = adjust_colors( scene, tcolor, pcount);
			if (!mono) glColorPointer( 3, GL_FLOAT, 0, tcolor);
			glDrawArrays( GL_LINE_STRIP, 0, pcount);
			glDisableClientState( GL_VERTEX_ARRAY);
			glDisableClientState( GL_COLOR_ARRAY);
			glEnable( GL_LIGHTING);
			if (antialias) {
				glDisable( GL_BLEND);
				glDisable( GL_LINE_SMOOTH);
			}
		}
		else {
			thickline( scene, spos, tcolor, pcount, scaled_radius);
			shiny_complete();
			lighting_complete();
		}
		c->gl_cache.gl_compile_end();
		c->checksum = check;
		c->gl_cache.gl_render();
	}

	check_gl_error();

}

} } // !namespace cvisual::python
