#ifndef VPYTHON_PYTHON_GIL_HPP
#define VPYTHON_PYTHON_GIL_HPP

#include <boost/python/detail/wrap_python.hpp>

namespace cvisual { namespace python {

class gil_release
{
	PyThreadState *save;
 public:
	inline gil_release() : save(PyEval_SaveThread()) {}
	inline ~gil_release() { PyEval_RestoreThread(save); }
};

class gil_lock
{
	PyGILState_STATE state;
 public:
	inline gil_lock() : state( PyGILState_Ensure()) {}
	inline ~gil_lock() { PyGILState_Release( state); }
};
	
} } // !namespace cvisual::python

#endif
