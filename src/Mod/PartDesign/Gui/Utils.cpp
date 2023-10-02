/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
# include <gp_Pln.hxx>
# include <Precision.hxx>
#endif

#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Utils.h"
#include "DlgActiveBody.h"
#include "ReferenceSelection.h"
#include "WorkflowManager.h"


FC_LOG_LEVEL_INIT("PartDesignGui",true,true)

//===========================================================================
// Helper for Body
//===========================================================================
using namespace Attacher;

namespace PartDesignGui {

// TODO: Refactor DocumentObjectItem::getSubName() that has similar logic
App::DocumentObject* getParent(App::DocumentObject* obj, std::string& subname)
{
    auto inlist = obj->getInList();
    for (auto it : inlist) {
        if (it->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            std::string parent;
            parent += obj->getNameInDocument();
            parent += '.';
            subname = parent + subname;
            return getParent(it, subname);
        }
    }

    return obj;
}

bool setEdit(App::DocumentObject *obj, PartDesign::Body *body) {
    if (!obj || !obj->getNameInDocument()) {
        FC_ERR("invalid object");
        return false;
    }
    if (!body) {
        body = getBodyFor(obj, false);
        if (!body) {
            FC_ERR("no body found");
            return false;
        }
    }
    auto *activeView = Gui::Application::Instance->activeView();
    if (!activeView)
        return false;
    App::DocumentObject *parent = nullptr;
    std::string subname;
    auto activeBody = activeView->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    if (activeBody != body) {
        parent = obj;
    }
    else {
        parent = getParent(obj, subname);
    }

    Gui::cmdGuiDocument(parent, std::ostringstream() << "setEdit("
                                                     << Gui::Command::getObjectCmd(parent)
                                                     << ", 0, '" << subname << "')");
    return true;
}

/*!
 * \brief Return active body or show a warning message.
 * If \a autoActivate is true (the default) then if there is
 * only single body in the document it will be activated.
 * \param messageIfNot
 * \param autoActivate
 * \return Body
 */
PartDesign::Body *getBody(bool messageIfNot, bool autoActivate, bool assertModern,
        App::DocumentObject **topParent, std::string *subname)
{
    PartDesign::Body * activeBody = nullptr;
    Gui::MDIView *activeView = Gui::Application::Instance->activeView();

    if (activeView) {
        auto doc = activeView->getAppDocument();
        bool singleBodyDocument = doc->countObjectsOfType(PartDesign::Body::getClassTypeId()) == 1;
        if (assertModern && PartDesignGui::assureModernWorkflow (doc) ) {
            activeBody = activeView->getActiveObject<PartDesign::Body*>(PDBODYKEY,topParent,subname);

            if (!activeBody && singleBodyDocument && autoActivate) {
                auto bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
                App::DocumentObject *body = nullptr;
                if(bodies.size()==1) {
                    body = bodies[0];
                    activeBody = makeBodyActive(body, doc, topParent, subname);
                }
            }
            if (!activeBody && messageIfNot) {
                DlgActiveBody dia(
                    Gui::getMainWindow(),
                    doc,
                    QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                                "Please make one active (double click) or create one."
                                "\n\nIf you have a legacy document with PartDesign objects without Body, "
                                "use the migrate function in PartDesign to put them into a Body."
                        ));
                if (dia.exec() == QDialog::DialogCode::Accepted)
                    activeBody = dia.getActiveBody();
            }
        }
    }

    return activeBody;
}

PartDesign::Body * makeBodyActive(App::DocumentObject *body, App::Document *doc,
                                  App::DocumentObject **topParent,
                                  std::string *subname)
{
    App::DocumentObject *parent = nullptr;
    std::string sub;

    for(auto &v : body->getParents()) {
        if(v.first->getDocument()!=doc)
            continue;
        if(parent) {
            body = nullptr;
            break;
        }
        parent = v.first;
        sub = v.second;
    }

    if(body) {
        auto _doc = parent?parent->getDocument():body->getDocument();
        Gui::cmdGuiDocument(_doc, std::stringstream()
                      << "ActiveView.setActiveObject('" << PDBODYKEY
                      << "'," << Gui::Command::getObjectCmd(parent?parent:body)
                      << ",'" << sub << "')");
        return Gui::Application::Instance->activeView()->
            getActiveObject<PartDesign::Body*>(PDBODYKEY,topParent,subname);
    }

    return dynamic_cast<PartDesign::Body*>(body);
}

void needActiveBodyError()
{
    QMessageBox::warning( Gui::getMainWindow(),
        QObject::tr("Active Body Required"),
        QObject::tr("To create a new PartDesign object, there must be "
                    "an active Body object in the document. Please make "
                    "one active (double click) or create a new Body.") );
}

