﻿/***************************************************************************
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
# include <QAction>
# include <QColor>
# include <QMenu>
#endif

#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Gui/ActionFunction.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/LandmarkDimension.h>

#include "PreferencesGui.h"
#include "TaskDimension.h"
#include "QGIViewDimension.h"
#include "ViewProviderDimension.h"

using namespace TechDrawGui;
using namespace TechDraw;

const char *ViewProviderDimension::StandardAndStyleEnums[]=
    { "ISO Oriented", "ISO Referencing", "ASME Inlined", "ASME Referencing", nullptr };

const char *ViewProviderDimension::RenderingExtentEnums[]=
    { "None", "Minimal", "Confined", "Reduced", "Normal", "Expanded", nullptr };

PROPERTY_SOURCE(TechDrawGui::ViewProviderDimension, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderDimension::ViewProviderDimension()
{
    sPixmap = "TechDraw_Dimension";

    static const char *group = "Dimension Format";

    ADD_PROPERTY_TYPE(Font, (Preferences::labelFont().c_str()),
                                              group, App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize, (Preferences::dimFontSizeMM()), 
    								 group, (App::PropertyType)(App::Prop_None),
                                                                     "Dimension text size in units");
    ADD_PROPERTY_TYPE(LineWidth, (prefWeight()), group, (App::PropertyType)(App::Prop_None), 
                                                        "Dimension line width");
    ADD_PROPERTY_TYPE(Color, (prefColor()), group, App::Prop_None, "Color of the dimension");
    ADD_PROPERTY_TYPE(StandardAndStyle, (prefStandardAndStyle()), group, App::Prop_None, 
                                        "Standard and style according to which dimension is drawn");
    StandardAndStyle.setEnums(StandardAndStyleEnums);

    ADD_PROPERTY_TYPE(RenderingExtent, (REND_EXTENT_NORMAL), group, App::Prop_None,
                                         "Select the rendering mode by space requirements");
    RenderingExtent.setEnums(RenderingExtentEnums);
    ADD_PROPERTY_TYPE(FlipArrowheads, (false), group, App::Prop_None,
                                          "Reverses usual direction of dimension line terminators");
}

ViewProviderDimension::~ViewProviderDimension()
{
}

void ViewProviderDimension::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);

    sPixmap = "TechDraw_Dimension";
    if (getViewObject()->isDerivedFrom(TechDraw::LandmarkDimension::getClassTypeId())) {
        sPixmap = "TechDraw_LandmarkDimension";
    }
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

bool ViewProviderDimension::doubleClicked(void)
{
    startDefaultEditMode();
    return true;
}

void ViewProviderDimension::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue())));
    act->setData(QVariant((int)ViewProvider::Default));
    func->trigger(act, boost::bind(&ViewProviderDimension::startDefaultEditMode, this));

    ViewProviderDrawingView::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDimension::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (Gui::Control().activeDialog()) { // if TaskPanel already open
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        auto qgivDimension(dynamic_cast<QGIViewDimension*>(getQView()));
        if (qgivDimension) {
            Gui::Control().showDialog(new TaskDlgDimension(qgivDimension, this));
        }
        return true;
    }
    else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderDimension::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
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
    if (p == &Color) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(qgiv);
            if (qgivd != nullptr) {
                qgivd->setNormalColorAll();
            }
        }
    }

    ViewProviderDrawingView::onChanged(p);
}

TechDraw::DrawViewDimension* ViewProviderDimension::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawViewDimension*>(pcObject);
}

App::Color ViewProviderDimension::prefColor() const
{
   return PreferencesGui::dimColor();
}

std::string ViewProviderDimension::prefFont() const
{
    return Preferences::labelFont();
}

double ViewProviderDimension::prefFontSize() const
{
    return Preferences::dimFontSizeMM();
}

double ViewProviderDimension::prefWeight() const
{
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);
    double weight = lg->getWeight("Thin");
    delete lg;                                   //Coverity CID 174670
    return weight;
}

int ViewProviderDimension::prefStandardAndStyle() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    int standardStyle = hGrp->GetInt("StandardAndStyle", STD_STYLE_ISO_ORIENTED);
    return standardStyle;
}

void ViewProviderDimension::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }
    else {
        ViewProviderDrawingView::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool ViewProviderDimension::canDelete(App::DocumentObject *obj) const
{
    // deletions of dimension objects don't destroy anything
    // thus we can pass this action
    Q_UNUSED(obj)
    return true;
}
