#ifndef VPYTHON_UTIL_EXTENT_HPP
#define VPYTHON_UTIL_EXTENT_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/vector.hpp"
#include "util/tmatrix.hpp"

namespace cvisual {

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
	size_t buffer_depth; ///< The required depth of the selection buffer.
	int frame_depth;
 
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
	
    /** Merge a local extent with this one.
    * @param fwt   The transform from the frame coordinates to the parent's
    *     coordinate space.
    * @param local   The extent of the objects rendered within frame.
    */    
    void merge_local( const tmatrix& fwt, const extent& local);
    
	/** Report the number of bodies that this object represents.  This is used
	 *  for the calculation of the hit buffer size.
	 */
	void add_body();
	/** See implementation of frame::grow_extent()
	 */
	void push_frame();
	void pop_frame();
	/** Returns the size for the select buffer when rendering in select mode,
	 * after having traversed the world. */
	size_t get_select_buffer_depth();

	/** Zeros out the extent to be a single point at the origin. */
	void reset();

	// The following functions represent the interface for render_surface objects.
	/** Returns the center position of the scene in world space. */
	vector center() const;


	/** Returns a distance from the camera that minimizes forward encroachment into the scene. */
	double nearclip( const vector& camera, const vector& forward) const;
	
	/** Returns the distance to the point that is farthest forward from the 
	 * camera. 
	 * @param camera  The location of the camera in world space.
	 * @parm forward  The direction of the camera (should be a unit vector). 
	 */
	double farclip( const vector& camera, const vector& forward) const;
	
	/** Compute the maximum perpenduclar distance from the line defined by
	 * forward and center and the corners of the bounding box. 
	 * @param forward  The direction the camera is facing.
	 * @param center  A point that the camera is pointing towards.
	 * @return a scalar.  If the scene is a point, this returns 1.0. */
	double widest_offset( const vector& forward, const vector& center) const;
	
	/** Determine the magnitude of the size of the world. 
		@return the length along the three-axis diagonal of the bounding box, in
			world space.
	*/
	double scale() const;
	
	/** Determines the range that the axes need to be to include the bounding
	 * box.
	 */
	vector range( vector center) const;
	double uniform_range( vector center) const;
 
	/** Print the state of the extent out to stderr.  Only useful for internal debuggin.
	 */
	void dump_extent() const;
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_EXTENT_HPP
