#ifndef VPYTHON_SPHERE_HPP
#define VPYTHON_SPHERE_HPP

#include "simple_displayobject.hpp"
#include "util/displaylist.hpp"
#include "util/texture.hpp"

// TODO: implement culling plane logic to cut out the back half of any particular
// sphere when the viewpoint isn't inside of the body.
class sphere : public simple_displayobject
{
 private:
	/// The radius of the sphere.
	double radius;
	displaylist model;
	shared_ptr<texture> tex;

	// The level-of-detail cache.  It is stored for the life of the program, and
	// initialized when the first sphere is rendered.
	static displaylist lod_cache[12];
	static bool first;
 
 public:
	sphere();
	sphere( int lod);
	virtual ~sphere();
	
	void set_radius(const double& r);
	double get_radius() const;
	void set_texture( shared_ptr<texture>);
	shared_ptr<texture> get_texture();
 
 protected:
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual void update_cache( const view&);
	virtual vector get_scale();
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};

#endif // !defined VPYTHON_SPHERE_HPP
