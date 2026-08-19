// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <algorithm>
#include <memory>

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

inline SbColor4f toColor4(const SbColor& color, float alpha = 1.0f)
{
    return {color[0], color[1], color[2], alpha};
}

// BRep's legacy highlight overlay is rendered as an opaque material. Keep
// retained selection output visually identical across both pipelines.
constexpr float DEFAULT_HIGHLIGHT_ALPHA = 1.0f;

inline void reset(SoSelectionState& selection, int commandIndex)
{
    auto removeCommandTargets = [commandIndex](auto& targets) {
        targets.erase(
            std::remove_if(
                targets.begin(),
                targets.end(),
                [commandIndex](const SoSelectionTarget& target) {
                    return target.commandIndex == commandIndex;
                }
            ),
            targets.end()
        );
    };
    removeCommandTargets(selection.selected);
    removeCommandTargets(selection.highlighted);
}

inline void addTarget(
    std::vector<SoSelectionTarget>& targets,
    int commandIndex,
    SoPickElementType type,
    int elementIndex,
    const SbColor4f& color
)
{
    SoSelectionTarget target;
    target.commandIndex = commandIndex;
    target.type = type;
    target.elementIndex = elementIndex;
    target.color = color;
    targets.push_back(target);
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
    SoSelectionState& selection,
    int commandIndex,
    SoPickElementType elementType,
    const std::shared_ptr<ContextT>& ctx,
    const RootSelectionState& root,
    ContainsElementFn&& containsElement
)
{
    reset(selection, commandIndex);

    if (ctx && ctx->isHighlightAll()) {
        addTarget(
            selection.highlighted,
            commandIndex,
            SO_PICK_OBJECT,
            -1,
            toColor4(ctx->highlightColor, DEFAULT_HIGHLIGHT_ALPHA)
        );
    }
    else if (root.highlighted) {
        addTarget(
            selection.highlighted,
            commandIndex,
            SO_PICK_OBJECT,
            -1,
            toColor4(root.highlightColor, DEFAULT_HIGHLIGHT_ALPHA)
        );
    }
    else if (ctx && ctx->highlightIndex >= 0 && containsElement(ctx->highlightIndex)) {
        addTarget(
            selection.highlighted,
            commandIndex,
            elementType,
            ctx->highlightIndex,
            toColor4(ctx->highlightColor, DEFAULT_HIGHLIGHT_ALPHA)
        );
    }

    if (ctx && ctx->isSelectAll()) {
        addTarget(selection.selected, commandIndex, SO_PICK_OBJECT, -1, toColor4(ctx->selectionColor));
    }
    else if (root.selected) {
        addTarget(selection.selected, commandIndex, SO_PICK_OBJECT, -1, toColor4(root.selectionColor));
    }
    else if (ctx) {
        for (int idx : ctx->selectionIndex) {
            if (idx >= 0 && containsElement(idx)) {
                addTarget(
                    selection.selected,
                    commandIndex,
                    elementType,
                    idx,
                    toColor4(ctx->selectionColor)
                );
            }
        }
    }
}

template<typename ContainsElementFn>
inline void applySelectionOverlay(
    SoSelectionState& selection,
    int commandIndex,
    SoPickElementType elementType,
    const SoMFInt32& overlayIndices,
    const SbColor& overlayColor,
    ContainsElementFn&& containsElement
)
{
    if (overlayIndices.getNum() <= 0) {
        return;
    }

    const int32_t* indices = overlayIndices.getValues(0);
    for (int i = 0; i < overlayIndices.getNum(); ++i) {
        const int idx = indices[i];
        if (idx < 0) {
            addTarget(selection.selected, commandIndex, SO_PICK_OBJECT, -1, toColor4(overlayColor));
            return;
        }
        if (containsElement(idx)) {
            addTarget(selection.selected, commandIndex, elementType, idx, toColor4(overlayColor));
        }
    }
}

template<typename ContainsElementFn>
inline void applyHighlightOverlay(
    SoSelectionState& selection,
    int commandIndex,
    SoPickElementType elementType,
    const SoMFInt32& overlayIndices,
    const SbColor& overlayColor,
    ContainsElementFn&& containsElement
)
{
    if (overlayIndices.getNum() <= 0) {
        return;
    }

    const int32_t* indices = overlayIndices.getValues(0);
    for (int i = 0; i < overlayIndices.getNum(); ++i) {
        const int idx = indices[i];
        if (idx < 0) {
            addTarget(
                selection.highlighted,
                commandIndex,
                SO_PICK_OBJECT,
                -1,
                toColor4(overlayColor, 1.0f)
            );
            return;
        }
        if (containsElement(idx)) {
            addTarget(selection.highlighted, commandIndex, elementType, idx, toColor4(overlayColor, 1.0f));
        }
    }
}

}  // namespace PartGui::SelectionIR
