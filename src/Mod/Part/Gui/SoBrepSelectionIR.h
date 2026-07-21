// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <memory>

#include <Inventor/SbVec4f.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/rendering/SoRenderIR.h>

#include <Gui/Selection/SoFCSelectionContext.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>

namespace PartGui::SelectionIR
{

struct RootSelectionState
{
    bool selected = false;
    bool highlighted = false;
    SbColor selectionColor;
    SbColor highlightColor;
};

inline SbVec4f toVec4(const SbColor& color)
{
    return {color[0], color[1], color[2], 1.0f};
}

inline void reset(SoRenderCommand& cmd)
{
    cmd.selection.highlightWholeObject = false;
    cmd.selection.highlightedElements.clear();
    cmd.selection.selectWholeObject = false;
    cmd.selection.selectedElements.clear();
}

inline RootSelectionState getRootSelection(SoAction* action)
{
    RootSelectionState state;
    Gui::SoFCSelectionRoot::checkSelection(
        action,
        state.selected,
        state.selectionColor,
        state.highlighted,
        state.highlightColor
    );
    return state;
}

template<typename ContextT, typename ContainsElementFn>
inline void applyPrimary(
    SoRenderCommand& cmd,
    const std::shared_ptr<ContextT>& ctx,
    const RootSelectionState& root,
    ContainsElementFn&& containsElement
)
{
    reset(cmd);

    if (ctx && ctx->isHighlightAll()) {
        cmd.selection.highlightWholeObject = true;
        cmd.selection.highlightColor = toVec4(ctx->highlightColor);
    }
    else if (root.highlighted) {
        cmd.selection.highlightWholeObject = true;
        cmd.selection.highlightColor = toVec4(root.highlightColor);
    }
    else if (ctx && ctx->highlightIndex >= 0 && containsElement(ctx->highlightIndex)) {
        cmd.selection.highlightedElements.push_back(ctx->highlightIndex);
        cmd.selection.highlightColor = toVec4(ctx->highlightColor);
    }

    if (ctx && ctx->isSelectAll()) {
        cmd.selection.selectionColor = toVec4(ctx->selectionColor);
        cmd.selection.selectWholeObject = true;
    }
    else if (root.selected) {
        cmd.selection.selectionColor = toVec4(root.selectionColor);
        cmd.selection.selectWholeObject = true;
    }
    else if (ctx) {
        bool hasSelectedElements = false;
        for (int idx : ctx->selectionIndex) {
            if (idx >= 0 && containsElement(idx)) {
                if (!hasSelectedElements) {
                    cmd.selection.selectionColor = toVec4(ctx->selectionColor);
                    hasSelectedElements = true;
                }
                cmd.selection.selectedElements.push_back(idx);
            }
        }
    }
}

template<typename ContainsElementFn>
inline void applySelectionOverlay(
    SoRenderCommand& cmd,
    const SoMFInt32& overlayIndices,
    const SbColor& overlayColor,
    ContainsElementFn&& containsElement
)
{
    if (overlayIndices.getNum() <= 0) {
        return;
    }

    cmd.selection.selectionColor = toVec4(overlayColor);
    cmd.selection.selectWholeObject = false;
    cmd.selection.selectedElements.clear();

    const int32_t* indices = overlayIndices.getValues(0);
    for (int i = 0; i < overlayIndices.getNum(); ++i) {
        const int idx = indices[i];
        if (idx < 0) {
            cmd.selection.selectWholeObject = true;
            cmd.selection.selectedElements.clear();
            return;
        }
        if (containsElement(idx)) {
            cmd.selection.selectedElements.push_back(idx);
        }
    }
}

template<typename ContainsElementFn>
inline void applyHighlightOverlay(
    SoRenderCommand& cmd,
    const SoMFInt32& overlayIndices,
    const SbColor& overlayColor,
    ContainsElementFn&& containsElement
)
{
    if (overlayIndices.getNum() <= 0) {
        return;
    }

    cmd.selection.highlightColor = toVec4(overlayColor);
    cmd.selection.highlightWholeObject = false;
    cmd.selection.highlightedElements.clear();

    const int32_t* indices = overlayIndices.getValues(0);
    for (int i = 0; i < overlayIndices.getNum(); ++i) {
        const int idx = indices[i];
        if (idx < 0) {
            cmd.selection.highlightWholeObject = true;
            cmd.selection.highlightedElements.clear();
            return;
        }
        if (containsElement(idx)) {
            cmd.selection.highlightedElements.push_back(idx);
        }
    }
}

}  // namespace PartGui::SelectionIR
