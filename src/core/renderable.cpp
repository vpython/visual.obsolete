#include "renderable.hpp"
#include <iostream>

renderable::renderable()
	: model_damaged(true), z_damaged(true), visible(true)
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
