#include "render_surface.hpp"
#include "arrow.hpp"
#include "ellipsoid.hpp"

#include <iostream>

int 
main( void)
{
	basic_app app( "Ellipsoid test");
	rgba yellow( 1, 1, 0);
	rgba trans_green( 0.1, 1, 1, 0.4);
	shared_ptr<arrow> x( new arrow());
	
	shared_ptr<ellipsoid> simple( new ellipsoid());
	simple->set_pos( vector(2, 0, 0));
	simple->set_color( yellow);
	simple->set_shininess( 0.8);
	
	shared_ptr<ellipsoid> longer( new ellipsoid());
	longer->set_pos( vector(-2, 0, 0));
	longer->set_axis( vector( 0, 2, 0));
	longer->set_up( vector(-1, 0, 0));
	longer->set_width( 0.5);
	longer->set_height( 0.5);
	longer->set_shininess( 0.8);
	
	shared_ptr<ellipsoid> green( new ellipsoid());
	green->set_color( trans_green);
	green->set_pos( vector(0, 2, 0));
	green->set_axis( vector( -1, -1, 0));
	green->set_width( 0.75);
	green->set_height( 0.6);
	green->set_shininess( 0.99);
	
	app.scene.add_renderable( x);
	app.scene.add_renderable( simple);
	app.scene.add_renderable( longer);
	app.scene.add_renderable( green);

	std::cout << "You should see three ellipsoids and an arrow.  The arrow is a "
		"measure of one unit length.  The cone on the right is built with "
		"default length and radius, with a shininess of 0.5.  The one on the "
		"left has twice the length and half the radius.  The one on top is "
		"greenish blue, transparent, and points down and to the left.\n";
	app.run();	
}
