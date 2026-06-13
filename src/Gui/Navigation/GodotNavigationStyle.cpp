// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2026 Trevor Blythe <trevorblythe82@gmail.com>

#include <cmath>

#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMotion3Event.h>
#include "Inventor/SoMouseWheelEvent.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QElapsedTimer>
#include <Base/Rotation.h>
#include "Navigation/NavigationStyle.h"
#include "View3DInventorViewer.h"

namespace Gui
{

// we HAVE to define a seperate object for handling input,
// handling input the way other nav objects do it, fail here,
// because we need to know button press's instantly while holding RMB.
// This is all self contained.
class GodotNavInputEventHandler: public QObject
{
public:
    explicit GodotNavInputEventHandler(Gui::GodotNavigationStyle* nav, QObject* parent = nullptr)
        : QObject(parent)
        , navStyle(nav)
    {}


protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        // -- Mouse move
        if (event->type() == QEvent::MouseMove && navStyle->flyModeActive) {
            auto* me = static_cast<QMouseEvent*>(event);
            QPoint gpos = me->globalPos();

            if (gpos == navStyle->lockPos) {
                return true;
            }

            int dx = gpos.x() - navStyle->lockPos.x();
            int dy = gpos.y() - navStyle->lockPos.y();

            QCursor::setPos(navStyle->lockPos);

            if (dx != 0 || dy != 0) {
                navStyle->applyMouseLook(dx, dy);
            }
            return true;
        }

        // -- Key events
        bool isShortcutOverride = (event->type() == QEvent::ShortcutOverride);
        bool isKeyPress = (event->type() == QEvent::KeyPress);
        bool isRelease = (event->type() == QEvent::KeyRelease);

        if (isShortcutOverride || isKeyPress || isRelease) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (isKeyPress && ke->isAutoRepeat()) {
                return false;
            }
            bool press = isShortcutOverride || isKeyPress;

            if (!navStyle->flyModeActive) {
                return false;
            }

            switch (ke->key()) {
                case Qt::Key_W:
                    navStyle->moveForward = press;
                    break;
                case Qt::Key_S:
                    navStyle->moveBackward = press;
                    break;
                case Qt::Key_A:
                    navStyle->moveLeft = press;
                    break;
                case Qt::Key_D:
                    navStyle->moveRight = press;
                    break;
                case Qt::Key_E:
                    navStyle->moveUp = press;
                    break;
                case Qt::Key_Q:
                    navStyle->moveDown = press;
                    break;
                default:
                    return false;
            }

            ke->accept();
            return true;
        }
        return false;
    }

private:
    Gui::GodotNavigationStyle* navStyle;

    friend class Gui::GodotNavigationStyle;
};

}  // namespace Gui

using namespace Gui;
TYPESYSTEM_SOURCE(Gui::GodotNavigationStyle, Gui::UserNavigationStyle)

GodotNavigationStyle::GodotNavigationStyle()
    : moveForward(false)
    , moveBackward(false)
    , moveLeft(false)
    , moveRight(false)
    , moveUp(false)
    , moveDown(false)
    , flyModeActive(false)
    , upsideDown(false)
    , flySpeed(300.0f)
    , currentYaw(0.0f)
    , currentPitch(0.0f)
    , baseOrientation(SbRotation::identity())
    , keyFilter(nullptr)
{
    flyTimer = new QTimer();
    flyTimer->setInterval(0);
    QObject::connect(flyTimer, &QTimer::timeout, [this]() { this->applyFlyMovement(); });
}

GodotNavigationStyle::~GodotNavigationStyle()
{
    stopFlyMode();
    flyTimer->stop();
    delete flyTimer;
}

