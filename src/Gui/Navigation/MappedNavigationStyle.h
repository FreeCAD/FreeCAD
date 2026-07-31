// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <optional>

#include <Gui/Navigation/NavigationResolver.h>

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
namespace Gui
{

/** Shared event-processing pipeline for navigation styles driven by button/modifier chords. */
class GuiExport MappedNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    const char* mouseButtons(ViewerMode mode) override;

protected:
    enum InputFlag : unsigned int
    {
        LeftDown = NavigationInputState::LeftDown,
        MiddleDown = NavigationInputState::MiddleDown,
        RightDown = NavigationInputState::RightDown,
        CtrlDown = NavigationInputState::CtrlDown,
        ShiftDown = NavigationInputState::ShiftDown,
        AltDown = NavigationInputState::AltDown,
    };

    struct EventContext
    {
        // Observed event state and normalized pointer data.
        const SoEvent* event;
        ViewerMode initialMode;
        unsigned int chord;
        SbVec2s position;
        SbVec2f normalizedPosition;
        SbVec2f previousNormalizedPosition;

        // Mutable event outcomes. Hooks may update the resolved mode and processing state.
        ViewerMode resolvedMode;
        bool processed = false;
        bool selectionDragAttempted = false;
    };

    MappedNavigationStyle() = default;
    ~MappedNavigationStyle() override = default;

    SbBool processSoEvent(const SoEvent* const ev) override;

    /**
     * Handle style-specific button behavior after profile resolution.
     * The hook may update resolvedMode or processed before shared policies run.
     */
    virtual void processStyleButtonEvent(EventContext& context);

    /** Whether adding a button to an active motion gesture should force rotation. */
    virtual bool shouldForceRotationWhenButtonAdded(const EventContext& context) const;

    /** Whether the common mouse-button dispatcher should handle this event. */
    virtual bool shouldProcessMouseButtonEvent(const SoEvent* event) const;

    /**
     * Handle style-specific pointer motion before profile resolution.
     * Return true to skip common pointer-motion handling; the hook may request
     * a mode by updating resolvedMode.
     */
    virtual bool processStylePointerMotionEvent(EventContext& context);

    /** Return the declarative profile used by this style. */
    virtual const NavigationProfile& profile() const = 0;

    /**
     * Decide whether an event not already consumed should reach UserNavigationStyle.
     */
    virtual bool shouldPropagate(const EventContext& context) const;

    /**
     * Apply style-specific adjustments after resolution and button handling,
     * but before mode-entry effects and selection-lock policy.
     */
    virtual void adjustResolvedMode(EventContext& context);

    /** Apply the style's editing-selection policy during shared selection handling. */
    virtual void preserveEditingSelection(EventContext& context);

private:
    EventContext createContext(const SoEvent* const ev);
    NavigationInputState currentInputState() const;

    void processKeyboardEventCommon(EventContext& context);
    void processMouseButtonEventCommon(EventContext& context);
    void processPointerMotionCommon(EventContext& context);
    void processMotion3EventCommon(EventContext& context);
    void resolveNavigationMode(EventContext& context);
    void applySelectionLockPolicy(EventContext& context);
    void applyModeEntryEffects(EventContext& context);
    void prepareMiddleButtonPress(const EventContext& context);
    void finalizeModeTransition(EventContext& context);
    void resetGestureFlagsIfIdle(const EventContext& context);
    SbBool propagateEvent(EventContext& context);

    bool lockButton1 {false};
    std::optional<GestureOwnership> activeGesture;
};

}  // namespace Gui
// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
