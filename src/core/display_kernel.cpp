// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "util/errors.hpp"
#include "util/tmatrix.hpp"
#include "frame.hpp"
# include "font.hpp"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <boost/lexical_cast.hpp>

namespace cvisual {

void 
display_kernel::enable_lights()
{
	glEnable( GL_LIGHTING);
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &ambient.red);
	// Note that this cascades.
	GLenum light_ids[] = {
		GL_LIGHT0,
		GL_LIGHT1,
		GL_LIGHT2,
		GL_LIGHT3,
		GL_LIGHT4,
		GL_LIGHT5,
		GL_LIGHT6,
		GL_LIGHT7		
	};
	GLenum* light_id = light_ids;
	GLenum* light_end = light_id + 8;
	light_iterator i = lights.begin();
	while (i != light_iterator(lights.end()) && light_id != light_end) {
		i->gl_begin( *light_id, gcf);
		++i;
		++light_id;
	}
	check_gl_error();
}

void
display_kernel::disable_lights()
{
	GLenum light_ids[] = {
		GL_LIGHT0,
		GL_LIGHT1,
		GL_LIGHT2,
		GL_LIGHT3,
		GL_LIGHT4,
		GL_LIGHT5,
		GL_LIGHT6,
		GL_LIGHT7		
	};
	GLenum* light_id = light_ids;
	GLenum* light_end = light_id + 8;
	light_iterator i = lights.begin();
	while (i != light_iterator(lights.end()) && light_id != light_end) {
		i->gl_end( *light_id);
		++i;
		++light_id;
	}
	glDisable( GL_LIGHTING);
	check_gl_error();
}	


void 
display_kernel::add_light( shared_ptr<light> n_light)
{
	lock L(mtx);
	if (lights.size() >= 8)
		throw std::invalid_argument( "There may be no more than 8 lights.");
	lights.push_back( n_light);
}

void 
display_kernel::remove_light( shared_ptr<light> old_light)
{
	lock L(mtx);
	lights.remove( old_light);
	// std::remove( lights.begin(), lights.end(), old_light);
}

void 
display_kernel::illuminate_default()
{
	lights.clear();
	ambient = rgba( 0.2, 0.2, 0.2);
	shared_ptr<light> n_light( new 
		light( vector(0.25, 0.5, 1.0).norm(), rgba( 0.8, 0.8, 0.8)));
	n_light->set_local( false);
	add_light( n_light);
	
	n_light =  shared_ptr<light>(
		new light( vector( -1.0, -0.25, -0).norm(), rgba( 0.3, 0.3, 0.3)));
	n_light->set_local( false);
	add_light( n_light);
}


display_kernel::display_kernel()
	: window_width(384),
	window_height(256),
	center(mtx, 0, 0, 0),
	forward(mtx, 0, 0, -1),
	up(mtx, 0, 1, 0),
	forward_changed(true),
	cycles_since_extent(4),
	fov( 60 * M_PI / 180.0),
	autoscale(true),
	autocenter(false),
	user_scale(1.0),
	world_scale(10.0),
	gcf(1.0),
	gcf_changed(false),
	ambient( 0.2, 0.2, 0.2),
	fps( 3e-3), // Ambitiously initialize to 3 ms per cycle.
	background(0, 0, 0, 0), //< Transparent black.
	mouse_mode( ZOOM_ROTATE),
	stereo_mode( NO_STEREO),
	lod_adjust(0)
{
}

display_kernel::~display_kernel()
{
}

