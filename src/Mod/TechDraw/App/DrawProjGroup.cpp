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
# include <sstream>
#include <QRectF>
#include <cmath>
#endif

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"
#include "Preferences.h"
#include "DrawPage.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroup.h"

#include <Mod/TechDraw/App/DrawProjGroupPy.h>  // generated from DrawProjGroupPy.xml

using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"First Angle",
                                                    "Third Angle",
                                                    "Default",          //Use Page setting
                                                    nullptr};

PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup(void) :
    m_lockScale(false)
{
    static const char *group = "Base";
    static const char *agroup = "Distribute";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                               GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool autoDist = hGrp->GetBool("AutoDist",true);
    
    ADD_PROPERTY_TYPE(Source, (nullptr), group, App::Prop_None, "Shape to view");
    Source.setScope(App::LinkScope::Global);
    Source.setAllowExternal(true);
    ADD_PROPERTY_TYPE(XSource, (nullptr), group,App::Prop_None, "External 3D Shape to view");

    ADD_PROPERTY_TYPE(Anchor, (nullptr), group, App::Prop_None, "The root view to align projections with");
    Anchor.setScope(App::LinkScope::Global);

    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY_TYPE(ProjectionType, ((long)getDefProjConv()), group,
                                App::Prop_None, "First or Third angle projection");

    ADD_PROPERTY_TYPE(AutoDistribute, (autoDist), agroup,
                                App::Prop_None, "Distribute views automatically or manually");
    ADD_PROPERTY_TYPE(spacingX, (15), agroup, App::Prop_None, "If AutoDistribute is on, this is the horizontal \nspacing between the borders of views \n(if label width is not wider than the object)");
    ADD_PROPERTY_TYPE(spacingY, (15), agroup, App::Prop_None, "If AutoDistribute is on, this is the vertical \nspacing between the borders of views");
    Rotation.setStatus(App::Property::Hidden, true);   //DPG does not rotate
    Caption.setStatus(App::Property::Hidden, true);
}

DrawProjGroup::~DrawProjGroup()
{
}

//TODO: this duplicates code in DVP
std::vector<App::DocumentObject*> DrawProjGroup::getAllSources(void) const
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
    TechDraw::DrawPage *page = getPage();
    if (!isRestoring() && page) {
        if (prop == &Scale) {
            if (!m_lockScale) {
                updateChildrenScale();
                updateChildrenEnforce();
            }
        }

        if (prop == &ProjectionType) {
            updateChildrenEnforce();
        }

        if ( (prop == &Source) ||
             (prop == &XSource) ) {
            updateChildrenSource();
        }

        if ((prop == &spacingX) || (prop == &spacingY)) {
            updateChildrenEnforce();
        }

        if (prop == &LockPosition) {
            updateChildrenLock();
        }

        if (prop == &ScaleType) {
            if (ScaleType.isValue("Automatic")) {
                //Nothing in particular
            } else if (ScaleType.isValue("Page")) {
                double newScale = page->Scale.getValue();
                if(std::abs(getScale() - newScale) > FLT_EPSILON) {
                    Scale.setValue(newScale);
                }
            }
            updateChildrenScale();
        }
        if (prop == &Rotation) {
            if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
                Rotation.setValue(0.0);
                purgeTouched();
                Base::Console().Log("DPG: Projection Groups do not rotate. Change ignored.\n");
            }
        }
    }

    TechDraw::DrawViewCollection::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawProjGroup::execute(void)
{
//    Base::Console().Message("DPG::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!page) {
        return DrawViewCollection::execute();
    }

    std::vector<App::DocumentObject*> docObjs = getAllSources();
    if (docObjs.empty()) {
        return DrawViewCollection::execute();
    }

    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj == nullptr) {
        //no anchor yet.  nothing to do.
        return DrawViewCollection::execute();
    }

    if (ScaleType.isValue("Automatic")) {
        if (!checkFit()) {
            double newScale = autoScale();
            m_lockScale = true;
            Scale.setValue(newScale);
            Scale.purgeTouched();
            updateChildrenScale();
            m_lockScale = false;
        }
    }

    autoPositionChildren();
    return DrawViewCollection::execute();
}

short DrawProjGroup::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result = Views.isTouched() ||
                 Source.isTouched() ||
                 XSource.isTouched() ||
                 Scale.isTouched()  ||
                 ScaleType.isTouched() ||
                 ProjectionType.isTouched() ||
                 Anchor.isTouched() ||
                 AutoDistribute.isTouched() ||
                 LockPosition.isTouched()||
                 spacingX.isTouched() ||
                 spacingY.isTouched();
    }
    if (result)
        return result;
    return TechDraw::DrawViewCollection::mustExecute();
}

