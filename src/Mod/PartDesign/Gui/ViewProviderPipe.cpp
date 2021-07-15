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
# include <QMessageBox>
# include <QMenu>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include "Utils.h"
#include "ViewProviderPipe.h"
//#include "TaskPipeParameters.h"
#include "TaskPipeParameters.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPipe,PartDesignGui::ViewProviderAddSub)

ViewProviderPipe::ViewProviderPipe()
{
}

ViewProviderPipe::~ViewProviderPipe()
{
}

std::vector<App::DocumentObject*> ViewProviderPipe::_claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;

    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(getObject());

    App::DocumentObject* sketch = pcPipe->Profile.getValue();
    if (sketch != NULL && !sketch->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        temp.push_back(sketch);


    for(App::DocumentObject* obj : pcPipe->Sections.getValues()) {
        if (obj != NULL && !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            temp.push_back(obj);
    }

    App::DocumentObject* spine = pcPipe->Spine.getValue();
    if (spine != NULL && !spine->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        temp.push_back(spine);

    App::DocumentObject* auxspine = pcPipe->AuxillerySpine.getValue();
    if (auxspine != NULL && !auxspine->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        temp.push_back(auxspine);

    return temp;
}

void ViewProviderPipe::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit pipe"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderPipe::getEditDialog() {
    return new TaskDlgPipeParameters(this, false);
}

QIcon ViewProviderPipe::getIcon(void) const {
    auto prim = Base::freecad_dynamic_cast<PartDesign::Pipe>(getObject());
    if (prim) {
        if(prim->getAddSubType() == PartDesign::FeatureAddSub::Additive)
            const_cast<ViewProviderPipe*>(this)->sPixmap = "PartDesign_AdditivePipe";
        else
            const_cast<ViewProviderPipe*>(this)->sPixmap = "PartDesign_SubtractivePipe";
    }
    return ViewProvider::getIcon();
}

