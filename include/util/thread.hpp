#ifndef VPYTHON_UTIL_THREAD_HPP
#define VPYTHON_UTIL_THREAD_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace cvisual {

using boost::mutex;
typedef mutex::scoped_lock lock;
using boost::condition;

} // !namesapce cvisual

#endif // !defined VPYTHON_UTIL_THREAD_HPP
