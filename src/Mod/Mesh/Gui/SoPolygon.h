// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/nodes/SoShape.h>
#include <Mod/Mesh/MeshGlobal.h>


namespace MeshGui
{

// NOLINTBEGIN
class MeshGuiExport SoPolygon: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoPolygon);

public:
    static void initClass();
    SoPolygon();

    SoSFInt32 startIndex;
    SoSFInt32 numVertices;
    SoSFBool highlight;
    SoSFBool render;

protected:
    ~SoPolygon() override = default;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void rayPick(SoRayPickAction* action) override;
    void generatePrimitives(SoAction* action) override;

private:
    void drawPolygon(const SbVec3f*, int32_t) const;
};
// NOLINTEND

}  // namespace MeshGui
