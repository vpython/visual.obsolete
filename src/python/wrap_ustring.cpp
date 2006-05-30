#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/extract.hpp>

#include <glibmm/init.h>
#include <glibmm/convert.h>
#include <glibmm/thread.h>

#include <string>
#include <iostream>
#include <cstdlib>

namespace cvisual {

namespace {
Glib::IConv* utf8_to_utf16 = 0;
Glib::IConv* utf16_to_utf8 = 0;
}

namespace py = boost::python;

struct glib_ustring_from_pyunicode
{
	glib_ustring_from_pyunicode()
	{
		py::converter::registry::push_back( 
			&convertible,
			&construct,
			py::type_id<Glib::ustring>());
	}
	
	static void* convertible( PyObject* obj)
	{
		using py::handle;
		using py::allow_null;
		if (PyString_CheckExact(obj)) {
			return obj;
		}
		// This object will be released in construct()
		PyObject* uni_object = PyUnicode_FromObject( obj);
		return uni_object;
	}

	static void construct( 
		PyObject* obj, 
		py::converter::rvalue_from_python_stage1_data* data)
	{
		using namespace boost::python;
		
		void* storage = (
			(py::converter::rvalue_from_python_storage<Glib::ustring>*)
			data)->storage.bytes;
		
		if (PyString_CheckExact(obj)) {
			py::object cpp_obj = py::object(py::handle<>(py::borrowed(obj)));
			new (storage) Glib::ustring( py::extract<std::string>( cpp_obj));
		}
		else {
			assert( PyUnicode_Check( obj) == true);
			size_t remaining = PyUnicode_GET_DATA_SIZE( obj);
			const char *src_data = PyUnicode_AS_DATA( obj);
			new (storage) Glib::ustring(
				utf16_to_utf8->convert( std::string(src_data, remaining)));
			Py_DECREF( obj); // Was allocated in convertible
		}
		data->convertible = storage;
	}
};

struct glib_ustring_to_pyunicode
{
	static PyObject* convert( const Glib::ustring& str)
	{
		std::string converted = utf8_to_utf16->convert( str);
		return PyUnicode_FromUnicode( 
			(const Py_UNICODE*)converted.data(), converted.size());
	}
};

void
wrap_glib_ustring(void)
{
	// Despite Python's assertions to the contrary, Py_UNICODE is 32 bits wide
	// on Linux-x86.  The following will determine the size of Py_UNICODE and
	// initialize the correct convertion routines.
	size_t pyunicode_size = sizeof( Py_UNICODE);
	std::string pyunicode_encoding;
	switch (pyunicode_size) {
		case 1:
			pyunicode_encoding = "UTF-8";
			break;
		case 2:
			pyunicode_encoding = "UTF-16";
			break;
		case 4:
			pyunicode_encoding = "UTF-32";
			break;
		default:
			pyunicode_encoding = "UTF-8";
	}
	
	Glib::init();
	if (!Glib::thread_supported())
		Glib::thread_init();
	
	try {
	utf8_to_utf16 = new Glib::IConv( pyunicode_encoding, "UTF-8");
	} catch (Glib::ConvertError& e) {
		std::cerr << "Failed creating converter from UTF-8 to "
			<< pyunicode_encoding << ": " << e.what() << std::endl;
	}
	try {
	utf16_to_utf8 = new Glib::IConv( "UTF-8", pyunicode_encoding);
	} catch (Glib::ConvertError& e) {
		std::cerr << "Faile creating converter from " << pyunicode_encoding
			<< " to UTF8: " << e.what() << std::endl;
	}
	if (!(utf8_to_utf16 && utf16_to_utf8)) {
		std::cerr << "Aborting." << std::endl;
		std::exit(1);
	}
	
	glib_ustring_from_pyunicode();
	py::to_python_converter< Glib::ustring, glib_ustring_to_pyunicode>();
}

} // !namespace cvisual
