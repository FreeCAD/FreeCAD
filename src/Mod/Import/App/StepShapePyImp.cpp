
#include "PreCompiled.h"

#include "Mod/Import/App/StepShape.h"

// inclusion of the generated files (generated out of StepShapePy.xml)
#include "StepShapePy.h"
#include "StepShapePy.cpp"

using namespace Import;

// returns a string which represents the object e.g. when printed in python
std::string StepShapePy::representation(void) const
{
    return std::string("<StepShape object>");
}

PyObject *StepShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of StepShapePy and the Twin object 
    return new StepShapePy(new StepShape);
}

// constructor method
int StepShapePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    char* fileName;
    if (PyArg_ParseTuple(args, "s", &fileName)) {
        getStepShapePtr()->read(fileName);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "StepShape needs a file name\n");
    return -1;
}


PyObject* StepShapePy::read(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}





PyObject *StepShapePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int StepShapePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


