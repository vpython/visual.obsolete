#include "render_core.hpp"
#include "util/errors.hpp"
#include "util/tmatrix.hpp"

#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include <GL/gl.h>
#include <GL/glu.h>

void 
render_core::enable_lights()
{
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &ambient.red);
	// Note that this cascades.
	switch (lights.size()) {
		case 8:
			lights[7]->gl_begin( GL_LIGHT7, gcf);
		case 7:
			lights[6]->gl_begin( GL_LIGHT6, gcf);
		case 6:
			lights[5]->gl_begin( GL_LIGHT5, gcf);
		case 5:
			lights[4]->gl_begin( GL_LIGHT4, gcf);
		case 4:
			lights[3]->gl_begin( GL_LIGHT3, gcf);
		case 3:
			lights[2]->gl_begin( GL_LIGHT2, gcf);
		case 2:
			lights[1]->gl_begin( GL_LIGHT1, gcf);
		case 1:
			lights[0]->gl_begin( GL_LIGHT0, gcf);
		default:
			return;
	}
	check_gl_error();
}

void
render_core::disable_lights()
{
	switch (lights.size()) {
		case 8:
			lights[7]->gl_end( GL_LIGHT7);
		case 7:
			lights[6]->gl_end( GL_LIGHT6);
		case 6:
			lights[5]->gl_end( GL_LIGHT5);
		case 5:
			lights[4]->gl_end( GL_LIGHT4);
		case 4:
			lights[3]->gl_end( GL_LIGHT3);
		case 3:
			lights[2]->gl_end( GL_LIGHT2);
		case 2:
			lights[1]->gl_end( GL_LIGHT1);
		case 1:
			lights[0]->gl_end( GL_LIGHT0);
		default:
			return;		
	}
	check_gl_error();
}	


void 
render_core::add_light( shared_ptr<light> n_light)
{
	if (lights.size() >= 8)
		throw std::invalid_argument( "There may be no more than 8 lights.");
	lights.push_back( n_light);
}

void 
render_core::remove_light( shared_ptr<light> old_light)
{
	std::remove( lights.begin(), lights.end(), old_light);
}

void 
render_core::illuminate_default()
{
	add_light( shared_ptr<light>( 
		new light( vector(0.25, 0.5, 1.0).norm(), rgba( 0.8, 0.8, 0.8))));
	add_light( shared_ptr<light>(
		new light( vector( -1.0, -0.25, -0).norm(), rgba( 0.3, 0.3, 0.3))));
}


render_core::render_core()
	: window_width(384),
	window_height(256),
	center(0, 0, 0),
	forward(0, 0, -1),
	up(0, 1, 0),
	forward_changed(true),
	cycles_since_extent(4),
	fov( 60), //< In degrees - not radians!!!
	autoscale(true),
	autocenter(false),
	user_scale(1.0),
	world_scale(10.0),
	gcf(1.0),
	gcf_changed(false),
	last_mousepos_x(0),
	last_mousepos_y(0),
	ambient( 0.4, 0.4, 0.4),
	fps( 3e-3), // Ambitiously initialize to 3 ms per cycle.
	cycles_since_fps(0),
	mouse_mode( ZOOM_ROTATE),
	stereo_mode( NO_STEREO),
	lod_adjust(0),
	background(0, 0, 0, 0) //< Transparent black.
{
	illuminate_default();
}

