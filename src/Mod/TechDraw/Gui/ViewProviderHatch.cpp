/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
#include <Precision.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "TaskHatch.h"
#include "ViewProviderHatch.h"

using namespace TechDrawGui;

//scaleRange = {lowerLimit, upperLimit, stepSize}
//original range is far too broad for drawing.  causes massive loop counts.
//App::PropertyFloatConstraint::Constraints ViewProviderHatch::scaleRange = {Precision::Confusion(),
//                                                                  std::numeric_limits<double>::max(),
//                                                                  pow(10, - Base::UnitsApi::getDecimals())};
App::PropertyFloatConstraint::Constraints ViewProviderHatch::scaleRange = {pow(10, - Base::UnitsApi::getDecimals()),
                                                                  1000.0,
                                                                  0.1};


PROPERTY_SOURCE(TechDrawGui::ViewProviderHatch, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderHatch::ViewProviderHatch()
{
    sPixmap = "TechDraw_TreeHatch";

    static const char *vgroup = "Hatch";
    ADD_PROPERTY_TYPE(HatchColor, (TechDraw::DrawHatch::prefSvgHatchColor()),
                        vgroup, App::Prop_None, "The color of the hatch pattern");
    ADD_PROPERTY_TYPE(HatchScale, (1.0), vgroup, App::Prop_None, "Hatch pattern size adjustment");
    HatchScale.setConstraints(&scaleRange);
    ADD_PROPERTY_TYPE(HatchRotation, (0.0), vgroup, App::Prop_None, "Hatch pattern rotation");
    ADD_PROPERTY_TYPE(HatchOffset, (0.0, 0.0, 0.0), vgroup, App::Prop_None, "Hatch pattern offset");
}

ViewProviderHatch::~ViewProviderHatch()
{
}

bool ViewProviderHatch::setEdit(int ModNum)
{
    if (ModNum != ViewProvider::Default) {
        return Gui::ViewProviderDocumentObject::setEdit(ModNum);
    }
    if (Gui::Control().activeDialog()) {
        return false; //TaskPanel already open!
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();
    Gui::Control().showDialog(new TaskDlgHatch(this));
    return true;
}

bool ViewProviderHatch::doubleClicked()
{
    setEdit(0);
    return true;
}

void ViewProviderHatch::onChanged(const App::Property* prop)
{
    if (prop == &HatchScale ||
        prop == &HatchColor ||
        prop == &HatchRotation ||
        prop == &HatchOffset)  {
        if (HatchScale.getValue() > 0.0) {
            TechDraw::DrawViewPart* parent = getViewObject()->getSourceView();
            if (parent) {
                parent->requestPaint();
            }
        }
    }
}

void ViewProviderHatch::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->HatchPattern)) {
        TechDraw::DrawViewPart* parent = getViewObject()->getSourceView();
        if (parent) {
            parent->requestPaint();
        }
    }
    Gui::ViewProviderDocumentObject::updateData(prop);
}

TechDraw::DrawHatch* ViewProviderHatch::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawHatch*>(pcObject);
}

bool ViewProviderHatch::canDelete(App::DocumentObject *obj) const
{
    // deletion of hatches don't destroy anything
    // thus we can pass this action
    Q_UNUSED(obj)
    return true;
}

Gui::MDIView *ViewProviderHatch::getMDIView() const
{
    auto obj = getViewObject();
    if(!obj)
        return nullptr;
    auto vp = Gui::Application::Instance->getViewProvider(obj->getSourceView());
    if(!vp)
        return nullptr;
    return vp->getMDIView();
}
