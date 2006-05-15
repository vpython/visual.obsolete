/** This file contains the implementation of the Numeric back-end.
*/

#include "python/num_util_impl.hpp"

#ifdef VISUAL_HAVE_NUMERIC
# define PY_ARRAY_UNIQUE_SYMBOL visual_numeric_PyArrayHandle
# include <Numeric/arrayobject.h>

  // Local function prototypes
  using namespace boost::python;
  namespace cvisual { namespace python {
    static int rank(const numeric::array& arr);
    static PyArray_TYPES type(array_types);
    static array_types type( char);
    static char type2char( array_types);
  } } // !namespace cvisual::python
#endif // !VISUAL_HAVE_NUMERIC

#include <boost/python/extract.hpp>
#include <stdexcept>

namespace cvisual { namespace python {


// True when the PyArray_API pointers have been initialized.
static bool arrayapi_initialized = false;

void
init_numeric_impl()
{
#ifdef VISUAL_HAVE_NUMERIC
	import_array();
	arrayapi_initialized = true;
#else
	throw std::runtime_error( "This build of Visual does not support Numeric.");
#endif
}

#ifdef VISUAL_HAVE_NUMERIC
static PyArray_TYPES 
type( array_types t)
{
	switch (t) {
	    case char_t:
	    	return PyArray_CHAR;
	    case uchar_t:
	    	return PyArray_UBYTE;
	    case schar_t:
	    	return PyArray_SBYTE;
	    case short_t:
	    	return PyArray_SHORT;
	    case int_t:
	    	return PyArray_INT;
	    case long_t:
	    	return PyArray_LONG;
	    case float_t:
	    	return PyArray_FLOAT;
	    case double_t:
	    	return PyArray_DOUBLE;
	    case cfloat_t:
	    	return PyArray_CFLOAT;
	    case cdouble_t:
	    	return PyArray_CDOUBLE;
	    case object_t:
	    	return PyArray_OBJECT;	
		default:
			bool type_is_recognized = false;
			assert( type_is_recognized == true);
	}
}

static array_types
type( char t)
{
	switch (t) {
	    case 'c':
	    	return char_t;
	    case 'b':
	    	return uchar_t;
	    case '1':
	    	return schar_t;
	    case 's':
	    	return short_t;
	    case 'i':
	    	return int_t;
	    case 'l':
	    	return long_t;
	    case 'f':
	    	return float_t;
	    case 'd':
	    	return double_t;
	    case 'F':
	    	return cfloat_t;
	    case 'D':
	    	return cdouble_t;
	    case 'O':
	    	return object_t;	
		default:
			bool type_is_recognized = false;
			assert( type_is_recognized == true);
	}
}

static char type2char( array_types t)
{
	switch (t) {
	    case char_t:
	    	return 'c';
		case uchar_t:
			return 'b';
	    case schar_t:
	    	return '1';
	    case short_t:
	    	return 's';
	    case int_t:
	    	return 'i';
	    case long_t:
	    	return 'l';
	    case float_t:
	    	return 'f';
	    case double_t:
	    	return 'd';
	    case cfloat_t:
	    	return 'F';
	    case cdouble_t:
	    	return 'D';
	    case object_t:
	    	return 'O';
		default:
			return 0;
	}
}

static numeric::array 
makeNum_impl_numeric(std::vector<int> dimens, array_types t)
{
	object obj(handle<>(PyArray_FromDims(dimens.size(), &dimens[0], type(t))));
	return extract<numeric::array>(obj);	
}

static array_types 
type_impl_numeric(numeric::array arr)
{
	return type(arr.typecode());
}

// Return the number of dimensions
static int 
rank(const numeric::array& arr)
{
	return ((PyArrayObject*) arr.ptr())->nd;
}

static std::vector<int>
shape_impl_numeric(numeric::array arr)
{
	std::vector<int> out_dims;
	int* dims_ptr = ((PyArrayObject*) arr.ptr())->dimensions;
	int the_rank = rank(arr);
	for (int i = 0; i < the_rank; i++){
		out_dims.push_back(*(dims_ptr + i));
	}
	return out_dims;
	
}

static bool 
iscontiguous_impl_numeric(numeric::array arr)
{
	return PyArray_ISCONTIGUOUS((PyArrayObject*)arr.ptr());	
}

static char* 
data_impl_numeric(const numeric::array& arr)
{
	return ((PyArrayObject*) arr.ptr())->data;
}

static numeric::array 
astype_impl_numeric(numeric::array arr, array_types t)
{
	return (numeric::array) arr.astype(type2char(t));
}
#endif // VISUAL_HAVE_NUMERIC

void
use_numeric_impl()
{
#ifdef VISUAL_HAVE_NUMERIC
	if (!arrayapi_initialized)
		init_numeric_impl();
	makeNum_impl = makeNum_impl_numeric;
	type_impl = type_impl_numeric;
	shape_impl = shape_impl_numeric;
	iscontiguous_impl = iscontiguous_impl_numeric;
	data_impl = data_impl_numeric;
	astype_impl = astype_impl_numeric;
	numeric::array::set_module_and_type( "Numeric", "ArrayType");
#else
	throw std::runtime_error( "This build of Visual does not support Numeric.");
#endif
}

} } // !namespace cvisual::python
