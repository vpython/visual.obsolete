#include "python/points.hpp"
#include "python/num_util.hpp"
#include "python/slice.hpp"
#include "util/sorted_model.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

#include "wrap_gl.hpp"

#include <vector>
#include <sstream>
#include <algorithm>
#include <set>

namespace cvisual { namespace python {

using boost::python::make_tuple;
using boost::python::object;

namespace {

float* findex( const array& a, size_t i)
{
	return ((float*)data(a)) + (i)*3; // (red,green,blue)
}

double*
index( const array& a, size_t i)
{
	return ((double*)data(a)) + (i) * 3;
}

} // !namespace (anon)

points::points()
	: pos(0), color(0), preallocated_size(256), count(0),
	size_type(SCREEN),
	points_shape(ROUND),
	size( 1.5)
{
	std::vector<npy_intp> dims(2);
	dims[0] = preallocated_size;
	dims[1] = 3;
	pos = makeNum(dims);
	color = makeNum(dims, NPY_FLOAT);

	double* pos_i = index( pos, 0);
	float* color_i = findex( color, 0);

	pos_i[0] = 0;
	pos_i[1] = 0;
	pos_i[2] = 0;
	color_i[0] = 1;
	color_i[1] = 1;
	color_i[2] = 1;
}

points::points( const points& other)
	: renderable( other),
	pos( other.pos),
	color( other.color),
	preallocated_size( other.preallocated_size),
	count( other.count),
	size_type( other.size_type),
	points_shape( other.points_shape),
	size( other.size)
{
}

points::~points()
{
}

void
points::set_length( size_t length)
{
	// The number of points that are valid.
	size_t npoints = count;
	if (npoints > length) // A shrink operation - never done by VPython.
		npoints = length;
	if (npoints == 0)
		// The first allocation.
		npoints = 1;

	if (length > preallocated_size) {
		VPYTHON_NOTE( "Reallocating buffers for a points object.");
		std::vector<npy_intp> dims(2);
		dims[0] = 2*length;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		array n_color = makeNum( dims, NPY_FLOAT);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * (npoints+1));
		std::memcpy( data( n_color), data( color), sizeof(float) * 3 * (npoints+1));
		pos = n_pos;
		color = n_color;
		preallocated_size = dims[0];
	}
	if (length > npoints) {
		// Copy the last good element to the new positions.
		const double* last_pos = index( pos, npoints-1);
		double* pos_i = index( pos, npoints);
		double* pos_end = index( pos, length);
		while (pos_i < pos_end) {
			pos_i[0] = last_pos[0];
			pos_i[1] = last_pos[1];
			pos_i[2] = last_pos[2];
			pos_i += 3;
		}

		const float* last_color = findex( color, npoints-1);
		float* color_i = findex( color, npoints);
		float* color_end = findex( color, length);
		while (color_i < color_end) {
			color_i[0] = last_color[0];
			color_i[1] = last_color[1];
			color_i[2] = last_color[2];
			color_i += 3;
		}
	}
	count = length;
}

void
points::append_rgb( vector npos, float red, float green, float blue)
{
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	if (red != -1)
		last_color[0] = red;
	if (green != -1)
		last_color[1] = green;
	if (blue != -1)
		last_color[2] = blue;
}

void
points::append( vector npos, rgb ncolor)
{
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	last_color[0] = ncolor.red;
	last_color[1] = ncolor.green;
	last_color[2] = ncolor.blue;
}

void
points::append( vector npos)
{
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
}

object
points::get_pos()
{
	return pos[slice(0, (int)count)];
}

object
points::get_color()
{
	return color[slice(0, (int)count)];
}

void
points::set_size( float size)
{
	this->size = size;
}

void
points::set_points_shape( const std::string& n_type)
{
	if (n_type == "round") {
		points_shape = ROUND;
	}
	else if (n_type == "square") {
		points_shape = SQUARE;
	}
	else
		throw std::invalid_argument( "Unrecognized shape type");
}

std::string
points::get_points_shape( void)
{
	switch (points_shape) {
		case ROUND:
			return "round";
		case SQUARE:
			return "square";
		default:
			return "";
	}
}

void
points::set_size_type( const std::string& n_type)
{
	if (n_type == "screen") {
		size_type = SCREEN;
	}
	else if (n_type == "world") {
		size_type = WORLD;
	}
	else
		throw std::invalid_argument( "Unrecognized coordinate type");
}

std::string
points::get_size_type( void)
{
	switch (size_type) {
		case SCREEN:
			return "screen";
		case WORLD:
			return "world";
		default:
			return "";
	}
}

void
points::set_pos( array n_pos)
{

    NPY_TYPES t = type( n_pos);
    if (t != NPY_DOUBLE) {
   		n_pos = astype(n_pos, NPY_DOUBLE);
    }

	std::vector<npy_intp> dims = shape( n_pos);
	if (dims.size() == 1 && !dims[0]) {
		set_length(0);
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
	if (dims[1] == 2) {
		set_length( dims[0]);
		pos[make_tuple(slice(0, count), slice(0,2))] = n_pos;
		pos[make_tuple(slice(0, count), 2)] = 0.0;
		return;
	}
	else if (dims[1] == 3) {
		set_length( dims[0]);
		pos[make_tuple(slice(0, count), slice())] = n_pos;
		return;
	}
	else {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
}

void
points::set_pos_l( const list& pos)
{
	this->set_pos( array(pos));
}

// Interpreted as an initial append operation, with no color specified.
void
points::set_pos_v( const vector& npos)
{
	using namespace boost::python;
	tuple t_pos = make_tuple( make_tuple( npos.x, npos.y, npos.z));
	set_pos( array( t_pos));
}

void
points::set_color( array n_color)
{
	NPY_TYPES t = type(n_color);
	if (t != NPY_FLOAT) {
		n_color = astype(n_color, NPY_FLOAT);
	}

	std::vector<npy_intp> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		color[slice( 0, npoints)] = n_color;
		return;
	}
	if (dims.size() == 2 && dims[1] == 3) {
		// An RGB chunk of color
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		// The following doesn't work; I don't know why. 
		// Note that it works with a single color above.
		//color[slice(1, count+1), slice(0, 3)] = n_color;
		// So instead do it by brute force:
		float* color_i = findex( color, 0);
		float* color_end = findex( color, count);
		float* n_color_i = findex( n_color,0);
		while (color_i < color_end) {
			color_i[0] = n_color_i[0];
			color_i[1] = n_color_i[1];
			color_i[2] = n_color_i[2];
			color_i += 3;
			n_color_i += 3;
		}
		return;
	}
	throw std::invalid_argument( "color must be an Nx4 array");
}

void
points::set_color_l( const list& color)
{
	this->set_color( array( color));
}

void
points::set_color_t( const rgb& color)
{
	this->set_color( array( make_tuple( color.red, color.green, color.blue)));
}


void
points::set_red( const array& red)
{
	set_length( shape( red).at(0));
	color[make_tuple( slice( 0, count), 0)] = red;
}

void
points::set_green( const array& green)
{
	set_length( shape( green).at(0));
	color[make_tuple( slice( 0, count), 1)] = green;
}

void
points::set_blue( const array& blue)
{
	set_length( shape( blue).at(0));
	color[make_tuple( slice( 0, count), 2)] = blue;
}

void
points::set_opacity( const array& opacity)
{
	set_length(shape( opacity).at(0));
	// Needs work....
	//color[make_tuple( slice( 0, count), 3)] = opacity;
}

void
points::set_red_l( const list& red)
{
	this->set_red( array(red));
}

void
points::set_green_l( const list& green)
{
	this->set_green( array(green));
}

void
points::set_blue_l( const list& blue)
{
	this->set_blue( array(blue));
}

void
points::set_opacity_l( const list& opacity)
{
	this->set_opacity( array( opacity));
}

void
points::set_x( const array& x)
{
	set_length( shape(x).at(0));
	pos[make_tuple( slice(1, count+1), 0)] = x;
}

void
points::set_y( const array& y)
{
	set_length( shape(y).at(0));
	pos[make_tuple( slice(1, count+1), 1)] = y;
}

void
points::set_z( const array& z)
{
	set_length( shape(z).at(0));
	pos[make_tuple( slice(1, count+1), 2)] = z;
}

void
points::set_x_l( const list& x)
{
	set_x( array(x));
}

void
points::set_y_l( const list& y)
{
	set_y( array(y));
}

void
points::set_z_l( const list& z)
{
	set_z( array(z));
}


void
points::set_x_d( const double x)
{
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 0)] = x;
}