Base::BoundBox3d DrawProjGroup::getBoundingBox() const
{
    Base::BoundBox3d bbox;

    std::vector<App::DocumentObject*> views = Views.getValues();
    TechDraw::DrawProjGroupItem *anchorView = dynamic_cast<TechDraw::DrawProjGroupItem *>(Anchor.getValue());
    if (anchorView == nullptr) {
        //if an element in Views is not a DPGI, something really bad has happened somewhere
        Base::Console().Log("PROBLEM - DPG::getBoundingBox - non DPGI entry in Views! %s\n",
                                getNameInDocument());
        throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
    }
    for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
         if ((*it)->getTypeId().isDerivedFrom(DrawViewPart::getClassTypeId())) {
            DrawViewPart *part = static_cast<DrawViewPart *>(*it);
            Base::BoundBox3d  bb = part->getBoundingBox();

            bb.ScaleX(1. / part->getScale());
            bb.ScaleY(1. / part->getScale());
            bb.ScaleZ(1. / part->getScale());

            // X and Y of dependent views are relative to the anchorView
            if (part != anchorView) {
                bb.MoveX(part->X.getValue());
                bb.MoveY(part->Y.getValue());
            }

            bbox.Add(bb);
        }
    }
    return bbox;
}

TechDraw::DrawPage * DrawProjGroup::getPage(void) const
{
    return findParentPage();
}

// obs? replaced by autoscale?
// Function provided by Joe Dowsett, 2014
double DrawProjGroup::calculateAutomaticScale() const
{
    TechDraw::DrawPage *page = getPage();
    if (page == nullptr)
      throw Base::RuntimeError("No page is assigned to this feature");

    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);   //get SCALED boxes!
                                        // if Page.keepUpdated is false, and DrawViews have never been executed,
                                        // bb's will be 0x0 and this routine will return 0!!!
                                        // if we return 1.0, AutoScale will sort itself out once bb's are non-zero.
    double bbFudge = 1.2;
    width *= bbFudge;
    height *= bbFudge;

    // C++ Standard says casting bool to int gives 0 or 1
    int numVertSpaces = (viewPtrs[0] || viewPtrs[3] || viewPtrs[7]) +
                        (viewPtrs[2] || viewPtrs[5] || viewPtrs[9]) +
                        (viewPtrs[6] != nullptr);
    int numHorizSpaces = (viewPtrs[0] || viewPtrs[1] || viewPtrs[2]) +
                         (viewPtrs[7] || viewPtrs[8] || viewPtrs[9]);

    double availableX = page->getPageWidth();
    double availableY = page->getPageHeight();
    double xWhite = spacingX.getValue() * (numVertSpaces + 1);
    double yWhite = spacingY.getValue() * (numHorizSpaces + 1);
    width += xWhite;
    height += yWhite;
    double scale_x = availableX / width;
    double scale_y = availableY / height;

    double scaleFudge = 0.80;
    float working_scale = scaleFudge * std::min(scale_x, scale_y);
    double result = DrawUtil::sensibleScale(working_scale);
    if (!(result > 0.0)) {
        Base::Console().Log("DPG - %s - bad scale found (%.3f) using 1.0\n",getNameInDocument(),result);
        result = 1.0;
    }

    return result;
}

//returns the (scaled) bounding rectangle of all the views.
QRectF DrawProjGroup::getRect() const         //this is current rect, not potential rect
{
//    Base::Console().Message("DPG::getRect - views: %d\n", Views.getValues().size());
    DrawProjGroupItem *viewPtrs[10];
    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);                //this is scaled!
    double xSpace = spacingX.getValue() * 3.0 * std::max(1.0, getScale());
    double ySpace = spacingY.getValue() * 2.0 * std::max(1.0, getScale());
    double rectW = 0.0;
    double rectH = 0.0;
    if ( !(DrawUtil::fpCompare(width, 0.0) &&
           DrawUtil::fpCompare(height, 0.0)) ) {
        rectW = width + xSpace;
        rectH = height + ySpace;
    }
    double fudge = 1.3;  //make rect a little big to make sure it fits
    rectW *= fudge;
    rectH *= fudge;
    return QRectF(0,0,rectW,rectH);
}

//find area consumed by Views only in current scale
void DrawProjGroup::minimumBbViews(DrawProjGroupItem *viewPtrs[10],
                                            double &width, double &height) const
{
    // Get bounding boxes in object scale
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes, true);   //true => scaled

    //TODO: note that TLF/TRF/BLF,BRF extend a bit farther than a strict row/col arrangement would suggest.
    //get widest view in each row/column
    double col0w = std::max(std::max(bboxes[0].LengthX(), bboxes[3].LengthX()), bboxes[7].LengthX()),
           col1w = std::max(std::max(bboxes[1].LengthX(), bboxes[4].LengthX()), bboxes[8].LengthX()),
           col2w = std::max(std::max(bboxes[2].LengthX(), bboxes[5].LengthX()), bboxes[9].LengthX()),
           col3w = bboxes[6].LengthX(),
           row0h = std::max(std::max(bboxes[0].LengthY(), bboxes[1].LengthY()), bboxes[2].LengthY()),
           row1h = std::max(std::max(bboxes[3].LengthY(), bboxes[4].LengthY()),
                            std::max(bboxes[5].LengthY(), bboxes[6].LengthY())),
           row2h = std::max(std::max(bboxes[7].LengthY(), bboxes[8].LengthY()), bboxes[9].LengthY());

    width = col0w + col1w + col2w + col3w;
    height = row0h + row1h + row2h;
}

