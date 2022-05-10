/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/DocumentObject.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "PreferencesGui.h"
#include "QGIView.h"
#include "TaskRichAnno.h"
#include "ViewProviderRichAnno.h"

using namespace TechDrawGui;
using namespace TechDraw;

PROPERTY_SOURCE(TechDrawGui::ViewProviderRichAnno, TechDrawGui::ViewProviderDrawingView)

const char* ViewProviderRichAnno::LineStyleEnums[] = { "NoLine",
                                                  "Continuous",
                                                  "Dash",
                                                  "Dot",
                                                  "DashDot",
                                                  "DashDotDot",
                                                  nullptr };

//**************************************************************************
// Construction/Destruction

ViewProviderRichAnno::ViewProviderRichAnno()
{
    sPixmap = "actions/TechDraw_RichTextAnnotation";

    static const char *group = "Frame Format";

    ADD_PROPERTY_TYPE(LineWidth, (getDefLineWeight()), group,(App::PropertyType)(App::Prop_None), "Frame line width");
    LineStyle.setEnums(LineStyleEnums);
    ADD_PROPERTY_TYPE(LineStyle, (1), group, (App::PropertyType)(App::Prop_None), "Frame line style");
    ADD_PROPERTY_TYPE(LineColor, (getDefLineColor()), group, App::Prop_None, "The color of the frame");

}

ViewProviderRichAnno::~ViewProviderRichAnno()
{
}

void ViewProviderRichAnno::attach(App::DocumentObject *pcFeat)
{
    ViewProviderDrawingView::attach(pcFeat);
}

bool ViewProviderRichAnno::setEdit(int ModNum)
{
//    Base::Console().Message("VPRA::setEdit(%d)\n",ModNum);
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog()) { //TaskPanel already open!
            return false;
        }
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgRichAnno(this));
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderRichAnno::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

bool ViewProviderRichAnno::doubleClicked(void)
{
//    Base::Console().Message("VPRA::doubleClicked()\n");
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderRichAnno::updateData(const App::Property* p)
{
    // only if there is a frame we can enable the frame line parameters
    if (getViewObject() != nullptr) {
        if (getViewObject()->ShowFrame.getValue()) {
            LineWidth.setStatus(App::Property::ReadOnly, false);
            LineStyle.setStatus(App::Property::ReadOnly, false);
            LineColor.setStatus(App::Property::ReadOnly, false);
        }
        else {
            LineWidth.setStatus(App::Property::ReadOnly, true);
            LineStyle.setStatus(App::Property::ReadOnly, true);
            LineColor.setStatus(App::Property::ReadOnly, true);
        }
    }
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderRichAnno::onChanged(const App::Property* p)
{
    if ((p == &LineColor) ||
        (p == &LineWidth) ||
        (p == &LineStyle)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }

    ViewProviderDrawingView::onChanged(p);
}

TechDraw::DrawRichAnno* ViewProviderRichAnno::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawRichAnno*>(pcObject);
}

TechDraw::DrawRichAnno* ViewProviderRichAnno::getFeature() const
{
    return dynamic_cast<TechDraw::DrawRichAnno*>(pcObject);
}

App::Color ViewProviderRichAnno::getDefLineColor(void)
{
    return PreferencesGui::leaderColor();
}

std::string ViewProviderRichAnno::getDefFont(void)
{
    return Preferences::labelFont();
}

double ViewProviderRichAnno::getDefFontSize()
{
    return Preferences::dimFontSizeMM();
}

double ViewProviderRichAnno::getDefLineWeight(void)
{
    double result = 0.0;
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);
    result = lg->getWeight("Graphics");
    delete lg;
    return result;
}

void ViewProviderRichAnno::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }

    // property LineStyle had App::PropertyInteger and was changed to App::PropertyIntegerConstraint
    else if (prop == &LineStyle && strcmp(TypeName, "App::PropertyInteger") == 0) {
        App::PropertyInteger LineStyleProperty;
        // restore the PropertyInteger to be able to set its value
        LineStyleProperty.Restore(reader);
        LineStyle.setValue(LineStyleProperty.getValue());
    }

    // property LineStyle had App::PropertyIntegerConstraint and was changed to App::PropertyEnumeration
    else if (prop == &LineStyle && strcmp(TypeName, "App::PropertyIntegerConstraint") == 0) {
        App::PropertyIntegerConstraint LineStyleProperty;
        // restore the PropertyIntegerConstraint to be able to set its value
        LineStyleProperty.Restore(reader);
        LineStyle.setValue(LineStyleProperty.getValue());
    }

    else {
        ViewProviderDrawingView::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool ViewProviderRichAnno::canDelete(App::DocumentObject *obj) const
{
    // deletions of RichAnno objects don't destroy anything
    // thus we can pass this action
    // only for information: RichAnnos that have a parent
    // view will get the page as new parent if the view is deleted
    Q_UNUSED(obj)
    return true;
}
