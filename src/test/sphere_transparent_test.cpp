#include <iostream>
#include <limits>

#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/gl/init.h>

#include "gtk2/render_surface.hpp"
#include "sphere.hpp"

int 
main( int, char**)
{
	basic_app main_window( "Sphere transparency test");
	
	// Draw two yellow spheres, one transparent.
	shared_ptr<sphere> sph( new sphere(4));
	sph->set_pos( vector(1, -1, 0) * 2);
	sph->set_color( rgba( 0.2, 0.2, 1, 0.3));
	
	shared_ptr<sphere> sph2( new sphere(4));
	sph2->set_pos( vector(1, 1, 0) * 2);
	sph2->set_radius( 0.95);
	sph2->set_color( rgba( 0.2, 0.2, 1));

	main_window.scene.add_renderable( sph);
	main_window.scene.add_renderable( sph2);

	main_window.run();
	return 0;
}