App::DocumentObject * DrawProjGroup::getProjObj(const char *viewProjType) const
{
    for( auto it : Views.getValues() ) {
        auto projPtr( dynamic_cast<DrawProjGroupItem *>(it) );
        if (projPtr == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::getProjObj - non DPGI entry in Views! %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else if(strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
            return it;
        }
    }

    return nullptr;
}

DrawProjGroupItem* DrawProjGroup::getProjItem(const char *viewProjType) const
{
    App::DocumentObject* docObj = getProjObj(viewProjType);
    auto result( dynamic_cast<TechDraw::DrawProjGroupItem *>(docObj) );
    if ( (result == nullptr) &&
         (docObj != nullptr) ) {
        //should never have a item in DPG that is not a DPGI.
        Base::Console().Log("PROBLEM - DPG::getProjItem finds non-DPGI in Group %s / %s\n",
                                getNameInDocument(),viewProjType);
        throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
    }
    return result;
}

bool DrawProjGroup::checkViewProjType(const char *in)
{
    if ( strcmp(in, "Front") == 0 ||
         strcmp(in, "Left") == 0 ||
         strcmp(in, "Right") == 0 ||
         strcmp(in, "Top") == 0 ||
         strcmp(in, "Bottom") == 0 ||
         strcmp(in, "Rear") == 0 ||
         strcmp(in, "FrontTopLeft") == 0 ||
         strcmp(in, "FrontTopRight") == 0 ||
         strcmp(in, "FrontBottomLeft") == 0 ||
         strcmp(in, "FrontBottomRight") == 0) {
        return true;
    }
    return false;
}

//********************************
// ProjectionItem A/D/I
//********************************
bool DrawProjGroup::hasProjection(const char *viewProjType) const
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<TechDraw::DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //should never have a item in DPG that is not a DPGI.
            Base::Console().Log("PROBLEM - DPG::hasProjection finds non-DPGI in Group %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }

        if (strcmp(viewProjType, view->Type.getValueAsString()) == 0 ) {
            return true;
        }
    }
    return false;
}

