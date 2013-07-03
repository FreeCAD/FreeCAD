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
#include "GLPainter.h"

using namespace Gui; 

AbstractMouseSelection::AbstractMouseSelection() : _pcView3D(0)
{
    m_bInner = true;
    mustRedraw = false;
}

void AbstractMouseSelection::grabMouseModel( Gui::View3DInventorViewer* viewer )
{
    _pcView3D = viewer;
    m_cPrevCursor = _pcView3D->getWidget()->cursor();

    // do initialization of your mousemodel
    initialize();
}

void AbstractMouseSelection::releaseMouseModel()
{
    if (_pcView3D) {
        // do termination of your mousemodel
        terminate();

        _pcView3D->getWidget()->setCursor(m_cPrevCursor);
        _pcView3D = 0;
    }
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
            GLPainter p;
            p.begin(_pcView3D);
            p.setColor(1.0f,1.0f,1.0f);
            p.setLogicOp(GL_XOR);
            for (std::vector<QPoint>::iterator it = _cNodeVector.begin()+1; it != _cNodeVector.end(); ++it) {
                p.drawLine(start.x(),start.y(),it->x(), it->y());
                start = *it;
            }
            p.end();
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
                GLPainter p;
                p.begin(_pcView3D);
                p.setColor(1.0f,1.0f,1.0f);
                p.setLogicOp(GL_XOR);
                p.drawLine(m_iXnew,m_iYnew,start.x(), start.y());
                p.end();
            }
        }
        else {
            GLPainter p;
            p.begin(_pcView3D);
            p.setColor(1.0f,1.0f,1.0f);
            p.setLogicOp(GL_XOR);
            p.drawLine(m_iXnew,m_iYnew,m_iXold,m_iYold);
            if (_cNodeVector.size() > 1) {
                QPoint start = _cNodeVector.front();
                p.drawLine(m_iXnew,m_iYnew,start.x(), start.y());
            }
            p.end();
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
            }   break;
        default:
            {
            }   break;
        }
    }
    // release
    else {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON2:
            {
                QCursor cur = _pcView3D->getWidget()->cursor();
                _pcView3D->getWidget()->setCursor(m_cPrevCursor);

                // The pop-up menu should be shown when releasing mouse button because
                // otherwise the navigation style doesn't get the UP event and gets into
                // an inconsistent state.
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

BrushSelection::BrushSelection()
  : r(1.0f), g(0.0f), b(0.0f), a(0.0f), l(2.0f)
{
    m_iNodes     = 0;
    m_bWorking   = false;
}

void BrushSelection::initialize()
{
    QPixmap p(cursor_cut_scissors);
    QCursor cursor(p, 4, 4);
    _pcView3D->getWidget()->setCursor(cursor);
}

void BrushSelection::terminate()
{
}

void BrushSelection::setColor(float r, float g, float b, float a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

void BrushSelection::setLineWidth(float l)
{
    this->l = l;
}

void BrushSelection::draw ()
{
    if (mustRedraw){
        if (_cNodeVector.size() > 1) {
            QPoint start = _cNodeVector.front();
            GLPainter p;
            p.begin(_pcView3D);
            p.setLineWidth(this->l);
            p.setColor(this->r, this->g, this->b, this->a);
            for (std::vector<QPoint>::iterator it = _cNodeVector.begin()+1; it != _cNodeVector.end(); ++it) {
                p.drawLine(start.x(),start.y(),it->x(), it->y());
                start = *it;
            }
            p.end();
        }

        // recursive call, but no infinite loop
        mustRedraw = false;
        draw();
    }
    if (m_bWorking) {
        GLPainter p;
        p.begin(_pcView3D);
        p.setLineWidth(this->l);
        p.setColor(this->r, this->g, this->b, this->a);
        p.drawLine(m_iXnew, m_iYnew, m_iXold, m_iYold);
        p.end();
    }
}

BrushSelection::~BrushSelection()
{
}

int BrushSelection::popupMenu()
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

int BrushSelection::mouseButtonEvent(const SoMouseButtonEvent * const e, const QPoint& pos)
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

                    _cNodeVector.push_back(pos);

                    m_iXnew = pos.x();  m_iYnew = pos.y();
                    m_iXold = pos.x();  m_iYold = pos.y();
                }
            }   break;
        case SoMouseButtonEvent::BUTTON2:
            {
                if (_cNodeVector.size() > 0) {
                    if (_cNodeVector.back() != pos)
                        _cNodeVector.push_back(pos);
                    m_iXnew = pos.x();  m_iYnew = pos.y();
                    m_iXold = pos.x();  m_iYold = pos.y();
                }
            }   break;
        default:
            {
            }   break;
        }
    }
    // release
    else {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON1:
            return Finish;
        case SoMouseButtonEvent::BUTTON2:
            {
                QCursor cur = _pcView3D->getWidget()->cursor();
                _pcView3D->getWidget()->setCursor(m_cPrevCursor);

                // The pop-up menu should be shown when releasing mouse button because
                // otherwise the navigation style doesn't get the UP event and gets into
                // an inconsistent state.
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

int BrushSelection::locationEvent(const SoLocation2Event * const e, const QPoint& pos)
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
        }

        SbVec2s last = _clPoly.back();
        SbVec2s curr = e->getPosition();
        if (abs(last[0]-curr[0]) > 20 || abs(last[1]-curr[1]) > 20)
            _clPoly.push_back(curr);
        _cNodeVector.push_back(clPoint);
    }

    m_iXnew = clPoint.x();
    m_iYnew = clPoint.y();
    draw();
    m_iXold = clPoint.x();
    m_iYold = clPoint.y();

    return Continue;
}

