#include "gtk2/render_surface.hpp"
#include "arrow.hpp"
#include "box.hpp"
#include "sphere.hpp"
#include "gtk2/file_texture.hpp"
#include "vpython-config.h"

#include <iostream>

int 
main( void)
{
	basic_app app( "Box test");

	shared_ptr<texture> crate = file_texture::create( VPYTHON_PREFIX
		"/data/crate.bmp");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 0.7, 0.7, 0.7));
	
	shared_ptr<box> simple( new box());
	simple->set_pos( vector(-2, 0, 0));
	simple->set_color( rgba( 0.7, 0.7, 0.7));
	
	shared_ptr<box> trans( new box());
	trans->set_color( rgba( 1, 1, 1, 0.5));
	trans->set_pos( vector(2, 0, 0));
	
	shared_ptr<box> texed( new box());
	texed->set_pos( vector(0, 2, 0));
	// texed->set_color( rgba( 1, 0.5, 0.5));
	texed->set_texture( crate);
	
	shared_ptr<box> trans_texed( new box());
	trans_texed->set_pos( vector( 0, -2, 0));
	trans_texed->set_color( rgba( 1, 1, 1, 0.5));
	trans_texed->set_texture( crate);
	
	shared_ptr<sphere> hidden_sphere( new sphere());
	hidden_sphere->set_pos( vector( 0, -2, 0));
	hidden_sphere->set_radius( 0.4);
	
	app.scene.add_renderable( x);
	app.scene.add_renderable( simple);
	app.scene.add_renderable( trans);
	app.scene.add_renderable( texed);
	app.scene.add_renderable( trans_texed);
	app.scene.add_renderable( hidden_sphere);
	app.scene.core.background = rgba();

	std::cout << "You should see four boxes and an arrow.  The box on the right "
		"should be opaque and have the same length, width, and height, all "
		"equal in size to the length of the arrow.  The box on the left should "
		"look exactly like the one on the right, except that it is transparent."
		"  The box on the top should look like a wooden crate.  The one on the"
		" bottom should look like a translucent wooden crate.\n";
	
	app.run();
	return 0;
}