App::DocumentObject * DrawProjGroup::addProjection(const char *viewProjType)
{
    DrawProjGroupItem *view( nullptr );
    std::pair<Base::Vector3d,Base::Vector3d> vecs;

    DrawPage* dp = findParentPage();
    if (dp == nullptr) {
        Base::Console().Error("DPG:addProjection - %s - DPG is not on a page!\n",getNameInDocument());
    }

    if ( checkViewProjType(viewProjType) && !hasProjection(viewProjType) ) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        auto docObj( getDocument()->addObject( "TechDraw::DrawProjGroupItem",     //add to Document
                                               FeatName.c_str() ) );
        view = dynamic_cast<TechDraw::DrawProjGroupItem *>(docObj);
        if ( (view == nullptr) &&
             (docObj != nullptr) ) {
            //should never happen that we create a DPGI that isn't a DPGI!!
            Base::Console().Log("PROBLEM - DPG::addProjection - created a non DPGI! %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: new projection is not a DPGI!");
        }
        if (view != nullptr) {                        //coverity CID 151722
            // the label must be set before the view is added
            view->Label.setValue(viewProjType);
            addView(view);                            //from DrawViewCollection
            view->Source.setValues(Source.getValues());
//            std::vector<DocumentObject*> xLinks;
//            XSource.getLinks(xLinks);
//            view->XSource.setValues(xLinks);
            view->XSource.setValues(XSource.getValues());

            // the Scale is already set by DrawView
            view->Type.setValue(viewProjType);
            if (strcmp(viewProjType, "Front") != 0 ) {  //not Front!
                vecs = getDirsFromFront(view);
                view->Direction.setValue(vecs.first);
                view->XDirection.setValue(vecs.second);
                view->recomputeFeature();
            } else {  //Front
                Anchor.setValue(view);
                Anchor.purgeTouched();
                requestPaint();   //make sure the group object is on the Gui page
                view->LockPosition.setValue(true);  //lock "Front" position within DPG (note not Page!).
                view->LockPosition.setStatus(App::Property::ReadOnly,true); //Front should stay locked.
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
int DrawProjGroup::removeProjection(const char *viewProjType)
{
    // TODO: shouldn't be able to delete "Front" unless deleting whole group
    if ( checkViewProjType(viewProjType) ) {
        if( !hasProjection(viewProjType) ) {
            throw Base::RuntimeError("The projection doesn't exist in the group");
        }

        // Iterate through the child views and find the projection type
        for( auto it : Views.getValues() ) {
            auto projPtr( dynamic_cast<TechDraw::DrawProjGroupItem *>(it) );
            if( projPtr != nullptr) {
                if ( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
                    removeView(projPtr);                                           // Remove from collection
                    getDocument()->removeObject( it->getNameInDocument() );        // Remove from the document
                    return Views.getValues().size();
                }
            } else {
                //if an element in Views is not a DPGI, something really bad has happened somewhere
                Base::Console().Log("PROBLEM - DPG::removeProjection - tries to remove non DPGI! %s / %s\n",
                                    getNameInDocument(),viewProjType);
                throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
            }
        }
    }

    return -1;
} 

//removes all DPGI - used when deleting DPG
int DrawProjGroup::purgeProjections()
{
    while (!Views.getValues().empty())   {
        std::vector<DocumentObject*> views = Views.getValues();
        DrawProjGroupItem* dpgi;
        DocumentObject* dObj =  views.back();
        dpgi = dynamic_cast<DrawProjGroupItem*>(dObj);
        if (dpgi != nullptr) {
            std::string itemName = dpgi->Type.getValueAsString();
            removeProjection(itemName.c_str());
        } else {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::purgeProjection - tries to remove non DPGI! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
    }
    auto page = findParentPage();
    if (page != nullptr) {
        page->requestPaint();
    }

    return Views.getValues().size();
}

std::pair<Base::Vector3d,Base::Vector3d> DrawProjGroup::getDirsFromFront(DrawProjGroupItem* view)
{
    std::pair<Base::Vector3d,Base::Vector3d> result;
    std::string viewType = view->Type.getValueAsString();
    result = getDirsFromFront(viewType);
    return result;
}

std::pair<Base::Vector3d,Base::Vector3d> DrawProjGroup::getDirsFromFront(std::string viewType)
{
//    Base::Console().Message("DPG::getDirsFromFront(%s)\n", viewType.c_str());
    std::pair<Base::Vector3d,Base::Vector3d> result;

    Base::Vector3d projDir, rotVec;
    DrawProjGroupItem* anch = getAnchor();
    if (anch == nullptr) {
        Base::Console().Warning("DPG::getDirsFromFront - %s - No Anchor!\n",Label.getValue());
        throw Base::RuntimeError("Project Group missing Anchor projection item");
    }

    Base::Vector3d dirAnch = anch->Direction.getValue();
    Base::Vector3d rotAnch = anch->getXDirection();
    result = std::make_pair(dirAnch,rotAnch);

    Base::Vector3d org(0.0,0.0,0.0);
    gp_Ax2 anchorCS = anch->getProjectionCS(org);
    gp_Pnt gOrg(0.0, 0.0, 0.0);
    gp_Dir gDir = anchorCS.Direction();
    gp_Dir gXDir = anchorCS.XDirection();
    gp_Dir gYDir = anchorCS.YDirection();
    gp_Ax1 gUpAxis(gOrg, gYDir);
    gp_Ax2 newCS;
    gp_Dir gNewDir;
    gp_Dir gNewXDir;
    
    double angle = M_PI / 2.0;                        //90*

    if (viewType == "Right") {
        newCS = anchorCS.Rotated(gUpAxis, angle);
        projDir = dir2vec(newCS.Direction());
        rotVec  = dir2vec(newCS.XDirection());
    } else if (viewType == "Left") {
        newCS = anchorCS.Rotated(gUpAxis, -angle);
        projDir = dir2vec(newCS.Direction());
        rotVec  = dir2vec(newCS.XDirection());
    } else if (viewType == "Top") {
        projDir = dir2vec(gYDir);
        rotVec  = dir2vec(gXDir);
    } else if (viewType == "Bottom") {
        projDir = dir2vec(gYDir.Reversed());
        rotVec  = dir2vec(gXDir);
    } else if (viewType == "Rear") {
        projDir = dir2vec(gDir.Reversed());
        rotVec  = dir2vec(gXDir.Reversed());
    } else if (viewType == "FrontTopLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) -
                               gp_Vec(gXDir) +
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) +
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontTopRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) +
                               gp_Vec(gXDir) +
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) -
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontBottomLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) -
                               gp_Vec(gXDir) -
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) +
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontBottomRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) +
                               gp_Vec(gXDir) -
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) -
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else {
        Base::Console().Error("DrawProjGroup - %s unknown projection: %s\n",getNameInDocument(),viewType.c_str());
        return result;
    }

    result = std::make_pair(projDir,rotVec);
    return result;
}

