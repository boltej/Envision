/**
 * @file pyembed.cpp
 * Defines a wrapper to the Python C API.
 * @see pyembed.h
 * Copyright (C) 2006 Mindteck.
 * @author Vijay Mathew Pandyalakal
 * @date 08 May, 2006
 */

#include <iostream>
using std::cout;
#include <sstream>

#include "pyembed.h"

// a generic function to raise an exception when a python
// function call fails.
static void raise_func_failed_exception(const std::string& func_name)  
  throw (pyembed::Python_exception);

// functions to convert complex python data structures to their C++ 
// counterparts
static void make_list_from_tuple(PyObject* tuple, pyembed::String_list& out);
static void make_list_from_list(PyObject* list, pyembed::String_list& out);
static void make_map_from_dict(PyObject* dict, pyembed::String_map& out);
static std::string to_string(PyObject* val);
// :~

// class Python_env

pyembed::Python_env::Python_env(int argc, char** argv)
{
  Py_Initialize();    
  PySys_SetArgv(argc, argv);
}

pyembed::Python_env::~Python_env()
{
  Py_Finalize();
}

// class Python_exception : public std::exception

pyembed::Python_exception::Python_exception(const std::string& m)
{
  message = m;
}

pyembed::Python_exception::~Python_exception() throw()
{
}

const char* 
pyembed::Python_exception::what() throw()
{
  return message.c_str();
}

// class Python

pyembed::Python::Python(int argc, char** argv) 
{
  env = new Python_env(argc, argv);
  module = 0;
}
pyembed::Python::~Python()
{
  if (module)
    Py_DECREF(module);
  if (env)
    delete env;
}

void 
pyembed::Python::run_string(const std::string& s)
  throw (pyembed::Python_exception)
{
  int ret = PyRun_SimpleString(s.c_str());
  if (ret)
    throw Python_exception("There was a error in the script.");
}

void 
pyembed::Python::run_file(const std::string& f)
  throw (pyembed::Python_exception)
{
  FILE* file = fopen(f.c_str(), "r");
  if (!file)
    {
      std::ostringstream oss;
      oss << "Failed to open file <" << f << '>';
      throw Python_exception(oss.str());
    }
  int ret = PyRun_SimpleFile(file, f.c_str());
  fclose(file);
  if (ret)
    throw Python_exception("There was a error in the script.");
}

void 
pyembed::Python::load(const std::string& module_name)
  throw (pyembed::Python_exception)
{
  PyObject* name = PyString_FromString(module_name.c_str());
  module = PyImport_Import(name);    
  Py_DECREF(name);
  if (!module)
    {
      std::ostringstream oss;
      oss << "Failed to load module <" << module_name << ">";
      throw Python_exception(oss.str());
    }
}

void 
pyembed::Python::call(const std::string& func_name)			       
  throw (pyembed::Python_exception)
{
  Arg_map args;
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    Py_DECREF(ret_val);
}

void 
pyembed::Python::call(const std::string& func_name, 
			       const pyembed::Arg_map& args)
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    Py_DECREF(ret_val);
}

void 
pyembed::Python::call(const std::string& func_name, 
			       const pyembed::Arg_map& args,
			       long& ret
			       )
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    {
      if (!PyInt_Check(ret_val))
	{
	  Py_DECREF(ret_val);
	  throw Python_exception("Not an integer value in return.");
	}
      ret = PyInt_AsLong(ret_val);
      Py_DECREF(ret_val);
    }
  else
    raise_func_failed_exception(func_name);
}

void 
pyembed::Python::call(const std::string& func_name, 
				    const pyembed::Arg_map& args,
				    double& ret
				    )
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    {
      if (!PyFloat_Check(ret_val))
	{
	  Py_DECREF(ret_val);
	  throw Python_exception("Not a real value in return.");
	}
      ret = PyFloat_AsDouble(ret_val);
      Py_DECREF(ret_val);
    }
  else
    raise_func_failed_exception(func_name);
}

void 
pyembed::Python::call(const std::string& func_name, 
			       const pyembed::Arg_map& args,
			       std::string& ret
			       )
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    {
      if (!PyString_Check(ret_val))
	{
	  Py_DECREF(ret_val);            
	  throw Python_exception("Not a string value in return.");
	}
      ret = PyString_AsString(ret_val);
      Py_DECREF(ret_val);            
    }
  else
    raise_func_failed_exception(func_name);
}

void 
pyembed::Python::call(const std::string& func_name, 
			       const pyembed::Arg_map& args,
			       pyembed::String_list& ret
			       )
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    {
      if (PyTuple_Check(ret_val))
	make_list_from_tuple(ret_val, ret);
      else if (PyList_Check(ret_val))
	make_list_from_list(ret_val, ret);
      else
	{
	  Py_DECREF(ret_val);
	  throw Python_exception("Not a tuple or list in return.");
	}
      Py_DECREF(ret_val);            
    }
  else
    raise_func_failed_exception(func_name);
}

void 
pyembed::Python::call(const std::string& func_name, 
			       const pyembed::Arg_map& args,
			       pyembed::String_map& ret
			       )
  throw (pyembed::Python_exception)
{
  PyObject* ret_val = make_call(func_name, args);
  if (ret_val)
    {
      if (!PyDict_Check(ret_val))
	{
	  Py_DECREF(ret_val);            
	  throw Python_exception("Not a dictionary object in return.");
	}
      make_map_from_dict(ret_val, ret);
      Py_DECREF(ret_val);            
    }
  else
    raise_func_failed_exception(func_name);
}

