/***************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QScrollBar>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "QGSPage.h"
#include "QGVNavStyle.h"
#include "QGVPage.h"


using namespace TechDraw;
using namespace TechDrawGui;

namespace TechDrawGui
{

QGVNavStyle::QGVNavStyle(QGVPage* qgvp) : m_viewer(qgvp) { initialize(); }

QGVNavStyle::~QGVNavStyle() {}

void QGVNavStyle::initialize()
{
    this->button1down = false;
    this->button2down = false;
    this->button3down = false;
    this->ctrldown = false;
    this->shiftdown = false;
    this->altdown = false;
    this->invertZoom = App::GetApplication()
                           .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                           ->GetBool("InvertZoom", true);
    this->zoomAtCursor = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                             ->GetBool("ZoomAtCursor", true);
    this->zoomStep = App::GetApplication()
                         .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                         ->GetFloat("ZoomStep", 0.2f);

    m_reversePan = Preferences::getPreferenceGroup("General")->GetInt("KbPan", 1);
    m_reverseScroll = Preferences::getPreferenceGroup("General")->GetInt("KbScroll", 1);

    panningActive = false;
    zoomingActive = false;
    m_clickPending = false;
    m_panPending = false;
    m_zoomPending = false;
    m_clickButton = Qt::NoButton;
    m_saveCursor = getViewer()->cursor();
    m_wheelDeltaCounter = 0;
    m_mouseDeltaCounter = 0;
}

void QGVNavStyle::setAnchor()
{
    if (m_viewer) {
        if (zoomAtCursor) {
            m_viewer->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
            m_viewer->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        }
        else {
            m_viewer->setResizeAnchor(QGraphicsView::AnchorViewCenter);
            m_viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        }
    }
}

void QGVNavStyle::handleEnterEvent(QEvent* event)
{
    Q_UNUSED(event);
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->getBalloonCursor()->hide();
    }
}

void QGVNavStyle::handleFocusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    getViewer()->cancelBalloonPlacing();
}

void QGVNavStyle::handleKeyPressEvent(QKeyEvent* event)
{
//    Base::Console().Message("QGNS::handleKeyPressEvent(%d)\n", event->key());
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        switch (event->key()) {
            case Qt::Key_Plus: {
                zoomIn();
                event->accept();
                return;
            }
            case Qt::Key_Minus: {
                zoomOut();
                event->accept();
                return;
            }
            default: {
                return;
            }
        }
    }

    if (event->modifiers().testFlag(Qt::NoModifier)) {
        switch (event->key()) {
            case Qt::Key_Left: {
                getViewer()->kbPanScroll(1, 0);
                event->accept();
                return;
            }
            case Qt::Key_Up: {
                getViewer()->kbPanScroll(0, 1);
                event->accept();
                return;
            }
            case Qt::Key_Right: {
                getViewer()->kbPanScroll(-1, 0);
                event->accept();
                return;
            }
            case Qt::Key_Down: {
                getViewer()->kbPanScroll(0, -1);
                event->accept();
                return;
            }
            case Qt::Key_Escape: {
                getViewer()->cancelBalloonPlacing();
                event->accept();
                return;
            }
            case Qt::Key_Shift: {
                this->shiftdown = true;
                event->accept();
                return;
            }
            default: {
                return;
            }
        }
    }
}

void QGVNavStyle::handleKeyReleaseEvent(QKeyEvent* event)
{
    //    Q_UNUSED(event);
    if (event->modifiers().testFlag(Qt::NoModifier)) {
        switch (event->key()) {
            case Qt::Key_Shift: {
                this->shiftdown = false;
                event->accept();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void QGVNavStyle::handleLeaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    if (getViewer()->isBalloonPlacing()) {
        int left_x;
        if (getViewer()->getBalloonCursorPos().x() < 32)
            left_x = 0;
        else if (getViewer()->getBalloonCursorPos().x()
                 > (getViewer()->contentsRect().right() - 32))
            left_x = getViewer()->contentsRect().right() - 32;
        else
            left_x = getViewer()->getBalloonCursorPos().x();

        int left_y;
        if (getViewer()->getBalloonCursorPos().y() < 32)
            left_y = 0;
        else if (getViewer()->getBalloonCursorPos().y()
                 > (getViewer()->contentsRect().bottom() - 32))
            left_y = getViewer()->contentsRect().bottom() - 32;
        else
            left_y = getViewer()->getBalloonCursorPos().y();

        /* When cursor leave the page, display getViewer()->balloonCursor where it left */
        getViewer()->getBalloonCursor()->setGeometry(left_x, left_y, 32, 32);
        getViewer()->getBalloonCursor()->show();
    }
}

