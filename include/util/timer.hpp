#ifndef VPYTHON_UTIL_TIMER_HPP
#define VPYTHON_UTIL_TIMER_HPP

#include <vector>
#include <utility>

// A lap timer that maintains all of its lap data for statistical analysis.
class timer
{
 private:
	std::vector<double> times;
	double last_start;
 
 public:
	timer();
	void lap_start();
	void lap_stop();
	void reset(); 
	double average() const;
	double std_deviation() const;
	// Returns a 95% confidence interval for the true mean elapsed time, in 
	// seconds.
	std::pair<double, double> confidence_interval() const;
};

// A timer with a built-in histeresis(sp?) filter.
class hist_timer
{
 private:
	double cumulative;
	double last_start;

 public:
	hist_timer();
	// Create a new timer, internally initialized to init.  This may improve the
	// initial seek if the approximate average is known.
	hist_timer( double init);
	void start();
	void stop();
	// Read the last elapsed time.
	double read() const;	
};

#endif // !defined VPYTHON_UTIL_TIMER_HPP
