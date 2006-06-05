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
float* 
findex( const array& a, size_t i)
{
	return ((float*)data(a)) + (i) * 4;
}

double* 
index( const array& a, size_t i)
{
	return ((double*)data(a)) + (i) * 3;
}

double gl_aliased_radius_range[2] = {-1, -1};
double gl_smooth_radius_range[2] = {-1, -1};
int world_scale_points_supported = -1;

#if defined( _WIN32) || defined (_MSC_VER)
void default_glPointParameterfvARB( GLenum, const GLfloat*)
{
	return;
}
PFNGLPOINTPARAMETERFVARBPROC glPointParameterfvARB = 
	(PFNGLPOINTPARAMETERFVARBPROC)default_glPointParameterfvARB;
#endif

void 
init_pointparam_extension(void)
{
	// Determine if GLPointParameters is supported
	using namespace std;
	set<string> extensions;
	istringstream strm( string( (const char*)(glGetString( GL_EXTENSIONS))));
	copy( istream_iterator<string>(strm), istream_iterator<string>(), 
		inserter( extensions, extensions.begin()));
	if (extensions.find("GL_ARB_point_parameters") != extensions.end()) {
		VPYTHON_NOTE( "Using GL_ARB_point_parameters extension");
		world_scale_points_supported = 1;
		#if defined( _WIN32) || defined (_MSC_VER)
		glPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)wglGetProcAddress( "glPointParameterfvARB");
		#endif
	}
	else
		world_scale_points_supported = 0;
	
	glGetDoublev( GL_ALIASED_POINT_SIZE_RANGE, gl_aliased_radius_range);
	glGetDoublev( GL_SMOOTH_POINT_SIZE_RANGE, gl_smooth_radius_range);
}

} // !namespace (anon)


points::points()
	: pos(0), color(0), preallocated_size(256), count(0), 
	size_type(SCREEN), 
	antialias(true),
	size( 1.5)
{
	std::vector<int> dims(2);
	dims[0] = preallocated_size;
	dims[1] = 3;
	pos = makeNum(dims);
	dims[1] = 4;
	color = makeNum(dims, float_t);
	
	double* pos_i = index( pos, 0);
	float* color_i = findex( color, 0);
	
	pos_i[0] = 0;
	pos_i[1] = 0;
	pos_i[2] = 0;
	color_i[0] = 1;
	color_i[1] = 1;
	color_i[2] = 1;
	color_i[3] = 1;
	
	gl_aliased_radius_range[0] = -1;
	gl_aliased_radius_range[1] = -1;
	gl_smooth_radius_range[0] = -1;
	gl_smooth_radius_range[1] = -1;
}

points::points( const points& other)
	: renderable( other),
	pos( other.pos),
	color( other.color),
	antialias( other.antialias),
	size( other.size), size_type( other.size_type),
	preallocated_size( other.preallocated_size),
	count( other.count)
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
		std::vector<int> dims(2);
		dims[0] = 2*length;
		dims[1] = 3;
		array n_pos = makeNum( dims);
		dims[1] = 4;
		array n_color = makeNum( dims, float_t);
		std::memcpy( data( n_pos), data( pos), sizeof(double) * 3 * (npoints+1));
		std::memcpy( data( n_color), data( color), sizeof(float) * 4 * (npoints+1));
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
			color_i[3] = last_color[3];
			color_i += 4;
		}
	}
	count = length;
}

void
points::append_rgba( vector npos, float red, float blue, float green, float alpha) 
{
	lock L(mtx);
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
	if (alpha != -1)
		last_color[3] = alpha;
}

void
points::append( vector npos)
{
	lock L(mtx);
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;	
}

void
points::append( vector npos, rgba ncolor)
{
	lock L(mtx);
	set_length( count+1);
	double* last_pos = index( pos, count-1);
	float* last_color = findex( color, count-1);
	last_pos[0] = npos.x;
	last_pos[1] = npos.y;
	last_pos[2] = npos.z;
	last_color[0] = ncolor.red;
	last_color[1] = ncolor.green;
	last_color[2] = ncolor.blue;
	last_color[3] = ncolor.alpha;
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
points::set_antialias( bool aa)
{
	lock L(mtx);
	this->antialias = aa;
}

void
points::set_size( double size)
{
	lock L(mtx);
	this->size = size;
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
	}
}


