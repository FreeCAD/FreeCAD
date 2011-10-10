/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <qapplication.h>
# include <qevent.h>
# include <qpainter.h>
# include <qpixmap.h>
# include <QMenu>
# include <Inventor/SbBox.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Console.h>

#include "MouseSelection.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui; 

AbstractMouseSelection::AbstractMouseSelection() : _pcView3D(0)
{
    m_bInner = true;
    mustRedraw = false;
}

void AbstractMouseSelection::grabMouseModel( Gui::View3DInventorViewer* viewer )
{
    _pcView3D=viewer;
    m_cPrevCursor = _pcView3D->getWidget()->cursor();

    // do initialization of your mousemodel
    initialize();
}

void AbstractMouseSelection::releaseMouseModel()
{
    // do termination of your mousemodel
    terminate();

    _pcView3D->getWidget()->setCursor(m_cPrevCursor);
    _pcView3D = 0;
}

void AbstractMouseSelection::redraw()
{
    // Note: For any reason it does not work to do a redraw in the actualRedraw() method of the
    // viewer class. So, we do the redraw when the user continues moving the cursor. E.g. have
    // a look to PolyPickerSelection::draw()
    mustRedraw = true;
}

int AbstractMouseSelection::handleEvent(const SoEvent * const ev, const SbViewportRegion& vp)
{
    int ret=Continue;

    const SbVec2s& sz = vp.getWindowSize(); 
    short w,h; sz.getValue(w,h);

    SbVec2s loc = ev->getPosition();
    short x,y; loc.getValue(x,y);
    y = h-y; // the origin is at the left bottom corner (instead of left top corner)

    if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

        if (press) {
            _clPoly.push_back(ev->getPosition());
            ret = mouseButtonEvent(static_cast<const SoMouseButtonEvent*>(ev), QPoint(x,y));
        }
        else {
            ret = mouseButtonEvent(static_cast<const SoMouseButtonEvent*>(ev), QPoint(x,y));
        }
    }
    else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        ret = locationEvent(static_cast<const SoLocation2Event*>(ev), QPoint(x,y));
    }
    else if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        ret = keyboardEvent(static_cast<const SoKeyboardEvent*>(ev));
    }

    if (ret == Restart)
        _clPoly.clear();

    return ret;
}

// -----------------------------------------------------------------------------------

BaseMouseSelection::BaseMouseSelection()
  : AbstractMouseSelection()
{
}

// -----------------------------------------------------------------------------------
#if 0
/* XPM */
static const char *cursor_polypick[]={
"32 32 2 1",
"# c #646464",
". c None",
"................................",
"................................",
".......#........................",
".......#........................",
".......#........................",
"................................",
".......#........................",
"..###.###.###...................",
".......#...............#........",
"......................##........",
".......#..............#.#.......",
".......#.............#..#.......",
".......#............#...#.......",
"....................#....#......",
"...................#.....#......",
"..................#......#......",
"............#.....#.......#.....",
"...........#.##..#........#.....",
"..........#....##.........#.....",
".........#...............#......",
"........#................#......",
".......#................#.......",
"......#.................#.......",
".....#.................#........",
"....#####..............#........",
".........#########....#.........",
"..................#####.........",
"................................",
"................................",
"................................",
"................................",
"................................"};

