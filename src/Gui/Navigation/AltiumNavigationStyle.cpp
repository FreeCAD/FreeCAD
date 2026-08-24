/*
TODO Updates:
  - Implement altium nav style for technical drawings, update QGVPage.cpp
  - Rotate and zoom on mouse cursor
 */

#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbVec2f.h>
#include "Inventor/SoMouseWheelEvent.h"
#include <QApplication>

#include "Navigation/NavigationStyle.h"
#include "View3DInventorViewer.h"

#include <Base/Console.h>

// forward declarations
class SoEvent;
class SoMouseWheelEvent;
class SoMotion3Event;
class SoQtViewer;
class SoCamera;
class SoSensor;
class SbSphereSheetProjector;

using namespace Gui;

// Register the class to FreeCAD's type system
TYPESYSTEM_SOURCE(Gui::AltiumNavigationStyle, Gui::UserNavigationStyle)

AltiumNavigationStyle::AltiumNavigationStyle()
    : UserNavigationStyle()
{}

AltiumNavigationStyle::~AltiumNavigationStyle() = default;

const char* AltiumNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Press left mouse button");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Press right mouse button");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Press shift and right mouse button");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Scroll middle Mouse Button");
        default:
            return "No description";
    }
}

SbBool AltiumNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing()) {
        this->setViewing(false);  // by default disable viewing mode to render the scene
    }

    const SoType type(ev->getTypeId());

    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s pos(ev->getPosition());         // 2d vector in short vector format
    const SbVec2f posn = normalizePixelPos(pos);  // 2d vector in float format

    // posn & lastmouseposition is latest mouse position
    // lastmouseposition used within NavigationStyle?
    const SbVec2f prevposn = this->lastmouseposition;
    this->lastmouseposition = posn;

    // Set to true if any event processing happened. Note that it is not
    // necessary to restrict ourselves to only do one "action" for an
    // event, we only need this flag to see if any processing happened
    // at all.
    SbBool processed = false;

    const ViewerMode curmode = this->currentmode;
    ViewerMode newmode = curmode;
    // Base::Console().message("curmode init = %i\n", curmode);
    // Base::Console().message("curmode init: %u\n", static_cast<int>(curmode));

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    syncModifierKeys(ev);  // this updates ctrl, shift and alt key states

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        processed = handleEventInForeground(ev);
        if (processed) {
            return true;
        }
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const auto event = static_cast<const SoKeyboardEvent*>(ev);
        processed = processKeyboardEvent(event);
    }

    // Mouse Button / Spaceball Button handling / NO mouse scroll
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto event = (const SoMouseButtonEvent*)ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        // Base::Console().message("mouse button = %d\n", button);

        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                this->lockrecenter = true;
                this->button1down = press;

                processed = processClickEvent(event);
                break;
            // button2 may be pressed for panning or dragging/rotating, or context menu
            case SoMouseButtonEvent::BUTTON2:
                // If we are in edit mode then simply ignore the RMB events
                // to pass the event to the base class.
                this->lockrecenter = true;
                this->button2down = press;

                // !press means all buttons are up (has been released)
                // Don't show the context menu after dragging, panning or zooming
                // Only panning and dragging are important here since zooming is not done with button2
                if (!press && (hasDragged || hasPanned || hasZoomed)) {
                    processed = true;
                }
                else if (!press && !viewer->isEditing()) {
                    if (curmode != NavigationStyle::ZOOMING && curmode != NavigationStyle::DRAGGING) {
                        if (this->isPopupMenuEnabled()) {
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }
                break;
            // if pressing button3 (MMB), then we are zooming
            case SoMouseButtonEvent::BUTTON3:
                this->button3down = press;
                newmode = NavigationStyle::ZOOMING;
                break;
            default:
                break;
        }
    }

    // Base::Console().message("curmode aftr mousebuttons: %u\n", static_cast<int>(curmode));

    // Mouse scroll wheel
    if (type.isDerivedFrom(SoMouseWheelEvent::getClassTypeId())) {
        const auto event = (const SoMouseWheelEvent*)ev;
        int scroll_delta = event->getDelta();
        float scroll_norm = scroll_delta / (120.0 * 30.0);

        // Base::Console().message("mouse wheel delta %0.2f\n", scroll_norm);

        // if ctrl gets pressed then handle zooming by not setting processed = true.
        if (this->ctrldown == false) {
            newmode = NavigationStyle::PANNING;
            this->setViewingMode(newmode);

            float posn_x, posn_y;
            posn.getValue(posn_x, posn_y);

            if (this->shiftdown) {
                posn_x -= scroll_norm;
            }
            else {
                posn_y += scroll_norm;
            }

            SbVec2f scroll_posn = SbVec2f(posn_x, posn_y);

            float ratio = vp.getViewportAspectRatio();
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                ratio,
                this->panningplane,
                posn,
                scroll_posn
            );

            newmode = NavigationStyle::IDLE;
            this->setViewingMode(newmode);

            processed = true;
        }
    }
    // Base::Console().message("curmode aftr scroll: %u\n", static_cast<int>(curmode));

    // Mouse Movement handling for zooming, dragging, panning
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const auto event = (const SoLocation2Event*)ev;
        // Base::Console().message("mouse movement curmode %u\n", static_cast<int>(curmode)); TODO delete
        if (curmode == NavigationStyle::ZOOMING) {
            this->setZoomAtCursor(true);
            this->zoomByCursor(posn, prevposn);
            processed = true;
        }
        else if (curmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(viewer->getSoRenderManager()->getCamera(), ratio, this->panningplane, posn, prevposn);
            processed = true;
        }
        else if (curmode == NavigationStyle::DRAGGING) {
            // TODO change the rotationcenter location only if starting a drag
            // shift down only locks and displays cursor position. need to also right click
            // to actually drag

            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);  // this performs the spin
            moveCursorPosition();
            processed = true;
        }
    }
    // Base::Console().message("curmode aftr mouse move: %u\n", static_cast<int>(curmode));

    // Spaceball & Joystick handling
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const auto event = static_cast<const SoMotion3Event*>(ev);
        if (event) {
            this->processMotionEvent(event);
        }
        processed = true;
    }

    enum
    {
        BUTTON1DOWN = 1 << 0,  // LMB
        BUTTON3DOWN = 1 << 1,  // MMB
        CTRLDOWN = 1 << 2,
        SHIFTDOWN = 1 << 3,
        BUTTON2DOWN = 1 << 4  // RMB
    };
    unsigned int combo = (this->button1down ? BUTTON1DOWN : 0)
        | (this->button2down ? BUTTON2DOWN : 0) | (this->button3down ? BUTTON3DOWN : 0)
        | (this->ctrldown ? CTRLDOWN : 0) | (this->shiftdown ? SHIFTDOWN : 0);

    // Base::Console().message("Combo: %i\n", combo);
    switch (combo) {
        case 0:  // no button pressed
            newmode = NavigationStyle::IDLE;
            // The left mouse button has been released right now so unlock the flag
            if (this->lockButton1) {
                this->lockButton1 = false;
                if (curmode != NavigationStyle::SELECTION) {
                    processed = true;
                }
            }
            break;

        // BUTTON1 KEY COMBINATIONS
        // multi-selection
        case BUTTON1DOWN | CTRLDOWN:
        case BUTTON1DOWN | SHIFTDOWN:
            // make sure not to change the selection when stopping spinning
            /*
            if ( curmode == NavigationStyle::SPINNING
                || (this->lockButton1 && curmode != NavigationStyle::SELECTION)
                || (curmode == NavigationStyle::DRAGGING) )
            {
                newmode = NavigationStyle::IDLE;
            }
            else
            {
                newmode = NavigationStyle::SELECTION;
            }*/
            newmode = NavigationStyle::SELECTION;
            break;

        // BUTTON2 KEY COMBINATIONS
        case BUTTON2DOWN:
            newmode = NavigationStyle::PANNING;
            processed = true;
            break;

        case BUTTON2DOWN | SHIFTDOWN:
            newmode = NavigationStyle::DRAGGING;
            processed = true;
            break;

        case BUTTON2DOWN | CTRLDOWN | SHIFTDOWN:  // zoom
        case BUTTON2DOWN | CTRLDOWN:              // zoom
            newmode = NavigationStyle::ZOOMING;
            processed = true;
            break;

        // BUTTON3 KEY COMBINATIONS not here since all combos result in zooming

        // KEYBOARD KEYS ONLY
        case SHIFTDOWN:
            // start of a drag, shift was pressed first
            // or panning across x-axis
            // or multi select

            // TODO change the rotationcenter location only if starting a drag
            // shift down only locks and displays cursor position. need to also right click
            // to actually drag

            newmode = NavigationStyle::IDLE;
            // Not processed as SHIFT is also used for multi-select
            break;
        case CTRLDOWN:
            // if only ctrl is down, then go to idle, for example if button2 was released
            newmode = NavigationStyle::IDLE;
            // Not processed as ctrl is also used for multi-select
            break;
        default:
            break;
    }

    // Base::Console().message("curmode aftr combo: %u\n", static_cast<int>(curmode));

    // If the selection button is pressed together with another button
    // and the other button is released, don't switch to selection mode.
    // Process when selection button is pressed together with other buttons that could trigger
    // different actions.
    if (this->button1down && (this->button2down || this->button3down)) {
        this->lockButton1 = true;
        processed = true;
    }

    // Prevent interrupting rubber-band selection in sketcher
    if (viewer->isEditing() && curmode == NavigationStyle::SELECTION
        && newmode != NavigationStyle::IDLE) {
        if (!button1down || !button2down) {  // Allow canceling rubber-band in sketcher if both
                                             // button 1 and button 2 are pressed
            newmode = NavigationStyle::SELECTION;
        }
        processed = false;
    }

    // Reset flags when newmode is IDLE and the buttons are released
    if (newmode == IDLE && !button1down && !button2down && !button3down) {
        hasPanned = false;
        hasDragged = false;
        hasZoomed = false;
    }

    // Base::Console().message("curmode b4 setviewmode: %u\n", static_cast<int>(curmode));
    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (!processed) {
        // Base::Console().message("processSoEvent\n");
        processed = inherited::processSoEvent(ev);  // this will handle zoom by scroll or other things
    }
    // Base::Console().message("After processSoEvent %i\n", processed);
    return processed;
}
