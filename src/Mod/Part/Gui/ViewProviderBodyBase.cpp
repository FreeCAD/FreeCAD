/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)       <vv.titov@gmail.com> *
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
#include <Gui/Command.h>
#include <Gui/ActionFunction.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderOrigin.h>
#include <Gui/ViewProviderOriginFeature.h>

#include <Mod/Part/App/BodyBase.h>

#include "ViewProviderBodyBase.h"
#include <Gui/MDIView.h>

using namespace PartGui;

const char* PartGui::ViewProviderBodyBase::BodyModeEnum[] = {"Through","Tip",NULL};

PROPERTY_SOURCE(PartGui::ViewProviderBodyBase,PartGui::ViewProviderPart)

ViewProviderBodyBase::ViewProviderBodyBase()
{
    ADD_PROPERTY(DisplayModeBody,((long)0));
    DisplayModeBody.setEnums(BodyModeEnum);

    pcBodyChildren = new SoSeparator();
    pcBodyChildren->ref();

    sPixmap = "Part_Body_Tree.svg";
}

ViewProviderBodyBase::~ViewProviderBodyBase()
{
    pcBodyChildren->unref ();
    connectChangedObjectApp.disconnect();
    connectChangedObjectGui.disconnect();
}

void ViewProviderBodyBase::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);

    addDisplayMaskMode(pcBodyChildren, "Through");
    setDisplayMaskMode("Through");

    App::Document *adoc  = pcObject->getDocument ();
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( adoc ) ;

    assert ( adoc );
    assert ( gdoc );

    connectChangedObjectApp = adoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBodyBase::slotChangedObjectApp, this, _1, _2) );

    connectChangedObjectGui = gdoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBodyBase::slotChangedObjectGui, this, _1, _2) );
}

// TODO on activating the body switch to the "Through" mode (2015-09-05, Fat-Zer)
// TODO differnt icon in tree if mode is Through (2015-09-05, Fat-Zer)
// TODO drag&drop (2015-09-05, Fat-Zer)
// TODO Add activate () call (2015-09-08, Fat-Zer)

void ViewProviderBodyBase::setDisplayMode(const char* ModeName) {

    //if we show "Through" we must avoid to set the display mask modes, as this would result
    //in going into "tip" mode. When through is chosen the child features are displayed, and all
    //we need to ensure is that the display mode change is propagated to them fro within the
    //onChanged() method.
    if(DisplayModeBody.getValue() == 1)
        PartGui::ViewProviderPartExt::setDisplayMode(ModeName);
}

void ViewProviderBodyBase::setOverrideMode(const std::string& mode) {

    //if we are in through mode, we need to ensure that the override mode is not set for the body
    //(as this would result in "tip" mode), it is enough when the children are set to the correct
    //override mode.

    if(DisplayModeBody.getValue() != 0)
        Gui::ViewProvider::setOverrideMode(mode);
    else
        overrideMode = mode;
}

void ViewProviderBodyBase::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(tr("Toggle active body"));
    func->trigger(act, boost::bind(&ViewProviderBodyBase::doubleClicked, this));
}

bool ViewProviderBodyBase::doubleClicked(void)
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
        // set correct active objects
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

std::vector<App::DocumentObject*> ViewProviderBodyBase::claimChildren(void)const
{
    Part::BodyBase* body = static_cast<Part::BodyBase*> ( getObject () );
    const std::vector<App::DocumentObject*> &model = body->Model.getValues ();
    std::set<App::DocumentObject*> outSet; //< set of objects not to claim (childrens of childrens)

    // search for objects handled (claimed) by the features
    for( auto obj: model){
        if (!obj) { continue; }
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider ( obj );
        if (!vp) { continue; }

        auto children = vp->claimChildren();
        std::remove_copy ( children.begin (), children.end (), std::inserter (outSet, outSet.begin () ), nullptr);
    }

    // remove the otherwise handled objects, preserving their order so the order in the TreeWidget is correct
    std::vector<App::DocumentObject*> Result;

    if (body->Origin.getValue()) { // Claim for the Origin
        Result.push_back (body->Origin.getValue());
    }

    // claim for rest content not claimed by any other features
    std::remove_copy_if (model.begin(), model.end(), std::back_inserter (Result),
            [outSet] (App::DocumentObject* obj) {
                return outSet.find (obj) != outSet.end();
            } );

    return Result;
}

std::vector<App::DocumentObject*> ViewProviderBodyBase::claimChildren3D(void)const
{
    Part::BodyBase* body = static_cast<Part::BodyBase*>(getObject());

    const std::vector<App::DocumentObject*> & features = body->Model.getValues();

    std::vector<App::DocumentObject*> rv;

    if ( body->Origin.getValue() ) { // Add origin
        rv.push_back (body->Origin.getValue());
    }

    // Add all other stuff
    std::copy (features.begin(), features.end(), std::back_inserter (rv) );

    return rv;
}