void
display_kernel::report_mouse_motion( float dx, float dy, mouse_button button)
{
	// This stuff handles automatic movement of the camera in responce to user
	// input.  See also view_to_world_transform for how the affected variables 
	// are used to actually position the camera.
	
	// Scaling conventions:
	// the full width of the widget rotates the scene horizontally by 120 degrees.
	// the full height of the widget rotates the scene vertically by 120 degrees.
	// the full height of the widget zooms the scene by a factor of 10
	
	// Panning conventions:
	// The full height or width of the widget pans the scene by the eye distance.
	
	// The vertical and horizontal fractions of the window's height that the 
	// mouse has traveled for this event.
	// TODO: Implement ZOOM_ROLL modes.
	float vfrac = dy / window_height;
	float hfrac = dx / window_width;
	
	// The amount by which the scene should be scaled left-to-right.
	// TODO: This is flat wrong.  See label.cpp for proper screen-to-world scaling.
	double pan_rate = user_scale * world_scale * gcf / std::tan( fov*0.5);
	
	switch (button) {
		case NONE: case LEFT:
			break;
		case MIDDLE:
			switch (mouse_mode) {
				case FIXED:
					// Locked.
					break;
				case PAN:
					// Pan front/back.
					center += pan_rate * vfrac * forward.norm();
					break;
				case ZOOM_ROLL: case ZOOM_ROTATE: {
					// Zoom in/out.
						lock L(mtx);
						user_scale *= std::pow( 10.0f, vfrac);
					}
					break;
			}
			break;
		case RIGHT:
			switch (mouse_mode) {
				case FIXED: case ZOOM_ROLL:
					break;
				case PAN: {
					// Pan up/down and left/right.
					// A vector pointing along the camera's horizontal axis.
					vector horiz_dir = forward.cross(up).norm();
					// A vector pointing along the camera's vertical axis.
					vector vert_dir = horiz_dir.cross(forward).norm();
					center += -horiz_dir * pan_rate * hfrac;
					center += vert_dir * pan_rate * vfrac;
					break;
				}
				case ZOOM_ROTATE: {
					// Rotate
					// First perform the rotation about the up vector.
					tmatrix R = rotation( -hfrac * 2.0, up.norm());
					forward = R * forward;
					
					// Then perform rotation about an axis orthogonal to up and forward.
					double vertical_angle = vfrac * 2.0;
					double max_vertical_angle = up.diff_angle(-forward.norm());
					if (vertical_angle > max_vertical_angle - 0.02) {
						vertical_angle = max_vertical_angle - 0.02;
					}
					else if (vertical_angle < -M_PI + max_vertical_angle + 0.02) {
						vertical_angle = -M_PI + max_vertical_angle + 0.02;
					}
					R = rotation( -vertical_angle, forward.cross(up).norm());
					forward = R * forward;
					forward_changed = true;
					break;
				}
			}
			break;
	}
}

void
display_kernel::report_resize( float new_width, float new_height)
{
	lock L(mtx);
	window_height = new_height;
	window_width = new_width;
}

void
display_kernel::report_realize()
{
	lock L(mtx);
	gl_begin();
	clear_gl_error();
	
	// Those features of OpenGL that are always used are set up here.	
	// Depth buffer properties
	glClearDepth( 1.0);
	glEnable( GL_DEPTH_TEST);
	glDepthFunc( GL_LEQUAL);
	
	// Lighting model properties
	glShadeModel( GL_SMOOTH);
	// TODO: Figure out what the concrete costs/benefits of this command are.
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable( GL_NORMALIZE);
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable( GL_COLOR_MATERIAL);
	
	// FSAA.  Doesn't seem to have much of an effect on my TNT2 card.  Grrr.
	glEnable( GL_MULTISAMPLE_ARB);
	// fps_font = shared_ptr<FTGLPixmapFont>( new FTGLPixmapFont( 
	// 	"/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf"));
	// fps_font->FaceSize( 12, 100);
	// assert( !fps_font->Error());
	check_gl_error();
	gl_end();
}

