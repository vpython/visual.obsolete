#include "render_surface.hpp"
#include "convex.hpp"

// Aack!  I just discovered that uniform_real is really broken.
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_01.hpp>
using boost::minstd_rand0;
using boost::uniform_01;

#include <iostream>

int 
realmain( std::vector<std::string>&)
{
	basic_app app( "Convex test");
	
	shared_ptr<convex> triangle( new convex());
	triangle->append( vector(-4, 2.5));
	triangle->append( vector( -5, 2.5));
	triangle->append( vector( -4.5, 3.25));
	triangle->set_color( rgba( 0, 0.75, 0.75));
	
	shared_ptr<convex> circle( new convex());
	circle->set_color( rgba( .75, .75, 0));
	for (double i = 0; i < M_PI*2.0; i += 0.1) {
		circle->append( vector( sin(i), cos(i)+2));
	}
	
	// random_device rng;
	minstd_rand0 rng;
	uniform_01<minstd_rand0> dist(rng);
	
	shared_ptr<convex> sphere( new convex());
	for (int i = 0; i < 1000; ++i) {
		double x0 = dist() * 2.0 - 1.0;
		double x1 = dist() * 2.0 - 1.0;
		double x2 = dist() * 2.0 - 1.0;
		sphere->append( vector(2) + vector(x0, x1, x2).norm());
		// if (i % 100 == 0)
		//	std::cout << x0 << " " << x1 << " " << x2 << std::endl;
	}
	
	app.scene.add_renderable( triangle);
	app.scene.add_renderable( circle);
	app.scene.add_renderable( sphere);
	
	app.run();
	return 0;
}
