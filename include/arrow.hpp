#ifndef VPYTHON_ARROW_HPP
#define VPYTHON_ARROW_HPP

#include "simple_displayobject.hpp"
#include "util/displaylist.hpp"
#include "util/sorted_model.hpp"
#include <boost/scoped_ptr.hpp>

using boost::scoped_ptr;

class arrow : public simple_displayobject
{
 private:
	displaylist model;
	scoped_ptr<z_sorted_model<triangle, 22> > sorted_model;
	void recalc_sorted_model( const double& gcf);
	void cache_transparent_model( const view&);
	// True if the width of the point and shaft should not vary with the length
	// of the arrow.
	bool fixedwidth;
	// If zero, then use automatic scaling for the width's of the parts of the
	// arrow.  If nonzero, they specify a size for the arrow in world space.
	double headwidth;
	double headlength;
	double shaftwidth;
 
 public:
	arrow();
	void set_headwidth( double hw);
	void set_headlength( double hl);
	void set_shaftwidth( double sw);
	void set_fixedwidth( bool fixed);
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual void update_cache( const view&);
	virtual void update_z_sort( const view&);
	virtual vector get_center() const;
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;

	// initializes these four variables with the effective geometry for the
	// arrow.  The resulting geometry is scaled to view space, but oriented
	// and positioned in model space.  Therefore, get_scale is left to the default
	// <1,1,1>, and the resulting tmatrix is only reorientation and translation.
	void effective_geometry( 
		double& headwidth, double& shaftwidth, double& length, 
		double& headlength, double gcf);
};

#endif // !defined VPYTHON_ARROW_HPP
