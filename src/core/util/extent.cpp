// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/extent.hpp"
#include <algorithm>
#include <iostream>

#ifdef __GNUC__
#define EXPECT(boolean_expression, boolean_constant) \
	__builtin_expect( boolean_expression, boolean_constant)
#else
#define EXPECT(boolean_expression, boolean_constant) \
	boolean_expression
#endif	

namespace cvisual {	

extent::extent()
	: first(true), buffer_depth(0), frame_depth(0)
{
}


void
extent::reset()
{
	first = true;
	mins = vector();
	maxs = vector();
	buffer_depth = 0;
	frame_depth = 0;
}

void
extent::add_point( vector point)
{	
	if (EXPECT(first, false)) {
		first = false;
		mins = point;
		maxs = point;
	}
	else {
		mins.x = std::min( point.x, mins.x);
		maxs.x = std::max( point.x, maxs.x);
		mins.y = std::min( point.y, mins.y);
		maxs.y = std::max( point.y, maxs.y);
		mins.z = std::min( point.z, mins.z);
		maxs.z = std::max( point.z, maxs.z);
	}
}

void 
extent::add_sphere( vector center, double radius)
{
	if (radius < 0.0)
		radius = -radius;
	
	if (EXPECT(first, false)) {
		first = false;
		mins.x = center.x - radius;
		maxs.x = center.x + radius;
		mins.y = center.y - radius;
		maxs.y = center.y + radius;
		mins.z = center.z - radius;
		maxs.z = center.z + radius;
		
	}
	else {
		mins.x = std::min( center.x - radius, mins.x);
		maxs.x = std::max( center.x + radius, maxs.x);
		mins.y = std::min( center.y - radius, mins.y);
		maxs.y = std::max( center.y + radius, maxs.y);
		mins.z = std::min( center.z - radius, mins.z);
		maxs.z = std::max( center.z + radius, maxs.z);
	}	
}

void
extent::merge_local( const tmatrix& fwt, const extent& local)
{
    if (local.first)
        return;
    
    // Really should get extent of box, not approximate sphere
    vector min_corner = fwt*local.mins;
    vector max_corner = fwt*local.maxs;
    vector local_center = (min_corner + max_corner) * 0.5;
    double radius = 0.5*(min_corner - max_corner).mag();
    add_sphere( local_center, radius);
}

vector
extent::center() const
{
	if (first)
		return vector();
	
	return (mins + maxs) * 0.5;
}

void
extent::near_and_far( const vector& forward, double& nearest, double& farthest ) const
{
	if (mins == maxs) {
		// The only way that this should happen is if the scene is empty.
		nearest = 1.0;
		farthest = 10.0;
	} 

    double corners[] = {
       maxs.dot(forward), // front upper right
       vector( mins.x, mins.y, maxs.z).dot(forward), // front lower left
       vector( mins.x, maxs.y, maxs.z).dot(forward), // front upper left
       vector( maxs.x, mins.y, maxs.z).dot(forward), // front lower right
       vector( mins.x, maxs.y, mins.z).dot(forward), // back upper left
       vector( maxs.x, mins.y, mins.z).dot(forward), // back lower right
       vector( maxs.x, maxs.y, mins.z).dot(forward) // back upper right
    };
    nearest = farthest = mins.dot(forward); // back lower left
    for (size_t i = 0; i < 7; ++i) {
		if (corners[i] < nearest) {
        	nearest = corners[i];
        	//std::cerr << "nearest=" << nearest << std::endl;
        }
		if (corners[i] > farthest) {
			farthest = corners[i];
        	//std::cerr << "farthest=" << farthest << std::endl;
		}
	//nearest = center+nearest.dot(forward)*forward;
	//farthest = center+farthest.dot(forward)*forward;
	//std::cerr << "near/far=" << nearest << farthest << std::endl;
	}
}

double
extent::widest_offset( const vector& forward, const vector& center) const
{
	double farthest_dist = 0.0;
	
	// Find the corner of the extent that is the farthest away from the
	// line passing through center, in the forward direction.
	vector corners[] = {
		mins,
		maxs,
		vector( mins.x, mins.y, maxs.z),
		vector( mins.x, maxs.y, mins.z),
		vector( mins.x, maxs.y, maxs.z),
		vector( maxs.x, mins.y, mins.z),
		vector( maxs.x, maxs.y, mins.z),
		vector( maxs.x, mins.y, maxs.z)		
	};
	for (size_t i = 0; i < 8; ++i) {
		// The closest point which lies on the line of sight to the corner in
		// question.
		vector closest_point_on_los = 
			(corners[i] - center).dot( forward)*forward + center;
		// The distance from that point to the corner.
		double dist = (corners[i] - closest_point_on_los).mag();
		if (dist > farthest_dist) {
			farthest_dist = dist;
		}
	}
	return farthest_dist;
}

vector 
extent::range( vector center) const
{
    if (first)
        return vector(0,0,0);
 
	return vector(
		std::max( fabs( center.x - mins.x), fabs( center.x - maxs.x)),
		std::max( fabs( center.y - mins.y), fabs( center.y - maxs.y)),
		std::max( fabs( center.z - mins.z), fabs( center.z - maxs.z)));
}

double 
extent::uniform_range( vector center) const
{
    if (first)
        return 0.0;
	double ret = std::max( fabs(center.x - mins.x), fabs(center.x - maxs.x));
	ret = std::max( fabs( center.y - mins.y), ret);
	ret = std::max( fabs( center.y - maxs.y), ret);
	ret = std::max( fabs( center.z - mins.z), ret);
	ret = std::max( fabs( center.z - maxs.z), ret);
	return ret;
}

void 
extent::add_body()
{
	buffer_depth += 4 + frame_depth;
}
	
void 
extent::push_frame()
{
	frame_depth++;
}

void 
extent::pop_frame()
{
	frame_depth--;
	assert( frame_depth >= 0);
}

size_t 
extent::get_select_buffer_depth()
{
	return buffer_depth;
}

void
extent::dump_extent() const
{
	std::cerr << " extent.mins: " << mins << " extent.maxs: " << maxs << std::endl;
}

} // !namespace cvisual
