#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/gl/init.h>

#include "gtk2/render_surface.hpp"
#include "sphere.hpp"
#include "gtk2/file_texture.hpp"

int 
main( int, char**)
{
	basic_app main_window( "Texture sharing test");
	
	// Load up a map of the earth.
	shared_ptr<texture> earth = file_texture::create( "/home/jonathan/Projects/"
		"vpython_basic_gtkmm/data/land_ocean_ice_cloud_2048.tif");

	// Draw two textured spheres, each sharing the same texture.
	shared_ptr<sphere> sph( new sphere(2));
	sph->set_pos( vector(2, 0, 0));
	sph->set_texture( earth);
	
	// To the left and 180 degrees rotated from the first.
	shared_ptr<sphere> sph2( new sphere(2));
	sph2->set_pos( vector(-2,0,0));
	sph2->set_axis( vector(-1,0,0));
	sph2->set_texture( earth);

	main_window.scene.add_renderable( sph);
	main_window.scene.add_renderable( sph2);

	main_window.run();
	return 0;
}
