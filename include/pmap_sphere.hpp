#ifndef VPYTHON_PMAP_SPHERE_HPP
#define VPYTHON_PMAP_SPHERE_HPP

// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "simple_displayobject.hpp"
#include "util/displaylist.hpp"
#include "util/texture.hpp"

namespace cvisual {

// TODO: Fold this special case of texture coordinates back into the sphere
// object.
class pmap_sphere : public simple_displayobject
{
 private:
	/// The radius of the sphere.
	double radius;
	displaylist model;
	shared_ptr<texture> tex;

	// The level-of-detail cache.  It is stored for the life of the program, and
	// initialized when the first sphere is rendered.
	static displaylist lod_cache[2];
	static bool first;
 
 public:
	pmap_sphere();
	pmap_sphere( int lod);
	virtual ~pmap_sphere();
	
	void set_radius(const double& r);
	double get_radius() const;
	void set_texture( shared_ptr<texture>);
	shared_ptr<texture> get_texture();
 
 protected:
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual void update_cache( const view&);
	virtual vector get_scale();
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};

} // !namespace cvisual

#endif // !defined VPYTHON_PMAP_SPHERE_HPP
