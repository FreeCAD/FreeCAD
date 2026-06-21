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

#include <FCConfig.h>

#include <algorithm>
#include <limits>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/actions/SoSearchAction.h>

#include <Gui/Selection/SelectionColors.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Base/Color.h>
#include "SoBrepEdgeSet.h"
#include "ViewProviderExt.h"

#include <Gui/Inventor/So3DAnnotation.h>


using namespace PartGui;

SO_NODE_SOURCE(SoBrepEdgeSet)

namespace
{
constexpr float defaultPresentationLineWidth = 3.0F;
constexpr float defaultPresentationHaloLineWidth = 5.0F;
}  // namespace

struct SoBrepEdgeSet::SelContext: Gui::SoFCSelectionContextEx
{
    std::vector<int32_t> hl, sl;
};

/// Controls how B-rep overlay primitives interact with the scene depth buffer.
enum class OverlayDepthMode
{
    /// Keep normal occlusion so committed selection does not expose hidden geometry.
    RespectDepth,
    /// Render above model geometry for hover and preselection feedback.
    DrawOnTop,
};

static void applyOverlayPrimitiveState(SoState* state, SoNode* node)
{
    if (!state || !node) {
        return;
    }

    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoTextureEnabledElement::set(state, node, false);
    SoMaterialBindingElement::set(state, SoMaterialBindingElement::OVERALL);
    SoOverrideElement::setMaterialBindingOverride(state, node, true);
}

static void applyOverlayDepthState(SoState* state, OverlayDepthMode depthMode)
{
    switch (depthMode) {
        case OverlayDepthMode::DrawOnTop:
            SoDepthBufferElement::set(
                state,
                FALSE,
                FALSE,
                SoDepthBufferElement::ALWAYS,
                SbVec2f(0.0f, 1.0f)
            );
            return;
        case OverlayDepthMode::RespectDepth:
            SoDepthBufferElement::set(
                state,
                TRUE,
                FALSE,
                SoDepthBufferElement::LEQUAL,
                SbVec2f(0.0f, 1.0f)
            );
            return;
    }
}

static void renderOverlayLines(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const Base::Color& color,
    OverlayDepthMode depthMode,
    float lineWidth = 0.0F
)
{
    if (!action || !lineSet || !indices || numIndices <= 0) {
        return;
    }

    // Match the legacy GL path by drawing each edge segment independently.
    std::vector<int32_t> lineIndices;
    lineIndices.reserve(static_cast<size_t>(numIndices) * 3);

    int32_t previous = -1;
    for (int i = 0; i < numIndices; i++) {
        const int32_t current = indices[i];
        if (current < 0) {
            previous = -1;
            continue;
        }
        if (previous >= 0) {
            lineIndices.push_back(previous);
            lineIndices.push_back(current);
            lineIndices.push_back(-1);
        }
        previous = current;
    }

    if (lineIndices.empty()) {
        return;
    }

    auto state = action->getState();
    state->push();

    applyOverlayPrimitiveState(state, lineSet);
    applyOverlayDepthState(state, depthMode);
    if (lineWidth > 0.0F) {
        SoLineWidthElement::set(state, lineSet, lineWidth);
    }

    const SbColor sbColor(color.r, color.g, color.b);
    const float transparency = std::max(0.0f, 1.0f - color.a);
    const bool hasTransparency = transparency > 0.0f;
    if (hasTransparency) {
        SoShapeStyleElement::setTransparencyType(state, SoGLRenderAction::BLEND);
        SoLazyElement::setTransparencyType(state, SoGLRenderAction::BLEND);
    }

    SoLazyElement::setEmissive(state, &sbColor);
    uint32_t packedColor = sbColor.getPackedValue(transparency);
    SoLazyElement::setPacked(state, lineSet, 1, &packedColor, hasTransparency);

    // setValues() does not shrink the field, so rewrite the overlay index
    // array to the exact size to avoid stale segments from the previous
    // highlight.
    lineSet->coordIndex.setNum(static_cast<int>(lineIndices.size()));
    int32_t* coordIndex = lineSet->coordIndex.startEditing();
    std::copy(lineIndices.begin(), lineIndices.end(), coordIndex);
    lineSet->coordIndex.finishEditing();
    lineSet->GLRender(action);

    state->pop();
}

