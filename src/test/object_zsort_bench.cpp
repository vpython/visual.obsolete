
#include <boost/nondet_random.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <list>

#include "sphere.hpp"
#include "util/timer.hpp"
#include "util/vector.hpp"

using namespace boost;

// Based on runs of this program with various universe sizes, it is my conclusion
// that we should use stable_sort().  It has very low variablility in its
// performance across the entire range of universe sizes, while maintaining 
// good performance.  Using raw pointers vice shared_ptr imprives performance
// by 2-4x.

// The following results are for runs on an Intel PIII @ 800 MHz on Debian
// Sid when compiled with GNU G++ 3.4 -O2

// For a 10-element universe:
// stable_sort() speed 95% confidence interval:  (1.56875e-05 +/- 1.42787e-05)
//         std deviation: 0.000103026
// sort() speed 95% confidence interval:         (0.000648145 +/- 0.0175939)
//          std deviation: 0.126947
// partial_sort() speed 95% confidence interval: (2.42251e-05 +/- 8.43723e-05)
//          std deviation: 0.000608778
// set recreate speed 95% confidence interval:   (2.04133e-05 +/- 7.12576e-05)
//         std deviation: 0.00051415

// For a 50-element universe:
// stable_sort() speed 95% confidence interval:  (0.000127183 +/- 0.000190856)
//          std deviation: 0.0013771
// sort() speed 95% confidence interval:         (0.000761354 +/- 0.0175411)
//          std deviation: 0.126566
// partial_sort() speed 95% confidence interval: (0.000802236 +/- 0.0176203)
//          std deviation: 0.127137
// set recreate speed 95% confidence interval:   (0.000135654 +/- 0.000117537)
//         std deviation: 0.000848073

// For a 100-element universe:
// stable_sort() speed 95% confidence interval:  (0.000303304 +/- 0.000193099)
//          std deviation: 0.00139328
// sort() speed 95% confidence interval:         (0.000316737 +/- 0.000323565)
//          std deviation: 0.00233464
// partial_sort() speed 95% confidence interval: (0.00101631 +/- 0.0173155)
//          std deviation: 0.124938
// set recreate speed 95% confidence interval:   (0.000319299 +/- 0.000227813)
//         std deviation: 0.00164376

// For a 1000-element universe:
// stable_sort() speed 95% confidence interval:  (0.00526668 +/- 0.00174511)
//          std deviation: 0.0125916
// sort() speed 95% confidence interval:         (0.00567024 +/- 0.0169863)
//          std deviation: 0.122562
// partial_sort() speed 95% confidence interval: (0.00605227 +/- 0.0169822)
//          std deviation: 0.122533
// set recreate speed 95% confidence interval:   (0.00476033 +/- 0.00132668)
//         std deviation: 0.0095725


int
main( int argc, char* argv[])
{
	size_t test_size = 100;
	size_t test_iterations = 200;
	if (argc >= 2)
		test_size = lexical_cast<size_t>( argv[1]);
	if (argc >= 3)
		test_iterations = lexical_cast<size_t>( argv[2]);
	
	std::cout << "Performing " << test_iterations << " resorts each with " 
		<< test_size << " elements\n";
	
	// A source of entropy that never runs dry.
	random_device rng;
	uniform_real<> dist( -100.0, 100.0);
	
	{
		// Create the universe of objects.
		size_t this_test_size = test_size;
		std::vector<shared_ptr<renderable> > universe;
		while (this_test_size != 0) {
			sphere* n_sphere( new sphere());
			n_sphere->set_pos( vector( dist(rng), dist(rng), dist(rng)));
			universe.push_back( shared_ptr<renderable>(n_sphere));
			--this_test_size;
		}
		
		
		// First test - perform stable_sort of a vector in-place.
		size_t test = test_iterations;
		timer times;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::stable_sort( universe.begin(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		std::pair<double, double> interval = times.confidence_interval();
		std::cout << "stable_sort() speed 95% confidence interval:  (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Second test, perform std::sort() of the vector in-place
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::sort( universe.begin(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		interval = times.confidence_interval();
		std::cout << "sort() speed 95% confidence interval:         (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Third test, perform std::partial_sort() of the vector in-place.
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::partial_sort( universe.begin(), universe.end(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		interval = times.confidence_interval();
		std::cout << "partial_sort() speed 95% confidence interval: (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Fourth test, perform recreation of a std::set based on new sorting
		// criteria, followed by a swap.
		std::set<shared_ptr<renderable>, z_comparator> universe2( 
			universe.begin(), universe.end(), z_comparator( vector(0, 0, -1)));
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::set< shared_ptr<renderable>, z_comparator> tmp(
				universe2.begin(), universe2.end(), cmp);
			std::swap( tmp, universe2);
			times.lap_stop();
			--test;		
		}
		interval = times.confidence_interval();
		std::cout << "set recreate speed 95% confidence interval:   (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Fifth test: perform in-place sorting of a std::list with its built-in
		// sorting function.
		std::list< shared_ptr<renderable> > universe3( 
			universe.begin(), universe.end());
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			universe3.sort( cmp);
			times.lap_stop();
			--test;		
		}
		interval = times.confidence_interval();
		std::cout << "list::sort() speed 95% confidence interval:   (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
	}
	std::cout << "\n\nRepeating tests with raw pointers.\n";
	{
		std::size_t this_test_size = test_size;
		// Create the universe of objects.
		std::vector<renderable*> universe;
		while (this_test_size != 0) {
			sphere* n_sphere( new sphere());
			n_sphere->set_pos( vector( dist(rng), dist(rng), dist(rng)));
			universe.push_back( n_sphere);
			--this_test_size;
		}
		
		
		// First test - perform stable_sort of a vector in-place.
		size_t test = test_iterations;
		timer times;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::stable_sort( universe.begin(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		std::pair<double, double> interval = times.confidence_interval();
		std::cout << "stable_sort() speed 95% confidence interval:  (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Second test, perform std::sort() of the vector in-place
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::sort( universe.begin(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		interval = times.confidence_interval();
		std::cout << "sort() speed 95% confidence interval:         (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Third test, perform std::partial_sort() of the vector in-place.
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::partial_sort( universe.begin(), universe.end(), universe.end(), cmp);
			times.lap_stop();
			--test;
		}
		interval = times.confidence_interval();
		std::cout << "partial_sort() speed 95% confidence interval: (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Fourth test, perform recreation of a std::set based on new sorting
		// criteria, followed by a swap.
		std::set<renderable*, z_comparator> universe2( 
			universe.begin(), universe.end(), z_comparator( vector(0, 0, -1)));
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			std::set<renderable*, z_comparator> tmp(
				universe2.begin(), universe2.end(), cmp);
			std::swap( tmp, universe2);
			times.lap_stop();
			--test;		
		}
		interval = times.confidence_interval();
		std::cout << "set recreate speed 95% confidence interval:   (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
		
		// Fifth test: perform in-place sorting of a std::list with its built-in
		// sorting function.
		std::list<renderable*> universe3( 
			universe.begin(), universe.end());
		test = test_iterations;
		while (test != 0) {
			vector forward( dist(rng), dist(rng), dist(rng));
			z_comparator cmp( forward.norm());
			times.lap_start();
			universe3.sort( cmp);
			times.lap_stop();
			--test;		
		}
		interval = times.confidence_interval();
		std::cout << "list::sort() speed 95% confidence interval:   (";
		std::cout << interval.first << " +/- " << interval.second << ")\n";
		std::cout << "\t std deviation: " << times.std_deviation() << "\n";
		times.reset();
	}
	
	return 0;
}
