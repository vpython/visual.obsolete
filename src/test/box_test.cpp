#include <iostream>
#include <limits>

#include "gtk2/render_surface.hpp"
#include "arrow.hpp"
#include "box.hpp"
#include "gtk2/file_texture.hpp"

int 
main( int, char**)
{
	basic_app app( "Box test");
	
	shared_ptr<texture> crate = file_texture::create( "/home/jonathan/Projects/"
		"vpython_basic_gtkmm/data/crate.bmp");
	
	shared_ptr<arrow> x( new arrow());
	
	shared_ptr<box> simple( new box());
	simple->set_pos( vector(-2, 0, 0));
	
	shared_ptr<box> trans( new box());
	trans->set_color( rgba( 1, 1, 1, 0.5));
	trans->set_pos( vector(2, 0, 0));
	
	shared_ptr<box> texed( new box());
	texed->set_pos( vector(0, 2, 0));
	texed->set_color( rgba( 1, 0.3, 0.3));
	texed->set_texture( crate);
	
	app.scene.add_renderable( x);
	app.scene.add_renderable( simple);
	app.scene.add_renderable( trans);
	app.scene.add_renderable( texed);

	std::cout << "You should see two boxes and an arrow.  The box on the right "
		"should be opaque and have the same length, width, and height, all "
		"equal in size to the length of the arrow.  The box on the left should "
		"look exactly like the one on the right, except that it is transparent."
		"  The box on the top should look like a wooden crate.\n";
	
	app.run();	
}
