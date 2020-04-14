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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "TaskRichAnno.h"
#include "ViewProviderRichAnno.h"

using namespace TechDrawGui;

// there are only 5 frame line styles
App::PropertyIntegerConstraint::Constraints ViewProviderRichAnno::LineStyleRange = {0, 5, 1};

PROPERTY_SOURCE(TechDrawGui::ViewProviderRichAnno, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderRichAnno::ViewProviderRichAnno()
{
    sPixmap = "actions/techdraw-RichTextAnnotation";

    static const char *group = "Frame Format";

    ADD_PROPERTY_TYPE(LineWidth,(getDefLineWeight()), group,(App::PropertyType)(App::Prop_None),"Frame line width");
    ADD_PROPERTY_TYPE(LineStyle,(1),group,(App::PropertyType)(App::Prop_None),"Frame line style index");
    ADD_PROPERTY_TYPE(LineColor,(getDefLineColor()),group,App::Prop_None,"The color of the frame");

    LineStyle.setConstraints(&LineStyleRange);
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
        if (Gui::Control().activeDialog())  {         //TaskPanel already open!
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
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                 GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Markups");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    return result;
}

std::string ViewProviderRichAnno::getDefFont(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    return fontName;
}

double ViewProviderRichAnno::getDefFontSize()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    double fontSize = hGrp->GetFloat("FontSize", 5.0);
    return fontSize;
}

double ViewProviderRichAnno::getDefLineWeight(void)
{
    double result = 0.0;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);
    result = lg->getWeight("Graphics");
    delete lg;
    return result;
}

void ViewProviderRichAnno::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // property LineWidth had the App::PropertyFloat and was changed to App::PropertyLength
    if (prop == &LineWidth && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat LineWidthProperty;
        // restore the PropertyFloat to be able to set its value
        LineWidthProperty.Restore(reader);
        LineWidth.setValue(LineWidthProperty.getValue());
    }

    // property LineStyle had the App::PropertyInteger and was changed to App::PropertyIntegerConstraint
    if (prop == &LineStyle && strcmp(TypeName, "App::PropertyInteger") == 0) {
        App::PropertyInteger LineStyleProperty;
        // restore the PropertyInteger to be able to set its value
        LineStyleProperty.Restore(reader);
        LineStyle.setValue(LineStyleProperty.getValue());
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
