// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	// A series of checks to ensure that the anonymous struct+union black magic
	// used in vector and rgba doesn't change the layout of the classes in 
	// memory.
	assert( sizeof( vector) == sizeof(double) * 3);
	assert( sizeof( rgba) == sizeof(float) * 4);
	
	basic_app main_window( "Arrow scaling test");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	x->set_axis( vector(1e11, 0, 0));
	
	main_window.scene.core.add_renderable( x);
	
	main_window.run();
	return 0;
}
