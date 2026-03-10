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

#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"

#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
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
    ~SoFCControlPoints() override = default;
    ;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void generatePrimitives(SoAction* action) override;

private:
    void drawControlPoints(const SbVec3f*, int32_t) const;
};

}  // namespace PartGui