// Set up matricies for transforms from world coordinates to view coordinates
// Precondition: the OpenGL Modelview and Projection matrix stacks should be
// at the bottom.
// Postcondition: active matrix stack is GL_MODELVIEW, matrix stacks are at
// the bottom.  Viewing transformations have been applied.  geometry.camera
// is initialized.
// whicheye: -1 for left, 0 for center, 1 for right.
void
display_kernel::world_to_view_transform(view& geometry, int whicheye, bool forpick)
{
	// Scaling the view.  Problem: objects in VPython are specified using
	// double-precision floats, while OpenGL automatically truncates those
	// down to single precision.  So, objects larger than about 1e16 generate
	// NaN or infinity when rendered.  Solution:  When we calculate the extent
	// of the universe in world space we calculate two scaling factors.
	// The first (render_surface::world_scale) is equal to the size of the
	// universe across its 3-axis diagonal.  The second (render_surface::gcf;
	// is sized such that fabs(most_extreme_coordinate * gcf) < 1e10f.
	// All world space coordinates are multiplied by the gcf
	// prior to rendering.  All objects' rendering algorithms are specified such
	// that their total size in model space is about 1 unit, and their size is
	// blown up to world space by overriding get_scale().  Finally, the camera's
	// position from the center (which determines the overall appearance of the
	// size of the universe) is determined by four scaling factors.  The first
	// is determined by the total extent of the scene (render_surface::world_scale).
	// The second applies the scaling factor applied to the object coordinates
	// by multiplying the camera distance by gcf.  The third is the interactive
	// scaling factor (render_surface::user_scale), initialized to 1.0, and 
	// changed by dragging the mouse middle button across the viewing area up/down.
	// Finally, the product of those scaling factors approximates the maximum
	// apparent width of the scene in the display, it is divided by tan(fov/2)
	// to correctly determine the distance from the center that encompasses the
	// calculated width.  This algorithm is stable for objects up to about
	// 1e+/-150, whereas vpython's original algorithm was stable to a range of 
	// about 1e-38 - 1e45, which was deemed acceptable before.
	
	// See http://www.stereographics.com/support/developers/pcsdk.htm for a
	// discussion regarding the design basis for the frustum offset code.
	
	// Calculate the extent of the universe.  For speed, this is only performed
	// once every four renders.
	if (cycles_since_extent >= 4) {
		recalc_extent();
	}
	
	// Verify a precondition:
	// Objects must be within a reasonable size on the screen.
	assert( std::fabs(std::log( world_scale * gcf)) < std::log( 1e12));
	
	// TODO: adjust camera distance calculation based on the scene center's
	// offset from the extent's center.
	vector scene_center = center * gcf;
	vector scene_up = up.norm();
	// tangent of half the field of view.
	double tan_hfov = std::tan( fov*0.5);
	// the vertical and horizontal tangents of half the field of view.
	double aspect_ratio = window_height / geometry.window_width;
	double tan_hfov_x = 0.0;
	double tan_hfov_y = 0.0;
	if (aspect_ratio > 1.0) {
		tan_hfov_y = tan_hfov;
		tan_hfov_x = tan_hfov_y / aspect_ratio;
	}
	else {
		tan_hfov_x = tan_hfov;
		tan_hfov_y = tan_hfov_x * aspect_ratio;
	}
	// The scaled distance between the camera and the visual center of the scene.
	double eye_length = user_scale * world_scale * gcf / tan_hfov;
	// The position of the camera.
	vector camera = -forward.norm() * eye_length + scene_center;
	// Establish the distances to the clipping planes.
	double nearclip = eye_length * 0.01;
	// TODO: revisit this in the face of user-driven camera panning.
	double farclip = (world_extent.center() - camera).mag()*2 + world_scale * gcf;
	// Translate camera left/right 2% of the viewable width of the scene
	double camera_stereo_offset = tan_hfov * eye_length * 0.02;
	// TODO: This should be doable with a simple glTranslated() call, but I haven't
	// found the magic formula for it.
	vector camera_stereo_delta = camera_stereo_offset * up.cross( camera).norm() * whicheye;
	camera += camera_stereo_delta;
	scene_center += camera_stereo_delta;
	// A multiple of the number of eye_length's away from the camera to place the
	// zero-parallax plane.
	double stereodepth = 1.0; // TODO: make this value configurable.
	// The distance from the camera to the zero-parallax plane.
	double focallength = eye_length * stereodepth;
	// The amount to translate the frustum to the left and right.
	double frustum_stereo_offset = camera_stereo_offset * nearclip / focallength;
	
	// Finally, the OpenGL transforms based on the geometry just calculated.
	clear_gl_error();
	// Position the camera.
	glMatrixMode( GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		camera.x, camera.y, camera.z,
		scene_center.x, scene_center.y, scene_center.z,
		scene_up.x, scene_up.y, scene_up.z);

	
	// Establish a parallel-axis asymmetric stereo projection frustum.
	glMatrixMode( GL_PROJECTION);
	if (!forpick)
		glLoadIdentity();
	if (whicheye == 1) {
		frustum_stereo_offset = -frustum_stereo_offset;
	}
	else if (whicheye == 0) {
		frustum_stereo_offset = 0;
	}
	glFrustum( 
		-nearclip * tan_hfov_x + frustum_stereo_offset,
		nearclip * tan_hfov_x + frustum_stereo_offset,
		-nearclip * tan_hfov_y, 
		nearclip * tan_hfov_y,
		nearclip,
		farclip);
	
	glMatrixMode( GL_MODELVIEW);
	check_gl_error();
	
	// Finish initializing the view object.
	geometry.camera = camera / gcf;
	geometry.tan_hfov_x = tan_hfov_x;
	geometry.tan_hfov_y = tan_hfov_y;
	// The true viewing vertical direction is not the same as what is needed for
	// gluLookAt().
	geometry.up = forward.cross(up).cross(forward).norm();
}

