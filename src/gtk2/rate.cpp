#include "util/rate.hpp"

#include <sys/time.h>
#include <time.h>
#include <math.h>

#include <stdexcept>

namespace cvisual {
namespace {

// Parts of the rate_timer common to both the OSX and Linux implementations.
class rate_timer
{
 private:
	timeval origin;

 public:
	rate_timer()
	{
		timerclear( &origin);
		gettimeofday( &origin, 0);
	}
	
	void delay( double);
};

void
rate_timer::delay( double _delay)
{
	timeval now;
	timerclear(&now);
	gettimeofday( &now, 0);
	
	// Convert the requested delay into units of the clock.
	timeval delay = {
		(long)_delay,
		(long)((_delay - (long)_delay) * 1e6)
	};
	
	// The amount of time to wait.
	timespec wait = { 
		delay.tv_sec - (now.tv_sec - origin.tv_sec),
		(delay.tv_usec - (now.tv_usec - origin.tv_usec)) * 1000
	};
	if (wait.tv_sec < 0 || wait.tv_nsec < 0) {
		gettimeofday( &origin, 0);
		return;
	}
	// The amout of time remaining when nanosleep returns
	timespec remaining = { 0, 0};
#ifdef __APPLE__ // OSX.
	// OSX's nanosleep() is very accurate :)
	nanosleep( &wait, &remaining);
#else
	// Computation of the requested delay is the same, but the execution differs
	// from OSX.  This is to provide somewhat more accurate timing on Linux, 
	// whose timing is abominable.
	wait.tv_nsec -= 10000000;
	if (wait.tv_nsec > 0) {
		nanosleep( &wait, &remaining);
	}

	// Busy wait out the remainder of the time.
	gettimeofday( &now, 0);
	wait.tv_sec = delay.tv_sec - (now.tv_sec - origin.tv_sec);
	wait.tv_nsec = (delay.tv_usec - (now.tv_usec - origin.tv_usec));
	while (wait.tv_sec >= 0 && wait.tv_nsec > 0) {
		gettimeofday( &now, 0);
		wait.tv_sec = delay.tv_sec - (now.tv_sec - origin.tv_sec);
		wait.tv_nsec = (delay.tv_usec - (now.tv_usec - origin.tv_usec));
	}
#endif
	gettimeofday( &origin, 0);
}

} // !namespace (unnamed)

void 
rate( const double& freq)
{
	static rate_timer* rt = 0;
	if (!rt)
		rt = new rate_timer();
	
	if (freq <= 0.0)
		throw std::invalid_argument( "Rate must be positive and nonzero.");
	rt->delay( 1.0/freq );
}

} // !namespace cvisual
