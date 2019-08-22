/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <boost/bind.hpp>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Precision.hxx>
# include <QMenu>
#endif

#include <Base/Console.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <Gui/ActionFunction.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderOrigin.h>
#include <Gui/ViewProviderOriginFeature.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeatureBase.h>

#include "ViewProviderDatum.h"
#include "Utils.h"

#include "ViewProviderBody.h"
#include "ViewProvider.h"
#include <Gui/Application.h>
#include <Gui/MDIView.h>

using namespace PartDesignGui;

const char* PartDesignGui::ViewProviderBody::BodyModeEnum[] = {"Through","Tip",NULL};

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    ADD_PROPERTY(DisplayModeBody,((long)0));
    DisplayModeBody.setEnums(BodyModeEnum);

    sPixmap = "PartDesign_Body_Tree.svg";

    Gui::ViewProviderOriginGroupExtension::initExtension(this);
}

ViewProviderBody::~ViewProviderBody()
{
    connectChangedObjectApp.disconnect();
    connectChangedObjectGui.disconnect();
}

void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);

    //set default display mode
    onChanged(&DisplayModeBody);

    App::Document *adoc  = pcObject->getDocument ();
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( adoc ) ;

    assert ( adoc );
    assert ( gdoc );

    connectChangedObjectApp = adoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBody::slotChangedObjectApp, this, _1, _2) );

    connectChangedObjectGui = gdoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBody::slotChangedObjectGui, this, _1, _2) );
}

// TODO on activating the body switch to the "Through" mode (2015-09-05, Fat-Zer)
// TODO different icon in tree if mode is Through (2015-09-05, Fat-Zer)
// TODO drag&drop (2015-09-05, Fat-Zer)
// TODO Add activate () call (2015-09-08, Fat-Zer)

void ViewProviderBody::setDisplayMode(const char* ModeName) {

    //if we show "Through" we must avoid to set the display mask modes, as this would result
    //in going into "tip" mode. When through is chosen the child features are displayed, and all
    //we need to ensure is that the display mode change is propagated to them from within the
    //onChanged() method.
    if(DisplayModeBody.getValue() == 1)
        PartGui::ViewProviderPartExt::setDisplayMode(ModeName);
}

void ViewProviderBody::setOverrideMode(const std::string& mode) {

    //if we are in through mode, we need to ensure that the override mode is not set for the body
    //(as this would result in "tip" mode), it is enough when the children are set to the correct
    //override mode.

    if(DisplayModeBody.getValue() != 0)
        Gui::ViewProvider::setOverrideMode(mode);
    else
        overrideMode = mode;
}

void ViewProviderBody::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Q_UNUSED(receiver);
    Q_UNUSED(member);
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(tr("Toggle active body"));
    func->trigger(act, boost::bind(&ViewProviderBody::doubleClicked, this));

    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderBody::doubleClicked(void)
{
    //first, check if the body is already active.
    App::DocumentObject* activeBody = nullptr;
    Gui::MDIView* activeView = this->getActiveView();
    if ( activeView ) {
        activeBody = activeView->getActiveObject<App::DocumentObject*> (PDBODYKEY);
    }

    if (activeBody == this->getObject()){
        //active body double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.getDocument('%s').ActiveView.setActiveObject('%s', None)",
                this->getObject()->getDocument()->getName(),
                PDBODYKEY);
    } else {

        // assure the PartDesign workbench
        Gui::Command::assureWorkbench("PartDesignWorkbench");

        // and set correct active objects
        auto* part = App::Part::getPartOfObject ( getObject() );
        if ( part && part != getActiveView()->getActiveObject<App::Part*> ( PARTKEY ) ) {
            Gui::Command::doCommand(Gui::Command::Gui,
                    "Gui.getDocument('%s').ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                    part->getDocument()->getName(),
                    PARTKEY,
                    part->getDocument()->getName(),
                    part->getNameInDocument());
        }

        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.getDocument('%s').ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                this->getObject()->getDocument()->getName(),
                PDBODYKEY,
                this->getObject()->getDocument()->getName(),
                this->getObject()->getNameInDocument());
    }

    return true;
}


// TODO To be deleted (2015-09-08, Fat-Zer)
//void ViewProviderBody::updateTree()
//{
//    if (ActiveGuiDoc == NULL) return;
//
//    // Highlight active body and all its features
//    //Base::Console().Error("ViewProviderBody::updateTree()\n");
//    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());
//    bool active = body->IsActive.getValue();
//    //Base::Console().Error("Body is %s\n", active ? "active" : "inactive");
//    ActiveGuiDoc->signalHighlightObject(*this, Gui::Blue, active);
//    std::vector<App::DocumentObject*> features = body->Group.getValues();
//    bool highlight = true;
//    App::DocumentObject* tip = body->Tip.getValue();
//    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
//        //Base::Console().Error("Highlighting %s: %s\n", (*f)->getNameInDocument(), highlight ? "true" : "false");
//        Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->getViewProvider(*f));
//        if (vp != NULL)
//            ActiveGuiDoc->signalHighlightObject(*vp, Gui::LightBlue, active ? highlight : false);
//        if (highlight && (tip == *f))
//            highlight = false;
//    }
//}

