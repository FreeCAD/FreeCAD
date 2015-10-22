#include "CXX/WrapPython.h"

#if PY_MAJOR_VERSION == 2
#include "CXX/Python2/IndirectPythonInterface.hxx"
#else
#include "CXX/Python3/IndirectPythonInterface.hxx"
#endif
