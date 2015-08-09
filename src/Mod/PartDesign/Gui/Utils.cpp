/***************************************************************************
 *  Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com      *
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
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/ViewProviderPart.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/PartDesign/App/Body.h>

#include "Utils.h"


//===========================================================================
// Helper for Body
//===========================================================================

namespace PartDesignGui {

PartDesign::Body *getBody(bool messageIfNot)
{
    PartDesign::Body * activeBody = nullptr;
    Gui::MDIView *activeView = Gui::Application::Instance->activeView();

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

App::Part* getPartFor(const App::DocumentObject* obj, bool messageIfNot) {

    if(!obj)
        return nullptr;

    PartDesign::Body* body = getBodyFor(obj, false);
    if(body)
        obj = body;

    //get the part every body should belong to
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

PartDesign::Body *setUpPart(const App::Part *part)
{
    // first do the general Part setup
    Gui::ViewProviderPart::setUpPart(part);

    // check for Bodies
    // std::vector<App::DocumentObject*> bodies = part->getObjectsOfType(PartDesign::Body::getClassTypeId());
    // assert(bodies.size() == 0);

    // std::string PartName = part->getNameInDocument();
    // std::string BodyName = part->getDocument()->getUniqueObjectName("MainBody");

    Gui::Command::addModule(Gui::Command::Doc, "PartDesign");
    // Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('PartDesign::Body','%s')", BodyName.c_str());
    // Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", part->getNameInDocument());
    // Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PDBODYKEY, BodyName.c_str());
    Gui::Command::updateActive();

    return NULL;
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

    Base::Placement plm = sketch->Placement.getValue();
    Base::Vector3d pnt = plm.getPosition();

    // Currently we only handle positions that are parallel to the base planes
    Base::Rotation rot = plm.getRotation();
    Base::Vector3d sketchVector(0,0,1);
    rot.multVec(sketchVector, sketchVector);
    bool reverseSketch = (sketchVector.x + sketchVector.y + sketchVector.z) < 0.0 ;
    if (reverseSketch) sketchVector *= -1.0;
    int index;

    if (sketchVector == Base::Vector3d(0,0,1))
        index = 0;
    else if (sketchVector == Base::Vector3d(0,1,0))
        index = 1;
    else if (sketchVector == Base::Vector3d(1,0,0))
        index = 2;
    else {
        throw Base::Exception("Sketch plane cannot be migrated");
    }

    // Find the normal distance from origin to the sketch plane
    gp_Pln pln(gp_Pnt (pnt.x, pnt.y, pnt.z), gp_Dir(sketchVector.x, sketchVector.y, sketchVector.z));
    double offset = pln.Distance(gp_Pnt(0,0,0));

    if (fabs(offset) < Precision::Confusion()) {
        // One of the base planes
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), App::Part::BaseplaneTypes[index]);
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
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('PartDesign::Plane','%s')",Datum.c_str());
        QString refStr = QString::fromAscii("[(App.activeDocument().") + QString::fromAscii(App::Part::BaseplaneTypes[index]) +
            QString::fromAscii(",'')]");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = %s",Datum.c_str(), refStr.toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",Datum.c_str(), AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.superPlacement.Base.z = %f",Datum.c_str(), offset);
        Gui::Command::doCommand(Gui::Command::Doc,
                "App.activeDocument().%s.insertFeature(App.activeDocument().%s, App.activeDocument().%s)",
                body->getNameInDocument(), Datum.c_str(), sketch->getNameInDocument());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,[''])",
                sketch->getNameInDocument(), Datum.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapReversed = %s",
                sketch->getNameInDocument(), reverseSketch ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.MapMode = '%s'",
                sketch->getNameInDocument(),Attacher::AttachEngine::eMapModeStrings[Attacher::mmFlatFace]);
        Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");  // recompute the feature based on its references
    }
}


} /* PartDesignGui */ 