bool ViewProviderBody::onDelete ( const std::vector<std::string> &) {
    // TODO May be do it conditionally? (2015-09-05, Fat-Zer)
    Gui::Command::doCommand(Gui::Command::Doc,
            "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
            ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}

void ViewProviderBody::updateData(const App::Property* prop)
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    if (prop == &body->Group || prop == &body->BaseFeature) {
        // update sizes of origins and datums
        updateOriginDatumSize ();
        //ensure all model features are in visual body mode
        setVisualBodyMode(true);
    }

    if (prop == &body->Tip) {
        // We changed Tip
        App::DocumentObject* tip = body->Tip.getValue();

        auto features = body->Group.getValues();

        // restore icons
        for (auto feature : features) {
            Gui::ViewProvider* vp = Gui::Application::Instance->activeDocument()->getViewProvider(feature);
            if (vp && vp->isDerivedFrom(PartDesignGui::ViewProvider::getClassTypeId())) {
                static_cast<PartDesignGui::ViewProvider*>(vp)->setTipIcon(feature == tip);
            }
        }
    }

    PartGui::ViewProviderPart::updateData(prop);
}


void ViewProviderBody::slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop ) {

    if (!obj.isDerivedFrom ( Part::Feature::getClassTypeId () ) ||
        obj.isDerivedFrom ( Part::BodyBase::getClassTypeId () )    ) { // we are interested only in Part::Features, not in bodies
        return;
    }

    const Part::Feature *feat = static_cast <const Part::Feature *>(&obj);

    if ( &feat->Shape != &prop && &feat->Placement != &prop) { // react only on changes in shapes and placement
        return;
    }

    PartDesign::Body *body = static_cast<PartDesign::Body*> ( getObject() );
    if ( body && body->hasObject (&obj ) ) {
        updateOriginDatumSize ();
    }
}

void ViewProviderBody::slotChangedObjectGui (
        const Gui::ViewProviderDocumentObject& vp, const App::Property& prop )
{
    if (&vp.Visibility != &prop) { // react only on visibility changes
        return;
    }

    if ( !vp.isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) &&
         !vp.isDerivedFrom ( Gui::ViewProviderOriginFeature::getClassTypeId () ) ) {
        // Ignore origins to avoid infinite recursion (not likely in a well-formed document,
        //          but may happen in documents designed in old versions of assembly branch )
        return;
    }

    PartDesign::Body *body = static_cast<PartDesign::Body*> ( getObject() );
    App::DocumentObject *obj = vp.getObject ();

    if ( body && obj && body->hasObject ( obj ) ) {
        updateOriginDatumSize ();
    }
}

void ViewProviderBody::updateOriginDatumSize () {
    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );

    // Use different bounding boxes for datums and for origins:
    Gui::Document* gdoc = Gui::Application::Instance->getDocument(getObject()->getDocument());
    if(!gdoc)
        return;

    Gui::MDIView* view = gdoc->getViewOfViewProvider(this);
    if(!view)
        return;

    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());

    const auto & model = body->getFullModel ();

    // BBox for Datums is calculated from all visible objects but treating datums as their basepoints only
    SbBox3f bboxDatums = ViewProviderDatum::getRelevantBoundBox ( bboxAction, model );
    // BBox for origin should take into account datums size also
    SbBox3f bboxOrigins = bboxDatums;

    for(App::DocumentObject* obj : model) {
        if ( obj->isDerivedFrom ( Part::Datum::getClassTypeId () ) ) {
            ViewProvider *vp = Gui::Application::Instance->getViewProvider(obj);
            if (!vp) { continue; }

            ViewProviderDatum *vpDatum = static_cast <ViewProviderDatum *> (vp) ;

            vpDatum->setExtents ( bboxDatums );

            bboxAction.apply ( vp->getRoot () );
            bboxOrigins.extendBy ( bboxAction.getBoundingBox () );
        }
    }

    // get the bounding box values
    SbVec3f max = bboxOrigins.getMax();
    SbVec3f min = bboxOrigins.getMin();

    // obtain an Origin and it's ViewProvider
    App::Origin* origin = 0;
    Gui::ViewProviderOrigin* vpOrigin = 0;
    try {
        origin = body->getOrigin ();
        assert (origin);

        Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider(origin);
        if (!vp) {
            throw Base::ValueError ("No view provider linked to the Origin");
        }
        assert ( vp->isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) );
        vpOrigin = static_cast <Gui::ViewProviderOrigin *> ( vp );
    } catch (const Base::Exception &ex) {
        if(!getExtendedViewProvider()->getDocument()->getDocument()->testStatus(App::Document::Restoring))
            Base::Console().Error ("%s\n", ex.what() );
        return;
    }

    // calculate the desired origin size
    Base::Vector3d size;

    for (uint_fast8_t i=0; i<3; i++) {
        size[i] = std::max ( fabs ( max[i] ), fabs ( min[i] ) );
        if (size[i] < Precision::Confusion() ) {
            size[i] = Gui::ViewProviderOrigin::defaultSize();
        }
    }

    vpOrigin->Size.setValue ( size*1.2 );
}

