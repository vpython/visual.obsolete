// This file takes roughly 115 MB RAM to compile.

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <stdexcept>
#include <exception>

#include <boost/python/exception_translator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/numeric.hpp>
#include <boost/python/def.hpp>

#define PY_ARRAY_UNIQUE_SYMBOL visual_PyArrayHandle
#include <Numeric/arrayobject.h>

#include "util/rate.hpp"
#include "display.hpp"
#include "util/errors.hpp"

namespace cvisual {
void wrap_display_kernel();
void wrap_primitive();
void wrap_rgba();
void wrap_light(); // Also defined in wrap_rgba.cpp
void wrap_vector();
void wrap_arrayobjects();

void 
translate_std_out_of_range( std::out_of_range e)
{
	PyErr_SetString( PyExc_IndexError, e.what());
}

void
translate_std_invalid_argument( std::invalid_argument e)
{
	PyErr_SetString( PyExc_ValueError, e.what());
}

void
translate_std_runtime_error( std::runtime_error e)
{
	PyErr_SetString( PyExc_RuntimeError, e.what());
}

BOOST_PYTHON_MODULE( libcvisual)
{
	VPYTHON_NOTE( "Importing cvisual from vpython-core2.");
	using namespace boost::python;
	
#if __GNUG__
#if __GNUC__ == 3
#if __GNUCMINOR__ >= 1 && __GNUCMINOR__ < 4
	std::set_terminate( __gnu_cxx::__verbose_terminate_handler);
#endif
#endif
#endif

	import_array();
	// Force the use of Numeric.ArrayType for Boost's array backend.
	numeric::array::set_module_and_type( "Numeric", "ArrayType");
	
	// A subset of the python standard exceptions may be thrown from visual
	register_exception_translator<std::out_of_range>( 
		&translate_std_out_of_range);
	register_exception_translator<std::invalid_argument>( 
		&translate_std_invalid_argument);
	register_exception_translator<std::runtime_error>( 
		&translate_std_runtime_error);

	def( "rate", rate, "rate(arg) -> Limits the execution rate of a loop to arg"
		" iterations per second.");

	wrap_vector();
	wrap_rgba();
	wrap_light();
	wrap_display_kernel();
	wrap_primitive();
	wrap_arrayobjects();
	
	// To be exported:
	// convex, faces, vector_array, scalar_array, mouse objects.
}

} // !namespace cvisual
