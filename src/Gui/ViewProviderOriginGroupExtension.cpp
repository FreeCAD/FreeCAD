/***************************************************************************
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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
#endif

#include "ViewProviderOriginGroupExtension.h"
#include "Application.h"
#include "Document.h"
#include "ViewProviderOriginFeature.h"
#include "ViewProviderOrigin.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "Command.h"
#include <App/OriginGroupExtension.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <boost/bind.hpp>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSeparator.h>

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderOriginGroupExtension, Gui::ViewProviderGeoFeatureGroupExtension)

ViewProviderOriginGroupExtension::ViewProviderOriginGroupExtension()
{
    initExtensionType(ViewProviderOriginGroupExtension::getExtensionClassTypeId());
}

ViewProviderOriginGroupExtension::~ViewProviderOriginGroupExtension()
{
    connectChangedObjectApp.disconnect();
    connectChangedObjectGui.disconnect();
}

std::vector<App::DocumentObject*> ViewProviderOriginGroupExtension::constructChildren (
        const std::vector<App::DocumentObject*> &children ) const
{
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::OriginGroupExtension>();
    if(!group)
        return children;
    
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


std::vector<App::DocumentObject*> ViewProviderOriginGroupExtension::extensionClaimChildren () const {
    return constructChildren ( ViewProviderGeoFeatureGroupExtension::extensionClaimChildren () );
}

std::vector<App::DocumentObject*> ViewProviderOriginGroupExtension::extensionClaimChildren3D () const {
    return constructChildren ( ViewProviderGeoFeatureGroupExtension::extensionClaimChildren3D () );
}

void ViewProviderOriginGroupExtension::extensionAttach(App::DocumentObject *pcObject) {
    ViewProviderGeoFeatureGroupExtension::extensionAttach ( pcObject );

    App::Document *adoc  = pcObject->getDocument ();
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( adoc ) ;

    assert ( adoc );
    assert ( gdoc );

    connectChangedObjectApp = adoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderOriginGroupExtension::slotChangedObjectApp, this, _1) );
    
    connectChangedObjectGui = gdoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderOriginGroupExtension::slotChangedObjectGui, this, _1) );
}

void ViewProviderOriginGroupExtension::extensionUpdateData( const App::Property* prop ) {
    
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::OriginGroupExtension>();
    if ( group && prop == &group->Group ) {
        updateOriginSize();
    }

    ViewProviderGeoFeatureGroupExtension::extensionUpdateData ( prop );
}

void ViewProviderOriginGroupExtension::slotChangedObjectApp ( const App::DocumentObject& obj) {
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::OriginGroupExtension>();
    if ( group && group->hasObject (&obj, /*recusive=*/ true ) ) {
        updateOriginSize ();
    }
}

void ViewProviderOriginGroupExtension::slotChangedObjectGui ( const Gui::ViewProviderDocumentObject& vp) {
    if ( !vp.isDerivedFrom ( Gui::ViewProviderOriginFeature::getClassTypeId () )) {
        // Ignore origins to avoid infinite recursion (not likely in a well-formed focument, 
        //          but may happen in documents designed in old versions of assembly branch )
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::OriginGroupExtension>();
        App::DocumentObject *obj = vp.getObject ();

        if ( group && obj && group->hasObject (obj, /*recusive=*/ true ) ) {
            updateOriginSize ();
        }
    }
}

void ViewProviderOriginGroupExtension::updateOriginSize () {
    
    if(getExtendedViewProvider()->getObject()->isRemoving() ||
       getExtendedViewProvider()->getObject()->getDocument()->testStatus(App::Document::Restoring))
        return;
    
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::OriginGroupExtension>();
    if(!group)
        return;

    // obtain an Origin and it's ViewProvider
    App::Origin* origin = 0;
    Gui::ViewProviderOrigin* vpOrigin = 0;
    try {
        origin = group->getOrigin ();
        assert (origin);

        Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider(origin);
        if (!vp) {
            throw Base::RuntimeError ("No view provider linked to the Origin");
        }
        assert ( vp->isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) );
        vpOrigin = static_cast <Gui::ViewProviderOrigin *> ( vp );
    } catch (const Base::Exception &ex) {
        // if is restoring it is normal that the origin property is null until after restored, so no need to report this.
        if(!getExtendedViewProvider()->getDocument()->getDocument()->testStatus(App::Document::Restoring))
            Base::Console().Error ("%s\n", ex.what() );
        return;
    }


    Gui::Document* gdoc = getExtendedViewProvider()->getDocument();
    if(!gdoc) 
        return;
    
    Gui::MDIView* view = gdoc->getViewOfViewProvider(getExtendedViewProvider());
    if(!view)
        return;
    
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());

    // calculate the bounding box for out content
    SbBox3f bbox(0,0,0, 0,0,0);
    for(App::DocumentObject* obj : group->Group.getValues()) {
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

namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderOriginGroupExtensionPython, Gui::ViewProviderOriginGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderOriginGroupExtension>;
}
