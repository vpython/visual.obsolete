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
	return ((float*)data(a)) + i * 3; // (red,green,blue)
}

} // !namespace (unnamed)

faces::faces()
	: pos(0), color(0), normal(0), preallocated_size(256),
	count(0)
{
	std::vector<npy_intp> dims(2);
	dims[0] = 256;
	dims[1] = 3;
	pos = makeNum(dims);
	normal = makeNum(dims);
	dims[1] = 4;
	color = makeNum(dims, NPY_FLOAT);
	double* i = index( pos, 0);
	i[0] = i[1] = i[2] = 0.0;
	float* j = findex( color,0);
	j[0] = j[1] = j[2] = 1.0;
	double* k = index( normal,0);
	k[0] = k[1] = k[2] = 0.0;
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
		std::vector<npy_intp> dims(2);
		dims[0] = 2 * length;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		array n_normal = makeNum( dims);
		array n_color = makeNum( dims, NPY_FLOAT);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * npoints);
		std::memcpy( data( n_normal), data( normal), sizeof(double) * 3 * npoints);
		std::memcpy( data( n_color), data( color), sizeof(float) * 3 * npoints);
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

		const double* last_normal = index( normal, npoints-1);
		double* normal_i = index( normal, npoints);
		double* normal_end = index( normal, length);
		while (normal_i < normal_end) {
			normal_i[0] = last_normal[0];
			normal_i[1] = last_normal[1];
			normal_i[2] = last_normal[2];
			normal_i += 3;
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
faces::append_rgb( vector nv_pos, vector nv_normal, float red, float green, float blue)
{
	set_length( count+1);
	double* pos_data = index( pos, count-1);
	double* norm_data = index(normal, count-1);
	float* last_color = findex( color, count-1);
	pos_data[0] = nv_pos.get_x();
	pos_data[1] = nv_pos.get_y();
	pos_data[2] = nv_pos.get_z();
	norm_data[0] = nv_normal.get_x();
	norm_data[1] = nv_normal.get_y();
	norm_data[2] = nv_normal.get_z();
	if (red != -1)
		last_color[0] = red;
	if (green != -1)
		last_color[1] = green;
	if (blue != -1)
		last_color[2] = blue;
}

void
faces::append( vector nv_pos, vector nv_normal, rgb nv_color)
{
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
	color_data[1] = nv_color.green;
	color_data[2] = nv_color.blue;
}

void
faces::append( vector n_pos, vector n_normal)
{
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
	//AS added "const" to allow template match for VC++ build
 	bool operator()( const vector& lhs, const vector& rhs) const	{
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

	// positions -> normals
	std::map< const vector, vector, stl_cmp_vector> vertices;
	std::map< const vector, vector, stl_cmp_vector> vertices_backface;

	const double* pos_i = index(pos, 0);
	double* norm_i = index(normal, 0);
	const double* pos_end = index( pos, count);
	for ( ; pos_i < pos_end; pos_i+=3, norm_i+=3) {
		// If there isn't a normal at the specified position, it will be default
		// initialized to zero.  If there already is one, it will be returned.
		if (doublesided) {
			if (vertices[vector(pos_i)].dot( vector(norm_i)) >= 0.0) {
				vertices[vector(pos_i)] += vector(norm_i);
			}
			else {
				vertices_backface[vector(pos_i)] += vector(norm_i);
			}
		}
		else {
			vertices[vector(pos_i)] += vector(norm_i);
		}
	}

	pos_i = index(pos, 0);
	norm_i = index(normal, 0);
	vector tmp;
	for ( ; pos_i < pos_end; pos_i+=3, norm_i+=3) {
		if (doublesided) {
			if (vertices[vector(pos_i)].dot( vector(norm_i)) >= 0.0) {
				tmp = vertices[vector(pos_i)].norm();
			}
			else {
				tmp = vertices_backface[vector(pos_i)].norm();
			}
		}
		else {
			tmp = vertices[vector(pos_i)].norm();
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

	std::vector<npy_intp> n_dims = shape(n_pos);
	std::vector<npy_intp> dims = shape(this->pos);

	if (n_dims.size() == 1 && !n_dims[0]) {
		set_length(0);
		return;
	}
	if (n_dims.size() != 2)
		throw std::invalid_argument( "Numeric.array members must be Nx3 arrays.");

	if (n_dims[1] == 2) {
		set_length( n_dims[0]);
		pos[make_tuple( slice(0,count), slice(0,2))] = n_pos;
		pos[make_tuple( slice(0,count), 2)] = 0.0;
	}
	else if (n_dims[1] == 3) {
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
	
	NPY_TYPES t = type(n_color);
	if (t != NPY_FLOAT) {
		n_color = astype(n_color, NPY_FLOAT);
	}

	std::vector<npy_intp> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		color[slice( 0, npoints)] = n_color;
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
		float* color_i = findex( color, 0);
		float* color_end = findex( color, count);
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
	throw std::invalid_argument( "color must be an Nx4 array");
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
	color[slice(0, npoints)] = make_tuple( c.red, c.green, c.blue);
}

void
faces::set_normal( const array& n_normal)
{
	normal[slice(0, count)] = n_normal;
}

void
faces::set_normal_l( boost::python::list normals)
{
	set_normal( array(normals));
}

void
faces::set_normal_v( vector v)
{
	using boost::python::make_tuple;
	// Broadcast the new normal across the array.
	int npoints = count ? count : 1;
	normal[slice(0, npoints)] = make_tuple( v.x, v.y, v.z);
}

void
faces::gl_render( const view& scene)
{
	if (degenerate())
		return;

	std::vector<vector> spos;
	std::vector<rgb> tcolor;
	
	gl_enable_client vertexes( GL_VERTEX_ARRAY);
	gl_enable_client normals( GL_NORMAL_ARRAY);
	gl_enable_client colors( GL_COLOR_ARRAY);

	glNormalPointer( GL_DOUBLE, 0, index( normal, 0));

	if (scene.gcf != 1.0 || (scene.gcfvec[0] != scene.gcfvec[1])) {
		std::vector<vector> tmp( count);
		spos.swap( tmp);
		const double* pos_i = index(pos, 0);
		for (std::vector<vector>::iterator i = spos.begin(); i != spos.end(); ++i) {
			*i = vector(pos_i).scale(scene.gcfvec);
			pos_i += 3;
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
			color_i += 3;
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
	double* pos_end = index( pos, count - count%3);
	while (pos_i < pos_end) {
		ret += vector(pos_i);
		pos_i += 3; // 3 doubles per vector point
	}
	if (count)
		ret /= count;
	return ret;
}

void
faces::grow_extent( extent& world)
{
	double* pos_i = index( pos, 0);
	double* pos_end = index( pos, count - count%3);
	while (pos_i < pos_end) {
		world.add_point( vector(pos_i));
		pos_i += 3; // 3 doubles per vector point
	}
	world.add_body();
}

void 
faces::get_material_matrix( const view& v, tmatrix& out ) {
	if (degenerate()) return;
	
	// xxx Add some caching for extent with grow_extent etc; once locking changes so we can trust the primitive not to change during rendering
	vector min_extent, max_extent;
	double* pos_i = index( pos, 0);
	double* pos_end = index( pos, count - count%3);
	min_extent = max_extent = vector( pos_i ); pos_i += 3;
	while (pos_i < pos_end)
		for(int j=0; j<3; j++) {
			if (*pos_i < min_extent[j]) min_extent[j] = *pos_i;
			else if (*pos_i > max_extent[j]) max_extent[j] = *pos_i;
			pos_i++;
		}
	
	out.translate( vector(.5,.5,.5) );
	out.scale( vector(1,1,1) * (.999 / (v.gcf * std::max(max_extent.x-min_extent.x, std::max(max_extent.y-min_extent.y, max_extent.z-min_extent.z)))) );
	out.translate( -.5 * v.gcf * (min_extent + max_extent) );
}

} } // !namespace cvisual::python