void GodotNavigationStyle::startFlyMode(const SoEvent* /*ev*/)
{
    if (flyModeActive) {
        return;
    }

    flyElapsed.start();
    flyModeActive = true;
    upsideDown = false;
    flyTimer->start();
    QApplication::setOverrideCursor(Qt::BlankCursor);
    lockPos = QCursor::pos();

    if (viewer) {
        SoCamera* cam = viewer->getSoRenderManager()->getCamera();
        if (cam) {
            SbRotation camOri = cam->orientation.getValue();

            SbVec3f camRight, bUp;
            camOri.multVec(SbVec3f(1, 0, 0), camRight);
            baseOrientation.multVec(SbVec3f(0, 1, 0), bUp);

            // User used the orientation cube in the top right.
            // Change the horizon to whatever they set.
            if (std::abs(camRight.dot(bUp)) > 0.01f) {
                baseOrientation = camOri;
            }

            // Recalc pitch/yaw as it can change in between flights.
            SbRotation relativeRot = camOri * baseOrientation.inverse();
            SbMatrix relMat;
            relMat.setRotate(relativeRot);
            float tx = relMat[2][0];
            float ty = relMat[2][1];
            float tz = relMat[2][2];
            bool isUpSideDown = (relMat[1][1] < 0);
            currentPitch = -std::asin(qBound(-1.0f, ty, 1.0f)) * 180.0f / M_PI;
            if (isUpSideDown) {
                currentPitch = (currentPitch >= 0) ? (180.0f - currentPitch)
                                                   : (-180.0f - currentPitch);
                currentYaw = std::atan2(-tx, -tz) * 180.0f / M_PI;
            }
            else {
                currentYaw = std::atan2(tx, tz) * 180.0f / M_PI;
            }
            this->upsideDown = isUpSideDown;
            this->applyMouseLook(0, 0);
        }
    }

    if (!keyFilter) {
        keyFilter = new GodotNavInputEventHandler(this, qApp);
        qApp->installEventFilter(keyFilter);
        if (viewer && viewer->getGLWidget()) {
            viewer->getGLWidget()->setFocusPolicy(Qt::StrongFocus);
            viewer->getGLWidget()->setFocus(Qt::MouseFocusReason);
        }
    }
}


void GodotNavigationStyle::stopFlyMode()
{
    if (!flyModeActive) {
        return;
    }
    flyModeActive = false;

    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    flyTimer->stop();
    moveForward = moveBackward = moveLeft = moveRight = moveUp = moveDown = false;

    if (keyFilter) {
        qApp->removeEventFilter(keyFilter);
        delete keyFilter;
        keyFilter = nullptr;
    }
}

void GodotNavigationStyle::applyMouseLook(int dx, int dy)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    const float sensitivity = 0.25f;


    // This fixes any accumulated roll error over time.
    SbVec3f camForward, camRight, camUp;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), camForward);
    cam->orientation.getValue().multVec(SbVec3f(1, 0, 0), camRight);
    cam->orientation.getValue().multVec(SbVec3f(0, 1, 0), camUp);


    SbVec3f baseUp;
    baseOrientation.multVec(SbVec3f(0, 1, 0), baseUp);


    this->upsideDown = (camUp.dot(baseUp) < 0);

    SbVec3f levelRight = camForward.cross(baseUp);

    if (levelRight.length() > 0.001f) {
        levelRight.normalize();


        SbVec3f healedRight = camRight + (levelRight - camRight) * 0.05f;
        healedRight.normalize();


        SbVec3f healedUp = healedRight.cross(camForward);
        healedUp.normalize();

        SbMatrix mat;
        mat.makeIdentity();
        mat[0][0] = healedRight[0];
        mat[0][1] = healedRight[1];
        mat[0][2] = healedRight[2];
        mat[1][0] = healedUp[0];
        mat[1][1] = healedUp[1];
        mat[1][2] = healedUp[2];
        mat[2][0] = -camForward[0];
        mat[2][1] = -camForward[1];
        mat[2][2] = -camForward[2];

        cam->orientation.setValue(SbRotation(mat));
    }

    // Applying mouse look
    currentYaw -= dx * sensitivity * (upsideDown ? -1.0f : 1.0f);
    currentPitch -= dy * sensitivity;

    float lrRad = currentYaw * (float)M_PI / 180.0f;
    float udRad = currentPitch * (float)M_PI / 180.0f;

    SbRotation lrRot(SbVec3f(0, 1, 0), lrRad);
    SbRotation afterYaw = lrRot * baseOrientation;

    SbVec3f localX;
    afterYaw.multVec(SbVec3f(1, 0, 0), localX);
    localX.normalize();
    SbRotation pitch(localX, udRad);

    cam->orientation = afterYaw * pitch;

    viewer->getSoRenderManager()->scheduleRedraw();
}
const char* GodotNavigationStyle::mouseButtons(ViewerMode mode)
{
    switch (mode) {
        case NavigationStyle::SELECTION:
            return QT_TR_NOOP("Press left mouse button");
        case NavigationStyle::PANNING:
            return QT_TR_NOOP("Hold right click and press 'ad'");
        case NavigationStyle::DRAGGING:
            return QT_TR_NOOP("Hold right click and press 'wasd'");
        case NavigationStyle::ZOOMING:
            return QT_TR_NOOP("Hold right click and press 'w/s'");
        default:
            return "No description";
    }
}
void GodotNavigationStyle::applyFlyMovement()
{
    if (!flyModeActive) {
        return;
    }

    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    bool fast = (QApplication::queryKeyboardModifiers() & Qt::ShiftModifier);
    bool anyMove = moveForward || moveBackward || moveLeft || moveRight || moveUp || moveDown;
    if (!anyMove) {
        flyElapsed.restart();
        return;
    }

    float dt = flyElapsed.isValid() ? flyElapsed.elapsed() / 1000.0f : 0.016f;
    flyElapsed.restart();
    dt = qMin(dt, 0.1f);

    float speed = flySpeed * dt * (fast ? 4.0f : 1.0f);

    SbVec3f forward, right, up;
    cam->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), forward);
    cam->orientation.getValue().multVec(SbVec3f(1.0f, 0.0f, 0.0f), right);
    cam->orientation.getValue().multVec(SbVec3f(0.0f, 1.0f, 0.0f), up);

    SbVec3f movement(0.0f, 0.0f, 0.0f);
    if (moveForward) {
        movement += forward;
    }
    if (moveBackward) {
        movement -= forward;
    }
    if (moveRight) {
        movement += right;
    }
    if (moveLeft) {
        movement -= right;
    }
    if (moveUp) {
        movement += up;
    }
    if (moveDown) {
        movement -= up;
    }

    if (movement.length() > 0.0f) {
        movement.normalize();
        movement *= speed;
    }

    cam->position = cam->position.getValue() + movement;
}

