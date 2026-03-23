%typemap(in) (const void * bufpointer, size_t bufsize) {

#ifdef PY_2
  if (PyString_Check($input))
    {
      $1 = (void *) PyString_AsString($input);
      $2 = PyString_Size($input);
    }
#else
  if (PyUnicode_Check($input))
  {
    $1 = (void *) PyUnicode_AsUTF8($input);
    $2 = PyUnicode_GET_LENGTH($input);
  }
  else if (PyBytes_Check($input))
  { 
    $1 = (void *) PyBytes_AsString($input);
    $2 = PyBytes_Size($input);
  }
#endif
  else
  {
    PyErr_SetString(PyExc_ValueError, "Expecting a string");
    return NULL;
  }
}