Base::Vector3d DrawProjGroup::dir2vec(gp_Dir d)
{
    Base::Vector3d result(d.X(),
                        d.Y(),
                        d.Z());
    return result;
}

gp_Dir DrawProjGroup::vec2dir(Base::Vector3d v)
{
    gp_Dir result(v.x,
                  v.y,
                  v.z);
    return result;
}

//this can be improved.  this implementation positions views too far apart.
Base::Vector3d DrawProjGroup::getXYPosition(const char *viewTypeCStr)
{
//    Base::Console().Message("DPG::getXYPosition(%s)\n",Label.getValue());

    Base::Vector3d result(0.0,0.0,0.0);
    //Front view position is always (0,0)
    if (strcmp(viewTypeCStr, "Front") == 0 ) {  // Front!
        return result;
    }
    const int idxCount = 10;
    DrawProjGroupItem *viewPtrs[idxCount];
    arrangeViewPointers(viewPtrs);
    int viewIndex = getViewIndex(viewTypeCStr);

        //TODO: bounding boxes do not take view orientation into account
        //      i.e. X&Y widths might be swapped on page

    if (viewPtrs[viewIndex]->LockPosition.getValue()) {
        result.x = viewPtrs[viewIndex]->X.getValue();
        result.y = viewPtrs[viewIndex]->Y.getValue();
        return result;
    }

    if (AutoDistribute.getValue()) {
        std::vector<Base::Vector3d> position(idxCount);

        // Calculate bounding boxes for each displayed view
        Base::BoundBox3d bboxes[10];
        makeViewBbs(viewPtrs, bboxes);         //scaled

        double xSpacing = spacingX.getValue();    //in mm, no scale
        double ySpacing = spacingY.getValue();    //in mm, no scale

        double bigRow    = 0.0;
        double bigCol    = 0.0;
        int ibbx = 0;
        for (auto& b: bboxes) {          //space based on width/height of biggest view
            if (!b.IsValid()) {
                Base::Console().Message("DVP::getXYPos - bbox %d is not valid!\n", ibbx);
                continue;
            }
            if (b.LengthX() > bigCol) {
                bigCol = b.LengthX();
            }
            if (b.LengthY() > bigRow) {
                bigRow = b.LengthY();
            }
            ibbx++;
        }

        //if we have iso's, make sure they fit the grid.
        if (viewPtrs[0] || viewPtrs[2]  || viewPtrs[7] ||  viewPtrs[9]) {
            bigCol = std::max(bigCol,bigRow);
            bigRow = bigCol;
        }

        if (viewPtrs[4] &&                       //Front
            bboxes[4].IsValid()) {
            position[4].x = 0.0;
            position[4].y = 0.0;
        }

        if (viewPtrs[3] &&                        // L/R  (third/first)
            bboxes[3].IsValid() &&
            bboxes[4].IsValid()) {
            position[3].x = -(xSpacing + bigCol);
            position[3].y = 0.0;
        }

        if (viewPtrs[5] &&                        // R/L (third/first)
            bboxes[5].IsValid() &&
            bboxes[4].IsValid()) {
            position[5].x = xSpacing + bigCol;
            position[5].y = 0.0;
        }

        if (viewPtrs[6] &&
            bboxes[6].IsValid()) {    //"Rear"
            if (viewPtrs[5] &&
                bboxes[5].IsValid()) {
                //there is a view between Front and Rear
                position[6].x = 2.0 * (xSpacing + bigCol);
                position[6].y = 0.0;
            } else if (viewPtrs[4] &&
                bboxes[4].IsValid()) {
                // there is no view between Front and Rear
                position[6].x = xSpacing + bigCol;
                position[6].y = 0.0;
            }
        }

        if (viewPtrs[1] &&                        // T/B (third/first)
            bboxes[1].IsValid() &&
            bboxes[4].IsValid()) {
            position[1].x = 0.0;
            position[1].y = ySpacing + bigRow;
        }

        if (viewPtrs[8] &&                        // B/T (third/first)
            bboxes[8].IsValid() &&
            bboxes[4].IsValid()) {
            position[8].x = 0.0;
            position[8].y = -(ySpacing + bigRow);
        }

        if (viewPtrs[0] &&                      // iso top left
            bboxes[0].IsValid()) {
            position[0].x = -(xSpacing + bigCol);
            position[0].y = ySpacing + bigRow;
        }

        if (viewPtrs[2] &&                      // iso top right
            bboxes[2].IsValid()) {
            position[2].x = xSpacing + bigCol;
            position[2].y = ySpacing + bigRow;
        }

        if (viewPtrs[7] &&                      // iso bottom left
            bboxes[7].IsValid()) {
            position[7].x = -(xSpacing + bigCol);
            position[7].y = -(ySpacing + bigRow);
        }

        if (viewPtrs[9] &&                      // iso bottom right
            bboxes[9].IsValid()) {
            position[9].x = xSpacing + bigCol;;
            position[9].y = -(ySpacing + bigRow);
        }

        result.x = position[viewIndex].x;
        result.y = position[viewIndex].y;
    } else {
        result.x = viewPtrs[viewIndex]->X.getValue();
        result.y = viewPtrs[viewIndex]->Y.getValue();
    }
    return result;
}

