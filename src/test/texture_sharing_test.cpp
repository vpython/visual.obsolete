#include "render_surface.hpp"
#include "sphere.hpp"
#include "file_texture.hpp"
#include "vpython-config.h"

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Texture sharing test");
	
	// Load up a map of the earth.
	shared_ptr<texture> earth = file_texture::create( VPYTHON_PREFIX
		"/data/land_ocean_ice_cloud_2048.tif");

	// Draw two textured spheres, each sharing the same texture.
	shared_ptr<sphere> sph( new sphere());
	sph->set_pos( vector(2, 0, 0));
	sph->set_texture( earth);
	
	// To the left and 180 degrees rotated from the first.
	shared_ptr<sphere> sph2( new sphere());
	sph2->set_pos( vector(-2,0,0));
	sph2->set_axis( vector(-1,0,0));
	sph2->set_texture( earth);

	main_window.scene.add_renderable( sph);
	main_window.scene.add_renderable( sph2);

	main_window.run();
	return 0;
}