// Calculate a new extent for the universe, adjust gcf, center, and world_scale
// as required.
void
display_kernel::recalc_extent(void)
{
	world_extent.reset();
	world_iterator i( layer_world.begin());
	world_iterator end( layer_world.end());
	while (i != end) {
		i->grow_extent( world_extent);
		++i;
	}
	world_trans_iterator j( layer_world_transparent.begin());
	world_trans_iterator j_end( layer_world_transparent.end());
	while (j != j_end) {
		j->grow_extent( world_extent);
		++j;
	}
	cycles_since_extent = 0;
	if (autocenter) {
		// Move the camera to accomodate the new center of the scene
		center = world_extent.center();
		// Ensure that subsequent usage of the scene extent is correct
		// in the new camera space.
		world_extent.recenter();
	}
	if (autoscale) {
		world_scale = world_extent.scale();
		if (world_scale == 0.0)
			world_scale = 10.0;
		if (world_scale > 1e154) {
			VPYTHON_CRITICAL_ERROR( "Cannot represent scene geometry with"
				" an extent greater than about 1e154 units.");
		}
	}
	// TODO: There should be a faster way to do this comparison.
	if (std::fabs(std::log( world_scale * gcf)) >= std::log( 1e10)) {
		// Reset the global correction factor.
		gcf = 1.0 / world_scale;
		gcf_changed = true;
	}
}

void 
display_kernel::add_renderable( shared_ptr<renderable> obj)
{
	lock L(mtx);
	if (obj->color.alpha == 1.0)
		layer_world.push_back( obj);
	else
		layer_world_transparent.push_back( obj);
}
	
void 
display_kernel::remove_renderable( shared_ptr<renderable> obj)
{
	lock L(mtx);
	// Choice of removal algorithms:  For containers that support thier own
	// removal methods (list, set), use the member function.  Else, use 
	// std::remove.
	layer_world.remove( obj);
	std::remove( 
		layer_world_transparent.begin(), layer_world_transparent.end(),	obj);
}

bool
display_kernel::draw( 
	view& scene_geometry, int whicheye, bool anaglyph, bool coloranaglyph)
{
	// Set up the base modelview and projection matricies
	world_to_view_transform( scene_geometry, whicheye);
	
	// Render all opaque objects in the world space layer
	enable_lights();
	world_iterator i( layer_world.begin());
	world_iterator i_end( layer_world.end());
	while (i != i_end) {
		if (i->color.alpha != 1.0) {
			// The color of the object has become transparent when it was not
			// initially.  Move it to the transparent layer.  The penalty for
			// being rendered in the transparent layer when it is opaque is only
			// a small speed hit when it has to be sorted.  Therefore, that case
			// is not tested at all.
			layer_world_transparent.push_back( *i.base());
			i = layer_world.erase(i.base());
			continue;
		}
		i->refresh_cache( scene_geometry);
		rgba actual_color = i->color;
		if (anaglyph) {
			if (coloranaglyph) {
				i->color = actual_color.desaturate();
			}
			else {
				i->color = actual_color.grayscale();
			}
		}
		i->gl_render( scene_geometry);
		if (anaglyph)
			i->color = actual_color;
		++i;
	}
	
	// Perform a depth sort of the transparent world from back to front.
	if (layer_world_transparent.size() > 1)
		std::stable_sort( layer_world_transparent.begin(), layer_world_transparent.end(),
			z_comparator( forward.norm()));
	
	// Render translucent objects in world space.
	world_trans_iterator j( layer_world_transparent.begin());
	world_trans_iterator j_end( layer_world_transparent.end());
	while (j != j_end) {
		j->refresh_cache( scene_geometry);
		rgba actual_color = j->color;
		if (anaglyph) {
			if (coloranaglyph) {
				j->color = actual_color.desaturate();
			}
			else {
				j->color = actual_color.grayscale();
			}
		}
		j->gl_render( scene_geometry);
		if (anaglyph)
			j->color = actual_color;
		++j;
	}
	
	// Render all objects in screen space.
	disable_lights();
	glDisable( GL_DEPTH_TEST);
	typedef std::multimap<vector, displaylist, z_comparator>::iterator
		screen_iterator;
	screen_iterator k( scene_geometry.screen_objects.begin());
	screen_iterator k_end( scene_geometry.screen_objects.end());
	while ( k != k_end) {
		k->second.gl_render();
		++k;
	}
	scene_geometry.screen_objects.clear();
	glEnable( GL_DEPTH_TEST);
	
	return true;
}


