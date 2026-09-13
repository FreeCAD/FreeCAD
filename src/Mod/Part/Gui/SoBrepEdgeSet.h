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

#include <boost/algorithm/string/predicate.hpp>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <memory>
#include <vector>
#include <Gui/Selection/SoFCSelectionContext.h>
#include <Mod/Part/PartGlobal.h>


class SoCoordinateElement;

namespace PartGui
{

class ViewProviderPartExt;

class PartGuiExport SoBrepEdgeSet: public SoIndexedLineSet
{
    using inherited = SoIndexedLineSet;

    SO_NODE_HEADER(SoBrepEdgeSet);

public:
    static void initClass();
    SoBrepEdgeSet();

    void setViewProvider(ViewProviderPartExt* vp)
    {
        viewProvider = vp;
    }

    /*! Returned by lineIndexFromEdge() for an edge that has no rendered line. */
    static constexpr int InvalidLine = -1;

    /*! Record which topological edge each rendered polyline belongs to.
     *
     *  The line set contains a polyline only for edges that actually produced one -
     *  an edge whose Poly_PolygonOnTriangulation is null is skipped - so a Coin line
     *  index does not generally correspond to edge index + 1. lineToEdgeIn holds the
     *  1-based topological edge index of each emitted polyline, in render order.
     *  Called by ViewProviderPartExt::setupCoinGeometry().
     */
    void setEdgeMapping(std::vector<int> lineToEdgeIn);

    /*! 1-based topological edge index of a rendered line.
     *  Falls back to line + 1 when no mapping has been built yet.
     */
    int edgeIndexFromLine(int line) const;

    /*! Rendered line index of a 1-based topological edge, or InvalidLine when that
     *  edge has no rendered line. Falls back to edge - 1 when no mapping exists yet.
     */
    int lineIndexFromEdge(int edge) const;

    SoMFInt32 highlightCoordIndex;
    SoMFInt32 selectionCoordIndex;
    SoSFColor highlightColor;
    SoSFColor selectionColor;

protected:
    ~SoBrepEdgeSet() override;
    void GLRender(SoGLRenderAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void doAction(SoAction* action) override;
    SoDetail* createLineSegmentDetail(
        SoRayPickAction* action,
        const SoPrimitiveVertex* v1,
        const SoPrimitiveVertex* v2,
        SoPickedPoint* pp
    ) override;

    void getBoundingBox(SoGetBoundingBoxAction* action) override;

private:
    struct SelContext;
    using SelContextPtr = std::shared_ptr<SelContext>;

    void renderHighlight(SoGLRenderAction* action, SelContextPtr);
    void renderSelection(SoGLRenderAction* action, SelContextPtr, bool push = true);
    bool validIndexes(const SoCoordinateElement*, const std::vector<int32_t>&) const;


private:
    //! Rendered line index -> 1-based topological edge index.
    std::vector<int> lineToEdge;
    //! 1-based topological edge index -> rendered line index, InvalidLine if none.
    std::vector<int> edgeToLine;
    //! Whether setEdgeMapping() has run. An empty mapping is meaningful - it means
    //! nothing is rendered - and must be distinguished from "not built yet".
    bool hasEdgeMapping {false};

    SelContextPtr selContext;
    SelContextPtr selContext2;
    Gui::SoFCSelectionCounter selCounter;
    SoIndexedLineSet* overlayLineSet {nullptr};

    // backreference to viewprovider that owns this node
    ViewProviderPartExt* viewProvider = nullptr;
};

}  // namespace PartGui
