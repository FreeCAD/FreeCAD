/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "TaskPrimitiveParameters.h"
#include "ViewProviderDatumCS.h"
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/Part/Gui/DlgPrimitives.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>

using namespace PartDesignGui;

TaskPrimitiveParameters::TaskPrimitiveParameters(ViewProviderPrimitive* PrimitiveView)
{
    
    assert(PrimitiveView);
    
    PartDesign::FeaturePrimitive* prm = static_cast<PartDesign::FeaturePrimitive*>(PrimitiveView->getObject());
    cs  = static_cast<PartDesign::CoordinateSystem*>(prm->CoordinateSystem.getValue());
    
    //if no coordinate system exist we need to add one, it is highly important that it exists!
    if(!cs) {
        std::string CSName = App::GetApplication().getActiveDocument()->getUniqueObjectName("CoordinateSystem");
        cs = static_cast<PartDesign::CoordinateSystem*>(
                App::GetApplication().getActiveDocument()->addObject("PartDesign::CoordinateSystem", CSName.c_str()));
        prm->CoordinateSystem.setValue(cs);
    }
            
    ViewProviderDatumCoordinateSystem* vp = static_cast<ViewProviderDatumCoordinateSystem*>(
            Gui::Application::Instance->activeDocument()->getViewProvider(cs)); 
    
    assert(vp);    
    cs_visibility = vp->isVisible();
    vp->Visibility.setValue(true);
    parameter  = new TaskDatumParameters(vp);
    Content.push_back(parameter);
}

TaskPrimitiveParameters::~TaskPrimitiveParameters()
{
    ViewProviderDatumCoordinateSystem* vp = static_cast<ViewProviderDatumCoordinateSystem*>(
            Gui::Application::Instance->activeDocument()->getViewProvider(cs)); 
    vp->setVisible(cs_visibility);
}

bool TaskPrimitiveParameters::accept()
{
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        
    return true;
}

bool TaskPrimitiveParameters::reject() {
    
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}

QDialogButtonBox::StandardButtons TaskPrimitiveParameters::getStandardButtons(void) const {
    return Gui::TaskView::TaskDialog::getStandardButtons();
}


#include "moc_TaskPrimitiveParameters.cpp"