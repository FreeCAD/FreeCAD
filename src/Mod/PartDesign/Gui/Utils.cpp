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
#include <QPointer>
#include <QMessageBox>
#include <QCheckBox>
# include <Precision.hxx>
# include <gp_Pln.hxx>
#endif

#include <boost_bind_bind.hpp>

#include <Base/Console.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/ViewProviderPart.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include <Mod/Part/App/PartParams.h>
#include <Mod/Part/Gui/PartParams.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeatureWrap.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ReferenceSelection.h"
#include "Utils.h"
#include "WorkflowManager.h"
#include "ViewProviderBody.h"
#include "TaskWrapParameters.h"
#include "ViewProviderAddSub.h"

namespace bp = boost::placeholders;

FC_LOG_LEVEL_INIT("PartDesignGui",true,true)

//===========================================================================
// Helper for Body
//===========================================================================
using namespace Attacher;

namespace PartDesignGui {

bool setEdit(App::DocumentObject *obj, App::DocumentObject *container, const char *key) {
    if(!obj || !obj->getNameInDocument()) {
        FC_ERR("invalid object");
        return false;
    }
    if(!container) {
        if (std::strcmp(key, PDBODYKEY)==0) {
            container = getBodyFor(obj, false);
            if(!container) 
                return false;
        } else if (std::strcmp(key,PARTKEY)==0) {
            container = getPartFor(obj, false);
            if(!container)
                return false;
        }
    }
    auto *activeView = Gui::Application::Instance->activeView();
    if(!activeView)
        return false;

    App::DocumentObject *parent = 0;
    std::string subname;
    auto active = activeView->getActiveObject<App::DocumentObject*>(key,&parent,&subname);
    if(container && active!=container) {
        parent = obj;
        subname.clear();
    }else{
        subname += obj->getNameInDocument();
        subname += '.';
    }
    _FCMD_OBJ_DOC_CMD(Gui,parent,"setEdit(" << Gui::Command::getObjectCmd(parent) 
            << ",0,'" << subname << "')");
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
        if (assertModern && PartDesignGui::assureModernWorkflow ( activeView->getAppDocument() ) ) {
            activeBody = activeView->getActiveObject<PartDesign::Body*>(PDBODYKEY,topParent,subname);

            if (!activeBody && autoActivate) {
                auto doc = activeView->getAppDocument();
                for (auto & sel : Gui::Selection().getSelectionT(doc->getName(), 0)) {
                    auto objs = sel.getSubObjectList();
                    for (auto it = objs.begin(); it != objs.end(); ++it) {
                        auto linked = (*it)->getLinkedObject(true);
                        if (linked->isDerivedFrom(PartDesign::Body::getClassTypeId())) {
                            App::SubObjectT sobjT(objs.begin(), it+1);
                            Gui::cmdGuiDocument(doc, std::ostringstream()
                                    << "ActiveView.setActiveObject('" << PDBODYKEY << "',"
                                    << sobjT.getObjectPython() << ",'" << sobjT.getSubName() << "')");
                            return activeView->getActiveObject<PartDesign::Body*>(PDBODYKEY,topParent,subname);
                        }
                    }
                }
            }
            if (!activeBody && messageIfNot) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
                    QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                                "Please make one active (double click) or create one.\n\nIf you have a legacy document "
                                "with PartDesign objects without Body, use the migrate function in "
                                "PartDesign to put them into a Body."
                                ));
            }
        }
    }

    return activeBody;
}

void needActiveBodyError(void)
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
    auto body = static_cast<PartDesign::Body*>(
            doc->addObject("PartDesign::Body", "Body"));
    if(body) {
        App::DocumentObject *topParent = nullptr;
        std::string topSubName;
        App::Part *actPart = PartDesignGui::getActivePart (&topParent, &topSubName);
        if (actPart) {
            actPart->addObject(body);
            topSubName += body->getNameInDocument();
            topSubName += ".";
        } else
            topParent = body;
        auto gdoc = Gui::Application::Instance->getDocument(doc);
        if (gdoc && gdoc->getActiveView())
            gdoc->getActiveView()->setActiveObject(topParent, PDBODYKEY, topSubName.c_str());
    }
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

App::Part* getActivePart(App::DocumentObject **topParent, std::string *subname) {
    Gui::MDIView *activeView = Gui::Application::Instance->activeView();
    if ( activeView ) {
        return activeView->getActiveObject<App::Part*> (PARTKEY,topParent,subname);
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
    PartDesign::Body *body = getBodyFor(sketch, /*messageIfNot*/ 0);
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

    App::Plane *plane =0;

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

                if ( !valueStr.empty () ) {
                    FCMD_OBJ_CMD(obj,prop->getName() << '=' << valueStr);
                }
            }
        }
    }
}

