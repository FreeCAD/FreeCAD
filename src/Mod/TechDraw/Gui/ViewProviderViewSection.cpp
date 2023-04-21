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

#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/Preferences.h>


#include "TaskSectionView.h"
#include "TaskComplexSection.h"
#include "ViewProviderViewSection.h"
#include "QGIView.h"


using namespace TechDraw;
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
    ADD_PROPERTY_TYPE(ShowCutSurface ,(true), sgroup, App::Prop_Hidden, "Show/hide the cut surface");
    ADD_PROPERTY_TYPE(CutSurfaceColor, (0.0, 0.0, 0.0), sgroup, App::Prop_None, "The color to shade the cut surface");
    //HatchCutSurface is obsolete - use CutSurfaceDisplay
    ADD_PROPERTY_TYPE(HatchCutSurface ,(false), hgroup, App::Prop_Hidden, "Hatch the cut surface");

    ADD_PROPERTY_TYPE(HatchColor, (TechDraw::DrawHatch::prefSvgHatchColor()),
                        hgroup, App::Prop_None, "The color of the Svg hatch pattern");
    ADD_PROPERTY_TYPE(GeomHatchColor, (TechDraw::DrawGeomHatch::prefGeomHatchColor()),
                        hgroup, App::Prop_None, "The color of the Geometric hatch pattern");

    ADD_PROPERTY_TYPE(WeightPattern, (0.1), hgroup, App::Prop_None, "GeomHatch pattern line thickness");

    getParameters();

}

ViewProviderViewSection::~ViewProviderViewSection()
{
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
        prop == &(getViewObject()->HatchScale)  ||
        prop == &(getViewObject()->HatchRotation) ) {
        updateGraphic();
    }

    ViewProviderViewPart::updateData(prop);
}

void ViewProviderViewSection::updateGraphic()
{
    // redraw QGIVP
    QGIView* qgiv = getQView();
    if (qgiv) {
        qgiv->updateView(true);
    }
}

bool ViewProviderViewSection::setEdit(int ModNum)
{
    if (ModNum != ViewProvider::Default ) {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    if (Gui::Control().activeDialog())  {         //TaskPanel already open!
        return false;
    }
    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    auto dcs = dynamic_cast<TechDraw::DrawComplexSection*>(getViewObject());
    if (dcs) {
        Gui::Control().showDialog(new TaskDlgComplexSection(dcs));
        return true;
    }
    Gui::Control().showDialog(new TaskDlgSectionView(getViewObject()));
    return true;
}

bool ViewProviderViewSection::doubleClicked()
{
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderViewSection::getParameters()
{
    App::Color cutColor = App::Color((uint32_t) Preferences::getPreferenceGroup("Colors")->GetUnsigned("CutSurfaceColor", 0xD3D3D3FF));
    CutSurfaceColor.setValue(cutColor);

//    App::Color hatchColor = App::Color((uint32_t) hGrp->GetUnsigned("SectionHatchColor", 0x00000000));
//    HatchColor.setValue(hatchColor);

    double lineWeight = Preferences::getPreferenceGroup("PAT")->GetFloat("GeomWeight", 0.1);
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
