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
#include <Mod/TechDraw/App/DrawRichAnno.h>

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "TaskLeaderLine.h"
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
        Gui::Control().showDialog(new TaskDlgLeaderLine(this));
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
    if (!getFeature()->isRestoring())  {
        if (p == &getFeature()->LeaderParent)  {
            App::DocumentObject* docObj = getFeature()->LeaderParent.getValue();
            TechDraw::DrawView* dv = dynamic_cast<TechDraw::DrawView*>(docObj);
            if (dv != nullptr) {
                QGIView* qgiv = getQView();
                if (qgiv) {
                    qgiv->onSourceChange(dv);
                }
            }
        }
    }
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

std::vector<App::DocumentObject*> ViewProviderLeader::claimChildren(void) const
{
    // Collect any child Document Objects and put them in the right place in the Feature tree
    // valid children of a ViewLeader are:
    //    - Rich Annotations
    std::vector<App::DocumentObject*> temp;
    const std::vector<App::DocumentObject *> &views = getFeature()->getInList();
    try {
       for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
           if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawRichAnno::getClassTypeId())) {
                temp.push_back((*it));
            }
        }
        return temp;
    } 
    catch (...) {
        std::vector<App::DocumentObject*> tmp;
        return tmp;
    }
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


