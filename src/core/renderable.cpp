// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"

namespace cvisual {

double
view::pixel_coverage( const vector& pos, double radius) const
{
	// The distance from the camera to this position, in the direction of the
	// camera.  This is the distance to the viewing plane that the coverage 
	// circle lies in.
	double dist = (pos - camera).dot(forward);
	// Half of the width of the viewing plane at this distance.
	double apparent_hwidth = tan_hfov_x * dist;
	// The fraction of the apparent width covered by the coverage circle.
	double coverage_fraction = radius / apparent_hwidth;
	// Convert from fraction to pixels.
	return coverage_fraction * window_width;
	
}

renderable::renderable()
	: model_damaged(true), z_damaged(true), visible(true), lit(true)
{
}

renderable::renderable( const renderable& other)
	: model_damaged(true), z_damaged(true), visible(true), lit(other.lit)
{
}

renderable::~renderable()
{
}

void
renderable::gl_render( const view&)
{
	return;
}

void
renderable::gl_pick_render( const view&)
{
}

void
renderable::grow_extent( extent&)
{
	return;
}

// A function that must be overridden if an object wants to cache its state
// for rendering optimization.
void 
renderable::update_cache(const view&)
{
	return;
}

// By default, z-sorting is ignored.
void
renderable::update_z_sort( const view&)
{
	return;
}

void 
renderable::refresh_cache(const view& geometry)
{
	if (color.alpha != 1.0 && (z_damaged || geometry.forward_changed)) {
		if (model_damaged || geometry.gcf_changed)
			update_cache( geometry);
		else
			update_z_sort( geometry);
		model_damaged = false;
		z_damaged = false;
	}
	else if (model_damaged || geometry.gcf_changed) {
		update_cache( geometry);
		model_damaged = false;
	}
}

} // !namespace cvisual
