// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <boost/python/detail/wrap_python.hpp>

#include "util/errors.hpp"
#include "util/checksum.hpp"

#include "python/slice.hpp"
#include "python/curve.hpp"

#include <stdexcept>
#include <cassert>
#include <GL/gle.h>

using boost::python::make_tuple;
using boost::python::object;

// Recall that the default constructor for object() is a reference to None.

namespace cvisual { namespace python {

const size_t curve::c_cache::items;

namespace {
	
// returns a pointer to the ith vector in the array.
double* index( array a, size_t i)
{
	// This is technically an unsafe cast since the alignment requirement
	// goes up for the cast.  It is made safe by padding actions within the Numeric
	// library itself, but the compiler doesn't know that, so I am just using a
	// raw cast vice a static_cast<>.
	return ((double*)data(a)) + (i+1) * 3;
}

float* findex( array a, size_t i)
{
	return ((float*)data(a)) + (i+1)*3;
}
	
} // !namespace visual::(unnamed)


curve::curve()
	: pos( 0), color( 0), antialias( false),
	radius(0.0),
	preallocated_size(257),
	count(0)
{
	// Perform initial allocation of storage space for the buggar.
	std::vector<int> dims(2);
	dims[0] = preallocated_size;
	dims[1] = 3;
	pos = makeNum( dims);
	color = makeNum( dims, PyArray_FLOAT);
	double* pos_i = index( pos, 0);
	float* color_i = findex( color, 0);
	pos_i[0] = 0;
	pos_i[1] = 0;
	pos_i[2] = 0;
	color_i[0] = 1;
	color_i[1] = 1;
	color_i[2] = 1;
}

curve::curve( const curve& other)
	: renderable( other), pos( other.pos), color( other.color), 
	antialias( other.antialias), 
	radius( other.radius), preallocated_size( other.preallocated_size),
	count( other.count)
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
	if (npoints > length) // A shrink operation - never done by VPython.
		npoints = length;
	if (npoints == 0)
		// The first allocation.
		npoints = 1;
		
