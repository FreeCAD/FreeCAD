/***************************************************************************
 *  Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>     *
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
#include <QMessageBox>
# include <Precision.hxx>
# include <gp_Pln.hxx>
#endif

#include <App/Part.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/DocumentObjectGroup.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/ViewProviderPart.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>

#include "ReferenceSelection.h"
#include "Utils.h"
#include "WorkflowManager.h"

//===========================================================================
// Helper for Body
//===========================================================================
using namespace Attacher;

namespace PartDesignGui {

PartDesign::Body *getBody(bool messageIfNot)
{
    PartDesign::Body * activeBody = nullptr;
    Gui::MDIView *activeView = Gui::Application::Instance->activeView();

    if ( PartDesignGui::assureModernWorkflow ( activeView->getAppDocument() ) ) {

        if (activeView) {
            activeBody = activeView->getActiveObject<PartDesign::Body*>(PDBODYKEY);
        }

        if (!activeBody && messageIfNot) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
                QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                            "Please make one active (double click) or create one. If you have a legacy document "
                            "with PartDesign objects without Body, use the transfer function in "
                            "PartDesign to put them into a Body."
                            ));
        }
    }
    return activeBody;

}

PartDesign::Body *getBodyFor(const App::DocumentObject* obj, bool messageIfNot)
{
    if(!obj)
        return nullptr;

    PartDesign::Body * rv = getBody( /*messageIfNot =*/ false);
    if(rv && rv->hasFeature(obj))
        return rv;

    rv = PartDesign::Body::findBodyOf(obj);
    if (rv) {
        return rv;
    }

    if (messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Feature is not in a body"),
            QObject::tr("In order to use this feature it needs to belong to a body object in the document."));
    }

    return nullptr;
}

App::Part* getActivePart() {
    Gui::MDIView *activeView = Gui::Application::Instance->activeView();
    if ( activeView ) {
        return activeView->getActiveObject<App::Part*> (PARTKEY);
    } else {
        return 0;
    }
}

App::Part* getPartFor(const App::DocumentObject* obj, bool messageIfNot) {

    if(!obj)
        return nullptr;

    PartDesign::Body* body = getBodyFor(obj, false);
    if(body)
        obj = body;

    //get the part
    for(App::Part* p : obj->getDocument()->getObjectsOfType<App::Part>()) {
        if(p->hasObject(obj)) {
            return p;
        }
    }

    if (messageIfNot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Feature is not in a part"),
            QObject::tr("In order to use this feature it needs to belong to a part object in the document."));
    }

    return nullptr;
}

static void buildDefaultPartAndBody(const App::Document* doc)
{
  // This adds both the base planes and the body
    std::string PartName = doc->getUniqueObjectName("Part");
    //// create a PartDesign Part for now, can be later any kind of Part or an empty one
    Gui::Command::addModule(Gui::Command::Doc, "PartDesignGui");
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')", PartName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "PartDesignGui.setUpPart(App.activeDocument().%s)", PartName.c_str());
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('Part',App.activeDocument().%s)", PartName.c_str());
}


void fixSketchSupport (Sketcher::SketchObject* sketch)
{
    App::DocumentObject* support = sketch->Support.getValue();

    if (support)
        return; // Sketch is on a face of a solid, do nothing

    const App::Document* doc = sketch->getDocument();
    PartDesign::Body *body = getBodyFor(sketch, /*messageIfNot*/ 0);
    if (!body) {
        throw Base::Exception ("Coudn't find body for the sketch");
    }

    // Get the Origin for the body
    App::Origin *origin = body->getOrigin (); // May throw by itself

    Base::Placement plm = sketch->Placement.getValue();
    Base::Vector3d pnt = plm.getPosition();

    // Currently we only handle positions that are parallel to the base planes
    Base::Rotation rot = plm.getRotation();
    Base::Vector3d sketchVector(0,0,1);
    rot.multVec(sketchVector, sketchVector);
    bool reverseSketch = (sketchVector.x + sketchVector.y + sketchVector.z) < 0.0 ;
    if (reverseSketch) sketchVector *= -1.0;

    App::Plane *plane =0;

    if (sketchVector == Base::Vector3d(0,0,1))
        plane = origin->getXY ();
    else if (sketchVector == Base::Vector3d(0,1,0))
        plane = origin->getXZ ();
    else if (sketchVector == Base::Vector3d(1,0,0))
        plane = origin->getYZ ();
    else {
        throw Base::Exception("Sketch plane cannot be migrated");
    }
    assert (plane);

    // Find the normal distance from origin to the sketch plane
    gp_Pln pln(gp_Pnt (pnt.x, pnt.y, pnt.z), gp_Dir(sketchVector.x, sketchVector.y, sketchVector.z));
    double offset = pln.Distance(gp_Pnt(0,0,0));
    // TODO Issue a message if sketch have coordinates offset inside the plain (2016-08-15, Fat-Zer)

    if (fabs(offset) < Precision::Confusion()) {
        // One of the base planes
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), plane->getNameInDocument () );
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapReversed = %s",
                sketch->getNameInDocument(), reverseSketch ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                sketch->getNameInDocument(), Attacher::AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);

    } else {
        // Offset to base plane
        // Find out which direction we need to offset
        double a = sketchVector.GetAngle(pnt);
        if ((a < -M_PI_2) || (a > M_PI_2))
            offset *= -1.0;

        std::string Datum = doc->getUniqueObjectName("DatumPlane");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('PartDesign::Plane','%s')",
                Datum.c_str());
        QString refStr = QString::fromAscii("[(App.activeDocument().%1,'')]")
            .arg ( QString::fromAscii ( plane->getNameInDocument () ) );
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = %s",
                Datum.c_str(), refStr.toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                Datum.c_str(), AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.superPlacement.Base.z = %f",
                Datum.c_str(), offset);
        Gui::Command::doCommand(Gui::Command::Doc,
                "App.activeDocument().%s.insertFeature(App.activeDocument().%s, App.activeDocument().%s)",
                body->getNameInDocument(), Datum.c_str(), sketch->getNameInDocument());
        Gui::Command::doCommand(Gui::Command::Doc,
                "App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), Datum.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapReversed = %s",
                sketch->getNameInDocument(), reverseSketch ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                sketch->getNameInDocument(),Attacher::AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
    }
}