bool isFeatureMovable(App::DocumentObject* const feat)
{
    if (feat->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        auto prim = static_cast<PartDesign::Feature*>(feat);
        App::DocumentObject* bf = prim->BaseFeature.getValue();
        if (bf)
            return false;
    }

    if (feat->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
        auto prim = static_cast<PartDesign::ProfileBased*>(feat);
        auto sk = prim->getVerifiedSketch(true);

        if (!isFeatureMovable(static_cast<App::DocumentObject*>(sk)))
            return false;

        if (auto prop = static_cast<App::PropertyLinkList*>(prim->getPropertyByName("Sections"))) {
            if (std::any_of(prop->getValues().begin(), prop->getValues().end(), [](App::DocumentObject* obj){return !isFeatureMovable(obj); }))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("ReferenceAxis"))) {
            App::DocumentObject* axis = prop->getValue();
            if (!isFeatureMovable(static_cast<App::DocumentObject*>(axis)))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("Spine"))) {
            App::DocumentObject* axis = prop->getValue();
            if (!isFeatureMovable(static_cast<App::DocumentObject*>(axis)))
                return false;
        }

        if (auto prop = static_cast<App::PropertyLinkSub*>(prim->getPropertyByName("AuxillerySpine"))) {
            App::DocumentObject* axis = prop->getValue();
            if (!isFeatureMovable(static_cast<App::DocumentObject*>(axis)))
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

PartDesign::Body *queryCommandOverride()
{
    if (Part::PartParams::CommandOverride() == 0)
        return nullptr;

    PartDesign::Body * body = nullptr;
    auto sels = Gui::Selection().getSelection();
    if (sels.empty())
        body = PartDesignGui::getBody(false, false);
    else {
        for (auto & sel : sels) {
            body = PartDesign::Body::findBodyOf(sel.pObject);
            if (body)
                break;
        }
    }
    if (!body || Part::PartParams::CommandOverride() == 1)
        return body;

    QMessageBox box(Gui::getMainWindow());
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("PartDesign Command override"));
    if (sels.empty())
        box.setText(QObject::tr("You are invoking a non-PartDesign command while referecing a "
                                "PartDesign feature.\n\nDo you want to override this command with "
                                "an equivalent PartDesign command?"));
    else
        box.setText(QObject::tr("You are invoking a non-PartDesign command while having an active "
                                "PartDesign body.\n\nDo you want to override this command with an "
                                "equivalent PartDesign command?"));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);
    box.setEscapeButton(QMessageBox::No);

    QCheckBox checkBox(QObject::tr("Remember the choice"));
    checkBox.blockSignals(true);
    box.addButton(&checkBox, QMessageBox::ResetRole); 
    int res = box.exec();
    if (checkBox.isChecked()) {
        QMessageBox::information(Gui::getMainWindow(),
                QObject::tr("PartDesign Command override"),
                QObject::tr("You can change your choice in 'Part design' preference page."));
        Part::PartParams::set_CommandOverride(res == QMessageBox::Yes ? 1 : 0);
    }
    return res == QMessageBox::Yes ? body : nullptr;
}

