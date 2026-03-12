// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <QMenu>
#include <QMessageBox>


#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>

#include "ViewProviderPrimitive.h"
#include "TaskPrimitiveParameters.h"


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPrimitive, PartDesignGui::ViewProvider)

ViewProviderPrimitive::ViewProviderPrimitive() = default;

ViewProviderPrimitive::~ViewProviderPrimitive() = default;

void ViewProviderPrimitive::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Primitive"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderPrimitive::getEditDialog()
{
    return new TaskDlgPrimitiveParameters(this);
}

QIcon ViewProviderPrimitive::getIcon() const
{

    QString str = QStringLiteral("PartDesign_");
    auto* prim = getObject<PartDesign::FeaturePrimitive>();
    if (prim->getAddSubType() == PartDesign::FeatureAddSub::Additive) {
        str += QStringLiteral("Additive");
    }
    else {
        str += QStringLiteral("Subtractive");
    }

    switch (prim->getPrimitiveType()) {
        case PartDesign::FeaturePrimitive::Box:
            str += QStringLiteral("Box");
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            str += QStringLiteral("Cylinder");
            break;
        case PartDesign::FeaturePrimitive::Sphere:
            str += QStringLiteral("Sphere");
            break;
        case PartDesign::FeaturePrimitive::Cone:
            str += QStringLiteral("Cone");
            break;
        case PartDesign::FeaturePrimitive::Ellipsoid:
            str += QStringLiteral("Ellipsoid");
            break;
        case PartDesign::FeaturePrimitive::Torus:
            str += QStringLiteral("Torus");
            break;
        case PartDesign::FeaturePrimitive::Prism:
            str += QStringLiteral("Prism");
            break;
        case PartDesign::FeaturePrimitive::Wedge:
            str += QStringLiteral("Wedge");
            break;
    }

    str += QStringLiteral(".svg");
    return PartDesignGui::ViewProvider::mergeGreyableOverlayIcons(
        Gui::BitmapFactory().pixmap(str.toStdString().c_str())
    );
}
