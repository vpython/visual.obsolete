// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "sphere.hpp"
#include "pmap_sphere.hpp"
#include "arrow.hpp"
#include "file_texture.hpp"
#include "vpython-config.h"

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Sphere texture test");
	
	// The coordinates of Chapel Hill, NC.
	double lat = 35.0 + 46.0 / 60.0;
	double lng = -(79.0 + 4.0 / 60.0);
	tmatrix horiz_rot = rotation( lng * G_PI / 180.0, vector(0,1,0));
	// Apply me first.
	tmatrix vert_rot = rotation( lat * G_PI / 180.0, vector(0, 0, 1));
	vector axis( 1, 0, 0);
	axis = horiz_rot * (vert_rot * axis);
	vector pos = 1.2 * axis;
	axis = -axis * 0.2;
	
	// Load up a map of the earth.
	shared_ptr<texture> earth = file_texture::create( VPYTHON_PREFIX
		"/data/land_ocean_ice_2048.tif");
	
	// Load up a map of the sky.
	shared_ptr<texture> sky = file_texture::create( VPYTHON_PREFIX
		"/data/stars_polar.jpg");
	
	// Draw one sphere with a map of the earth.
	shared_ptr<sphere> sph( new sphere());
	sph->set_texture( earth);
	sph->set_radius( 1);
	
	// Draw the celesital sphere.
	shared_ptr<pmap_sphere> stars( new pmap_sphere());
	stars->set_texture( sky);
	stars->set_radius( 1e4);
	

	
	shared_ptr<arrow> arr( new arrow());
	arr->set_pos( pos);
	arr->set_axis( axis);
	arr->set_color( rgba( 1, 1, 1, 0.60));
	
	main_window.scene.core.add_renderable( sph);
	main_window.scene.core.add_renderable( arr);
	main_window.scene.core.add_renderable( stars);
	
	main_window.run();
	return 0;
}