	if (length > preallocated_size-2) {
		VPYTHON_NOTE( "Reallocating buffers for a curve object.");
		std::vector<int> dims(2);
		dims[0] = 2*length + 2;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		array n_color = makeNum( dims, PyArray_FLOAT);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * (npoints+1));
		std::memcpy( data( n_color), data( color), sizeof(float) * 3 * (npoints+1));
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
	int cache_length = (count+c_cache::items)/(c_cache::items-1) - cache.size();
	while (cache_length > 0) {
		cache.push_back( c_cache());
		cache_length--;
	}
}

void
curve::append_rgb( vector npos, float red, float blue, float green) 
{
	lock L(mtx);
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
curve::append( vector npos)
{
	lock L(mtx);
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
curve::set_pos( array n_pos)
{
	PyArray_TYPES t = type( n_pos);
	if (t != PyArray_DOUBLE) {
		n_pos = astype( n_pos, PyArray_DOUBLE);
	}
	std::vector<int> dims = shape( n_pos);
	if (dims.size() == 1 && !dims[0]) {
		lock L(mtx);
		set_length(0);
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
	if (dims[1] == 2) {
		lock L(mtx);
		set_length( dims[0]);
		pos[make_tuple(slice(1, count+1), slice(0,2))] = n_pos;
		pos[make_tuple(slice(1, count+1), 2)] = 0.0;
		return;
	}
	else if (dims[1] == 3) {
		lock L(mtx);
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
	tuple t_pos = make_tuple( make_tuple( npos.x, npos.y, npos.z));
	set_pos( array(t_pos));
}

void
curve::set_color( array n_color)
{
	PyArray_TYPES t = type(n_color);
	if (t != PyArray_FLOAT) {
		n_color = astype( n_color, PyArray_FLOAT);
	}
	std::vector<int> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		lock L(mtx);
		color[slice(1,npoints+1)] = n_color;
		return;
	}
	if (dims.size() == 2 && dims[1] == 3) {
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		lock L(mtx);
		color[slice(1, count+1)] = n_color;
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
	color[make_tuple(slice(0,count), 2)] = blue;
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

bool
curve::degenerate() const
{
	return count < 2;
}


bool
curve::monochrome( size_t begin, size_t end)
{
	const float* color_i = findex( color, begin);
	const float* color_end = findex( color, end);
	
	rgb first_color( color_i[0], color_i[1], color_i[2]);
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
	vector begin( index(pos, 1));
	vector end( index(pos, count-1));
	return eq_frac(begin.x, end.x, 1e-5) 
		&& eq_frac(begin.y, end.y, 1e-5) 
		&& eq_frac(begin.z, end.z, 1e-5);
}

long
curve::checksum( size_t begin, size_t end)
{
	using cvisual::checksum;
	long ret = 0;
	
	checksum( ret, &radius);
	const double* pos_i = index( pos, begin);
	const double* pos_end = index( pos, end);
	while (pos_i < pos_end) {
		checksum( ret, pos_i);
		checksum( ret, pos_i+1);
		checksum( ret, pos_i+2);
		pos_i += 3;
	}
	const float* color_i = findex( color, begin);
	const float* color_end = findex( color, end);
	while (color_i < color_end) {
		checksum( ret, color_i);
		checksum( ret, color_i+1);
		checksum( ret, color_i+2);
		color_i += 3;
	}
	return ret;
}

vector
curve::get_center() const
{
	if (count == 0)
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
	const double* pos_i = index(pos, 0);
	const double* pos_end = index( pos, count);
	for ( ; pos_i < pos_end; pos_i += 3) {
		world.add_point( vector(pos_i));
	}
}

void
curve::thinline( const view& scene, size_t begin, size_t end)
{
	assert( end > begin);
	// The following may be empty, but they are kept at this scope to ensure
	// that their memory will be valid when rendering the body below.
	double (*spos)[3] = NULL;
	float (*tcolor)[3] = NULL;
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		spos = new double[end-begin][3];
		const double* pos_i = index( pos, begin);
		for (size_t i = 0; i < end-begin; ++i, pos_i += 3) {
			spos[i][0] = pos_i[0] * scene.gcf;
			spos[i][1] = pos_i[1] * scene.gcf;
			spos[i][2] = pos_i[2] * scene.gcf;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, spos);
	}
	else {
		glVertexPointer( 3, GL_DOUBLE, 0, index(pos, begin));
	}
	bool segment_monochrome = monochrome( begin, end);
	if (segment_monochrome) {
		// We can get away without using a color array.
		const float* c_i = findex( color, begin);
		rgb scolor( c_i[0], c_i[1], c_i[2]);
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				scolor.desaturate().gl_set();
			else
				scolor.grayscale().gl_set();
		}
		else
			scolor.gl_set();
	}
	else {
		glEnableClientState( GL_COLOR_ARRAY);
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			tcolor = new float[end-begin][3];
			
			const float* color_i = findex( color, begin);
			const float* color_end = findex( color, end);
			for (size_t i = 0; i < end-begin; ++i, color_i += 3) {
				rgb scolor( color_i[0], color_i[1], color_i[2]);
				if (scene.coloranaglyph)
					scolor = scolor.desaturate();
				else
					scolor = scolor.grayscale();
				tcolor[i][0] = scolor.red;
				tcolor[i][1] = scolor.green;
				tcolor[i][2] = scolor.blue;
			}
			glColorPointer( 3, GL_FLOAT, 0, tcolor);
		}
		else
			glColorPointer( 3, GL_FLOAT, 0, index( color, begin));
	}
	glDrawArrays( GL_LINE_STRIP, 0, end-begin);
	
	if (!segment_monochrome)
		glDisableClientState( GL_COLOR_ARRAY);
	if (spos)
		delete[] spos;
	if (tcolor)
		delete[] tcolor;
}

namespace {
template <typename T>
struct converter
{
	T data[3];
};
} // !namespace (anonymous)


void
curve::thickline( const view& scene, size_t begin, size_t end)
{
	assert( end > begin);
	std::vector<rgb> tcolor;
	std::vector<vector> spos;
	float (*used_color)[3] = &((converter<float>*)index( color, begin-1))->data;
	double (*used_pos)[3] = &((converter<double>*)index( pos, begin-1))->data;
	
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		std::vector<vector> tmp(end-begin+2);
		spos.swap( tmp);
		const double* pos_i = index( pos, begin-1);
		const double* pos_end = index( pos, end+1);
		for (std::vector<vector>::iterator i = spos.begin(); 
				i < spos.end() && pos_i < pos_end; ++i, pos_i += 3) {
			i->x = pos_i[0] * scene.gcf;
			i->y = pos_i[1] * scene.gcf;
			i->z = pos_i[2] * scene.gcf;
		}
		used_pos = &((converter<double>*)&*spos.begin())->data;
	}
	if (monochrome( begin, end)) {
		// Can get away without using a color array.
		const float* color_i = findex( color, begin);
		rgb scolor( color_i[0], color_i[1], color_i[2]);
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				scolor.desaturate().gl_set();
			else
				scolor.grayscale().gl_set();
		}
		else
			scolor.gl_set();
		
		glePolyCylinder( 
			end-begin+2, used_pos,
			0, radius * scene.gcf);
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			std::vector<rgb> tmp( end-begin+2);
			tcolor.swap(tmp);
			const float* color_i = findex( color, begin-1);
			const float* color_end = findex( color, begin+1);
			for (std::vector<rgb>::iterator i = tcolor.begin(); 
				i < tcolor.end() && color_i < color_end; 
				++i, color_i+=3) {
				rgb scolor( color_i[0], color_i[1], color_i[2]);
				if (scene.coloranaglyph)
					*i = scolor.desaturate();
				else
					*i = scolor.grayscale();
			}
			used_color = &((converter<float>*)&*tcolor.begin())->data;
		}
		
		glePolyCylinder( 
			end-begin+2, used_pos,
			used_color, radius * scene.gcf);
	}
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

