// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"
#include "frame.hpp"

#include <iostream>

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Transparent arrow test");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));

	shared_ptr<arrow> z( new arrow());
	z->set_color( rgba( 0, 0, 1));
	z->set_axis( vector( 0, 0, 1));

	shared_ptr<arrow> y( new arrow());
	y->set_color( rgba( 0, 1, 0));
	y->set_axis( vector(0, 1, 0));
	y->set_up( vector(-1, 0, 0));
	
	shared_ptr<arrow> net( new arrow());
	net->set_axis( vector(1, 1, 1));
	net->set_color( rgba( 1, 1, 1, 0.6));
	
	shared_ptr<arrow> net2( new arrow());
	net2->set_axis( vector(-1, -1, -1));
	net2->set_color( rgba( 1, 1, 1, 0.6));
	
	// For my next trick...
	shared_ptr<frame> repeated( new frame());
	repeated->set_axis( vector( -1, 0, 0));
	repeated->set_up( vector( 0, -1, 0));
	repeated->set_pos( vector( 1, 1, 1));
	repeated->set_scale( vector( 1, 1, -1));
	// repeated->add_child( net2);
	repeated->add_renderable( x);
	repeated->add_renderable( y);
	repeated->add_renderable( z);
	
	main_window.scene.core.add_renderable( y);
	main_window.scene.core.add_renderable( x);
	main_window.scene.core.add_renderable( z);
	main_window.scene.core.add_renderable( net);
	main_window.scene.core.add_renderable( repeated);

	std::cout << "You should see four arrow objects.  A red one along +x,"
		" a green one along +y, a blue one along +z, and a translucent one"
		" pointing along <1,1,1>.\n";
	main_window.run();
	return 0;
}
