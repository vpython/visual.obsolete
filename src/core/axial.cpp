#include "axial.hpp"

namespace cvisual {
	
axial::axial()
	: radius(1.0)
{
}

axial::axial( const axial& other)
	: primitive( other), radius( other.radius)
{
}

axial::~axial()
{
}

void
axial::set_radius( double r)
{
	lock L(mtx);
	model_damage();
	radius = r;
}

double
axial::get_radius()
{
	return radius;
}
	
} // !namespace cvisual
