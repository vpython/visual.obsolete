#include "render_surface.hpp"
#include "sphere.hpp"
#include "pmap_sphere.hpp"
#include "arrow.hpp"
#include "file_texture.hpp"
#include "vpython-config.h"

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "VPython Rendering Core #2 Demonstration.");
	
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
		"/data/land_ocean_ice_cloud_2048.tif");
	
	// Load up a map of the sky.
	shared_ptr<texture> sky = file_texture::create( VPYTHON_PREFIX
		"/data/stars_polar.jpg");
	
	// Load up the city lights
	shared_ptr<texture> lights = file_texture::create( VPYTHON_PREFIX
		"/data/lights_2048.png");
	
	// Draw one sphere with a map of the earth.
	shared_ptr<sphere> sph( new sphere());
	sph->set_texture( earth);
	sph->set_radius( 1);
	
	shared_ptr<pmap_sphere> stars( new pmap_sphere());
	stars->set_texture( sky);
	stars->set_radius( 1e4);
	stars->set_color( rgba( 0.7, 0.7, 0.7));
	
	// Draw the lights of the earth
	shared_ptr<sphere> ear_night( new sphere());
	ear_night->set_texture( lights);
	ear_night->lit = false;
	ear_night->set_radius( 1.001);
	ear_night->set_color( rgba( 0.45, 0.45, 0.45, 1));
	
	shared_ptr<arrow> arr( new arrow());
	arr->set_pos( pos);
	arr->set_axis( axis);
	arr->set_color( rgba( 1, 1, 1, 0.60));
	
	vector declination( std::cos(13 * M_PI / 180), std::sin(13 * M_PI / 180));
	vector up( -std::cos(77 * M_PI / 180), std::sin(77 * M_PI / 180));
	shared_ptr<light> sun( new light( -declination * 100, rgba(), true));
	const_cast<std::list<shared_ptr<light> >& >(
		main_window.scene.core.get_lights()).clear();
	main_window.scene.core.add_light( sun);
	main_window.scene.core.set_ambient( rgba( 0.1, 0.1, 0.1));
	main_window.scene.core.set_up( up);
	
	main_window.scene.add_renderable( stars);
	main_window.scene.add_renderable( sph);
	// main_window.scene.add_renderable( arr);
	main_window.scene.add_renderable( ear_night);
	
	main_window.run();
	return 0;
}
