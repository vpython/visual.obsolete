// Copyright (c) 2004 by Jonathan Brandmeyer.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "render_surface.hpp"
#include "arrow.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "ring.hpp"
#include "pyramid.hpp"
#include "box.hpp"
#include "ellipsoid.hpp"
#include "cone.hpp"
#include "curve.hpp"
#include "convex.hpp"

#include <iostream>

using namespace cvisual;

void
on_object_clicked( shared_ptr<renderable> body)
{
	std::cout << "Body centered at " << body->get_center() << " was clicked.\n";
}

int 
realmain( std::vector<std::string>&)
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
	
	shared_ptr<curve> thin( new curve());
	shared_ptr<curve> thick( new curve());
	thick->set_radius(0.05);
	vector v_base( -10, 0, 0);
	vector v_gain(0, 0.02, 0);
	vector v_0( 2, 0, 0);
	double delta_angle = .1;
	
	tmatrix rotator = rotation( delta_angle, vector( 0, 1, 0));
	for (int i = 0; i < 258; ++i) {
		v_0 = rotator * v_0;
		thin->append( v_base + v_0 + v_gain*i, rgb(0,1,0));
		thick->append( v_base + v_0 - v_gain*i, rgb(0,0,1));
	}
	
	shared_ptr<convex> triangle( new convex());
	triangle->append( vector(15+-4, 2.5));
	triangle->append( vector( 15+-5, 2.5));
	triangle->append( vector( 15+-4.5, 3.25));
	triangle->set_color( rgba( 0, 0.75, 0.75));
	
	app.scene.core.add_renderable( arr);
	app.scene.core.add_renderable( cyl);
	app.scene.core.add_renderable( sph);
	app.scene.core.add_renderable( con);
	app.scene.core.add_renderable( pyr);
	app.scene.core.add_renderable( rin);
	app.scene.core.add_renderable( b);
	app.scene.core.add_renderable( ell);
	app.scene.core.add_renderable( thin);
	app.scene.core.add_renderable( thick);
	app.scene.core.add_renderable( triangle);
	app.scene.object_clicked.connect( SigC::slot( on_object_clicked));

	app.run();
	return 0;
}
