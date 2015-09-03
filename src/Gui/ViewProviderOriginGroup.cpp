/***************************************************************************
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoSeparator.h>
# include <boost/bind.hpp>
#endif

#include <Base/Console.h>
#include <App/OriginGroup.h>
#include <App/Origin.h>

#include "Application.h"
#include "Document.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProviderOrigin.h"
#include "ViewProviderOriginFeature.h"

#include "ViewProviderOriginGroup.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderOriginGroup, Gui::ViewProviderGeoFeatureGroup)

ViewProviderOriginGroup::ViewProviderOriginGroup ()
{ }

ViewProviderOriginGroup::~ViewProviderOriginGroup () { 
    connectChangedObjectApp.disconnect();
    connectChangedObjectGui.disconnect();
}

std::vector<App::DocumentObject*> ViewProviderOriginGroup::constructChildren (
        const std::vector<App::DocumentObject*> &children ) const
{
    App::OriginGroup *group = static_cast <App::OriginGroup *> ( getObject() );
    App::DocumentObject *originObj = group->Origin.getValue();

    // Origin must be first
    if (originObj) {
        std::vector<App::DocumentObject*> rv;
        rv.push_back (originObj);
        std::copy (children.begin(), children.end(), std::back_inserter (rv));
        return rv;
    } else { // Generally shouldn't happen but must be handled in case origin is lost
        return children;
    }
}


std::vector<App::DocumentObject*> ViewProviderOriginGroup::claimChildren () const {
    return constructChildren ( ViewProviderGeoFeatureGroup::claimChildren () );
}

std::vector<App::DocumentObject*> ViewProviderOriginGroup::claimChildren3D () const {
    return constructChildren ( ViewProviderGeoFeatureGroup::claimChildren3D () );
}

void ViewProviderOriginGroup::attach(App::DocumentObject *pcObject) {
    ViewProviderGeoFeatureGroup::attach ( pcObject );

    App::Document *adoc  = pcObject->getDocument ();
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( adoc ) ;

    assert ( adoc );
    assert ( gdoc );

    connectChangedObjectApp = adoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderOriginGroup::slotChangedObjectApp, this, _1) );
    
    connectChangedObjectGui = gdoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderOriginGroup::slotChangedObjectGui, this, _1) );
}

void ViewProviderOriginGroup::updateData ( const App::Property* prop ) {
    App::OriginGroup *group = static_cast<App::OriginGroup*> ( getObject() );
    if ( group && prop == &group->Group ) {
        updateOriginSize();
    }

    ViewProviderGeoFeatureGroup::updateData ( prop );
}

void ViewProviderOriginGroup::slotChangedObjectApp ( const App::DocumentObject& obj) {
    App::OriginGroup *group = static_cast<App::OriginGroup*> ( getObject() );
    if ( group && group->hasObject (&obj, /*recusive=*/ true ) ) {
        updateOriginSize ();
    }
}

void ViewProviderOriginGroup::slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& vp) {
    if ( !vp.isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) &&
         !vp.isDerivedFrom ( Gui::ViewProviderOriginFeature::getClassTypeId () ) ) {
        // Ignore origins to avoid infinite recursion (not likely in a well-formed focument, 
        //          but may happen in documents designed in old versions of assembly branch )
        App::OriginGroup *group = static_cast<App::OriginGroup*> ( getObject() );
        App::DocumentObject *obj = vp.getObject ();

        if ( group && obj && group->hasObject (obj, /*recusive=*/ true ) ) {
            updateOriginSize ();
        }
    }
}

void ViewProviderOriginGroup::updateOriginSize () {
    App::OriginGroup* group = static_cast<App::OriginGroup*> ( getObject() );

    // obtain an Origin and it's ViewProvider
    App::Origin* origin = 0;
    Gui::ViewProviderOrigin* vpOrigin = 0;
    try {
        origin = group->getOrigin ();
        assert (origin);

        Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider(origin);
        if (!vp) {
            throw Base::Exception ("No view provider linked to the Origin");
        }
        assert ( vp->isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) );
        vpOrigin = static_cast <Gui::ViewProviderOrigin *> ( vp );
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what() );
        return;
    }

    View3DInventorViewer* viewer = static_cast<View3DInventor*>(this->getActiveView())->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());

    // calculate the bounding box for out content
    SbBox3f bbox(0,0,0, 0,0,0);
    for(App::DocumentObject* obj : group->getGeoSubObjects()) {
        ViewProvider *vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;
        }

        bboxAction.apply ( vp->getRoot () );
        bbox.extendBy ( bboxAction.getBoundingBox () );
    };

    // get the bounding box values
    SbVec3f max = bbox.getMax();
    SbVec3f min = bbox.getMin();

    Base::Vector3d size;

    for (uint_fast8_t i=0; i<3; i++) {
        size[i] = std::max ( fabs ( max[i] ), fabs ( min[i] ) );
        if (size[i] < 1e-7) { // TODO replace the magic values (2015-08-31, Fat-Zer)
            size[i] = ViewProviderOrigin::defaultSize();
        }
    }

    vpOrigin->Size.setValue ( size * 1.3 );
}
