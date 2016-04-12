/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2015     *
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
# include <QApplication>
# include <QPixmap>
#endif

#include <App/Origin.h>
#include <App/Plane.h>
#include <App/Line.h>
#include <App/Document.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderOrigin.h"
#include "ViewProviderPlane.h"
#include "ViewProviderLine.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "Tree.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#include "Base/Console.h"
#include <boost/bind.hpp>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSeparator.h>

using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderOrigin, Gui::ViewProviderGeoFeatureGroup)


/**
 * Creates the view provider for an object group.
 */
ViewProviderOrigin::ViewProviderOrigin() 
  : tempVisMode(false)
{
    sPixmap = "CoordinateSystem";
    Visibility.setValue(false);
}

ViewProviderOrigin::~ViewProviderOrigin()
{
}


bool ViewProviderOrigin::canDragObjects() const
{
    return false;
}

bool ViewProviderOrigin::canDropObjects() const
{
    return false;
}


bool ViewProviderOrigin::setEdit(int ModNum)
{
    return true;
}

void ViewProviderOrigin::unsetEdit(int ModNum)
{

}

QIcon ViewProviderOrigin::getIcon(void) const
{
    return Gui::ViewProvider::getIcon();
}

void ViewProviderOrigin::setTemporaryVisibilityMode(bool onoff, Gui::Document* doc)
{
    tempVisDoc = doc;
    
    if(tempVisMode == onoff)
        return;
    
    tempVisMode = onoff;
    if(onoff && doc) {
        App::Origin* origin = static_cast<App::Origin*>(pcObject);
        tempVisMap.clear();
        
        for(App::DocumentObject* obj : origin->getObjects()) {
            Gui::ViewProvider* vp = doc->getViewProvider(obj);
            if(vp) {
                tempVisMap[vp] = vp->isVisible();
                vp->setVisible(false);
            }
        }
        tempVisMap[this] = isVisible();
        setVisible(true);
    }
    else if(!onoff) {    
        for(std::pair<Gui::ViewProvider*, bool> pair : tempVisMap) 
            pair.first->setVisible(pair.second);
    }
}

void ViewProviderOrigin::setTemporaryVisibility(App::DocumentObject* obj, bool onoff)
{
    Gui::ViewProvider* vp = tempVisDoc->getViewProvider(obj);
    if(vp) {
        vp->setVisible(onoff);
    }
}

bool ViewProviderOrigin::isTemporaryVisibilityMode()
{
    return tempVisMode;
}

void ViewProviderOrigin::setTemporaryVisibilityAxis(bool onoff)
{
    for(App::DocumentObject* obj : static_cast<App::Origin*>(pcObject)->getObjectsOfType(App::Line::getClassTypeId())) {
     
        Gui::ViewProvider* vp = tempVisDoc->getViewProvider(obj);
        vp->setVisible(onoff);        
    }
}

void ViewProviderOrigin::setTemporaryVisibilityPlanes(bool onoff)
{
    for(App::DocumentObject* obj : static_cast<App::Origin*>(pcObject)->getObjectsOfType(App::Plane::getClassTypeId())) {
     
        Gui::ViewProvider* vp = tempVisDoc->getViewProvider(obj);
        vp->setVisible(onoff);        
    }
}



// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderOriginPython, Gui::ViewProviderOrigin)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderOrigin>;
}