class Monitor
{
public:
    Monitor()
    {
        editTimer.setSingleShot(true);
        proxy.connect(&editTimer, SIGNAL(timeout()), &proxy, SLOT(onEditTimer()));

        Gui::Application::Instance->signalHighlightObject.connect(
            [this](const Gui::ViewProviderDocumentObject &vp,
                   const Gui::HighlightMode &, 
                   bool set,
                   App::DocumentObject * parent,
                   const char *subname)
            {
                if (!set) {
                    if (activeBody == vp.getObject()) {
                        activeBody = nullptr;
                        connChangedObject.disconnect();
                        connDeletedObject.disconnect();
                        connDeleteDocument.disconnect();
                    }
                }
                else if (vp.isDerivedFrom(ViewProviderBody::getClassTypeId())) {
                    activeBody = Base::freecad_dynamic_cast<PartDesign::Body>(vp.getObject());
                    if (activeBody) {
                        connChangedObject = activeBody->getDocument()->signalChangedObject.connect(
                                boost::bind(&Monitor::slotChangedObject, this, bp::_1, bp::_2));
                        connDeletedObject = activeBody->getDocument()->signalDeletedObject.connect(
                                boost::bind(&Monitor::slotDeletedObject, this, bp::_1));
                        connDeleteDocument = App::GetApplication().signalDeleteDocument.connect(
                                boost::bind(&Monitor::slotDeleteDocument, this, bp::_1));
                        if (parent)
                            activeBodyT = App::SubObjectT(parent, subname);
                        else
                            activeBodyT = App::SubObjectT(activeBody, "");
                    }
                }
            });

        Gui::Application::Instance->signalInEdit.connect(
                boost::bind(&Monitor::slotInEdit, this, bp::_1));

        Gui::Application::Instance->signalResetEdit.connect(
            [this](const Gui::ViewProviderDocumentObject &) {
                resetEdit();
            });

        Gui::Control().signalShowDialog.connect(
                boost::bind(&Monitor::slotShowDialog, this, bp::_1, bp::_2));

        Gui::Control().signalRemoveDialog.connect(
            [this](QWidget *, std::vector<QWidget*> &contents) {
                if (!taskWidget)
                    return;
                for (auto it=contents.begin(); it!=contents.end(); ) {
                    if (*it == taskWidget) {
                        it = contents.erase(it);
                        taskWidget->deleteLater();
                    } else
                        ++it;
                }
            });

    }

    void slotInEdit(const Gui::ViewProviderDocumentObject & vp)
    {
        auto doc = Gui::Application::Instance->editDocument();
        if (!doc)
            return;
        auto view = Base::freecad_dynamic_cast<Gui::View3DInventor>(doc->getEditingView());
        if (!view)
            return;

        Gui::ViewProviderDocumentObject *parentVp = nullptr;
        std::string subname;
        int mode = 0;
        doc->getInEdit(&parentVp, &subname, &mode);
        if (mode != Gui::ViewProvider::Default)
            return;

        App::DocumentObject *parent = parentVp ? parentVp->getObject() : nullptr;

        for(auto obj : vp.getObject()->getInList()) {
            if (obj->isDerivedFrom(PartDesign::FeatureWrap::getClassTypeId())) {
                auto wrap = static_cast<PartDesign::FeatureWrap*>(obj);
                if (wrap->WrapFeature.getValue() != vp.getObject())
                    return;
                if (parent) {
                    App::SubObjectT objT(parent, subname.c_str());
                    auto objs = objT.getSubObjectList();
                    auto it = objs.begin();
                    for (; it!=objs.end(); ++it) {
                        auto linked = (*it)->getLinkedObject(true);
                        if (linked->isDerivedFrom(PartDesign::Body::getClassTypeId()))
                            break;
                    }
                    if (it != objs.end()) {
                        subname = App::SubObjectT(objs.begin(), it+1).getSubName()
                            + wrap->getNameInDocument() + ".";
                    } else
                        parent = nullptr;
                }
                if (!parent && activeBody && activeBody == PartDesign::Body::findBodyOf(wrap)) {
                    parent = activeBodyT.getObject();
                    subname = activeBodyT.getSubName() + wrap->getNameInDocument() + ".";
                }
                break;
            }
        }

        if (!parent)
            return;

        App::SubObjectT sobjT(parent, subname.c_str());
        auto objs = sobjT.getSubObjectList();
        for (auto it=objs.begin(); it!=objs.end(); ++it) {
            auto linked = (*it)->getLinkedObject(true);
            if (linked->isDerivedFrom(PartDesign::Body::getClassTypeId())) {
                connVisibilityChanged = 
                    static_cast<PartDesign::Body*>(linked)->signalSiblingVisibilityChanged.connect(
                        boost::bind(&Monitor::slotVisibilityChanged, this, bp::_1));
                editBodyT = App::SubObjectT(objs.begin(), it+1);
                auto editObj = sobjT.getSubObject();
                editObjT = editObj;
                editView = view;
                editDoc = editBodyT.getDocument();
                if (hasEditCheckBox) {
                    if (editObj && editObj->isDerivedFrom(
                                PartDesign::FeaturePrimitive::getClassTypeId())) {
                        connPrimitiveMoved = static_cast<Part::Feature*>(
                                editObj)->Placement.signalChanged.connect(
                                    [this](const App::Property &) {
                                        if (!this->editPreview)
                                            this->editTimer.start(
                                                    PartGui::PartParams::EditRecomputeWait());
                                    });
                    }
                    if (PartGui::PartParams::PreviewOnEdit()) {
                        auto vp = Base::freecad_dynamic_cast<ViewProviderAddSub>(
                                Gui::Application::Instance->getViewProvider(editObj));
                        if (vp) {
                            editPreview = true;
                            vp->setPreviewDisplayMode(true);
                        }
                    } else if (editObj)
                        editObj->Visibility.setValue(true);
                    if (PartGui::PartParams::EditOnTop())
                        showEditOnTop(true);
                    else {
                        App::DocumentObject *parent = *objs.begin();
                        bool hidden = !parent->Visibility.getValue();
                        if (!hidden) {
                            for (auto it2 = objs.begin(); it2 != it+1; ++ it2) {
                                auto obj = *it2;
                                int vis = parent->isElementVisible(obj->getNameInDocument());
                                if (vis == 0 || (vis<0 && !obj->Visibility.getValue())) {
                                    hidden = true;
                                    break;
                                }
                                parent = obj;
                            }
                        }
                        if (hidden)
                            showEditOnTop(true);
                    }
                }
                break;
            }
        }
    }

