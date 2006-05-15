#ifndef VPYTHON_PYTHON_NUM_UTIL_IMPL_HPP
#define VPYTHON_PYTHON_NUM_UTIL_IMPL_HPP

/**
	This file contains the set of function pointers that will be called
	by the utility functions in num_util.
	They are declared here, and defined in num_util.cpp.  They are initialized
	to NULL statically, and the functions init_numarray() in 
	num_util_numarray.{h,h}pp and init_numeric() in num_util_numeric.{h,c}pp
	initialize both their internal function pointers and set the implementation
	function pointers to their independant implementations.
*/

#include "python/num_util.hpp"

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
// A slightly more sane default?
# define VISUAL_HAVE_NUMERIC
# undef VISUAL_HAVE_NUMARRAY
#endif

namespace cvisual { namespace python {
	
extern array (*makeNum_impl)(std::vector<int>, array_types);
extern array_types (*type_impl)(array);
extern std::vector<int> (*shape_impl)(array);
extern bool (*iscontiguous_impl)(array);
extern char* (*data_impl)(const array&);
extern array (*astype_impl)(array, array_types);

} } // !namespace cvisual::python

#endif // !defined VISUAL_NUM_UTIL_IMPL_HPP