void
points::set_y_d( const double y)
{
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 1)] = y;
}

void
points::set_z_d( const double z)
{
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 2)] = z;
}


void
points::set_red_d( float red)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 0)] = red;
}

void
points::set_green_d( float green)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 1)] = green;
}

void
points::set_blue_d( float blue)
{
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 2)] = blue;
}

void
points::set_opacity_d( float opacity)
{
	if (count == 0) {
		set_length(1);
	}
	// Needs work.....
	//color[make_tuple(slice(0,count), 3)] = opacity;
}

bool
points::degenerate() const
{
	return count == 0;
}

struct point_coord
{
	vector center;
	mutable rgb color;
	inline point_coord( const vector& p, const rgb& c)
		: center( p), color(c)
	{}
};

void
points::gl_render( const view& scene)
{
	if (degenerate())
		return;

	std::vector<point_coord> translucent_points;
	typedef std::vector<point_coord>::iterator translucent_iterator;

	std::vector<point_coord> opaque_points;
	typedef std::vector<point_coord>::iterator opaque_iterator;

	const double* pos_i = index( pos, 0);
	const double* pos_end = index( pos, count);

	const float* color_i = findex( color, 0);
	const float* color_end = findex( color, count);

	// First classify each point based on whether or not it is translucent
	if (points_shape == ROUND) { // Every point must be depth sorted
		for ( ; pos_i < pos_end && color_i < color_end; pos_i += 3, color_i += 3) {
			translucent_points.push_back( point_coord( vector(pos_i), rgb(color_i)));
		}
	}
	else { // Only translucent points need to be depth-sorted
		for ( ; pos_i < pos_end && color_i < color_end; pos_i += 3, color_i += 3) {
			if (0) // opacity not done
				translucent_points.push_back( point_coord( vector(pos_i), rgb(color_i)));
			else
				opaque_points.push_back( point_coord( vector(pos_i), rgb(color_i)));
		}
	}
	// Now conditionally apply transformations for gcf and anaglyph color
// Needs work
//	if (translucent_points.size())
//		renderable::color.opacity = 0.5;
	if (scene.gcf != 1.0 || (scene.gcfvec[0] != scene.gcfvec[1])) {
		for (opaque_iterator i = opaque_points.begin(); i != opaque_points.end(); ++i) {
			i->center = (i->center).scale(scene.gcfvec);
		}
		for (translucent_iterator i = translucent_points.begin(); i != translucent_points.end(); ++i) {
			i->center = (i->center).scale(scene.gcfvec);
		}
	}
	if (scene.anaglyph) {
		if (scene.coloranaglyph) {
			for (opaque_iterator i = opaque_points.begin(); i != opaque_points.end(); ++i) {
				i->color = i->color.desaturate();
			}
			for (translucent_iterator i = translucent_points.begin(); i != translucent_points.end(); ++i) {
				i->color = i->color.desaturate();
			}
		}
		else {
			for (opaque_iterator i = opaque_points.begin(); i != opaque_points.end(); ++i) {
				i->color = i->color.grayscale();
			}
			for (translucent_iterator i = translucent_points.begin(); i != translucent_points.end(); ++i) {
				i->color = i->color.grayscale();
			}
		}
	}
	// Sort the translucent points
	if (!translucent_points.empty()) {
		std::stable_sort( translucent_points.begin(), translucent_points.end(),
			face_z_comparator(scene.forward));
	}

	clear_gl_error();

	if (points_shape == ROUND)
		glEnable( GL_POINT_SMOOTH);

	if (size_type == WORLD && scene.glext.ARB_point_parameters) {
		// This is simpler and more robust than what was here before, but it's still
		// a little tacky and probably not perfectly general.  I'm not sure that it 
		// should work with stereo frustums, but I can't find a case where it's 
		// obviously wrong.
		// However, note that point attenuation (regardless of parameters) isn't a
		// correct perspective calculation, because it divides by distance, not by Z.
		// Points not at the center of the screen will be too small, particularly
		// at high fields of view.  This is in addition to the implementation limits
		// on point size, which will be a problem when points get too big or close.
		
		tmatrix proj; proj.gl_projection_get();  // Projection matrix
		
		vector p(proj * vertex(.5,0,1,1));  // eye coordinates .5,0,1 -> window coordinates
		
		// At an eye z of 1, a sphere of world-space diameter 1 is p.x * scene.view_width pixels wide,
		// so a sphere of world-space diameter (size*scene.gcf) is
		double point_radius_at_z_1 = size * scene.gcf * p.x * scene.view_width;

		float attenuation_eqn[] =  { 0.0f, 0.0f, 1.0f / (float)(point_radius_at_z_1*point_radius_at_z_1) };
		scene.glext.glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, attenuation_eqn);
		glPointSize( 1 );
	}
	else if (size_type == SCREEN) {
		// Restore to default (aka, disable attenuation)
		if (scene.glext.ARB_point_parameters) {
			float attenuation_eqn[] = {1.0f, 0.0f, 0.0f};
			scene.glext.glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, attenuation_eqn);
		}
		if (points_shape == ROUND) {
			glPointSize( size );
		}
		else {
			glPointSize( size );
		}
	}
	// Finish GL state prep
	gl_disable ltg( GL_LIGHTING);
	gl_enable_client v( GL_VERTEX_ARRAY);
	gl_enable_client c( GL_COLOR_ARRAY);

	// Render opaque points (if any)
	if (opaque_points.size()) {
		const std::ptrdiff_t chunk = 256;
		opaque_iterator begin = opaque_points.begin();
		opaque_iterator end = opaque_points.end();
		while (begin < end) {
			std::ptrdiff_t block = std::min( chunk, end - begin);
			glColorPointer( 3, GL_FLOAT, sizeof(point_coord), &begin->color.red);
			glVertexPointer( 3, GL_DOUBLE, sizeof(point_coord), &begin->center.x);
			glDrawArrays( GL_POINTS, 0, block);
			begin += block;
		}
	}

	// Render translucent points (if any)
	if (!translucent_points.empty()) {
		gl_enable blend( GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const std::ptrdiff_t chunk = 256;
		translucent_iterator begin = translucent_points.begin();
		translucent_iterator end = translucent_points.end();
		while (begin < end) {
			std::ptrdiff_t block = std::min( chunk, end - begin);
			glColorPointer( 3, GL_FLOAT, sizeof(point_coord), &begin->color.red);
			glVertexPointer( 3, GL_DOUBLE, sizeof(point_coord), &begin->center.x);
			glDrawArrays( GL_POINTS, 0, block);
			begin += block;
		}
	}

	if (!(points_shape == ROUND)) {
		glDisable( GL_POINT_SMOOTH);
	}
	check_gl_error();
}

vector
points::get_center() const
{
	if (degenerate())
		return vector();
	vector ret;
	const double* pos_i = index( pos, 0);
	const double* pos_end = index( pos, count);
	const float* color_i = findex( color, 0);
	const float* color_end = findex( color, count);
	for ( ;pos_i < pos_end && color_i < color_end; pos_i += 3, color_i += 3) {
		if (points_shape == ROUND) // || opacity)
			ret += vector(pos_i);
	}
	ret /= count;
	return ret;
}

void
points::gl_pick_render( const view& scene)
{
	gl_render( scene);
}

void
points::grow_extent( extent& world)
{
	if (degenerate())
		return;
	const double* pos_i = index(pos, 0);
	const double* pos_end = index( pos, count);
	if (size_type == SCREEN)
		for ( ; pos_i < pos_end; pos_i += 3)
			world.add_point( vector(pos_i));
	else
		for ( ; pos_i < pos_end; pos_i += 3)
			world.add_sphere( vector(pos_i), size);
	world.add_body();
}

} } // !namespace cvisual::python
