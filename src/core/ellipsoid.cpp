#include "ellipsoid.hpp"


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
