#include "render_surface.hpp"
#include "file_texture.hpp"
#include "sphere.hpp"

int 
main( void)
{
	basic_app main_window( "Sphere transparency test");
	
	// Draw two spheres, one translucent.
	shared_ptr<sphere> sph( new sphere());
	sph->set_pos( vector(1, -1, 0) * 2);
	sph->set_color( rgba( 1, 1, 1, 0.3));
	
	shared_ptr<sphere> sph2( new sphere());
	sph2->set_pos( vector(1, 1, 0) * 2);
	sph2->set_radius( 0.95);
	sph2->set_color( rgba( 0.2, 0.2, 1));

	main_window.scene.add_renderable( sph);
	main_window.scene.add_renderable( sph2);
	main_window.scene.core.background = rgba();

	main_window.run();
	return 0;
}
