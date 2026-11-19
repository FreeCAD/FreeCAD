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

#include "TopoShapeViewProvider.h"
#include <Gui/View3DInventorViewer.h>

namespace CAMSimulator
{

class Dummy3DViewer: public Gui::View3DInventorViewer
{
public:
    Dummy3DViewer(QWidget* parent = nullptr);

    void cloneFrom(Dummy3DViewer& viewer);

    void setStockShape(const Part::TopoShape& shape);
    void setStockVisible(bool b);
    void setBaseShape(const Part::TopoShape& shape);
    void setBaseVisible(bool b);

protected:
    void paintEvent(QPaintEvent* event) override;

public:
    bool discardPaintEvent_ = true;

private:
    TopoShapeViewProvider stockViewProvider;
    TopoShapeViewProvider baseViewProvider;
};

}  // namespace CAMSimulator
