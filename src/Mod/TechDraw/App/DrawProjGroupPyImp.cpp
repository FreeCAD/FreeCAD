
#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>

#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"

// inclusion of the generated files (generated out of DrawProjGroupPy.xml)
#include <Mod/TechDraw/App/DrawProjGroupPy.h>
#include <Mod/TechDraw/App/DrawProjGroupPy.cpp>
#include <Mod/TechDraw/App/DrawProjGroupItemPy.h>
#include <Base/VectorPy.h>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawProjGroupPy::representation(void) const
{
    return std::string("<DrawProjGroup object>");
}

PyObject* DrawProjGroupPy::addProjection(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->addProjection(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );
    if (!newProj) {
        PyErr_SetString(PyExc_TypeError, "wrong type for adding projection");
        return nullptr;
    }

    return new DrawProjGroupItemPy(newProj);
}

PyObject* DrawProjGroupPy::removeProjection(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    int i = projGroup->removeProjection(projType);

#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong((long) i);
#else
    return PyLong_FromLong((long) i);
#endif
}

PyObject* DrawProjGroupPy::purgeProjections(PyObject* /*args*/)
{
    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    int i = projGroup->purgeProjections();

#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong((long) i);
#else
    return PyLong_FromLong((long) i);
#endif
}

PyObject* DrawProjGroupPy::getItemByLabel(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->getProjObj(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );
    if (!newProj) {
        PyErr_SetString(PyExc_TypeError, "wrong type for getting item");
        return nullptr;
    }

    return new DrawProjGroupItemPy(newProj);
}

PyObject* DrawProjGroupPy::getXYPosition(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    Base::Vector3d v = projGroup->getXYPosition(projType);
    return new Base::VectorPy(v);
}



PyObject *DrawProjGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawProjGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
