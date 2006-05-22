// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "python/faces.hpp"
#include <boost/python/tuple.hpp>

#include <map>
#include "wrap_gl.hpp"

#include "python/slice.hpp"
#include "util/gl_enable.hpp"

using boost::python::numeric::array;

namespace cvisual { namespace python {
	
bool
faces::degenerate() const
{
	return count < 3;
}


namespace {	
// returns a pointer to the ith vector in the array.
double* index( const array& a, size_t i)
{
	// This is technically an unsafe cast since the alignment requirement
	// goes up for the cast.  It is made safe by padding actions within the Numeric
	// library itself, but the compiler doesn't know that, so I am just using a
	// raw cast vice a static_cast<>.
	return ((double*)data(a)) + i * 3;
}

float* findex( const array& a, size_t i)
{
	return ((float*)data(a)) + i * 3;
}

} // !namespace (unnamed)
	
faces::faces()
	: pos(0), color(0), normal(0), preallocated_size(256),
	count(0)
{
	std::vector<int> dims(2);
	dims[0] = 256;
	dims[1] = 3;
	pos = makeNum(dims);
	color = makeNum(dims, float_t);
	normal = makeNum(dims);
	double* i = index( pos, 0);
	i[0] = i[1] = i[2] = 0.0;
	float* j = findex( color,0);
	j[0] = j[1] = j[2] = 1.0;
	i = index( normal,0);
	i[0] = i[1] = i[2] = 0.0;
}

faces::faces( const faces& other)
	: renderable( other), pos( other.pos), color( other.color), 
	normal( other.normal),
	preallocated_size( other.preallocated_size),
	count( other.count)
{
}

faces::~faces()
{
}

void
faces::set_length( int length)
{
	int npoints = count;
	if (npoints > length) // A shrink operation - never done by VPython.
		npoints = length;
	if (npoints == 0) // The first allocation.
		npoints = 1;
		
	if (length > preallocated_size) {
		std::vector<int> dims(2);
		dims[0] = 2 * length;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		array n_color = makeNum( dims, float_t);
		array n_normal = makeNum( dims);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * npoints);
		std::memcpy( data( n_color), data( color), sizeof(float) * 3 * npoints);
		std::memcpy( data( n_normal), data( normal), sizeof(double) * 3*npoints);
		pos = n_pos;
		color = n_color;
		normal = n_normal;
		preallocated_size = dims[0];
	}
	if (length > npoints) {
		// Copy the last good element to the new positions.
		const double* last_element = index( pos, npoints-1);
		double* element_i = index( pos, npoints);
		double* element_end = index( pos, length);
		while (element_i < element_end) {
			element_i[0] = last_element[0];
			element_i[1] = last_element[1];
			element_i[2] = last_element[2];
			element_i += 3;
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
		
		last_element = index( normal, npoints-1);
		element_i = index( normal, npoints);
		element_end = index( normal, length);
		while (element_i < element_end) {
			element_i[0] = last_element[0];
			element_i[1] = last_element[1];
			element_i[2] = last_element[2];
			element_i += 3;
		}
	}
	count = length;
}

void
faces::append( vector nv_pos, vector nv_normal, rgb nv_color)
{
	lock L(mtx);
	set_length( count+1);
	double* pos_data = index( pos, count-1);
	double* norm_data = index(normal, count-1);
	float* color_data = findex(color, count-1);
	pos_data[0] = nv_pos.get_x();
	pos_data[1] = nv_pos.get_y();
	pos_data[2] = nv_pos.get_z();
	norm_data[0] = nv_normal.get_x();
	norm_data[1] = nv_normal.get_y();
	norm_data[2] = nv_normal.get_z();
	color_data[0] = nv_color.red;
	color_data[1] = nv_color.blue;
	color_data[2] = nv_color.green;
}

void
faces::append( vector n_pos, vector n_normal)
{
	lock L(mtx);
	set_length( count+1);
	double* pos_data = index( pos, count-1);
	double* norm_data = index(normal, count-1);
	pos_data[0] = n_pos.get_x();
	pos_data[1] = n_pos.get_y();
	pos_data[2] = n_pos.get_z();
	norm_data[0] = n_normal.get_x();
	norm_data[1] = n_normal.get_y();
	norm_data[2] = n_normal.get_z();
}

// Define an ordering for the stl-sorting criteria.
struct stl_cmp_vector
{
	bool operator()( const vector& lhs, const vector& rhs)
	{
		if (lhs.x < rhs.x)
			return true;
		else if (lhs.x > rhs.x)
			return false;
		else
			if (lhs.y < rhs.y)
				return true;
			else if (lhs.y > rhs.y)
				return false;
			else
				if (lhs.z < rhs.z)
					return true;
				else
					return false;
	}
};

void
faces::smooth_shade(bool doublesided)
{
	if (shape(pos) != shape(normal))
		throw std::invalid_argument( "Dimension mismatch between pos and normal.");
	
	lock L(mtx);
	
	// positions -> normals
	std::map< const vector, vector, stl_cmp_vector> verticies;
	std::map< const vector, vector, stl_cmp_vector> verticies_backface;
	
	const double* pos_i = index(pos, 0);
	double* norm_i = index(normal, 0);
	const double* pos_end = index( pos, count);
	for ( ; pos_i < pos_end; pos_i+=3, norm_i+=3) {
		// If there isn't a normal at the specified position, it will be default
		// initialized to zero.  If there already is one, it will be returned.
		if (doublesided) {
			if (verticies[vector(pos_i)].dot( vector(norm_i)) >= 0.0) {
				verticies[vector(pos_i)] += vector(norm_i);
			}
			else {
				verticies_backface[vector(pos_i)] += vector(norm_i);
			}
		}
		else {
			verticies[vector(pos_i)] += vector(norm_i);
		}
	}
	
	pos_i = index(pos, 0);
	norm_i = index(normal, 0);
	vector tmp;
	for ( ; pos_i < pos_end; pos_i+=3, norm_i+=3) {
		if (doublesided) {
			if (verticies[vector(pos_i)].dot( vector(norm_i)) >= 0.0) {
				tmp = verticies[vector(pos_i)].norm();
			}
			else {
				tmp = verticies_backface[vector(pos_i)].norm();
			}
		}
		else {
			tmp = verticies[vector(pos_i)].norm();
		}
		norm_i[0] = tmp.get_x();
		norm_i[1] = tmp.get_y();
		norm_i[2] = tmp.get_z();
	}
}
	
void  
faces::set_pos( const array& n_pos)
{
	using namespace python;

	std::vector<int> n_dims = shape(n_pos);
	std::vector<int> dims = shape(this->pos);

	if (n_dims.size() == 1 && !n_dims[0]) {
		lock L(mtx);
		set_length(0);
		return;	
	}
	if (n_dims.size() != 2)
		throw std::invalid_argument( "Numeric.array members must be Nx3 arrays.");

	if (n_dims[1] == 2) {
		lock L(mtx);
		set_length( n_dims[0]);
		pos[make_tuple( slice(0,count), slice(0,2))] = n_pos;
		pos[make_tuple( slice(0,count), 2)] = 0.0;
	}
	else if (n_dims[1] == 3) {
		lock L(mtx);
		set_length( n_dims[0]);
		pos[slice(0, count)] = n_pos;
	}
	else
		throw std::invalid_argument( "Numeric.array members must be Nx3 arrays.");
}

void
faces::set_pos_l( boost::python::list l)
{
	set_pos( array( l));
}

boost::python::object 
faces::get_pos() 
{ 
	return pos[slice(0, count)];
}

boost::python::object 
faces::get_color()
{ 
	return color[slice(0, count)]; 
}

boost::python::object 
faces::get_normal() 
{ 
	return normal[slice(0, count)]; 
}


void  
faces::set_color( array n_color)
{
	using namespace boost::python;
	using cvisual::python::slice;
	
	std::vector<int> n_dims = shape(n_color);

	if (n_dims.size() != 2 && n_dims[1] != 3)
		throw std::invalid_argument( "color must be an Nx3 array.");
	if (n_dims[0] != count)
		throw std::invalid_argument( "color must be the same size as pos.");

	if (type(n_color) != float_t) {
		n_color = astype( n_color, float_t);
	}
	lock L(mtx);
	color[slice(0, count)] = n_color;
}

void
faces::set_color_l( boost::python::list color)
{
	set_color( array(color));	
}

void
faces::set_color_t( rgb c)
{
	using boost::python::make_tuple;
	// Broadcast the new color across the array.
	int npoints = count ? count : 1;
	lock L(mtx);
	color[slice(0, npoints)] = make_tuple( c.red, c.green, c.blue);
}

void  
faces::set_normal( const array& n_normal)
{
	lock L(mtx);
	normal[slice(0, count)] = n_normal;
}

void
faces::gl_render( const view& scene)
{
	if (degenerate())
		return;
	
	// Initialize iterators and endpoints.
	const float* color_i = findex(color, 0);
	
	std::vector<vector> spos;
	std::vector<rgb> tcolor;
	
	gl_enable_client vertexes( GL_VERTEX_ARRAY);
	gl_enable_client normals( GL_NORMAL_ARRAY);
	gl_enable_client colors( GL_COLOR_ARRAY);
	
	glNormalPointer( GL_DOUBLE, 0, index( normal, 0));
	
	if (scene.gcf != 1.0) {
		std::vector<vector> tmp( count);
		spos.swap( tmp);
		const double* pos_i = index(pos, 0);
		for (std::vector<vector>::iterator i = spos.begin(); i != spos.end(); ++i) {
			*i = scene.gcf * vector(pos_i);
			pos_i++;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, &*spos.begin());
	}
	else
		glVertexPointer( 3, GL_DOUBLE, 0, index( pos,0));
		
	if (scene.anaglyph) {
		std::vector<rgb> tmp( count);
		tcolor.swap( tmp);
		const float* color_i = findex( color, 0);
		for (std::vector<rgb>::iterator i = tcolor.begin(); i != tcolor.end(); ++i) {
			if (scene.coloranaglyph)
				*i = rgb(color_i[0], color_i[1], color_i[2]).desaturate();
			else
				*i =  rgb(color_i[0], color_i[1], color_i[2]).grayscale();
			color_i++;
		}
		glColorPointer( 3, GL_FLOAT, 0, &*tcolor.begin());
	}
	else
		glColorPointer( 3, GL_FLOAT, 0, findex( color, 0));
	
	gl_enable cull_face( GL_CULL_FACE);
	for (int drawn = 0; drawn < count - count%3; drawn += 54) {
		glDrawArrays( GL_TRIANGLES, drawn, 
			std::min( count - count%3 - drawn, (int)54));
	}
}

void 
faces::gl_pick_render( const view& scene)
{
	gl_render( scene);
}

vector 
faces::get_center() const
{
	vector ret;
	double* pos_i = index( pos, 0);
	double* pos_end = index( pos, count);
	while (pos_i < pos_end) {
		ret += vector(pos_i);
		pos_i++;
	}
	if (count)
		ret /= count;
	return ret;
}

void 
faces::grow_extent( extent& world)
{
	double* pos_i = index( pos, 0);
	double* pos_end = index( pos, count);
	while (pos_i < pos_end) {
		world.add_point( vector(pos_i));
		pos_i++;
	}
}

void
faces::set_normal_l( boost::python::list normals)
{
	set_normal( array(normals));	
}
	
} } // !namespace cvisual::python
