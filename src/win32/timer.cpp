#include "util/timer.hpp"

#include <windows.h>

#include <cmath>
#include <numeric>

// The inverse of the tick count provided by QueryPerformanceFrequency()
static double inv_tick_count = 0;

timer::timer()
	: last_start(0)
{
}

void
timer::lap_start()
{
	LARGE_INTEGER count;
	if (!inv_tick_count) {
		QueryPerformanceFrequency(&count);
		inv_tick_count = 1.0 / static_cast<double>( count.QuadPart);
	}
	QueryPerformanceCounter( &count);
	last_start = static_cast<double>(count.QuadPart) * inv_tick_count;
}

void
timer::lap_stop()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter( &count);
	double delta_time = static_cast<double>(count.QuadPart)*inv_tick_count 
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
	LARGE_INTEGER count;
	if (!inv_tick_count) {
		QueryPerformanceFrequency(&count);
		inv_tick_count = 1.0 / static_cast<double>( count.QuadPart);
	}
	QueryPerformanceCounter( &count);
	last_start = static_cast<double>(count.QuadPart) * inv_tick_count;
}

void
hist_timer::stop()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter( &count);
	double delta_time = static_cast<double>(count.QuadPart)*inv_tick_count 
		- last_start;
	cumulative = cumulative * old_fraction + delta_time * (1.0 - old_fraction);
}

double
hist_timer::read() const
{
	return cumulative;
}