bool isPartDesignAwareObjecta (App::DocumentObject *obj, bool respectGroups = false ) {
    return (obj->isDerivedFrom( PartDesign::Feature::getClassTypeId () ) ||
            PartDesign::Body::isAllowed ( obj ) ||
            obj->isDerivedFrom ( PartDesign::Body::getClassTypeId () ) ||
            ( respectGroups && (
                                obj->isDerivedFrom (App::GeoFeatureGroup::getClassTypeId () ) ||
                                obj->isDerivedFrom (App::DocumentObjectGroup::getClassTypeId () )
                               ) ) );
}

bool isAnyNonPartDesignLinksTo ( PartDesign::Feature *feature, bool respectGroups ) {
    App::Document *doc = feature->getDocument();

    for ( const auto & obj: doc->getObjects () ) {
         if ( !isPartDesignAwareObjecta ( obj, respectGroups ) ) {
             std::vector <App::Property *> properties;
             obj->getPropertyList ( properties );
             for (auto prop: properties ) {
                if ( prop->isDerivedFrom ( App::PropertyLink::getClassTypeId() ) ) {
                    if ( static_cast <App::PropertyLink *> ( prop )->getValue () == feature ) {
                        return true;
                    }
                } else if ( prop->isDerivedFrom ( App::PropertyLinkSub::getClassTypeId() ) ) {
                    if ( static_cast <App::PropertyLinkSub *> ( prop )->getValue () == feature ) {
                        return true;
                    }
                } else if ( prop->isDerivedFrom ( App::PropertyLinkList::getClassTypeId() ) ) {
                    auto values = static_cast <App::PropertyLinkList *> ( prop )->getValues ();
                    if ( std::find ( values.begin (), values.end (), feature ) != values.end() ) {
                        return true;
                    }
                } else if ( prop->isDerivedFrom ( App::PropertyLinkSubList::getClassTypeId() ) ) {
                    auto values = static_cast <App::PropertyLinkSubList *> ( prop )->getValues ();
                    if ( std::find ( values.begin (), values.end (), feature ) != values.end() ) {
                        return true;
                    }
                }
             }
         }
    }

    return false;
}

void relinkToBody (PartDesign::Feature *feature) {
    App::Document *doc = feature->getDocument();
    PartDesign::Body *body = PartDesign::Body::findBodyOf ( feature );

    if (!body) {
        throw Base::Exception ("Coudn't find body for the feature");
    }

    std::string bodyName = body->getNameInDocument ();

    for ( const auto & obj: doc->getObjects () ) {
        if ( !isPartDesignAwareObjecta ( obj ) ) {
            std::vector <App::Property *> properties;
            obj->getPropertyList ( properties );
            for (auto prop: properties ) {
                std::string valueStr;
                if ( prop->isDerivedFrom ( App::PropertyLink::getClassTypeId() ) ) {
                    App::PropertyLink *propLink = static_cast <App::PropertyLink *> ( prop );
                    if ( propLink->getValue() != feature ) {
                        continue;
                    }
                    valueStr = std::string ( "App.activeDocument()." ).append ( bodyName );
                } else if ( prop->isDerivedFrom ( App::PropertyLinkSub::getClassTypeId() ) ) {
                    App::PropertyLinkSub *propLink = static_cast <App::PropertyLinkSub *> ( prop );
                    if ( propLink->getValue() != feature ) {
                        continue;
                    }
                    valueStr = buildLinkSubPythonStr ( body, propLink->getSubValues() );
                } else if ( prop->isDerivedFrom ( App::PropertyLinkList::getClassTypeId() ) ) {
                    App::PropertyLinkList *propLink = static_cast <App::PropertyLinkList *> ( prop );
                    std::vector <App::DocumentObject *> linkList = propLink->getValues ();
                    bool valueChanged=false;
                    for (auto & link : linkList ) {
                        if ( link == feature ) {
                            link = body;
                            valueChanged = true;
                        }
                    }
                    if ( valueChanged ) {
                        valueStr = buildLinkListPythonStr ( linkList );
                        // TODO Issue some message here due to it likely will break something
                        //     (2015-08-13, Fat-Zer)
                    }
                } else if ( prop->isDerivedFrom ( App::PropertyLinkSub::getClassTypeId() ) ) {
                    App::PropertyLinkSubList *propLink = static_cast <App::PropertyLinkSubList *> ( prop );
                    std::vector <App::DocumentObject *> linkList = propLink->getValues ();
                    bool valueChanged=false;
                    for (auto & link : linkList ) {
                        if ( link == feature ) {
                            link = body;
                            valueChanged = true;
                        }
                    }
                    if ( valueChanged ) {
                        valueStr = buildLinkSubListPythonStr ( linkList, propLink->getSubValues() );
                        // TODO Issue some message here due to it likely will break something
                        //     (2015-08-13, Fat-Zer)
                    }
                }

                if ( !valueStr.empty () ) {
                    Gui::Command::doCommand ( Gui::Command::Doc, "App.activeDocument().%s.%s=%s",
                            obj->getNameInDocument (), prop->getName (), valueStr.c_str() );
                }
            }
        }
    }
}

} /* PartDesignGui */
