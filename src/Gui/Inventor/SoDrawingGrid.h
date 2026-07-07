// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <Inventor/SbVec2s.h>
#include <FCGlobal.h>

#include "SoFCScreenSpaceGroup.h"

class SoLineSet;
class SoState;
class SoVertexProperty;

namespace Gui
{
namespace Inventor
{

class GuiExport SoDrawingGrid: public SoFCScreenSpaceGroup
{
    using inherited = SoFCScreenSpaceGroup;

    SO_NODE_HEADER(SoDrawingGrid);

public:
    static void initClass();
    SoDrawingGrid();

public:
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;
    void GLRenderOffPath(SoGLRenderAction* action) override;
    void doAction(SoAction* action) override;

private:
    void ensureGeometry(SoState* state);
    // Force using the reference count mechanism.
    ~SoDrawingGrid() override;

private:
    SoVertexProperty* m_VertexProperty {nullptr};
    SoLineSet* m_LineSet {nullptr};
    SbVec2s m_CachedViewportSize {0, 0};
};

}  // namespace Inventor

}  // namespace Gui
