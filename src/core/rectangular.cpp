#include "rectangular.hpp"

namespace cvisual {

rectangular::rectangular()
	: width(1.0), height(1.0)
{
}

rectangular::rectangular( const rectangular& other)
	: primitive( other), width( other.width), height( other.height)
{
}

rectangular::~rectangular()
{
}

void 
rectangular::set_length( double l)
{
	axis = axis.norm() * l;
}

double 
rectangular::get_length()
{
	return axis.mag();
}
	
void 
rectangular::set_height( double h)
{
	lock L(mtx);
	height = h;
}

double 
rectangular::get_height()
{
	return height;
}
	
void 
rectangular::set_width( double w)
{
	lock L(mtx);
	width = w;
}

double 
rectangular::get_width()
{
	return width;
}
	
vector 
rectangular::get_size()
{
	return vector(axis.mag(), height, width);
}

void 
rectangular::set_size( const vector& s)
{
	axis = axis.norm() * s.x;
	lock L(mtx);
	height = s.y;
	width = s.z;
}

} // !namespace cvisual
