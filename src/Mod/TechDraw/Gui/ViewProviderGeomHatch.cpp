/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2017 Wandererfan <wandererfan@gmail.com>                *
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
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawView.h>
#include "ViewProviderDrawingView.h"
#include "ViewProviderGeomHatch.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderGeomHatch, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderGeomHatch::ViewProviderGeomHatch()
{
    static const char *vgroup = "Format";

    sPixmap = "actions/techdraw-geomhatch";

    ADD_PROPERTY_TYPE(ColorPattern,(0),vgroup,App::Prop_None,"The color of the pattern");
    ADD_PROPERTY_TYPE(WeightPattern,(0.1),vgroup,App::Prop_None,"GeomHatch pattern line thickness");

    getParameters();

}

ViewProviderGeomHatch::~ViewProviderGeomHatch()
{
}

void ViewProviderGeomHatch::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderGeomHatch::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderGeomHatch::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

//for VP properties
void ViewProviderGeomHatch::onChanged(const App::Property* prop)
{
    if (prop == &WeightPattern   ||
        prop == &ColorPattern ) {
        updateGraphic();   
    }

    Gui::ViewProviderDocumentObject::onChanged(prop);
}

//for feature properties
void ViewProviderGeomHatch::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->ScalePattern)) {
        updateGraphic();
    }
    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderGeomHatch::updateGraphic(void)
{
    TechDraw::DrawGeomHatch* dc = getViewObject();
    if (dc) {
        TechDraw::DrawViewPart* dvp = dc->getSourceView();
        if (dvp) {
            Gui::ViewProvider* view = Gui::Application::Instance->getDocument(dvp->getDocument())->getViewProvider(dvp);
            TechDrawGui::ViewProviderDrawingView* vpDV = dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(view);
            if (vpDV) {
                vpDV->show();
                QGIView* qgiv = vpDV->getQView();
                if (qgiv) {
                    qgiv->updateView(true);
                }
            }
        }
   }
}


void ViewProviderGeomHatch::getParameters(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("GeomHatch", 0x00000000));
    ColorPattern.setValue(fcColor);
   
    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/PAT"); 
    double lineWeight = hGrp->GetFloat("GeomWeight",0.1);
    WeightPattern.setValue(lineWeight);
}

TechDraw::DrawGeomHatch* ViewProviderGeomHatch::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawGeomHatch*>(pcObject);
}