static void renderOverlayLines(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const SbColor& color,
    OverlayDepthMode depthMode,
    float lineWidth = 0.0F
)
{
    renderOverlayLines(
        action,
        lineSet,
        indices,
        numIndices,
        Base::Color(color[0], color[1], color[2], 1.0f),
        depthMode,
        lineWidth
    );
}

static void renderColorOverrides(
    SoGLRenderAction* action,
    SoIndexedLineSet* lineSet,
    const int32_t* indices,
    int numIndices,
    const std::map<int, Base::Color>& colors
)
{
    if (!action || !lineSet || !indices || numIndices <= 0 || colors.empty()) {
        return;
    }

    struct ColorGroup
    {
        Base::Color color;
        std::vector<int32_t> indices;
    };

    std::map<uint32_t, ColorGroup> colorGroups;
    const auto wildcard = colors.find(-1);

    int lineIndex = 0;
    for (int i = 0; i < numIndices; ++lineIndex) {
        const int sectionStart = i;
        while (i < numIndices && indices[i] >= 0) {
            ++i;
        }

        const Base::Color* color = nullptr;
        auto it = colors.find(lineIndex);
        if (it != colors.end()) {
            color = &it->second;
        }
        else if (wildcard != colors.end()) {
            color = &wildcard->second;
        }

        if (color) {
            const SbColor sbColor(color->r, color->g, color->b);
            const uint32_t key = sbColor.getPackedValue(std::max(0.0f, 1.0f - color->a));
            auto& group = colorGroups[key];
            if (group.indices.empty()) {
                group.color = *color;
            }
            group.indices.insert(group.indices.end(), indices + sectionStart, indices + i);
            group.indices.push_back(-1);
        }

        if (i < numIndices && indices[i] < 0) {
            ++i;
        }
    }

    for (const auto& [_, group] : colorGroups) {
        renderOverlayLines(
            action,
            lineSet,
            group.indices.data(),
            static_cast<int>(group.indices.size()),
            group.color,
            OverlayDepthMode::DrawOnTop
        );
    }
}

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
    : selContext(std::make_shared<SelContext>())
    , selContext2(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
    SO_NODE_ADD_FIELD(highlightCoordIndex, (0));
    SO_NODE_ADD_FIELD(selectionCoordIndex, (0));
    SO_NODE_ADD_FIELD(faceEdgeIndex, (0));
    SO_NODE_ADD_FIELD(highlightColor, (SbColor(1.0f, 0.0f, 0.0f)));
    SO_NODE_ADD_FIELD(selectionColor, (SbColor(0.0f, 0.6f, 0.0f)));

    highlightCoordIndex.setNum(0);
    selectionCoordIndex.setNum(0);
    faceEdgeIndex.setNum(0);
    overlayLineSet = new SoIndexedLineSet;
    overlayLineSet->ref();
}

SoBrepEdgeSet::~SoBrepEdgeSet()
{
    if (overlayLineSet) {
        overlayLineSet->unref();
        overlayLineSet = nullptr;
    }
}

void SoBrepEdgeSet::setFaceHighlight(
    int faceIndex,
    const SbColor& accentColor,
    const SbColor& haloColor,
    Gui::HighlightPresentation presentation,
    float lineWidth,
    float haloLineWidth
)
{
    faceHighlightActive = faceIndex >= 0;
    faceHighlightIndex = faceIndex;
    faceHighlightAccentColor = accentColor;
    faceHighlightHaloColor = haloColor;
    faceHighlightPresentation = presentation;
    faceHighlightLineWidth = std::max(1.0F, lineWidth);
    faceHighlightHaloLineWidth = std::max(faceHighlightLineWidth, haloLineWidth);
    touch();
}

