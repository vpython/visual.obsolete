#ifndef VPYTHON_UTIL_EXTENT_HPP
#define VPYTHON_UTIL_EXTENT_HPP

#include "util/vector.hpp"
#include "util/tmatrix.hpp"

class extent
{
 private:
	// Defines a minimum bounding box for the scene in camera coordinates.
	vector maxs; //< The lower left rear corner or the bounding box
	vector mins; //< The upper right forward corner of the bounding box
	bool first;  //< True if the first element in the box has not been set.
 
 public:
	extent();
 
	// The following functions represent the interface for renderable objects.
	// Extend the range to include this point.
	void add_point( vector point);
	// Extend the range to include this sphere.
	void add_sphere( vector center, double radius);

	// Resets the extent, so that it may be reevaluated.
	void reset();

	// The following functions represent the interface for render_surface objects.
	// Returns the center position of the scene in camera space.
	vector center() const;
	
	// Adjust the bounds of the scene to accomodate a new centerpoint.
	void recenter();
	
	// Determine the magnitude of the size of the world.
	double scale() const;
};

#endif // !defined VPYTHON_UTIL_EXTENT_HPP
