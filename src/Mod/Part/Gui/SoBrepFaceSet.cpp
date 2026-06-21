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

#include <algorithm>
#include <limits>
#include <set>
#include <vector>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>

#include <Base/Profiler.h>

#include <Gui/SoFCInteractiveElement.h>
#include <Gui/Selection/SoFCSelectionAction.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/Inventor/So3DAnnotation.h>

#include "SoBrepFaceSet.h"
#include "ViewProviderExt.h"

using namespace PartGui;

SO_NODE_SOURCE(SoBrepFaceSet)

namespace
{

static void buildOverlayCoordIndex(
    std::vector<int32_t>& out,
    const int32_t* coordIndex,
    int coordIndexCount,
    const int32_t* partTriCounts,
    int partCount,
    const std::set<int>& parts,
    bool selectAll
)
{
    out.clear();
    if (!coordIndex || coordIndexCount <= 0) {
        return;
    }
    if (selectAll) {
        out.insert(out.end(), coordIndex, coordIndex + coordIndexCount);
        return;
    }
    if (!partTriCounts || partCount <= 0 || parts.empty()) {
        return;
    }

    std::vector<int32_t> face;
    face.reserve(8);

    int pos = 0;
    for (int part = 0; part < partCount && pos < coordIndexCount; ++part) {
        const bool include = (parts.find(part) != parts.end());
        const int tris = partTriCounts[part];
        for (int t = 0; t < tris && pos < coordIndexCount; ++t) {
            // Skip any stray delimiters.
            while (pos < coordIndexCount && coordIndex[pos] < 0) {
                pos++;
            }
            face.clear();
            while (pos < coordIndexCount && coordIndex[pos] >= 0) {
                face.push_back(coordIndex[pos]);
                pos++;
            }
            if (pos < coordIndexCount && coordIndex[pos] < 0) {
                // Consume one delimiter.
                pos++;
            }
            if (include && face.size() >= 3) {
                out.insert(out.end(), face.begin(), face.end());
                out.push_back(-1);
            }
        }
    }
}

static void renderOverlayFaces(
    SoGLRenderAction* action,
    SoIndexedFaceSet* faceSet,
    const std::vector<int32_t>& coordIndex,
    const SbColor& color,
    bool onTop
)
{
    if (!action || !faceSet || coordIndex.empty()) {
        return;
    }

    auto state = action->getState();
    state->push();

    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoTextureEnabledElement::set(state, faceSet, false);
    SoMaterialBindingElement::set(state, SoMaterialBindingElement::OVERALL);
    SoOverrideElement::setMaterialBindingOverride(state, faceSet, true);

    if (onTop) {
        SoDepthBufferElement::set(state, FALSE, FALSE, SoDepthBufferElement::ALWAYS, SbVec2f(0.0f, 1.0f));
        SoShapeStyleElement::setTransparencyType(state, SoGLRenderAction::BLEND);
        SoLazyElement::setTransparencyType(state, SoGLRenderAction::BLEND);
    }
    else {
        SoPolygonOffsetElement::set(state, faceSet, -0.00001f, -1.0f, SoPolygonOffsetElement::FILLED, TRUE);
        SoDepthBufferElement::set(state, TRUE, FALSE, SoDepthBufferElement::LEQUAL, SbVec2f(0.0f, 1.0f));
    }

    SoLazyElement::setEmissive(state, &color);
    const uint32_t packed = color.getPackedValue(0.0f);
    SoLazyElement::setPacked(state, faceSet, 1, &packed, false);

    faceSet->coordIndex.setValues(0, static_cast<int32_t>(coordIndex.size()), coordIndex.data());
    faceSet->GLRender(action);

    state->pop();
}

static void expandPartMaterialIndexToFaceMaterialIndex(
    std::vector<int32_t>& outFaceMaterialIndex,
    const int32_t* partTriCounts,
    int partCount,
    const std::vector<int32_t>& perPartMaterialIndex
)
{
    outFaceMaterialIndex.clear();
    if (!partTriCounts || partCount <= 0
        || perPartMaterialIndex.size() < static_cast<size_t>(partCount)) {
        return;
    }

    size_t triangleCount = 0;
    for (int i = 0; i < partCount; ++i) {
        triangleCount += static_cast<size_t>(std::max(partTriCounts[i], 0));
    }

    outFaceMaterialIndex.reserve(triangleCount);
    for (int i = 0; i < partCount; ++i) {
        const int repeats = std::max(partTriCounts[i], 0);
        outFaceMaterialIndex.insert(outFaceMaterialIndex.end(), repeats, perPartMaterialIndex[i]);
    }
}

}  // namespace

void SoBrepFaceSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepFaceSet, SoIndexedFaceSet, "IndexedFaceSet");
}

SoBrepFaceSet::SoBrepFaceSet()
{
    SO_NODE_CONSTRUCTOR(SoBrepFaceSet);
    SO_NODE_ADD_FIELD(partIndex, (-1));
    SO_NODE_ADD_FIELD(highlightPartIndex, (0));
    SO_NODE_ADD_FIELD(selectionPartIndex, (0));
    SO_NODE_ADD_FIELD(highlightColor, (SbColor(1.0f, 0.0f, 0.0f)));
    SO_NODE_ADD_FIELD(selectionColor, (SbColor(0.0f, 0.6f, 0.0f)));

    highlightPartIndex.setNum(0);
    selectionPartIndex.setNum(0);

    selContext = std::make_shared<SelContext>();
    selContext2 = std::make_shared<SelContext>();
    packedColor = 0;

    overlayFaceSet = new SoIndexedFaceSet;
    overlayFaceSet->ref();
}

SoBrepFaceSet::~SoBrepFaceSet()
{
    if (overlayFaceSet) {
        overlayFaceSet->unref();
        overlayFaceSet = nullptr;
    }
}

void SoBrepFaceSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        auto* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        selCounter.checkAction(hlaction);
        if (!hlaction->isHighlighted()) {
            SelContextPtr ctx
                = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
            if (ctx) {
                ctx->removeHighlight();
                touch();
            }
            if (viewProvider) {
                viewProvider->clearFaceHighlight();
            }
            return;
        }

        const SoDetail* detail = hlaction->getElement();
        if (!detail) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
            ctx->highlightIndex = std::numeric_limits<int>::max();
            ctx->highlightColor = hlaction->getColor();
            ctx->highlightPresentation = hlaction->getHighlightPresentation();
            if (viewProvider) {
                viewProvider->clearFaceHighlight();
            }
            touch();
        }
        else {
            if (!detail->isOfType(SoFaceDetail::getClassTypeId())) {
                SelContextPtr ctx
                    = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                if (ctx) {
                    ctx->removeHighlight();
                    touch();
                }
                if (viewProvider) {
                    viewProvider->clearFaceHighlight();
                }
            }
            else {
                int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                ctx->highlightIndex = index;
                ctx->highlightColor = hlaction->getColor();
                ctx->highlightPresentation = hlaction->getHighlightPresentation();
                if (viewProvider) {
                    viewProvider->setFaceHighlight(
                        index,
                        hlaction->getColor(),
                        hlaction->getHighlightPresentation()
                    );
                }
                touch();
            }
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        auto* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
        switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::All: {
                SelContextPtr ctx
                    = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action, this, selContext);
                selCounter.checkAction(selaction, ctx);
                ctx->selectionIndex.clear();
                ctx->selectionIndex.insert(-1);
                ctx->selectionColor = selaction->getColor();
                touch();
                break;
            }
            case Gui::SoSelectionElementAction::None: {
                if (selaction->isSecondary()) {
                    if (Gui::SoFCSelectionRoot::removeActionContext(action, this)) {
                        touch();
                    }
                }
                else {
                    SelContextPtr ctx
                        = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (ctx && (!ctx->selectionIndex.empty() || !ctx->colors.empty())) {
                        ctx->selectionIndex.clear();
                        ctx->colors.clear();
                        touch();
                    }
                }
                break;
            }
            case Gui::SoSelectionElementAction::Color:
                if (selaction->isSecondary()) {
                    const auto& colors = selaction->getColors();
                    auto ctx
                        = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (colors.empty()) {
                        if (ctx) {
                            ctx->colors.clear();
                            if (ctx->isSelectAll()) {
                                Gui::SoFCSelectionRoot::removeActionContext(action, this);
                            }
                            touch();
                        }
                        return;
                    }

                    static const std::string element("Face");
                    auto it = colors.lower_bound(element);
                    if (colors.begin()->first.empty()
                        || (it != colors.end() && it->first.compare(0, element.size(), element) == 0)) {
                        if (!ctx) {
                            ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action, this);
                            selCounter.checkAction(selaction, ctx);
                            ctx->selectAll();
                        }
                        if (ctx->setColors(colors, element)) {
                            touch();
                        }
                    }
                }
                return;
            case Gui::SoSelectionElementAction::Append:
            case Gui::SoSelectionElementAction::Remove: {
                const SoDetail* detail = selaction->getElement();
                if (!detail || !detail->isOfType(SoFaceDetail::getClassTypeId())) {
                    if (selaction->isSecondary()) {
                        auto ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action, this);
                        selCounter.checkAction(selaction, ctx);
                        touch();
                    }
                    return;
                }
                int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
                if (selaction->getType() == Gui::SoSelectionElementAction::Append) {
                    auto ctx = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext);
                    selCounter.checkAction(selaction, ctx);
                    ctx->selectionColor = selaction->getColor();
                    if (ctx->isSelectAll()) {
                        ctx->selectionIndex.clear();
                    }
                    if (ctx->selectionIndex.insert(index).second) {
                        touch();
                    }
                }
                else {
                    auto ctx
                        = Gui::SoFCSelectionRoot::getActionContext(action, this, selContext, false);
                    if (ctx && ctx->removeIndex(index)) {
                        touch();
                    }
                }
                break;
            }
            default:
                break;
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoVRMLAction::getClassTypeId()) {
        // Keep materialIndex in sync when using PER_PART binding with one color per part.
        SoState* state = action->getState();
        Binding mbind = this->findMaterialBinding(state);
        if (mbind == PER_PART) {
            const SoLazyElement* mat = SoLazyElement::getInstance(state);
            const int numParts = partIndex.getNum();
            if (mat && mat->getNumDiffuse() == numParts) {
                int count = 0;
                const int32_t* indices = this->partIndex.getValues(0);
                for (int i = 0; i < numParts; i++) {
                    count += indices[i];
                }
                this->materialIndex.setNum(count);
                int32_t* matind = this->materialIndex.startEditing();
                int32_t k = 0;
                for (int i = 0; i < numParts; i++) {
                    for (int j = 0; j < indices[i]; j++) {
                        matind[k++] = i;
                    }
                }
                this->materialIndex.finishEditing();
            }
        }
    }

    inherited::doAction(action);
}

