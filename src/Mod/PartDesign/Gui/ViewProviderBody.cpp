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
# include <boost_bind_bind.hpp>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/SbColor.h>
# include <Precision.hxx>
# include <QMenu>
#endif

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <App/Link.h>
#include <App/DocumentObserver.h>
#include <Gui/ActionFunction.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderOrigin.h>
#include <Gui/ViewProviderOriginFeature.h>
#include <Gui/SoFCUnifiedSelection.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureSolid.h>
#include <Mod/PartDesign/App/FeatureExtrusion.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeatureBase.h>
#include <Mod/PartDesign/App/FeatureWrap.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ViewProviderDatum.h"
#include "Utils.h"

#include "ViewProviderBody.h"
#include "ViewProvider.h"
#include <Gui/Application.h>
#include <Gui/MDIView.h>

using namespace PartDesignGui;
namespace bp = boost::placeholders;

const char* PartDesignGui::ViewProviderBody::BodyModeEnum[] = {"Through","Tip",NULL};

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    PartDesignGui::initMonitor();

    ADD_PROPERTY(DisplayModeBody,((long)0));
    DisplayModeBody.setEnums(BodyModeEnum);

    sPixmap = "PartDesign_Body_Tree.svg";

    MapTransparency.setValue(true);
    MapFaceColor.setValue(true);
    MapLineColor.setValue(true);
    MapPointColor.setValue(true);
    
    Gui::ViewProviderOriginGroupExtension::initExtension(this);
}

ViewProviderBody::~ViewProviderBody()
{
}

void ViewProviderBody::beforeEdit(PartDesignGui::ViewProvider *vp)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return;
    visibleFeatures.clear();
    if (!PartDesign::Body::isSolidFeature(vp->getObject())) {
        vp->Visibility.setValue(true);
        return;
    }
    for (auto obj : body->Group.getValues()) {
        if (!obj->Visibility.getValue()
                || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                || !PartDesign::Body::isSolidFeature(obj))
            continue;
        auto feature = static_cast<PartDesign::Feature*>(obj);
        visibleFeatures.emplace_back(
                App::DocumentObjectT(feature),
                App::DocumentObjectT(feature->BaseFeature.getValue()));
        feature->Visibility.setValue(false);
    }
    vp->Visibility.setValue(true);
}

void ViewProviderBody::afterEdit(PartDesignGui::ViewProvider *)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return;
    for (auto &pair : visibleFeatures) {
        auto obj = pair.first.getObject();
        if (!obj) {
            obj = pair.second.getObject();
            if (!obj)
                continue;
        }
        obj->Visibility.setValue(true);
    }
    visibleFeatures.clear();
}

void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);

    //set default display mode
    onChanged(&DisplayModeBody);

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
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if(!activeDoc)
        activeDoc = getDocument();
    auto activeView = activeDoc->setActiveView(this);
    if(!activeView) 
        return false;

    if (activeView->isActiveObject(getObject(),PDBODYKEY)) {
        //active body double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', None)", PDBODYKEY);
    } else {

        // assure the PartDesign workbench
        if(App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign")->GetBool("SwitchToWB", true))
            Gui::Command::assureWorkbench("PartDesignWorkbench");

        // and set correct active objects
        auto* part = App::Part::getPartOfObject ( getObject() );
        if ( part && part != activeView->getActiveObject<App::Part*> ( PARTKEY ) ) {
            Gui::Command::doCommand(Gui::Command::Gui,
                    "Gui.ActiveDocument.ActiveView.setActiveObject('%s',%s)",
                    PARTKEY, Gui::Command::getObjectCmd(part).c_str());
        }

        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s',%s)",
                PDBODYKEY, Gui::Command::getObjectCmd(getObject()).c_str());
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
    FCMD_OBJ_CMD(getObject(),"removeObjectsFromDocument()");
    return true;
}

