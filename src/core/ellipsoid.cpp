// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "ellipsoid.hpp"

namespace cvisual {

ellipsoid::ellipsoid()
	: width( 1.0), height( 1.0)
{
}

void
ellipsoid::set_width( double w)
{
	width = w;
}

void
ellipsoid::set_length( double l)
{
	axis = axis.norm() * l;
}

void
ellipsoid::set_height( double h)
{
	height = h;
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

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(ellipsoid);

} // !namespace cvisual
