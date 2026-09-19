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

#pragma once

#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "GeometryCreationMode3D.h"

class SoMaterial;

namespace App
{
class DocumentObject;
}

namespace Sketcher3D
{
class Sketch3DObject;
}

namespace Sketcher3DGui
{

class ViewProviderSketch3D;

// Returns true if a Sketch3DObject is currently being edited.
Sketcher3DGuiExport bool isSketch3DInEdit();

// Returns the ViewProviderSketch3D of the sketch currently being edited,
Sketcher3DGuiExport ViewProviderSketch3D* getActiveSketch3DVP();

// Returns the Sketch3DObject.
Sketcher3DGuiExport Sketcher3D::Sketch3DObject* activeSketch3D();

Sketcher3DGuiExport GeometryCreationMode3D currentGeometryCreationMode3D();
Sketcher3DGuiExport bool isConstructionMode();
Sketcher3DGuiExport void toggleConstructionCreationMode();
Sketcher3DGuiExport void applyConstructionPreviewColor(SoMaterial* material);

}  // namespace Sketcher3DGui
