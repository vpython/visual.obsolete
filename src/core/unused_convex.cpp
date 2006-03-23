// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "convex.hpp"

#include <boost/crc.hpp>

namespace cvisual {

convex::jitter_table convex::jitter;

long
convex::checksum() const
{
	boost::crc_32_type engine;
	engine.process_block( &pos.begin()->x, &pos.end()->x);
	return engine.checksum();
}

bool
convex::degenerate() const
{
	return pos.size() < 3;
}

void
convex::recalc()
{
	hull.clear();
	hull.push_back( face( pos[0], pos[1], pos[2]));
	hull.push_back( face( pos[0], pos[2], pos[1]));
	for (size_t i = 3; i < pos.size(); ++i) {
		add_point( i, pos[i]);
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
	: last_checksum(0)
{
}

void
convex::append( vector p)
{
	pos.push_back(p);
}

void
convex::set_color( const rgba& n_color)
{
	color = n_color;
}

void 
convex::gl_render( const view& scene)
{
	if (degenerate())
		return;
	long check = checksum();
	if (check != last_checksum) {
		recalc();
	}

	glShadeModel(GL_FLAT);
	glEnable(GL_CULL_FACE);
	color.gl_set();
	
	glBegin(GL_TRIANGLES);
	for (std::vector<face>::const_iterator f = hull.begin(); f != hull.end(); ++f) {
		f->normal.gl_normal();
		(f->corner[0] * scene.gcf).gl_render();
		(f->corner[1] * scene.gcf).gl_render();
		(f->corner[2] * scene.gcf).gl_render();
	}
	glEnd();
	glDisable(GL_CULL_FACE);
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
}

} // !namespace cvisual
