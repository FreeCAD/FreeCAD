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
#include <QApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QScrollBar>
#endif

#include <Base/Console.h>
#include <Base/Parameter.h>

#include <App/Application.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGVNavStyle.h"
#include "QGVPage.h"
#include "QGSPage.h"

using namespace TechDrawGui;

namespace TechDrawGui {

QGVNavStyle::QGVNavStyle(QGVPage *qgvp) :
    m_viewer(qgvp)
{
    initialize();
}

QGVNavStyle::~QGVNavStyle()
{
}

void QGVNavStyle::initialize()
{
    this->button1down = false;
    this->button2down = false;
    this->button3down = false;
    this->ctrldown = false;
    this->shiftdown = false;
    this->altdown = false;
    this->invertZoom = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetBool("InvertZoom",true);
    this->zoomAtCursor = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetBool("ZoomAtCursor",true);
    this->zoomStep = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetFloat("ZoomStep",0.2f);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    m_reversePan = hGrp->GetInt("KbPan",1);
    m_reverseScroll = hGrp->GetInt("KbScroll",1);

    panningActive = false;
    zoomingActive = false;
    m_clickPending = false;
    m_panPending = false;
    m_zoomPending = false;
    m_clickButton = Qt::NoButton;
    m_saveCursor = getViewer()->cursor();
}

void QGVNavStyle::setAnchor()
{
    if (m_viewer != nullptr) {
        if (zoomAtCursor) {
            m_viewer->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
            m_viewer->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        } else {
            m_viewer->setResizeAnchor(QGraphicsView::AnchorViewCenter);
            m_viewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        }
    }
}

void QGVNavStyle::handleEnterEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (getViewer()->isBalloonPlacing()) {
        getViewer()->getBalloonCursor()->hide();
    }
}

void QGVNavStyle::handleFocusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    getViewer()->cancelBalloonPlacing();
}

