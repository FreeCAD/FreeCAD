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

#include "MappedNavigationStyle.h"
#include "NavigationResolver.h"

#include <Inventor/nodes/SoCamera.h>
#include <QApplication>

#include "View3DInventorViewer.h"

using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::MappedNavigationStyle, Gui::UserNavigationStyle)

MappedNavigationStyle::EventContext MappedNavigationStyle::createContext(const SoEvent* const ev)
{
    const SbVec2s position(ev->getPosition());
    const SbVec2f normalizedPosition = normalizePixelPos(position);
    const SbVec2f previousNormalizedPosition = lastmouseposition;
    lastmouseposition = normalizedPosition;

    return {
        .event = ev,
        .initialMode = currentmode,
        .chord = currentInputState().chord(),
        .position = position,
        .normalizedPosition = normalizedPosition,
        .previousNormalizedPosition = previousNormalizedPosition,
        .resolvedMode = currentmode,
    };
}

const char* MappedNavigationStyle::mouseButtons(ViewerMode mode)
{
    const NavigationProfile& navigationProfile = profile();
    switch (mode) {
        case NavigationStyle::SELECTION:
            return navigationProfile.selectionDescription;
        case NavigationStyle::PANNING:
            return navigationProfile.panDescription;
        case NavigationStyle::DRAGGING:
            return navigationProfile.rotateDescription;
        case NavigationStyle::ZOOMING:
            return navigationProfile.zoomDescription;
        default:
            return "No description";
    }
}

SbBool MappedNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Events when in "ready-to-seek" mode are handled by NavigationStyle.
    if (isSeekMode()) {
        return inherited::processSoEvent(ev);
    }

    if (!isAnimating() && isViewing()) {
        setViewing(false);  // by default disable viewing mode to render the scene
    }

    syncModifierKeys(ev);

    if (!viewer->isEditing()) {
        const SbBool processed = handleEventInForeground(ev);
        if (processed) {
            return true;
        }
    }

    EventContext context = createContext(ev);

    // Phase 1: interpret the raw Coin event and update physical input state.
    processKeyboardEventCommon(context);
    processMouseButtonEventCommon(context);
    processPointerMotionCommon(context);
    processMotion3EventCommon(context);

    // Phase 2: resolve the requested mode from profile rules and gesture ownership.
    resolveNavigationMode(context);

    // Phase 3: apply style-specific post-resolution behavior.
    if (context.event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        processStyleButtonEvent(context);
    }
    adjustResolvedMode(context);

    // Phase 4: apply shared policies, execute entry effects, and commit the transition.
    applyModeEntryEffects(context);
    applySelectionLockPolicy(context);
    finalizeModeTransition(context);
    resetGestureFlagsIfIdle(context);

    return propagateEvent(context);
}

void MappedNavigationStyle::processKeyboardEventCommon(EventContext& context)
{
    if (context.event->isOfType(SoKeyboardEvent::getClassTypeId())) {
        const auto* const event = static_cast<const SoKeyboardEvent*>(context.event);
        context.processed = processKeyboardEvent(event);
    }
}

void MappedNavigationStyle::processMouseButtonEventCommon(EventContext& context)
{
    if (!context.event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        return;
    }

    const auto* const event = static_cast<const SoMouseButtonEvent*>(context.event);
    if (!shouldProcessMouseButtonEvent(context.event)) {
        return;
    }

    const int button = event->getButton();
    const SbBool press = event->getState() == SoButtonEvent::DOWN;

    switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            lockrecenter = true;
            button1down = press;
            updateSelectionStartPosition(press, context.position);
            if (press && currentmode == NavigationStyle::SEEK_WAIT_MODE) {
                context.resolvedMode = NavigationStyle::SEEK_MODE;
                seekToPoint(context.position);  // implicitly calls interactiveCountInc()
                context.processed = true;
            }
            else if (
                press && shouldForceRotationWhenButtonAdded(context)
                && (currentmode == NavigationStyle::PANNING || currentmode == NavigationStyle::ZOOMING)
            ) {
                context.resolvedMode = NavigationStyle::DRAGGING;
                context.processed = true;
            }
            else if (!press && currentmode == NavigationStyle::DRAGGING) {
                context.processed = true;
            }
            else if (viewer->isEditing() && currentmode == NavigationStyle::SPINNING) {
                context.processed = true;
            }
            else {
                context.processed = processClickEvent(event);
            }
            break;

        case SoMouseButtonEvent::BUTTON2:
            lockrecenter = true;

            // Don't show the context menu after dragging, panning or zooming.
            if (!press && (hasDragged || hasPanned || hasZoomed)) {
                context.processed = true;
            }
            else if (!press && !viewer->isEditing()) {
                if (currentmode != NavigationStyle::ZOOMING && currentmode != NavigationStyle::PANNING
                    && currentmode != NavigationStyle::DRAGGING && isPopupMenuEnabled()) {
                    openPopupMenu(event->getPosition());
                }
            }

            if (press && shouldForceRotationWhenButtonAdded(context)
                && (currentmode == NavigationStyle::PANNING
                    || currentmode == NavigationStyle::ZOOMING)) {
                context.resolvedMode = NavigationStyle::DRAGGING;
                context.processed = true;
            }
            button2down = press;
            break;

        case SoMouseButtonEvent::BUTTON3:
            if (press) {
                prepareMiddleButtonPress(context);
                lockrecenter = false;
            }
            else {
                const SbTime elapsed = context.event->getTime() - centerTime;
                const float doubleClickInterval
                    = static_cast<float>(QApplication::doubleClickInterval()) / 1000.0F;
                if (profile().recenterOnMiddleClick && elapsed.getValue() < doubleClickInterval
                    && !lockrecenter) {
                    lookAtPoint(context.position);
                    context.processed = true;
                }
            }
            button3down = press;
            break;

        default:
            break;
    }
}

