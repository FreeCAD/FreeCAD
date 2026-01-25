// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Inventor/nodes/SoPointSet.h>
#include <memory>
#include <vector>
#include <Gui/Selection/SoFCSelectionContext.h>
#include <Mod/Part/PartGlobal.h>


class SoCoordinateElement;
class SoGLCoordinateElement;
class SoTextureCoordinateBundle;

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

protected:
    ~SoBrepPointSet() override = default;
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
    uint32_t packedColor {0};

    // backreference to viewprovider that owns this node
    ViewProviderPartExt* viewProvider = nullptr;
};

}  // namespace PartGui
