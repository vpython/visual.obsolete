// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display.hpp"
#include "sphere.hpp"
#include "box.hpp"

#include <iostream>

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	display app;
	app.set_title( "Ellipsoid test");
	rgba red( 1, 0, 0);
	rgba blue( 0, 0, 1);
	shared_ptr<box> floor( new box());
	floor->set_length(4);
	floor->set_height(0.5);
	floor->set_width( 4);
	floor->set_color( blue);
	app.add_renderable( floor);
	
	shared_ptr<sphere> ball( new sphere());
	ball->set_pos( vector(0,4));
	ball->set_color( red);
	app.add_renderable( ball);
	
	vector velocity(0, -1, 0);
	double dt = 0.000001;
	while (!gtk2_main::allclosed()) {
	    ball->get_pos() += velocity*dt;
	    if (ball->get_y() < 1)
	        velocity.y = -velocity.y;
	    else
	        velocity.y -= 9.8*dt;
	}
	return 0;
}
