#include "render_surface.hpp"
#include "arrow.hpp"
#include "ring.hpp"
#include "file_texture.hpp"

#include <iostream>

int 
main( void)
{
	basic_app app( "Ring test");
	rgba yellow( 1, 1, 0);
	rgba trans_green( 0.1, 1, 1, 0.4);
	shared_ptr<arrow> x( new arrow());
	
	shared_ptr<ring> simple( new ring());
	simple->set_pos( vector(2, 0, 0));
	simple->set_axis( vector(1,1,0).norm());
	simple->set_color( yellow);
	simple->set_shininess( 0.5);
	
	app.scene.add_renderable( x);
	app.scene.add_renderable( simple);

	std::cout << "You should see three cones and an arrow.  The arrow is a "
		"measure of one unit length.  The cone on the right is built with "
		"default length and radius, with a shininess of 0.5.  The one on the "
		"left has twice the length and half the radius.  The one on top is "
		"greenish blue, transparent, and points down and to the left.\n";
	app.run();	
}
