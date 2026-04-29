#include <Inventor/nodes/SoCamera.h>
#include <QApplication>

#include "Navigation/NavigationStyle.h"
#include "View3DInventorViewer.h"

using namespace Gui;

// Register the class to FreeCAD's type system
TYPESYSTEM_SOURCE(Gui::AltiumNavigationStyle, Gui::UserNavigationStyle)

AltiumNavigationStyle::AltiumNavigationStyle() = default;

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
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn = normalizePixelPos(pos);

    const SbVec2f prevnormalized = this->lastmouseposition;
    this->lastmouseposition = posn;

    // Set to true if any event processing happened. Note that it is not
    // necessary to restrict ourselves to only do one "action" for an
    // event, we only need this flag to see if any processing happened
    // at all.
    SbBool processed = false;

    const ViewerMode curmode = this->currentmode;
    ViewerMode newmode = curmode;

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    syncModifierKeys(ev);

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

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto event = (const SoMouseButtonEvent*)ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        // SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                this->lockrecenter = true;
                this->button1down = press;
                if (press && (this->currentmode == NavigationStyle::SEEK_WAIT_MODE)) {
                    newmode = NavigationStyle::SEEK_MODE;
                    this->seekToPoint(pos);  // implicitly calls interactiveCountInc()
                    processed = true;
                }
                else if (press
                         && (this->currentmode == NavigationStyle::PANNING
                             || this->currentmode == NavigationStyle::ZOOMING)) {
                    newmode = NavigationStyle::DRAGGING;
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                    processed = true;
                }
                else if (!press && (this->currentmode == NavigationStyle::DRAGGING)) {
                    processed = true;
                }
                else if (viewer->isEditing() && (this->currentmode == NavigationStyle::SPINNING)) {
                    processed = true;
                }
                else {
                    processed = processClickEvent(event);
                }
                break;

            // LMB
            case SoMouseButtonEvent::BUTTON2: //changed from button2 to button3 for panning
                // If we are in edit mode then simply ignore the RMB events
                // to pass the event to the base class.
                this->lockrecenter = true;
                this->button2down = press;

                // Don't show the context menu after dragging, panning or zooming
                if (!press && (hasDragged || hasPanned || hasZoomed)) {
                    processed = true;
                }
                else if (!press && !viewer->isEditing()) {
                    if (this->currentmode != NavigationStyle::ZOOMING
                        && this->currentmode != NavigationStyle::PANNING
                        && this->currentmode != NavigationStyle::DRAGGING) {
                        if (this->isPopupMenuEnabled()) {
                            this->openPopupMenu(event->getPosition());
                        }
                    }
                }

                // Alternative way of rotating & zooming
                if (press
                    && (this->currentmode == NavigationStyle::PANNING
                        || this->currentmode == NavigationStyle::ZOOMING)) {
                    newmode = NavigationStyle::DRAGGING;
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                    processed = true;
                }

                break;

            // MMB
            case SoMouseButtonEvent::BUTTON3: //changed from button3 to button2
                this->button3down = press;
                if (press) {
                    this->centerTime = ev->getTime();
                    setupPanningPlane(getCamera());
                    this->lockrecenter = false;
                }
                else if (curmode == NavigationStyle::PANNING) {
                    newmode = NavigationStyle::IDLE;
                    processed = true;
                }
                /*
                else {
                    SbTime tmp = (ev->getTime() - this->centerTime);
                    float dci = (float)QApplication::doubleClickInterval() / 1000.0f;
                    // is it just a middle click?
                    if (tmp.getValue() < dci && !this->lockrecenter) {
                        lookAtPoint(pos);
                        processed = true;
                    }
                }*/
                break;
            default:
                break;
        }
    }

    // Mouse Movement handling
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        this->lockrecenter = true;
        const auto event = (const SoLocation2Event*)ev;
        if (this->currentmode == NavigationStyle::ZOOMING) {
            this->zoomByCursor(posn, prevnormalized);
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::PANNING) {
            float ratio = vp.getViewportAspectRatio();
            panCamera(
                viewer->getSoRenderManager()->getCamera(),
                ratio,
                this->panningplane,
                posn,
                prevnormalized
            );
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::DRAGGING) {
            this->addToLog(event->getPosition(), event->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = true;
        }
    }

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
        BUTTON1DOWN = 1 << 0, // LMB
        BUTTON3DOWN = 1 << 1, // MMB
        CTRLDOWN = 1 << 2,
        SHIFTDOWN = 1 << 3,
        BUTTON2DOWN = 1 << 4    // RMB
    };
    unsigned int combo = (this->button1down ? BUTTON1DOWN : 0)
        | (this->button2down ? BUTTON2DOWN : 0) | (this->button3down ? BUTTON3DOWN : 0)
        | (this->ctrldown ? CTRLDOWN : 0) | (this->shiftdown ? SHIFTDOWN : 0);

    switch (combo) {
        case 0:
            if (curmode == NavigationStyle::SPINNING) {
                break;
            }
            newmode = NavigationStyle::IDLE;
            // The left mouse button has been released right now
            if (this->lockButton1) {
                this->lockButton1 = false;
                if (curmode != NavigationStyle::SELECTION) {
                    processed = true;
                }
            }
            break;
        // for mouse movement do nothing with just button1down
        case BUTTON1DOWN:

        // multi-selection
        case CTRLDOWN | BUTTON1DOWN:
            // make sure not to change the selection when stopping spinning
            if (curmode == NavigationStyle::SPINNING
                || (this->lockButton1 && curmode != NavigationStyle::SELECTION)) {
                newmode = NavigationStyle::IDLE;
            }
            else {
                newmode = NavigationStyle::SELECTION;
            }
            break;
        // do nothing
        case BUTTON1DOWN | BUTTON2DOWN:

        case BUTTON2DOWN: //changed from BUTTON3DOWN
            newmode = NavigationStyle::PANNING;
            break;

        case SHIFTDOWN | BUTTON2DOWN:  //changed from button2down
            if (newmode != NavigationStyle::DRAGGING) {
                saveCursorPosition(ev);
            }
            newmode = NavigationStyle::DRAGGING;
            break;
        case CTRLDOWN | SHIFTDOWN | BUTTON2DOWN:
        
        case CTRLDOWN | BUTTON2DOWN:
            NEWMODE = NavigationStyle::ZOOMING;
            break;

        case BUTTON3DOWN | CTRLDOWN:
        case BUTTON3DOWN | SHIFTDOWN:
        case BUTTON3DOWN:
            newmode = NavigationStyle::ZOOMING;
            break;

        default:
            // Reset mode to IDLE when button 2 is released
            // This stops the DRAGGING when button 2is released but SHIFT is still pressed
            // This stops the ZOOMING when button 2 is released but CTRL is still pressed
            if ((curmode == NavigationStyle::DRAGGING || curmode == NavigationStyle::ZOOMING)
                && !this->button2down) {
                newmode = NavigationStyle::IDLE;
            }
            break;
    }

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

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    // If not handled in this class, pass on upwards in the inheritance
    // hierarchy.
    if (!processed) {
        processed = inherited::processSoEvent(ev);
    }
    return processed;
}