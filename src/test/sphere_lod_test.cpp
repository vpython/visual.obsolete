#include "render_surface.hpp"
#include "sphere.hpp"

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Sphere level of detail test");
	
	// Draw 4 spheres with each level of detail.
	shared_ptr<sphere> sph( new sphere());
	sph->set_pos( vector(1, -1, 0) * 2);
	sph->set_color( rgba( 1, 1, 0));
	sph->set_shininess( 0.99);
	shared_ptr<sphere> sph2( new sphere());
	sph2->set_pos( vector(1, 1, 0) * 2);
	sph2->set_color( rgba( 1, 1, 0));
	sph2->set_shininess( 0.7);
	shared_ptr<sphere> sph3( new sphere());
	sph3->set_pos( vector(-1, 1, 0) * 2);
	sph3->set_color( rgba( 1, 1, 0));
	sph3->set_shininess( 0.5);
	shared_ptr<sphere> sph4( new sphere());
	sph4->set_pos( vector(-1, -1, 0) * 2);
	sph4->set_color( rgba(1, 1, 0));
	sph4->set_shininess( 0.2);
	shared_ptr<sphere> sph5( new sphere());
	sph5->set_pos( vector(-1, -1, 0) * 4);
	sph5->set_color( rgba(1, 1, 0));
	shared_ptr<sphere> sph6( new sphere());
	sph6->set_pos( vector(-1, -1, 0) * 6);
	sph6->set_color( rgba(1, 1, 0));
	
	main_window.scene.add_renderable( sph6);
	main_window.scene.add_renderable( sph2);
	main_window.scene.add_renderable( sph3);
	main_window.scene.add_renderable( sph4);
	main_window.scene.add_renderable( sph5);
	main_window.scene.add_renderable( sph);
	
	main_window.run();
}
