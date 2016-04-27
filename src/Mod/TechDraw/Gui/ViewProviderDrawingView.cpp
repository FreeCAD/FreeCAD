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
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include "ViewProviderDrawingView.h"


using namespace TechDrawGui;

PROPERTY_SOURCE(TechDrawGui::ViewProviderDrawingView, Gui::ViewProviderDocumentObject)

ViewProviderDrawingView::ViewProviderDrawingView()
{
    sPixmap = "TechDraw_Tree_View";

    // Do not show in property editor
    //DisplayMode.StatusBits.set(3, true);
    DisplayMode.setStatus(App::Property::ReadOnly,true);
}

ViewProviderDrawingView::~ViewProviderDrawingView()
{
}

void ViewProviderDrawingView::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderDrawingView::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderDrawingView::getDisplayModes(void) const
{
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();
    return StrList;
}

void ViewProviderDrawingView::show(void)
{
    ViewProviderDocumentObject::show();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<TechDraw::DrawView*>(obj)->Visible.setValue(true);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
}

void ViewProviderDrawingView::hide(void)
{
    ViewProviderDocumentObject::hide();

    App::DocumentObject* obj = getObject();
    if (!obj || obj->isRestoring())
        return;
    if (obj->getTypeId().isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
        // The 'Visible' property is marked as 'Output'. To update the drawing on recompute
        // the parent page object is touched.
        static_cast<TechDraw::DrawView*>(obj)->Visible.setValue(false);
        std::vector<App::DocumentObject*> inp = obj->getInList();
        for (std::vector<App::DocumentObject*>::iterator it = inp.begin(); it != inp.end(); ++it)
            (*it)->touch();
    }
}

bool ViewProviderDrawingView::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderDrawingView::startRestoring()
{
    // do nothing
}

void ViewProviderDrawingView::finishRestoring()
{
    // do nothing
}

void ViewProviderDrawingView::updateData(const App::Property*)
{
}

TechDraw::DrawView* ViewProviderDrawingView::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawView*>(pcObject);
}
