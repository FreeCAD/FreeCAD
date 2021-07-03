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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
//#include <Mod/TechDraw/App/Preferences.h>

#include "PreferencesGui.h"
#include "TaskSectionView.h"
#include "ViewProviderViewSection.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderViewSection, TechDrawGui::ViewProviderViewPart)

//**************************************************************************
// Construction/Destruction

ViewProviderViewSection::ViewProviderViewSection()
{
    static const char *sgroup = "Cut Surface";
    static const char *hgroup = "Surface Hatch";
    sPixmap = "TechDraw_TreeSection";
    //ShowCutSurface is obsolete - use CutSurfaceDisplay
    ADD_PROPERTY_TYPE(ShowCutSurface ,(true),sgroup,App::Prop_Hidden,"Show/hide the cut surface");
    ADD_PROPERTY_TYPE(CutSurfaceColor,(0.0,0.0,0.0),sgroup,App::Prop_None,"The color to shade the cut surface");
    //HatchCutSurface is obsolete - use CutSurfaceDisplay
    ADD_PROPERTY_TYPE(HatchCutSurface ,(false),hgroup,App::Prop_Hidden,"Hatch the cut surface");

    ADD_PROPERTY_TYPE(HatchColor,(TechDraw::DrawHatch::prefSvgHatchColor()),
                        hgroup,App::Prop_None,"The color of the Svg hatch pattern");
    ADD_PROPERTY_TYPE(GeomHatchColor,(TechDraw::DrawGeomHatch::prefGeomHatchColor()),
                        hgroup,App::Prop_None,"The color of the Geometric hatch pattern");

    ADD_PROPERTY_TYPE(WeightPattern,(0.1),hgroup,App::Prop_None,"GeomHatch pattern line thickness");

    getParameters();

}

ViewProviderViewSection::~ViewProviderViewSection()
{
}

void ViewProviderViewSection::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderViewPart::attach(pcFeat);
}

void ViewProviderViewSection::setDisplayMode(const char* ModeName)
{
    ViewProviderViewPart::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderViewSection::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderViewPart::getDisplayModes();

    return StrList;
}

//for VP properties
void ViewProviderViewSection::onChanged(const App::Property* prop)
{
    if (prop == &WeightPattern   ||
//        prop == &HatchCutSurface ||
        prop == &HatchColor      ||
        prop == &GeomHatchColor      ||
//        prop == &ShowCutSurface  ||
        prop == &CutSurfaceColor ) {
        updateGraphic();   
    }

    ViewProviderViewPart::onChanged(prop);
}

//for Feature properties
void ViewProviderViewSection::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->FileHatchPattern)   ||
        prop == &(getViewObject()->CutSurfaceDisplay)    ||
        prop == &(getViewObject()->NameGeomPattern)    ||
        prop == &(getViewObject()->HatchScale)   ) {
        updateGraphic();
    }

    ViewProviderViewPart::updateData(prop);
}

std::vector<App::DocumentObject*> ViewProviderViewSection::claimChildren(void) const
{
    return ViewProviderViewPart::claimChildren();
}

void ViewProviderViewSection::updateGraphic(void)
{
    // redraw QGIVP
    QGIView* qgiv = getQView();
    if (qgiv) {
        qgiv->updateView(true);
    }
}

bool ViewProviderViewSection::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {         //TaskPanel already open!
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgSectionView(getViewObject()));
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderViewSection::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

bool ViewProviderViewSection::doubleClicked(void)
{
    setEdit(ViewProvider::Default);
    return true;
}


void ViewProviderViewSection::getParameters(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color cutColor = App::Color((uint32_t) hGrp->GetUnsigned("CutSurfaceColor", 0xD3D3D3FF));
    CutSurfaceColor.setValue(cutColor);

//    App::Color hatchColor = App::Color((uint32_t) hGrp->GetUnsigned("SectionHatchColor", 0x00000000));
//    HatchColor.setValue(hatchColor);
  
    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/PAT"); 
    double lineWeight = hGrp->GetFloat("GeomWeight",0.1);
    WeightPattern.setValue(lineWeight);
}

bool ViewProviderViewSection::canDelete(App::DocumentObject *obj) const
{
    // a section view can be deleted
    // that its base view cannot be deleted is handled in its the onDelete() function
    Q_UNUSED(obj)
    return true;
}

TechDraw::DrawViewSection* ViewProviderViewSection::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewSection*>(pcObject);
}
