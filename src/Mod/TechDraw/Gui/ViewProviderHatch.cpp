/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Wandererfan <wandererfan@gmail.com>                *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
//#include <Base/Exception.h>
//#include <Base/Sequencer.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
//#include <Gui/SoFCSelection.h>
//#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include "ViewProviderHatch.h"

using namespace TechDrawGui;

//scaleRange = {lowerLimit, upperLimit, stepSize}
//original range is far too broad for drawing.  causes massive loop counts.
//App::PropertyFloatConstraint::Constraints ViewProviderHatch::scaleRange = {Precision::Confusion(),
//                                                                  std::numeric_limits<double>::max(),
//                                                                  pow(10,- Base::UnitsApi::getDecimals())};
App::PropertyFloatConstraint::Constraints ViewProviderHatch::scaleRange = {pow(10,- Base::UnitsApi::getDecimals()),
                                                                  1000.0,
                                                                  pow(10,- Base::UnitsApi::getDecimals())};


PROPERTY_SOURCE(TechDrawGui::ViewProviderHatch, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderHatch::ViewProviderHatch()
{
    sPixmap = "TechDraw_Tree_Hatch";

    static const char *vgroup = "Hatch";
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Hatch", 0x00FF0000));

    ADD_PROPERTY_TYPE(HatchColor,(fcColor),vgroup,App::Prop_None,"The color of the hatch pattern");
    ADD_PROPERTY_TYPE(HatchScale,(1.0),vgroup,App::Prop_None,"Hatch pattern size adjustment");
    HatchScale.setConstraints(&scaleRange);
}

ViewProviderHatch::~ViewProviderHatch()
{
}

void ViewProviderHatch::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderHatch::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderHatch::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

void ViewProviderHatch::onChanged(const App::Property* prop)
{
    if ((prop == &HatchScale) ||
        (prop == &HatchColor)) {
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
    Gui::ViewProviderDocumentObject::updateData(prop);
}

TechDraw::DrawHatch* ViewProviderHatch::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawHatch*>(pcObject);
}
