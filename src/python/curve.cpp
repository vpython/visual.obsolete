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

namespace {

// returns a pointer to the ith vector in the array.
double* index( const array& a, size_t i)
{
	// This is technically an unsafe cast since the alignment requirement
	// goes up for the cast.  It is made safe by padding actions within the Numeric
	// library itself, but the compiler doesn't know that, so I am just using a
	// raw cast vice a static_cast<>.
	return ((double*)data(a)) + (i+1) * 3; // (x,y,z)
}

float* findex( const array& a, size_t i)
{
	return ((float*)data(a)) + (i+1)*3; // (red,green,blue)
}

} // !namespace visual::(unnamed)


curve::curve()
	: pos( 0), color( 0), antialias( true),
	radius(0.0),
	preallocated_size(257),
	count(0), sides(4)
{
	// Perform initial allocation of storage space.
	std::vector<npy_intp> dims(2);
	dims[0] = preallocated_size;
	dims[1] = 3;
	pos = makeNum( dims);
	color = makeNum( dims, NPY_FLOAT);
	opacity = 1.0; // transparency not yet implmented for curve
	double* pos_i = index( pos, 0);
	float* color_i = findex( color, 0);
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

	// curve_slice is a list of indices for picking out the correct vertices from
	// a list of vertices representing one side of a thick-line curve. The lower
	// indices (0-255) are used for all but one of the sides. The upper indices
	// (256-511) are used for the final side.
	for (int i=0; i<128; i++) {
		curve_slice[i*2]       = i*curve_around;
		curve_slice[i*2+1]     = i*curve_around + 1;
		curve_slice[i*2 + 256] = i*curve_around + (curve_around - 1);
		curve_slice[i*2 + 257] = i*curve_around;
	}
}

curve::curve( const curve& other)
	: renderable( other), pos( other.pos), color( other.color),
	antialias( other.antialias),
	radius( other.radius), preallocated_size( other.preallocated_size),
	count( other.count)
{
	int curve_around = sides;
	opacity = 1.0; // transparency not yet implmented for curve

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

		float* color_i = findex( color, shift);
		float* color_end = findex( color, npoints);
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
		array n_color = makeNum( dims, NPY_FLOAT);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * (npoints + 1));
		std::memcpy( data( n_color), data( color), sizeof(float) * 3 * (npoints + 1));
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

		const float* last_color = findex( color, npoints-1);
		float* color_i = findex( color, npoints);
		float* color_end = findex( color, length);
		while (color_i < color_end) {
			color_i[0] = last_color[0];
			color_i[1] = last_color[1];
			color_i[2] = last_color[2];
			color_i += 3;
		}
	}
	count = length;
}

void
curve::append_rgb( vector npos, float red, float green, float blue, int retain)
{
	if (retain >= 0 && (int)count >= retain) {
		set_length( retain); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
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
curve::append( vector npos, rgb ncolor, int retain)
{
	if (retain >= 0 && (int)count >= retain) {
		set_length( retain); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	last_color[0] = ncolor.red;
	last_color[1] = ncolor.green;
	last_color[2] = ncolor.blue;
}

void
curve::append( vector npos, rgb ncolor)
{
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	last_color[0] = ncolor.red;
	last_color[1] = ncolor.green;
	last_color[2] = ncolor.blue;
}

void
curve::append( vector npos, int retain)
{
	if (retain >= 0 && (int)count >= retain) {
		set_length( retain); //move pos and color lists down
	}
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
}

void
curve::append( vector npos)
{
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
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
			set_length(0);
			return;
		}
		else {
			set_length( dims[0]);
			pos[make_tuple(slice(1, count+1), slice())] = n_pos;
			return;
		}
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
	if (dims[1] == 2) {
		set_length( dims[0]);
		pos[make_tuple(slice(1, count+1), slice(0,2))] = n_pos;
		pos[make_tuple(slice(1, count+1), 2)] = 0.0;
		return;
	}
	else if (dims[1] == 3) {
		set_length( dims[0]);
		pos[make_tuple(slice(1, count+1), slice())] = n_pos;
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
	if (t != NPY_FLOAT) {
		n_color = astype(n_color, NPY_FLOAT);
	}
    std::vector<npy_intp> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		color[slice( 1, npoints+1)] = n_color;
		return;
	}
	if (dims.size() == 2 && dims[1] == 3) {
		// An RGB chunk of color
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		// The following doesn't work; I don't know why.
		// Note that it works with a single color above.
		//color[slice(1, count+1), slice(0, 3)] = n_color;
		// So instead do it by brute force:
		float* color_i = findex( color, 1);
		float* color_end = findex( color, count+1);
		float* n_color_i = findex( n_color,0);
		while (color_i < color_end) {
			color_i[0] = n_color_i[0];
			color_i[1] = n_color_i[1];
			color_i[2] = n_color_i[2];
			color_i += 3;
			n_color_i += 3;
		}
		return;
	}
	throw std::invalid_argument( "color must be an Nx3 or Nx4 array");
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
	set_length( shape(red).at(0));
	color[make_tuple(slice(1,count+1), 0)] = red;
}

void
curve::set_green( const array& green)
{
	set_length( shape(green).at(0));
	color[make_tuple(slice(1,count+1), 1)] = green;
}

void
curve::set_blue( const array& blue)
{
	set_length( shape(blue).at(0));
	color[make_tuple(slice(1,count+1), 2)] = blue;
}

void
curve::set_x( const array& x)
{
	set_length( shape(x).at(0));
	pos[make_tuple( slice(1, count+1), 0)] = x;
}

void
curve::set_y( const array& y)
{
	set_length( shape(y).at(0));
	pos[make_tuple( slice(1, count+1), 1)] = y;
}

void
curve::set_z( const array& z)
{
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
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 0)] = x;
}

