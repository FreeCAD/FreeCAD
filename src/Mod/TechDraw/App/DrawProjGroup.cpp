/***************************************************************************
 *   Copyright (c) 2013-2014 Luke Parry <l.parry@warwick.ac.uk>            *
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
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
#include <QRectF>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "DrawPage.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroupPy.h"// generated from DrawProjGroupPy.xml
#include "DrawUtil.h"
#include "Preferences.h"


using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"First Angle", "Third Angle",
                                                    "Default",//Use Page setting
                                                    nullptr};

PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup()
{
    static const char* group = "Base";
    static const char* agroup = "Distribute";

    bool autoDist = Preferences::getPreferenceGroup("General")->GetBool("AutoDist", true);

    ADD_PROPERTY_TYPE(Source, (nullptr), group, App::Prop_None, "Shape to view");
    Source.setScope(App::LinkScope::Global);
    Source.setAllowExternal(true);
    ADD_PROPERTY_TYPE(XSource, (nullptr), group, App::Prop_None, "External 3D Shape to view");

    ADD_PROPERTY_TYPE(Anchor, (nullptr), group, App::Prop_None,
                      "The root view to align projections with");
    Anchor.setScope(App::LinkScope::Global);

    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY_TYPE(ProjectionType, ((long)getDefProjConv()), group, App::Prop_None,
                      "First or Third angle projection");

    ADD_PROPERTY_TYPE(AutoDistribute, (autoDist), agroup, App::Prop_None,
                      "Distribute views automatically or manually");
    ADD_PROPERTY_TYPE(spacingX, (15), agroup, App::Prop_None,
                      "If AutoDistribute is on, this is the horizontal \nspacing between the "
                      "borders of views \n(if label width is not wider than the object)");
    ADD_PROPERTY_TYPE(
        spacingY, (15), agroup, App::Prop_None,
        "If AutoDistribute is on, this is the vertical \nspacing between the borders of views");
    Rotation.setStatus(App::Property::Hidden, true);//DPG does not rotate
    Caption.setStatus(App::Property::Hidden, true);
}

//TODO: this duplicates code in DVP
std::vector<App::DocumentObject*> DrawProjGroup::getAllSources() const
{
    //    Base::Console().Message("DPG::getAllSources()\n");
    const std::vector<App::DocumentObject*> links = Source.getValues();
    std::vector<DocumentObject*> xLinks;
    XSource.getLinks(xLinks);
    std::vector<App::DocumentObject*> result = links;
    if (!xLinks.empty()) {
        result.insert(result.end(), xLinks.begin(), xLinks.end());
    }
    return result;
}


void DrawProjGroup::onChanged(const App::Property* prop)
{
    //TODO: For some reason, when the projection type is changed, the isometric views show change appropriately, but the orthographic ones don't... Or vice-versa.  WF: why would you change from 1st to 3rd in mid drawing?
    //if group hasn't been added to page yet, can't scale or distribute projItems
    if (isRestoring() || !getPage()) {
        return TechDraw::DrawViewCollection::onChanged(prop);
    }

    TechDraw::DrawPage* page = getPage();

    if (prop == &Scale) {
        updateChildrenScale();
        recomputeChildren();
        return;
    }

    if (prop == &ProjectionType) {
        updateChildrenEnforce();
        return;
    }

    if (prop == &Source || prop == &XSource) {
        updateChildrenSource();
        return;
    }

    if (prop == &spacingX || prop == &spacingY) {
        updateChildrenEnforce();
        return;
    }

    if (prop == &LockPosition) {
        updateChildrenLock();
        return;
    }

    if (prop == &ScaleType) {
        if (ScaleType.isValue("Page")) {
            double newScale = page->Scale.getValue();
            if (std::abs(getScale() - newScale) > FLT_EPSILON) {
                Scale.setValue(newScale);
                updateChildrenScale();
            }
        }
    }

    //        if ( ScaleType.isValue("Automatic") ||
    //             ScaleType.isValue("Custom") ){
    //            //just documenting that nothing is required here
    //            //DrawView::onChanged will sort out Scale hidden/readonly/etc
    //        }

    if (prop == &Rotation) {
        if (!DrawUtil::fpCompare(Rotation.getValue(), 0.0)) {
            Rotation.setValue(0.0);
            purgeTouched();
        }
        return;
    }

    TechDraw::DrawViewCollection::onChanged(prop);
}

App::DocumentObjectExecReturn* DrawProjGroup::execute()
{
    //    Base::Console().Message("DPG::execute() - %s - waitingForChildren: %d\n",
    //                            getNameInDocument(), waitingForChildren());
    if (!keepUpdated())
        return App::DocumentObject::StdReturn;

    //if group hasn't been added to page yet, can't scale or distribute projItems
    if (!getPage())
        return DrawViewCollection::execute();

    if (!Anchor.getValue())
        //no anchor yet.  nothing to do.
        return DrawViewCollection::execute();

    if (waitingForChildren()) {
        return DrawViewCollection::execute();
    }

    if (ScaleType.isValue("Automatic") && !checkFit()) {
        if (!DrawUtil::fpCompare(getScale(), autoScale(), 0.00001)) {
            Scale.setValue(autoScale());
            //don't bother repositioning children since they will be
            //recomputed at new scale
            overrideKeepUpdated(false);
            return DrawViewCollection::execute();
        }
    }

    if (AutoDistribute.getValue()) {
        autoPositionChildren();
    }
    overrideKeepUpdated(false);
    return DrawViewCollection::execute();
}

short DrawProjGroup::mustExecute() const
{
    if (!isRestoring()) {
        if(
            Views.isTouched() ||
            Source.isTouched() ||
            XSource.isTouched() ||
            Scale.isTouched() ||
            ScaleType.isTouched() ||
            ProjectionType.isTouched() ||
            Anchor.isTouched() ||
            AutoDistribute.isTouched() ||
            LockPosition.isTouched() ||
            spacingX.isTouched() ||
            spacingY.isTouched()
        ) {
            return true;
        }
    }
    return TechDraw::DrawViewCollection::mustExecute();
}

void DrawProjGroup::reportReady()
{
    //    Base::Console().Message("DPG::reportReady - waitingForChildren: %d\n", waitingForChildren());
    if (waitingForChildren()) {
        //not ready yet
        return;
    }
    //all the secondary views are ready so we can now figure out alignment
    if (AutoDistribute.getValue()) {
        recomputeFeature();
    }
}

bool DrawProjGroup::waitingForChildren() const
{
    for (const auto v : Views.getValues()) {
        DrawProjGroupItem* dpgi = static_cast<DrawProjGroupItem*>(v);
        if (dpgi->waitingForHlr() ||//dpgi is still thinking
            dpgi->isTouched()) {    //dpgi needs to execute
            return true;
        }
    }
    return false;
}

TechDraw::DrawPage* DrawProjGroup::getPage() const { return findParentPage(); }

//does the unscaled DPG fit on the page?
bool DrawProjGroup::checkFit() const
{
    //    Base::Console().Message("DPG::checkFit() - %s\n", getNameInDocument());
    if (waitingForChildren()) {
        //assume everything fits since we don't know what size the children are
        return true;
    }
    auto page = findParentPage();
    if (!page)
        throw Base::RuntimeError("No page is assigned to this feature");
    return checkFit(page);
}

bool DrawProjGroup::checkFit(DrawPage* page) const
{
    //    Base::Console().Message("DPG::checkFit(page) - %s\n", getNameInDocument());
    if (waitingForChildren()) {
        return true;
    }

    QRectF bigBox = getRect(false);
    if (bigBox.width() <= page->getPageWidth() && bigBox.height() <= page->getPageHeight()) {
        return true;
    }
    return false;
}

//calculate a scale that fits all views on page
double DrawProjGroup::autoScale() const
{
    //    Base::Console().Message("DPG::autoScale() - %s\n", getNameInDocument());
    auto page = findParentPage();
    if (!page) {
        throw Base::RuntimeError("No page is assigned to this feature");
    }
    return autoScale(page->getPageWidth(), page->getPageHeight());
}

double DrawProjGroup::autoScale(double w, double h) const
{
    //    Base::Console().Message("DPG::autoScale(%.3f, %.3f) - %s\n", w, h, getNameInDocument());
    //get the space used by views + white space at 1:1 scale
    QRectF bigBox = getRect(false);//unscaled box

    double xScale = w / bigBox.width(); // > 1 page bigger than figure
    double yScale = h / bigBox.height();// < 1 page is smaller than figure

    double newScale = std::min(xScale, yScale);
    return DrawUtil::sensibleScale(newScale);
}

//returns the bounding rectangle of all the views in the current scale
QRectF DrawProjGroup::getRect() const { return getRect(true); }

QRectF DrawProjGroup::getRect(bool scaled) const
{
    //    Base::Console().Message("DPG::getRect - views: %d\n", Views.getValues().size());
    std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT> viewPtrs;
    arrangeViewPointers(viewPtrs);
    double totalWidth, totalHeight;
    getViewArea(viewPtrs, totalWidth, totalHeight, scaled);
    double xSpace = spacingX.getValue() * 3.0;
    double ySpace = spacingY.getValue() * 2.0;
    double rectW = totalWidth + xSpace;
    double rectH = totalHeight + ySpace;
    double fudge = 1.2;//make rect a little big to make sure it fits
    rectW *= fudge;
    rectH *= fudge;

    return {0, 0, rectW, rectH};
}

//find area consumed by Views only - scaled or unscaled
void DrawProjGroup::getViewArea(std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT>& viewPtrs,
                                double& width, double& height, bool scaled) const
{
    // Get the child view bounding boxes
    std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes;
    makeViewBbs(viewPtrs, bboxes, scaled);

    //TODO: note that TLF/TRF/BLF, BRF extend a bit farther than a strict row/col arrangement would suggest.
    //get widest view in each row/column
    double col0w =
               std::max(std::max(bboxes[0].LengthX(), bboxes[3].LengthX()), bboxes[7].LengthX()),
           col1w =
               std::max(std::max(bboxes[1].LengthX(), bboxes[4].LengthX()), bboxes[8].LengthX()),
           col2w =
               std::max(std::max(bboxes[2].LengthX(), bboxes[5].LengthX()), bboxes[9].LengthX()),
           col3w = bboxes[6].LengthX(),
           row0h =
               std::max(std::max(bboxes[0].LengthY(), bboxes[1].LengthY()), bboxes[2].LengthY()),
           row1h = std::max(std::max(bboxes[3].LengthY(), bboxes[4].LengthY()),
                            std::max(bboxes[5].LengthY(), bboxes[6].LengthY())),
           row2h =
               std::max(std::max(bboxes[7].LengthY(), bboxes[8].LengthY()), bboxes[9].LengthY());

    width = col0w + col1w + col2w + col3w;
    height = row0h + row1h + row2h;
}

App::DocumentObject* DrawProjGroup::getProjObj(const char* viewProjType) const
{
    for (auto it : Views.getValues()) {
        auto projPtr(dynamic_cast<DrawProjGroupItem*>(it));
        if (!projPtr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error("PROBLEM - DPG::getProjObj - non DPGI entry in Views! %s / %s\n",
                                  getNameInDocument(), viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        else if (strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0) {
            return it;
        }
    }

    return nullptr;
}

DrawProjGroupItem* DrawProjGroup::getProjItem(const char* viewProjType) const
{
    App::DocumentObject* docObj = getProjObj(viewProjType);
    auto result(dynamic_cast<TechDraw::DrawProjGroupItem*>(docObj));
    if (!result && docObj) {
        //should never have a item in DPG that is not a DPGI.
        Base::Console().Error("PROBLEM - DPG::getProjItem finds non-DPGI in Group %s / %s\n",
                              getNameInDocument(), viewProjType);
        throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
    }
    return result;
}

bool DrawProjGroup::checkViewProjType(const char* in)
{
    return (
        strcmp(in, "Front") == 0 ||
        strcmp(in, "Left") == 0 ||
        strcmp(in, "Right") == 0 ||
        strcmp(in, "Top") == 0 ||
        strcmp(in, "Bottom") == 0 ||
        strcmp(in, "Rear") == 0 ||
        strcmp(in, "FrontTopLeft") == 0 ||
        strcmp(in, "FrontTopRight") == 0 ||
        strcmp(in, "FrontBottomLeft") == 0 ||
        strcmp(in, "FrontBottomRight") == 0
    );
}

//********************************
// ProjectionItem A/D/I
//********************************
bool DrawProjGroup::hasProjection(const char* viewProjType) const
{
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<TechDraw::DrawProjGroupItem*>(it));
        if (!view) {
            //should never have a item in DPG that is not a DPGI.
            Base::Console().Error("PROBLEM - DPG::hasProjection finds non-DPGI in Group %s / %s\n",
                                  getNameInDocument(), viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }

        if (strcmp(viewProjType, view->Type.getValueAsString()) == 0) {
            return true;
        }
    }
    return false;
}

bool DrawProjGroup::canDelete(const char* viewProjType) const
{
    //    Base::Console().Message("DPG::canDelete(%s)\n", viewProjType);
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<TechDraw::DrawProjGroupItem*>(it));
        if (!view) {
            //should never have a item in DPG that is not a DPGI.
            Base::Console().Error("PROBLEM - DPG::hasProjection finds non-DPGI in Group %s / %s\n",
                                  getNameInDocument(), viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }

        if (strcmp(viewProjType, view->Type.getValueAsString()) != 0) {
            continue;
        }

        auto linkedItems = view->getInList();
        for (auto& item : linkedItems) {
            if (item == this) {
                continue;
            }
            if (item->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
                return false;
            }
        }
    }

    return true;
}

App::DocumentObject* DrawProjGroup::addProjection(const char* viewProjType)
{
    DrawProjGroupItem* view(nullptr);
    std::pair<Base::Vector3d, Base::Vector3d> vecs;

    DrawPage* dp = findParentPage();
    if (!dp)
        Base::Console().Error("DPG:addProjection - %s - DPG is not on a page!\n",
                              getNameInDocument());

    if (checkViewProjType(viewProjType) && !hasProjection(viewProjType)) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        auto docObj(getDocument()->addObject("TechDraw::DrawProjGroupItem",//add to Document
                                             FeatName.c_str()));
        view = dynamic_cast<TechDraw::DrawProjGroupItem*>(docObj);
        if (!view && docObj) {
            //should never happen that we create a DPGI that isn't a DPGI!!
            Base::Console().Error("PROBLEM - DPG::addProjection - created a non DPGI! %s / %s\n",
                                  getNameInDocument(), viewProjType);
            throw Base::TypeError("Error: new projection is not a DPGI!");
        }
        if (view) {//coverity CID 151722
            // the label must be set before the view is added
            view->Label.setValue(viewProjType);
            // somewhere deep in DocumentObject, duplicate Labels have a numeric suffix applied,
            // so we need to wait until that happens before building the translated label
            view->translateLabel("DrawProjGroupItem", viewProjType, view->Label.getValue());
            addView(view);//from DrawViewCollection
            view->Source.setValues(Source.getValues());
            view->XSource.setValues(XSource.getValues());

            // the Scale is already set by DrawView
            view->Type.setValue(viewProjType);
            if (strcmp(viewProjType, "Front") != 0) {//not Front!
                vecs = getDirsFromFront(view);
                view->Direction.setValue(vecs.first);
                view->XDirection.setValue(vecs.second);
                view->recomputeFeature();
            }
            else {//Front
                Anchor.setValue(view);
                Anchor.purgeTouched();
                requestPaint();//make sure the group object is on the Gui page
                view->LockPosition.setValue(
                    true);//lock "Front" position within DPG (note not Page!).
                view->LockPosition.setStatus(App::Property::ReadOnly,
                                             true);//Front should stay locked.
                view->LockPosition.purgeTouched();
            }
            //        addView(view);                            //from DrawViewCollection
            //        if (view != getAnchor()) {                //anchor is done elsewhere
            //            view->recomputeFeature();
            //        }
        }
    }
    return view;
}

//NOTE: projections can be deleted without using removeProjection - ie regular DocObject deletion process.
int DrawProjGroup::removeProjection(const char* viewProjType)
{
    // TODO: shouldn't be able to delete "Front" unless deleting whole group
    if (checkViewProjType(viewProjType)) {
        if (!hasProjection(viewProjType)) {
            throw Base::RuntimeError("The projection doesn't exist in the group");
        }

        // Iterate through the child views and find the projection type
        for (auto it : Views.getValues()) {
            auto projPtr(dynamic_cast<TechDraw::DrawProjGroupItem*>(it));
            if (projPtr) {
                if (strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0) {
                    removeView(projPtr);                                 // Remove from collection
                    getDocument()->removeObject(it->getNameInDocument());// Remove from the document
                    return Views.getValues().size();
                }
            }
            else {
                //if an element in Views is not a DPGI, something really bad has happened somewhere
                Base::Console().Error(
                    "PROBLEM - DPG::removeProjection - tries to remove non DPGI! %s / %s\n",
                    getNameInDocument(), viewProjType);
                throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
            }
        }
    }

    return -1;
}

//removes all DPGI - used when deleting DPG
int DrawProjGroup::purgeProjections()
{
    while (!Views.getValues().empty()) {
        std::vector<DocumentObject*> views = Views.getValues();
        DrawProjGroupItem* dpgi;
        DocumentObject* dObj = views.back();
        dpgi = dynamic_cast<DrawProjGroupItem*>(dObj);
        if (dpgi) {
            std::string itemName = dpgi->Type.getValueAsString();
            removeProjection(itemName.c_str());
        }
        else {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error("PROBLEM - DPG::purgeProjection - tries to remove non DPGI! %s\n",
                                  getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
    }
    auto page = findParentPage();
    if (page) {
        page->requestPaint();
    }

    return Views.getValues().size();
}

std::pair<Base::Vector3d, Base::Vector3d> DrawProjGroup::getDirsFromFront(DrawProjGroupItem* view)
{
    std::pair<Base::Vector3d, Base::Vector3d> result;
    std::string viewType = view->Type.getValueAsString();
    return getDirsFromFront(viewType);
}

std::pair<Base::Vector3d, Base::Vector3d> DrawProjGroup::getDirsFromFront(std::string viewType)
{
    //    Base::Console().Message("DPG::getDirsFromFront(%s)\n", viewType.c_str());
    std::pair<Base::Vector3d, Base::Vector3d> result;

    Base::Vector3d projDir, rotVec;
    DrawProjGroupItem* anch = getAnchor();
    if (!anch) {
        Base::Console().Warning("DPG::getDirsFromFront - %s - No Anchor!\n", Label.getValue());
        throw Base::RuntimeError("Project Group missing Anchor projection item");
    }

    Base::Vector3d org(0.0, 0.0, 0.0);
    gp_Ax2 anchorCS = anch->getProjectionCS(org);
    gp_Pnt gOrg(0.0, 0.0, 0.0);
    gp_Dir gDir = anchorCS.Direction();
    gp_Dir gXDir = anchorCS.XDirection();
    gp_Dir gYDir = anchorCS.YDirection();
    gp_Ax1 gUpAxis(gOrg, gYDir);
    gp_Ax2 newCS;
    gp_Dir gNewDir;
    gp_Dir gNewXDir;

    double angle = M_PI / 2.0;//90*

    if (viewType == "Right") {
        newCS = anchorCS.Rotated(gUpAxis, angle);
        projDir = dir2vec(newCS.Direction());
        rotVec = dir2vec(newCS.XDirection());
    }
    else if (viewType == "Left") {
        newCS = anchorCS.Rotated(gUpAxis, -angle);
        projDir = dir2vec(newCS.Direction());
        rotVec = dir2vec(newCS.XDirection());
    }
    else if (viewType == "Top") {
        projDir = dir2vec(gYDir);
        rotVec = dir2vec(gXDir);
    }
    else if (viewType == "Bottom") {
        projDir = dir2vec(gYDir.Reversed());
        rotVec = dir2vec(gXDir);
    }
    else if (viewType == "Rear") {
        projDir = dir2vec(gDir.Reversed());
        rotVec = dir2vec(gXDir.Reversed());
    }
    else if (viewType == "FrontTopLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) - gp_Vec(gXDir) + gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) + gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == "FrontTopRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) + gp_Vec(gXDir) + gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) - gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == "FrontBottomLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) - gp_Vec(gXDir) - gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) + gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    }
    else if (viewType == "FrontBottomRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) + gp_Vec(gXDir) - gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) - gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else {
        // not one of the standard view directions, so complain and use the values for "Front"
        Base::Console().Error("DrawProjGroup - %s unknown projection: %s\n", getNameInDocument(),
                            viewType.c_str());
        Base::Vector3d dirAnch = anch->Direction.getValue();
        Base::Vector3d rotAnch = anch->getXDirection();
        return std::make_pair(dirAnch, rotAnch);
    }

    return std::make_pair(projDir, rotVec);
}

Base::Vector3d DrawProjGroup::dir2vec(gp_Dir d)
{
    return Base::Vector3d(d.X(), d.Y(), d.Z());
}

gp_Dir DrawProjGroup::vec2dir(Base::Vector3d v)
{
    return gp_Dir(v.x, v.y, v.z);
}

//this can be improved.  this implementation positions views too far apart.
Base::Vector3d DrawProjGroup::getXYPosition(const char* viewTypeCStr)
{
    //    Base::Console().Message("DPG::getXYPosition(%s)\n", Label.getValue());
    //   Third Angle:  FTL  T  FTRight          0  1  2
    //                  L   F   Right   Rear    3  4  5  6
    //                 FBL  B  FBRight          7  8  9
    //
    //   First Angle:  FBRight  B  FBL          0  1  2
    //                  Right   F   L  Rear     3  4  5  6
    //                 FTRight  T  FTL          7  8  9

    //Front view position is always (0, 0)
    if (strcmp(viewTypeCStr, "Front") == 0) {// Front!
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
    const int idxCount = MAXPROJECTIONCOUNT;
    std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT> viewPtrs;
    arrangeViewPointers(viewPtrs);
    int viewIndex = getViewIndex(viewTypeCStr);

    //TODO: bounding boxes do not take view orientation into account
    //      i.e. X&Y widths might be swapped on page

    if (viewPtrs[viewIndex]->LockPosition.getValue() || !AutoDistribute.getValue()) {
        return Base::Vector3d(
            viewPtrs[viewIndex]->X.getValue(),
            viewPtrs[viewIndex]->Y.getValue(),
            0.0
        );
    }

    std::vector<Base::Vector3d> position(idxCount);

    // Calculate bounding boxes for each displayed view
    std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes;
    makeViewBbs(viewPtrs, bboxes);//scaled

    double xSpacing = spacingX.getValue();//in mm, no scale
    double ySpacing = spacingY.getValue();//in mm, no scale

    std::array<int, 3> topRowBoxes {0, 1, 2};
    std::array<int, 3> middleRowBoxes {3, 4, 5};
    std::array<int, 3> bottomRowBoxes {7, 8, 9};
    std::array<int, 3> leftColBoxes {0, 3, 7};
    std::array<int, 3> middleColBoxes {1, 4, 8};
    std::array<int, 3> rightColBoxes {2, 5, 9};
    double bigHeightTop = getMaxRowHeight(topRowBoxes, bboxes);
    double bigHeightMiddle = getMaxRowHeight(middleRowBoxes, bboxes);
    double bigHeightBottom = getMaxRowHeight(bottomRowBoxes, bboxes);
    double bigWidthLeft = getMaxColWidth(leftColBoxes, bboxes);
    double bigWidthMiddle = getMaxColWidth(middleColBoxes, bboxes);
    double bigWidthRight = getMaxColWidth(rightColBoxes, bboxes);
    double bigWidthFarRight = 0.0;
    if (bboxes[6].IsValid()) {
        bigWidthFarRight = bboxes[6].LengthX();
    }


    if (viewPtrs[4] &&//Front
        bboxes[4].IsValid()) {
        position[4].x = 0.0;
        position[4].y = 0.0;
    }

    if (viewPtrs[3] &&// L/R  (third/first) middle/left
        bboxes[3].IsValid() && bboxes[4].IsValid()) {
        position[3].x = -(0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthLeft);
        position[3].y = 0.0;
    }

    if (viewPtrs[5] &&// R/L (third/first) middle/right
        bboxes[5].IsValid() && bboxes[4].IsValid()) {
        position[5].x = 0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthRight;
        position[5].y = 0.0;
    }

    if (viewPtrs[6] && bboxes[6].IsValid()) {//"Rear"  middle/far right
        if (viewPtrs[5] && bboxes[5].IsValid()) {
            //there is a view between Front and Rear
            position[6].x = 0.5 * bigWidthMiddle + xSpacing + bigWidthRight + xSpacing
                + 0.5 * bigWidthFarRight;
            position[6].y = 0.0;
        }
        else if (viewPtrs[4] && bboxes[4].IsValid()) {
            // there is no view between Front and Rear
            position[6].x = 0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthRight;
            position[6].y = 0.0;
        }
    }

    if (viewPtrs[1] &&// T/B (third/first) top/middle
        bboxes[1].IsValid() && bboxes[4].IsValid()) {
        position[1].x = 0.0;
        position[1].y = 0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightTop;
    }

    if (viewPtrs[8] &&// B/T (third/first) bottom/middle
        bboxes[8].IsValid() && bboxes[4].IsValid()) {
        position[8].x = 0.0;
        position[8].y = -(0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightBottom);
    }

    if (viewPtrs[0] &&// iso top left
        bboxes[0].IsValid()) {
        position[0].x = -(0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthLeft);
        position[0].y = 0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightTop;
    }

    if (viewPtrs[2] &&// iso top right
        bboxes[2].IsValid()) {
        position[2].x = 0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthRight;
        position[2].y = 0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightTop;
    }

    if (viewPtrs[7] &&// iso bottom left
        bboxes[7].IsValid()) {
        position[7].x = -(0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthLeft);
        position[7].y = -(0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightBottom);
    }

    if (viewPtrs[9] &&// iso bottom right
        bboxes[9].IsValid()) {
        position[9].x = 0.5 * bigWidthMiddle + xSpacing + 0.5 * bigWidthRight;
        position[9].y = -(0.5 * bigHeightMiddle + ySpacing + 0.5 * bigHeightBottom);
    }

    return Base::Vector3d(
        position[viewIndex].x,
        position[viewIndex].y,
        0.0
    );
}

double DrawProjGroup::getMaxRowHeight(std::array<int, 3> list,
                                      std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes)
{
    double bigHeight = 0.0;
    for (auto index : list) {
        if (!bboxes.at(index).IsValid()) {
            continue;
        }
        bigHeight = std::max(bigHeight, bboxes.at(index).LengthY());
    }
    return bigHeight;
}

double DrawProjGroup::getMaxColWidth(std::array<int, 3> list,
                                     std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes)
{
    double bigWidth = 0.0;
    for (auto index : list) {
        if (!bboxes.at(index).IsValid()) {
            continue;
        }
        bigWidth = std::max(bigWidth, bboxes.at(index).LengthX());
    }
    return bigWidth;
}

int DrawProjGroup::getViewIndex(const char* viewTypeCStr) const
{
    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    DrawPage* dp = findParentPage();
    if (ProjectionType.isValue("Default")) {
        if (dp) {
            projType = dp->ProjectionType.getValueAsString();
        }
        else {
            Base::Console().Warning(
                "DPG: %s - can not find parent page. Using default Projection Type. (1)\n",
                getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv];
        }
    }
    else {
        projType = ProjectionType.getValueAsString();
    }

    if (strcmp(projType, "Third Angle") != 0 && strcmp(projType, "First Angle") != 0) {
        throw Base::ValueError("Unknown Projection convention in DrawProjGroup::getViewIndex()");
    }

    //   Third Angle:  FTL  T  FTRight          0  1  2
    //                  L   F   Right   Rear    3  4  5  6
    //                 FBL  B  FBRight          7  8  9
    //
    //   First Angle:  FBRight  B  FBL          0  1  2
    //                  Right   F   L  Rear     3  4  5  6
    //                 FTRight  T  FTL          7  8  9

    bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
    if (strcmp(viewTypeCStr, "Front") == 0) {
        return 4;
    }
    else if (strcmp(viewTypeCStr, "Left") == 0) {
        return thirdAngle ? 3 : 5;
    }
    else if (strcmp(viewTypeCStr, "Right") == 0) {
        return thirdAngle ? 5 : 3;
    }
    else if (strcmp(viewTypeCStr, "Top") == 0) {
        return thirdAngle ? 1 : 8;
    }
    else if (strcmp(viewTypeCStr, "Bottom") == 0) {
        return thirdAngle ? 8 : 1;
    }
    else if (strcmp(viewTypeCStr, "Rear") == 0) {
        return 6;
    }
    else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
        return thirdAngle ? 0 : 9;
    }
    else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
        return thirdAngle ? 2 : 7;
    }
    else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
        return thirdAngle ? 7 : 2;
    }
    else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
        return thirdAngle ? 9 : 0;
    }

    throw Base::TypeError("Unknown view type in DrawProjGroup::getViewIndex()");
    return 4;  // Default to front view's position;
}

void DrawProjGroup::arrangeViewPointers(
    std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT>& viewPtrs) const
{
    for (int i = 0; i < MAXPROJECTIONCOUNT; ++i) {
        viewPtrs[i] = nullptr;
    }

    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    if (ProjectionType.isValue("Default")) {
        DrawPage* dp = findParentPage();
        if (dp) {
            projType = dp->ProjectionType.getValueAsString();
        }
        else {
            Base::Console().Error("DPG:arrangeViewPointers - %s - DPG is not on a page!\n",
                                  getNameInDocument());
            Base::Console().Warning(
                "DPG:arrangeViewPointers - using system default Projection Type\n",
                getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv + 1];
        }
    }
    else {
        projType = ProjectionType.getValueAsString();
    }

    // Iterate through views and populate viewPtrs
    if (strcmp(projType, "Third Angle") != 0 && strcmp(projType, "First Angle") != 0) {
        Base::Console().Warning("DPG: %s - unknown Projection convention: %s\n",
                                getNameInDocument(), projType);
        throw Base::ValueError(
            "Unknown Projection convention in DrawProjGroup::arrangeViewPointers");
    }

    //   Third Angle:  FTL  T  FTRight          0  1  2
    //                  L   F   Right   Rear    3  4  5  6
    //                 FBL  B  FBRight          7  8  9
    //
    //   First Angle:  FBRight  B  FBL          0  1  2
    //                  Right   F   L  Rear     3  4  5  6
    //                 FTRight  T  FTL          7  8  9

    bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
    for (auto it : Views.getValues()) {
        auto oView(dynamic_cast<DrawProjGroupItem*>(it));
        if (!oView) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error(
                "PROBLEM - DPG::arrangeViewPointers - non DPGI in Views! %s\n",
                getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        else {
            const char* viewTypeCStr = oView->Type.getValueAsString();
            if (strcmp(viewTypeCStr, "Front") == 0) {
                //viewPtrs[thirdAngle ? 4 : 4] = oView;
                viewPtrs[4] = oView;
            }
            else if (strcmp(viewTypeCStr, "Left") == 0) {
                viewPtrs[thirdAngle ? 3 : 5] = oView;
            }
            else if (strcmp(viewTypeCStr, "Right") == 0) {
                viewPtrs[thirdAngle ? 5 : 3] = oView;
            }
            else if (strcmp(viewTypeCStr, "Top") == 0) {
                viewPtrs[thirdAngle ? 1 : 8] = oView;
            }
            else if (strcmp(viewTypeCStr, "Bottom") == 0) {
                viewPtrs[thirdAngle ? 8 : 1] = oView;
            }
            else if (strcmp(viewTypeCStr, "Rear") == 0) {
                viewPtrs[6] = oView;
            }
            else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
                viewPtrs[thirdAngle ? 0 : 9] = oView;
            }
            else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
                viewPtrs[thirdAngle ? 2 : 7] = oView;
            }
            else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
                viewPtrs[thirdAngle ? 7 : 2] = oView;
            }
            else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
                viewPtrs[thirdAngle ? 9 : 0] = oView;
            }
            else {
                Base::Console().Warning("DPG: %s - unknown view type: %s. \n",
                                        getNameInDocument(), viewTypeCStr);
                throw Base::TypeError(
                    "Unknown view type in DrawProjGroup::arrangeViewPointers.");
            }
        }
    }
}

void DrawProjGroup::makeViewBbs(std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT>& viewPtrs,
                                std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT>& bboxes,
                                bool scaled) const
{
    Base::BoundBox3d empty(Base::Vector3d(0.0, 0.0, 0.0), 0.0);
    for (int i = 0; i < MAXPROJECTIONCOUNT; ++i) {
        bboxes[i] = empty;
        if (!viewPtrs[i]) {
            continue;
        }

        bboxes[i] = viewPtrs[i]->getBoundingBox();
        if (scaled) {
            continue;
        }
        double scale = 1.0 / viewPtrs[i]->getScale();//convert bbx to 1:1 scale
        //                double scale = 1.0 / viewPtrs[i]->getLastScale();    //convert bbx to 1:1 scale
        bboxes[i].ScaleX(scale);
        bboxes[i].ScaleY(scale);
        bboxes[i].ScaleZ(scale);
    }
}

void DrawProjGroup::recomputeChildren()
{
    //    Base::Console().Message("DPG::recomputeChildren() - waiting: %d\n", waitingForChildren());
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        else {
            view->recomputeFeature();
        }
    }
}

void DrawProjGroup::autoPositionChildren()
{
    //    Base::Console().Message("DPG::autoPositionChildren() - %s - waiting: %d\n",
    //                            getNameInDocument(), waitingForChildren());
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        else {
            view->autoPosition();
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Scale
 */
