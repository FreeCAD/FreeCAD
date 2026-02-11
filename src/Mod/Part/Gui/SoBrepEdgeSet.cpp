// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif
#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif
#include <algorithm>
#include <limits>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/actions/SoSearchAction.h>

#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/Selection/Selection.h>
#include <Base/Console.h>
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "ViewProviderExt.h"

#include <Gui/Inventor/So3DAnnotation.h>


using namespace PartGui;

SO_NODE_SOURCE(SoBrepEdgeSet)

struct SoBrepEdgeSet::SelContext: Gui::SoFCSelectionContextEx
{
    std::vector<int32_t> hl, sl;
};

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
    : selContext(std::make_shared<SelContext>())
    , selContext2(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
}

void SoBrepEdgeSet::GLRender(SoGLRenderAction* action)
{
    auto state = action->getState();
    selCounter.checkRenderCache(state);

    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this, selContext, ctx2);
    if (ctx2 && ctx2->selectionIndex.empty() && ctx2->colors.empty()) {
        return;
    }


    bool hasContextHighlight = ctx && !ctx->hl.empty();
    bool hasFaceHighlight = viewProvider && viewProvider->isFaceHighlightActive();
    bool hasAnyHighlight = hasContextHighlight || hasFaceHighlight;

    if (Gui::Selection().isClarifySelectionActive()
        && !Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasAnyHighlight) {
        // if we are using clarifyselection - add this to delayed paths with priority
        // as we want to get this rendered on top of everything
        if (viewProvider) {
            viewProvider->setFaceHighlightActive(true);
        }
        Gui::SoDelayedAnnotationsElement::addDelayedPath(
            action->getState(),
            action->getCurPath()->copy(),
            200
        );
        return;
    }

    if (selContext2->checkGlobal(ctx)) {
        if (selContext2->isSelectAll()) {
            selContext2->sl.clear();
            selContext2->sl.push_back(-1);
        }
        else if (ctx) {
            selContext2->sl = ctx->sl;
        }
        if (selContext2->highlightIndex == std::numeric_limits<int>::max()) {
            selContext2->hl.clear();
            selContext2->hl.push_back(-1);
        }
        else if (ctx) {
            selContext2->hl = ctx->hl;
        }
        ctx = selContext2;
    }

    bool hasColorOverride = (ctx2 && !ctx2->colors.empty());
    if (hasColorOverride) {
        // Special handling for edge color overrides (e.g. highlighting specific edges).
        // We initially attempted to use the same logic as SoBrepFaceSet (setting
        // SoMaterialBindingElement::PER_PART_INDEXED and populating SoLazyElement arrays).
        // However, this proved brittle for SoIndexedLineSet, causing persistent crashes
        // in SoMaterialBundle/SoGLLazyElement (SIGSEGV) due to internal Coin3D state
        // mismatches when mixing Lit (default) and Unlit (highlighted) states.
        //
        // To ensure stability, we bypass the base class GLRender entirely and perform
        // a manual dual-pass render using direct OpenGL calls:
        // Pass 1: Render default lines using the current Coin3D state (Lighting enabled).
        // Pass 2: Render highlighted lines with Lighting disabled to ensure bright, flat colors.
        state->push();

        const SoCoordinateElement* coords;
        const SbVec3f* normals;
        const int32_t* cindices;
        const int32_t* nindices;
        const int32_t* tindices;
        const int32_t* mindices;
        int numcindices;
        SbBool normalCacheUsed;

        // We request normals (true) because default lines need them for lighting
        this->getVertexData(
            state,
            coords,
            normals,
            cindices,
            nindices,
            tindices,
            mindices,
            numcindices,
            true,
            normalCacheUsed
        );

        const SbVec3f* coords3d = coords->getArrayPtr3();

        // Apply the default material settings (Standard Lighting/Material)
        // This ensures default lines look correct (e.g. Black)
        SoMaterialBundle mb(action);
        mb.sendFirst();

        // We will collect highlighted segments to render them in a second pass
        // so we don't have to switch GL state constantly.
        struct HighlightSegment
        {
            int startIndex;
            Base::Color color;
        };
        std::vector<HighlightSegment> highlights;

        int linecount = 0;
        int i = 0;

        // --- PASS 1: Render Default Lines (Lit) ---
        while (i < numcindices) {
            int startIndex = i;

            // Check if this line index has an override color
            const Base::Color* pColor = nullptr;
            auto it = ctx2->colors.find(linecount);
            if (it != ctx2->colors.end()) {
                pColor = &it->second;
            }
            else {
                // Check for wildcard color
                auto it_all = ctx2->colors.find(-1);
                if (it_all != ctx2->colors.end()) {
                    pColor = &it_all->second;
                }
            }

            if (pColor) {
                // This is a highlighted line. Save it for Pass 2.
                highlights.push_back({startIndex, *pColor});

                // Skip over the indices for this line
                while (i < numcindices && cindices[i] >= 0) {
                    i++;
                }
                i++;  // skip the -1 separator
            }
            else {
                // This is a default line. Render immediately with current (Lit) state.
                glBegin(GL_LINE_STRIP);
                while (i < numcindices) {
                    int32_t idx = cindices[i++];
                    if (idx < 0) {
                        break;
                    }

                    if (idx < coords->getNum()) {
                        if (normals) {
                            glNormal3fv((const GLfloat*)(normals + idx));
                        }
                        glVertex3fv((const GLfloat*)(coords3d + idx));
                    }
                }
                glEnd();
            }
            linecount++;
        }

        // --- PASS 2: Render Highlighted Lines (Unlit) ---
        if (!highlights.empty()) {
            // Disable lighting and textures so the color is flat and bright
            glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);

            for (const auto& segment : highlights) {
                // Apply the explicit color from the map
                // Note: FreeCAD Base::Color transparency is 0.0 (opaque) to 1.0 (transparent)
                // OpenGL Alpha is 1.0 (opaque) to 0.0 (transparent)
                glColor4f(segment.color.r, segment.color.g, segment.color.b, 1.0f - segment.color.a);

                glBegin(GL_LINE_STRIP);
                int j = segment.startIndex;
                while (j < numcindices) {
                    int32_t idx = cindices[j++];
                    if (idx < 0) {
                        break;
                    }

                    if (idx < coords->getNum()) {
                        glVertex3fv((const GLfloat*)(coords3d + idx));
                    }
                }
                glEnd();
            }
            glPopAttrib();
        }

        // Do NOT call inherited::GLRender(action). We have handled all rendering manually.
        state->pop();
        return;
    }

    if (ctx && ctx->highlightIndex == std::numeric_limits<int>::max()) {
        if (ctx->selectionIndex.empty() || ctx->isSelectAll()) {
            if (ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action, ctx2);
            }
            else {
                renderHighlight(action, ctx);
            }
        }
        else {
            if (!action->isRenderingDelayedPaths()) {
                renderSelection(action, ctx);
            }
            if (ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action, ctx2);
            }
            else {
                renderHighlight(action, ctx);
            }
            if (action->isRenderingDelayedPaths()) {
                renderSelection(action, ctx);
            }
        }
        return;
    }

    if (!action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    if (ctx && !ctx->selectionIndex.empty()) {
        if (ctx->isSelectAll()) {
            if (ctx2) {
                ctx2->selectionColor = ctx->selectionColor;
                renderSelection(action, ctx2);
            }
            else if (ctx->isSelectAll()) {
                renderSelection(action, ctx);
            }
            if (action->isRenderingDelayedPaths()) {
                renderHighlight(action, ctx);
            }
            return;
        }
        if (!action->isRenderingDelayedPaths()) {
            renderSelection(action, ctx);
        }
    }
    if (ctx2 && !ctx2->selectionIndex.empty()) {
        renderSelection(action, ctx2, false);
    }
    else if (Gui::Selection().isClarifySelectionActive()
             && !Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasAnyHighlight) {
        state->push();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(false);
        glDisable(GL_DEPTH_TEST);

        inherited::GLRender(action);

        state->pop();
    }
    else {
        inherited::GLRender(action);
    }

    // Workaround for #0000433
    // #if !defined(FC_OS_WIN32)
    if (!action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    if (ctx && !ctx->selectionIndex.empty()) {
        renderSelection(action, ctx);
    }
    if (action->isRenderingDelayedPaths()) {
        renderHighlight(action, ctx);
    }
    // #endif
}

void SoBrepEdgeSet::GLRenderBelowPath(SoGLRenderAction* action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepEdgeSet::getBoundingBox(SoGetBoundingBoxAction* action)
{

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action, this);
    if (!ctx2 || (ctx2->sl.size() == 1 && ctx2->sl[0] < 0)) {
        inherited::getBoundingBox(action);
        return;
    }

    if (ctx2->sl.empty()) {
        return;
    }

    auto state = action->getState();
    auto coords = SoCoordinateElement::getInstance(state);
    const SbVec3f* coords3d = coords->getArrayPtr3();

    if (!validIndexes(coords, ctx2->sl)) {
        return;
    }

    SbBox3f bbox;

    int32_t i;
    const int32_t* cindices = &ctx2->sl[0];
    const int32_t* end = cindices + ctx2->sl.size();
    while (cindices < end) {
        bbox.extendBy(coords3d[*cindices++]);
        i = (cindices < end) ? *cindices++ : -1;
        while (i >= 0) {
            bbox.extendBy(coords3d[i]);
            i = cindices < end ? *cindices++ : -1;
        }
    }
    if (!bbox.isEmpty()) {
        action->extendBy(bbox);
    }
}

void SoBrepEdgeSet::renderShape(
    const SoGLCoordinateElement* const coords,
    const int32_t* cindices,
    int numindices
)
{

    const SbVec3f* coords3d = coords->getArrayPtr3();

    int32_t i;
    int previ;
    const int32_t* end = cindices + numindices;
    while (cindices < end) {
        glBegin(GL_LINE_STRIP);
        previ = *cindices++;
        i = (cindices < end) ? *cindices++ : -1;
        while (i >= 0) {
            glVertex3fv((const GLfloat*)(coords3d + previ));
            glVertex3fv((const GLfloat*)(coords3d + i));
            previ = i;
            i = cindices < end ? *cindices++ : -1;
        }
        glEnd();
    }
}

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction* action, SelContextPtr ctx)
{
    if (!ctx || ctx->highlightIndex < 0) {
        return;
    }

    SoState* state = action->getState();
    state->push();
    // SoLineWidthElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &ctx->highlightColor);
    packedColor = ctx->highlightColor.getPackedValue(0.0);
    SoLazyElement::setPacked(state, this, 1, &packedColor, false);

    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    const int32_t* cindices;
    int numcindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
    SbBool normalCacheUsed;

    this->getVertexData(
        state,
        coords,
        normals,
        cindices,
        nindices,
        tindices,
        mindices,
        numcindices,
        false,
        normalCacheUsed
    );

    SoMaterialBundle mb(action);
    mb.sendFirst();  // make sure we have the correct material

    int num = (int)ctx->hl.size();
    if (num > 0) {
        if (ctx->hl[0] < 0) {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
        }
        else {
            const int32_t* id = &(ctx->hl[0]);
            if (!validIndexes(coords, ctx->hl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderHighlight",
                    "highlightIndex out of range"
                );
            }
            else {
                renderShape(static_cast<const SoGLCoordinateElement*>(coords), id, num);
            }
        }
    }
    state->pop();
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction* action, SelContextPtr ctx, bool push)
{
    SoState* state = action->getState();
    if (push) {
        state->push();
        // SoLineWidthElement::set(state, this, 4.0f);

        SoLazyElement::setEmissive(state, &ctx->selectionColor);
        packedColor = ctx->selectionColor.getPackedValue(0.0);
        SoLazyElement::setPacked(state, this, 1, &packedColor, false);
    }

    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    const int32_t* cindices;
    int numcindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
    SbBool normalCacheUsed;

    this->getVertexData(
        state,
        coords,
        normals,
        cindices,
        nindices,
        tindices,
        mindices,
        numcindices,
        false,
        normalCacheUsed
    );

    SoMaterialBundle mb(action);
    mb.sendFirst();  // make sure we have the correct material

    int num = (int)ctx->sl.size();
    if (num > 0) {
        if (ctx->sl[0] < 0) {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
        }
        else {
            cindices = &(ctx->sl[0]);
            numcindices = (int)ctx->sl.size();
            if (!validIndexes(coords, ctx->sl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderSelection",
                    "selectionIndex out of range"
                );
            }
            else {
                renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
            }
        }
    }
    if (push) {
        state->pop();
    }
}