unsigned long ViewProviderBody::generateIconColor(App::DocumentObject *feat) const
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || (feat && !PartDesign::Body::isSolidFeature(feat)))
        return 0;

    if (feat) {
        PartDesignGui::ViewProvider *vp = nullptr;
        unsigned long color = 0;
        for (auto o : body->getSiblings(feat)) {
            if (o == feat)
                break;
            vp = Base::freecad_dynamic_cast<PartDesignGui::ViewProvider>(
                    Gui::Application::Instance->getViewProvider(o));
        }
        if (vp) {
            color = vp->IconColor.getValue().getPackedValue();
            if (color)
                return color;
        }
    }

    std::set<unsigned long> colors;
    colors.insert(0x00ff00ff);
    colors.insert(0xff);
    colors.insert(0xffffffff);
    for (auto o : body->Group.getValue()) {
        auto vp = Base::freecad_dynamic_cast<PartDesignGui::ViewProvider>(
                Gui::Application::Instance->getViewProvider(o));
        if (vp)
            colors.insert(vp->IconColor.getValue().getPackedValue() | 0xff);
    }
    std::vector<SbVec3f> hsvs;
    for (unsigned long color : colors) {
        SbColor c;
        float t;
        c.setPackedValue(color, t);
        hsvs.emplace_back();
        c.getHSVValue(hsvs.back()[0], hsvs.back()[1], hsvs.back()[2]);
    }
    float h, s, v;
    for(int retry=0; retry<100; ++retry) {
        h = ((float)rand())/(float(RAND_MAX));
        s = 1.0;
        v = ((float)rand())/(float(RAND_MAX)) * 0.5f + 0.25f;
        bool done = true;
        for (auto hsv : hsvs) {
            if(fabs(h-hsv[0]) + fabs(v-hsv[2]) < 0.2f) {
                done = false;
                break;
            }
        }
        if (done)
            break;
    }
    float t = 0.0f;
    SbColor c;
    c.setHSVValue(h, s, v);
    return c.getPackedValue(t);
}

bool ViewProviderBody::checkSiblings()
{
    // This function is to make sure the sibling self grouping only contain
    // previous siblings that are not already grouped.
    //
    // In addition, it makes sure any non solid feature do not mix with with
    // grouping siblingings.

    if (checkingSiblings)
        return false;

    Base::StateLocker guard(checkingSiblings);

    bool touched = false;
    std::set<App::DocumentObject *> grouped;
    std::vector<App::DocumentObject *> children;
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    bool groupTouched = false;
    auto group = body->Group.getValues();
    for (auto obj : body->Group.getValues()) {
        auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if (!feat || !feat->_Siblings.getSize() || grouped.count(feat))
            continue;

        auto siblings = body->getSiblings(feat, false);
        children.clear();
        children.reserve(siblings.size());
        for (auto sibling : siblings) {
            if (grouped.insert(sibling).second) {
                auto other = Base::freecad_dynamic_cast<PartDesign::Feature>(sibling);
                if (other && other->_Siblings.getSize() == 0)
                    children.insert(children.begin(), sibling);
            }
        }
        if (children.size() != siblings.size()) {
            touched = true;
            feat->_Siblings.setValues(children);
        }
        if (children.size() > 1) {
            int i = -1, j = -1;
            body->Group.find(children.front()->getNameInDocument(), &j);
            body->Group.find(children.back()->getNameInDocument(), &i);
            for (int k=i; k<j; ++k) {
                auto obj = group[k];
                if (!obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
                    group.erase(group.begin() + k);
                    group.insert(group.begin() + i, obj);
                    ++i;
                    groupTouched = true;
                }
            }
        }
    }
    if (groupTouched) {
        body->Group.setValues(group);
        return true;
    }
    if (touched)
        buildExport();
    return touched;
}

void ViewProviderBody::updateData(const App::Property* prop)
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    if (prop == &body->Group) {
        setVisualBodyMode(true);
        checkSiblings();
    } else if (prop == &body->BaseFeature) {
        //ensure all model features are in visual body mode
        setVisualBodyMode(true);
    } else if (prop == &body->Tip) {
        // We changed Tip
        App::DocumentObject* tip = body->Tip.getValue();
        auto features = body->Group.getValues();

        // restore icons
        for (auto feature : features) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(feature);
            if (vp && vp->isDerivedFrom(PartDesignGui::ViewProvider::getClassTypeId())) {
                static_cast<PartDesignGui::ViewProvider*>(vp)->setTipIcon(feature == tip);
            }
        }
    }

    PartGui::ViewProviderPart::updateData(prop);
}


