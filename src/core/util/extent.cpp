// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/extent.hpp"
#include <algorithm>

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

vector
extent::center() const
{
	if (first)
		return vector();
	
	return (mins + maxs) * 0.5;
}


void
extent::recenter()
{
	if (first)
		return;
	mins += center();
	maxs += center();
}

double
extent::scale() const
{
	if (first)
		return 0;
	return (maxs - mins).mag();
	// return (maxs - mins).stable_mag();
}

double
extent::scale( const vector& s) const
{
	if (first)
		return 0;
	return ((maxs - mins) * s).mag();
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

} // !namespace cvisual
