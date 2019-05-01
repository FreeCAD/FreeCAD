/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawRichAnnoPy.h>  // generated from DrawRichAnnoPy.xml
#include "DrawRichAnno.h"

using namespace TechDraw;

//===========================================================================
// DrawRichAnno - movable rich text block
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawRichAnno, TechDraw::DrawView)

DrawRichAnno::DrawRichAnno(void)
{
    static const char *group = "Text Block";

    ADD_PROPERTY_TYPE(AnnoParent,(0),group,(App::PropertyType)(App::Prop_None),
                      "Object to which this annontation is attached");
    ADD_PROPERTY_TYPE(AnnoText, (""), group, App::Prop_None, "Anno text");
//    Base::Vector3d pos(0.0,0.0,0.0);
//    ADD_PROPERTY_TYPE(TextPosition, (pos), group, App::Prop_None, "Anno position relative to parent");
    ADD_PROPERTY_TYPE(ShowFrame, (true), group, App::Prop_None, "Outline rectangle on/off");
    ADD_PROPERTY_TYPE(MaxWidth, (-1.0), group, App::Prop_None, "Width limit before auto wrap");
}

DrawRichAnno::~DrawRichAnno()
{
}

void DrawRichAnno::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //nothing in particular
    }
    DrawView::onChanged(prop);

}

short DrawRichAnno::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result =  (AnnoText.isTouched());
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawRichAnno::execute(void)
{ 
//    Base::Console().Message("DRA::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    return DrawView::execute();
}

DrawView* DrawRichAnno::getBaseView(void) const
{
//    Base::Console().Message("DRA::getBaseView() - %s\n", getNameInDocument());
    DrawView* result = nullptr;
    App::DocumentObject* baseObj = AnnoParent.getValue();
    if (baseObj != nullptr) {
        DrawView* cast = dynamic_cast<DrawView*>(baseObj);
        if (cast != nullptr) {
            result = cast;
        }
    }
    return result;
}


PyObject *DrawRichAnno::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawRichAnnoPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawRichAnnoPython, TechDraw::DrawRichAnno)
template<> const char* TechDraw::DrawRichAnnoPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderRichAnno";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawRichAnno>;
}