void ViewProviderBody::updateOriginSize () {
    if(App::GetApplication().isRestoring()
            || !getObject()
            || !getObject()->getDocument()
            || getObject()->testStatus(App::ObjectStatus::Remove)
            || getObject()->getDocument()->isPerformingTransaction())
        return;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );

    const auto & model = body->getFullModel ();

    // BBox for Datums is calculated from all visible objects but treating datums as their basepoints only
    SbBox3f bboxDatums = ViewProviderDatum::getRelevantBoundBox ( model );
    // BBox for origin should take into account datums size also
    SbBox3f bboxOrigins(0,0,0,0,0,0);
    bool isDatumEmpty = bboxDatums.isEmpty();
    if(!isDatumEmpty)
        bboxOrigins.extendBy(bboxDatums);

    for(App::DocumentObject* obj : model) {
        if ( obj->isDerivedFrom ( Part::Datum::getClassTypeId () ) ) {
            ViewProvider *vp = Gui::Application::Instance->getViewProvider(obj);
            if (!vp || !vp->isVisible()) { continue; }

            ViewProviderDatum *vpDatum = static_cast <ViewProviderDatum *> (vp) ;

            if(!isDatumEmpty)
                vpDatum->setExtents ( bboxDatums );

            if(App::GroupExtension::getGroupOfObject(obj))
                continue;
            auto bbox = vp->getBoundingBox();
            if(bbox.IsValid())
                bboxOrigins.extendBy ( SbBox3f(bbox.MinX,bbox.MinY,bbox.MinZ,bbox.MaxX,bbox.MaxY,bbox.MaxZ) );
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
    } catch (const Base::Exception &e) {
        e.ReportException();
        return;
    }

    // calculate the desired origin size
    Base::Vector3d size;

    for (uint_fast8_t i=0; i<3; i++) {
        size[i] = std::max ( fabs ( max[i] ), fabs ( min[i] ) );
        if (min[i]>max[i] || size[i] < Precision::Confusion() ) {
            size[i] = Gui::ViewProviderOrigin::defaultSize();
        }
    }

    vpOrigin->Size.setValue ( size );
}