void SoBrepFaceSet::renderHighlight(SoGLRenderAction* action, SelContextPtr ctx)
{
    if (!ctx || ctx->highlightIndex < 0) {
        return;
    }

    const int32_t* partCounts = this->partIndex.getValues(0);
    const int partCount = this->partIndex.getNum();
    const int32_t* ci = this->coordIndex.getValues(0);
    const int ciCount = this->coordIndex.getNum();

    const int id = ctx->highlightIndex;
    if (id != std::numeric_limits<int>::max() && (id < 0 || id >= partCount)) {
        SoDebugError::postWarning("SoBrepFaceSet::renderHighlight", "highlightIndex out of range");
        return;
    }

    std::set<int> parts;
    const bool selectAll = (id == std::numeric_limits<int>::max());
    if (!selectAll) {
        parts.insert(id);
    }
    buildOverlayCoordIndex(overlayCoordIndex, ci, ciCount, partCounts, partCount, parts, selectAll);

    const bool onTop = ctx->hasHighlightPresentation(Gui::HighlightPresentation::DrawOnTop)
        && Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths;

    renderOverlayFaces(action, overlayFaceSet, overlayCoordIndex, ctx->highlightColor, onTop);
}

void SoBrepFaceSet::renderSelection(SoGLRenderAction* action, SelContextPtr ctx, bool /*push*/)
{
    if (!ctx || ctx->selectionIndex.empty()) {
        return;
    }

    const int32_t* partCounts = this->partIndex.getValues(0);
    const int partCount = this->partIndex.getNum();
    const int32_t* ci = this->coordIndex.getValues(0);
    const int ciCount = this->coordIndex.getNum();

    if (ctx->isSelectAll()) {
        std::set<int> dummy;
        buildOverlayCoordIndex(overlayCoordIndex, ci, ciCount, partCounts, partCount, dummy, true);
        renderOverlayFaces(action, overlayFaceSet, overlayCoordIndex, ctx->selectionColor, false);
        return;
    }

    std::set<int> parts;
    for (int idx : ctx->selectionIndex) {
        if (idx >= 0 && idx < partCount) {
            parts.insert(idx);
        }
    }
    if (parts.empty()) {
        SoDebugError::postWarning("SoBrepFaceSet::renderSelection", "selectionIndex out of range");
        return;
    }

    buildOverlayCoordIndex(overlayCoordIndex, ci, ciCount, partCounts, partCount, parts, false);
    renderOverlayFaces(action, overlayFaceSet, overlayCoordIndex, ctx->selectionColor, false);
}