void MappedNavigationStyle::processPointerMotionCommon(EventContext& context)
{
    if (!context.event->isOfType(SoLocation2Event::getClassTypeId())) {
        return;
    }

    lockrecenter = true;
    const auto* const event = static_cast<const SoLocation2Event*>(context.event);
    if (processStylePointerMotionEvent(context)) {
        return;
    }

    if (currentmode == NavigationStyle::SELECTION && button1down) {
        context.selectionDragAttempted = true;
        context.processed = handleSelectionDragMotion(event, context.resolvedMode, ctrldown);
    }
    else if (currentmode == NavigationStyle::ZOOMING) {
        zoomByCursor(context.normalizedPosition, context.previousNormalizedPosition);
        context.processed = true;
    }
    else if (currentmode == NavigationStyle::PANNING) {
        const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();
        const float ratio = vp.getViewportAspectRatio();
        panCamera(
            viewer->getSoRenderManager()->getCamera(),
            ratio,
            panningplane,
            context.normalizedPosition,
            context.previousNormalizedPosition
        );
        context.processed = true;
    }
    else if (currentmode == NavigationStyle::DRAGGING) {
        addToLog(event->getPosition(), event->getTime());
        spin(context.normalizedPosition);
        moveCursorPosition();
        context.processed = true;
    }
}

void MappedNavigationStyle::processMotion3EventCommon(EventContext& context)
{
    if (!context.event->isOfType(SoMotion3Event::getClassTypeId())) {
        return;
    }

    const auto* const event = static_cast<const SoMotion3Event*>(context.event);
    if (event) {
        processMotionEvent(event);
    }
    context.processed = true;
}

void MappedNavigationStyle::resolveNavigationMode(EventContext& context)
{
    if (context.selectionDragAttempted) {
        return;
    }

    const NavigationInputState inputState = currentInputState();
    context.chord = inputState.chord();
    ResolutionInput resolutionInput {
        .currentMode = context.initialMode,
        .chord = context.chord,
        .requestedMode = context.resolvedMode != context.initialMode
            ? std::optional {context.resolvedMode}
            : std::nullopt,
        .activeGesture = activeGesture,
    };
    ResolutionResult resolution = resolveNavigation(profile(), resolutionInput);
    context.resolvedMode = resolution.mode;
    activeGesture = resolution.activeGesture;
}

void MappedNavigationStyle::applySelectionLockPolicy(EventContext& context)
{
    const bool hasSecondaryButton = button2down || button3down;
    if (button1down && hasSecondaryButton) {
        clearSelectionStartPosition();
        if (profile().lockPrimaryAfterMultiButton) {
            lockButton1 = true;
        }
        context.processed = true;
    }

    if (!button1down && !button2down && !button3down) {
        if (lockButton1) {
            lockButton1 = false;
            if (context.initialMode != NavigationStyle::SELECTION) {
                context.processed = true;
            }
        }
    }
    else if (
        button1down && !hasSecondaryButton && profile().lockPrimaryAfterMultiButton
        && (lockButton1 || context.initialMode == NavigationStyle::SPINNING)
        && context.initialMode != NavigationStyle::SELECTION
    ) {
        // Do not turn a multi-button gesture into a selection on release, or interrupt a spin.
        context.resolvedMode = NavigationStyle::IDLE;
    }

    if (viewer->isEditing() && context.initialMode == NavigationStyle::SELECTION
        && context.resolvedMode != NavigationStyle::IDLE) {
        if (profile().editingSelectionPolicy == EditingSelectionPolicy::CancelOnLeftRightChord
            && context.chord == (LeftDown | RightDown)) {
            context.resolvedMode = NavigationStyle::IDLE;
        }
        else {
            context.resolvedMode = NavigationStyle::SELECTION;
        }
        context.processed = false;
    }
}

void MappedNavigationStyle::finalizeModeTransition(EventContext& context)
{
    if (context.resolvedMode != context.initialMode) {
        setViewingMode(context.resolvedMode);
    }
}

void MappedNavigationStyle::resetGestureFlagsIfIdle(const EventContext& context)
{
    if (context.resolvedMode == NavigationStyle::IDLE && !button1down && !button2down
        && !button3down) {
        hasPanned = false;
        hasDragged = false;
        hasZoomed = false;
    }
}

SbBool MappedNavigationStyle::propagateEvent(EventContext& context)
{
    if (!context.processed && shouldPropagate(context)) {
        context.processed = inherited::processSoEvent(context.event);
    }
    return context.processed;
}

void MappedNavigationStyle::processStyleButtonEvent(EventContext&)
{}

bool MappedNavigationStyle::shouldForceRotationWhenButtonAdded(const EventContext&) const
{
    return profile().forceRotationOnAddedButton;
}

bool MappedNavigationStyle::shouldProcessMouseButtonEvent(const SoEvent*) const
{
    return true;
}

bool MappedNavigationStyle::processStylePointerMotionEvent(EventContext&)
{
    return false;
}

void MappedNavigationStyle::adjustResolvedMode(EventContext&)
{}

bool MappedNavigationStyle::shouldPropagate(const EventContext& context) const
{
    return !context.selectionDragAttempted;
}

void MappedNavigationStyle::applyModeEntryEffects(EventContext& context)
{
    if (context.resolvedMode == NavigationStyle::DRAGGING
        && context.initialMode != NavigationStyle::DRAGGING) {
        saveCursorPosition(context.event);
        centerTime = context.event->getTime();
    }
}

void MappedNavigationStyle::prepareMiddleButtonPress(const EventContext& context)
{
    centerTime = context.event->getTime();
    setupPanningPlane(getCamera());
}
