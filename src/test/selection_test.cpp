#include "gtk2/render_surface.hpp"
#include "arrow.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "ring.hpp"
#include "pyramid.hpp"
#include "box.hpp"
#include "ellipsoid.hpp"
#include "cone.hpp"

#include <iostream>

void
on_object_clicked( shared_ptr<renderable> body)
{
	std::cout << "Body centered at " << body->get_center() << " was clicked.\n";
}

int 
main( void)
{
	basic_app app( "Object selection test");
	rgba yellow( 1, 1, 0);
	rgba trans_green( 0.1, 1, 1, 0.4);
	rgba green( 0, 1, 0);
	rgba blue( 0, 0, 1);
	rgba aqua( 0, 1, 1);
	rgba red( 1, 0, 0);
	rgba purple( 1, 0, 1);
	rgba white(1,1,1);
	
	shared_ptr<arrow> arr( new arrow());
	arr->set_color( white);
	
	shared_ptr<cylinder> cyl( new cylinder());
	cyl->set_pos( vector(2, 0, 0));
	cyl->set_color( yellow);
	
	shared_ptr<sphere> sph( new sphere());
	sph->set_pos( vector( 4, 0, 0));
	sph->set_color( red);
	
	shared_ptr<cone> con( new cone());
	con->set_pos( vector(0, -4, 0));
	con->set_color( purple);
	con->set_axis( vector(-1,1,1));
	con->set_radius( 0.5);
	
	shared_ptr<pyramid> pyr( new pyramid());
	pyr->set_pos( vector( -2, 0, 0));
	pyr->set_width( 0.5);
	pyr->set_height( 2.0);
	pyr->set_color( green);
	
	shared_ptr<ring> rin( new ring());
	rin->set_pos( vector( 0, 4, 0));
	rin->set_radius( 3);
	rin->set_up( vector(-1,0,0));
	rin->set_axis( vector(0,1,0));
	rin->set_color( aqua);
	
	shared_ptr<box> b( new box());
	b->set_pos( vector( 0, 0, 4));
	b->set_color( blue);
	
	shared_ptr<ellipsoid> ell( new ellipsoid());
	ell->set_pos( vector( 0, 0, -4));
	ell->set_color( trans_green);
	
	app.scene.add_renderable( arr);
	app.scene.add_renderable( cyl);
	app.scene.add_renderable( sph);
	app.scene.add_renderable( con);
	app.scene.add_renderable( pyr);
	app.scene.add_renderable( rin);
	app.scene.add_renderable( b);
	app.scene.add_renderable( ell);
	app.scene.object_clicked.connect( SigC::slot( on_object_clicked));

	app.run();	
}
