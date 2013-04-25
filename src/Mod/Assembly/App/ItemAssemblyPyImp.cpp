
#include "PreCompiled.h"

#include "Mod/Assembly/App/ItemAssembly.h"

// inclusion of the generated files (generated out of ItemAssemblyPy.xml)
#include "ItemAssemblyPy.h"
#include "ItemAssemblyPy.cpp"
#include <ItemPartPy.h>

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ItemAssemblyPy::representation(void) const
{
    return std::string("<ItemAssembly object>");
}


PyObject*  ItemAssemblyPy::addPart(PyObject * args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj)) {
	Base::Console().Message("Test 1 fails\n");
        return 0;
    }

    if (PyObject_TypeCheck(pcObj, &(Assembly::ItemPartPy::Type))) {
        Assembly::ItemPart *c = static_cast<Assembly::ItemPartPy*>(pcObj)->getItemPartPtr();
        getItemAssemblyPtr()->addPart(c);
	Base::Console().Message("Add Part\n");
    }
    else Base::Console().Message("Test 2 fails\n");
    Py_Return; 
}

PyObject*  ItemAssemblyPy::addComponent(PyObject * args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj)) {
	Base::Console().Message("Test 1 fails\n");
        return 0;
    }

    if (PyObject_TypeCheck(pcObj, &(Assembly::ItemAssemblyPy::Type))) {
        Assembly::ItemAssembly *c = static_cast<Assembly::ItemAssemblyPy*>(pcObj)->getItemAssemblyPtr();
        getItemAssemblyPtr()->addComponent(c);
	Base::Console().Message("Add Component\n");
    }
    else Base::Console().Message("Test 2 fails\n");
    Py_Return; 
}


PyObject *ItemAssemblyPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ItemAssemblyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


