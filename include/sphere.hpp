#ifndef VPYTHON_SPHERE_HPP
#define VPYTHON_SPHERE_HPP

#include "simple_displayobject.hpp"
#include "util/displaylist.hpp"
#include "util/texture.hpp"

/** A simple monochrome sphere. 
 */
class sphere : public simple_displayobject
{
 private:
	/// The radius of the sphere.
	double radius;
	/// A texture map to be applied to the sphere.  A Mercator projection is 
	/// performed from the image to the sphere's surface.
	shared_ptr<texture> tex;

	/** The level-of-detail cache.  It is stored for the life of the program, and
		initialized when the first sphere is rendered.  The last six use the
		same detail level as the first six, but also include texture coordinates.
	*/
	static displaylist lod_cache[12];
	/// True until the first sphere is rendered, then false.
	static bool first;
 
 public:
	/** Construct a unit sphere at the origin. */
	sphere();
	virtual ~sphere();
	
	void set_radius(const double& r);
	double get_radius() const;
	void set_texture( shared_ptr<texture>);
	shared_ptr<texture> get_texture();
 
 protected:
	/** Renders a simple sphere with the #2 level of detail.  */
	virtual void gl_pick_render( const view&);
	/** Renders the sphere.  All of the spheres share the same basic set of 
	 * models, and then use matrix transforms to shape and position them.
	 */
	virtual void gl_render( const view&);
	/** Extent reported using extent::add_sphere(). */
	virtual void grow_extent( extent&);
	/** On the first render pass, computes the sphere model geometry and stores
	 * it in the lod_cache.  Otherwise it does nothing. */
	virtual void update_cache( const view&);
	/** Exposed for the benefit of the ellipsoid object, which overrides it. 
	 * The default is to use <radius, radius, radius> for the scale. 
	 */
	virtual vector get_scale();
	/** Returns true if this object should not be drawn.  Conditions are:
	 * zero radius, or visible is false.  (overridden by the ellipsoid class). 
	 */
	virtual bool degenerate();
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};

#endif // !defined VPYTHON_SPHERE_HPP
