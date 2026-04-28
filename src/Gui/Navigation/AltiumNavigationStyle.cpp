#include "PreCompiled.h"

#include "View3DInventorViewer.h"
#include "Application.h"

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseWheelEvent.h>

using namespace Gui;

// Register the class to FreeCAD's type system
TYPESYSTEM_SOURCE(Gui::AltiumNavigationStyle, Gui::UserNavigationStyle)

AltiumNavigationStyle::AltiumNavigationStyle() = default;

AltiumNavigationStyle::~AltiumNavigationStyle() = default;

const char* AltiumNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case SELECTION:
            return "Left Click";
        case PANNING:
            return "Right Click";
        case SPINNING:
            return "Shift + Right Click";
        case ZOOMING:
            return "Middle Mouse Button / Scroll";
        default:
            return "Altium Navigation";
    }
}

SbBool AltiumNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    // Synchronize modifier keys (Shift, Ctrl, Alt) with the base class
    syncModifierKeys(ev);

    // Process mouse button presses and releases
    if (ev->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent* const e = static_cast<const SoMouseButtonEvent*>(ev);
        const SbBool press = e->getState() == SoButtonEvent::DOWN;
        const int button = e->getButton();

        if (press) {
            // Left Click -> Selection
            if (button == SoMouseButtonEvent::BUTTON1) {
                this->button1down = true;
                // Base class handles selection math during IDLE state
            }
            // Middle Click -> Zooming
            else if (button == SoMouseButtonEvent::BUTTON2) {
                this->button2down = true;
                if (this->currentmode == IDLE) {
                    this->currentmode = ZOOMING;
                    this->lastmouseposition = e->getPosition();
                }
            }
            // Right Click -> Panning OR Spinning
            else if (button == SoMouseButtonEvent::BUTTON3) {
                this->button3down = true;
                if (this->currentmode == IDLE) {
                    if (e->wasShiftDown()) {
                        this->currentmode = SPINNING;  // Shift + Right Click
                    }
                    else {
                        this->currentmode = PANNING;  // Right Click only
                    }
                    this->lastmouseposition = e->getPosition();
                }
            }
        }
        else {  // Button Release
            if (button == SoMouseButtonEvent::BUTTON1) {
                this->button1down = false;
            }
            else if (button == SoMouseButtonEvent::BUTTON2) {
                this->button2down = false;
                if (this->currentmode == ZOOMING) {
                    this->currentmode = IDLE;
                }
            }
            else if (button == SoMouseButtonEvent::BUTTON3) {
                this->button3down = false;
                if (this->currentmode == PANNING || this->currentmode == SPINNING) {
                    this->currentmode = IDLE;
                }
            }
        }
    }

    // Let the base UserNavigationStyle handle the actual 3D math (selection picking,
    // applying camera translations for panning, or rotations for spinning)
    // based on the currentmode state we just mapped above.
    return UserNavigationStyle::processSoEvent(ev);
}