bool SoBrepFaceSet::overrideMaterialBinding(SoGLRenderAction* action, SelContextPtr ctx, SelContextPtr ctx2)
{
    // SoBrepFaceSet groups rendered triangles into topological faces via
    // partIndex. Coin's IndexedFaceSet consumes one material index per
    // rendered triangle face, so any per-part coloring must be remapped to
    // per-face indices before GLRender(). Selection/highlight overlays reuse
    // the same remap path.
    const bool hasPrimary = ctx && (ctx->isHighlighted() || !ctx->selectionIndex.empty());
    const bool hasSecondary = ctx2 && (!ctx2->colors.empty() || !ctx2->selectionIndex.empty());
    auto* state = action->getState();
    const auto mb = SoMaterialBindingElement::get(state);
    const int partCount = this->partIndex.getNum();
    if (partCount <= 0) {
        return false;
    }

    auto* element = SoLazyElement::getInstance(state);
    const SbColor* diffuse = element->getDiffusePointer();
    if (!diffuse) {
        return false;
    }
    const int diffuseSize = element->getNumDiffuse();
    const bool needsBasePerPartRemap
        = (mb == SoMaterialBindingElement::PER_PART && diffuseSize >= partCount);
    if (!hasPrimary && !hasSecondary && !needsBasePerPartRemap) {
        return false;
    }
    if (mb != SoMaterialBindingElement::OVERALL && !needsBasePerPartRemap) {
        static bool warnedUnsupportedBinding = false;
        if (!warnedUnsupportedBinding) {
            warnedUnsupportedBinding = true;
            SoDebugError::postWarning(
                "SoBrepFaceSet::overrideMaterialBinding",
                "Unsupported material binding for Coin face remap; falling back to explicit "
                "overlay passes. Current Part face rendering expects OVERALL or PER_PART."
            );
        }
        return false;
    }

    const float* trans = element->getTransparencyPointer();
    const int transSize = element->getNumTransparencies();
    float trans0 = 0.0f;
    bool hasBaseTransparency = false;
    if (trans && transSize > 0) {
        for (int i = 0; i < transSize; ++i) {
            if (trans[i] != 0.0f) {
                hasBaseTransparency = true;
                trans0 = trans[i];
                break;
            }
        }
    }

    state->push();
    packedColors.clear();

    if (ctx2) {
        ctx2->trans0 = trans0;
    }

    uint32_t diffuseColor = diffuse[0].getPackedValue(trans0);
    int singleColor = 0;
    if (ctx && ctx->isHighlightAll() && !ctx->isSelectAll()) {
        singleColor = 1;
        diffuseColor = ctx->highlightColor.getPackedValue(trans0);
    }
    else if (ctx && ctx->isSelectAll()) {
        diffuseColor = ctx->selectionColor.getPackedValue(trans0);
        singleColor = ctx->isHighlighted() ? -1 : 1;
    }
    else if (ctx2 && ctx2->isSingleColor(diffuseColor, hasBaseTransparency)) {
        singleColor = ctx ? -1 : 1;
    }

    const bool partialRender = ctx2 && !ctx2->selectionIndex.empty() && !ctx2->isSelectAll();

    if (singleColor > 0 && !partialRender) {
        SoMaterialBindingElement::set(state, SoMaterialBindingElement::OVERALL);
        SoOverrideElement::setMaterialBindingOverride(state, this, true);
        packedColors.push_back(diffuseColor);
        SoLazyElement::setPacked(state, this, 1, packedColors.data(), hasBaseTransparency);
        SoTextureEnabledElement::set(state, this, false);
        return true;
    }

    std::vector<int32_t> perPartMaterialIndex;
    perPartMaterialIndex.reserve(partCount);
    const uint32_t fullyTransparent = SbColor(1.0f, 1.0f, 1.0f).getPackedValue(1.0f);

    if (ctx && (ctx->isSelectAll() || ctx->isHighlightAll())) {
        perPartMaterialIndex.resize(partCount, 0);
        if (!partialRender) {
            packedColors.push_back(diffuseColor);
        }
        else {
            packedColors.push_back(fullyTransparent);
            packedColors.push_back(diffuseColor);
            if (ctx2) {
                for (int idx : ctx2->selectionIndex) {
                    if (idx >= 0 && idx < partCount) {
                        perPartMaterialIndex[idx] = static_cast<int32_t>(packedColors.size() - 1);
                    }
                }
            }
        }
        if (ctx->highlightIndex >= 0 && ctx->highlightIndex < partCount) {
            packedColors.push_back(ctx->highlightColor.getPackedValue(trans0));
            perPartMaterialIndex[ctx->highlightIndex] = static_cast<int32_t>(packedColors.size() - 1);
        }
    }
    else {
        if (partialRender) {
            packedColors.push_back(fullyTransparent);
            perPartMaterialIndex.resize(partCount, 0);

            if (mb == SoMaterialBindingElement::OVERALL || singleColor) {
                packedColors.push_back(diffuseColor);
                const auto visibleIndex = static_cast<int32_t>(packedColors.size() - 1);
                for (int idx : ctx2->selectionIndex) {
                    if (idx < 0 || idx >= partCount) {
                        continue;
                    }
                    if (!singleColor && ctx2->applyColor(idx, packedColors, hasBaseTransparency)) {
                        perPartMaterialIndex[idx] = static_cast<int32_t>(packedColors.size() - 1);
                    }
                    else {
                        perPartMaterialIndex[idx] = visibleIndex;
                    }
                }
            }
            else {
                for (int idx : ctx2->selectionIndex) {
                    if (idx < 0 || idx >= partCount) {
                        continue;
                    }
                    if (!ctx2->applyColor(idx, packedColors, hasBaseTransparency)) {
                        const float transparency = idx < transSize ? trans[idx] : trans0;
                        packedColors.push_back(diffuse[idx].getPackedValue(transparency));
                    }
                    perPartMaterialIndex[idx] = static_cast<int32_t>(packedColors.size() - 1);
                }
            }
        }
        else if (mb == SoMaterialBindingElement::OVERALL || singleColor) {
            packedColors.push_back(diffuseColor);
            perPartMaterialIndex.resize(partCount, 0);

            if (ctx2 && !singleColor) {
                for (const auto& [idx, color] : ctx2->colors) {
                    if (idx < 0 || idx >= partCount) {
                        continue;
                    }
                    packedColors.push_back(ctx2->packColor(color, hasBaseTransparency));
                    perPartMaterialIndex[idx] = static_cast<int32_t>(packedColors.size() - 1);
                }
            }
        }
        else {
            perPartMaterialIndex.reserve(partCount);
            packedColors.reserve(
                static_cast<size_t>(diffuseSize) + (ctx2 ? ctx2->colors.size() : 0) + 3
            );
            for (int i = 0; i < partCount; ++i) {
                perPartMaterialIndex.push_back(i);
                if (!ctx2 || !ctx2->applyColor(i, packedColors, hasBaseTransparency)) {
                    const float transparency = i < transSize ? trans[i] : trans0;
                    packedColors.push_back(diffuse[i].getPackedValue(transparency));
                }
            }
        }

        if (ctx && !ctx->selectionIndex.empty()) {
            packedColors.push_back(ctx->selectionColor.getPackedValue(trans0));
            const auto selectedIndex = static_cast<int32_t>(packedColors.size() - 1);
            for (int idx : ctx->selectionIndex) {
                if (idx >= 0 && idx < partCount) {
                    perPartMaterialIndex[idx] = selectedIndex;
                }
            }
        }
        if (ctx && ctx->highlightIndex >= 0 && ctx->highlightIndex < partCount) {
            packedColors.push_back(ctx->highlightColor.getPackedValue(trans0));
            perPartMaterialIndex[ctx->highlightIndex] = static_cast<int32_t>(packedColors.size() - 1);
        }
    }

    const int32_t* partCounts = this->partIndex.getValues(0);
    // partIndex groups triangles into topological faces, while SoIndexedFaceSet
    // consumes one material index per rendered triangle face.
    expandPartMaterialIndexToFaceMaterialIndex(matIndex, partCounts, partCount, perPartMaterialIndex);
    if (matIndex.empty()) {
        state->pop();
        return false;
    }

    const size_t num = materialIndex.getNum();
    if (num != matIndex.size() || materialIndex.getValues(0) != matIndex.data()) {
        SbBool notify = enableNotify(FALSE);
        materialIndex.setValuesPointer(matIndex.size(), matIndex.data());
        if (notify) {
            enableNotify(notify);
        }
    }

    const bool usesTransparencyMask = partialRender;
    const bool hasTransparency = hasBaseTransparency || usesTransparencyMask;

    SoMaterialBindingElement::set(state, this, SoMaterialBindingElement::PER_FACE_INDEXED);
    SoLazyElement::setPacked(state, this, packedColors.size(), packedColors.data(), hasTransparency);
    SoTextureEnabledElement::set(state, this, false);
    return true;
}

