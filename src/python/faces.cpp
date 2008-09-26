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

faces::faces()
{
	double* k = normal.data();
	k[0] = k[1] = k[2] = 0.0;
}

void faces::set_length(size_t new_len) {
	arrayprim_color::set_length(new_len);
	normal.set_length(new_len);
}

void
faces::append_rgb( const vector& nv_pos, const vector& nv_normal, float red, float green, float blue)
{
	arrayprim_color::append_rgb( nv_pos, red, green, blue );
	double* n = normal.data(count-1);
	n[0] = nv_normal.x;
	n[1] = nv_normal.y;
	n[2] = nv_normal.z;
}

void
faces::append( const vector& nv_pos, const vector& nv_normal, const rgb& nv_color)
{
	arrayprim_color::append( nv_pos, nv_color );
	double* n = normal.data(count-1);
	n[0] = nv_normal.x;
	n[1] = nv_normal.y;
	n[2] = nv_normal.z;
}

void
faces::append( const vector& nv_pos, const vector& nv_normal)
{
	arrayprim_color::append( nv_pos );
	double* n = normal.data(count-1);
	n[0] = nv_normal.x;
	n[1] = nv_normal.y;
	n[2] = nv_normal.z;
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

	const double* pos_i = pos.data();
	double* norm_i = normal.data();
	const double* pos_end = pos.end();
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

	pos_i = pos.data();
	norm_i = normal.data();
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
boost::python::object faces::get_normal() {
	return normal[all()];
}

void faces::set_normal( const double_array& n_normal)
{
	std::vector<npy_intp> dims = shape(n_normal);
	if (dims.size() == 2 && dims[1] == 3)
		set_length( dims[0] );

	normal[slice(0, count)] = n_normal;
}

void faces::set_normal_v( vector v)
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

	glNormalPointer( GL_DOUBLE, 0, normal.data() );

	if (scene.gcf != 1.0 || (scene.gcfvec[0] != scene.gcfvec[1])) {
		std::vector<vector> tmp( count);
		spos.swap( tmp);
		const double* pos_i = pos.data();
		for (std::vector<vector>::iterator i = spos.begin(); i != spos.end(); ++i) {
			*i = vector(pos_i).scale(scene.gcfvec);
			pos_i += 3;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, &*spos.begin());
	}
	else
		glVertexPointer( 3, GL_DOUBLE, 0, pos.data() );

	if (scene.anaglyph) {
		std::vector<rgb> tmp( count);
		tcolor.swap( tmp);
		const float* color_i = color.data();
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
		glColorPointer( 3, GL_FLOAT, 0, color.data() );

	gl_enable cull_face( GL_CULL_FACE);
	for (size_t drawn = 0; drawn < count - count%3; drawn += 54) {
		glDrawArrays( GL_TRIANGLES, drawn,
			std::min( count - count%3 - drawn, (size_t)54));
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
	const double* pos_i = pos.data();
	const double* pos_end = pos.data( count - count%3 );
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
	const double* pos_i = pos.data();
	const double* pos_end = pos.data( count - count%3 );
	while (pos_i < pos_end) {
		world.add_point( vector(pos_i));
		pos_i += 3; // 3 doubles per vector point
	}
	world.add_body();
}

void
faces::get_material_matrix( const view& v, tmatrix& out ) {
	if (degenerate()) return;

	// TODO: Add some caching for extent with grow_extent etc
	vector min_extent, max_extent;
	const double* pos_i = pos.data();
	const double* pos_end = pos.data( count - count%3 );
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