void ViewProviderBody::onChanged(const App::Property* prop) {

    if(prop == &DisplayModeBody) {
        auto body = dynamic_cast<PartDesign::Body*>(getObject());

        if(pcModeSwitch->isOfType(Gui::SoFCSwitch::getClassTypeId())) {
            static_cast<Gui::SoFCSwitch*>(pcModeSwitch)->allowNamedOverride =
                (DisplayModeBody.getValue() != 0);
        }

        if ( DisplayModeBody.getValue() == 0 )  {
            //if we are in an override mode we need to make sure to come out, because
            //otherwise the maskmode is blocked and won't go into "through"
            if(getOverrideMode() != "As Is") {
                auto mode = getOverrideMode();
                ViewProvider::setOverrideMode("As Is");
                overrideMode = mode;
            }
            setDisplayMaskMode("Group");
            if(body)
                body->setShowTip(false);
        }
        else {
            if(body)
                body->setShowTip(true);
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

    if(!pcObject || isRestoring())
        return;

    if(prop == &Visibility ||
       prop == &Selectable ||
       prop == &DisplayModeBody ||
       prop == &MapFaceColor ||
       prop == &MapTransparency ||
       prop == &MapLineColor ||
       prop == &MapPointColor ||
       prop == &ForceMapColors ||
       prop == &MappedColors ||
       prop == &DiffuseColor ||
       prop == &PointColorArray ||
       prop == &LineColorArray)
        return;

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;

    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    // Experiementing element color mapping in PartDesign, enable visual copy on Tip only
#if 1
    auto feature = body->Tip.getValue();
    if (!feature)
        feature = body->BaseFeature.getValue();
    if(feature) {
#else
    auto features = body->Group.getValues();
    for(auto feature : features) {
        if(feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
#endif
        {
            //copy over the properties data
            auto vp = dynamic_cast<PartGui::ViewProviderPart*>(gdoc->getViewProvider(feature));
            if(vp) {
                auto p = vp->getPropertyByName(prop->getName());
                if(p && prop->isDerivedFrom(p->getTypeId())) {
                    if(prop==&ShapeColor) {
                        if(vp->MapFaceColor.getValue())
                            vp->MapFaceColor.setValue(false);
                    }else if(prop == &PointColor) {
                        if(vp->MapPointColor.getValue())
                            vp->MapPointColor.setValue(false);
                    }else if(prop == &LineColor) {
                        if(vp->MapLineColor.getValue())
                            vp->MapLineColor.setValue(false);
                    }else if(prop == &Transparency) {
                        if(vp->MapTransparency.getValue())
                            vp->MapTransparency.setValue(false);
                    }
                    p->Paste(*prop);
                }
            }
        }
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

bool ViewProviderBody::canDropObject(App::DocumentObject* obj) const
{
    if (!PartDesign::Body::isAllowed(obj))
        return false;

    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || PartDesign::Body::findBodyOf(obj) == body)
        return false;

    // App::Part checking is too restrictive. It may mess up things, or it may
    // not. Just let user undo if anything is wrong.

    // App::Part *actPart = PartDesignGui::getActivePart();
    // App::Part* partOfBaseFeature = App::Part::getPartOfObject(obj);
    // if (partOfBaseFeature != 0 && partOfBaseFeature != actPart)
    //     return false;

    return true;
}

bool ViewProviderBody::canDragObject(App::DocumentObject *obj) const
{
    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return false;
    if (body->BaseFeature.getValue() == obj)
        return false;

    return PartDesignGui::isFeatureMovable(obj);
}

std::string ViewProviderBody::dropObjectEx(App::DocumentObject *obj,
                                           App::DocumentObject *owner, 
                                           const char *subname,
                                           const std::vector<std::string> &elements)
{
    PartDesign::Body* body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        FC_THROWM(Base::RuntimeError, "No body");

    auto type = obj->getTypeId();
    if (type.isDerivedFrom(Part::Datum::getClassTypeId())   ||
        type.isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
        type.isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()) ||
        type.isDerivedFrom(PartDesign::SubShapeBinder::getClassTypeId()))
    {
        body->addObject(obj);
    }
    else if (PartDesign::Body::isAllowed(obj) && PartDesignGui::isFeatureMovable(obj)) {
        std::vector<App::DocumentObject*> move;
        move.push_back(obj);
        std::vector<App::DocumentObject*> deps = PartDesignGui::collectMovableDependencies(move);
        move.insert(std::end(move), std::begin(deps), std::end(deps));

        PartDesign::Body* source = PartDesign::Body::findBodyOf(obj);
        if (source == body)
            FC_THROWM(Base::RuntimeError, "Feature already inside the body");
        if(source)
            source->removeObjects(move);
        body->addObjects(move);
    }
    else if (elements.size()) {
        auto binder = static_cast<PartDesign::SubShapeBinder*>(
                body->getDocument()->addObject("PartDesign::SubShapeBinder",
                                               "Binder"));
        std::map<App::DocumentObject *, std::vector<std::string> > links;
        auto & subs = links[owner];
        std::string sub(subname ? subname : "");
        for (auto & element : elements)
            subs.emplace_back(sub + element);
        binder->setLinks(std::move(links));
        body->addObject(binder);
        obj = binder;
    }
    else if (!body->getPrevSolidFeature() && !body->BaseFeature.getValue()) {
        auto binder = static_cast<PartDesign::SubShapeBinder*>(
                body->getDocument()->addObject("PartDesign::SubShapeBinder",
                                                "BaseFeature"));
        std::map<App::DocumentObject *, std::vector<std::string> > links;
        auto & subs = links[owner];
        if (subname && subname[0])
            subs.emplace_back(subname);
        binder->setLinks(std::move(links));
        auto children = body->Group.getValues();
        children.insert(children.begin(), binder);
        body->Group.setValues(children);
        body->BaseFeature.setValue(binder);
        obj = binder;
    }
    return std::string(obj->getNameInDocument()) + ".";
}

bool ViewProviderBody::canReplaceObject(App::DocumentObject *oldObj,
                                        App::DocumentObject *newObj)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || !oldObj || !newObj || oldObj == newObj
            || oldObj == body->BaseFeature.getValue()
            || newObj == body->BaseFeature.getValue())
        return false;

    if (!body->Group.find(oldObj->getNameInDocument())
            || !body->Group.find(newObj->getNameInDocument()))
        return false;

    if (!newObj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        return true;

    return body->isSibling(oldObj, newObj);
}

int ViewProviderBody::replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body || !canReplaceObject(oldObj, newObj))
        return 0;

    int i, j;
    if (!body->Group.find(oldObj->getNameInDocument(), &i)
            || !body->Group.find(newObj->getNameInDocument(), &j))
        return 0;

    auto secondFeat = Base::freecad_dynamic_cast<PartDesign::Feature>(oldObj);
    auto firstFeat = Base::freecad_dynamic_cast<PartDesign::Feature>(newObj);

    // In case the old object has self sibling group, repoint the old object to
    // the earliest sibling
    if (i < j && secondFeat && secondFeat->_Siblings.getSize()) {
        const auto & siblings = secondFeat->_Siblings.getValues();
        for (auto rit=siblings.rbegin(); rit!=siblings.rend(); ++rit) {
            auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(*rit);
            if (feat) {
                if (feat != newObj && body->Group.find(feat->getNameInDocument(), &i))
                    secondFeat = feat;
                break;
            }
        }
    }

    // first, second refers to the order after replaceObject() operation
    if (i > j)
        std::swap(secondFeat, firstFeat);

    App::AutoTransaction committer("Reorder body feature");

    auto objs = body->Group.getValues();
    objs.erase(objs.begin() + j);
    objs.insert(objs.begin() + i, newObj);

    Base::StateLocker guard(checkingSiblings); // delay sibling check
    body->Group.setValues(objs);

    if (firstFeat && secondFeat) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard1(App::Property::User3, &firstFeat->BaseFeature);
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard2(App::Property::User3, &secondFeat->BaseFeature);

        if (secondFeat->NewSolid.getValue()) {
            secondFeat->NewSolid.setValue(false);
            firstFeat->NewSolid.setValue(true);
        }
        firstFeat->BaseFeature.setValue(secondFeat->BaseFeature.getValue());
        secondFeat->BaseFeature.setValue(firstFeat);
        for (auto obj : objs) {
            if (obj == secondFeat
                    || !PartDesign::Body::isSolidFeature(obj)
                    || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
                continue;
            auto feat = static_cast<PartDesign::Feature*>(obj);
            if (feat->BaseFeature.getValue() == firstFeat)
                feat->BaseFeature.setValue(secondFeat);
        }

        if (body->Tip.getValue() == firstFeat)
            body->setTip(secondFeat);

        checkingSiblings = false;

        if (!checkSiblings())
            buildExport();
    }

    Gui::Command::updateActive();
    return 1;
}

std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void) const {
    auto children = inherited::claimChildren3D();
    for(auto it=children.begin();it!=children.end();) {
        auto obj = *it;
        if(obj->isDerivedFrom(PartDesign::Solid::getClassTypeId()))
            it = children.erase(it);
        else
            ++it;
    }
    return children;
}

