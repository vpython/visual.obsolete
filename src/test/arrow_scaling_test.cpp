#include <iostream>
#include <limits>

#include "gtk2/render_surface.hpp"
#include "arrow.hpp"

int 
main( int, char**)
{
	basic_app main_window( "Arrow scaling test");
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	x->set_axis( vector(1e11, 0, 0));
	
	main_window.scene.add_renderable( x);
	
	main_window.run();
	return 0;
}
