#include "util/sorted_model.hpp"
#include "util/timer.hpp"

#include <boost/nondet_random.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/uniform_real.hpp>

#include <iostream>
#include <algorithm>
#include <vector>

using namespace boost;

// On average, on a PIII 800, it takes 23 usec to sort each arrow.
// A 1.43 GHz PowerPC G4 takes 14 usec per arrow model.
int
realmain( std::vector<std::string>& args)
{
	size_t test_iterations = 20;
	if (args.size() >= 1)
		test_iterations = lexical_cast<size_t>( args[0]);
	// At this size, the bench test essentially reports the time to sort per 
	// arrow in milliseconds
	size_t n_arrows = 1000;
	if (args.size() >= 2)
		n_arrows = lexical_cast<size_t>( args[1]);
	
	std::cout << "Performing " << test_iterations << " re-sorts each with " 
		<< n_arrows << " arrows\n";
	
	// Note that this isn't quite the same geometry as what is being used by
	// the actual arrow model.
	z_sorted_model<triangle, 22> arrow_model;
	const double sw = .05;
	const double sl = .7;
	const double hw = 2 * sw;
	// Left triangle
	arrow_model.faces[0] = triangle( 
		vector(0, sw, sw), vector(0, sw, -sw), vector(0, -sw, -sw));
	arrow_model.faces[1] = triangle(
		vector(0, -sw, -sw), vector(0, -sw, sw), vector(0, sw, sw));
	// Front triangle
	arrow_model.faces[2] = triangle( 
		vector(0, sw, sw), vector(0, -sw, sw), vector(sl, sw, sw));
	arrow_model.faces[3] = triangle(
		vector(sl, sw, sw), vector(sl, -sw, sw), vector(0, sw, sw));
	// Back triangle
	arrow_model.faces[4] = triangle( 
		vector(0, sw, -sw), vector(sl, sw, -sw), vector(0, -sw, -sw));
	arrow_model.faces[5] = triangle(
		vector(sl, sw, -sw), vector(0, sw, -sw), vector(sl, -sw, -sw));
	// Top Face
	arrow_model.faces[6] = triangle(
		vector(0, sw, sw), vector(sl, sw, sw), vector(sl, sw, -sw));
	arrow_model.faces[7] = triangle(
		vector(sl, sw, -sw), vector(0, sw, -sw), vector(0, sw, sw));
	// Bottom triangle
	arrow_model.faces[8] = triangle(
		vector(0, -sw, sw), vector(sl, -sw, -sw), vector(sl, -sw, sw));
	arrow_model.faces[9] = triangle(
		vector(sl, -sw, -sw), vector(0, -sw, sw), vector(0, -sw, -sw));
	// Arrowhead back
	arrow_model.faces[10] = triangle(
		vector(sl, -sw, sw), vector(sl, -hw, hw), vector(sl, sw, sw));
	arrow_model.faces[11] = triangle(
		vector(sl, sw, sw), vector(sl, -hw, hw), vector(sl, hw, hw));
	arrow_model.faces[12] = triangle(
		vector(sl, hw, hw), vector(sl, sw, -sw), vector(sl, sw, sw));
	arrow_model.faces[13] = triangle(
		vector(sl, hw, hw), vector(sl, hw, -hw), vector(sl, sw, -sw));
	arrow_model.faces[14] = triangle(
		vector(sl, sw, -sw), vector(sl, hw, -hw), vector(sl, -hw, -hw));
	arrow_model.faces[15] = triangle(
		vector(sl, -hw, -hw), vector(sl, sw, -sw), vector(sl, -sw, -sw));
	arrow_model.faces[16] = triangle(
		vector(sl, -hw, -hw), vector(sl, -sw, sw), vector(sl, -hw, -hw));
	arrow_model.faces[17] = triangle(
		vector(sl, -hw, -hw), vector(sl, -hw, hw), vector(sl, -sw, sw));
	// Arrowhead tip.
	arrow_model.faces[18] = triangle( // TOP
		vector(1.0, 0, 0), vector(sl, hw, -hw), vector(sl, hw, hw));
	arrow_model.faces[19] = triangle( // FRONT
		vector(1.0, 0, 0), vector(sl, hw, hw), vector(sl, -hw, hw));
	arrow_model.faces[20] = triangle( // BACK
		vector(1.0, 0, 0), vector(sl, -hw, hw), vector(sl, -hw, -hw));
	arrow_model.faces[21] = triangle( // BOTTOM
		vector(1.0, 0, 0), vector(sl, -hw, -hw), vector(sl, -hw, hw));
	
	// The number of arrow's to be resorting per change in "forward".
	std::vector<z_sorted_model<triangle, 22> > world( n_arrows, arrow_model);
	// A source of entropy that never runs dry.
	random_device rng;
	uniform_real<> dist( -100.0, 100.0);
	
	// First test - perform stable_sort of a vector in-place.
	size_t test = test_iterations;
	timer times;
	while (test != 0) {
		vector forward(dist(rng), dist(rng), dist(rng));
		forward = forward.norm();
		std::vector<z_sorted_model<triangle, 22> >::iterator i = world.begin();
		std::vector<z_sorted_model<triangle, 22> >::iterator end = world.end();
		times.lap_start();
		while (i != end) {
			i->sort(forward);
			++i;
		}
		times.lap_stop();
		--test;
	}
	std::pair<double, double> interval = times.confidence_interval();
	std::cout << "stable_sort() speed 95% confidence interval:  (";
	std::cout << interval.first << " +/- " << interval.second << ")\n";
	std::cout << "\t std deviation: " << times.std_deviation() << "\n";
	times.reset();

	return 0;
}
