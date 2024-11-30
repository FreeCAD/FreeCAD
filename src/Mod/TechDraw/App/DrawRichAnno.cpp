/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include "DrawRichAnno.h"
#include "DrawRichAnnoPy.h"  // generated from DrawRichAnnoPy.xml


using namespace TechDraw;

//===========================================================================
// DrawRichAnno - movable rich text block
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawRichAnno, TechDraw::DrawView)

DrawRichAnno::DrawRichAnno()
{
    static const char *group = "Text Block";

    ADD_PROPERTY_TYPE(AnnoParent, (nullptr), group, (App::PropertyType)(App::Prop_None),
                      "Object to which this annontation is attached");
    ADD_PROPERTY_TYPE(AnnoText, (""), group, App::Prop_None, "Annotation text");
    ADD_PROPERTY_TYPE(ShowFrame, (true), group, App::Prop_None, "Outline rectangle on/off");
    ADD_PROPERTY_TYPE(MaxWidth, (-1.0), group, App::Prop_None, "Width limit before auto wrap");
    Caption.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::Hidden, true);
    ScaleType.setStatus(App::Property::Hidden, true);

}

void DrawRichAnno::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if ((prop == &AnnoText) ||
            (prop == &ShowFrame) ||
            (prop == &MaxWidth) ) {
            requestPaint();
        }
    }

    DrawView::onChanged(prop);

}

//NOTE: DocumentObject::mustExecute returns 1/0 and not true/false
short DrawRichAnno::mustExecute() const
{
    if (!isRestoring()) {
        if (AnnoText.isTouched() ||
            AnnoParent.isTouched()) {
            return 1;
        }
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawRichAnno::execute()
{
//    Base::Console().Message("DRA::execute() - @ (%.3f, %.3f)\n", X.getValue(), Y.getValue());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    overrideKeepUpdated(false);
    return DrawView::execute();
}

DrawView* DrawRichAnno::getBaseView() const
{
//    Base::Console().Message("DRA::getBaseView() - %s\n", getNameInDocument());
    return dynamic_cast<DrawView*>(AnnoParent.getValue());
}

//finds the first DrawPage in this Document that claims to own this DrawRichAnno
//note that it is possible to manipulate the Views property of DrawPage so that
//more than 1 DrawPage claims a DrawRichAnno.
DrawPage* DrawRichAnno::findParentPage() const
{
//    Base::Console().Message("DRA::findParentPage()\n");
    if (!AnnoParent.getValue()) {
        return DrawView::findParentPage();
    }

    DrawView* parent = dynamic_cast<DrawView*>(AnnoParent.getValue());
    if (parent) {
        return parent->findParentPage();
    }

    return nullptr;
}

PyObject *DrawRichAnno::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawRichAnnoPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawRichAnnoPython, TechDraw::DrawRichAnno)
template<> const char* TechDraw::DrawRichAnnoPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderRichAnno";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawRichAnno>;
}