int DrawProjGroup::getViewIndex(const char *viewTypeCStr) const
{
    int result = 4;                                        //default to front view's position
    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    DrawPage* dp = findParentPage();
    if (ProjectionType.isValue("Default")) {
        if (dp != nullptr) {
            projType = dp->ProjectionType.getValueAsString();
        } else {
            Base::Console().Warning("DPG: %s - can not find parent page. Using default Projection Type. (1)\n",
                                    getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv];
        }
    } else {
        projType = ProjectionType.getValueAsString();
    }

    if ( strcmp(projType, "Third Angle") == 0 ||
         strcmp(projType, "First Angle") == 0    ) {
        //   Third Angle:  FTL  T  FTRight          0  1  2
        //                  L   F   Right   Rear    3  4  5  6
        //                 FBL  B  FBRight          7  8  9
        //
        //   First Angle:  FBRight  B  FBL          0  1  2
        //                  Right   F   L  Rear     3  4  5  6
        //                 FTRight  T  FTL          7  8  9

        bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
        if (strcmp(viewTypeCStr, "Front") == 0) {
            result = 4;
        } else if (strcmp(viewTypeCStr, "Left") == 0) {
            result = thirdAngle ? 3 : 5;
        } else if (strcmp(viewTypeCStr, "Right") == 0) {
            result = thirdAngle ? 5 : 3;
        } else if (strcmp(viewTypeCStr, "Top") == 0) {
            result = thirdAngle ? 1 : 8;
        } else if (strcmp(viewTypeCStr, "Bottom") == 0) {
            result = thirdAngle ? 8 : 1;
        } else if (strcmp(viewTypeCStr, "Rear") == 0) {
            result = 6;
        } else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
            result = thirdAngle ? 0 : 9;
        } else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
            result = thirdAngle ? 2 : 7;
        } else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
            result = thirdAngle ? 7 : 2;
        } else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
            result = thirdAngle ? 9 : 0;
        } else {
            throw Base::TypeError("Unknown view type in DrawProjGroup::getViewIndex()");
        }
    } else {
        throw Base::ValueError("Unknown Projection convention in DrawProjGroup::getViewIndex()");
    }
    return result;
}

void DrawProjGroup::arrangeViewPointers(DrawProjGroupItem *viewPtrs[10]) const
{
    for (int i=0; i<10; ++i) {
        viewPtrs[i] = nullptr;
    }

    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    if (ProjectionType.isValue("Default")) {
        DrawPage* dp = findParentPage();
        if (dp != nullptr) {
            projType = dp->ProjectionType.getValueAsString();
        } else {
            Base::Console().Error("DPG:arrangeViewPointers - %s - DPG is not on a page!\n",
                                    getNameInDocument());
            Base::Console().Warning("DPG:arrangeViewPointers - using system default Projection Type\n",
                                    getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv + 1];
        }
    } else {
        projType = ProjectionType.getValueAsString();
    }

    // Iterate through views and populate viewPtrs
    if ( strcmp(projType, "Third Angle") == 0 ||
         strcmp(projType, "First Angle") == 0    ) {
        //   Third Angle:  FTL  T  FTRight          0  1  2
        //                  L   F   Right   Rear    3  4  5  6
        //                 FBL  B  FBRight          7  8  9
        //
        //   First Angle:  FBRight  B  FBL          0  1  2
        //                  Right   F   L  Rear     3  4  5  6
        //                 FTRight  T  FTL          7  8  9

        bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
        for (auto it : Views.getValues()) {
            auto oView( dynamic_cast<DrawProjGroupItem *>(it) );
            if (oView == nullptr) {
                //if an element in Views is not a DPGI, something really bad has happened somewhere
                Base::Console().Log("PROBLEM - DPG::arrangeViewPointers - non DPGI in Views! %s\n",
                                    getNameInDocument());
                throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
            } else {
                const char *viewTypeCStr = oView->Type.getValueAsString();
                if (strcmp(viewTypeCStr, "Front") == 0) {
                  //viewPtrs[thirdAngle ? 4 : 4] = oView;
                    viewPtrs[4] = oView;
                } else if (strcmp(viewTypeCStr, "Left") == 0) {
                    viewPtrs[thirdAngle ? 3 : 5] = oView;
                } else if (strcmp(viewTypeCStr, "Right") == 0) {
                    viewPtrs[thirdAngle ? 5 : 3] = oView;
                } else if (strcmp(viewTypeCStr, "Top") == 0) {
                    viewPtrs[thirdAngle ? 1 : 8] = oView;
                } else if (strcmp(viewTypeCStr, "Bottom") == 0) {
                    viewPtrs[thirdAngle ? 8 : 1] = oView;
                } else if (strcmp(viewTypeCStr, "Rear") == 0) {
                    viewPtrs[6] = oView;
                } else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
                    viewPtrs[thirdAngle ? 0 : 9] = oView;
                } else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
                    viewPtrs[thirdAngle ? 2 : 7] = oView;
                } else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
                    viewPtrs[thirdAngle ? 7 : 2] = oView;
                } else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
                    viewPtrs[thirdAngle ? 9 : 0] = oView;
                } else {
                    Base::Console().Warning("DPG: %s - unknown view type: %s. \n",
                                            getNameInDocument(),viewTypeCStr);
                    throw Base::TypeError("Unknown view type in DrawProjGroup::arrangeViewPointers.");
                }
            }
        }
    } else {
        Base::Console().Warning("DPG: %s - unknown Projection convention: %s\n",getNameInDocument(),projType);
        throw Base::ValueError("Unknown Projection convention in DrawProjGroup::arrangeViewPointers");
    }
}

