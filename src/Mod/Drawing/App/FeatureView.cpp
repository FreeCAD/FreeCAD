/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#include <Standard_Failure.hxx>
#include <sstream>
#endif

#include <Base/Exception.h>

#include "FeatureView.h"


using namespace Drawing;

//===========================================================================
// FeatureView
//===========================================================================

PROPERTY_SOURCE(Drawing::FeatureView, App::DocumentObject)


FeatureView::FeatureView(void)
{
    static const char* group = "Drawing view";
    ADD_PROPERTY_TYPE(X,
                      (0),
                      group,
                      App::Prop_None,
                      "X position of the view on the drawing in modelling units (mm)");
    ADD_PROPERTY_TYPE(Y,
                      (0),
                      group,
                      App::Prop_None,
                      "Y position of the view on the drawing in modelling units (mm)");
    ADD_PROPERTY_TYPE(Scale, (1.0), group, App::Prop_None, "Scale factor of the view");
    ADD_PROPERTY_TYPE(Rotation,
                      (0),
                      group,
                      App::Prop_None,
                      "Rotation of the view in degrees counterclockwise");
    // The 'Visible' property is handled by the view provider exclusively. It has the 'Output' flag
    // set to avoid to call the execute() method. The view provider touches the page object,
    // instead.
    App::PropertyType propType =
        static_cast<App::PropertyType>(App::Prop_Hidden | App::Prop_Output);
    ADD_PROPERTY_TYPE(Visible,
                      (true),
                      group,
                      propType,
                      "Control whether view is visible in page object");

    App::PropertyType type = (App::PropertyType)(App::Prop_Hidden);
    ADD_PROPERTY_TYPE(ViewResult, (nullptr), group, type, "Resulting SVG fragment of that view");
}

FeatureView::~FeatureView()
{}

App::DocumentObjectExecReturn* FeatureView::recompute(void)
{
    try {
        return App::DocumentObject::recompute();
    }
    catch (Standard_Failure& e) {
        App::DocumentObjectExecReturn* ret =
            new App::DocumentObjectExecReturn(e.GetMessageString());
        if (ret->Why.empty()) {
            ret->Why = "Unknown OCC exception";
        }
        return ret;
    }
}

App::DocumentObjectExecReturn* FeatureView::execute(void)
{
    return App::DocumentObject::StdReturn;
}


// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Drawing::FeatureViewPython, Drawing::FeatureView)
template<>
const char* Drawing::FeatureViewPython::getViewProviderName(void) const
{
    return "DrawingGui::ViewProviderDrawingViewPython";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<Drawing::FeatureView>;
}  // namespace App