SbBool GodotNavigationStyle::processSoEvent(const SoEvent* const ev)
{
    if (this->isSeekMode()) {
        return inherited::processSoEvent(ev);
    }
    if (!this->isSeekMode() && !this->isAnimating() && this->isViewing()) {
        this->setViewing(false);
    }

    const SoType type(ev->getTypeId());

    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s pos(ev->getPosition());
    const SbVec2f posn = normalizePixelPos(pos);
    const SbVec2f prevnormalized = this->lastmouseposition;
    this->lastmouseposition = posn;

    SbBool processed = false;
    const ViewerMode curmode = this->currentmode;
    ViewerMode newmode = curmode;

    syncModifierKeys(ev);

    if (!viewer->isEditing()) {
        processed = handleEventInForeground(ev);
        if (processed) {
            return true;
        }
    }

    // WASD/QE are handled by GodotNavInputEventHandler; handle other keys here.
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        if (!flyModeActive) {
            processed = processKeyboardEvent(static_cast<const SoKeyboardEvent*>(ev));
        }
    }

    // -- Mouse buttons
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto* const event = static_cast<const SoMouseButtonEvent*>(ev);
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN;

        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                this->button1down = press;
                if (press && this->currentmode == NavigationStyle::SEEK_WAIT_MODE) {
                    newmode = NavigationStyle::SEEK_MODE;
                    this->seekToPoint(pos);
                    processed = true;
                }
                else {
                    processed = processClickEvent(event);
                }
                break;

            case SoMouseButtonEvent::BUTTON2:
                this->button2down = press;
                if (press) {
                    button2PressPos = pos;
                    saveCursorPosition(ev);
                    this->centerTime = ev->getTime();
                    newmode = NavigationStyle::IDLE;
                    processed = true;
                }
                else {
                    bool wasFlyMode = flyModeActive;
                    stopFlyMode();
                    if (!wasFlyMode && this->isPopupMenuEnabled() && !viewer->isEditing()) {
                        this->openPopupMenu(event->getPosition());
                    }
                    newmode = NavigationStyle::IDLE;
                    processed = true;
                }
                break;

            case SoMouseButtonEvent::BUTTON3:
                if (press) {
                    this->centerTime = ev->getTime();
                    setupPanningPlane(getCamera());
                    this->lockrecenter = false;
                }
                else {
                    SbTime tmp = (ev->getTime() - this->centerTime);
                    float dci = (float)QApplication::doubleClickInterval() / 1000.0f;
                    if (tmp.getValue() < dci && !this->lockrecenter) {
                        lookAtPoint(pos);
                        processed = true;
                    }
                }
                this->button3down = press;
                break;

            default:
                break;
        }
    }

    // -- Mouse move (to track when to start fly mode)
    if (type.isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        const auto* const locEvent = static_cast<const SoLocation2Event*>(ev);

        if (flyModeActive) {
            processed = true;
        }
        else if (button2down && !flyModeActive) {
            int dx = pos[0] - button2PressPos[0];
            int dy = pos[1] - button2PressPos[1];
            if ((dx * dx + dy * dy) > 25) {
                startFlyMode(ev);
            }
            processed = true;
        }
        else if (this->currentmode == NavigationStyle::ZOOMING) {
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
            this->addToLog(locEvent->getPosition(), locEvent->getTime());
            this->spin(posn);
            moveCursorPosition();
            processed = true;
        }
    }

    // -- Scroll wheel
    if (ev->isOfType(SoMouseWheelEvent::getClassTypeId())) {
        const auto* const event = static_cast<const SoMouseWheelEvent*>(ev);
        int delta = event->getDelta();
        if (flyModeActive) {
            SoCamera* cam = viewer->getSoRenderManager()->getCamera();
            float focalDist = cam ? cam->focalDistance.getValue() : 100.0f;
            float factor = qBound(1.05f, 1.0f + focalDist / 2000.0f, 6.0f);
            flySpeed *= (delta > 0) ? factor : (1.0f / factor);
            flySpeed = qBound(0.01f, flySpeed, 10000.0f);
            processed = true;
        }
    }

    // -- Spaceball / joystick
    if (type.isDerivedFrom(SoMotion3Event::getClassTypeId())) {
        const auto* const event = static_cast<const SoMotion3Event*>(ev);
        if (event) {
            this->processMotionEvent(event);
        }
        processed = true;
    }

    // -- Mode state machine (middle mouse orbit/pan)
    enum
    {
        BUTTON1DOWN = 1 << 0,
        BUTTON3DOWN = 1 << 1,
        CTRLDOWN = 1 << 2,
        SHIFTDOWN = 1 << 3,
        BUTTON2DOWN = 1 << 4
    };
    unsigned int combo = (this->button1down ? BUTTON1DOWN : 0)
        | (this->button2down ? BUTTON2DOWN : 0) | (this->button3down ? BUTTON3DOWN : 0)
        | (this->ctrldown ? CTRLDOWN : 0) | (this->shiftdown ? SHIFTDOWN : 0);

    if (!flyModeActive) {
        switch (combo) {
            case 0:
                if (curmode != NavigationStyle::SPINNING) {
                    newmode = NavigationStyle::IDLE;
                }
                break;
            case BUTTON1DOWN:
            case CTRLDOWN | BUTTON1DOWN:
                if (curmode != NavigationStyle::SPINNING) {
                    newmode = NavigationStyle::SELECTION;
                }
                break;
            case SHIFTDOWN | BUTTON3DOWN:
                newmode = NavigationStyle::PANNING;
                this->lockrecenter = true;
                break;
            case BUTTON3DOWN:
                if (newmode != NavigationStyle::DRAGGING) {
                    saveCursorPosition(ev);
                }
                newmode = NavigationStyle::DRAGGING;
                this->lockrecenter = true;
                break;
            case CTRLDOWN | BUTTON3DOWN:
                newmode = NavigationStyle::ZOOMING;
                this->lockrecenter = true;
                break;
            default:
                if ((curmode == NavigationStyle::PANNING || curmode == NavigationStyle::ZOOMING)
                    && !this->button3down) {
                    newmode = NavigationStyle::IDLE;
                }
                break;
        }
    }

    if (newmode == IDLE && !button1down && !button2down && !button3down) {
        hasPanned = false;
        hasDragged = false;
        hasZoomed = false;
    }

    if (newmode != curmode) {
        this->setViewingMode(newmode);
    }

    if (!processed) {
        processed = inherited::processSoEvent(ev);
    }

    return processed;
}