bool ViewProviderBodyBase::onDelete ( const std::vector<std::string> &) {
    // TODO May be do it conditionally? (2015-09-05, Fat-Zer)
    Gui::Command::doCommand(Gui::Command::Doc,
            "App.getDocument(\"%s\").getObject(\"%s\").removeModelFromDocument()"
            ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}

void ViewProviderBodyBase::updateData(const App::Property* prop)
{
    Part::BodyBase* body = static_cast<Part::BodyBase*>(getObject());

    if (prop == &body->Model) {
        // update sizes of origins and datums
        updateOriginDatumSize ();
    }

    PartGui::ViewProviderPart::updateData(prop);
}

void ViewProviderBodyBase::slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop ) {

    if (!obj.isDerivedFrom ( Part::Feature::getClassTypeId () ) ||
        obj.isDerivedFrom ( Part::BodyBase::getClassTypeId () )    ) { // we are intrested only in Part::Features and not in bodies
        return;
    }

    const Part::Feature* feat = static_cast <const Part::Feature*>(&obj);

    if ( &feat->Shape != &prop && &feat->Placement != &prop) { // react only on changes in shapes and placement
        return;
    }

    Part::BodyBase* body = static_cast<Part::BodyBase*> ( getObject() );
    if ( body && body->hasFeature (&obj ) ) {
        updateOriginDatumSize ();
    }
}


void ViewProviderBodyBase::slotChangedObjectGui (
        const Gui::ViewProviderDocumentObject& vp, const App::Property& prop )
{
    if (&vp.Visibility != &prop) { // react only on visability changes
        return;
    }

    if ( !vp.isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) &&
         !vp.isDerivedFrom ( Gui::ViewProviderOriginFeature::getClassTypeId () ) ) {
        // Ignore origins to avoid infinite recursion (not likely in a well-formed document,
        //          but may happen in documents designed in old versions of assembly branch )
        return;
    }

    Part::BodyBase* body = static_cast<Part::BodyBase*> ( getObject() );
    App::DocumentObject* obj = vp.getObject ();

    if ( body && obj && body->hasFeature ( obj ) ) {
        updateOriginDatumSize ();
    }
}

void ViewProviderBodyBase::updateOriginDatumSize ()
{
    Part::BodyBase* body = static_cast<Part::BodyBase*> ( getObject() );

    //compute bounding box of contained features (bb_cumu)
    std::vector<App::DocumentObject*> features = body->getFullModel();
    Base::BoundBox3d bb_cumu;
    for(App::DocumentObject* obj: features){
        if (obj && obj->isDerivedFrom(Part::Feature::getClassTypeId())){
            Part::Feature &feat = *(static_cast<Part::Feature*>(obj));
            Base::BoundBox3d bb = feat.Shape.getBoundingBox();
            if(bb.IsValid() && bb.CalcDiagonalLength() < Precision::Infinite()){
                bb_cumu.Add(bb);
            }
        }
    }
    if (! bb_cumu.IsValid() || bb_cumu.CalcDiagonalLength() < Precision::Confusion()){
        bb_cumu.Add(Base::Vector3d(1.0,1.0,1.0));
        bb_cumu.Add(Base::Vector3d(-1.0,-1.0,-1.0));
    }
    assert(bb_cumu.CalcDiagonalLength() >= Precision::Confusion());


    //calculate size of origin.
    Base::Vector3d size;
    size.x = std::max(fabs(bb_cumu.MaxX), fabs(bb_cumu.MinX));
    size.y = std::max(fabs(bb_cumu.MaxY), fabs(bb_cumu.MinY));
    size.z = std::max(fabs(bb_cumu.MaxZ), fabs(bb_cumu.MinZ));
    //make sure size isn't squeezed too much in some direction
    double diag = size.Length();
    if (size.x < diag*0.2)
        size.x = diag*0.2;
    if (size.y < diag*0.2)
        size.y = diag*0.2;
    if (size.z < diag*0.2)
        size.z = diag*0.2;

    //obtain pointer to origin viewprovider
    Gui::ViewProviderOrigin* vpOrigin = 0;
    try {
        App::Origin* origin = body->getOrigin ();
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

    //update it!
    vpOrigin->Size.setValue(size*1.2);
}

void ViewProviderBodyBase::onChanged(const App::Property* prop) {

    if(prop == &DisplayModeBody) {

        if ( DisplayModeBody.getValue() == 0 )  {
            //if we are in an override mode we need to make sure to come out, because
            //otherwise the maskmode is blocked and won't go into "through"
            if(getOverrideMode() != "As Is") {
                auto mode = getOverrideMode();
                ViewProvider::setOverrideMode("As Is");
                overrideMode = mode;
            }
            setDisplayMaskMode("Through");
        }
        else {
            if(getOverrideMode() == "As Is")
                setDisplayMaskMode(DisplayMode.getValueAsString());
            else {
                Base::Console().Message("Set override mode: %s\n", getOverrideMode().c_str());
                setDisplayMaskMode(getOverrideMode().c_str());
            }
        }
    }

    PartGui::ViewProviderPartExt::onChanged(prop);
}