void DrawProjGroup::updateChildrenScale()
{
    //    Base::Console().Message("DPG::updateChildrenScale() - waiting: %d\n", waitingForChildren());
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }

        view->Scale.setValue(getScale());
        view->Scale.purgeTouched();
        view->purgeTouched();
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Source
 */
void DrawProjGroup::updateChildrenSource()
{
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error(
                "PROBLEM - DPG::updateChildrenSource - non DPGI entry in Views! %s\n",
                getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        if (view->Source.getValues() != Source.getValues()) {
            view->Source.setValues(Source.getValues());
        }
        if (view->XSource.getValues() != XSource.getValues()) {
            view->XSource.setValues(XSource.getValues());
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed LockPosition
 * (really for benefit of QGIV on Gui side)
 */
void DrawProjGroup::updateChildrenLock()
{
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error(
                "PROBLEM - DPG::updateChildrenLock - non DPGI entry in Views! %s\n",
                getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        view->requestPaint();
    }
}

void DrawProjGroup::updateChildrenEnforce(void)
{
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem*>(it));
        if (!view) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Error(
                "PROBLEM - DPG::updateChildrenEnforce - non DPGI entry in Views! %s\n",
                getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        view->enforceRecompute();
    }
}

App::Enumeration DrawProjGroup::usedProjectionType()
{
    //TODO: Would've been nice to have an Enumeration(const PropertyEnumeration &) constructor
    App::Enumeration ret(ProjectionTypeEnums, ProjectionType.getValueAsString());
    if (ret.isValue("Default")) {
        TechDraw::DrawPage* page = getPage();
        if (page) {
            ret.setValue(page->ProjectionType.getValueAsString());
        }
    }
    return ret;
}

bool DrawProjGroup::hasAnchor()
{
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj) {
        return true;
    }
    return false;
}

TechDraw::DrawProjGroupItem* DrawProjGroup::getAnchor()
{
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj) {
        DrawProjGroupItem* result = static_cast<DrawProjGroupItem*>(docObj);
        return result;
    }
    return nullptr;
}

