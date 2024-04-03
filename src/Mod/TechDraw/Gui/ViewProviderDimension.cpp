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
# include <QAction>
# include <QColor>
# include <QMenu>
#endif

#include <QMessageBox>

#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Gui/ActionFunction.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/LandmarkDimension.h>

#include "PreferencesGui.h"
#include "ZVALUE.h"
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
    ADD_PROPERTY_TYPE(Arrowsize, (Preferences::dimArrowSize()),
                                     group, (App::PropertyType)(App::Prop_None),
                                                                     "Arrow size in units");
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
    ADD_PROPERTY_TYPE(GapFactorISO, (Preferences::GapISO()), group, App::Prop_None,
                      "Adjusts the gap between dimension point and extension line");
    ADD_PROPERTY_TYPE(GapFactorASME, (Preferences::GapASME()), group, App::Prop_None,
                      "Adjusts the gap between dimension point and extension line");
    ADD_PROPERTY_TYPE(LineSpacingFactorISO, (Preferences::LineSpacingISO()), group, App::Prop_None,
                      "Adjusts the gap between dimension line and dimension text");

   StackOrder.setValue(ZVALUE::DIMENSION);
}

ViewProviderDimension::~ViewProviderDimension()
{
}

void ViewProviderDimension::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDrawingView::attach(pcFeat);

//    sPixmap = "TechDraw_Dimension";
    setPixmapForType();
    if (getViewObject()->isDerivedFrom(TechDraw::LandmarkDimension::getClassTypeId())) {
        sPixmap = "TechDraw_LandmarkDimension";
    }
}

bool ViewProviderDimension::doubleClicked()
{
    startDefaultEditMode();
    return true;
}

void ViewProviderDimension::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue())));
    act->setData(QVariant((int)ViewProvider::Default));
    func->trigger(act, [this](){
        this->startDefaultEditMode();
    });

    ViewProviderDrawingView::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDimension::setEdit(int ModNum)
{
    if (ModNum != ViewProvider::Default) {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
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

void ViewProviderDimension::updateData(const App::Property* prop)
{
    if (prop == &(getViewObject()->Type)) {
        setPixmapForType();
    }

    //Dimension handles X, Y updates differently that other QGIView
    //call QGIViewDimension::updateView
    if (prop == &(getViewObject()->X)  ||
        prop == &(getViewObject()->Y)  ||
        prop == &(getViewObject()->FormatSpec) ||
        prop == &(getViewObject()->Arbitrary) ||
        prop == &(getViewObject()->FormatSpecOverTolerance) ||
        prop == &(getViewObject()->FormatSpecUnderTolerance) ||
        prop == &(getViewObject()->ArbitraryTolerances) ||
        prop == &(getViewObject()->MeasureType) ||
        prop == &(getViewObject()->TheoreticalExact) ||
        prop == &(getViewObject()->EqualTolerance) ||
        prop == &(getViewObject()->OverTolerance) ||
        prop == &(getViewObject()->UnderTolerance) ||
        prop == &(getViewObject()->Inverted) ) {

        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
        return;
    }

    //Skip QGIView X, Y processing - do not call ViewProviderDrawingView
    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderDimension::setPixmapForType()
{
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

void ViewProviderDimension::onChanged(const App::Property* p)
{
    if ((p == &Font)  ||
        (p == &Fontsize) ||
        (p == &Arrowsize) ||
        (p == &LineWidth) ||
        (p == &StandardAndStyle) ||
        (p == &RenderingExtent) ||
        (p == &FlipArrowheads) ||
        (p == &GapFactorASME) ||
        (p == &GapFactorISO) ||
        p == &LineSpacingFactorISO)  {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    if (p == &Color) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            QGIViewDimension* qgivd = dynamic_cast<QGIViewDimension*>(qgiv);
            if (qgivd) {
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

double ViewProviderDimension::prefArrowSize() const
{
    return Preferences::dimArrowSize();
}

double ViewProviderDimension::prefWeight() const
{
    return TechDraw::LineGroup::getDefaultWidth("Thin");
}

int ViewProviderDimension::prefStandardAndStyle() const
{
    return Preferences::getPreferenceGroup("Dimensions")->GetInt("StandardAndStyle", STD_STYLE_ISO_ORIENTED);
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

bool ViewProviderDimension::onDelete(const std::vector<std::string> & parms)
{
    Q_UNUSED(parms)
//    Base::Console().Message("VPB::onDelete() - parms: %d\n", parms.size());
    auto dlg = Gui::Control().activeDialog();
    auto ourDlg = dynamic_cast<TaskDlgDimension*>(dlg);
    if (ourDlg)  {
        QString bodyMessage;
        QTextStream bodyMessageStream(&bodyMessage);
        bodyMessageStream << qApp->translate("TaskDimension",
            "You cannot delete this dimension now because\nthere is an open task dialog.");
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("TaskDimension", "Can Not Delete"), bodyMessage,
            QMessageBox::Ok);
        return false;
    }
    return true;
}