void SoBrepFaceSet::GLRender(SoGLRenderAction* action)
{
    ZoneScoped;

    if (this->coordIndex.getNum() < 3) {
        return;
    }

    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext(this, selContext, ctx2);
    const bool hasSecondaryColors = ctx2 && !ctx2->colors.empty();
    const bool hasOverlayFields = (highlightPartIndex.getNum() > 0)
        || (selectionPartIndex.getNum() > 0);
    if (!hasOverlayFields && ctx2 && ctx2->selectionIndex.empty() && !hasSecondaryColors) {
        return;
    }
    if (selContext2->checkGlobal(ctx)) {
        ctx = selContext2;
    }
    if (ctx && (ctx->selectionIndex.empty() && ctx->highlightIndex < 0)) {
        ctx.reset();
    }

    auto state = action->getState();
    selCounter.checkRenderCache(state);

    const bool hasContextHighlight = ctx && ctx->isHighlighted() && !ctx->isHighlightAll()
        && ctx->highlightIndex >= 0 && ctx->highlightIndex < partIndex.getNum();

    if (hasContextHighlight && ctx->hasHighlightPresentation(Gui::HighlightPresentation::DrawOnTop)) {
        if (!Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths) {
            const SoPath* currentPath = action->getCurPath();
            Gui::SoDelayedAnnotationsElement::addDelayedPath(state, currentPath->copy(), 100);
        }
        else {
            renderHighlight(action, ctx);
            return;
        }
    }

    SoMaterialBundle mb(action);
    mb.sendFirst();
    const bool pushed = overrideMaterialBinding(action, ctx, ctx2);
    if (!this->shouldGLRender(action)) {
        if (pushed) {
            state->pop();
        }
        return;
    }

    inherited::GLRender(action);
    if (pushed) {
        state->pop();
    }

    if (!pushed) {
        // If overrideMaterialBinding() cannot express the current material
        // binding through Coin state, render selection and highlight with
        // explicit overlay passes instead.
        if (ctx2 && !hasSecondaryColors && !ctx2->selectionIndex.empty()) {
            renderSelection(action, ctx2);
        }
        if (ctx && !ctx->selectionIndex.empty()) {
            renderSelection(action, ctx);
        }
        if (ctx) {
            renderHighlight(action, ctx);
        }
    }

    // Optional overlay rendering for deterministic tests (and programmatic usage).
    const int selNum = selectionPartIndex.getNum();
    const int hlNum = highlightPartIndex.getNum();
    if (selNum > 0 || hlNum > 0) {
        GLint oldDepthFunc = GL_LEQUAL;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        if (oldDepthFunc != GL_LEQUAL) {
            glDepthFunc(GL_LEQUAL);
        }

        if (selNum > 0) {
            SelContextPtr octx = std::make_shared<SelContext>();
            octx->selectionColor = selectionColor.getValue();
            const int32_t* vals = selectionPartIndex.getValues(0);
            for (int i = 0; i < selNum; i++) {
                octx->selectionIndex.insert(vals[i]);
            }
            renderSelection(action, octx);
        }
        if (hlNum > 0) {
            const int32_t* vals = highlightPartIndex.getValues(0);
            for (int i = 0; i < hlNum; i++) {
                SelContextPtr octx = std::make_shared<SelContext>();
                octx->highlightIndex = vals[i];
                octx->highlightColor = highlightColor.getValue();
                renderHighlight(action, octx);
            }
        }
        // Keep live face preselection on top when it overlaps the explicit
        // overlay selection/highlight fields.
        if (hasContextHighlight) {
            renderHighlight(action, ctx);
        }

        if (oldDepthFunc != GL_LEQUAL) {
            glDepthFunc(oldDepthFunc);
        }
    }
}

