
#include "PreCompiled.h"

#include "DrawSVGTemplate.h"

// inclusion of the generated files (generated out of DrawSVGTemplatePy.xml)
#include <Mod/TechDraw/App/DrawSVGTemplatePy.h>
#include <Mod/TechDraw/App/DrawSVGTemplatePy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawSVGTemplatePy::representation(void) const
{
    return std::string("<DrawSVGTemplate object>");
}

PyObject *DrawSVGTemplatePy::getCustomAttributes(const char* ) const
{
    return 0;
}

int DrawSVGTemplatePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    // search in PropertyList
    App::Property *prop = getDrawSVGTemplatePtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        short Type =  getDrawSVGTemplatePtr()->getPropertyType(prop);
        if (Type & App::Prop_ReadOnly) {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);
        return 1;
    }

    return 0;
}

PyObject* DrawSVGTemplatePy::getEditFieldContent(PyObject* args)
{
    PyObject* result = nullptr;
    char* fieldName;
    if (!PyArg_ParseTuple(args, "s",&fieldName)) {
        Base::Console().Error("Error: DrawSVGTemplatePy::getEditFieldNames - Bad Arg\n");
        return nullptr;
    }
    std::string content = getDrawSVGTemplatePtr()->EditableTexts[fieldName];
    if (!content.empty()) {
#if PY_MAJOR_VERSION < 3
        result = PyString_FromString(content.c_str());
#else
        result = PyUnicode_FromString(content.c_str());
#endif
    }
    return result;
}

PyObject* DrawSVGTemplatePy::setEditFieldContent(PyObject* args)
{
    PyObject* result = Py_True;
    char* fieldName;
    char* newContent;
    if (!PyArg_ParseTuple(args, "ss", &fieldName,&newContent)) {
        Base::Console().Error("Error: DrawSVGTemplatePy::getEditFieldNames - Bad Args\n");
        result = Py_False;
    } else {
        try {
            getDrawSVGTemplatePtr()->EditableTexts.setValue(fieldName, newContent);
        }
        catch (...) {
            result = Py_False;
        }
    }

    return result;
}
