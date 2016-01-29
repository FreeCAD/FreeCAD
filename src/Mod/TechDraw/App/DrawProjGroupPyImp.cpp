
#include "PreCompiled.h"

#include <App/DocumentObject.h>

#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"

// inclusion of the generated files (generated out of DrawProjGroupPy.xml)
#include "DrawProjGroupPy.h"
#include "DrawProjGroupPy.cpp"

#include "DrawProjGroupItemPy.h"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawProjGroupPy::representation(void) const
{
    return std::string("<DrawProjGroup object>");
}

PyObject* DrawProjGroupPy::addProjection(PyObject* args)
{
    const char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        Base::Console().Error("Error: DrawProjGroupPy::addProjection - Bad Arg - not string\n");
        return NULL;
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->addProjection(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );

    return new DrawProjGroupItemPy(newProj);
}

PyObject* DrawProjGroupPy::removeProjection(PyObject* args)
{
    const char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        Base::Console().Error("Error: DrawProjGroupPy::removeProjection - Bad Arg - not string\n");
        return NULL;
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    int i = projGroup->removeProjection(projType);

    return PyInt_FromLong((long) i);;
}

PyObject* DrawProjGroupPy::getItemByLabel(PyObject* args)
{
    const char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        Base::Console().Error("Error: DrawProjGroupPy::getItemByLabel - Bad Arg - not string\n");
        return NULL;
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->getProjObj(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );

    return new DrawProjGroupItemPy(newProj);
}

PyObject *DrawProjGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawProjGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
