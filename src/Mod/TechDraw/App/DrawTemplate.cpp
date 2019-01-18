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

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>

#include <App/Application.h>

#include <iostream>
#include <iterator>

#include "DrawPage.h"
#include "DrawTemplate.h"
#include <Mod/TechDraw/App/DrawTemplatePy.h>

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawTemplate, App::DocumentObject)


const char* DrawTemplate::OrientationEnums[]= {"Portrait",
                                                  "Landscape",
                                                  NULL};



DrawTemplate::DrawTemplate(void)
{
    const char *group = "Page Properties";

    Orientation.setEnums(OrientationEnums);
    ADD_PROPERTY(Orientation, ((long)0));

    // Physical Properties inherent to every template class
    ADD_PROPERTY_TYPE(Width,     (0),  group, (App::PropertyType)(App::Prop_None), "Width of page");
    ADD_PROPERTY_TYPE(Height,    (0),  group, (App::PropertyType)(App::Prop_None), "Height of page");
    //ADD_PROPERTY_TYPE(PaperSize, (""), group, (App::PropertyType)(App::Prop_None), "Paper Format");   //obs?

    ADD_PROPERTY_TYPE(EditableTexts, (), group, (App::PropertyType)(App::Prop_None),
                      "Editable strings in the template");
}

DrawTemplate::~DrawTemplate()
{
  Base::Console().Log("template destroyed");
}


PyObject *DrawTemplate::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTemplatePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int DrawTemplate::getMemSize(void) const
{
    return 0;
}

double DrawTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawTemplate::getHeight() const
{
    return Height.getValue();
}

short DrawTemplate::mustExecute() const
{
    return App::DocumentObject::mustExecute();
}

/// get called by the container when a Property was changed
void DrawTemplate::onChanged(const App::Property* prop)
{
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawTemplate::execute(void)
{
    DrawPage *page = 0;
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
            page = dynamic_cast<TechDraw::DrawPage *>(*it);
        }
    }

    if(page) {
        page->Template.touch();     //if you are on a page, execute yourself???
    }

    return App::DocumentObject::execute();
}

void DrawTemplate::getBlockDimensions(double & /*x*/, double & /*y*/, double & /*width*/, double & /*height*/) const
{
    throw Base::NotImplementedError("implement in virtual function");
}

DrawPage* DrawTemplate::getParentPage() const
{
    TechDraw::DrawPage* page = nullptr;
    std::vector<App::DocumentObject*> parent = getInList();
    for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
            page = static_cast<TechDraw::DrawPage *>(*it);
        }
    }
    return page;
}

// Python Template feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTemplatePython, TechDraw::DrawTemplate)
template<> const char* TechDraw::DrawTemplatePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTemplate>;
}   // namespace App