    void slotVisibilityChanged(const std::deque<App::DocumentObject*> &siblings)
    {
        if (!PartGui::PartParams::EditOnTop())
            return;
        auto feat = editObjT.getObject();
        if (!feat)
            return;
        for (auto obj : siblings) {
            if (obj == feat) {
                showEditOnTop(true, siblings);
                break;
            }
        }
    }

    void slotShowDialog(QWidget *parent, std::vector<QWidget*> &contents) {
        auto doc = Gui::Application::Instance->editDocument();
        if (!doc)
            return;
        Gui::ViewProviderDocumentObject *parentVp = nullptr;
        std::string subname;
        doc->getInEdit(&parentVp, &subname);
        if (!parentVp)
            return;
        App::SubObjectT sobjT(parentVp->getObject(), subname.c_str());
        auto editObj = sobjT.getSubObject();
        if (!editObj)
            return;
        for (auto obj : editObj->getInList()) {
            if (obj && !obj->isDerivedFrom(PartDesign::FeatureWrap::getClassTypeId()))
                continue;
            auto wrap = static_cast<PartDesign::FeatureWrap*>(obj);
            auto wrapVp = Base::freecad_dynamic_cast<ViewProviderWrap>(
                    Gui::Application::Instance->getViewProvider(wrap));
            if (wrapVp) {
                taskWidget = new TaskWrapParameters(wrapVp, parent);
                contents.insert(contents.begin(), taskWidget);
            }
            break;
        }
    }

    void showEditOnTop(bool enable, const std::deque<App::DocumentObject *> &siblings = {})
    {
        bool viewFound = false;
        auto doc = Gui::Application::Instance->getDocument(editDoc.getDocument());
        if (doc && editView) {
            for (auto view : doc->getViews()) {
                if (view == editView) {
                    viewFound = true;
                    break;
                }
            }
        }
        if (!viewFound)
            return;

        App::SubObjectT objT;
        if (enable) {
            auto obj = editBodyT.getSubObject();
            auto body = Base::freecad_dynamic_cast<PartDesign::Body>(
                    obj ? obj->getLinkedObject(true) : nullptr);
            if (body) {
                for (auto feat : siblings.size() ? siblings : body->getSiblings(editObjT.getObject())) {
                    if (feat->Visibility.getValue()) {
                        std::string subname;
                        subname = editBodyT.getSubName() + feat->getNameInDocument() + ".";
                        objT = App::SubObjectT(editBodyT.getObject(), subname.c_str());
                    }
                }
            }
        }
        if (objT == editOnTopT)
            return;

        if (editOnTopT.getObjectName().size()) {
            editView->getViewer()->checkGroupOnTop(
                    Gui::SelectionChanges(
                        Gui::SelectionChanges::RmvSelection,
                        editOnTopT.getDocumentName().c_str(),
                        editOnTopT.getObjectName().c_str(),
                        editOnTopT.getSubName().c_str()), true);
        }
        editView->getViewer()->checkGroupOnTop(
                Gui::SelectionChanges(
                    Gui::SelectionChanges::AddSelection,
                    objT.getDocumentName().c_str(),
                    objT.getObjectName().c_str(),
                    objT.getSubName().c_str()), true);
        editOnTopT = objT;
    }

