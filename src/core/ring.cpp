#include "ring.hpp"
#include "util/displaylist.hpp"
#include <utility>

namespace {
template <typename T>
T
clamp( T const& lower, T const& value, T const& upper)
{
	if (lower > value)
		return lower;
	if (upper < value)
		return upper;
	return value;
}	
	
} // !namespace (unnamed)

ring::ring()
	: thickness( 0.0), radius( 1.0)
{
}

void
ring::set_radius( double r)
{
	radius = r;
}

void
ring::set_thickness( double t)
{
	thickness = t;
}

void
ring::gl_pick_render( const view& scene)
{
	do_render_opaque( scene, 7, 10);
	return;
}

// TODO: Figure out how to do transparency for this body.
void
ring::gl_render( const view& scene)
{
#if 0
	// Implementation.  In software, I create a pair of arrays for vertex and
	// normal data, filling them with the coordinates for one band of 
	// the ring as a triangle strip.  Each successive band is created with 
	// sequential calls to glRotate() using the same vertex array, thereby 
	// taking advantage of OpenGL's hardware for the bulk of the transform labor.
	if (radius == 0.0)
		return;
	double scaled_radius = radius * scene.gcf;
	double scaled_thickness = scaled_radius * 0.1;
	if (thickness != 0.0)
		scaled_thickness = thickness * scene.gcf;
#endif
	
	// Level of detail estimation.  See sphere::gl_render().
	// TODO: This should be captured in the view struct since it is so common.
	double camera_dist = ((pos - scene.camera) * scene.gcf).mag();
	double lod_determinant = radius * scene.gcf / camera_dist;
	// TODO: Calibrate these parameters.
	// The number of subdivisions around the hoop's radial direction.
	size_t bands = static_cast<size_t>(lod_determinant * 50.0);
	bands = clamp<size_t>( 5, bands, 41);
	// The number of subdivions around the hoop's tangential direction.
	size_t rings = static_cast<size_t>(lod_determinant * 200.0);
	rings = clamp<size_t>( 7, rings, 51);
#if 1
	do_render_opaque( scene, rings, bands);
#else
	// The first band is a triangle strip at the point where the default ring
	// passes through the yz plane through the +z axis.  The extra pair of vertexes
	// and normals is to form a closed loop.  The format is of a pair of
	// interleaved rings, the first (in the xz plane) is stored at the even indexes;
	// the second (rotated slightly above the first) is stored at the odd indexes.
	vector vertexes[bands * 2 + 2];
	vector normals[bands * 2 + 2];
	vertexes[0] = vertexes[ bands * 2] = 
		vector( 0, 0, scaled_radius + scaled_thickness);
	normals[0] = normals[bands * 2] = vertexes[0].norm();
	tmatrix rotator = rotation(
		2.0 * M_PI / bands, 
		vector(0, 1, 0), 
		vector( 0, 0, scaled_radius));
	tmatrix normal_rotator = rotation(2.0 *  M_PI / bands, vector( 0, 1, 0));
	for (size_t i = 2; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-2];
		normals[i] = normal_rotator * normals[i-2];
	}
	// Rotate the single circle about the +x axis to produce the second 
	// interleaved circle.
	rotator = rotation( M_PI * 2.0 / rings, vector(1,0,0));
	for (size_t i = 1; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-1];
		normals[i] = rotator * normals[i-1];
	}
	vertexes[bands*2+1] = vertexes[1];
	normals[bands*2+1] = normals[1];
	
	// Point OpenGL at the vertex data for the first triangle strip.
	glEnableClientState( GL_VERTEX_ARRAY);
	glEnableClientState( GL_NORMAL_ARRAY);
	glVertexPointer( 3, GL_DOUBLE, 0, vertexes);
	glNormalPointer( GL_DOUBLE, 0, normals);
	color.gl_set();

	{	
		gl_matrix_stackguard guard;
		vector scaled_pos = pos * scene.gcf;
		glTranslated( scaled_pos.x, scaled_pos.y, scaled_pos.z);
		model_world_transform().gl_mult();
		// Draw the first strip.
		glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		for (size_t i = 0; i < rings; ++i) {
			// Successively render the same triangle strip for each band, 
			// rotated about the model's x axis into the next position.
			glRotated( 360.0 / rings, 1, 0, 0);
			glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		}
	}
	
	glDisableClientState( GL_VERTEX_ARRAY);
	glDisableClientState( GL_NORMAL_ARRAY);
#endif
	return;
}

void
ring::grow_extent( extent& world)
{
	world.add_point( pos);
}

void 
ring::do_render_opaque( const view& scene, size_t rings, size_t bands)
{
	// Implementation.  In software, I create a pair of arrays for vertex and
	// normal data, filling them with the coordinates for one band of 
	// the ring as a triangle strip.  Each successive band is created with 
	// sequential calls to glRotate() using the same vertex array, thereby 
	// taking advantage of OpenGL's hardware for the bulk of the transform labor.
	if (radius == 0.0)
		return;
		
	double scaled_radius = radius * scene.gcf;
	double scaled_thickness = scaled_radius * 0.1;
	if (thickness != 0.0)
		scaled_thickness = thickness * scene.gcf;

	// The first band is a triangle strip at the point where the default ring
	// passes through the yz plane through the +z axis.  The extra pair of vertexes
	// and normals is to form a closed loop.  The format is of a pair of
	// interleaved rings, the first (in the xz plane) is stored at the even indexes;
	// the second (rotated slightly above the first) is stored at the odd indexes.
	vector vertexes[bands * 2 + 2];
	vector normals[bands * 2 + 2];
	vertexes[0] = vertexes[ bands * 2] = 
		vector( 0, 0, scaled_radius + scaled_thickness);
	normals[0] = normals[bands * 2] = vertexes[0].norm();
	tmatrix rotator = rotation(
		2.0 * M_PI / bands, 
		vector(0, 1, 0), 
		vector( 0, 0, scaled_radius));
	tmatrix normal_rotator = rotation(2.0 *  M_PI / bands, vector( 0, 1, 0));
	for (size_t i = 2; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-2];
		normals[i] = normal_rotator * normals[i-2];
	}
	// Rotate the single circle about the +x axis to produce the second 
	// interleaved circle.
	rotator = rotation( M_PI * 2.0 / rings, vector(1,0,0));
	for (size_t i = 1; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-1];
		normals[i] = rotator * normals[i-1];
	}
	vertexes[bands*2+1] = vertexes[1];
	normals[bands*2+1] = normals[1];
	
	// Point OpenGL at the vertex data for the first triangle strip.
	glEnableClientState( GL_VERTEX_ARRAY);
	glEnableClientState( GL_NORMAL_ARRAY);
	glVertexPointer( 3, GL_DOUBLE, 0, vertexes);
	glNormalPointer( GL_DOUBLE, 0, normals);
	color.gl_set();

	{	
		gl_matrix_stackguard guard;
		vector scaled_pos = pos * scene.gcf;
		glTranslated( scaled_pos.x, scaled_pos.y, scaled_pos.z);
		model_world_transform().gl_mult();
		// Draw the first strip.
		glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		for (size_t i = 0; i < rings; ++i) {
			// Successively render the same triangle strip for each band, 
			// rotated about the model's x axis into the next position.
			glRotated( 360.0 / rings, 1, 0, 0);
			glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		}
	}
	
	glDisableClientState( GL_VERTEX_ARRAY);
	glDisableClientState( GL_NORMAL_ARRAY);
	return;
}
