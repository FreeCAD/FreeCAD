// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <Inventor/nodes/SoMaterial.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "Utils.h"
#include "ViewProviderSketch3D.h"

using namespace Sketcher3DGui;

namespace Sketcher3DGui
{

GeometryCreationMode3D geometryCreationMode3D = GeometryCreationMode3D::Normal;

}  // namespace Sketcher3DGui

Sketcher3DGui::GeometryCreationMode3D Sketcher3DGui::currentGeometryCreationMode3D()
{
    return geometryCreationMode3D;
}

bool Sketcher3DGui::isConstructionMode()
{
    return geometryCreationMode3D == GeometryCreationMode3D::Construction;
}

void Sketcher3DGui::toggleConstructionCreationMode()
{
    if (geometryCreationMode3D == GeometryCreationMode3D::Construction) {
        geometryCreationMode3D = GeometryCreationMode3D::Normal;
    }
    else {
        geometryCreationMode3D = GeometryCreationMode3D::Construction;
    }

    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.updateCommands("ToggleConstruction3D", static_cast<int>(geometryCreationMode3D));
}

void Sketcher3DGui::applyConstructionPreviewColor(SoMaterial* material)
{
    if (!material) {
        return;
    }
    if (isConstructionMode()) {
        material->diffuseColor.setValue(kReferenceColor[0], kReferenceColor[1], kReferenceColor[2]);
    }
    else {
        material->diffuseColor.setValue(1.0F, 1.0F, 1.0F);
    }
}

bool Sketcher3DGui::isSketch3DInEdit()
{
    return getActiveSketch3DVP() != nullptr;
}

Sketcher3DGui::ViewProviderSketch3D* Sketcher3DGui::getActiveSketch3DVP()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return nullptr;
    }
    Gui::ViewProvider* vp = doc->getInEdit();
    if (!vp) {
        return nullptr;
    }
    if (!vp->isDerivedFrom<ViewProviderSketch3D>()) {
        return nullptr;
    }
    return static_cast<ViewProviderSketch3D*>(vp);
}