    void resetEdit()
    {
        connPrimitiveMoved.disconnect();
        connVisibilityChanged.disconnect();
        auto vp = Base::freecad_dynamic_cast<ViewProviderAddSub>(
                Gui::Application::Instance->getViewProvider(editObjT.getObject()));
        if (vp)
            vp->setPreviewDisplayMode(false);

        for (auto & objs : visibleFeatures) {
            for (auto & objT : objs) {
                auto obj = objT.getObject();
                if (obj) {
                    obj->Visibility.setValue(true);
                    break;
                }
            }
        }
        visibleFeatures.clear();
        showEditOnTop(false);
        editOnTopT = App::SubObjectT();
        editBodyT = App::SubObjectT();
        editView = nullptr;
        hasEditCheckBox = false;
        editPreview = false;
    }

    void slotChangedObject(const App::DocumentObject &object, const App::Property &prop)
    {
        if (Part::PartParams::EnableWrapFeature() == 0)
            return;
        if (!activeBody|| activeBody->getDocument() != object.getDocument()
                       || !prop.isDerivedFrom(App::PropertyLinkBase::getClassTypeId()))
            return;
        auto type = object.getTypeId();
        if (!type.isDerivedFrom(Part::Feature::getClassTypeId())
                || type.isDerivedFrom(PartDesign::Feature::getClassTypeId())
                || type.isDerivedFrom(PartDesign::Body::getClassTypeId())
                || type.isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId())
                || type.isDerivedFrom(PartDesign::SubShapeBinder::getClassTypeId())
                || object.hasExtension(App::LinkBaseExtension::getExtensionClassTypeId()))
            return;
        if (App::GeoFeatureGroupExtension::getGroupOfObject(&object))
            return;

        // The current criteria of triggering wrap feature is quite
        // restrictive. The feature must contain only links that are inside the
        // active body.
        auto link = static_cast<const App::PropertyLinkBase*>(&prop);
        bool found = false;
        for (auto obj : link->linkedObjects()) {
            if (PartDesign::Body::findBodyOf(obj) != activeBody)
                return;
            else
                found = true;
        }
        if (!found)
            return;

        for (auto obj : object.getOutList()) {
            if (PartDesign::Body::findBodyOf(obj) != activeBody)
                return;
        }

        // Check if the object has been wrapped before
        for (auto parent : object.getInList()) {
            if (parent->isDerivedFrom(PartDesign::FeatureWrap::getClassTypeId()))
                return;
        }

        if (Part::PartParams::EnableWrapFeature() > 1) {
            QMessageBox box(Gui::getMainWindow());
            box.setIcon(QMessageBox::Question);
            box.setWindowTitle(QObject::tr("PartDesign feature wrap"));
            box.setText(QObject::tr("You are referencing a PartDesign feature in a non-PartDesign "
                                    "object.\n\nDo you want to incorporate this object into PartDesign "
                                    "body using a wrap feature?"));
            box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            box.setDefaultButton(QMessageBox::Yes);
            box.setEscapeButton(QMessageBox::No);

            QCheckBox checkBox(QObject::tr("Remember the choice"));
            checkBox.blockSignals(true);
            box.addButton(&checkBox, QMessageBox::ResetRole); 
            int res = box.exec();
            if (checkBox.isChecked()) {
                QMessageBox::information(Gui::getMainWindow(),
                        QObject::tr("PartDesign feature wrap"),
                        QObject::tr("You can change your choice in 'Part design' preference page."));
                Part::PartParams::set_EnableWrapFeature(res == QMessageBox::Yes ? 1 : 0);
            }
            if (res != QMessageBox::Yes)
                return;
        }
        try {
            auto wrap = static_cast<PartDesign::FeatureWrap*>(
                    activeBody->newObjectAt("PartDesign::FeatureWrap",
                                            "Wrap",
                                            link->linkedObjects(),
                                            false));
            wrap->Label.setValue(object.Label.getValue());
            wrap->WrapFeature.setValue(const_cast<App::DocumentObject*>(&object));
        } catch (Base::Exception &e) {
            e.ReportException();
        }
    }

    void slotDeletedObject(const App::DocumentObject &obj)
    {
        if (activeBody == &obj)
            disconnect();
    }

    void slotDeleteDocument(const App::Document &doc)
    {
        if (activeBody && activeBody->getDocument() == &doc)
            disconnect();
    }

    void disconnect()
    {
        activeBody = nullptr;
        connChangedObject.disconnect();
        connDeletedObject.disconnect();
        connDeleteDocument.disconnect();
    }
    
    void beforeEdit(App::DocumentObject *editingObj)
    {
        auto body = PartDesign::Body::findBodyOf(editingObj);
        if (!body)
            return;
        visibleFeatures.clear();
        if (!PartDesign::Body::isSolidFeature(editingObj)) {
            editingObj->Visibility.setValue(true);
            return;
        }
        for (auto obj : body->Group.getValues()) {
            if (!obj->Visibility.getValue()
                    || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                    || !PartDesign::Body::isSolidFeature(obj))
                continue;
            visibleFeatures.emplace_back();
            auto & objs = visibleFeatures.back();
            auto siblings = body->getSiblings(obj);
            objs.reserve(siblings.size());
            for (auto it = siblings.begin(); it != siblings.end(); ++it) {
                auto sibling = *it;
                if (!objs.empty())
                    objs.emplace_back(sibling);
                else if (sibling == obj) {
                    for (auto it2 = it; ;--it2) {
                        objs.emplace_back(*it2);
                        if (it2 == siblings.begin())
                            break;
                    }
                }
            }
            if (obj != editingObj)
                obj->Visibility.setValue(false);
        }
        editingObj->Visibility.setValue(true);
    }