/* XPM */
static const char *cursor_scissors[]={
"32 32 3 1",
"# c #000000",
"+ c #ffffff",
". c None",
"....+...........................",
"....+...........................",
"....+...........................",
"................................",
"+++.+.+++.......................",
"................................",
"....+...........................",
"....+...................#####...",
"....+.................########..",
".....................#########..",
".....###............##########..",
"....##++##.........#####...###..",
"...#++++++##.......####...####..",
"...##+++++++#......####.######..",
".....#+++++++##....##########...",
"......##+++++++##.##########....",
"........##+++++++#########......",
"..........#+++++++#####.........",
"...........##+++++####..........",
"...........##+++++###...........",
".........##+++++++########......",
"........##+++++++###########....",
"......##+++++++##.###########...",
"....##+++++++##....##########...",
"...#+++++++##......####..#####..",
"...#++++++#........#####..####..",
"....##++##..........#####..###..",
"......#.............##########..",
".....................#########..",
".......................######...",
"................................",
"................................"};
#endif
static const char *cursor_cut_scissors[]={
"32 32 6 1",
"a c #800000",
"c c #808080",
"+ c #c0c0c0",
"b c #ff0000",
"# c #ffffff",
". c None",
"....#...........................",
"....#...........................",
"....#...........................",
"................................",
"###.#.###.......................",
"................................",
"....#...........................",
"....#...................aaaaa...",
"....#.................aabbbbba..",
".....................abbbbbbba..",
".....ccc............abbaaaaabb..",
"....cc++cc.........babaa...aba..",
"...c+#++++cc.......abba...abba..",
"...cc+#+++++c......abba.aabbaa..",
".....c+++++#+cc....abbaaabbaa...",
"......cc+#+++#+cc.aabbbbbbaa....",
"........cc+#+++#+cabbbaaaa......",
"..........c+++++++abbaa.........",
"...........cc+++#+aaaa..........",
"...........cc+#+++caa...........",
".........cc+++++#+cbbaaaaa......",
"........cc+#+++#+cabbabbbaaa....",
"......cc+#+++#+cc.aaabbbbbbaa...",
"....cc+#+++#+cc....abbaaaabba...",
"...c++#++#+cc......abba..aabba..",
"...c+###++c........aabaa..aaba..",
"....cc++cc..........abbaa..aba..",
"......c.............aabbaaaaba..",
".....................baabbbbba..",
".......................aaaaaa...",
"................................",
"................................"};

PolyPickerSelection::PolyPickerSelection() 
{
    m_iRadius    = 2;
    m_iNodes     = 0;
    m_bWorking   = false;
}

void PolyPickerSelection::initialize()
{
    QPixmap p(cursor_cut_scissors);
    QCursor cursor(p, 4, 4);
    _pcView3D->getWidget()->setCursor(cursor);
}

void PolyPickerSelection::terminate()
{
//  _pcView3D->getGLWidget()->releaseMouse();
}

void PolyPickerSelection::draw ()
{
    if (mustRedraw){
        if (_cNodeVector.size() > 1) {
            QPoint start = _cNodeVector.front();
            for (std::vector<QPoint>::iterator it = _cNodeVector.begin()+1; it != _cNodeVector.end(); ++it) {
                _pcView3D->drawLine(start.x(),start.y(),it->x(), it->y() );
                start = *it;
            }
        }

        // recursive call, but no infinite loop
        mustRedraw = false;
        draw();
    }
    if (m_bWorking) {
        if (m_iNodes < int(_cNodeVector.size())) {
            m_iNodes = int(_cNodeVector.size());

            if (_cNodeVector.size() > 2) {
                QPoint start = _cNodeVector.front();
                _pcView3D->drawLine(m_iXnew,m_iYnew,start.x(), start.y() );
            }
        }
        else {
            _pcView3D->drawLine(m_iXnew,m_iYnew,m_iXold,m_iYold);
            if (_cNodeVector.size() > 1) {
                QPoint start = _cNodeVector.front();
                _pcView3D->drawLine(m_iXnew,m_iYnew,start.x(), start.y());
            }
        }
    }
}

PolyPickerSelection::~PolyPickerSelection()
{
}

int PolyPickerSelection::popupMenu()
{
    QMenu menu;
    QAction* fi = menu.addAction(QObject::tr("Finish"));
    menu.addAction(QObject::tr("Clear"));
    QAction* ca = menu.addAction(QObject::tr("Cancel"));
    if (getPositions().size() < 3)
        fi->setEnabled(false);
    QAction* id = menu.exec(QCursor::pos());
    if (id == fi)
        return Finish;
    else if (id == ca)
        return Cancel;
    else
        return Restart;
}

int PolyPickerSelection::mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos )
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

    if (press) {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON1:
            {
                // start working from now on
                if (!m_bWorking) {
                    m_bWorking = true;
                    // clear the old polygon
                    _cNodeVector.clear();
                    _pcView3D->getGLWidget()->update();
//                  _pcView3D->getGLWidget()->grabMouse();
                }

                _cNodeVector.push_back(pos);

                m_iXnew = pos.x();  m_iYnew = pos.y();
                m_iXold = pos.x();  m_iYold = pos.y();
            }   break;
        case SoMouseButtonEvent::BUTTON2:
            {
                if (_cNodeVector.size() > 0) {
                    if (_cNodeVector.back() != pos)
                        _cNodeVector.push_back(pos);
                    m_iXnew = pos.x();  m_iYnew = pos.y();
                    m_iXold = pos.x();  m_iYold = pos.y();
                }

                QCursor cur = _pcView3D->getWidget()->cursor();
                _pcView3D->getWidget()->setCursor(m_cPrevCursor);
//              _pcView3D->getGLWidget()->releaseMouse();

                int id = popupMenu();
                if (id == Finish || id == Cancel) {
                    releaseMouseModel();
                }
                else if (id == Restart) {
                    m_bWorking = false;
                    m_iNodes = 0;
                    _pcView3D->getWidget()->setCursor(cur);
                }
                return id;
            }   break;
        default:
            {
            }   break;
        }
    }

    return Continue;
}

