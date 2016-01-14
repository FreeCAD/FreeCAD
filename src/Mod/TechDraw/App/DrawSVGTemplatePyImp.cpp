
#include "PreCompiled.h"

#include "Mod/Drawing/App/DrawSVGTemplate.h"

// inclusion of the generated files (generated out of DrawSVGTemplatePy.xml)
#include "DrawSVGTemplatePy.h"
#include "DrawSVGTemplatePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawSVGTemplatePy::representation(void) const
{
    return std::string("<DrawSVGTemplate object>");
}

PyObject *DrawSVGTemplatePy::getCustomAttributes(const char* attr) const
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


