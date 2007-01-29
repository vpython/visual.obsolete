
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/rgba.hpp"
#include "util/lighting.hpp"

#include <boost/python/to_python_converter.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/proxy.hpp>
#include <boost/python/class.hpp>

namespace cvisual {
namespace py = boost::python;

struct rgb_from_seq
{
	rgb_from_seq()
	{
		py::converter::registry::push_back( 
			&convertible,
			&construct,
			py::type_id<rgb>());
	}
	
	static void* convertible( PyObject* obj)
	{
		using py::handle;
		using py::allow_null;
		if (PyInt_Check(obj) || PyFloat_Check(obj))
			return obj;
		
		handle<> obj_iter( allow_null( PyObject_GetIter(obj)));
		if (!obj_iter.get()) {
			PyErr_Clear();
			return 0;
		}
		int obj_size = PyObject_Length(obj);
		if (obj_size < 0) {
			PyErr_Clear();
			return 0;
		}
		if (obj_size != 3 && obj_size != 4)
			return 0;
		return obj;
	}
	
	static void construct( 
		PyObject* _obj, 
		py::converter::rvalue_from_python_stage1_data* data)
	{
		py::object obj = py::object(py::handle<>(py::borrowed(_obj)));
		void* storage = (
			(py::converter::rvalue_from_python_storage<rgb>*)
			data)->storage.bytes;
		if (PyFloat_Check(_obj) || PyInt_Check(_obj)) {
			new (storage) rgb( py::extract<float>(obj));
			data->convertible = storage;
			return;
		}
		new (storage) rgb( 
			py::extract<float>(obj[0]), 
			py::extract<float>(obj[1]),
			py::extract<float>(obj[2]));
		data->convertible = storage;
	}
};

struct rgba_from_seq
{
	rgba_from_seq()
	{
		py::converter::registry::push_back( 
			&convertible,
			&construct,
			py::type_id<rgba>());
	}
	
	static void* convertible( PyObject* obj)
	{
		using py::handle;
		using py::allow_null;
		if (PyInt_Check(obj) || PyFloat_Check(obj))
			return obj;
		
		handle<> obj_iter( allow_null( PyObject_GetIter(obj)));
		if (!obj_iter.get()) {
			PyErr_Clear();
			return 0;
		}
		int obj_size = PyObject_Length(obj);
		if (obj_size < 0) {
			PyErr_Clear();
			return 0;
		}
		if (obj_size != 3 && obj_size != 4)
			return 0;
		return obj;
	}
	
	static void construct( 
		PyObject* _obj, 
		py::converter::rvalue_from_python_stage1_data* data)
	{
		py::object obj = py::object(py::handle<>(py::borrowed(_obj)));
		void* storage = (
			(py::converter::rvalue_from_python_storage<rgba>*)
			data)->storage.bytes;
		if (PyFloat_Check(_obj) || PyInt_Check(_obj)) {
			new (storage) rgba( py::extract<float>(obj));
			data->convertible = storage;
			return;
		}
		int obj_size = PyObject_Length(_obj);
		if (obj_size == 3)
			new (storage) rgba( 
				py::extract<float>(obj[0]), 
				py::extract<float>(obj[1]),
				py::extract<float>(obj[2]));
		else
			new (storage) rgba( 
				py::extract<float>(obj[0]), 
				py::extract<float>(obj[1]),
				py::extract<float>(obj[2]),
				py::extract<float>(obj[3]));
		
		data->convertible = storage;
	}
};

struct rgb_to_tuple
{
	static PyObject* convert( const rgb& color)
	{
		py::tuple ret = py::make_tuple( color.red, color.green, color.blue);
		Py_INCREF(ret.ptr());
		return ret.ptr();
	}
};

struct rgba_to_tuple
{
	static PyObject* convert( const rgba& color)
	{
		py::tuple ret = py::make_tuple( color.red, color.green, color.blue, color.opacity);
		Py_INCREF(ret.ptr());
		return ret.ptr();
	}
};

void
wrap_rgba()
{
	rgb_from_seq();
	rgba_from_seq();
	py::to_python_converter< rgb, rgb_to_tuple>();
	py::to_python_converter< rgba, rgba_to_tuple>();
}

void
set_light_attenuation( light* This, py::object a)
{
	This->set_attenuation( 
		py::extract<float>(a[0]),
		py::extract<float>(a[1]),
		py::extract<float>(a[2]));
}

py::tuple
get_light_attenuation( light* This)
{
	vector ret = This->get_attentuation();
	return py::make_tuple( ret.x, ret.y, ret.z);
}

void
wrap_light()
{
	using namespace py;
	
	class_<light>( "light", init<const vector&>( args( "pos")))
		.def( init<const light&>())
		.add_property( "pos",
			make_function( &light::get_pos, 
				return_internal_reference<>()),
			&light::set_pos)
		.add_property( "local", &light::is_local, &light::set_local)
		.add_property( "spot_direction", 
			make_function( &light::get_spot_direction, 
				return_internal_reference<>()),
			&light::set_spot_direction)
		.add_property( "spot_cutoff", 
			&light::get_spot_cutoff, &light::set_spot_cutoff)
		.add_property( "attenuation",
			get_light_attenuation, set_light_attenuation)
		.add_property( "diffuse_color",
			&light::get_diffuse_color, &light::set_diffuse_color)
		.add_property( "specular_color",
			&light::get_specular_color, &light::set_specular_color)
		;
}

} // !namespace cvisual
