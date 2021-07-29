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
# include <QAction>
# include <QMenu>
# include <QMessageBox>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
#endif

#include "ViewProviderPrimitive.h"
#include "TaskPrimitiveParameters.h"
#include "Mod/Part/Gui/SoBrepFaceSet.h"
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Base/Console.h>



using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPrimitive,PartDesignGui::ViewProviderAddSub)

ViewProviderPrimitive::ViewProviderPrimitive()
{
}

ViewProviderPrimitive::~ViewProviderPrimitive()
{

}

void ViewProviderPrimitive::attach(App::DocumentObject* obj) {
    auto* prim = static_cast<PartDesign::FeaturePrimitive*>(obj);
    switch(prim->getPrimitiveType()) {
    case PartDesign::FeaturePrimitive::Box:
        sPixmap = "PartDesign_AdditiveBox.svg";
        break;
    case PartDesign::FeaturePrimitive::Cylinder:
        sPixmap = "PartDesign_AdditiveCylinder.svg";
        break;
    case PartDesign::FeaturePrimitive::Sphere:
        sPixmap = "PartDesign_AdditiveSphere.svg";
        break;
    case PartDesign::FeaturePrimitive::Cone:
        sPixmap = "PartDesign_AdditiveCone.svg";
        break;
    case PartDesign::FeaturePrimitive::Ellipsoid:
        sPixmap = "PartDesign_AdditiveEllipsoid.svg";
        break;
    case PartDesign::FeaturePrimitive::Torus:
        sPixmap = "PartDesign_AdditiveTorus.svg";
        break;
    case PartDesign::FeaturePrimitive::Prism:
        sPixmap = "PartDesign_AdditivePrism.svg";
        break;
    case PartDesign::FeaturePrimitive::Wedge:
        sPixmap = "PartDesign_AdditiveWedge.svg";
        break;
    }

    ViewProviderAddSub::attach(obj);
}

void ViewProviderPrimitive::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit primitive"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderPrimitive::getEditDialog() {
    return new TaskPrimitiveParameters(this);
}

void ViewProviderPrimitive::updateData(const App::Property* p) {
    PartDesignGui::ViewProviderAddSub::updateData(p);
}