void ViewProviderBody::groupSiblings(PartDesign::Feature *feat, bool collapse, bool all)
{
    auto body = Base::freecad_dynamic_cast<PartDesign::Body>(getObject());
    if (!body)
        return;

    std::string cmd(collapse ? "Collapse " : "Expand ");
    if (all)
        cmd += "all";
    else
        cmd += "siblings";
    App::AutoTransaction committer(cmd.c_str());
    if (all) {
        auto siblings = body->getSiblings(feat, true, true);
        PartDesign::Feature *feat = nullptr;
        while (siblings.size()) {
            feat = Base::freecad_dynamic_cast<PartDesign::Feature>(siblings.front());
            siblings.pop_front();
            if (feat)
                break;
        }
        if (!feat || siblings.empty())
            return;
        if (collapse) {
            std::vector<App::DocumentObject*> objs(siblings.begin(), siblings.end());
            feat->_Siblings.setValues(objs);
        } else
            feat->_Siblings.setValues();
        for (auto obj : siblings) {
            auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
            if (feat)
                feat->_Siblings.setValues();
        }
        if (!collapse || !checkSiblings())
            buildExport();
        return;
    }

    if (!collapse) {
        feat->_Siblings.setValues();
        buildExport();
        return;
    }

    auto siblings = body->getSiblings(feat, false, true);

    std::vector<App::DocumentObject*> objs;
    objs.reserve(feat->_Siblings.getSize() + siblings.size());

    objs = feat->_Siblings.getValues();
    std::set<App::DocumentObject*> existing(objs.begin(), objs.end());

    bool first = true;
    for (auto obj : siblings) {
        if (existing.count(obj))
            continue;
        auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if (!feat || !feat->_Siblings.getSize()) {
            if (feat)
                first = false;
            objs.push_back(obj);
            continue;
        }
        if (first) {
            // expand the first sibling that has its own group, and join its
            // group to this feature. 
            first = false;
            objs.push_back(obj);
            for (auto o : feat->_Siblings.getValues())
                objs.push_back(o);
            feat->_Siblings.setValues();
            break;
        }
    }
    feat->_Siblings.setValues(objs);
    if (!checkSiblings())
        buildExport();
}