bool SoBrepEdgeSet::validIndexes(const SoCoordinateElement* coords, const std::vector<int32_t>& pts) const
{
    for (int32_t it : pts) {
        if (it >= coords->getNum()) {
            return false;
        }
    }
    return true;
}

void SoBrepEdgeSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        selCounter.checkAction(hlaction);
        if (!hlaction->isHighlighted()) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->highlightIndex = -1;
                ctx->hl.clear();
                touch();
            }
            return;
        }
        const SoDetail* detail = hlaction->getElement();
        if (!detail) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
            ctx->highlightColor = hlaction->getColor();
            ctx->highlightIndex = std::numeric_limits<int>::max();
            ctx->hl.clear();
            ctx->hl.push_back(-1);
            touch();
            return;
        }

        if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->highlightIndex = -1;
                ctx->hl.clear();
                touch();
            }
            return;
        }

        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
        ctx->highlightColor = hlaction->getColor();
        int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
        const int32_t* cindices = this->coordIndex.getValues(0);
        int numcindices = this->coordIndex.getNum();

        ctx->hl.clear();
        for (int section = 0, i = 0; i < numcindices; i++) {
            if (cindices[i] < 0) {
                if (++section > index) {
                    break;
                }
            }
            else if (section == index) {
                ctx->hl.push_back(cindices[i]);
            }
        }
        if (!ctx->hl.empty()) {
            ctx->highlightIndex = index;
        }
        else {
            ctx->highlightIndex = -1;
        }
        touch();
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);

        switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::Color:
                if (selaction->isSecondary()) {
                    const auto& colors = selaction->getColors();

                    // Case 1: The color map is empty. This is a "clear" command.
                    if (colors.empty()) {
                        // We must find and remove any existing secondary context for this node.
                        if (Gui::SoFCSelectionRoot::removeActionContext(action, this)) {
                            touch();
                        }
                        return;
                    }

                    // Case 2: The color map is NOT empty. This is a "set color" command.
                    static std::string element("Edge");
                    bool hasEdgeColors = false;
                    for (const auto& [name, color] : colors) {
                        if (name.empty() || boost::starts_with(name, element)) {
                            hasEdgeColors = true;
                            break;
                        }
                    }

                    if (hasEdgeColors) {
                        auto ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action, this);
                        selCounter.checkAction(selaction, ctx);
                        ctx->selectAll();

                        if (ctx->setColors(colors, element)) {
                            touch();
                        }
                    }
                }
                return;
            case Gui::SoSelectionElementAction::None: {
                if (selaction->isSecondary()) {
                    if (Gui::SoFCSelectionRoot::removeActionContext(action, this)) {
                        touch();
                    }
                }
                else {
                    SelContextPtr ctx
                        = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (ctx) {
                        ctx->selectionIndex.clear();
                        ctx->sl.clear();
                        ctx->colors.clear();
                        touch();
                    }
                }
                return;
            }
            case Gui::SoSelectionElementAction::All: {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                selCounter.checkAction(selaction, ctx);
                ctx->selectionColor = selaction->getColor();
                ctx->selectionIndex.clear();
                ctx->selectionIndex.insert(-1);  // all
                ctx->sl.clear();
                ctx->sl.push_back(-1);
                touch();
                return;
            }
            case Gui::SoSelectionElementAction::Append:
            case Gui::SoSelectionElementAction::Remove: {
                const SoDetail* detail = selaction->getElement();
                if (!detail || !detail->isOfType(SoLineDetail::getClassTypeId())) {
                    if (selaction->isSecondary()) {
                        // For secondary context, a detail of different type means
                        // the user may want to partial render only other type of
                        // geometry. So we call below to obtain a action context.
                        // If no secondary context exist, it will create an empty
                        // one, and an empty secondary context inhibites drawing
                        // here.
                        auto ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                        selCounter.checkAction(selaction, ctx);
                        touch();
                    }
                    return;
                }
                int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
                SelContextPtr ctx;
                if (selaction->getType() == Gui::SoSelectionElementAction::Append) {
                    ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                    selCounter.checkAction(selaction, ctx);
                    ctx->selectionColor = selaction->getColor();
                    if (ctx->isSelectAll()) {
                        ctx->selectionIndex.clear();
                    }
                    if (!ctx->selectionIndex.insert(index).second) {
                        return;
                    }
                }
                else {
                    ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (!ctx || !ctx->removeIndex(index)) {
                        return;
                    }
                }
                ctx->sl.clear();
                if (!ctx->selectionIndex.empty()) {
                    const int32_t* cindices = this->coordIndex.getValues(0);
                    int numcindices = this->coordIndex.getNum();
                    auto it = ctx->selectionIndex.begin();
                    for (int section = 0, i = 0; i < numcindices; i++) {
                        if (section == *it) {
                            ctx->sl.push_back(cindices[i]);
                        }
                        if (cindices[i] < 0) {
                            if (++section > *it) {
                                if (++it == ctx->selectionIndex.end()) {
                                    break;
                                }
                            }
                        }
                    }
                }
                touch();
                break;
            }
            default:
                break;
        }
        return;
    }

    inherited::doAction(action);
}

SoDetail* SoBrepEdgeSet::createLineSegmentDetail(
    SoRayPickAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    SoPickedPoint* pp
)
{
    SoDetail* detail = inherited::createLineSegmentDetail(action, v1, v2, pp);
    SoLineDetail* line_detail = static_cast<SoLineDetail*>(detail);
    int index = line_detail->getLineIndex();
    line_detail->setPartIndex(index);
    return detail;
}