void
render_core::report_mouse_motion( float dx, float dy, mouse_button button)
{
	// The far more complicated code: this stuff handles twisting and zooming
	// the camera around.
	// Scaling conventions:
	// the full width of the widget rotates the scene horizontally by 120 degrees.
	// the full height of the widget rotates the scene vertically by 120 degrees.
	// the full height of the widget zooms the scene by a factor of 10
	
	// Panning conventions:
	// The full height or width of the widget pans the scene by the eye distance.
	
	// The vertical and horizontal fractions of the window's height that the 
	// mouse has traveled for this event.
	// TODO: Implement PAN and ZOOM_ROLL modes.
	float vfrac = dy / window_height;
	float hfrac = dx / window_width;
	
	// TODO: maintain synchronized with the eye_length calculation below.
	double pan_rate = user_scale * world_scale * gcf / std::tan( fov*0.5 * M_PI/180.0);
	
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
					center += pan_rate * vfrac * forward;
					break;
				case ZOOM_ROLL: case ZOOM_ROTATE:
					// Zoom in/out.
					user_scale *= std::pow( 10.0f, vfrac);
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
					double max_vertical_angle = up.diff_angle(-forward);
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
render_core::report_resize( float new_width, float new_height)
{
	window_height = new_height;
	window_width = new_width;
}

void
render_core::report_realize()
{
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
	glEnable( GL_LIGHTING);
	glEnable( GL_NORMALIZE);
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable( GL_COLOR_MATERIAL);
	
	// FSAA.  Doesn't seem to have much of an effect on my TNT2 card.  Grrr.
	glEnable( GL_MULTISAMPLE_ARB);
	fps_font = shared_ptr<FTGLPixmapFont>( new FTGLPixmapFont( 
		"/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf"));
	fps_font->FaceSize( 12, 100);
	assert( !fps_font->Error());
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
render_core::world_to_view_transform(view& geometry, int whicheye)
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
	
	// Verify some preconditions:
	// forward must be a unit vector.
	assert( std::fabs(1.0 - forward.mag()) < 0.001);
	// Objects must be within a reasonable size on the screen.
	assert( std::fabs(std::log( world_scale * gcf)) < std::log( 1e12));
	
	// TODO: adjust camera distance calculation based on the scene center's
	// offset from the extent's center.
	vector scene_center = center * gcf;
	vector scene_up = up.norm();
	// tangent of half the field of view.
	double tan_hfov = std::tan( fov*0.5 * M_PI / 180.0);
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
	vector camera = -forward * eye_length + scene_center;
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
}

// Calculate a new extent for the universe, adjust gcf, center, and world_scale
// as required.
void
render_core::recalc_extent(void)
{
	world_extent.reset();
	world_iterator i( layer_world.begin());
	world_iterator end( layer_world.end());
	while (i != end) {
		i->grow_extent( world_extent);
		++i;
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

#if 1

bool
render_core::draw( 
	view& scene_geometry, int whicheye, bool anaglyph, bool coloranaglyph)
{
	// Set up the base modelview and projection matricies
	world_to_view_transform( scene_geometry, whicheye);
	// Establish the proper background.
	
	// Render all opaque objects in the world space layer
	enable_lights();
	world_iterator i( layer_world.begin());
	world_iterator i_end( layer_world.end());
	while (i != i_end) {
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
	
	// Perform a depth sort of the transparent world from forward to backward.
	if (layer_world_transparent.size() > 1)
		std::stable_sort( layer_world_transparent.begin(), layer_world_transparent.end(),
			z_comparator( forward));
	
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
	disable_lights();
	
	// Render objects in the screen layer.
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW);
	screen_iterator k( layer_screen.begin());
	screen_iterator k_end( layer_screen.end());
	while (k != k_end) {
		k->refresh_cache( scene_geometry);
		rgba actual_color = k->color;
		if (anaglyph) {
			if (coloranaglyph) {
				k->color = actual_color.desaturate();
			}
			else {
				k->color = actual_color.grayscale();
			}
		}
		k->gl_render( scene_geometry);
		if (anaglyph)
			k->color = actual_color;
		++k;
	}
	return true;
}

#endif

// Renders the entire scene.
bool
render_core::render_scene(void)
{
	try {
#if 1
		fps.start();
		view scene_geometry( forward, center, window_width, window_height, 
			forward_changed, gcf, gcf_changed);
		scene_geometry.lod_adjust = lod_adjust;
		gl_begin();
		clear_gl_error();
		glClearColor( background.red, background.green, background.blue, 0);
		// Control which type of stereo to perform.
		switch (stereo_mode) {
			case NO_STEREO:
				glViewport( 0, 0, static_cast<int>(window_width), 
					static_cast<int>(window_height));
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw(scene_geometry, CENTERED);
				break;
			case ACTIVE_STEREO:
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
		glRasterPos2f( -1.0f, -0.96f);
		glColor3f( 1.0f, 1.0f, 1.0f);
		std::string fps_msg( "Cycle time: ");
		fps_msg += boost::lexical_cast<std::string>(fps.read());
		fps_font->Render( fps_msg.c_str());
		
		// Cleanup
		gl_swap_buffers();
		// get_gl_window()->swap_buffers();
		check_gl_error();
		// get_gl_window()->gl_end();
		gl_end();
		fps.stop();
		cycles_since_extent++;
		cycles_since_fps++;
		gcf_changed = false;
		forward_changed = false;
		if (cycles_since_fps >= 20) {
			// std::cout << "cycle time: " << fps.read() << "\n";
			cycles_since_fps = 0;
		}
#else
		
		fps.start();
		view scene_geometry( forward, center, window_width, window_height, 
			forward_changed, gcf, gcf_changed);
		scene_geometry.lod_adjust = lod_adjust;
		gl_begin();
		clear_gl_error();
		// Setup
		// Set up the base modelview and projection matricies
		world_to_view_transform( scene_geometry);
		glViewport( 0, 0, static_cast<int>(window_width), 
			static_cast<int>(window_height));
		// Establish the proper background.
		glClearColor( background.red, background.green, background.blue, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Render all opaque objects in the world space layer
		lights.gl_begin();
		world_iterator i( layer_world.begin());
		world_iterator i_end( layer_world.end());
		while (i != i_end) {
			i->refresh_cache( scene_geometry);
			i->gl_render( scene_geometry);
			++i;
		}
		
		// Perform a depth sort of the transparent world from forward to backward.
		if (layer_world_transparent.size() > 1)
			std::stable_sort( layer_world_transparent.begin(), layer_world_transparent.end(),
				z_comparator( forward));
		
		// Render translucent objects in world space.
		world_trans_iterator j( layer_world_transparent.begin());
		world_trans_iterator j_end( layer_world_transparent.end());
		while (j != j_end) {
			j->refresh_cache( scene_geometry);
			j->gl_render( scene_geometry);
			++j;
		}
		lights.gl_end();
		
		
		// Render objects in the screen layer.
		glLoadIdentity();
		glMatrixMode( GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW);
		screen_iterator k( layer_screen.begin());
		screen_iterator k_end( layer_screen.end());
		while (k != k_end) {
			k->refresh_cache( scene_geometry);
			k->gl_render( scene_geometry);
			++k;
		}
		
		// Cleanup
		gl_swap_buffers();
		// get_gl_window()->swap_buffers();
		check_gl_error();
		// get_gl_window()->gl_end();
		gl_end();
		cycles_since_extent++;
		fps.stop();
		cycles_since_fps++;
		gcf_changed = false;
		forward_changed = false;
		if (cycles_since_fps >= 20) {
			std::cout << "cycle time: " << fps.read() << "\n";
			cycles_since_fps = 0;
		}
#endif
	}
	catch (gl_error e) {
		std::ostringstream msg;
		msg << "OpenGL error: " << e.what() << ", aborting.\n";
		VPYTHON_CRITICAL_ERROR( msg.str());
		std::exit(1);
	}
	return true;
}