// private functions

PyObject* 
pyembed::Python::get_function_object(const std::string& func_name)
  throw (pyembed::Python_exception)
{
  if (!module)
    throw Python_exception("No module loaded.");
  PyObject* func = PyObject_GetAttrString(module, (char*)func_name.c_str());
  if (!func || !PyCallable_Check(func))
    {
      std::ostringstream oss;
      oss << '<' << func_name << "> is not a function or is not callable.";
      throw Python_exception(oss.str());
    }
  return func;
}

PyObject* 
pyembed::Python::create_args(const Arg_map& args)
  throw (pyembed::Python_exception)
{
  if (!module)
    throw Python_exception("No module loaded.");
  size_t sz = args.size();  
  if (!sz)
    return 0;
  PyObject* tuple = PyTuple_New(sz);
  size_t i = 0;
  PyObject* val = 0;
  Arg_map::const_iterator it;
  for (it = args.begin(); it != args.end(); it++)
    {
      switch (it->second)
	{
	case Py_long:
	  {
	    long l = strtol(it->first.c_str(), 0, 10);
	    val = PyInt_FromLong(l);
	    break;
	  }
	case Py_real:
	  {
	    double d = strtod(it->first.c_str(), 0);	    
	    val = PyFloat_FromDouble(d);
	    break;
	  }
	case Py_string:
	  {
	    val = PyString_FromString(it->first.c_str());
	    break;
	  }
	default:
	  {
	    Py_DECREF(tuple);	    
	    throw Python_exception("Not a supported type for argument passing.");
	  }
	}
      if (!val)
	{
	  Py_DECREF(tuple);	  
	  std::ostringstream oss;
	  oss << "Failed to convert <" << it->second << "> to PyObject.";
	  throw Python_exception(oss.str());
	}      
      PyTuple_SetItem(tuple, i++, val);
      Py_DECREF(val);      
    }
  return tuple;
}

static bool cleared = false;

PyObject*
pyembed::Python::make_call(const std::string& func_name,
			   const pyembed::Arg_map& args)
  throw (pyembed::Python_exception)
{
  PyObject* func = get_function_object(func_name);
  PyObject* args_tuple = create_args(args);
  Py_XINCREF(args_tuple);
  PyObject* ret = PyObject_CallObject(func, args_tuple);
  Py_XDECREF(args_tuple);
  Py_DECREF(func);  
  return ret;
}

// local functions

void
raise_func_failed_exception(const std::string& func_name)
  throw (pyembed::Python_exception)
{
  std::ostringstream oss;
  oss << "Call to function <" << func_name << "> failed.";
  throw pyembed::Python_exception(oss.str());
}

void 
make_list_from_tuple(PyObject* tuple, pyembed::String_list& out)
{
  if (tuple)
    {
      int size = PyTuple_Size(tuple);
      for (int i=0; i<size; i++)
	{
	  PyObject* val = PyTuple_GetItem(tuple, i);
	  out.push_back(to_string(val));
	}
    }
}

void 
make_list_from_list(PyObject* list, pyembed::String_list& out)
{
  if (list)
    {
      int size = PyList_Size(list);
      for (int i=0; i<size; i++)
	{
	  PyObject* val = PyList_GetItem(list, i);
	  out.push_back(to_string(val));
	}
    }
}

void 
make_map_from_dict(PyObject* dict, pyembed::String_map& out)
{
  if (dict)
    {
      PyObject* key;
      PyObject* value;
      int pos = 0;
      while (PyDict_Next(dict, &pos, &key, &value)) 
	out[to_string(key)] = to_string(value);
    }
}

std::string 
to_string(PyObject* val)
{
  if (val)
    {
      std::ostringstream oss;
      if (PyInt_Check(val))
	oss << PyInt_AsLong(val);
      else if (PyLong_Check(val))
	oss << PyLong_AsLong(val);
      else if (PyFloat_Check(val))
	oss << PyFloat_AsDouble(val);
      else if (PyString_Check(val))
	oss << '\"' << PyString_AsString(val) << '\"';
      else if (PyTuple_Check(val))
	{
	  oss << '(';
	  pyembed::String_list ret;
	  make_list_from_tuple(val, ret);
	  size_t sz = ret.size();
	  for (size_t i=0; i<sz; i++)
	    {
	      oss << ret[i];
	      if (i != (sz - 1))
		oss << ',';
	    }
	  oss << ')';
	}
      else if (PyList_Check(val))
	{
	  oss << '[';
	  pyembed::String_list ret;
	  make_list_from_list(val, ret);
	  size_t sz = ret.size();
	  for (size_t i=0; i<sz; i++)
	    {
	      oss << ret[i];
	      if (i != (sz - 1))
		oss << ',';
	    }
	  oss << ']';
	}
      else if (PyDict_Check(val))
	{
	  oss << '{';
	  pyembed::String_map ret;
	  make_map_from_dict(val, ret);
	  size_t sz = ret.size();
	  size_t i = 0;
	  pyembed::String_map::const_iterator it;
	  for (it=ret.begin(); it != ret.end(); it++)
	    {
	      oss << it->first << ':' << it->second;
	      if (i != (sz - 1))
		oss << ',';
	      i++;
	    }
	  oss << '}';
	}
      else
	return "";
      return oss.str();
    }
  else
    return "";
}
