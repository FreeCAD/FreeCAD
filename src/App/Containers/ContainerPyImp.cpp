#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Containers/Container.h>
#include <App/PropertyContainerPy.h>

// inclusion of the generated files (generated out of ContainerPy.xml)
#include "ContainerPy.h"
#include "ContainerPy.cpp"

using namespace App;

PyObject* ContainerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of AttachEngine3D
    return new ContainerPy(new Container);
}

// constructor method
int ContainerPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PropertyContainerPy::Type), &o)) {
        PropertyContainer* cnt = static_cast<PropertyContainerPy*>(o)->getPropertyContainerPtr();
        this->_pcTwinPointer = new Container(cnt);
        return 0;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "Wrong set of constructor arguments. Can be: (), (document), (documentObject).");
    return -1;

}


// returns a string which represents the object e.g. when printed in python
std::string ContainerPy::representation(void) const
{
    std::stringstream repr;
    repr << "<App::Container around " ;
    if (getContainerBasePtr()->object())
        repr << getContainerBasePtr()->getName();
    else
        repr << "None";
    repr << ">";
    return std::string(repr.str());
}



PyObject* ContainerPy::getCustomAttributes(const char*) const
{
    return 0;
}

int ContainerPy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