void ViewProviderBody::onChanged(const App::Property* prop) {

    if(prop == &DisplayModeBody) {

        if ( DisplayModeBody.getValue() == 0 )  {
            //if we are in an override mode we need to make sure to come out, because
            //otherwise the maskmode is blocked and won't go into "through"
            if(getOverrideMode() != "As Is") {
                auto mode = getOverrideMode();
                ViewProvider::setOverrideMode("As Is");
                overrideMode = mode;
            }
            setDisplayMaskMode("Group");
        }
        else {
            if(getOverrideMode() == "As Is")
                setDisplayMaskMode(DisplayMode.getValueAsString());
            else {
                Base::Console().Message("Set override mode: %s\n", getOverrideMode().c_str());
                setDisplayMaskMode(getOverrideMode().c_str());
            }
        }

        // #0002559: Body becomes visible upon changing DisplayModeBody
        Visibility.touch();
    }
    else
        unifyVisualProperty(prop);

    PartGui::ViewProviderPartExt::onChanged(prop);
}


void ViewProviderBody::unifyVisualProperty(const App::Property* prop) {

    if(prop == &Visibility ||
       prop == &Selectable ||
       prop == &DisplayModeBody)
        return;

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Group.getValues();
    for(auto feature : features) {

        if(!feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;

        //copy over the properties data
        auto p = gdoc->getViewProvider(feature)->getPropertyByName(prop->getName());
        p->Paste(*prop);
    }
}

void ViewProviderBody::setVisualBodyMode(bool bodymode) {

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Group.getValues();
    for(auto feature : features) {

        if(!feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;

        auto* vp = static_cast<PartDesignGui::ViewProvider*>(gdoc->getViewProvider(feature));
        if (vp) vp->setBodyMode(bodymode);
    }
}

std::vector< std::string > ViewProviderBody::getDisplayModes(void) const {

    //we get all display modes and remove the "Group" mode, as this is what we use for "Through"
    //body display mode
    std::vector< std::string > modes = ViewProviderPart::getDisplayModes();
    modes.erase(modes.begin());
    return modes;
}

bool ViewProviderBody::canDropObjects() const
{
    // if the BaseFeature property is marked as hidden or read-only then
    // it's not allowed to modify it.
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());
    if (body->BaseFeature.testStatus(App::Property::Status::Hidden))
        return false;
    if (body->BaseFeature.testStatus(App::Property::Status::ReadOnly))
        return false;
    return true;
}

bool ViewProviderBody::canDropObject(App::DocumentObject* obj) const
{
    if (!obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
        return false;
    }
    else if (PartDesign::Body::findBodyOf(obj)) {
        return false;
    }
    else if (obj->isDerivedFrom (Part::BodyBase::getClassTypeId())) {
        return false;
    }

    App::Part *actPart = PartDesignGui::getActivePart();
    App::Part* partOfBaseFeature = App::Part::getPartOfObject(obj);
    if (partOfBaseFeature != 0 && partOfBaseFeature != actPart)
        return false;

    return true;
}

void ViewProviderBody::dropObject(App::DocumentObject* obj)
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());
    if (obj->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        body->addObject(obj);
    }
    else if (PartDesign::Body::isAllowed(obj) && PartDesignGui::isFeatureMovable(obj)) {
        std::vector<App::DocumentObject*> move;
        move.push_back(obj);
        std::vector<App::DocumentObject*> deps = PartDesignGui::collectMovableDependencies(move);
        move.insert(std::end(move), std::begin(deps), std::end(deps));

        PartDesign::Body* source = PartDesign::Body::findBodyOf(obj);
        if (source)
            source->removeObjects(move);
        try {
            body->addObjects(move);
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
    }
    else if (body->BaseFeature.getValue() == nullptr) {
        body->BaseFeature.setValue(obj);
    }

    App::Document* doc  = body->getDocument();
    doc->recompute();

    // check if a proxy object has been created for the base feature
    std::vector<App::DocumentObject*> links = body->Group.getValues();
    for (auto it : links) {
        if (it->getTypeId().isDerivedFrom(PartDesign::FeatureBase::getClassTypeId())) {
            PartDesign::FeatureBase* base = static_cast<PartDesign::FeatureBase*>(it);
            if (base && base->BaseFeature.getValue() == obj) {
                Gui::Application::Instance->hideViewProvider(obj);
                break;
            }
        }
    }
}
