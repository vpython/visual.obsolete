#include <iostream>
#include <math.h>

#include "util/timer.hpp"
#include "util/tmatrix.hpp"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

// The purpose of this bench test is to determine the relative speed of 
// performing a vector rotation by computing sin and cos or by computing a 
// matrix-vector multiply.  

// Results:
// On a 1.42 GHz PowerPC G4, the sin/cos rotation is performed in .29 usec per 
// rotation, and the matrix multiply is performed in about .08 usec per 
// rotation.  Switching to using single precision floats for the sin and cos
// operations slows it down to .35 usec per rotation.
// Adding -mcpu=7450 reduces the times to .205 usec for 
// sin/cos rotation and .048 usec for matrix multiply.

// On an 800 MHz Pentium 3, sin/cos rotation is performed in .29 usec and the
// matrix multiply is performed in about .06 usec per rotation.  Using the
// GNU extension sincos improves the sin/cos rotation to .22 usec.  Using
// single-precision floating point results in no change in performance.  Using
// mfpmath=sse does not change performance.

// On a 3.0 GHz Pentium 4, sin/cos rotation is performed in .09 usec and matrix
// multiply in .025 usec.  -mfpmath and -msse2 slightly reduced the matrix multiply
// speed to .023 usec.
int
main( int argc, char** argv)
{
	// Extract iteration and repeat times.
	size_t iterations = 100;
	size_t n_turns = 1000;
	if (argc >= 2)
		iterations = lexical_cast<size_t>( argv[1]);
	if (argc >= 3)
		n_turns = lexical_cast<size_t>( argv[2]);
	std::cout << "Performing " << iterations << " test runs of " << n_turns
		<< " vector rotations each.\n";

	
	timer sincos_times;
	timer matrix_times;
	const float delta_angle = M_PI * 0.02;
	tmatrix rotator = rotation( delta_angle, vector( 1,1,0).norm());
	
	while (iterations) {
		size_t test_turns = n_turns;
		vector i(1, 0, 0);
		float angle = 0.0, val_sin, val_cos;
		
		sincos_times.lap_start();
		while (test_turns) {
			angle += delta_angle;
			sincosf( angle, &val_sin, &val_cos);
			i = vector( val_cos, val_sin);
			--test_turns;
		}
		sincos_times.lap_stop();
		
		i = vector(1,0,0);
		test_turns = n_turns;
		matrix_times.lap_start();
		while (test_turns) {
			i = rotator * i;
			--test_turns;
		}
		matrix_times.lap_stop();
		iterations--;
	}
	
	std::pair<double, double> interval = sincos_times.confidence_interval();
	std::cout << "std::sin()/std::cos() speed 95% confidence interval: (";
	std::cout << interval.first << " +/- " << interval.second << ")\n";
	std::cout << "\t std deviation: " << sincos_times.std_deviation() << "\n\n";

	interval = matrix_times.confidence_interval();
	std::cout << "matrix multiply speed 95% confidence interval:       (";
	std::cout << interval.first << " +/- " << interval.second << ")\n";
	std::cout << "\t std deviation: " << matrix_times.std_deviation() << "\n";


	return 0;
		
}