public:
    boost::signals2::scoped_connection connChangedObject;
    boost::signals2::scoped_connection connDeletedObject;
    boost::signals2::scoped_connection connDeleteDocument;
    boost::signals2::scoped_connection connVisibilityChanged;
    boost::signals2::scoped_connection connPrimitiveMoved;
    PartDesign::Body *activeBody = nullptr;
    App::SubObjectT activeBodyT;
    App::DocumentObjectT editObjT;
    App::SubObjectT editBodyT;
    App::DocumentT editDoc;
    App::SubObjectT editOnTopT;
    
    Gui::View3DInventor *editView = nullptr;
    QPointer<QWidget> taskWidget;

    MonitorProxy proxy;
    QTimer editTimer;
    bool hasEditCheckBox = false;
    bool editPreview = false;

    std::vector<std::vector<App::DocumentObjectT> > visibleFeatures;
};

static Monitor *_MonitorInstance;
void initMonitor()
{
    if (!_MonitorInstance)
        _MonitorInstance = new Monitor;
}

void beforeEdit(App::DocumentObject *obj)
{
    initMonitor();
    _MonitorInstance->beforeEdit(obj);
}

void MonitorProxy::addCheckBox(QWidget * widget, int index)
{
    QBoxLayout * layout = qobject_cast<QBoxLayout*>(widget->layout());
    if (!layout) {
        FC_WARN("unknonw widget layout");
        return;
    }

    QHBoxLayout * hlayout = new QHBoxLayout;
    auto checkbox = new QCheckBox(widget);
    checkbox->setText(tr("Show preview"));
    checkbox->setToolTip(tr("Show base feature with preview shape"));
    checkbox->setChecked(PartGui::PartParams::PreviewOnEdit());
    connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(onPreview(bool)));
    hlayout->addWidget(checkbox);

    checkbox = new QCheckBox(widget);
    checkbox->setText(tr("Show on top"));
    checkbox->setToolTip(tr("Show the editing feature always on top"));
    checkbox->setChecked(PartGui::PartParams::EditOnTop());
    connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(onShowOnTop(bool)));
    hlayout->addWidget(checkbox);

    layout->insertLayout(index, hlayout);
}

void MonitorProxy::onPreview(bool checked)
{
    PartGui::PartParams::set_PreviewOnEdit(checked);
    if (!_MonitorInstance)
        return;

    auto vp = Base::freecad_dynamic_cast<ViewProviderAddSub>(
            Gui::Application::Instance->getViewProvider(
                _MonitorInstance->editObjT.getObject()));

    if (vp) {
        _MonitorInstance->editPreview = checked;
        vp->setPreviewDisplayMode(checked);
        if (!checked) {
            App::AutoTransaction guard("Recompute");
            vp->getObject()->recomputeFeature(true);
            vp->show();
        }
    }
}

void MonitorProxy::onShowOnTop(bool checked)
{
    PartGui::PartParams::set_EditOnTop(checked);
    if (!_MonitorInstance)
        return;

    _MonitorInstance->showEditOnTop(checked);
}

void MonitorProxy::onEditTimer()
{
    auto obj = _MonitorInstance->editObjT.getObject();
    if (obj) {
        App::AutoTransaction guard("Recompute");
        obj->recomputeFeature(true);
    }
}

void addTaskCheckBox(QWidget * widget, int index)
{
    initMonitor();
    _MonitorInstance->hasEditCheckBox = true;
    _MonitorInstance->proxy.addCheckBox(widget, index);
}

} /* PartDesignGui */

#include "moc_Utils.cpp"
