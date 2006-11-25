// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms. 
// See the file authors.txt for a complete list of contributors.

#include "python/convex.hpp"
#include "python/slice.hpp"
#include "util/gl_enable.hpp"
#include "util/errors.hpp"

#include <boost/python/extract.hpp>
#include <boost/crc.hpp>

namespace cvisual { namespace python {

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

} // !namespace (unnamed)


convex::jitter_table convex::jitter;

long
convex::checksum() const
{
	boost::crc_32_type engine;
	engine.process_block( index( pos, 0), index( pos, count));
	return engine.checksum();
}

bool
convex::degenerate() const
{
	return count < 3;
}

void
convex::recalc()
{
	hull.clear();

	const double* pos_i = index( pos, 0);
	// A face from the first, second, and third vectors.
	hull.push_back( face( vector(pos_i), vector(pos_i+3), vector(pos_i+3*2)));
	// The reverse face from the first, third, and second vectors.
	hull.push_back( face( vector(pos_i), vector(pos_i+3*2), vector(pos_i+3)));
	// The remainder of the possible faces.
	for (size_t i = 3; i < count; ++i) {
		add_point( i, vector(pos_i + i*3));
	}

	last_checksum = checksum();
}

void
convex::add_point( size_t n, vector pv)
{
	double m = pv.mag();
	pv.x += m * jitter.v[(n  ) & jitter.mask];
	pv.y += m * jitter.v[(n+1) & jitter.mask];
	pv.z += m * jitter.v[(n+2) & jitter.mask];

	std::vector<edge> hole;
	for (size_t f=0; f<hull.size(); ) {
		if ( hull[f].visible_from(pv) ) {
			// hull[f] is visible from pv.  We will never get here if pv is
			//   inside the hull.

			// add the edges to the hole.  If an edge is already in the hole,
			//   it is not on the boundary of the hole and is removed.
			for(int e=0; e<3; ++e) {
				edge E( hull[f].corner[e], hull[f].corner[(e+1)%3] );

				bool boundary = true;
				for(std::vector<edge>::iterator h = hole.begin(); h != hole.end(); ++h) {
					if (*h == E) {
						*h = hole.back();
						hole.pop_back();
						boundary = false;
						break;
					}
				}

				if (boundary) {
					hole.push_back(E);
				}
			}

			// remove hull[f]
			hull[f] = hull.back();
			hull.pop_back();
		}
		else
			f++;
	}

	// Now add the boundary of the hole to the hull.  If pv was inside
	//   the hull, the hole will be empty and nothing happens here.
	for (std::vector<edge>::const_iterator h = hole.begin(); h != hole.end(); ++h) {
		hull.push_back(face(h->v[0], h->v[1], pv));
	}
}

convex::convex()
	: pos(0), preallocated_size(256), count(0), last_checksum(0)
{
	std::vector<npy_intp> dims(2);
	dims[0] = 256;
	dims[1] = 3;
	this->pos = makeNum( dims);
	double* i = index(pos, 0);
	i[2] = i[1] = i[0] = 0.0;
}

convex::convex( const convex& other)
	: renderable( other), pos( other.pos),
	preallocated_size( other.preallocated_size), count( other.count),
	last_checksum(0)
{
}

convex::~convex()
{
}

boost::python::object
convex::get_pos()
{
	return pos[slice(0, count)];
}

void
convex::set_pos( array n_pos)
{
	using namespace boost::python;
	using python::slice;

	NPY_TYPES t = type( n_pos);
	if (t != NPY_DOUBLE) {
		n_pos = astype(n_pos, NPY_DOUBLE);
	}
	std::vector<npy_intp> dims = shape( n_pos);
	if (dims.size() == 1 && count == 0) {
		// perform a single append
		lock L(mtx);
		set_length( 1);
		double* pos_data = index(pos, 0);
		pos_data[0] = extract<double>(n_pos[0]);
		pos_data[1] = extract<double>(n_pos[1]);
		pos_data[2] = extract<double>(n_pos[2]);
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 or Nx2 array");
	}
	if (dims[1] == 2) {
		lock L(mtx);
		set_length(dims[0]);
		pos[make_tuple(slice(0, count), slice(_,1))] = n_pos;
	}
	else if (dims[1] == 3) {
		lock L(mtx);
		set_length(dims[0]);
		pos[slice(0, count)] = n_pos;

	}
	else
		throw std::invalid_argument( "pos must be an Nx3 or Nx2 array");
}

void
convex::set_pos_l( const boost::python::list& pos)
{
	this->set_pos( array(pos));
}

void
convex::set_color( const rgba& n_color)
{
	lock L(mtx);
	color = n_color;
}

rgba
convex::get_color()
{
	return color;
}

void
convex::set_length( size_t length)
{
	size_t npoints = count;
	if (npoints > length) // A shrink operation - never done by VPython.
		npoints = length;
	if (npoints == 0)
		npoints = 1;

	if (length > preallocated_size) {
		VPYTHON_NOTE( "Reallocating buffers for a convex object");
		std::vector<npy_intp> dims(2);
		dims[0] = 2 * length;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * npoints);
		pos = n_pos;
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
	}
	count = length;
}

void
convex::append( vector nv_pos)
{
	lock L(mtx);
	set_length( count + 1);

	double* pos_data = index(pos, count - 1);
	pos_data[0] = nv_pos.get_x();
	pos_data[1] = nv_pos.get_y();
	pos_data[2] = nv_pos.get_z();
}

void
convex::gl_render( const view& scene)
{
	if (degenerate())
		return;
	long check = checksum();
	if (check != last_checksum) {
		recalc();
		last_checksum = check;
	}

	glShadeModel(GL_FLAT);
	gl_enable cull_face( GL_CULL_FACE);
	color.gl_set();

	glBegin(GL_TRIANGLES);
	for (std::vector<face>::const_iterator f = hull.begin(); f != hull.end(); ++f) {
		f->normal.gl_normal();
		(f->corner[0] * scene.gcf).gl_render();
		(f->corner[1] * scene.gcf).gl_render();
		(f->corner[2] * scene.gcf).gl_render();
	}
	glEnd();
	glShadeModel( GL_SMOOTH);
}

vector
convex::get_center() const
{
	if (degenerate())
		return vector();

	vector ret;
	for (std::vector<face>::const_iterator f = hull.begin(); f != hull.end(); ++f) {
		ret += f->center;
	}
	ret /= hull.empty() ? 1 : hull.size();

	return ret;
}

void
convex::gl_pick_render( const view& scene)
{
	gl_render( scene);
}

void
convex::grow_extent( extent& world)
{
	if (degenerate())
		return;

	long check = checksum();
	if (check != last_checksum) {
		recalc();
	}
	assert( hull.size() != 0);

	for (std::vector<face>::const_iterator f = hull.begin(); f != hull.end(); ++f) {
		world.add_point( f->corner[0]);
		world.add_point( f->corner[1]);
		world.add_point( f->corner[2]);
	}
	world.add_body();
}

} } // !namespace cvisual::python