void QGVNavStyle::handleMousePressEvent(QMouseEvent* event)
{
    //    Base::Console().Message("QGVNS::handleMousePressEvent()\n");
    if (!panningActive && (event->button() == Qt::MiddleButton)) {
        startPan(event->pos());
        event->accept();
    }
}

void QGVNavStyle::handleMouseMoveEvent(QMouseEvent* event)
{
    //    Base::Console().Message("QGVNS::handleMouseMoveEvent()\n");
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->setBalloonCursorPos(event->pos());
    }

    if (panningActive) {
        pan(event->pos());
        event->accept();
    }
}

//NOTE: QGraphicsView::contextMenuEvent consumes the mouse release event for the
//button that caused the event (typically RMB)
void QGVNavStyle::handleMouseReleaseEvent(QMouseEvent* event)
{
    //    Base::Console().Message("QGVNS::handleMouseReleaseEvent()\n");
    if (getViewer()->isBalloonPlacing()) {
        placeBalloon(event->pos());
    }

    if (panningActive && (event->button() == Qt::MiddleButton)) {
        stopPan();
        event->accept();
    }
}

bool QGVNavStyle::allowContextMenu(QContextMenuEvent* event)
{
    Q_UNUSED(event)
    //    Base::Console().Message("QGVNS::allowContextMenu()\n");
    //    if (event->reason() == QContextMenuEvent::Mouse) {
    //        //must check for a button combination involving context menu button
    //    }
    return true;
}

void QGVNavStyle::pseudoContextEvent() { getViewer()->pseudoContextEvent(); }

void QGVNavStyle::handleWheelEvent(QWheelEvent* event)
{
    //gets called once for every click of the wheel. the sign of event->angleDelta().y()
    //gives the direction of wheel rotation. positive indicates rotation forwards away
    //from the user; negative backwards toward the user. the magnitude of
    //event->angleDelta().y() is 120 for most mice which represents 120/8 = 15 degrees of
    //rotation. Some high resolution mice/trackpads report smaller values - ie a click is less than
    //15 degrees of wheel rotation.
    //https://doc.qt.io/qt-5/qwheelevent.html#angleDelta
    //to avoid overly sensitive behaviour in high resolution mice/touchpads,
    //save up wheel clicks until the wheel has rotated at least 15 degrees.
    constexpr int wheelDeltaThreshold = 120;
    m_wheelDeltaCounter += std::abs(event->angleDelta().y());
    if (m_wheelDeltaCounter < wheelDeltaThreshold) {
        return;
    }
    m_wheelDeltaCounter = 0;
    //starting with -ve direction keeps us in sync with the behaviour of the 3d window
    int rotationDirection = -event->angleDelta().y() / std::abs(event->angleDelta().y());
    if (invertZoom) {
        rotationDirection = -rotationDirection;
    }
    double zoomFactor = 1 + rotationDirection * zoomStep;
    zoom(zoomFactor);
}

void QGVNavStyle::zoom(double factor)
{
    constexpr double minimumScale(0.01);
    QTransform transform = getViewer()->transform();
    double xScale = transform.m11();
    if (xScale <= minimumScale && factor < 1.0) {
        //don't scale any smaller than this
        return;
    }

    setAnchor();
    getViewer()->scale(factor, factor);
    m_zoomPending = false;
}

