// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"
#include "cylinder.hpp"

#include <iostream>

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	basic_app app( "Cone test");
	rgba yellow( 1, 1, 0);
	rgba trans_green( 0.1, 1, 1, 0.4);
	shared_ptr<arrow> x( new arrow());
	
	shared_ptr<cylinder> simple( new cylinder());
	simple->set_pos( vector(2, 0, 0));
	simple->set_color( yellow);
	simple->set_shininess( 0.5);
	
	shared_ptr<cylinder> longer( new cylinder());
	longer->set_pos( vector(-2, 0, 0));
	longer->set_axis( vector( 0, 2, 0));
	longer->set_up( vector(-1, 0, 0));
	longer->set_radius( .5);
	longer->set_color( yellow);
	
	shared_ptr<cylinder> green( new cylinder());
	green->set_color( trans_green);
	green->set_pos( vector(0, 2, 0));
	green->set_axis( vector( -1, -1, 0));
	green->set_radius( 0.75);
	green->set_shininess( 0.8);
	
	app.scene.add_renderable( x);
	app.scene.add_renderable( simple);
	app.scene.add_renderable( longer);
	app.scene.add_renderable( green);

	std::cout << "You should see three cylinders and an arrow.  The arrow is a "
		"measure of one unit length.  The cone on the right is built with "
		"default length and radius, with a shininess of 0.5.  The one on the "
		"left has twice the length and half the radius.  The one on top is "
		"greenish blue, transparent, and points down and to the left.\n";
	app.run();
	return 0;
}
