// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Shai Seger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TopoShapeViewProvider.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Mod/Part/App/TopoShape.h>

namespace CAMSimulator
{

TopoShapeViewProvider::TopoShapeViewProvider()
{
    pcSwitch = new SoSwitch;
    pcRoot->addChild(pcSwitch);

    setShapeVisible(true);
}

TopoShapeViewProvider& TopoShapeViewProvider::operator=(TopoShapeViewProvider&& vp)
{
    clear();

    pcShape = vp.pcShape;
    if (pcShape) {
        pcSwitch->addChild(pcShape);
    }

    vp.clear();

    return *this;
}

void TopoShapeViewProvider::clear()
{
    if (!pcShape) {
        return;
    }

    pcSwitch->removeChild(pcShape);
    pcShape = nullptr;
}

void TopoShapeViewProvider::setShape(const Part::TopoShape& shape)
{
    clear();

    // create SoNode from TopoShape

    std::stringstream s;
    shape.exportFaceSet(0.1f, 0.0f, {}, s);

    const auto str = s.str();
    SoInput in;
    in.setBuffer(str.data(), str.length());

    SoDB::read(&in, pcShape);
    pcSwitch->addChild(pcShape);
}

void TopoShapeViewProvider::setShapeVisible(bool b)
{
    pcSwitch->whichChild = b ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

} /* namespace CAMSimulator */
