#ifndef VPYTHON_PYTHON_NUM_UTIL_HPP
#define VPYTHON_PYTHON_NUM_UTIL_HPP

#define PY_ARRAY_UNIQUE_SYMBOL visual_PyArrayHandle
#ifndef IMPORT_ARRAY
#define NO_IMPORT_ARRAY
#endif

// num_util.h and num_util.cpp were obtained from:
// http://www.eos.ubc.ca/research/clouds/num_util.html on 2003-12-17 under the
// terms and conditions of the Boost Software License, version 1.0.  num_util
// was written by Rhys Goldstein, Chris Seymour and Phil Austin.

// Questions or comments about num_util should be directed to Phil Austin at
// paustin@eos.ubs.ca.



/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

// Changes from the original num_util V1:
// - Moved into the visual namespace to prevent clashes with other projects
// using num_util.  2003-12-18
// - Changed header inclusion guards to follow the same conventions as the rest
// of Visual.  2003-12-18
// - Changed the definition of PY_ARRAY_UNIQUE_SYMBOL to prevent clashes with
// other projects using num_util.  2003-12-18
// - Changed header #includes to reduce compile times.  2003-12-19
// - Bring boost::python::numeric::array into the visual namespace for
// convienience.  2003-12-19
// - data(), shape(), rank(): Modified to ensure that no calls are made into the
// Python interpreter, espcially reference counting operations and PyArray_Check()
// For the reference counting operations, we can guarantee that we do not need
// them base on the fact that these functions are never called without owning
// at least one real boost::python::numeric::array.  For the PyArray_Check(),
// we can guarantee that we *never* have an array on our hands that is not a
// genuine array.  These functions needed to be changed to ensure that we do not
// call any functions in the interpreter or Numeric from within the rendering
// loop since they cause the interpreter to crash sporadically on multiprocessor
// machines. 2004-01-12

// Incorporated num_util release 2 to move to compatibility with numpy
// Eliminate references to numeric and numarray initiation, add numpy initiation


#include <boost/python/numeric.hpp>
#include <boost/python/extract.hpp>
#include <numpy/arrayobject.h>
//#include <iostream>
//#include <sstream>
#include <vector>
#include <numeric>
#include <map>
#include <complex>

namespace cvisual { namespace python {

  using boost::python::numeric::array;

  class double_array : public array {
  public:
  	double_array( const boost::python::handle<>& h ) : array(h) {}
  	double_array( const array& a ) : array(a) {}  //< TODO: callers are doing unnecessary copying; somewhat type unsafe
  };

  //!


  /**
   *Creates an one-dimensional numpy array of length n and numpy type t.
   * The elements of the array are initialized to zero.
   *@param n an integer representing the length of the array.
   *@param t elements' numpy type. Default is double.
   *@return a numeric array of size n with elements initialized to zero.
   */
  array makeNum(npy_intp n, NPY_TYPES t =NPY_DOUBLE);


 /**
   *Creates a n-dimensional numpy array with dimensions dimens and numpy
   *type t. The elements of the array are initialized to zero.
   *@param dimens a vector of interger specifies the dimensions of the array.
   *@param t elements' numpy type. Default is double.
   *@return a numeric array of shape dimens with elements initialized to zero.
   */
  array makeNum(const std::vector<npy_intp>& dimens, NPY_TYPES t =NPY_DOUBLE);

  /**
   *Function template returns PyArray_Type for C++ type
   *See num_util.cpp for specializations
   *@param T C++ type
   *@return numpy type enum
   */



  /**
   *A free function that retrieves the numpy type of a numpy array.
   *@param arr a Boost/Python numeric array.
   *@return the numpy type of the array's elements
   */
  NPY_TYPES type(array arr);

  /**
   *Throws an exception if the actual array type is not equal to the expected
   *type.
   *@param arr a Boost/Python numeric array.
   *@param expected_type an expected numpy type.
   *@return -----
   */
  void check_type(array arr,
		  NPY_TYPES expected_type);

  /**
   *Returns the dimensions in a vector.
   *@param arr a Boost/Python numeric array.
   *@return a vector with integer values that indicates the shape of the array.
  */
  std::vector<npy_intp> shape(array arr);


  /**
   *Throws an exception if the actual dimensions of the array are not equal to
   *the expected dimensions.
   *@param arr a Boost/Python numeric array.
   *@param expected_dims an integer vector of expected dimension.
   *@return -----
   */
  void check_shape(array arr,
		   std::vector<npy_intp> expected_dims);


  /**
   *Returns true if the array is contiguous.
   *@param arr a Boost/Python numeric array.
   *@return true if the array is contiguous, false otherwise.
  */
  bool iscontiguous(array arr);

  /**
   *Throws an exception if the array is not contiguous.
   *@param arr a Boost/Python numeric array.
   *@return -----
  */
  void check_contiguous(array arr);

/**
   *Returns a pointer to the data in the array.
   *@param arr a Boost/Python numeric array.
   *@return a char pointer pointing at the first element of the array.
  void* data(array arr);
  */


  /**
   *Returns a pointer to the data in the array.
   *@param arr a Boost/Python numeric array.
   *@return a char pointer pointing at the first element of the array.
   */
  // USED
  char* data(const array& arr);

  /**
   *Returns a clone of this array with a new type.
   *@param arr a Boost/Python numeric array.
   *@param t NPY_TYPES of the output array.
   *@return a replicate of 'arr' with type set to 't'.
   */
  array astype(array arr,
				       NPY_TYPES t);


  /**
   *Mapping from a PyArray_TYPE to its corresponding name in string.
   */
  typedef std::map<NPY_TYPES, std::string> KindStringMap;

  /**
   *Mapping from a PyArray_TYPE to its corresponding typeID in char.
   */
  typedef std::map<NPY_TYPES, char> KindCharMap;

  /**
   *Mapping from a typeID to its corresponding PyArray_TYPE.
   */
  typedef std::map<char, NPY_TYPES> KindTypeMap;

  /**
   *Converts a PyArray_TYPE to its name in string.
   *@param t_type a NPY_TYPES.
   *@return the corresponding name in string.
   */
  std::string type2string(NPY_TYPES t_type);

  /**
   *Converts a PyArray_TYPE to its single character typecode.
   *@param t_type a NPY_TYPES.
   *@return the corresponding typecode in char.
   */
  char type2char(NPY_TYPES t_type);

  /**
   *Coverts a single character typecode to its NPY_TYPES.
   *@param e_type a NPY_TYPES typecode in char.
   *@return its corresponding NPY_TYPES.
   */
  NPY_TYPES char2type(char e_type);

  void init_numpy();
//  void use_numpy_impl();
  size_t typesize( NPY_TYPES t);

  template <class T>
  struct type_npy_traits {
  };

  template <>
  struct type_npy_traits<float> {
	  static const int npy_type = NPY_FLOAT;
  };

  template <>
  struct type_npy_traits<double> {
	  static const int npy_type = NPY_DOUBLE;
  };

} } //  visual

#endif
