#ifndef VPYTHON_UTIL_EXTENT_HPP
#define VPYTHON_UTIL_EXTENT_HPP

#include "util/vector.hpp"
#include "util/tmatrix.hpp"

/** A helper class to determine the extent of the rendered universe in world 
	space. 
	*/
class extent
{
 private:
	// Defines a minimum bounding box for the scene in camera coordinates.
	vector maxs; ///< The lower left rear corner or the bounding box
	vector mins; ///< The upper right forward corner of the bounding box
	bool first;  ///< True if the first element in the box has not been set.
 
 public:
	/** Construct a new extent object.  Defaults to a point at (0,0,0). */
	extent();
 
	// The following functions represent the interface for renderable objects.
	/** Extend the range to include this point. 
 		@param point a point in world space coordinates. 
 	*/
	void add_point( vector point);
	/** Extend the range to include this sphere.
 		@param center The center of the sphere.
 		@param radius The radius of the bounding sphere.
 	*/
	void add_sphere( vector center, double radius);

	/** Zeros out the extent to be a single point at the origin. */
	void reset();

	// The following functions represent the interface for render_surface objects.
	/** Returns the center position of the scene in world space. */
	vector center() const;
	
	// TODO: WTF is this still for??
	// Adjust the bounds of the scene to accomodate a new centerpoint.
	void recenter();
	
	/** Determine the magnitude of the size of the world. 
		@return the length along the three-axis diagonal of the bounding box, in
			world space.
	*/
	double scale() const;
 
	/** Determine the magnitude of a scaled subworld.  See also 
		frame::grow_extent() 
	*/
	double scale( const vector& scale) const;
};

#endif // !defined VPYTHON_UTIL_EXTENT_HPP