void
curve::set_y_d( const double y)
{
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 1)] = y;
}

void
curve::set_z_d( const double z)
{
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(1,count+1), 2)] = z;
}

void
curve::set_red_d( const double red)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 0)] = red;
}

void
curve::set_green_d( const double green)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 1)] = green;
}

void
curve::set_blue_d( const double blue)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(1,count+1), 2)] = blue;
}

void
curve::set_radius( const double& radius)
{
	this->radius = radius;
}

void
curve::set_antialias( bool aa)
{
	this->antialias = aa;
}


bool
curve::degenerate() const
{
	return count < 2;
}


bool
curve::monochrome(float* tcolor, size_t pcount)
{
	rgb first_color( tcolor[0], tcolor[1], tcolor[2]);
	size_t nn;

	for(nn=0; nn<pcount; nn++)  {
		if (tcolor[nn] != first_color.red)
			return false;
		if (tcolor[nn+1] != first_color.green)
			return false;
		if (tcolor[nn+2] != first_color.blue)
			return false;
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
	// Aack, I can't think of any obvious optimizations here.
	// But since Visual 3 didn't permit picking of curves, omit for now.
	// We can't afford it; serious impact on performance.
	//gl_render( scene);
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
	bool mono = monochrome(tcolor, pcount);
	if (mono) {
		// We can get away without using a color array.
		rendered_color = rgb( tcolor[0], tcolor[1], tcolor[2]);
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				rendered_color.desaturate().gl_set(opacity);
			else
				rendered_color.grayscale().gl_set(opacity);
		}
		else
			rendered_color.gl_set(opacity);
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
curve::thickline( const view& scene, double* spos, float* tcolor, size_t pcount, double scaled_radius)
{
	size_t curve_around = sides;
	std::vector<vector> projected;
	std::vector<vector> normals;
	std::vector<rgb> light;

	float *cost = curve_sc;
	float *sint = cost + curve_around;

	vector pts[MAX_SIDES]; // points around the previous segment

	vector lastA; // unit vector of previous segment

	// If closed curve (or very nearly), make (pcount-1) point be
	// exactly equal to point 0, and add two more points equal
	// to points 1 and 2. At start of vertex generation, don't add
	// 0th point to the vertex list; it will be added (with a mitered
	// joint) when processing the last point.
	bool closed = false;
	if (pcount > 3) {
		if (spos[3*(pcount-1)] == spos[0] &&
			spos[3*(pcount-1)+1] == spos[1] &&
			spos[3*(pcount-1)+2] == spos[2]) {
			closed = true;
			// Add an element with the 0th position and 0th color.
			// Add an element with the 1st position and color.
			// Add an element with the 2nd position and color.
			for (size_t i=0; i<9; i++) {
				spos[3*pcount+i] = spos[i];
				tcolor[3*pcount+i] = tcolor[i];
			}
			pcount += 2; // the last added point is there only for look-ahead mitering
		}
	}

	// pos and color iterators
	const double* v_i = spos;
	const float* c_i = tcolor;
	bool mono = adjust_colors( scene, tcolor, pcount);

	for(size_t corner=0; corner < pcount; ++corner, v_i += 3, c_i += 3 ) {
		// The vector to which v_i currently points towards.
		vector current( v_i[0], v_i[1], v_i[2] );

		/* For the first point in the curve, create orthogonal unit vectors x and y
		 * and use these to generate vertices in the cross section of the first segment.
		 *
		 * Also save the generated points in the pts array.
		 *
		 * At each succeeding point in the curve, use the pts array as the basis
		 * for generating vertices for the end of the previous segment, on an
		 * ellipse that is the intersection of the previous and next segments.
		 * Update the pts array with points reflected through the ellipse, which
		 * will be used to generate vertices at the end of the new segment.
		 *
		 * The result is mitered joints, with no twisting from one joint to the next.
		 *
		 * In the case of a closed curve, we added copies of the first three points and
		 * we don't initial add vertices for the first point (point 0) in the vertex array.
		 * These get added at the end of the process, with a clean break in the color (if
		 * the color of the first and last point are different).
		 */

		vector next, A;
		if ((corner == pcount-1) && !closed) {
			next = lastA;
			A = lastA;
		} else {
			next = vector( v_i[3], v_i[4], v_i[5]); // The next vector in spos
			A = (next - current).norm();
		}
		if (corner == 0) {
			lastA = A;
		}
		int savecorner = -1; // assume no duplication of neighboring points
		if (!A) {
			savecorner = corner;
			for(corner += 1, v_i += 3, c_i += 3; corner < pcount; ++corner, v_i += 3, c_i += 3 ) {
				next = vector( v_i[3], v_i[4], v_i[5]);
				A = (next - current).norm();
			 	if (!A) continue;
			 	break;
			}
			if (!A) A = lastA;
			if (!A) A = vector(1,0,0); // desperation
		}

		if (corner == 0) {
			vector y = vector(0,1,0);
			vector x = A.cross(y).norm();
			if (!x) {
				x = A.cross( vector(0, 0, 1));
				if (!x) x = A.cross( vector(0, 1, 0));
			}
			y = x.cross(A).norm();

			if (!x || !y || x == y) {
				std::ostringstream msg;
				msg << "Degenerate curve case! please report the following "
					"information to visualpython-users@lists.sourceforge.net: ";
				msg << "current:" << current << " next:" << next
			 		<< " A:" << A << " x:" << x << " y:" << y
			 		<< std::endl;
			 	VPYTHON_WARNING( msg.str());
			}

			// scale radii
			x *= scaled_radius;
			y *= scaled_radius;

			for (size_t a=0; a < curve_around; a++) {
				float c = cost[a];
				float s = sint[a];
				vector rel = x*s + y*c; // first point is "up"
				pts[a] = rel;
				if (!closed) {
					normals.push_back( rel.norm());
					projected.push_back( current + rel );
					if (!mono) {
						light.push_back( rgb( c_i[0], c_i[1], c_i[2]) );
					}
				}
			}
			continue;
		}

		// Make a "mitered" joint; the cross section of the joint is an ellipse.
		vector joint_axis = lastA.cross(A).norm(); // perpendicular to the plane of lastA and A
		vector perp = (lastA+A).norm(); // perpendicular to joint cross section
		double costheta = lastA.dot(perp); // cos of angle new cross section makes with previous segment (0 to pi/2)
		double tantheta;
		if (fabs(costheta) > .99999 || fabs(costheta) < .00001) {
			tantheta = 0.; // 2nd segment in same or opposite direction to 1st segment, or this is the last point
		} else {
			tantheta = sqrt(1.-costheta*costheta)/costheta;
		}

		if (savecorner >= 0) { // a duplicated point
			for (size_t a=0; a < curve_around; a++) {
				vector rel = pts[a];
				vector rel_ellipse = rel - rel.cross(joint_axis)*tantheta;
				normals.push_back( rel_ellipse.norm() );
				projected.push_back( current + rel_ellipse );
				if (!mono) {
					light.push_back(
						rgb( tcolor[3*savecorner],
							 tcolor[3*savecorner+1],
							 tcolor[3*savecorner+2]) );
				}
			}
		}

		for (size_t a=0; a < curve_around; a++) {
			vector rel = pts[a];
			pts[a] = rel - 2*(rel.dot(perp))*perp;
			vector rel_ellipse = rel - rel.cross(joint_axis)*tantheta;
			normals.push_back( rel_ellipse.norm() );
			projected.push_back( current + rel_ellipse );
			if (!mono) {
				light.push_back( rgb( c_i[0], c_i[1], c_i[2]) );
			}
		}
		lastA = A;
	}

	if (closed) pcount -= 1; // correct for extra point added in closed case

	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	if (!mono) {
		glEnableClientState( GL_COLOR_ARRAY);
	}

	int *ind = curve_slice;
	for (size_t a=0; a < curve_around; a++) {
		size_t ai = a;
		if (a == curve_around-1) {
			ind += 256; // upper portion of curve_slice indices, for the last side
			ai = 0;
		}

		// List all the vertices for the ai-th side of the thick line:
		for (size_t i = 0; i < pcount; i += 127u) {
			glVertexPointer(3, GL_DOUBLE, sizeof( vector), &projected[i*curve_around + ai].x);
			if (!mono)
				glColorPointer(3, GL_FLOAT, sizeof( rgb), &light[(i*curve_around + ai)].red );
			glNormalPointer( GL_DOUBLE, sizeof(vector), &normals[i*curve_around + ai].x);
			if (pcount-i < 128)
				glDrawElements(GL_TRIANGLE_STRIP, 2*(pcount-i), GL_UNSIGNED_INT, ind);
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
	// The maximum number of points to display.
	const int LINE_LENGTH = 1000;
	// Data storage for the position and color data (plus room for 3 extra points)
	double spos[3*(LINE_LENGTH+3)];
	float tcolor[3*(LINE_LENGTH+3)]; // opacity not yet implemented for curves
	float fstep = (float)(count-1)/(float)(LINE_LENGTH-1);
	if (fstep < 1.0F) fstep = 1.0F;
	size_t iptr=0, iptr3, cptr, pcount=0;

	const double* p_i = (double*)( data(this->pos));
	const float* c_i = (float*)( data(this->color));

	// Choose which points to display
	for (float fptr=0.0; iptr < count && pcount < LINE_LENGTH; fptr += fstep, iptr = (int) (fptr+.5), ++pcount) {
		iptr3 = 3*(iptr+1); // first real point is the second in the data array
		cptr = 3*(iptr+1); // color is rgb (3 floats)
		spos[3*pcount] = p_i[iptr3];
		spos[3*pcount+1] = p_i[iptr3+1];
		spos[3*pcount+2] = p_i[iptr3+2];
		tcolor[3*pcount] = c_i[cptr];
		tcolor[3*pcount+1] = c_i[cptr+1];
		tcolor[3*pcount+2] = c_i[cptr+2];
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

	if (radius == 0.0) {
		glEnableClientState( GL_VERTEX_ARRAY);
		glDisable( GL_LIGHTING);
		// Assume monochrome.
		if (antialias) {
			glEnable( GL_LINE_SMOOTH);
		}

		glVertexPointer( 3, GL_DOUBLE, 0, spos);
		bool mono = adjust_colors( scene, tcolor, pcount);
		if (!mono) glColorPointer( 3, GL_FLOAT, 0, tcolor);
		glDrawArrays( GL_LINE_STRIP, 0, pcount);
		glDisableClientState( GL_VERTEX_ARRAY);
		glDisableClientState( GL_COLOR_ARRAY);
		glEnable( GL_LIGHTING);
		if (antialias) {
			glDisable( GL_LINE_SMOOTH);
		}
	}
	else {
		thickline( scene, spos, tcolor, pcount, scaled_radius);
	}

	check_gl_error();

}

void
curve::get_material_matrix( const view& v, tmatrix& out ) {
	if (degenerate()) return;

	// xxx note this code is identical to faces::get_material_matrix, except for considering radius

	// xxx Add some caching for extent with grow_extent etc; once locking changes so we can trust the primitive not to change during rendering
	vector min_extent, max_extent;
	double* pos_i = index( pos, 0);
	double* pos_end = index( pos, count );
	min_extent = max_extent = vector( pos_i ); pos_i += 3;
	while (pos_i < pos_end)
		for(int j=0; j<3; j++) {
			if (*pos_i < min_extent[j]) min_extent[j] = *pos_i;
			else if (*pos_i > max_extent[j]) max_extent[j] = *pos_i;
			pos_i++;
		}

	min_extent -= vector(radius,radius,radius);
	max_extent += vector(radius,radius,radius);

	out.translate( vector(.5,.5,.5) );
	out.scale( vector(1,1,1) * (.999 / (v.gcf * std::max(max_extent.x-min_extent.x, std::max(max_extent.y-min_extent.y, max_extent.z-min_extent.z)))) );
	out.translate( -.5 * v.gcf * (min_extent + max_extent) );
}

} } // !namespace cvisual::python
