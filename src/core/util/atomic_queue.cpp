#include "util/atomic_queue.hpp"
#include "python/gil.hpp"

#include <boost/thread/xtime.hpp>
#include <iostream>
#include "util/errors.hpp"

namespace cvisual {
	
void 
atomic_queue_impl::push_notify()
{
	empty = false;
	if (waiting)
		ready.notify_all();
}

void 
atomic_queue_impl::pop_wait( lock& L)
{
	while (empty) {
		waiting = true;
		ready.wait(L);
	}
	waiting = false;
}

namespace {
void
xtime_increment( boost::xtime& time, int ms)
{
	assert( ms < 100);
	time.nsec += ms * 1000000;
	if (time.nsec > 1000000000) {
		time.nsec -= 1000000000;
		time.sec += 1;
	}
}
} // !namespace (anon)

void 
atomic_queue_impl::py_pop_wait( lock& L)
{
	using python::gil_release;
	using python::gil_lock;
	
	gil_release release;
	
	boost::xtime release_time;
	xtime_get( &release_time, boost::TIME_UTC);
	
	while (empty) {
		waiting = true;
		xtime_increment( release_time, 10);
		if (!ready.timed_wait( L, release_time)) {
			gil_lock py_lock;
			Py_MakePendingCalls();
		}
		else
			break;
	}
	waiting = false;
}
	
} // !namespace cvisual