	clear_gl_error();
	static bool first = true;
	if (first) {
		gleSetNumSides(6);
		gleSetJoinStyle( TUBE_JN_ANGLE | TUBE_NORM_PATH_EDGE);
		first = false;
	}
	
	size_t size = std::min(c_cache::items, true_size);
	size_t begin = 0;
	cache_iterator c = cache.begin();
	cache_iterator c_end = cache.end();
	const bool do_thinline = (radius == 0.0);
	if (do_thinline) {
		glEnableClientState( GL_VERTEX_ARRAY);
		glDisable( GL_LIGHTING);
		// Assume monochrome.
		glEnable( GL_BLEND);
		glEnable( GL_LINE_SMOOTH);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE);	
	}
	while (size > 1) {
		assert( c != c_end);
		long check = checksum( begin, begin+size);
		if (check == c->checksum)
			c->gl_cache.gl_render();
		else {
			c->gl_cache.gl_compile_begin();
			if (do_thinline)
				thinline( scene, begin, begin+size);
			else
				thickline( scene, begin, begin+size);
			c->gl_cache.gl_compile_end();
			c->checksum = check;
			c->gl_cache.gl_render();
		}
		begin += size-1;
		size = (begin + 256 < true_size)
			? 256 : true_size-begin;
		++c;
	}
	if (do_thinline) {
		glDisableClientState( GL_VERTEX_ARRAY);
		glDisableClientState( GL_COLOR_ARRAY);
		glEnable( GL_LIGHTING);
		glDisable( GL_BLEND);
		glDisable( GL_LINE_SMOOTH);
	}
	check_gl_error();

}

} } // !namespace cvisual::python