void SoBrepFaceSet::GLRenderBelowPath(SoGLRenderAction* action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepFaceSet::generatePrimitives(SoAction* action)
{
    inherited::generatePrimitives(action);
}

void SoBrepFaceSet::getBoundingBox(SoGetBoundingBoxAction* action)
{
    inherited::getBoundingBox(action);
}

SoDetail* SoBrepFaceSet::createTriangleDetail(
    SoRayPickAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    const SoPrimitiveVertex* v3,
    SoPickedPoint* pp
)
{
    SoDetail* detail = inherited::createTriangleDetail(action, v1, v2, v3, pp);
    const int32_t* indices = this->partIndex.getValues(0);
    const int num = this->partIndex.getNum();
    if (indices) {
        auto* face_detail = static_cast<SoFaceDetail*>(detail);
        const int index = face_detail->getFaceIndex();
        int count = 0;
        for (int i = 0; i < num; i++) {
            count += indices[i];
            if (index < count) {
                face_detail->setPartIndex(i);
                break;
            }
        }
    }
    return detail;
}

SoBrepFaceSet::Binding SoBrepFaceSet::findMaterialBinding(SoState* const state) const
{
    Binding binding = OVERALL;
    const auto matbind = SoMaterialBindingElement::get(state);

    switch (matbind) {
        case SoMaterialBindingElement::OVERALL:
            binding = OVERALL;
            break;
        case SoMaterialBindingElement::PER_VERTEX:
            binding = PER_VERTEX;
            break;
        case SoMaterialBindingElement::PER_VERTEX_INDEXED:
            binding = PER_VERTEX_INDEXED;
            break;
        case SoMaterialBindingElement::PER_PART:
            binding = PER_PART;
            break;
        case SoMaterialBindingElement::PER_FACE:
            binding = PER_FACE;
            break;
        case SoMaterialBindingElement::PER_PART_INDEXED:
            binding = PER_PART_INDEXED;
            break;
        case SoMaterialBindingElement::PER_FACE_INDEXED:
            binding = PER_FACE_INDEXED;
            break;
        default:
            break;
    }
    return binding;
}

SoBrepFaceSet::Binding SoBrepFaceSet::findNormalBinding(SoState* const state) const
{
    Binding binding = PER_VERTEX_INDEXED;
    const auto normbind = static_cast<SoNormalBindingElement::Binding>(
        SoNormalBindingElement::get(state)
    );

    switch (normbind) {
        case SoNormalBindingElement::OVERALL:
            binding = OVERALL;
            break;
        case SoNormalBindingElement::PER_VERTEX:
            binding = PER_VERTEX;
            break;
        case SoNormalBindingElement::PER_VERTEX_INDEXED:
            binding = PER_VERTEX_INDEXED;
            break;
        case SoNormalBindingElement::PER_PART:
            binding = PER_PART;
            break;
        case SoNormalBindingElement::PER_FACE:
            binding = PER_FACE;
            break;
        case SoNormalBindingElement::PER_PART_INDEXED:
            binding = PER_PART_INDEXED;
            break;
        case SoNormalBindingElement::PER_FACE_INDEXED:
            binding = PER_FACE_INDEXED;
            break;
        default:
            break;
    }
    return binding;
}
