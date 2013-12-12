/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QPixmap>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/events/SoLocation2Event.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <App/PropertyGeo.h>

#include "ViewProvider.h"
#include "Application.h"
#include "Document.h"
#include "ViewProviderPy.h"
#include "BitmapFactory.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "SoFCDB.h"

using namespace std;
using namespace Gui;


//**************************************************************************
//**************************************************************************
// ViewProvider
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

PROPERTY_SOURCE_ABSTRACT(Gui::ViewProvider, App::PropertyContainer)

ViewProvider::ViewProvider() 
  : pcAnnotation(0), pyViewObject(0), _iActualMode(-1), _iEditMode(-1), _updateData(true)
{
    pcRoot = new SoSeparator();
    pcRoot->ref();
    pcModeSwitch = new SoSwitch();
    pcModeSwitch->ref();
    pcTransform  = new SoTransform();
    pcTransform->ref();
    pcRoot->addChild(pcTransform);
    pcRoot->addChild(pcModeSwitch);
    sPixmap = "px";
    pcModeSwitch->whichChild = _iActualMode;
}

ViewProvider::~ViewProvider()
{
    if (pyViewObject) {
        pyViewObject->setInvalid();
        pyViewObject->DecRef();
    }

    pcRoot->unref();
    pcTransform->unref();
    pcModeSwitch->unref();
    if (pcAnnotation)
        pcAnnotation->unref();
}

bool ViewProvider::startEditing(int ModNum)
{
    bool ok = setEdit(ModNum);
    if (ok) _iEditMode = ModNum;
    return ok;
}

int ViewProvider::getEditingMode() const
{
    return _iEditMode;
}

bool ViewProvider::isEditing() const
{
    return getEditingMode() > -1;
}

void ViewProvider::finishEditing()
{
    unsetEdit(_iEditMode);
    _iEditMode = -1;
}

bool ViewProvider::setEdit(int ModNum)
{
    return true;
}

void ViewProvider::unsetEdit(int ModNum)
{
}

void ViewProvider::setEditViewer(View3DInventorViewer*, int ModNum)
{
}

void ViewProvider::unsetEditViewer(View3DInventorViewer*)
{
}

bool ViewProvider::isUpdatesEnabled () const
{
    return _updateData;
}

void ViewProvider::setUpdatesEnabled (bool enable)
{
    _updateData = enable;
}

void highlight(const HighlightMode& high)
{
    

}

void ViewProvider::eventCallback(void * ud, SoEventCallback * node)
{
    const SoEvent * ev = node->getEvent();
    Gui::View3DInventorViewer* viewer = reinterpret_cast<Gui::View3DInventorViewer*>(node->getUserData());
    ViewProvider *self = reinterpret_cast<ViewProvider*>(ud);
    assert(self);

    try {
        // Keyboard events
        if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
            SoKeyboardEvent * ke = (SoKeyboardEvent *)ev;
            const SbBool press = ke->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;
            switch (ke->getKey()) {
            case SoKeyboardEvent::ESCAPE:
                if (self->keyPressed (press, ke->getKey()))
                    node->setHandled();
                else
                    Gui::Application::Instance->activeDocument()->resetEdit();
                break;
            default:
                // call the virtual method
                if (self->keyPressed (press, ke->getKey()))
                    node->setHandled();
                break;
            }
        }
        // switching the mouse buttons
        else if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {

            const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
            const int button = event->getButton();
            const SbBool press = event->getState() == SoButtonEvent::DOWN ? TRUE : FALSE;

            // call the virtual method
            if (self->mouseButtonPressed(button,press,ev->getPosition(),viewer))
                node->setHandled();
        }
        // Mouse Movement handling
        else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
            if (self->mouseMove(ev->getPosition(),viewer))
                node->setHandled();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Unhandled exception in ViewProvider::eventCallback: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std exception in ViewProvider::eventCallback: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown C++ exception in ViewProvider::eventCallback");
    }
}

SoSeparator* ViewProvider::getAnnotation(void)
{
    if (!pcAnnotation) {
        pcAnnotation = new SoSeparator();
        pcAnnotation->ref();
        pcRoot->addChild(pcAnnotation);
    }
    return pcAnnotation;
}