void SoBrepEdgeSet::clearFaceHighlight()
{
    if (!faceHighlightActive && faceHighlightPresentation == Gui::HighlightPresentation::None) {
        return;
    }
    faceHighlightActive = false;
    faceHighlightIndex = -1;
    faceHighlightPresentation = Gui::HighlightPresentation::None;
    faceHighlightLineWidth = 1.0F;
    faceHighlightHaloLineWidth = 1.0F;
    touch();
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

    const bool hasContextHighlight = ctx && !ctx->hl.empty();
    const bool hasFaceHighlight = faceHighlightActive
        && Gui::hasHighlightPresentation(
                                      faceHighlightPresentation,
                                      Gui::HighlightPresentation::DrawOnTop
        );
    const bool hasOnTopHighlight
        = (hasContextHighlight && ctx->hasHighlightPresentation(Gui::HighlightPresentation::DrawOnTop))
        || hasFaceHighlight;

    if (!Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasOnTopHighlight) {
        Gui::SoDelayedAnnotationsElement::addDelayedPath(
            action->getState(),
            action->getCurPath()->copy(),
            200
        );
        return;
    }

    bool hasColorOverride = (ctx2 && !ctx2->colors.empty());

    if (ctx && ctx->highlightIndex == std::numeric_limits<int>::max() && !ctx->isSelectAll()) {
        if (ctx->selectionIndex.empty()) {
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
    if (hasColorOverride) {
        renderColorOverrides(
            action,
            overlayLineSet,
            this->coordIndex.getValues(0),
            this->coordIndex.getNum(),
            ctx2->colors
        );
    }
    else if (ctx2 && !ctx2->selectionIndex.empty()) {
        renderSelection(action, ctx2, false);
    }
    else {
        inherited::GLRender(action);
    }

    if (Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths && hasFaceHighlight) {
        renderFaceHighlight(action);
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

    // Optional overlay rendering for deterministic tests (and programmatic usage).
    const int hlNum = highlightCoordIndex.getNum();
    if (hlNum > 0) {
        renderOverlayLines(
            action,
            overlayLineSet,
            highlightCoordIndex.getValues(0),
            hlNum,
            highlightColor.getValue(),
            OverlayDepthMode::DrawOnTop
        );
    }
    const int selNum = selectionCoordIndex.getNum();
    if (selNum > 0) {
        renderOverlayLines(
            action,
            overlayLineSet,
            selectionCoordIndex.getValues(0),
            selNum,
            selectionColor.getValue(),
            OverlayDepthMode::DrawOnTop
        );
    }
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

void SoBrepEdgeSet::appendLineCoordIndex(int lineIndex, std::vector<int32_t>& out) const
{
    if (lineIndex < 0) {
        return;
    }

    const int32_t* cindices = this->coordIndex.getValues(0);
    const int numcindices = this->coordIndex.getNum();
    int section = 0;
    bool found = false;
    for (int i = 0; i < numcindices; ++i) {
        if (cindices[i] < 0) {
            if (found) {
                out.push_back(-1);
                return;
            }
            ++section;
            continue;
        }
        if (section == lineIndex) {
            out.push_back(cindices[i]);
            found = true;
        }
    }
    if (found) {
        out.push_back(-1);
    }
}

void SoBrepEdgeSet::appendFaceEdgeCoordIndex(int faceIndex, std::vector<int32_t>& out) const
{
    if (faceIndex < 0) {
        return;
    }

    const int32_t* edgeIndices = this->faceEdgeIndex.getValues(0);
    const int numEdgeIndices = this->faceEdgeIndex.getNum();
    int face = 0;
    for (int i = 0; i < numEdgeIndices; ++i) {
        const int32_t edgeIndex = edgeIndices[i];
        if (edgeIndex < 0) {
            if (face == faceIndex) {
                return;
            }
            ++face;
            continue;
        }
        if (face == faceIndex) {
            appendLineCoordIndex(edgeIndex, out);
        }
    }
}

void SoBrepEdgeSet::renderFaceHighlight(SoGLRenderAction* action)
{
    if (!faceHighlightActive) {
        return;
    }

    std::vector<int32_t> indices;
    appendFaceEdgeCoordIndex(faceHighlightIndex, indices);
    if (indices.empty()) {
        return;
    }

    renderPresentationLines(
        action,
        indices.data(),
        static_cast<int>(indices.size()),
        faceHighlightAccentColor,
        faceHighlightHaloColor,
        faceHighlightPresentation,
        faceHighlightLineWidth,
        faceHighlightHaloLineWidth
    );
}

void SoBrepEdgeSet::renderPresentationLines(
    SoGLRenderAction* action,
    const int32_t* indices,
    int numIndices,
    const SbColor& accentColor,
    const SbColor& haloColor,
    Gui::HighlightPresentation presentation,
    float lineWidth,
    float haloLineWidth
)
{
    const bool drawOnTop
        = Gui::hasHighlightPresentation(presentation, Gui::HighlightPresentation::DrawOnTop)
        && Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths;

    if (!drawOnTop) {
        renderOverlayLines(
            action,
            overlayLineSet,
            indices,
            numIndices,
            accentColor,
            OverlayDepthMode::DrawOnTop,
            lineWidth
        );
        return;
    }

    const float accentLineWidth = lineWidth > 0.0F ? lineWidth : defaultPresentationLineWidth;
    const float outlineLineWidth = haloLineWidth > 0.0F
        ? std::max(haloLineWidth, accentLineWidth)
        : std::max(defaultPresentationHaloLineWidth, accentLineWidth + 2.0F);

    renderOverlayLines(
        action,
        overlayLineSet,
        indices,
        numIndices,
        haloColor,
        OverlayDepthMode::DrawOnTop,
        outlineLineWidth
    );
    renderOverlayLines(
        action,
        overlayLineSet,
        indices,
        numIndices,
        accentColor,
        OverlayDepthMode::DrawOnTop,
        accentLineWidth
    );
}

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction* action, SelContextPtr ctx)
{
    if (!ctx || ctx->highlightIndex < 0) {
        return;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    if (!coords) {
        return;
    }

    int num = (int)ctx->hl.size();
    if (num > 0) {
        const SbColor haloColor = Gui::SelectionColors::contrastOutlineColor(
            ctx->highlightColor,
            Gui::SelectionColors::defaultBackgroundColor()
        );
        if (ctx->hl[0] < 0) {
            renderPresentationLines(
                action,
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                ctx->highlightColor,
                haloColor,
                ctx->highlightPresentation
            );
        }
        else {
            if (!validIndexes(coords, ctx->hl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderHighlight",
                    "highlightIndex out of range"
                );
            }
            else {
                renderPresentationLines(
                    action,
                    ctx->hl.data(),
                    num,
                    ctx->highlightColor,
                    haloColor,
                    ctx->highlightPresentation
                );
            }
        }
    }
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction* action, SelContextPtr ctx, bool /*push*/)
{
    if (!ctx) {
        return;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    if (!coords) {
        return;
    }

    int num = (int)ctx->sl.size();
    if (num > 0) {
        if (ctx->sl[0] < 0) {
            renderOverlayLines(
                action,
                overlayLineSet,
                this->coordIndex.getValues(0),
                this->coordIndex.getNum(),
                ctx->selectionColor,
                OverlayDepthMode::RespectDepth
            );
        }
        else {
            if (!validIndexes(coords, ctx->sl)) {
                SoDebugError::postWarning(
                    "SoBrepEdgeSet::renderSelection",
                    "selectionIndex out of range"
                );
            }
            else {
                renderOverlayLines(
                    action,
                    overlayLineSet,
                    ctx->sl.data(),
                    num,
                    ctx->selectionColor,
                    OverlayDepthMode::RespectDepth
                );
            }
        }
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
                ctx->removeHighlight();
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
            ctx->highlightPresentation = hlaction->getHighlightPresentation();
            ctx->hl.clear();
            ctx->hl.push_back(-1);
            touch();
            return;
        }

        if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->removeHighlight();
                ctx->hl.clear();
                touch();
            }
            return;
        }

        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
        ctx->highlightColor = hlaction->getColor();
        ctx->highlightPresentation = hlaction->getHighlightPresentation();
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
            ctx->removeHighlight();
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
