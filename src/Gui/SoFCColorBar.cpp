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
# include <boost/core/ignore_unused.hpp>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoSwitch.h>
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
SoFCColorBarBase::SoFCColorBarBase() : _windowSize(0,0)
{
    SO_NODE_CONSTRUCTOR(SoFCColorBarBase);
}

/*!
  Destructor.
*/
SoFCColorBarBase::~SoFCColorBarBase() = default;

// doc from parent
void SoFCColorBarBase::initClass()
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

void SoFCColorBarBase::setModified()
{
    _boxWidth = -1.0f;
}

float SoFCColorBarBase::getBoundingWidth(const SbVec2s& size)
{
    float fRatio = static_cast<float>(size[0]) / static_cast<float>(size[1]);
    if (fRatio >= 1.0f && _boxWidth >= 0.0f) {
        return _boxWidth;
    }

    // These are the same camera settings for front nodes as defined in the 3d view
    auto cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 5); // the 5 is just a value > 0
    cam->height = 10; // sets the coordinate range of the screen to [-5, +5]
    cam->nearDistance = 0;
    cam->farDistance = 10;

    auto group = new SoGroup();
    group->ref();
    group->addChild(cam);
    group->addChild(this);

    SbViewportRegion vpr(size);
    SoGetBoundingBoxAction bbact(vpr);
    bbact.apply(group);
    SbBox3f box = bbact.getBoundingBox();
    SbVec3f minPt, maxPt;
    box.getBounds(minPt, maxPt);
    group->unref();

    float boxWidth = maxPt[0] - minPt[0];
    _boxWidth = boxWidth;
    return boxWidth;
}

float SoFCColorBarBase::getBounds(const SbVec2s& size, float& fMinX, float&fMinY, float& fMaxX, float& fMaxY)
{
    // ratio of window width / height
    float fRatio = static_cast<float>(size[0]) / static_cast<float>(size[1]);

    // The cam height is set in SoFCColorBarBase::getBoundingWidth to 10.
    // Therefore the normalized coordinates are in the range [-5, +5] x [-5ratio, +5ratio] if ratio > 1
    //  and [-5ratio, +5ratio] x [-5, +5] if ratio < 1.
    // We don't want the whole height covered by the color bar (to have e.g space to the axis cross
    // and the Navigation Cube) thus we take as base 3 or if the height reduces significantly it is 2.5.

    float baseYValue;
    if (fRatio > 3.0f) {
        baseYValue = 2.5f;
    }
    else {
        baseYValue = 3.0f;
    }
    float barWidth = 0.5f;

    // we want the color bar at the rightmost position, therefore we take 4.95 as base
    fMinX = 4.95f * fRatio; // must be scaled with the ratio to assure it stays at the right

    fMaxX = fMinX + barWidth;
    fMinY = -baseYValue - 0.6f; // Extend shortened bar towards axis cross
    fMaxY = baseYValue; // bar has the height of almost whole window height

    if (fRatio < 1.0f) {
        // must be adjusted to assure that the size of the bar doesn't shrink
        fMinX /= fRatio;
        fMaxX /= fRatio;
        fMinY = -baseYValue / fRatio;
        fMaxY = baseYValue / fRatio;
    }

    // get the bounding box width of the color bar and labels
    float boxWidth = getBoundingWidth(size);
    return boxWidth;
}

// --------------------------------------------------------------------------

namespace Gui {
// Proxy class that receives an asynchronous custom event
class SoFCColorBarProxyObject : public QObject
{
public:
    explicit SoFCColorBarProxyObject(SoFCColorBar* b)
        : QObject(nullptr), bar(b) {}
    ~SoFCColorBarProxyObject() override = default;
    void customEvent(QEvent *) override
    {
        bar->customize(bar->getActiveBar());
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

//  SoEventCallback * cb = new SoEventCallback;
//  cb->addEventCallback(SoMouseButtonEvent::getClassTypeId(), eventCallback, this);
//  insertChild(cb, 0);

    pColorMode = new SoSwitch;
    addChild(pColorMode);

    _colorBars.push_back( new SoFCColorGradient );
    _colorBars.push_back( new SoFCColorLegend );

    for (auto it : _colorBars)
        pColorMode->addChild(it);
    pColorMode->whichChild = 0;
}

/*!
  Destructor.
*/
SoFCColorBar::~SoFCColorBar() = default;

// doc from parent
void SoFCColorBar::initClass()
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
    boost::ignore_unused(size);
}

void SoFCColorBar::setRange( float fMin, float fMax, int prec )
{
    for (auto it : _colorBars)
        it->setRange(fMin, fMax, prec);
}

void SoFCColorBar::setOutsideGrayed (bool bVal)
{
    for (auto it : _colorBars)
        it->setOutsideGrayed(bVal);
}

bool SoFCColorBar::isVisible (float fVal) const
{
    return this->getActiveBar()->isVisible(fVal);
}

float SoFCColorBar::getMinValue () const
{
    return this->getActiveBar()->getMinValue();
}

float SoFCColorBar::getMaxValue () const
{
    return this->getActiveBar()->getMaxValue();
}

void SoFCColorBar::triggerChange(SoFCColorBarBase*)
{
    Notify(0);
}

void SoFCColorBar::customize(SoFCColorBarBase* child)
{
    try {
        return child->customize(this);
    }
    catch (const Base::ValueError& e) {
        e.ReportException();
    }
}

App::Color SoFCColorBar::getColor( float fVal ) const
{
    return this->getActiveBar()->getColor( fVal );
}

void SoFCColorBar::eventCallback(void * /*userdata*/, SoEventCallback * node)
{
    const SoEvent * event = node->getEvent();
    if (event->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const auto e = static_cast<const SoMouseButtonEvent*>(event);
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
        const auto e = static_cast<const SoMouseButtonEvent*>(event);

        // check if the cursor is near to the color bar
        if (!action->getPickedPoint())
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
                for (auto it : _colorBars) {
                    QAction* item = menu.addAction(QObject::tr(it->getColorBarName()));
                    item->setCheckable(true);
                    item->setChecked(it == current);
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
