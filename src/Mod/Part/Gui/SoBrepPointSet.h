// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/nodes/SoPointSet.h>
#include <memory>
#include <vector>
#include <Gui/Selection/SoFCSelectionContext.h>
#include <Mod/Part/PartGlobal.h>


class SoCoordinateElement;
class SoIndexedPointSet;

namespace PartGui
{

class ViewProviderPartExt;

class PartGuiExport SoBrepPointSet: public SoPointSet
{
    using inherited = SoPointSet;

    SO_NODE_HEADER(SoBrepPointSet);

public:
    static void initClass();
    SoBrepPointSet();

    void setViewProvider(ViewProviderPartExt* vp)
    {
        viewProvider = vp;
    }

    SoMFInt32 highlightCoordIndex;
    SoMFInt32 selectionCoordIndex;
    SoSFColor highlightColor;
    SoSFColor selectionColor;

protected:
    ~SoBrepPointSet() override;
    void GLRender(SoGLRenderAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void doAction(SoAction* action) override;

    void getBoundingBox(SoGetBoundingBoxAction* action) override;

private:
    using SelContext = Gui::SoFCSelectionContext;
    using SelContextPtr = Gui::SoFCSelectionContextPtr;
    void renderHighlight(SoGLRenderAction* action, SelContextPtr);
    void renderSelection(SoGLRenderAction* action, SelContextPtr, bool push = true);

private:
    SelContextPtr selContext;
    SelContextPtr selContext2;
    Gui::SoFCSelectionCounter selCounter;
    SoIndexedPointSet* overlayPointSet {nullptr};

    // backreference to viewprovider that owns this node
    ViewProviderPartExt* viewProvider = nullptr;
};

}  // namespace PartGui