void DrawProjGroup::makeViewBbs(DrawProjGroupItem *viewPtrs[10],
                                          Base::BoundBox3d bboxes[10],
                                          bool documentScale) const
{
    Base::BoundBox3d empty(Base::Vector3d(0.0, 0.0, 0.0), 0.0);
    for (int i = 0; i < 10; ++i) {
        bboxes[i] = empty;
        if (viewPtrs[i]) {
            bboxes[i] = viewPtrs[i]->getBoundingBox();
//            bboxes[i] = viewPtrs[i]->getBoundingBox(viewPtrs[i]->getProjectionCS(Base::Vector3d(0.0, 0.0, 0.0)));
            if (!documentScale) {
                double scale = 1.0 / viewPtrs[i]->getScale();    //convert bbx to 1:1 scale
                bboxes[i].ScaleX(scale);
                bboxes[i].ScaleY(scale);
                bboxes[i].ScaleZ(scale);
            }
        }
    }
}

void DrawProjGroup::recomputeChildren(void)
{
//    Base::Console().Message("DPG::recomputeChildren()\n");
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->recomputeFeature();
        }
    }
}

void DrawProjGroup::autoPositionChildren(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->autoPosition();
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Scale
 */
void DrawProjGroup::updateChildrenScale(void)
{
//    Base::Console().Message("DPG::updateChildrenScale\n");
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenScale - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->Scale.setValue(getScale());
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Source
 */
void DrawProjGroup::updateChildrenSource(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenSource - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            if (view->Source.getValues() != Source.getValues()) {
                view->Source.setValues(Source.getValues());
            }
            if (view->XSource.getValues() != XSource.getValues()) {
                view->XSource.setValues(XSource.getValues());
            }
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed LockPosition
 * (really for benefit of QGIV on Gui side)
 */
void DrawProjGroup::updateChildrenLock(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenLock - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->requestPaint();
        }
    }
}

void DrawProjGroup::updateViews(void) {
    // this is intended to update the views in general, e.g. when the spacing changed
    for (const auto it : Views.getValues()) {
        auto view(dynamic_cast<DrawProjGroupItem *>(it));
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateViews - non DPGI entry in Views! %s\n",
                getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
        else // the views are OK
            view->recomputeFeature();
    }
}

void DrawProjGroup::updateChildrenEnforce(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenEnforce - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->enforceRecompute();
        }
    }
}

App::Enumeration DrawProjGroup::usedProjectionType(void)
{
    //TODO: Would've been nice to have an Enumeration(const PropertyEnumeration &) constructor
    App::Enumeration ret(ProjectionTypeEnums, ProjectionType.getValueAsString());
    if (ret.isValue("Default")) {
        TechDraw::DrawPage * page = getPage();
        if ( page != nullptr ) {
            ret.setValue(page->ProjectionType.getValueAsString());
        }
    }
    return ret;
}

bool DrawProjGroup::hasAnchor(void)
{
    bool result = false;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        result = true;
    }
    return result;
}

TechDraw::DrawProjGroupItem* DrawProjGroup::getAnchor(void)
{
    DrawProjGroupItem* result = nullptr;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        result = static_cast<DrawProjGroupItem*>(docObj);
    }
    return result;
}

void DrawProjGroup::setAnchorDirection(const Base::Vector3d dir)
{
    App::DocumentObject* docObj = Anchor.getValue();
    DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
    item->Direction.setValue(dir);
}

