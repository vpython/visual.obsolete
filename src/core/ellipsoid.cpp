// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "ellipsoid.hpp"

namespace cvisual {

ellipsoid::ellipsoid()
	: height(1.0), width(1.0)
{
}

ellipsoid::ellipsoid( const ellipsoid& other)
	: sphere( other), height(other.height), width( other.width)
{
}

ellipsoid::~ellipsoid()
{
}

void 
ellipsoid::set_length( double l)
{
	axis = axis.norm() * l;
}

double 
ellipsoid::get_length()
{
	return axis.mag();
}
	
void 
ellipsoid::set_height( double h)
{
	lock L(mtx);
	height = h;
}

double 
ellipsoid::get_height()
{
	return height;
}
	
void 
ellipsoid::set_width( double w)
{
	lock L(mtx);
	width = w;
}

double 
ellipsoid::get_width()
{
	return width;
}
	
vector 
ellipsoid::get_size()
{
	return vector(axis.mag(), height, width);
}

void 
ellipsoid::set_size( const vector& s)
{
	axis = axis.norm() * s.x;
	lock L(mtx);
	height = s.y;
	width = s.z;
}

vector
ellipsoid::get_scale()
{
	return vector( axis.mag(), height, width)*0.5;
}

bool
ellipsoid::degenerate()
{
	return !visible || height == 0.0 || width == 0.0 || axis.mag() == 0.0;
}

void
ellipsoid::grow_extent( extent& world)
{
	if (degenerate())
		return;
	world.add_sphere( pos, std::max( width, std::max( height, axis.mag())));
	world.add_body();
}

PRIMITIVE_TYPEINFO_IMPL(ellipsoid)

} // !namespace cvisual
