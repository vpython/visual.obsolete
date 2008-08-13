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
	height = s.y;
	width = s.z;
}

void
rectangular::apply_transform( const view& scene )
{
	double gcf = scene.gcf;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	// OpenGL needs to invert the modelview matrix to generate the normal matrix,
	//   so try not to make it singular:
	double min_scale = std::max( axis.mag(), std::max(height,width) ) * 1e-6;
	glScaled( gcf * std::max(min_scale,axis.mag()), 
			  gcf * std::max(min_scale,height), 
			  gcf * std::max(min_scale,width) );
}

} // !namespace cvisual