void
points::set_pos( array n_pos)
{
	python::array_types t = type( n_pos);
	if (t != double_t) {
		n_pos = astype( n_pos, double_t);
	}
	std::vector<int> dims = shape( n_pos);
	if (dims.size() == 1 && !dims[0]) {
		lock L(mtx);
		set_length(0);
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "pos must be an Nx3 array");
	}
	if (dims[1] == 2) {
		lock L(mtx);
		set_length( dims[0]);
		pos[make_tuple(slice(0, count), slice(0,2))] = n_pos;
		pos[make_tuple(slice(0, count), 2)] = 0.0;
		return;
	}
	else if (dims[1] == 3) {
		lock L(mtx);
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
	python::array_types t = type(n_color);
	if (t != float_t) {
		n_color = astype( n_color, float_t);
	}
	std::vector<int> dims = shape( n_color);
	if (dims.size() == 1 && dims[0] == 3) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		lock L(mtx);
		color[slice( 0,npoints), slice(0, 3)] = n_color;
		return;
	}
	if (dims.size() == 1 && dims[0] == 4) {
		// A single color, broadcast across the entire (used) array.
		int npoints = (count) ? count : 1;
		lock L(mtx);
		color[slice( 0,npoints)] = n_color;
		return;
	}
	if (dims.size() == 2 && dims[1] == 3) {
		// An RGBA chunk of color
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		lock L(mtx);
		color[slice( 0, count), slice(0, 3)] = n_color;
		return;
	}
	if (dims.size() == 2 && dims[1] == 4) {
		// An RGBA chunk of color
		if (dims[0] != (long)count) {
			throw std::invalid_argument( "color must be the same length as pos.");
		}
		lock L(mtx);
		color[slice( 0, count)] = n_color;
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
points::set_color_t( const rgba& color)
{
	this->set_color( array( make_tuple( color.red, color.green, color.blue, color.alpha)));
}


void
points::set_red( const array& red)
{
	lock L(mtx);
	set_length( shape( red).at(0));
	color[make_tuple( slice( 0, count), 0)] = red;
}

void
points::set_green( const array& green)
{
	lock L(mtx);
	set_length( shape( green).at(0));
	color[make_tuple( slice( 0, count), 1)] = green;
}

void
points::set_blue( const array& blue)
{
	lock L(mtx);
	set_length( shape( blue).at(0));
	color[make_tuple( slice( 0, count), 2)] = blue;
}

void
points::set_alpha( const array& alpha)
{
	lock L(mtx);
	set_length(shape( alpha).at(0));
	color[make_tuple( slice( 0, count), 3)] = alpha;
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
points::set_alpha_l( const list& alpha)
{
	this->set_alpha( array( alpha));
}

void
points::set_x( const array& x)
{
	lock L(mtx);
	set_length( shape(x).at(0));
	pos[make_tuple( slice(1, count+1), 0)] = x;
}

void
points::set_y( const array& y)
{
	lock L(mtx);
	set_length( shape(y).at(0));
	pos[make_tuple( slice(1, count+1), 1)] = y;
}

void
points::set_z( const array& z)
{
	lock L(mtx);
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
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 0)] = x;	
}

void
points::set_y_d( const double y)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 1)] = y;	
}

void
points::set_z_d( const double z)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	pos[make_tuple(slice(0,count), 2)] = z;	
}


void
points::set_red_d( float red)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 0)] = red;	
}

void
points::set_green_d( float green)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 1)] = green;	
}

void
points::set_blue_d( float blue)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 2)] = blue;	
}

void
points::set_alpha_d( float alpha)
{
	lock L(mtx);
	if (count == 0) {
		set_length(1);
	}
	color[make_tuple(slice(0,count), 3)] = alpha;	
}

bool
points::degenerate() const
{
	return count == 0;
}