void QGVNavStyle::handleKeyPressEvent(QKeyEvent *event)
{
    if(event->modifiers().testFlag(Qt::ControlModifier)) {
        switch(event->key()) {
            case Qt::Key_Plus: { 
                zoom(1.0 + zoomStep);
                event->accept();
                break;
            }
            case Qt::Key_Minus: {
                zoom(1.0 - zoomStep);
                event->accept();
                break;
            }
            default: {
                break;
            }
        }
    }

    if(event->modifiers().testFlag( Qt::NoModifier)) {
        switch(event->key()) {
            case Qt::Key_Left: {
                getViewer()->kbPanScroll(1, 0);
                event->accept();
                break;
            }
            case Qt::Key_Up: {
                getViewer()->kbPanScroll(0, 1);
                event->accept();
                break;
            }
            case Qt::Key_Right: {
                getViewer()->kbPanScroll(-1, 0);
                event->accept();
                break;
            }
            case Qt::Key_Down: {
                getViewer()->kbPanScroll(0, -1);
                event->accept();
                break;
            }
            case Qt::Key_Escape: {
                getViewer()->cancelBalloonPlacing();
                event->accept();
                break;
            }
            case Qt::Key_Shift: {
                this->shiftdown = true;
                Base::Console().Message("QGVNS::handleKeyPressEvent - shift pressed\n");
                event->accept();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void QGVNavStyle::handleKeyReleaseEvent(QKeyEvent *event)
{
//    Q_UNUSED(event);
    if(event->modifiers().testFlag( Qt::NoModifier)) {
        switch(event->key()) {
            case Qt::Key_Shift: {
                this->shiftdown = false;
                Base::Console().Message("QGVNS::handleKeyPressEvent - shift released\n");
                event->accept();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void QGVNavStyle::handleLeaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (getViewer()->isBalloonPlacing()) {
        int left_x;
        if (getViewer()->getBalloonCursorPos().x() < 32)
            left_x = 0;
        else if (getViewer()->getBalloonCursorPos().x() > (getViewer()->contentsRect().right() - 32))
            left_x = getViewer()->contentsRect().right() - 32;
        else
            left_x = getViewer()->getBalloonCursorPos().x();

        int left_y;
        if (getViewer()->getBalloonCursorPos().y() < 32)
            left_y = 0;
        else if (getViewer()->getBalloonCursorPos().y() > (getViewer()->contentsRect().bottom() - 32))
            left_y = getViewer()->contentsRect().bottom() - 32;
        else
            left_y = getViewer()->getBalloonCursorPos().y();

        /* When cursor leave the page, display getViewer()->balloonCursor where it left */
        getViewer()->getBalloonCursor()->setGeometry(left_x ,left_y, 32, 32);
        getViewer()->getBalloonCursor()->show();
    }
}

void QGVNavStyle::handleMousePressEvent(QMouseEvent *event)
{
//    Base::Console().Message("QGVNS::handleMousePressEvent()\n");
    if (!panningActive && (event->button() == Qt::MiddleButton)) {
        startPan(event->pos());
        event->accept();
    }
}

void QGVNavStyle::handleMouseMoveEvent(QMouseEvent *event)
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
void QGVNavStyle::handleMouseReleaseEvent(QMouseEvent *event)
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

bool QGVNavStyle::allowContextMenu(QContextMenuEvent *event)
{
    Q_UNUSED(event)
//    Base::Console().Message("QGVNS::allowContextMenu()\n");
//    if (event->reason() == QContextMenuEvent::Mouse) {
//        //must check for a button combination involving context menu button
//    }
    return true;
}

void QGVNavStyle::pseudoContextEvent()
{
    getViewer()->pseudoContextEvent();
}

void QGVNavStyle::handleWheelEvent(QWheelEvent *event)
{
//Delta is the distance that the wheel is rotated, in eighths of a degree.
//positive indicates rotation forwards away from the user; negative backwards toward the user.
//Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
//1 click = 15 degrees.  15 degrees = 120 deltas.  delta/240 -> 1 click = 0.5 ==> factor = 1.2^0.5 = 1.095
//                                                              1 click = -0.5 ==> factor = 1.2^-0.5 = 0.91
//so to change wheel direction, multiply (event->delta() / 240.0) by +/-1
    double mouseBase = 1.2;        //magic numbers. change for different mice?
    double mouseAdjust = -240.0;
    if (invertZoom) {
        mouseAdjust = -mouseAdjust;
    }

    int delta = event->angleDelta().y();
    qreal factor = std::pow(mouseBase, delta / mouseAdjust);
    zoom(factor);
}

void QGVNavStyle::zoom(double factor)
{
    QPoint center = getViewer()->viewport()->rect().center();
    getViewer()->scale(factor,
                       factor);

    QPoint newCenter = getViewer()->viewport()->rect().center();
    QPoint change = newCenter - center;
    getViewer()->translate(change.x(), change.y());
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
//    Base::Console().Message("QGVNS::mouseZoomFactor(%s)\n", TechDraw::DrawUtil::formatVector(p).c_str());
    QPoint movement = p - zoomOrigin;
    double sensitivity = 0.1;
    double direction = 1.0;
    double invert = 1.0;
    if (movement.y() < 0.0) {
        direction = -direction;
    }
    if (invertZoom) {
        invert = -invert;
    }
    double factor = 1.0 + (direction * invert * zoomStep * sensitivity);
    zoomOrigin = p;
    return factor;
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
    QScrollBar *horizontalScrollbar = getViewer()->horizontalScrollBar();
    QScrollBar *verticalScrollbar = getViewer()->verticalScrollBar();
    QPoint direction = p - panOrigin;

    horizontalScrollbar->setValue(horizontalScrollbar->value() - m_reversePan*direction.x());
    verticalScrollbar->setValue(verticalScrollbar->value() - m_reverseScroll*direction.y());

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

void QGVNavStyle::stopClick(void)
{
    m_clickPending = false;
    m_clickButton = Qt::MouseButton::NoButton;
}

void QGVNavStyle::placeBalloon(QPoint p)
{
    getViewer()->getBalloonCursor()->hide();
    getViewer()->getScene()->createBalloon(getViewer()->mapToScene(p),
                               getViewer()->getDrawPage()->balloonParent);
    getViewer()->setBalloonPlacing(false);
}

//****************************************
KeyCombination::KeyCombination()
{
}

KeyCombination::~KeyCombination()
{
}

void KeyCombination::addKey(int inKey)
{
    bool found = false;
    //check for inKey already in keys
    if (!keys.empty()) {
        for (auto& k: keys) {
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
    for (auto& k: keys) {
        if (k != inKey) {
            newKeys.push_back(k);
        }
    }
    keys = newKeys;
}

void KeyCombination::clear()
{
    keys.clear();
}

bool KeyCombination::empty()
{
    return keys.empty();
}

//does inCombo match the keys we have in current combination
bool KeyCombination::haveCombination(int inCombo)
{
    bool matched = false;
    int combo = 0;      //no key
    if (keys.size() < 2) {
        //not enough keys for a combination
        return false;
    }
    for (auto& k: keys) {
        combo = combo | k;
    }
    if (combo == inCombo) {
        matched = true;
    }
    return matched;
}

}  //namespace TechDrawGui
