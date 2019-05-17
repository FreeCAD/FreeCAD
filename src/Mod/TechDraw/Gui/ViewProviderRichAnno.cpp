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

#include "MDIViewPage.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "TaskRichAnno.h"
#include "ViewProviderRichAnno.h"

using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderRichAnno, TechDrawGui::ViewProviderDrawingView)

//**************************************************************************
// Construction/Destruction

ViewProviderRichAnno::ViewProviderRichAnno()
{
    sPixmap = "actions/techdraw-textleader";

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
        // clear the selection (convenience)
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
    ViewProviderDrawingView::updateData(p);
}

void ViewProviderRichAnno::onChanged(const App::Property* p)
{
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