PartDesign::Body * makeBody(App::Document *doc)
{
    // This is intended as a convenience when starting a new document.
    auto bodyName( doc->getUniqueObjectName("Body") );
    Gui::Command::doCommand( Gui::Command::Doc,
                             "App.getDocument('%s').addObject('PartDesign::Body','%s')",
                             doc->getName(), bodyName.c_str() );
    auto body = dynamic_cast<PartDesign::Body*>(doc->getObject(bodyName.c_str()));
    if(body)
        makeBodyActive(body, doc);
    return body;
}

PartDesign::Body *getBodyFor(const App::DocumentObject* obj, bool messageIfNot,
                             bool autoActivate, bool assertModern,
                             App::DocumentObject **topParent, std::string *subname)
{
    if(!obj)
        return nullptr;

    PartDesign::Body * rv = getBody(/*messageIfNot =*/false, autoActivate, assertModern, topParent, subname);
    if (rv && rv->hasObject(obj))
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
        return nullptr;
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

//static void buildDefaultPartAndBody(const App::Document* doc)
//{
//  // This adds both the base planes and the body
//    std::string PartName = doc->getUniqueObjectName("Part");
//    //// create a PartDesign Part for now, can be later any kind of Part or an empty one
//    Gui::Command::addModule(Gui::Command::Doc, "PartDesignGui");
//    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')", PartName.c_str());
//    Gui::Command::doCommand(Gui::Command::Doc, "PartDesignGui.setUpPart(App.activeDocument().%s)", PartName.c_str());
//    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('Part',App.activeDocument().%s)", PartName.c_str());
//}


void fixSketchSupport (Sketcher::SketchObject* sketch)
{
    App::DocumentObject* support = sketch->Support.getValue();

    if (support)
        return; // Sketch is on a face of a solid, do nothing

    const App::Document* doc = sketch->getDocument();
    PartDesign::Body *body = getBodyFor(sketch, /*messageIfNot*/ false);
    if (!body) {
        throw Base::RuntimeError ("Couldn't find body for the sketch");
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

    App::Plane *plane =nullptr;

    if (sketchVector == Base::Vector3d(0,0,1))
        plane = origin->getXY ();
    else if (sketchVector == Base::Vector3d(0,1,0))
        plane = origin->getXZ ();
    else if (sketchVector == Base::Vector3d(1,0,0))
        plane = origin->getYZ ();
    else {
        throw Base::ValueError("Sketch plane cannot be migrated");
    }
    assert (plane);

    // Find the normal distance from origin to the sketch plane
    gp_Pln pln(gp_Pnt (pnt.x, pnt.y, pnt.z), gp_Dir(sketchVector.x, sketchVector.y, sketchVector.z));
    double offset = pln.Distance(gp_Pnt(0,0,0));
    // TODO Issue a message if sketch have coordinates offset inside the plain (2016-08-15, Fat-Zer)

    if (fabs(offset) < Precision::Confusion()) {
        // One of the base planes
        FCMD_OBJ_CMD(sketch,"Support = (" << Gui::Command::getObjectCmd(plane) << ",[''])");
        FCMD_OBJ_CMD(sketch,"MapReversed = " << (reverseSketch ? "True" : "False"));
        FCMD_OBJ_CMD(sketch,"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace) << "'");

    } else {
        // Offset to base plane
        // Find out which direction we need to offset
        double a = sketchVector.GetAngle(pnt);
        if ((a < -M_PI_2) || (a > M_PI_2))
            offset *= -1.0;

        std::string Datum = doc->getUniqueObjectName("DatumPlane");
        FCMD_DOC_CMD(doc,"addObject('PartDesign::Plane','"<<Datum<<"')");
        auto obj = doc->getObject(Datum.c_str());
        FCMD_OBJ_CMD(obj,"Support = [(" << Gui::Command::getObjectCmd(plane) << ",'')]");
        FCMD_OBJ_CMD(obj,"MapMode = '" << AttachEngine::getModeName(Attacher::mmFlatFace) << "'");
        FCMD_OBJ_CMD(obj,"AttachmentOffset.Base.z = " << offset);
        FCMD_OBJ_CMD(body,"insertObject("<<Gui::Command::getObjectCmd(obj)<<','<<
                Gui::Command::getObjectCmd(sketch)<<")");
        FCMD_OBJ_CMD(sketch,"Support = (" << Gui::Command::getObjectCmd(obj) << ",[''])");
        FCMD_OBJ_CMD(sketch,"MapReversed = " <<  (reverseSketch ? "True" : "False"));
        FCMD_OBJ_CMD(sketch,"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace) << "'");
    }
}

bool isPartDesignAwareObjecta (App::DocumentObject *obj, bool respectGroups = false ) {
    return (obj->isDerivedFrom( PartDesign::Feature::getClassTypeId () ) ||
            PartDesign::Body::isAllowed ( obj ) ||
            obj->isDerivedFrom ( PartDesign::Body::getClassTypeId () ) ||
            ( respectGroups && (
                                obj->hasExtension (App::GeoFeatureGroupExtension::getExtensionClassTypeId () ) ||
                                obj->hasExtension (App::GroupExtension::getExtensionClassTypeId () )
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
        throw Base::RuntimeError ("Couldn't find body for the feature");
    }

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
                    valueStr = Gui::Command::getObjectCmd(body);
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

                if ( !valueStr.empty () && prop->hasName()) {
                    FCMD_OBJ_CMD(obj,prop->getName() << '=' << valueStr);
                }
            }
        }
    }
}

bool isFeatureMovable(App::DocumentObject* const feat)
{
    if (!feat)
        return false;

    if (feat->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        auto prim = static_cast<PartDesign::Feature*>(feat);
        App::DocumentObject* bf = prim->BaseFeature.getValue();
        if (bf)
            return false;
    }

    if (feat->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
        auto prim = static_cast<PartDesign::ProfileBased*>(feat);
        auto sk = prim->getVerifiedSketch(true);

        if (!isFeatureMovable(sk))
            return false;

        if (auto prop = static_cast<App::PropertyLinkList*>(prim->getPropertyByName("Sections"))) {
            if (std::any_of(prop->getValues().begin(), prop->getValues().end(), [](App::DocumentObject* obj){
                return !isFeatureMovable(obj);
            }))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("ReferenceAxis"))) {
            App::DocumentObject* axis = prop->getValue();
            if (axis && !isFeatureMovable(axis))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("Spine"))) {
            App::DocumentObject* spine = prop->getValue();
            if (spine && !isFeatureMovable(spine))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("AuxillerySpine"))) {
            App::DocumentObject* auxSpine = prop->getValue();
            if (auxSpine && !isFeatureMovable(auxSpine))
                return false;
        }

    }

    if (feat->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {
        auto attachable = feat->getExtensionByType<Part::AttachExtension>();
        App::DocumentObject* support = attachable->Support.getValue();
        if (support && !support->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()))
            return false;
    }

    return true;
}

