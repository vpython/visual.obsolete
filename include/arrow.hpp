#ifndef VPYTHON_ARROW_HPP
#define VPYTHON_ARROW_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "primitive.hpp"
#include "util/displaylist.hpp"
#include "util/sorted_model.hpp"

#include <boost/scoped_ptr.hpp>

namespace cvisual {

using boost::scoped_ptr;

/** A 3D 4-sided arrow, with adjustable head and shaft.  TODO: revisit the cache
 * strategy for this object now that I have some more experience with the box
 * and pyramid models.
 */
class arrow : public primitive
{
 private:
	/** OpenGL storage for the model, per-object basis.  Since the arrow's 
	 * geometry must be recalculated every time any of the determining factors
	 * change (like fixedwidth, headwidth, and so on), the model is recomputed
	 * by refresh_cache() and stored here.
	 * */
	displaylist model;
	
	/** A sortable geometry for rendering transparent objects.  Some of the
	 * flat sides have been recentered at the center of the rectangle they form
	 * rather than the centroid of the triangle's themselves.
	 */
	scoped_ptr<z_sorted_model<triangle, 22> > sorted_model;
	/** Helper function to recalculate the sorted_model.  The model will need
	 * to be initially sorted after this function call.
	 */
	void recalc_sorted_model( const double& gcf);
	
	/** Renders the sorted_model into the model displaylist. */
	void cache_transparent_model( const view&);
	
	/** True if the width of the point and shaft should not vary with the length
		of the arrow. 
	 */
	bool fixedwidth;
	
	/** If zero, then use automatic scaling for the width's of the parts of the
		arrow.  If nonzero, they specify proportions for the arrow in world 
		space.
	*/
	double headwidth;
	double headlength;
	double shaftwidth;
 
 public:
	/** Default arrow.  Pointing along +x, unit length, 
	 */
	arrow();
	void set_headwidth( double hw);
	double get_headwidth();
	
	void set_headlength( double hl);
	double get_headlength();
	
	void set_shaftwidth( double sw);
	double get_shaftwidth();
	
	void set_fixedwidth( bool fixed);
	bool is_fixedwidth();
	
	void set_length( double l);
	double get_length();
	
 protected:
	/** Renderes the last cached state without checking its validity. */
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	/** The extent of this body is modeled as a pair of points: one at the tip,
	 * another at the base. 
	 */
	virtual void grow_extent( extent&);
	/** If needed, recomputes the entire bodies geometry.
	 */
	virtual void update_cache( const view&);
	/** Resorts the sorted_model without recomputing it.  This is presently the
	 * only object that takes advantage of this funciton.
	 */
	virtual void update_z_sort( const view&);
	/** The center of this body is considered to be halfway between the tip and 
	 * base. 
	 */
	virtual vector get_center() const;
	PRIMITIVE_TYPEINFO_DECL;

	/** Initializes these four variables with the effective geometry for the
		arrow.  The resulting geometry is scaled to view space, but oriented
		and positioned in model space.  The only requred transforms are
		reorientation and translation.
	*/
	void effective_geometry( 
		double& headwidth, double& shaftwidth, double& length, 
		double& headlength, double gcf);
};

} // !namespace cvisual

#endif // !defined VPYTHON_ARROW_HPP
