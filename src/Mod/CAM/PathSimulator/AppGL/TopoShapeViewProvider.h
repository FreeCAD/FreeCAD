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

#pragma once

#include <Gui/ViewProvider.h>

namespace Part
{
class TopoShape;
}

namespace CAMSimulator
{

class TopoShapeViewProvider: public Gui::ViewProvider
{
public:
    TopoShapeViewProvider();
    TopoShapeViewProvider& operator=(TopoShapeViewProvider&& vp);

    void clear();
    void setShape(const Part::TopoShape& shape);

    void setShapeVisible(bool b);

private:
    SoSwitch* pcSwitch = nullptr;
    SoNode* pcShape = nullptr;
};

}  // namespace CAMSimulator
