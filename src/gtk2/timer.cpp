
#include "util/timer.hpp"

#include <sys/time.h>
#include <time.h>

#include <cmath>
#include <numeric>


timer::timer()
	: last_start(0)
{
}

void
timer::lap_start()
{
	timeval t;
	timerclear(&t);
	gettimeofday( &t, NULL);
	last_start = static_cast<double>(t.tv_sec) 
		+ static_cast<double>(t.tv_usec) * 1.0e-6;
}

void
timer::lap_stop()
{
	timeval t;
	timerclear(&t);
	gettimeofday( &t, NULL);
	double delta_time = static_cast<double>(t.tv_sec) 
		+ static_cast<double>(t.tv_usec) * 1.0e-6
		- last_start;
	times.push_back( delta_time);
}

void
timer::reset()
{
	times.clear();
	last_start = 0.0;
}

double
timer::average() const
{
	return std::accumulate( times.begin(), times.end(), 0.0) 
		/ times.size();
}

double
timer::std_deviation() const
{
	double avg = average();
	double ret = 0.0;
	for ( std::vector<double>::const_iterator i = times.begin(); i != times.end(); ++i) {
		double dev = *i - avg;
		ret += dev*dev;
	}
	return std::sqrt( ret);

}

std::pair<double, double> 
timer::confidence_interval() const
{
	double avg = std::accumulate( times.begin(), times.end(), 0.0) 
		/ times.size();
	
	double stddev = 0.0;
	for ( std::vector<double>::const_iterator i = times.begin(); i != times.end(); ++i) {
		double dev = *i - avg;
		stddev += dev*dev;
	}
	stddev = std::sqrt( stddev);
	
	double delta = 1.96 * stddev / std::sqrt( static_cast<double>(times.size()));
	return std::make_pair( avg,	delta);
}

// The histeresis fraction.  By default, 90% old data, 10% new data.
namespace {
	const double old_fraction = 0.90;
}

hist_timer::hist_timer()
	: cumulative(0)
{
}

hist_timer::hist_timer( double init)
	: cumulative( init)
{
}

void
hist_timer::start()
{
	timeval t;
	timerclear(&t);
	gettimeofday( &t, NULL);
	last_start = static_cast<double>(t.tv_sec) 
		+ static_cast<double>(t.tv_usec) * 1e-6;
}

void
hist_timer::stop()
{
	timeval t;
	timerclear(&t);
	gettimeofday( &t, NULL);
	double delta_time = static_cast<double>(t.tv_sec) 
		+ static_cast<double>(t.tv_usec) * 1e-6
		- last_start;
	cumulative = cumulative * old_fraction + delta_time * (1.0 - old_fraction);
}

double
hist_timer::read() const
{
	return cumulative;
}