struct point_coord
{
	vector center;
	mutable rgba color;
	inline point_coord( const vector& p, const rgba& c)
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
	if (antialias) { // Every point must be depth sorted
		for ( ; pos_i < pos_end && color_i < color_end; pos_i += 3, color_i += 4) {
			translucent_points.push_back( point_coord( vector(pos_i), rgba(color_i)));
		}
	}
	else { // Only translucent points need to be depth-sorted
		for ( ; pos_i < pos_end && color_i < color_end; pos_i += 3, color_i += 4) {
			if (color_i[3] < 1.0)
				translucent_points.push_back( point_coord( vector(pos_i), rgba(color_i)));
			else
				opaque_points.push_back( point_coord( vector(pos_i), rgba(color_i)));
		}
	}
	// Now conditionally apply transformations for gcf and anaglyph color
	if (translucent_points.size())
		renderable::color.alpha = 0.5;
	if (scene.gcf != 1.0) {
		for (opaque_iterator i = opaque_points.begin(); i != opaque_points.end(); ++i) {
			i->center *= scene.gcf;
		}
		for (translucent_iterator i = translucent_points.begin(); i != translucent_points.end(); ++i) {
			i->center *= scene.gcf;
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
	if (world_scale_points_supported < 0) {
		init_pointparam_extension();
	}
	
	if (antialias)
		glEnable( GL_POINT_SMOOTH);

	if (size_type == WORLD && world_scale_points_supported > 0) {
		/* The coefficients of the point parameters equation are derived as
		 * follows:
		 * 
		 * Let |x-y| be the size of a unit-size point, such that x is the center
		 * of a sphere, and y is a to the right, one unit distance away.
		 * Let x' = modelview * x; x" = projection * x'
		 * 
		 * The eye-coordinate distance of x is |x'|
		 * The screen size of the sphere is equal to |x"-y"|*window_width
		 * 
		 * The point attenuation equation is: 
		 * 	s_result = s_original * 1.0/sqrt(a + b*d + c* d^2)
		 *  where d = |x'| (eye coordinate distance of the point)
		 * 
		 * Since we only desire linear attenuation commisserate with distance
		 * from the viewer, let a = b = 0
		 * s_result = s_original * 1.0/(sqrt(c)*d)
		 * 
		 * Substitue s_original == 2; s_result == |x"-y"|*window_width; d == |x'|
		 * |x"-y"|*window_width = 2/(sqrt(c)*|x'|)
		 * 
		 * Solve for c:
		 * c = {2.0/(|x'| * |x"-y"| * window_width)}^2
		 * 
		 * Note: There is almost certainly an easier way to get the same number
		 * from the projection or modelview matrices directly.  Nonetheless, this
		 * does work, and is implemented below.
		 */
		// World-to-eye transformation (modelview matrix)
		tmatrix mv; mv.gl_modelview_get();
		// Projection matrix
		tmatrix proj; proj.gl_projection_get();
		// Eye-to-world transform (inverse of modelview)
		tmatrix mv_inv;	inverse( mv_inv, mv);
		
		// Find the center of the universe, in eye coordinates
		vector world_center;
		vector eye_center = mv * world_center;
		// Its eye distance from the viewer
		double dist = eye_center.mag();
		// Find a vector in the direction of the user's right, in world space,
		// of unit length
		vector world_right = (mv_inv * (eye_center + vector(0.1, 0))).norm();
		// Compute position of the center and right vectors in screen space
		vector center_screen = proj.project( mv * world_center);
		vector right_screen = proj.project( mv * world_right);
		// The size of the sphere at this distance from the camera
		double scale = (center_screen - right_screen).mag() * scene.window_width * 0.5f;
		
		float attenuation_eqn[] = { 0, 0, 1.0f/std::pow( (scale * dist), 2.0) };
		glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, attenuation_eqn);
		glPointSize( size);
	}
	else if (size_type == SCREEN) {
		// Restore to default (aka, disable attenuation)
		if (world_scale_points_supported > 0) {
			float attenuation_eqn[] = {1.0f, 0.0f, 0.0f};
			glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, attenuation_eqn);
		}
		if (antialias) {
			glPointSize( clamp( gl_smooth_radius_range[0], size, gl_smooth_radius_range[1]));
		}
		else {
			glPointSize( clamp( gl_aliased_radius_range[0], size, gl_aliased_radius_range[1]));
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
			glColorPointer( 4, GL_FLOAT, sizeof(point_coord), &begin->color.red);
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
			glColorPointer( 4, GL_FLOAT, sizeof(point_coord), &begin->color.red);
			glVertexPointer( 3, GL_DOUBLE, sizeof(point_coord), &begin->center.x);
			glDrawArrays( GL_POINTS, 0, block);
			begin += block;
		}		
	}
	
	if (!antialias) {
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
	for ( ;pos_i < pos_end; pos_i += 3, color_i += 4) {
		if (antialias || color_i[3] != 1.0)
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
}

} } // !namespace cvisual::python
