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
# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <QApplication>
# include <QMenu>
#endif

#include "SoFCColorBar.h"
#include "SoFCColorGradient.h"
#include "SoFCColorLegend.h"

using namespace Gui;

SO_NODE_ABSTRACT_SOURCE(SoFCColorBarBase)

/*!
  Constructor.
*/
SoFCColorBarBase::SoFCColorBarBase()
{
    SO_NODE_CONSTRUCTOR(SoFCColorBarBase);
}

/*!
  Destructor.
*/
SoFCColorBarBase::~SoFCColorBarBase()
{
    //delete THIS;
}

// doc from parent
void SoFCColorBarBase::initClass(void)
{
    SO_NODE_INIT_ABSTRACT_CLASS(SoFCColorBarBase,SoSeparator,"Separator");
}

void SoFCColorBarBase::finish()
{
    atexit_cleanup();
}

void SoFCColorBarBase::GLRenderBelowPath(SoGLRenderAction *  action)
{
    const SbViewportRegion& vp = action->getViewportRegion();
    const SbVec2s&  size = vp.getWindowSize();
    if (_windowSize != size) {
        _windowSize = size;
        setViewportSize(size);
    }
    SoSeparator::GLRenderBelowPath(action);
}

// --------------------------------------------------------------------------

namespace Gui {
// Proxy class that receives an asynchronous custom event
class SoFCColorBarProxyObject : public QObject
{
public:
    SoFCColorBarProxyObject(SoFCColorBar* b)
        : QObject(0), bar(b) {}
    ~SoFCColorBarProxyObject() {}
    void customEvent(QEvent *)
    {
        if (bar->customize())
            bar->Notify(0);
        this->deleteLater();
    }

private:
    SoFCColorBar* bar;
};
}

SO_NODE_SOURCE(SoFCColorBar)

/*!
  Constructor.
*/
SoFCColorBar::SoFCColorBar()
{
    SO_NODE_CONSTRUCTOR(SoFCColorBar);

    _fMaxX = 0;
    _fMinX = 0;
    _fMaxY = 0;
    _fMinY = 0;

//  SoEventCallback * cb = new SoEventCallback;
//  cb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), eventCallback, this);
//  insertChild(cb, 0);

    pColorMode = new SoSwitch;
    addChild(pColorMode);

    _colorBars.push_back( new SoFCColorGradient );
    _colorBars.push_back( new SoFCColorLegend );

    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        pColorMode->addChild( *it );
    pColorMode->whichChild = 0;
}

/*!
  Destructor.
*/
SoFCColorBar::~SoFCColorBar()
{
    //delete THIS;
}

// doc from parent
void SoFCColorBar::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCColorBar,SoFCColorBarBase,"Separator");
}

void SoFCColorBar::finish()
{
    atexit_cleanup();
}

SoFCColorBarBase* SoFCColorBar::getActiveBar() const
{
    int child = pColorMode->whichChild.getValue();
    return _colorBars[child];
}

void SoFCColorBar::setViewportSize( const SbVec2s& size )
{
    // don't know why the parameter range isn't between [-1,+1]
    float fRatio = ((float)size[0])/((float)size[1]);
    _fMinX=  4.0f, _fMaxX=4.5f;
    _fMinY= -4.0f, _fMaxY=4.0f;
    if (fRatio > 1.0f) {
        _fMinX = 4.0f * fRatio;
        _fMaxX = _fMinX+0.5f;
    }
    else if (fRatio < 1.0f) {
        _fMinY =  -4.0f / fRatio;
        _fMaxY =   4.0f / fRatio;
    }
}

void SoFCColorBar::setRange( float fMin, float fMax, int prec )
{
    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        (*it)->setRange(fMin, fMax, prec);
}

void SoFCColorBar::setOutsideGrayed (bool bVal)
{
    for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it)
        (*it)->setOutsideGrayed(bVal);
}

bool SoFCColorBar::isVisible (float fVal) const
{
    return this->getActiveBar()->isVisible(fVal);
}

float SoFCColorBar::getMinValue (void) const
{
    return this->getActiveBar()->getMinValue();
}

float SoFCColorBar::getMaxValue (void) const
{
    return this->getActiveBar()->getMaxValue();
}

bool SoFCColorBar::customize()
{
    return this->getActiveBar()->customize();
}

App::Color SoFCColorBar::getColor( float fVal ) const
{
    return this->getActiveBar()->getColor( fVal );
}

void SoFCColorBar::eventCallback(void * /*userdata*/, SoEventCallback * node)
{
    const SoEvent * event = node->getEvent();
    if (event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent*  e = static_cast<const SoMouseButtonEvent*>(event);
        if ((e->getButton() == SoMouseButtonEvent::BUTTON2)) {
            if (e->getState() == SoButtonEvent::UP) {
                // do nothing here
            }
        }
    }
}

void SoFCColorBar::handleEvent (SoHandleEventAction *action)
{
    const SoEvent * event = action->getEvent();

    // check for mouse button events
    if (event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent*  e = static_cast<const SoMouseButtonEvent*>(event);

        // calculate the mouse position relative to the colorbar
        //
        const SbViewportRegion&  vp = action->getViewportRegion();
        float fRatio = vp.getViewportAspectRatio();
        SbVec2f pos = event->getNormalizedPosition(vp);
        float pX,pY; pos.getValue(pX,pY);

        pX = pX*10.0f-5.0f;
        pY = pY*10.0f-5.0f;

        // now calculate the real points respecting aspect ratio information
        //
        if (fRatio > 1.0f) {
            pX = pX * fRatio;
        }
        else if (fRatio < 1.0f) {
            pY = pY / fRatio;
        }

        // check if the cursor is near to the color bar
        if (_fMinX > pX || pX > _fMaxX || _fMinY > pY || pY > _fMaxY)
            return; // not inside the rectangle

        // left mouse pressed
        action->setHandled();
        if ((e->getButton() == SoMouseButtonEvent::BUTTON1)) {
            if (e->getState() == SoButtonEvent::DOWN) {
                // double click event
                if (!_timer.isValid()) {
                    _timer.start();
                }
                else if (_timer.restart() < QApplication::doubleClickInterval()) {
                    QApplication::postEvent(
                        new SoFCColorBarProxyObject(this),
                        new QEvent(QEvent::User));
                }
            }
        }
        // right mouse pressed
        else if ((e->getButton() == SoMouseButtonEvent::BUTTON2)) {
            if (e->getState() == SoButtonEvent::UP) {
                SoFCColorBarBase* current = getActiveBar();
                QMenu menu;
                int i=0;
                for (std::vector<SoFCColorBarBase*>::const_iterator it = _colorBars.begin(); it != _colorBars.end(); ++it) {
                    QAction* item = menu.addAction(QLatin1String((*it)->getColorBarName()));
                    item->setCheckable(true);
                    item->setChecked((*it) == current);
                    item->setData(QVariant(i++));
                }

                menu.addSeparator();
                QAction* option = menu.addAction(QObject::tr("Options..."));
                QAction* select = menu.exec(QCursor::pos());

                if (select == option) {
                    QApplication::postEvent(
                        new SoFCColorBarProxyObject(this),
                        new QEvent(QEvent::User));
                }
                else if (select) {
                    int id = select->data().toInt();
                    pColorMode->whichChild = id;
                }
            }
        }
    }
}
