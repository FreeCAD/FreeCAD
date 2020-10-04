/***************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <QMenu>
#endif

#include "TaskPadParameters.h"
#include <Mod/PartDesign/App/FeatureExtrusion.h>
#include "ViewProviderExtrusion.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderExtrusion,PartDesignGui::ViewProviderPad)

ViewProviderExtrusion::ViewProviderExtrusion()
{
    sPixmap = "PartDesign_Extrusion.svg";
}

ViewProviderExtrusion::~ViewProviderExtrusion()
{
}

void ViewProviderExtrusion::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    // Note: This methode couldn't be unified with others because menu entry string
    //       should present united in sources for proper translation and shouldn't be 
    //       constructed on runtime.
    QAction* act;
    act = menu->addAction(QObject::tr("Edit Extrusion"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProviderSketchBased::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters *ViewProviderExtrusion::getEditDialog()
{
    return new TaskDlgPadParameters( this, false,
            "PartDesign_Extrusion",  QObject::tr("Extrusion parameters"));
}

std::vector<App::DocumentObject*>
ViewProviderExtrusion::claimChildren(void) const {
    auto feature = Base::freecad_dynamic_cast<PartDesign::ProfileBased>(getObject());
    if (!feature || feature->ClaimChildren.getValue())
        return ViewProviderPad::claimChildren();
    return {};
}
