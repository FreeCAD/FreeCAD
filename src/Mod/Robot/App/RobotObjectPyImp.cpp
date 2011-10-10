
#include "PreCompiled.h"

#include "Mod/Robot/App/RobotObject.h"

// inclusion of the generated files (generated out of RobotObjectPy.xml)
#include "RobotObjectPy.h"
#include "RobotObjectPy.cpp"

using namespace Robot;

// returns a string which represents the object e.g. when printed in python
std::string RobotObjectPy::representation(void) const
{
    return std::string("<RobotObject object>");
}



PyObject* RobotObjectPy::getRobot(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}



PyObject *RobotObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int RobotObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


