/**
 * @file pyembed.h
 * Declares a wrapper to the Python C API.
 * Copyright (C) 2006 Mindteck.
 * @author Vijay Mathew Pandyalakal
 * @date 08 May, 2006
 */

#ifndef PYEMBED_H
#define PYEMBED_H

#include <string>
#include <map>
#include <vector>
#include <exception>

#include <Python.h>

namespace pyembed
{
  /**
   * Manages the Python environment.
   * All python api calls must be wrapped in 
   * the context of a Python_env object.
   */
  class Python_env
  {

  public:

    Python_env(int argc, char** argv);
    ~Python_env();

  }; // class Python_env
  
  /**
   * Exception thrown by Python api calls.
   */
  class Python_exception : public std::exception
  {

  public:

    Python_exception(const std::string& m);
    virtual ~Python_exception() throw();

    virtual const char* what() throw();

  private:

    std::string message;

  }; // class Python_exception

  enum Type { Py_long, Py_real, Py_string };

  typedef std::map<std::string, Type> Arg_map;
  typedef std::map<std::string, std::string> String_map;
  typedef std::vector<std::string> String_list;

  /**
   * Provide wrappers to low-level python calls.
   */
  class Python
  {

  public:

    Python(int argc, char** argv);
    virtual ~Python();

    /**
     * Executes a python statement or script.
     * @param s Python script to compile and run.
     */
    void run_string(const std::string& s)
      throw (Python_exception);

    /**
     * Executes a python script file.
     * @param f Name of Python script file to compile and run.
     */
    void run_file(const std::string& f)
      throw (Python_exception);

    /**
     * Loads a python module to the environment.
     * @param module_name
     */
    void load(const std::string& module_name)
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * that returns nothing and takes no arguments.
     * @param func_name
     */
    void call(const std::string& func_name)
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * A return value, if any, is ignored.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args)
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * The function should return an integer
     * and this is copied to ret.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     * @param ret Functions return value, if any is copied 
     * here.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args,
		       long& ret
		       )
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * The function should return a real number
     * and this is copied to ret.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     * @param ret Functions return value, if any is copied 
     * here.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args,
		       double& ret
		       )
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * Any return values, despite type, is copied to
     * ret.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     * @param ret Functions return value, if any is copied 
     * here.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args,
		       std::string& ret
		       )
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * The function should return a list or a tuple,
     * and this is copied to ret.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     * @param ret Functions return value, if any is copied 
     * here.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args,
		       String_list& ret
		       )
      throw (Python_exception);

    /**
     * Executes a function in the module.
     * The function should return a dictionary,
     * and this is copied to ret.
     * @param func_name
     * @param args This should be empty if the function
     * takes no arguments.
     * @param ret Functions return value, if any is copied 
     * here.
     */
    void call(const std::string& func_name, 
		       const Arg_map& args,
		       String_map& ret
		       )
      throw (Python_exception);

  private:

    /**
     * Retrieves a function handle from the python module.
     * @param func_name
     * @return PyObject*
     */
    PyObject* get_function_object(const std::string& func_name)
      throw (Python_exception);

    /**
     * Converts an Arg_map to a tuple of PyObjects
     * so that they can be passed as arguments to a 
     * python function.
     * @param args Arg_map to convert
     * @return PyObject* args converted to a python tuple.
     */
    PyObject* create_args(const Arg_map& args)
      throw (Python_exception);

    /**
     * Makes the actual function call, with the help of the
     * above functions.
     * @param func_name
     * @param args
     * @return PyObject* the value returned by the python function,
     * or null if no value is returned.
     */
    PyObject* make_call(const std::string& func_name,
			const pyembed::Arg_map& args)
      throw (Python_exception);

  private:

    Python_env* env;
    PyObject* module;
    PyObject* args_tuple;

  }; // class Python

} // namespace pyembed

#endif /* #ifndef PYEMBED_H */
