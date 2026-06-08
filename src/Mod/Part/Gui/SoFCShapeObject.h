// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <vector>

#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"

#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShape.h>

#include <Mod/Part/PartGlobal.h>

namespace PartGui
{

class PartGuiExport SoFCShape: public SoSeparator
{
    using inherited = SoSeparator;
    SO_NODE_HEADER(SoFCShape);

public:
    SoFCShape();
    static void initClass();

    SoCoordinate3* coords;
    SoNormal* norm;
    SoBrepFaceSet* faceset;
    SoBrepEdgeSet* lineset;
    SoBrepPointSet* nodeset;
};

class PartGuiExport SoFCControlPoints: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoFCControlPoints);

public:
    static void initClass();
    SoFCControlPoints();

    SoSFUInt32 numPolesU;
    SoSFUInt32 numPolesV;
    SoSFUInt32 numKnotsU;
    SoSFUInt32 numKnotsV;
    SoSFColor lineColor;

protected:
    ~SoFCControlPoints() override;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void generatePrimitives(SoAction* action) override;

private:
    void updateMeshCoordIndex(uint32_t nCtU, uint32_t nCtV);

    uint32_t cachedNumPolesU {0};
    uint32_t cachedNumPolesV {0};
    std::vector<int32_t> meshCoordIndex;

    SoIndexedLineSet* meshLineSet {nullptr};
    SoPointSet* polesPointSet {nullptr};
    SoPointSet* knotsPointSet {nullptr};
};

}  // namespace PartGui
