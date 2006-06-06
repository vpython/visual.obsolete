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
#include "python/num_util.hpp"
#include "python/gil.hpp"

namespace cvisual {
void wrap_display_kernel();
void wrap_primitive();
void wrap_rgba();
void wrap_light(); // Also defined in wrap_rgba.cpp
void wrap_vector();
void wrap_arrayobjects();
void wrap_glib_ustring();

namespace python {
	void wrap_vector_array();
	void wrap_scalar_array();
} // !namespace python

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

void
py_rate( double freq)
{
	python::gil_release R;
	rate(freq);
}

BOOST_PYTHON_MODULE( cvisual)
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

	// Private functions for initializing and choosing the numeric backend
	def( "_init_numeric_impl", python::init_numeric_impl);
	def( "_init_numarray_impl", python::init_numarray_impl);
	def( "_use_numeric_impl", python::use_numeric_impl);
	def( "_use_numarray_impl", python::use_numarray_impl);
	
	// Initialize the Python thread system.
	PyEval_InitThreads();
	
	// A subset of the python standard exceptions may be thrown from visual
	register_exception_translator<std::out_of_range>( 
		&translate_std_out_of_range);
	register_exception_translator<std::invalid_argument>( 
		&translate_std_invalid_argument);
	register_exception_translator<std::runtime_error>( 
		&translate_std_runtime_error);

	def( "rate", py_rate, "rate(arg) -> Limits the execution rate of a loop to arg"
		" iterations per second.");

	wrap_vector();
	wrap_rgba();
	wrap_light();
	wrap_display_kernel();
	wrap_primitive();
	wrap_arrayobjects();
	python::wrap_vector_array();
	python::wrap_scalar_array();
#if !(defined(_WIN32) || defined(_MSC_VER))
	wrap_glib_ustring();
#endif
}

} // !namespace cvisual
