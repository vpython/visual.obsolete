// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"
#include "label.hpp"

#include <iostream>

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Basic label test");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	// x->set_axis( vector(1e11, 0, 0));
	
	shared_ptr<label> tip( new label());
	tip->set_text( "tip");
	tip->set_space( 0.2);
	tip->set_pos( vector(1,0,0));
	
	shared_ptr<label> tail( new label());
	tail->set_text( "tail");
	tail->set_xoffset( 20);
	tail->set_yoffset( 10);
	
	shared_ptr<label> midp( new label());
	midp->set_text("a\nmidpoint");
	midp->set_xoffset(10);
	midp->set_yoffset( -20);
	midp->set_pos( vector(.5, 0, 0));
	
	main_window.scene.add_renderable( x);
	main_window.scene.add_renderable( tail);
	main_window.scene.add_renderable( tip);
	main_window.scene.add_renderable( midp);
	
	std::cout << "You should see a single arrow with three labels attached to it.\n"
		"Each label is translucent.  The one at the tip should be centered at \n"
		"the tip.  The head and tail labels have a single line of text.  The \n"
		"midpoint label has two.\n";
	
	main_window.run();
	return 0;
}
