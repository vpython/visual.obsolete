// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Arrow scaling test");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	x->set_axis( vector(1e11, 0, 0));
	
	main_window.scene.add_renderable( x);
	
	main_window.run();
	return 0;
}
