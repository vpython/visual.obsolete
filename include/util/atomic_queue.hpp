#ifndef VPYTHON_UTIL_ATOMIC_QUEUE_HPP
#define VPYTHON_UTIL_ATOMIC_QUEUE_HPP

#include "util/thread.hpp"
#include <queue>

namespace cvisual {

template <typename T>
class atomic_queue
{
 private:
	mutable mutex barrier;
	std::queue<T> data;
 
	condition ready;
	bool waiting;

 public:
	atomic_queue() : waiting(false) {}
	
	void push( const T& item)
	{
		lock L(barrier);
		data.push( item);
		if (waiting) {
			ready.notify_all();
		}
	}
	
	T pop()
	{
		lock L(barrier);
		if (data.empty()) {
			waiting = true;
			while (data.empty())
				ready.wait(L);
			waiting = false;
		}
		T ret = data.front();
		data.pop();
		return ret;
	}
	
	size_t size() const
	{
		lock L(barrier);
		return data.size();
	}
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_ATOMIC_QUEUE_HPP
