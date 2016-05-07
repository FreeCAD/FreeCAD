#include "PreCompiled.h"

#include "Mod/Part/App/Attacher.h"

#include "OCCError.h"

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include "AttachEnginePy.h"
#include "AttachEnginePy.cpp"

using namespace Attacher;

// returns a string which represents the object e.g. when printed in python
std::string AttachEnginePy::representation(void) const
{
    return std::string("<Attacher::AttachEngine>");
}

Py::String AttachEnginePy::getAttacherType(void) const
{
    return  Py::String(std::string(this->getAttachEnginePtr()->getTypeId().getName()));
}

PyObject* AttachEnginePy::getCustomAttributes(const char*) const
{
    return 0;
}

int AttachEnginePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

