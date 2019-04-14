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

#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawTextLeader.h>

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "TaskTextLeader.h"
#include "ViewProviderLeader.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderLeader, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderLeader::ViewProviderLeader()
{
    sPixmap = "actions/techdraw-mline";

    static const char *group = "Line Format";

    ADD_PROPERTY_TYPE(LineWidth,(getDefLineWeight())    ,group,(App::PropertyType)(App::Prop_None),"Line weight");
    ADD_PROPERTY_TYPE(LineStyle,(1)    ,group,(App::PropertyType)(App::Prop_None),"Line style");
    ADD_PROPERTY_TYPE(Color,(getDefLineColor()),group,App::Prop_None,"The color of the Markup");
}

ViewProviderLeader::~ViewProviderLeader()
{
}

void ViewProviderLeader::attach(App::DocumentObject *pcFeat)
{
    ViewProviderDrawingView::attach(pcFeat);
}

bool ViewProviderLeader::setEdit(int ModNum)
{
//    Base::Console().Message("VPL::setEdit(%d)\n",ModNum);
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {         //TaskPanel already open!
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgTextLeader(LINEMODE, this));
        return true;
    } else {
        return ViewProviderDrawingView::setEdit(ModNum);
    }
    return true;
}

void ViewProviderLeader::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDrawingView::unsetEdit(ModNum);
    }
}

bool ViewProviderLeader::doubleClicked(void)
{
//    Base::Console().Message("VPL::doubleClicked()\n");
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderLeader::updateData(const App::Property* p)
{
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderLeader::onChanged(const App::Property* p)
{
    if ((p == &Color) ||
        (p == &LineWidth) ||
        (p == &LineStyle)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    ViewProviderDrawingView::onChanged(p);

}

TechDraw::DrawLeaderLine* ViewProviderLeader::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawLeaderLine*>(pcObject);
}

TechDraw::DrawLeaderLine* ViewProviderLeader::getFeature() const
{
    return dynamic_cast<TechDraw::DrawLeaderLine*>(pcObject);
}


double ViewProviderLeader::getDefLineWeight(void)
{
    double result = 0.0;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);
    result = lg->getWeight("Thin");
    delete lg;                                   //Coverity CID 174670
    return result;
}

App::Color ViewProviderLeader::getDefLineColor(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                 GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Markups");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("Color", 0x00000000));
    return result;
}

//******************************************************************************
PROPERTY_SOURCE(TechDrawGui::ViewProviderTextLeader, TechDrawGui::ViewProviderLeader)

ViewProviderTextLeader::ViewProviderTextLeader()
{
    sPixmap = "actions/techdraw-textleader";

    static const char *group = "Text Format";

    ADD_PROPERTY_TYPE(Font ,(getDefFont().c_str()),group,App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Fontsize,(getDefFontSize())    ,group,(App::PropertyType)(App::Prop_None),
              "Text size in internal units");
    ADD_PROPERTY_TYPE(MaxWidth,(-1)    ,group,(App::PropertyType)(App::Prop_None),
              "Maximum width of text in mm");
    ADD_PROPERTY_TYPE(ShowFrame,(true)    ,group,(App::PropertyType)(App::Prop_None),
              "Draw a box around text or not");
}

ViewProviderTextLeader::~ViewProviderTextLeader()
{
}

void ViewProviderTextLeader::attach(App::DocumentObject *pcFeat)
{
    ViewProviderLeader::attach(pcFeat);
}

bool ViewProviderTextLeader::setEdit(int ModNum)
{
//    Base::Console().Message("VPTL::setEdit(%d)\n",ModNum);
    if (ModNum == ViewProvider::Default ) {
        if (Gui::Control().activeDialog())  {         //TaskPanel already open!
            return false;
        }
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgTextLeader(TEXTMODE, this));
        return true;
    } else {
        return ViewProviderLeader::setEdit(ModNum);
    }
    return true;
}

void ViewProviderTextLeader::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderLeader::unsetEdit(ModNum);
    }
}

bool ViewProviderTextLeader::doubleClicked(void)
{
//    Base::Console().Message("VPTL::doubleClicked()\n");
    setEdit(ViewProvider::Default);
    return true;
}

void ViewProviderTextLeader::updateData(const App::Property* p)
{
    ViewProviderLeader::updateData(p);
}

void ViewProviderTextLeader::onChanged(const App::Property* p)
{
    if ((p == &Font) ||
        (p == &Fontsize) ||
        (p == &MaxWidth) ||
        (p == &ShowFrame)) {
        QGIView* qgiv = getQView();
        if (qgiv) {
            qgiv->updateView(true);
        }
    }
    ViewProviderLeader::onChanged(p);

}

TechDraw::DrawTextLeader* ViewProviderTextLeader::getFeature() const
{
    return dynamic_cast<TechDraw::DrawTextLeader*>(pcObject);
}

std::string ViewProviderTextLeader::getDefFont(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Labels");
    std::string result = hGrp->GetASCII("LabelFont", "osifont");
    return result;
}

double ViewProviderTextLeader::getDefFontSize(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    double result = hGrp->GetFloat("FontSize", 6.0);
    return result;
}              


