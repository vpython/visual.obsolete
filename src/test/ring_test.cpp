// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"
#include "ring.hpp"

#include <iostream>

using namespace cvisual;

int 
realmain( std::vector<std::string>&)
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
	
	app.scene.core.add_renderable( x);
	app.scene.core.add_renderable( simple);

	// std::cout << "";
	app.run();
	return 0;
}
