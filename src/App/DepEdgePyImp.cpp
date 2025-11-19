#include <App/DepEdge.h>
#include <App/DocumentObject.h>

#include <App/DepEdgePy.h>
#include <App/DepEdgePy.cpp>

using namespace App;

std::string DepEdgePy::representation() const
{
    std::stringstream str;
    auto& [fromObj, fromProp, toObj, toProp] = *getDepEdgePtr();

    str << fromObj->getFullName() << "." << (fromProp.empty() ? "HEAD" : fromProp)
        << " --> "
        << toObj->getFullName() << "." << (toProp.empty() ? "HEAD" : toProp);
    return {str.str()};
}

Py::Object DepEdgePy::getFromObj() const
{
    return Py::Object(getDepEdgePtr()->fromObj->getPyObject(), true);
}

Py::String DepEdgePy::getFromProp() const
{
    return Py::String(getDepEdgePtr()->fromProp);
}

Py::Object DepEdgePy::getToObj() const
{
    return Py::Object(getDepEdgePtr()->toObj->getPyObject(), true);
}

Py::String DepEdgePy::getToProp() const
{
    return Py::String(getDepEdgePtr()->toProp);
}

PyObject* DepEdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return Py_None;
}

int DepEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
