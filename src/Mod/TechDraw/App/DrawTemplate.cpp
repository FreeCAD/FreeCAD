/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include <Base/Console.h>

#include "DrawTemplate.h"
#include "DrawTemplatePy.h"
#include "DrawPage.h"


using namespace TechDraw;

PROPERTY_SOURCE(TechDraw::DrawTemplate, App::DocumentObject)


const char* DrawTemplate::OrientationEnums[]= {"Portrait",
                                                  "Landscape",
                                                  nullptr};

DrawTemplate::DrawTemplate()
{
    const char *group = "Page Properties";

    Orientation.setEnums(OrientationEnums);
    ADD_PROPERTY(Orientation, (0l));

    // Physical Properties inherent to every template class
    ADD_PROPERTY_TYPE(Width,     (0),  group, App::PropertyType::Prop_None, "Width of page");
    ADD_PROPERTY_TYPE(Height,    (0),  group, App::PropertyType::Prop_None, "Height of page");

    ADD_PROPERTY_TYPE(EditableTexts, (), group, App::PropertyType::Prop_None,
                      "Editable strings in the template");
}

DrawTemplate::~DrawTemplate()
{
}

PyObject *DrawTemplate::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTemplatePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

double DrawTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawTemplate::getHeight() const
{
    return Height.getValue();
}

//find the (first) DrawPage which points to this template
DrawPage* DrawTemplate::getParentPage() const
{
    TechDraw::DrawPage* page(nullptr);
    std::vector<App::DocumentObject*> parents = getInList();
    for (auto& obj : parents) {
        if (obj->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
            page = static_cast<TechDraw::DrawPage *>(obj);
            break;
        }
    }
    return page;
}

// Python Template feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTemplatePython, TechDraw::DrawTemplate)
template<> const char* TechDraw::DrawTemplatePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTemplate>;
}   // namespace App