std::vector<App::DocumentObject*> collectMovableDependencies(std::vector<App::DocumentObject*>& features)
{
    std::set<App::DocumentObject*> unique_objs;

    for (auto const &feat : features)
    {

        // Get sketches and datums from profile based features
        if (feat->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
            auto prim = static_cast<PartDesign::ProfileBased*>(feat);
            Part::Part2DObject* sk = prim->getVerifiedSketch(true);
            if (sk) {
                unique_objs.insert(static_cast<App::DocumentObject*>(sk));
            }
            if (auto prop = static_cast<App::PropertyLinkList*>(prim->getPropertyByName("Sections"))) {
                for (App::DocumentObject* obj : prop->getValues()) {
                    unique_objs.insert(obj);
                }
            }
            if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("ReferenceAxis"))) {
                App::DocumentObject* axis = prop->getValue();
                if (axis && !axis->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())){
                    unique_objs.insert(axis);
                }
            }
            if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("Spine"))) {
                App::DocumentObject* axis = prop->getValue();
                if (axis && !axis->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())){
                    unique_objs.insert(axis);
                }
            }
            if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("AuxillerySpine"))) {
                App::DocumentObject* axis = prop->getValue();
                if (axis && !axis->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())){
                    unique_objs.insert(axis);
                }
            }
        }
    }

    std::vector<App::DocumentObject*> result;
    result.reserve(unique_objs.size());
    result.insert(result.begin(), unique_objs.begin(), unique_objs.end());

    return result;
}

void relinkToOrigin(App::DocumentObject* feat, PartDesign::Body* targetbody)
{
    if (feat->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {
        auto attachable = feat->getExtensionByType<Part::AttachExtension>();
        App::DocumentObject* support = attachable->Support.getValue();
        if (support && support->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())) {
            auto originfeat = static_cast<App::OriginFeature*>(support);
            App::OriginFeature* targetOriginFeature = targetbody->getOrigin()->getOriginFeature(originfeat->Role.getValue());
            if (targetOriginFeature) {
                attachable->Support.setValue(static_cast<App::DocumentObject*>(targetOriginFeature), "");
            }
        }
    }
    else if (feat->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
        auto prim = static_cast<PartDesign::ProfileBased*>(feat);
        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("ReferenceAxis"))) {
            App::DocumentObject* axis = prop->getValue();
            if (axis && axis->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId())){
                auto originfeat = static_cast<App::OriginFeature*>(axis);
                App::OriginFeature* targetOriginFeature = targetbody->getOrigin()->getOriginFeature(originfeat->Role.getValue());
                if (targetOriginFeature) {
                    prop->setValue(static_cast<App::DocumentObject*>(targetOriginFeature), std::vector<std::string>(0));
                }
            }
        }
    }
}

} /* PartDesignGui */