int BrushSelection::keyboardEvent( const SoKeyboardEvent * const e )
{
    return Continue;
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
    if (m_bWorking) {
        GLPainter p;
        p.begin(_pcView3D);
        p.setColor(1.0, 1.0, 0.0, 0.0);
        p.setLogicOp(GL_XOR);
        p.setLineWidth(3.0f);
        p.setLineStipple(2, 0x3F3F);
        p.drawRect(m_iXold, m_iYold, m_iXnew, m_iYnew);
        p.end();
    }
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
                    _clPoly.push_back(e->getPosition());
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

class RubberbandSelection::Private : public Gui::GLGraphicsItem
{
    Gui::View3DInventorViewer* viewer;
    int x_old, y_old, x_new, y_new;
    bool working;
public:
    Private(Gui::View3DInventorViewer* v) : viewer(v)
    {
        x_old = y_old = x_new = y_new = 0;
        working = false;
    }
    ~Private()
    {
    }
    void setWorking(bool on)
    {
        working = on;
    }
    void setCoords(int x1, int y1, int x2, int y2)
    {
        x_old = x1;
        y_old = y1;
        x_new = x2;
        y_new = y2;
    }
    void paintGL()
    {
        if (!working)
            return;
        const SbViewportRegion vp = viewer->getViewportRegion();
        SbVec2s size = vp.getViewportSizePixels();

        glMatrixMode(GL_PROJECTION);
        glOrtho(0, size[0], size[1], 0, 0, 100);
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(4.0);
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glRecti(x_old, y_old, x_new, y_new);
        glColor4f(1.0, 1.0, 0.0, 0.5);
        glLineStipple(3, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_LOOP);
            glVertex2i(x_old, y_old);
            glVertex2i(x_new, y_old);
            glVertex2i(x_new, y_new);
            glVertex2i(x_old, y_new);
        glEnd();

        glLineWidth(1.0);
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_BLEND);
    }
};

RubberbandSelection::RubberbandSelection()
{
    d = 0;
}

RubberbandSelection::~RubberbandSelection()
{
}

void RubberbandSelection::initialize()
{
    d = new Private(_pcView3D);
    _pcView3D->addGraphicsItem(d);
    _pcView3D->setRenderFramebuffer(true);
    _pcView3D->scheduleRedraw();
}

void RubberbandSelection::terminate()
{
    _pcView3D->removeGraphicsItem(d);
    delete d; d = 0;
    _pcView3D->setRenderFramebuffer(false);
    _pcView3D->scheduleRedraw();
}

void RubberbandSelection::draw ()
{
}

int RubberbandSelection::mouseButtonEvent(const SoMouseButtonEvent * const e, const QPoint& pos)
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

    int ret = Continue;

    if (press) {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON1:
            {
                d->setWorking(true);
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
                    d->setWorking(false);
                    releaseMouseModel();
                    _clPoly.push_back(e->getPosition());
                    ret = Finish;
                }   break;
            default:
                {
                }   break;
        }
    }

    return ret;
}

int RubberbandSelection::locationEvent(const SoLocation2Event * const e, const QPoint& pos)
{
    m_iXnew = pos.x(); 
    m_iYnew = pos.y();
    d->setCoords(m_iXold, m_iYold, m_iXnew, m_iYnew);
    _pcView3D->render();
    return Continue;
}

int RubberbandSelection::keyboardEvent(const SoKeyboardEvent * const e)
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
    RubberbandSelection::terminate();

    int xmin = std::min<int>(m_iXold, m_iXnew);
    int xmax = std::max<int>(m_iXold, m_iXnew);
    int ymin = std::min<int>(m_iYold, m_iYnew);
    int ymax = std::max<int>(m_iYold, m_iYnew);
    SbBox2s box(xmin, ymin, xmax, ymax);
    _pcView3D->boxZoom(box);
}
