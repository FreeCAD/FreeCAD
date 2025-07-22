#include "CXX/WrapPython.h"

#if PY_MAJOR_VERSION == 2
#include "Python2/cxx_exceptions.cxx"
#else
#include "Python3/cxx_exceptions.cxx"
#endif