// Renders the entire scene.
bool
display_kernel::render_scene(void)
{
	lock L(mtx);
	try {
		fps.start();
		view scene_geometry( forward.norm(), center, window_width, window_height, 
			forward_changed, gcf, gcf_changed);
		scene_geometry.lod_adjust = lod_adjust;
		gl_begin();
		clear_gl_error();
		glClearColor( background.red, background.green, background.blue, 0);
		// Control which type of stereo to perform.
		switch (stereo_mode) {
			case NO_STEREO:
				scene_geometry.anaglyph = false;
				scene_geometry.coloranaglyph = false;
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw(scene_geometry, 0);
				break;
			case ACTIVE_STEREO:
				scene_geometry.anaglyph = false;
				scene_geometry.coloranaglyph = false;
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glDrawBuffer( GL_BACK_LEFT);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, -1);
				glDrawBuffer( GL_BACK_RIGHT);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, 1);
				break;
			case REDBLUE_STEREO:
				// Red channel
				scene_geometry.anaglyph = true;
				scene_geometry.coloranaglyph = false;
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glColorMask( GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
				draw( scene_geometry, -1, true, false);
				// Blue channel
				glColorMask( GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
				glClear( GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, 1, true, false);
				// Put everything back
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				break;
			case REDCYAN_STEREO:
				// Red channel
				scene_geometry.anaglyph = true;
				scene_geometry.coloranaglyph = true;
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glColorMask( GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
				draw( scene_geometry, -1, true, true);
				// Cyan channel
				glColorMask( GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
				glClear( GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, 1, true, true);
				// Put everything back
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				break;
			case YELLOWBLUE_STEREO:
				// Yellow channel
				scene_geometry.anaglyph = true;
				scene_geometry.coloranaglyph = true;
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glColorMask( GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
				draw( scene_geometry, -1, true, true);
				// Blue channel
				glColorMask( GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
				glClear( GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, 1, true, true);
				// Put everything back
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				break;
			case GREENMAGENTA_STEREO:
				// Green channel
				scene_geometry.anaglyph = true;
				scene_geometry.coloranaglyph = true;
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glColorMask( GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
				draw( scene_geometry, -1, true, true);
				// Magenta channel
				glColorMask( GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
				glClear( GL_DEPTH_BUFFER_BIT);
				draw( scene_geometry, 1, true, true);
				// Put everything back
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				break;
			case PASSIVE_STEREO: {
				// Also handle viewport modifications.
				scene_geometry.window_width = window_width * 0.5;
				scene_geometry.anaglyph = false;
				scene_geometry.coloranaglyph = false;
				int stereo_width = int(scene_geometry.window_width);
				// Left eye
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
				glViewport( 0, 0, stereo_width,
					static_cast<int>(window_height));
				draw( scene_geometry, -1);
				// Right eye
				glViewport( stereo_width+1, 0, stereo_width, 
					static_cast<int>(window_height));
				draw( scene_geometry, 1);
				break;
			}
		}

		std::string fps_msg( "Cycle time: ");
		fps_msg += boost::lexical_cast<std::string>(fps.read());
		// glEnable( GL_TEXTURE_2D);
		glColor3f( 1.0f - background.red, 1.0f-background.green, 1.0f-background.blue);
		bitmap_font font;
#if 1
		glMatrixMode( GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D( 0, window_width, 0, window_height);
		glMatrixMode( GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslated( 0, -font.descent(), 0);
#endif
		// glRasterPos2d( 0, -font.descent());
		glRasterPos2d( 0, 0);
		font.gl_render( fps_msg);
#if 1
		glPopMatrix();
		glMatrixMode( GL_PROJECTION);
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW);
		// glDisable( GL_TEXTURE_2D);
#endif

		
		// Cleanup
		gl_swap_buffers();
		check_gl_error();
		gl_end();
		fps.stop();
		cycles_since_extent++;
		gcf_changed = false;
		forward_changed = false;
	}
	catch (gl_error e) {
		std::ostringstream msg;
		msg << "OpenGL error: " << e.what() << ", aborting.\n";
		VPYTHON_CRITICAL_ERROR( msg.str());
		std::exit(1);
	}
	return true;
}

shared_ptr<renderable> 
display_kernel::pick( float x, float y, float d_pixels)
{
	lock L(mtx);
	shared_ptr<renderable> best_pick;
	try {
		clear_gl_error();
		// Notes:
		// culled polygons don't count.  glRasterPos() does count.
	
		// Allocate a selection buffer of uints.  Format for returned hits is:
		// {uint32: n_names}{uint32: minimunm depth}{uint32: maximum depth}
		// {unit32[n_names]: name_stack}
		// n_names is the depth of the name stack at the time of the hit.
		// minimum and maximum depth are the minimum and maximum values in the 
		// depth buffer scaled between 0 and 2^32-1. (source is [0,1])
		// name_stack is the full contents of the name stack at the time of the hit.
		
		size_t hit_buffer_size = std::max(
				(layer_world.size()+layer_world_transparent.size())*4,
				world_extent.get_select_buffer_depth());
		// TODO: And this is a GNU extension...
		unsigned int hit_buffer[hit_buffer_size];
		
		// Allocate a std::vector<shared_ptr<renderable> > to lookup names
		// as they are rendered.
		std::vector<shared_ptr<renderable> > name_table;
		// Pass the name stack to OpenGL with glSelectBuffer.
		glSelectBuffer( hit_buffer_size, hit_buffer);
		// Enter selection mode with glRenderMode
		glRenderMode( GL_SELECT);
		glClear( GL_DEPTH_BUFFER_BIT);
		// Clear the name stack with glInitNames(), raise the height of the name
		// stack with glPushName() exactly once.
		glInitNames();
		glPushName(0);
		
		// Initialize the picking matrix.
		int viewport_bounds[4] = { 
			0, 0, static_cast<int>(window_width), static_cast<int>(window_height)
		};
		glMatrixMode( GL_PROJECTION);
		glLoadIdentity();
		gluPickMatrix( x, window_height - y, d_pixels, d_pixels, viewport_bounds);
		view scene_geometry( forward.norm(), center, window_width, window_height, 
			forward_changed, gcf, gcf_changed);
		scene_geometry.lod_adjust = lod_adjust;
		world_to_view_transform( scene_geometry, 0, true);
	
		// Iterate across the world, rendering each body for picking.
		std::list<shared_ptr<renderable> >::iterator i = layer_world.begin();
		std::list<shared_ptr<renderable> >::iterator i_end = layer_world.end();
		while (i != i_end) {
			glLoadName( name_table.size());
			name_table.push_back( *i);
			(*i)->gl_pick_render( scene_geometry);
			++i;
		}
		std::vector<shared_ptr<renderable> >::iterator j = layer_world_transparent.begin();
		std::vector<shared_ptr<renderable> >::iterator j_end = layer_world_transparent.end();
		while (j != j_end) {
			glLoadName( name_table.size());
			name_table.push_back( *j);
			(*j)->gl_pick_render( scene_geometry);
			++j;
		}
		// Return the name stack to the bottom with glPopName() exactly once.
		glPopName();
	
		// Exit selection mode, return to normal rendering rendering. (collects
		// the number of hits at this time).
		size_t n_hits = glRenderMode( GL_RENDER);
		check_gl_error();
		
		// Lookup the name to get the shared_ptr<renderable> associated with it.
		double best_pick_depth = 1.0; // The farthest point away in the depth buffer.
		unsigned int* hit_record = hit_buffer;
		unsigned int* const hit_buffer_end = hit_buffer + hit_buffer_size;
		while (n_hits > 0 && hit_record < hit_buffer_end) {
			unsigned int n_names = hit_record[0];
			if (hit_record + 3 + n_names > hit_buffer_end)
				break;
			double min_hit_depth = static_cast<double>(hit_record[1]) / 0xffffffffu;
			if (min_hit_depth < best_pick_depth) {
				best_pick_depth = min_hit_depth;
				best_pick = name_table[*(hit_record+3)];
				if (n_names > 1) {
					// Then the picked object is the child of a frame.
					frame* ref_frame = dynamic_cast<frame*>(best_pick.get());
					assert(ref_frame != NULL);
					best_pick = ref_frame->lookup_name(
						hit_record + 4, hit_record + 3 + n_names);
				}
			}
			hit_record += 3 + n_names;
			n_hits--;
		}
	}
	catch (gl_error e) {
		std::ostringstream msg;
		msg << "OpenGL error: " << e.what() << ", aborting.\n";
		VPYTHON_CRITICAL_ERROR( msg.str());
		std::exit(1);
	}
	return best_pick;
}

void
display_kernel::set_up( const vector& n_up)
{
	up = n_up;
}

shared_vector&
display_kernel::get_up()
{
	return up;
}

void
display_kernel::set_forward( const vector& n_forward)
{
	if (n_forward == vector())
		throw std::invalid_argument( "Forward cannot be zero.");
	forward = n_forward;
	forward_changed = true;
}

shared_vector&
display_kernel::get_forward()
{
	return forward;
}

void
display_kernel::set_center( const vector& n_center)
{
	center = n_center;
}

shared_vector&
display_kernel::get_center()
{
	return center;
}

void
display_kernel::set_fov(double n_fov)
{
	lock L(mtx);
	fov = n_fov;
}

double
display_kernel::get_fov()
{
	return fov;
}

void
display_kernel::set_background( const rgba& n_background)
{
	lock L(mtx);
	background = n_background;
}

rgba
display_kernel::get_background()
{
	return background;
}

void
display_kernel::set_forground( const rgba& n_forground)
{
	lock L(mtx);
	forground = n_forground;
}

rgba
display_kernel::get_forground()
{
	return forground;
}

void
display_kernel::set_autoscale( bool n_autoscale)
{
	lock L(mtx);
	autoscale = n_autoscale;
}

bool
display_kernel::get_autoscale()
{
	return autoscale;
}

bool
display_kernel::get_autocenter()
{
	return autocenter;
}

void
display_kernel::set_autocenter( bool n_autocenter)
{
	lock L(mtx);
	autocenter = n_autocenter;
}

void
display_kernel::set_stereomode( std::string mode)
{
	if (mode == "nostereo")
		stereo_mode = NO_STEREO;
	else if (mode == "active")
		stereo_mode = ACTIVE_STEREO;
	else if (mode == "passive")
		stereo_mode = PASSIVE_STEREO;
	else if (mode == "redblue")
		stereo_mode = REDBLUE_STEREO;
	else if (mode == "redcyan")
		stereo_mode = REDCYAN_STEREO;
	else if (mode == "yellowblue")
		stereo_mode = YELLOWBLUE_STEREO;
	else if (mode == "greenmagenta")
		stereo_mode = GREENMAGENTA_STEREO;
	else throw std::invalid_argument( "Unimplemented or invalid stereo mode");
}

std::string
display_kernel::get_stereomode()
{
	switch (stereo_mode) {
		case NO_STEREO:
			return "nostereo";
		case ACTIVE_STEREO:
			return "active";
		case PASSIVE_STEREO:
			return "passive";
		case REDBLUE_STEREO:
			return "redblue";
		case REDCYAN_STEREO:
			return "redcyan";
		case YELLOWBLUE_STEREO:
			return "yellowblue";
		case GREENMAGENTA_STEREO:
			return "greenmagenta";
		default:
			// Not strictly required, this just silences a warning about control
			// reaching the end of a non-void funciton.
			return "nostereo";
	}
}

std::list<shared_ptr<renderable> > 
display_kernel::get_objects() const
{
	lock L(mtx);
	std::list<shared_ptr<renderable> > ret = layer_world;
	ret.insert( ret.end(), layer_world_transparent.begin(), layer_world_transparent.end());
	return ret;
}

} // !namespace cvisual
