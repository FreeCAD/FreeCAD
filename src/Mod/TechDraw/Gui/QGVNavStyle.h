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


#ifndef TECHDRAW_NAVIGATIONSTYLE_H
#include <QPoint>
#define TECHDRAW_NAVIGATIONSTYLE_H

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <QCursor>

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
#include <Base/BaseClass.h>

namespace TechDrawGui {

class QGVPage;

//class to support multiple key combinations
class KeyCombination
{
public:
    KeyCombination();
    ~KeyCombination();

    void addKey(int inKey);
    void removeKey(int inKey);
    void clear();
    bool empty();

    bool haveCombination(int inCombo);

private:
    std::vector<int> keys;
};

class TechDrawGuiExport QGVNavStyle : public Base::BaseClass
{
public:
    QGVNavStyle(QGVPage* qgvp);
    ~QGVNavStyle() override;

    void setViewer(QGVPage* qgvp) { m_viewer = qgvp;} ;
    QGVPage* getViewer() { return m_viewer;};

    virtual void handleEnterEvent(QEvent *event);
    virtual void handleFocusOutEvent(QFocusEvent *event);
    virtual void handleKeyPressEvent(QKeyEvent *event);
    virtual void handleKeyReleaseEvent(QKeyEvent *event);
    virtual void handleLeaveEvent(QEvent *event);
    virtual void handleMouseMoveEvent(QMouseEvent *event);
    virtual void handleMousePressEvent(QMouseEvent *event);
    virtual void handleMouseReleaseEvent(QMouseEvent *event);
    virtual void handleWheelEvent(QWheelEvent *event);

    virtual bool allowContextMenu(QContextMenuEvent *event);
    virtual void pseudoContextEvent();

    virtual void startZoom(QPoint p);
    virtual void zoom(double factor);
    virtual void stopZoom();
    virtual double mouseZoomFactor(QPoint p);

    virtual void startPan(QPoint p);
    virtual void pan(QPoint p);
    virtual void stopPan();

    virtual void startClick(Qt::MouseButton b);
    virtual void stopClick();

    virtual void placeBalloon(QPoint p);

protected:
    virtual void initialize();
    virtual void setAnchor();
    QGVPage* m_viewer;
    int m_currentmode;

    bool ctrldown, shiftdown, altdown;
    bool button1down, button2down, button3down;
    bool invertZoom;
    bool zoomAtCursor;
    double zoomStep;
    int m_reversePan;
    int m_reverseScroll;
    QPoint panOrigin;
    bool panningActive;
    QPoint zoomOrigin;
    bool zoomingActive;
    bool m_clickPending;
    bool m_panPending;
    bool m_zoomPending;
    Qt::MouseButton m_clickButton;

    KeyCombination m_keyCombo;
    QCursor m_saveCursor;

private:

};

}
#endif // TECHDRAW_NAVIGATIONSTYLE_H
