No longer used (Nov. 2007)

#ifndef VPYTHON_UTIL_TIMER_HPP
#define VPYTHON_UTIL_TIMER_HPP

// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <vector>
#include <utility>

namespace cvisual {

/** A lap timer that maintains all of its lap data for statistical analysis. 
	It uses gettimeofday() on POSIX-ish platforms.
*/
class timer
{
 private:
	std::vector<double> times; ///< All of the time since the last reset() call.
	double last_start; ///< The system time at the last lap_start() call.
 
 public:
	/** Construct a new timer. */
	timer();
	/** Begin timing a single iteration. */
	void lap_start();
	/** Complete one iteration. */
	void lap_stop();
	/** Clears all of the accumulated timing data. */
	void reset(); 
	/** Retrieve the cumulative average time.
 		@return the average iteration time over all past laps, in seconds.
	*/
	double average() const;
	/** Compute the standard deviation in the lap times for the last sample 
 		period. 
	*/
	double std_deviation() const;
	/** Obtain a 95% confidence interval for the true mean elapsed time, in 
		seconds.
		@return .first is the average elapsed time, and .second is the delta
			for the range.
	*/
	std::pair<double, double> confidence_interval() const;
};

/**  A lap timer with a built-in histeresis(sp?) filter. */
class hist_timer
{
 private:
	double cumulative;  ///< The last filtered time.
	double last_start;  ///< The time at the last moment start() was called.

 public:
	/** Create a historesis filter that uses 90% old data, and 10% new.  */
	hist_timer();
	/** Create a new timer, internally initialized to init.  This may improve the
		initial seek if the approximate average is known.
	*/
	hist_timer( double init);
	void start();
	void stop();
	/** Read the historical elapsed time.
 		@return the filtered lap time, in seconds.
	*/
	double read() const;	
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_TIMER_HPP