int PolyPickerSelection::locationEvent( const SoLocation2Event * const e, const QPoint& pos )
{
    // do all the drawing stuff for us
    QPoint clPoint = pos;

    if (m_bWorking) {
        // check the position
        QRect r = _pcView3D->getGLWidget()->rect();
        if (!r.contains(clPoint)) {
            if (clPoint.x() < r.left())
                clPoint.setX( r.left());
            if (clPoint.x() > r.right())
                clPoint.setX(r.right());
            if (clPoint.y() < r.top())
                clPoint.setY(r.top());
            if (clPoint.y() > r.bottom())
                clPoint.setY(r.bottom());

#ifdef FC_OS_WINDOWS
            QPoint newPos = _pcView3D->getGLWidget()->mapToGlobal(clPoint);
            QCursor::setPos(newPos);
#endif
        }
    }

    draw();
    m_iXnew = clPoint.x();
    m_iYnew = clPoint.y();
    draw();

    return Continue;
}

int PolyPickerSelection::keyboardEvent( const SoKeyboardEvent * const e )
{
    return Continue;
}

// -----------------------------------------------------------------------------------

PolyClipSelection::PolyClipSelection() 
{
}

PolyClipSelection::~PolyClipSelection()
{
}

int PolyClipSelection::popupMenu()
{
    QMenu menu;
    QAction* ci = menu.addAction(QObject::tr("Inner"));
    QAction* co = menu.addAction(QObject::tr("Outer"));
    QAction* ca = menu.addAction(QObject::tr("Cancel"));
    if (getPositions().size() < 3) {
        ci->setEnabled(false);
        co->setEnabled(false);
    }
    QAction* id = menu.exec(QCursor::pos());
    if (id == ci) {
        m_bInner = true;
        return Finish;
    }
    else if (id == co) {
        m_bInner = false;
        return Finish;
    }
    else if (id == ca)
        return Cancel;
    else
        return Restart;
}

// -----------------------------------------------------------------------------------

RectangleSelection::RectangleSelection()
{
    m_bWorking = false;
}

RectangleSelection::~RectangleSelection()
{
}

void RectangleSelection::initialize()
{
}

void RectangleSelection::terminate()
{
}

void RectangleSelection::draw ()
{
    if (m_bWorking)
        _pcView3D->drawRect(m_iXold, m_iYold, m_iXnew, m_iYnew);
}

int RectangleSelection::mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos )
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

    int ret = Continue;

    if (press) {
        switch ( button )
        {
        case SoMouseButtonEvent::BUTTON1:
            {
                m_bWorking = true;
                m_iXold = m_iXnew = pos.x(); 
                m_iYold = m_iYnew = pos.y();
            }   break;
        default:
            {
            }   break;
        }
    }
    else {
        switch (button) {
            case SoMouseButtonEvent::BUTTON1:
                {
                    releaseMouseModel();
                    m_bWorking = false;
                    ret = Finish;
                }   break;
            default:
                {
                }   break;
        }
    }

    return ret;
}

int RectangleSelection::locationEvent( const SoLocation2Event * const e, const QPoint& pos )
{
    draw();
    m_iXnew = pos.x(); 
    m_iYnew = pos.y();
    draw();
    return Continue;
}

int RectangleSelection::keyboardEvent( const SoKeyboardEvent * const e )
{
    return Continue;
}

// -----------------------------------------------------------------------------------

BoxZoomSelection::BoxZoomSelection()
{
}

BoxZoomSelection::~BoxZoomSelection()
{
}

void BoxZoomSelection::terminate()
{
    int xmin = std::min<int>(m_iXold, m_iXnew);
    int xmax = std::max<int>(m_iXold, m_iXnew);
    int ymin = std::min<int>(m_iYold, m_iYnew);
    int ymax = std::max<int>(m_iYold, m_iYnew);
    SbBox2s box(xmin, ymin, xmax, ymax);
    _pcView3D->boxZoom(box);
}