void DrawProjGroup::setAnchorDirection(const Base::Vector3d dir)
{
    App::DocumentObject* docObj = Anchor.getValue();
    DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
    item->Direction.setValue(dir);
}

Base::Vector3d DrawProjGroup::getAnchorDirection()
{
    App::DocumentObject* docObj = Anchor.getValue();
    if (!docObj) {
        return Base::Vector3d();
    }
    DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
    return item->Direction.getValue();
}

//*************************************
//* view direction manipulation routines
//*************************************

//note: must calculate all the new directions before applying any of them or
// the process will get confused.
void DrawProjGroup::updateSecondaryDirs()
{
    DrawProjGroupItem* anchor = getAnchor();
    Base::Vector3d anchDir = anchor->Direction.getValue();
    Base::Vector3d anchRot = anchor->getXDirection();

    std::map<std::string, std::pair<Base::Vector3d, Base::Vector3d>> saveVals;
    std::string key;
    std::pair<Base::Vector3d, Base::Vector3d> data;
    for (auto& docObj : Views.getValues()) {
        std::pair<Base::Vector3d, Base::Vector3d> newDirs;
        std::string pic;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        ProjItemType t = static_cast<ProjItemType>(v->Type.getValue());
        switch (t) {
            case Front:
                data.first = anchDir;
                data.second = anchRot;
                key = "Front";
                saveVals[key] = data;
                break;
            case Rear:
                key = "Rear";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Left:
                key = "Left";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Right:
                key = "Right";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Top:
                key = "Top";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Bottom:
                key = "Bottom";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopLeft:
                key = "FrontTopLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopRight:
                key = "FrontTopRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomLeft:
                key = "FrontBottomLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomRight:
                key = "FrontBottomRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            default: {
                //TARFU invalid secondary type
                Base::Console().Message(
                    "ERROR - DPG::updateSecondaryDirs - invalid projection type\n");
            }
        }
    }

    for (auto& docObj : Views.getValues()) {
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        std::string type = v->Type.getValueAsString();
        data = saveVals[type];
        v->Direction.setValue(data.first);
        v->Direction.purgeTouched();
        v->XDirection.setValue(data.second);
        v->XDirection.purgeTouched();
    }
    recomputeChildren();
}

