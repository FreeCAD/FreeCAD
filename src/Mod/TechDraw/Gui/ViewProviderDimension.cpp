/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/LineGroup.h>


#include "ViewProviderDimension.h"

using namespace TechDrawGui;

const char *ViewProviderDimension::StandardAndStyleEnums[]=
    { "ISO Oriented", "ISO Referencing", "ASME Inlined", "ASME Referencing", NULL };

const char *ViewProviderDimension::RenderingExtentEnums[]=
    { "None", "Minimal", "Confined", "Reduced", "Normal", "Expanded", NULL };

PROPERTY_SOURCE(TechDrawGui::ViewProviderDimension, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderDimension::ViewProviderDimension()
{
    sPixmap = "TechDraw_Dimension";

    static const char *group = "Dim Format";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");

    hGrp = App::GetApplication().GetUserParameter()
                                         .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double fontSize = hGrp->GetFloat("FontSize", QGIView::DefaultFontSizeInMM);
    ADD_PROPERTY_TYPE(Font ,(fontName.c_str()),group,App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize,(fontSize)    ,group,(App::PropertyType)(App::Prop_None),"Dimension text size in units");


    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);
    double weight = lg->getWeight("Thin");
    delete lg;                                   //Coverity CID 174670
    ADD_PROPERTY_TYPE(LineWidth,(weight)    ,group,(App::PropertyType)(App::Prop_None),"Dimension line weight");


    hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    ADD_PROPERTY_TYPE(Color,(fcColor),group,App::Prop_None,"The color of the Dimension");

    int standardStyle = hGrp->GetInt("StandardAndStyle", STD_STYLE_ISO_ORIENTED);
    ADD_PROPERTY_TYPE(StandardAndStyle, (standardStyle), group, App::Prop_None, "Specifies the standard according to which this dimension is drawn");
    StandardAndStyle.setEnums(StandardAndStyleEnums);

    ADD_PROPERTY_TYPE(RenderingExtent, (REND_EXTENT_NORMAL),  group, App::Prop_None,"Select the rendering mode by space requirements");
    RenderingExtent.setEnums(RenderingExtentEnums);
    ADD_PROPERTY_TYPE(FlipArrowheads, (false), group, App::Prop_None,"Reverts the usual direction of dimension line terminators");
}

ViewProviderDimension::~ViewProviderDimension()
{
}

void ViewProviderDimension::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);
}

void ViewProviderDimension::setDisplayMode(const char* ModeName)
{
    ViewProviderDrawingView::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDimension::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDrawingView::getDisplayModes();

    return StrList;
}

void ViewProviderDimension::updateData(const App::Property* p)
{
    if (p == &(getViewObject()->Type)) {
        if (getViewObject()->Type.isValue("DistanceX")) {
            sPixmap = "TechDraw_HorizontalDimension";
        } else if (getViewObject()->Type.isValue("DistanceY")) {
            sPixmap = "TechDraw_VerticalDimension";
        } else if (getViewObject()->Type.isValue("Radius")) {
            sPixmap = "TechDraw_RadiusDimension";
        } else if (getViewObject()->Type.isValue("Diameter")) {
            sPixmap = "TechDraw_DiameterDimension";
        } else if (getViewObject()->Type.isValue("Angle")) {
            sPixmap = "TechDraw_AngleDimension";
        } else if (getViewObject()->Type.isValue("Angle3Pt")) {
            sPixmap = "TechDraw_3PtAngleDimension";
        }
    }
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderDimension::onChanged(const App::Property* p)
{
    if ((p == &Font)  ||
        (p == &Fontsize) ||
        (p == &LineWidth) ||
        (p == &StandardAndStyle) ||
        (p == &RenderingExtent) ||
        (p == &FlipArrowheads))
 {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    ViewProviderDrawingView::onChanged(p);
}

TechDraw::DrawViewDimension* ViewProviderDimension::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewDimension*>(pcObject);
}
