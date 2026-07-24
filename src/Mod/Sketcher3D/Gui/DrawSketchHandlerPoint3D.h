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

#include "DrawSketchHandler3D.h"

class SoCoordinate3;
class SoMaterial;

namespace Sketcher3DGui
{

class Sketcher3DGuiExport DrawSketchHandlerPoint3D: public DrawSketchHandler3D
{
public:
    DrawSketchHandlerPoint3D();
    ~DrawSketchHandlerPoint3D() override;

    bool pressButton(const Base::Vector3d& pos) override;
    bool mouseMove(const Base::Vector3d& pos) override;

protected:
    void onActivated() override;

private:
    SoCoordinate3* previewCoords {nullptr};
    SoMaterial* previewMaterial {nullptr};
};

}  // namespace Sketcher3DGui
