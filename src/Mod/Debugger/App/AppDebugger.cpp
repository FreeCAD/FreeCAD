
#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include "PythonDebugger.h"

PyMODINIT_FUNC initDebugger()
{
    Debugger::PythonDebugModule::init_module();
}