void DrawProjGroup::rotate(const std::string& rotationdirection)
{
    std::pair<Base::Vector3d, Base::Vector3d> newDirs;
    if (rotationdirection == "Right")
        newDirs = getDirsFromFront("Left");// Front -> Right -> Rear -> Left -> Front
    else if (rotationdirection == "Left")
        newDirs = getDirsFromFront("Right");// Front -> Left -> Rear -> Right -> Front
    else if (rotationdirection == "Up")
        newDirs = getDirsFromFront("Bottom");// Front -> Top -> Rear -> Bottom -> Front
    else if (rotationdirection == "Down")
        newDirs = getDirsFromFront("Top");// Front -> Bottom -> Rear -> Top -> Front

    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);

    updateSecondaryDirs();
}

void DrawProjGroup::spin(const std::string& spindirection)
{
    double angle;
    if (spindirection == "CW")
        angle = M_PI / 2.0;// Top -> Right -> Bottom -> Left -> Top
    if (spindirection == "CCW")
        angle = -M_PI / 2.0;// Top -> Left -> Bottom -> Right -> Top

    DrawProjGroupItem* anchor = getAnchor();
    Base::Vector3d org(0.0, 0.0, 0.0);
    Base::Vector3d curRot = anchor->getXDirection();
    Base::Vector3d curDir = anchor->Direction.getValue();
    Base::Vector3d newRot = DrawUtil::vecRotate(curRot, angle, curDir, org);
    anchor->XDirection.setValue(newRot);

    updateSecondaryDirs();
}

