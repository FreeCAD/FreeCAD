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

#include <Inventor/SbColor.h>
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

    SoMFInt32 highlightCoordIndex;
    SoMFInt32 selectionCoordIndex;
    SoMFInt32 faceEdgeIndex;
    SoSFColor highlightColor;
    SoSFColor selectionColor;

    void setFaceHighlight(
        int faceIndex,
        const SbColor& accentColor,
        const SbColor& haloColor,
        Gui::HighlightPresentation presentation,
        float lineWidth,
        float haloLineWidth
    );
    void clearFaceHighlight();

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
    void renderFaceHighlight(SoGLRenderAction* action);
    void renderPresentationLines(
        SoGLRenderAction* action,
        const int32_t* indices,
        int numIndices,
        const SbColor& accentColor,
        const SbColor& haloColor,
        Gui::HighlightPresentation presentation,
        float lineWidth = 0.0F,
        float haloLineWidth = 0.0F
    );
    bool validIndexes(const SoCoordinateElement*, const std::vector<int32_t>&) const;
    void appendLineCoordIndex(int lineIndex, std::vector<int32_t>& out) const;
    void appendFaceEdgeCoordIndex(int faceIndex, std::vector<int32_t>& out) const;


private:
    SelContextPtr selContext;
    SelContextPtr selContext2;
    Gui::SoFCSelectionCounter selCounter;
    SoIndexedLineSet* overlayLineSet {nullptr};

    bool faceHighlightActive {false};
    int faceHighlightIndex {-1};
    SbColor faceHighlightAccentColor;
    SbColor faceHighlightHaloColor;
    Gui::HighlightPresentation faceHighlightPresentation {Gui::HighlightPresentation::None};
    float faceHighlightLineWidth {1.0F};
    float faceHighlightHaloLineWidth {1.0F};

    // backreference to viewprovider that owns this node
    ViewProviderPartExt* viewProvider = nullptr;
};

}  // namespace PartGui