void QGVNavStyle::startZoom(QPoint p)
{
    //    Base::Console().Message("QGVNS::startZoom(%s)\n", TechDraw::DrawUtil::formatVector(p).c_str());
    zoomOrigin = p;
    zoomingActive = true;
    m_zoomPending = false;
    getViewer()->setZoomCursor();
}

void QGVNavStyle::stopZoom()
{
    //    Base::Console().Message("QGVNS::stopZoom()\n");
    zoomingActive = false;
    m_zoomPending = false;
    getViewer()->resetCursor();
}

double QGVNavStyle::mouseZoomFactor(QPoint p)
{
    constexpr int threshold(20);
    int verticalTravel = (p - zoomOrigin).y();
    m_mouseDeltaCounter += std::abs(verticalTravel);
    if (m_mouseDeltaCounter < threshold) {
        //do not zoom yet
        return 1.0;
    }
    m_mouseDeltaCounter = 0;
    double direction = verticalTravel / std::abs(verticalTravel);
    if (invertZoom) {
        direction = -direction;
    }
    double factor = 1.0 + (direction * zoomStep);
    zoomOrigin = p;
    return factor;
}

void QGVNavStyle::zoomIn()
{
    zoom(1.0 + zoomStep);
}

void QGVNavStyle::zoomOut()
{
    zoom(1.0 - zoomStep);
}

void QGVNavStyle::startPan(QPoint p)
{
    panOrigin = p;
    panningActive = true;
    m_panPending = false;
    getViewer()->setPanCursor();
}

void QGVNavStyle::pan(QPoint p)
{
    QScrollBar* horizontalScrollbar = getViewer()->horizontalScrollBar();
    QScrollBar* verticalScrollbar = getViewer()->verticalScrollBar();
    QPoint direction = p - panOrigin;

    horizontalScrollbar->setValue(horizontalScrollbar->value() - m_reversePan * direction.x());
    verticalScrollbar->setValue(verticalScrollbar->value() - m_reverseScroll * direction.y());

    panOrigin = p;
}

void QGVNavStyle::stopPan()
{
    //    Base::Console().Message("QGVNS::stopPan()\n");
    panningActive = false;
    m_panPending = false;
    getViewer()->resetCursor();
}

void QGVNavStyle::startClick(Qt::MouseButton b)
{
    m_clickPending = true;
    m_clickButton = b;
}

void QGVNavStyle::stopClick()
{
    m_clickPending = false;
    m_clickButton = Qt::MouseButton::NoButton;
}

void QGVNavStyle::placeBalloon(QPoint p)
{
    //    Base::Console().Message("QGVNS::placeBalloon()\n");
    getViewer()->getBalloonCursor()->hide();
    //balloon was created in Command.cpp.  Why are we doing it again?
    getViewer()->getScene()->createBalloon(getViewer()->mapToScene(p),
                                           getViewer()->getBalloonParent());
    getViewer()->setBalloonPlacing(false);
}

//****************************************
KeyCombination::KeyCombination() {}

KeyCombination::~KeyCombination() {}

void KeyCombination::addKey(int inKey)
{
    bool found = false;
    //check for inKey already in keys
    if (!keys.empty()) {
        for (auto& k : keys) {
            if (k == inKey) {
                found = true;
            }
        }
    }
    if (!found) {
        keys.push_back(inKey);
    }
}

void KeyCombination::removeKey(int inKey)
{
    std::vector<int> newKeys;
    for (auto& k : keys) {
        if (k != inKey) {
            newKeys.push_back(k);
        }
    }
    keys = newKeys;
}

void KeyCombination::clear() { keys.clear(); }

bool KeyCombination::empty() { return keys.empty(); }

//does inCombo match the keys we have in current combination
bool KeyCombination::haveCombination(int inCombo)
{
    bool matched = false;
    int combo = 0;//no key
    if (keys.size() < 2) {
        //not enough keys for a combination
        return false;
    }
    for (auto& k : keys) {
        combo = combo | k;
    }
    if (combo == inCombo) {
        matched = true;
    }
    return matched;
}

}//namespace TechDrawGui