std::vector<DrawProjGroupItem*> DrawProjGroup::getViewsAsDPGI()
{
    std::vector<DrawProjGroupItem*> result;
    auto views = Views.getValues();
    for (auto& v : views) {
        DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(v);
        result.push_back(item);
    }
    return result;
}

int DrawProjGroup::getDefProjConv() const { return Preferences::projectionAngle(); }

/*!
 *dumps the current iso DPGI's
 */
void DrawProjGroup::dumpISO(const char* title)
{
    Base::Console().Message("DPG ISO: %s\n", title);
    for (auto& docObj : Views.getValues()) {
        Base::Vector3d dir;
        Base::Vector3d axis;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        std::string t = v->Type.getValueAsString();
        dir = v->Direction.getValue();
        axis = v->getXDirection();

        Base::Console().Message("%s:  %s/%s\n", t.c_str(), DrawUtil::formatVector(dir).c_str(),
                                DrawUtil::formatVector(axis).c_str());
    }
}

PyObject* DrawProjGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawProjGroup::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName,
                                              App::Property* prop)
// transforms properties that had been changed
{
    // also check for changed properties of the base class
    DrawView::handleChangedPropertyType(reader, TypeName, prop);

    // property spacingX/Y had the App::PropertyFloat and were changed to App::PropertyLength
    if (prop == &spacingX && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat spacingXProperty;
        // restore the PropertyFloat to be able to set its value
        spacingXProperty.Restore(reader);
        spacingX.setValue(spacingXProperty.getValue());
    }
    else if (prop == &spacingY && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat spacingYProperty;
        spacingYProperty.Restore(reader);
        spacingY.setValue(spacingYProperty.getValue());
    }
}