void ViewProvider::update(const App::Property* prop)
{
    // Hide the object temporarily to speed up the update
    if (!isUpdatesEnabled())
        return;
    bool vis = ViewProvider::isShow();
    if (vis) ViewProvider::hide();
    updateData(prop);
    if (vis) ViewProvider::show();
}

QIcon ViewProvider::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap(sPixmap);
}

void ViewProvider::setTransformation(const Base::Matrix4D &rcMatrix)
{
    double dMtrx[16];
    rcMatrix.getGLMatrix(dMtrx);

    pcTransform->setMatrix(SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]));
}

void ViewProvider::setTransformation(const SbMatrix &rcMatrix)
{
    pcTransform->setMatrix(rcMatrix);
}

SbMatrix ViewProvider::convert(const Base::Matrix4D &rcMatrix) const
{
    double dMtrx[16];
    rcMatrix.getGLMatrix(dMtrx);
    return SbMatrix(dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]);
}

void ViewProvider::addDisplayMaskMode( SoNode *node, const char* type )
{
    _sDisplayMaskModes[ type ] = pcModeSwitch->getNumChildren();
    pcModeSwitch->addChild( node );
}

void ViewProvider::setDisplayMaskMode( const char* type )
{
    std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.find( type );
    if (it != _sDisplayMaskModes.end())
        pcModeSwitch->whichChild = it->second;
    else
        pcModeSwitch->whichChild = -1;
    _iActualMode = pcModeSwitch->whichChild.getValue();
}

std::vector<std::string> ViewProvider::getDisplayMaskModes() const
{
    std::vector<std::string> types;
    for (std::map<std::string, int>::const_iterator it = _sDisplayMaskModes.begin();
         it != _sDisplayMaskModes.end(); ++it)
        types.push_back( it->first );
    return types;
}

/**
 * If you add new viewing modes in @ref getDisplayModes() then you need to reimplement
 * also seDisplaytMode() to handle these new modes by setting the appropriate display
 * mode.
 */
void ViewProvider::setDisplayMode(const char* ModeName)
{
    _sCurrentMode = ModeName;
}

std::string ViewProvider::getActiveDisplayMode(void) const
{
    return _sCurrentMode;
}

void ViewProvider::hide(void)
{
    pcModeSwitch->whichChild = -1;
}

void ViewProvider::show(void)
{
    pcModeSwitch->whichChild = _iActualMode;
}

bool ViewProvider::isShow(void) const
{
    return pcModeSwitch->whichChild.getValue() != -1;
}

void ViewProvider::setVisible(bool s)
{
    s ? show() : hide();
}

bool ViewProvider::isVisible() const
{
    return isShow();
}

void ViewProvider::setDefaultMode(int val)
{
    _iActualMode = val;
}

void ViewProvider::onChanged(const App::Property* prop)
{
    Application::Instance->signalChangedObject(*this, *prop);
}

std::string ViewProvider::toString() const
{
    return SoFCDB::writeNodesToString(pcRoot);
}

PyObject* ViewProvider::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

SoPickedPoint* ViewProvider::getPointOnRay(const SbVec2s& pos, const View3DInventorViewer* viewer) const
{
    // for convenience make a pick ray action to get the (potentially) picked entity in the provider
    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(viewer->getCamera());
    root->addChild(pcRoot);

    SoRayPickAction rp(viewer->getViewportRegion());
    rp.setPoint(pos);
    rp.apply(root);
    root->unref();

    SoPickedPoint* pick = rp.getPickedPoint();
    return (pick ? new SoPickedPoint(*pick) : 0);
}

SoPickedPoint* ViewProvider::getPointOnRay(const SbVec3f& pos,const SbVec3f& dir, const View3DInventorViewer* viewer) const
{
    // Note: There seems to be a  bug with setRay() which causes SoRayPickAction
    // to fail to get intersections between the ray and a line
    SoRayPickAction rp(viewer->getViewportRegion());
    rp.setRay(pos,dir);
    rp.apply(pcRoot);

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : 0);
}

std::vector<Base::Vector3d> ViewProvider::getPickedPoints(const SoPickedPoint* pp) const
{
    // the default implementation just returns the picked point from the visual representation
    std::vector<Base::Vector3d> pts;
    const SbVec3f& vec = pp->getPoint();
    pts.push_back(Base::Vector3d(vec[0],vec[1],vec[2]));
    return pts;
}