Base::Vector3d DrawProjGroup::getAnchorDirection(void)
{
    Base::Vector3d result;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
        result = item->Direction.getValue();
    } else {
        Base::Console().Log("ERROR - DPG::getAnchorDir - no Anchor!!\n");
    }
    return result;
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

    std::map<std::string, std::pair<Base::Vector3d,Base::Vector3d> > saveVals;
    std::string key;
    std::pair<Base::Vector3d, Base::Vector3d> data;
    for (auto& docObj: Views.getValues()) {
        std::pair<Base::Vector3d,Base::Vector3d> newDirs;
        std::string pic;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        ProjItemType t = static_cast<ProjItemType>(v->Type.getValue());
        switch (t) {
            case Front :
                data.first = anchDir;
                data.second = anchRot;
                key = "Front";
                saveVals[key] = data;
                break;
            case Rear :
                key = "Rear";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Left :
                key = "Left";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Right :
            key = "Right";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Top :
                key = "Top";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Bottom :
                key = "Bottom";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopLeft :
                key = "FrontTopLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopRight :
                key = "FrontTopRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomLeft :
                key = "FrontBottomLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomRight :
                key = "FrontBottomRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            default: {
                //TARFU invalid secondary type
                Base::Console().Message("ERROR - DPG::updateSecondaryDirs - invalid projection type\n");
            }
        }
    }

    for (auto& docObj: Views.getValues()) {
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

void DrawProjGroup::rotate(const std::string &rotationdirection) {
    std::pair<Base::Vector3d,Base::Vector3d> newDirs;
    if(rotationdirection == "Right") newDirs = getDirsFromFront("Left"); // Front -> Right -> Rear -> Left -> Front
    else if(rotationdirection == "Left") newDirs = getDirsFromFront("Right"); // Front -> Left -> Rear -> Right -> Front
    else if(rotationdirection == "Up") newDirs = getDirsFromFront("Bottom"); // Front -> Top -> Rear -> Bottom -> Front
    else if(rotationdirection == "Down") newDirs = getDirsFromFront("Top"); // Front -> Bottom -> Rear -> Top -> Front

    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);

    updateSecondaryDirs();
}

void DrawProjGroup::spin(const std::string &spindirection)
{
    double angle;
    if(spindirection == "CW") angle = M_PI / 2.0; // Top -> Right -> Bottom -> Left -> Top
    if(spindirection == "CCW") angle = - M_PI / 2.0; // Top -> Left -> Bottom -> Right -> Top

    DrawProjGroupItem* anchor = getAnchor();
    Base::Vector3d org(0.0,0.0,0.0);
    Base::Vector3d curRot = anchor->getXDirection();
    Base::Vector3d curDir = anchor->Direction.getValue();
    Base::Vector3d newRot = DrawUtil::vecRotate(curRot,angle,curDir,org);
    anchor->XDirection.setValue(newRot);

    updateSecondaryDirs();
}

std::vector<DrawProjGroupItem*> DrawProjGroup::getViewsAsDPGI()
{
    std::vector<DrawProjGroupItem*> result;
    auto views = Views.getValues();
    for (auto& v:views) {
        DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(v);
        result.push_back(item);
    }
    return result;
}

int DrawProjGroup::getDefProjConv(void) const
{
    return Preferences::projectionAngle();
}

/*!
 *dumps the current iso DPGI's
 */
void DrawProjGroup::dumpISO(const char * title)
{
    Base::Console().Message("DPG ISO: %s\n", title);
    for (auto& docObj: Views.getValues()) {
        Base::Vector3d dir;
        Base::Vector3d axis;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        std::string t = v->Type.getValueAsString();
        dir = v->Direction.getValue();
        axis = v->getXDirection();

        Base::Console().Message("%s:  %s/%s\n",
                                t.c_str(),DrawUtil::formatVector(dir).c_str(),DrawUtil::formatVector(axis).c_str());
    }
}

PyObject *DrawProjGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawProjGroup::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // also check for changed properties of the base class
    DrawView::handleChangedPropertyType(reader, TypeName, prop);

    // property spacingX/Y had the App::PropertyFloat and were changed to App::PropertyLength
    if ((prop == &spacingX) && (strcmp(TypeName, "App::PropertyFloat") == 0)) {
        App::PropertyFloat spacingXProperty;
        // restore the PropertyFloat to be able to set its value
        spacingXProperty.Restore(reader);
        spacingX.setValue(spacingXProperty.getValue());
    }
    else if ((prop == &spacingY) && (strcmp(TypeName, "App::PropertyFloat") == 0)) {
        App::PropertyFloat spacingYProperty;
        spacingYProperty.Restore(reader);
        spacingY.setValue(spacingYProperty.getValue());
    }
}
