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

#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include <Base/Matrix.h>

#include "Cube.h"
#include "DrawUtil.h"
#include "DrawPage.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroup.h"

#include <Mod/TechDraw/App/DrawProjGroupPy.h>  // generated from DrawProjGroupPy.xml

using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"Default",
                                                              "First Angle",
                                                              "Third Angle",
                                                              NULL};

const std::map<std::string,Base::Vector3d> DrawProjGroup::m_frameToStdDir = {
        { "Front",            Base::Vector3d(0, -1, 0) },    //front
        { "Rear",             Base::Vector3d(0, 1, 0) },     //rear
        { "Right",            Base::Vector3d(1, 0, 0) },     //right
        { "Left",             Base::Vector3d(-1, 0, 0) },    //left
        { "Top",              Base::Vector3d(0, 0, 1) },     //top
        { "Bottom",           Base::Vector3d(0, 0, -1) },    //bottom
        { "FrontTopLeft",     Base::Vector3d(-1,-1,1) },     //FTL
        { "FrontTopRight",    Base::Vector3d(1, -1, 1) },    //FTR
        { "FrontBottomRight", Base::Vector3d(1, -1, -1) },   //FBR
        { "FrontBottomLeft",  Base::Vector3d(-1, -1, -1) } };  //FBL
                                                              
const std::map<std::string,Base::Vector3d> DrawProjGroup::m_frameToStdRot = {
        { "Front",            Base::Vector3d(1,  0, 0) },     //front
        { "Rear",             Base::Vector3d(-1, 0, 0) },     //rear
        { "Right",            Base::Vector3d(0, -1, 0) },     //right      this makes the first rendering correct, but
        { "Left",             Base::Vector3d(0,  1, 0) },     //left       every 2nd subsequent rendering upside down (ie cube is incorrect)
//        { "Right",            Base::Vector3d(0, 1, 0) },     //right         this makes the first rendering upside down
//        { "Left",             Base::Vector3d(0,  -1, 0) },     //left        but subsequent renderings right side up (ie cube is correct)
        { "Top",              Base::Vector3d(1,  0, 0) },     //top
        { "Bottom",           Base::Vector3d(1,  0, 0) },     //bottom
        { "FrontTopLeft",     Base::Vector3d(-2, -1, 0.5) },   //FTL  ???
        { "FrontTopRight",    Base::Vector3d(2, -1, 0.5) },   //FTR   OK
        { "FrontBottomRight", Base::Vector3d(2, -1, -0.5) },   //FBR  probable
        { "FrontBottomLeft",  Base::Vector3d(-2, -1, -0.5) } }; //FBL ???

PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup(void)
{
    static const char *group = "Base";
    static const char *agroup = "Distribute";


    ADD_PROPERTY_TYPE(Anchor, (0), group, App::Prop_None, "The root view to align projections with");
    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY(ProjectionType, ((long)0));

    ADD_PROPERTY_TYPE(AutoDistribute ,(true),agroup,App::Prop_None,"Distribute Views Automatically or Manually");
    ADD_PROPERTY_TYPE(spacingX, (15), agroup, App::Prop_None, "Horizontal spacing between views");
    ADD_PROPERTY_TYPE(spacingY, (15), agroup, App::Prop_None, "Vertical spacing between views");

    m_cube = new Cube();
    m_cube->initialize(m_frameToStdDir.at("Right"), m_frameToStdRot.at("Right"), 
                      m_frameToStdDir.at("Left"),   m_frameToStdRot.at("Left"),
                      m_frameToStdDir.at("Front"), m_frameToStdRot.at("Front"), m_frameToStdDir.at("Rear"),   m_frameToStdRot.at("Rear"),
                      m_frameToStdDir.at("Top"),   m_frameToStdRot.at("Top"),   m_frameToStdDir.at("Bottom"), m_frameToStdRot.at("Bottom"));
}

DrawProjGroup::~DrawProjGroup()
{
    delete m_cube;
}

void DrawProjGroup::onChanged(const App::Property* prop)
{
    //TODO: For some reason, when the projection type is changed, the isometric views show change appropriately, but the orthographic ones dont... Or vice-versa.  why would you change from 1st to 3rd in mid drawing?
    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!isRestoring() && page) {
        if ( prop == &Views ) {
            recompute();
        } else if (prop == &Scale) {
            updateChildren(Scale.getValue());
            //resetPositions();
            distributeProjections();
        } else if (prop == &Source) {
            App::DocumentObject* sourceObj = Source.getValue();
            if (sourceObj != nullptr) {
                if (!hasAnchor()) {
                    addProjection("Front");
                }
            } else {
                //Source has been changed to null! Why? What to do?
            }
        } else if (prop == &ScaleType) {
            recompute();
        } else if (prop == &AutoDistribute  &&
            AutoDistribute.getValue()) {
            resetPositions();
            recompute();
        }
    }
    TechDraw::DrawViewCollection::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawProjGroup::execute(void)
{
    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!page) {
        return DrawViewCollection::execute();
    }

    App::DocumentObject* docObj = Source.getValue();
    if (docObj == nullptr) {
        return DrawViewCollection::execute();
    }

    docObj = Anchor.getValue();                     //must have an anchor, so create one as soon as we have a Page and Source
    if (docObj == nullptr) {
       docObj = addProjection("Front");
    }

    double newScale = Scale.getValue();
    if (ScaleType.isValue("Automatic")) {
        //Recalculate scale if Group is too big or too small!
        if (!checkFit(page)) {
            newScale = calculateAutomaticScale();
            if(std::abs(Scale.getValue() - newScale) > FLT_EPSILON) {
                resetPositions();
                Scale.setValue(newScale);
            }
         }
    } else if (ScaleType.isValue("Page")) {
        newScale = page->Scale.getValue();
        if(std::abs(Scale.getValue() - newScale) > FLT_EPSILON) {
            resetPositions();
            Scale.setValue(newScale);
        }
    } else if (ScaleType.isValue("Custom")) {
        //don't have to do anything special
    }

    // recalculate positions for children
    if (Views.getSize()) {
        updateChildren(newScale);
        distributeProjections();
    }

    return DrawViewCollection::execute();
}

short DrawProjGroup::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result = Views.isTouched() ||
                 Source.isTouched() ||
                 Scale.isTouched()  ||
                 ScaleType.isTouched() ||
                 ProjectionType.isTouched() ||
                 Anchor.isTouched();
    }
    if (result) return result;
    return TechDraw::DrawViewCollection::mustExecute();
}

Base::BoundBox3d DrawProjGroup::getBoundingBox() const
{
    Base::BoundBox3d bbox;

    std::vector<App::DocumentObject*> views = Views.getValues();
    TechDraw::DrawProjGroupItem *anchorView = dynamic_cast<TechDraw::DrawProjGroupItem *>(Anchor.getValue());
    for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
         if ((*it)->getTypeId().isDerivedFrom(DrawViewPart::getClassTypeId())) {
            DrawViewPart *part = static_cast<DrawViewPart *>(*it);
            Base::BoundBox3d  bb = part->getBoundingBox();

            bb.ScaleX(1. / part->Scale.getValue());
            bb.ScaleY(1. / part->Scale.getValue());

            // X and Y of dependant views are relative to the anchorView
            if (part != anchorView) {
                bb.MoveX(part->X.getValue());
                bb.MoveY(part->Y.getValue());
            }

            bbox.Add(bb);
        }
    }
    // This /should/ leave the centre of the bounding box at (0,0) except when
    // we're in the process of updating the anchor view's position (eg called
    // by moveToCentre())
    if (anchorView) { //TODO: It looks like we might be getting called before an anchor view is set - weird...
        bbox.MoveX(anchorView->X.getValue());
        bbox.MoveY(anchorView->Y.getValue());
    }
    return bbox;
}

TechDraw::DrawPage * DrawProjGroup::getPage(void) const
{
    return findParentPage();
}

// Function provided by Joe Dowsett, 2014
double DrawProjGroup::calculateAutomaticScale() const
{
    TechDraw::DrawPage *page = getPage();
    if (page == NULL)
      throw Base::Exception("No page is assigned to this feature");

    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);                               //get 1:1 bbxs
    double bbFudge = 1.2;
    width *= bbFudge;
    height *= bbFudge;

    // C++ Standard says casting bool to int gives 0 or 1
    int numVertSpaces = (viewPtrs[0] || viewPtrs[3] || viewPtrs[7]) +
                        (viewPtrs[2] || viewPtrs[5] || viewPtrs[9]) +
                        (viewPtrs[6] != NULL);
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
    return result;
}

QRectF DrawProjGroup::getRect() const         //this is current rect, not potential rect
{
    DrawProjGroupItem *viewPtrs[10];
    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);                           // w,h of just the views at 1:1 scale
    double xSpace = spacingX.getValue() * 3.0 * std::max(1.0,Scale.getValue());
    double ySpace = spacingY.getValue() * 2.0 * std::max(1.0,Scale.getValue());
    double rectW = Scale.getValue() * width + xSpace;                  //scale the 1:1 w,h and add whitespace
    double rectH = Scale.getValue() * height + ySpace;
    return QRectF(0,0,rectW,rectH);
}

//find area consumed by Views only in 1:1 Scale
void DrawProjGroup::minimumBbViews(DrawProjGroupItem *viewPtrs[10],
                                            double &width, double &height) const
{
    // Get bounding boxes in object scale
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes, false);

    //TODO: note that TLF/TRF/BLF,BRF extent a bit farther than a strict row/col arrangement would suggest.
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


void DrawProjGroup::moveToCentre(void)
{
    // Update the anchor view's X and Y to keep the bounding box centred on the origin
    Base::BoundBox3d tempbbox = getBoundingBox();
    DrawProjGroupItem *anchorView = dynamic_cast<DrawProjGroupItem *>(Anchor.getValue());
    if (anchorView) {
        anchorView->X.setValue((tempbbox.MinX + tempbbox.MaxX) / -2.0);
        anchorView->Y.setValue((tempbbox.MinY + tempbbox.MaxY) / -2.0);
    }
}

App::DocumentObject * DrawProjGroup::getProjObj(const char *viewProjType) const
{
    for( auto it : Views.getValues() ) {
        auto projPtr( dynamic_cast<DrawProjGroupItem *>(it) );
        if( projPtr &&
            strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
            return it;
        }
    }

    return 0;
}

DrawProjGroupItem* DrawProjGroup::getProjItem(const char *viewProjType) const
{
    App::DocumentObject* docObj = getProjObj(viewProjType);
    DrawProjGroupItem* result = static_cast<DrawProjGroupItem*>(docObj);
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
        if( view && strcmp(viewProjType, view->Type.getValueAsString()) == 0 ) {
            return true;
        }
    }
    return false;
}

App::DocumentObject * DrawProjGroup::addProjection(const char *viewProjType)
{
    //Base::Console().Message("TRACE - DPG::addProjection(%s)\n",viewProjType);
    //if this is the first Projection added, it should automatically be the Anchor/front view
    // or if viewProjType == "Front" Anchor.setValue(view)
    DrawProjGroupItem *view( nullptr );

    if ( checkViewProjType(viewProjType) && !hasProjection(viewProjType) ) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        auto docObj( getDocument()->addObject( "TechDraw::DrawProjGroupItem",     //add to Document
                                               FeatName.c_str() ) );
        view = static_cast<TechDraw::DrawProjGroupItem *>( docObj );
        view->Source.setValue( Source.getValue() );
        if (ScaleType.isValue("Automatic")) {
            view->ScaleType.setValue("Custom");
        } else {
            view->ScaleType.setValue( ScaleType.getValue() );
        }
        view->Scale.setValue( Scale.getValue() );
        view->Type.setValue( viewProjType );
        view->Label.setValue( viewProjType );
        view->Source.setValue( Source.getValue() );
        if( strcmp(viewProjType,"Front") == 0 ) {

            Anchor.setValue(docObj);
            view->Direction.setValue(m_frameToStdDir.at("Front"));    //just (Base::Vector3d(0.0,-1.0,0.0))
            view->OrientBasis.setValue(m_frameToStdRot.at("Front"));
            //view->recomputeFeature();
        } else {
            //really need to check with Cube to get current dir & rot
            view->Direction.setValue(m_frameToStdDir.at(viewProjType));
            view->OrientBasis.setValue(m_frameToStdRot.at(viewProjType));
        }

        addView(view);         //from DrawViewCollection - add to ProjGroup Views
        moveToCentre();
        view->recomputeFeature();
    }

    Base::Console().Message("TRACE - DPG::addProjection exits\n");
    return view;
}

int DrawProjGroup::removeProjection(const char *viewProjType)
{
    // TODO: shouldn't be able to delete "Front" unless deleting whole group
    if ( checkViewProjType(viewProjType) ) {
        if( !hasProjection(viewProjType) ) {
            throw Base::Exception("The projection doesn't exist in the group");
        }

        // Iterate through the child views and find the projection type
        for( auto it : Views.getValues() ) {
            auto projPtr( dynamic_cast<TechDraw::DrawProjGroupItem *>(it) );
            if( projPtr ) {
                if ( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
                    removeView(projPtr);                                        // Remove from collection
                    getDocument()->remObject( it->getNameInDocument() );        // Remove from the document
                    moveToCentre();
                    return Views.getValues().size();
                }
            }
        }
    }

    return -1;
}

int DrawProjGroup::purgeProjections()
{
    while (!Views.getValues().empty())   {
        std::vector<DocumentObject*> views = Views.getValues();
        DrawProjGroupItem* dpgi;
        DocumentObject* dObj =  views.back();
        dpgi = dynamic_cast<DrawProjGroupItem*>(dObj);
        if (dpgi) {
            std::string itemName = dpgi->Type.getValueAsString();
            removeProjection(itemName.c_str());
        }
    }
    return Views.getValues().size();
}

void DrawProjGroup::arrangeViewPointers(DrawProjGroupItem *viewPtrs[10]) const
{
    for (int i=0; i<10; ++i) {
        viewPtrs[i] = NULL;
    }

    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    if (ProjectionType.isValue("Default")) {
        projType = findParentPage()->ProjectionType.getValueAsString();
    } else {
        projType = ProjectionType.getValueAsString();
    }

    // Iterate through views and populate viewPtrs
    if ( strcmp(projType, "Third Angle") == 0 ||
         strcmp(projType, "First Angle") == 0    ) {
        //   Third Angle:  FTL  T  FTRight
        //                  L   F   Right   Rear
        //                 FBL  B  FBRight
        //
        //   First Angle:  FBRight  B  FBL
        //                  Right   F   L  Rear
        //                 FTRight  T  FTL
        bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
        for (auto it : Views.getValues()) {
            auto oView( dynamic_cast<DrawProjGroupItem *>(it) );
            if (oView) {
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
                    throw Base::Exception("Unknown view type in DrawProjGroup::arrangeViewPointers()");
                }
            }
        }
    } else {
        throw Base::Exception("Unknown view type in DrawProjGroup::arrangeViewPointers()");
    }
}

void DrawProjGroup::makeViewBbs(DrawProjGroupItem *viewPtrs[10],
                                          Base::BoundBox3d bboxes[10],
                                          bool documentScale) const
{
    for (int i = 0; i < 10; ++i)
        if (viewPtrs[i]) {
            bboxes[i] = viewPtrs[i]->getBoundingBox();
            if (!documentScale) {
                double scale = 1.0 / viewPtrs[i]->Scale.getValue();    //convert bbx to 1:1 scale
                bboxes[i].ScaleX(scale);
                bboxes[i].ScaleY(scale);
                bboxes[i].ScaleZ(scale);
            }
        } else {
            // BoundBox3d defaults to length=(FLOAT_MAX + -FLOAT_MAX)
            bboxes[i].ScaleX(0);
            bboxes[i].ScaleY(0);
            bboxes[i].ScaleZ(0);
        }
}

bool DrawProjGroup::distributeProjections()
{
    //TODO: bounding boxes do not take view orientation into account
    if (!AutoDistribute.getValue()) {
        return true;
    }
    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);

    // TODO: Work on not requiring the front view...
    //WF: there is always a "Front" view. That's in the nature of the ProjGroup!
    if (!viewPtrs[4]) {
        return false;
    }

    // Calculate bounding boxes for each displayed view
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes);

    // Now that things are setup, do the spacing
    double scale = Scale.getValue();
    if (scale < 1.0) scale = 1.0;
    double xSpacing = scale * spacingX.getValue();    //in mm
    double ySpacing = scale * spacingY.getValue();    //in mm

    if (viewPtrs[0] && viewPtrs[0]->allowAutoPos() &&
        bboxes[0].IsValid()) {
        double displace = std::max(bboxes[0].LengthX() + bboxes[4].LengthX(),
                                   bboxes[0].LengthY() + bboxes[4].LengthY());
        viewPtrs[0]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[0]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[1] && viewPtrs[1]->allowAutoPos() &&
        bboxes[1].IsValid()) {
        viewPtrs[1]->Y.setValue((bboxes[1].LengthY() + bboxes[4].LengthY()) / 2.0 + ySpacing);
    }
    if (viewPtrs[2] && viewPtrs[2]->allowAutoPos()
        ) {
        double displace = std::max(bboxes[2].LengthX() + bboxes[4].LengthX(),
                                   bboxes[2].LengthY() + bboxes[4].LengthY());
        viewPtrs[2]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[2]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[3] && viewPtrs[3]->allowAutoPos() &&
        bboxes[3].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[3]->X.setValue((bboxes[3].LengthX() + bboxes[4].LengthX()) / -2.0 - xSpacing);
    }
    if (viewPtrs[4]) {  // TODO: Move this check above, and figure out a sane bounding box based on other existing views
    }
    if (viewPtrs[5] && viewPtrs[5]->allowAutoPos() &&
        bboxes[5].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[5]->X.setValue((bboxes[5].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
    }
    if (viewPtrs[6] && viewPtrs[6]->allowAutoPos() &&
        bboxes[6].IsValid()) {    //"Rear"
        if (viewPtrs[5] &&
            bboxes[5].IsValid()) {
            viewPtrs[6]->X.setValue(viewPtrs[5]->X.getValue() + bboxes[5].LengthX()/2.0 + xSpacing + bboxes[6].LengthX() / 2.0 );
        }else if (viewPtrs[4] &&
            bboxes[4].IsValid()) {
            viewPtrs[6]->X.setValue((bboxes[6].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
        }
    }
    if (viewPtrs[7] && viewPtrs[7]->allowAutoPos() &&
        bboxes[7].IsValid()) {
        double displace = std::max(bboxes[7].LengthX() + bboxes[4].LengthX(),
                                   bboxes[7].LengthY() + bboxes[4].LengthY());
        viewPtrs[7]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[7]->Y.setValue(displace / -2.0 - ySpacing);
    }
    if (viewPtrs[8] && viewPtrs[8]->allowAutoPos() &&
        bboxes[8].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[8]->Y.setValue((bboxes[8].LengthY() + bboxes[4].LengthY()) / -2.0 - ySpacing);
    }
    if (viewPtrs[9] && viewPtrs[9]->allowAutoPos() &&
        bboxes[9].IsValid()) {
        double displace = std::max(bboxes[9].LengthX() + bboxes[4].LengthX(),
                                   bboxes[9].LengthY() + bboxes[4].LengthY());
        viewPtrs[9]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[9]->Y.setValue(displace / -2.0 - ySpacing);
    }

    return true;
}

//!allow all child DPGI's to be automatically positioned
void DrawProjGroup::resetPositions(void)
{
    if (AutoDistribute.getValue()) {
        for( auto it : Views.getValues() ) {
            auto view( dynamic_cast<DrawProjGroupItem *>(it) );
            if( view ) {
                view->setAutoPos(true);
            }
         }
    }
}



void DrawProjGroup::updateChildren(double scale)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if( view ) {
            if (ScaleType.isValue("Automatic")) {
                view->ScaleType.setValue("Custom");
                view->Scale.setStatus(App::Property::ReadOnly,true);
            } else if (ScaleType.isValue("Page")) {
                view->ScaleType.setValue("Page");
                view->Scale.setStatus(App::Property::ReadOnly,true);
            } else if (ScaleType.isValue("Custom")) {
                view->ScaleType.setValue("Custom");
                view->Scale.setStatus(App::Property::ReadOnly,true);
            }
            if(std::abs(view->Scale.getValue() - scale) > FLT_EPSILON) {
                view->Scale.setValue(scale);
            }
            view->recomputeFeature();
        }
    }
}


//!check if ProjectionGroup fits on Page
bool DrawProjGroup::checkFit(TechDraw::DrawPage* p) const
{
    bool result = true;

    QRectF viewBox = getRect();
    double fudge = 1.1;
    double maxWidth = viewBox.width() * fudge;
    double maxHeight = viewBox.height() * fudge;

    if ( (maxWidth > p->getPageWidth()) ||
         (maxHeight > p->getPageHeight()) ) {
        result = false;
    }

    if (ScaleType.isValue("Automatic")) {                        //expand if too small
        double magnifyLimit = 0.60;
        if ( (maxWidth < p->getPageWidth() * magnifyLimit) &&
             (maxHeight < p->getPageHeight() * magnifyLimit) ) {
            result = false;
        }
    }
    return result;
}


App::Enumeration DrawProjGroup::usedProjectionType(void)
{
    //TODO: Would've been nice to have an Enumeration(const PropertyEnumeration &) constructor
    App::Enumeration ret(ProjectionTypeEnums, ProjectionType.getValueAsString());
    if (ret.isValue("Default")) {
        TechDraw::DrawPage * page = getPage();
        if ( page != NULL ) {
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
//    Base::Console().Message("TRACE - DPG::getAnchor() - %s\n",getNameInDocument());
    DrawProjGroupItem* result = nullptr;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj == nullptr) {
        //explode! DPG w/o anchor
        Base::Console().Message("TRACE - DPG::getAnchor - DPG has no Anchor!!!\n");
    } else {
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

void DrawProjGroup::updateSecondaryDirs()
{
//    Base::Console().Message("TRACE - DPG::updateSecondaryDirs() - %s\n",getNameInDocument());
    for (auto& docObj: Views.getValues()) {
        Base::Vector3d newDir;
        Base::Vector3d newAxis;
        std::string pic;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        ProjItemType t = static_cast<ProjItemType>(v->Type.getValue());
        switch (t) {
            case Front : {
                newDir = m_cube->getFront();
                newAxis = m_cube->getFrontRot();
                break;
            }
            case Rear : {
                newDir = m_cube->getRear();
                newAxis = m_cube->getRearRot();
                break;
            }
            case Left : {
                newDir = m_cube->getLeft();
                newAxis = m_cube->getLeftRot();
                break;
            }
            case Right : {
                newDir = m_cube->getRight();
                newAxis = m_cube->getRightRot();
                break;
            }
            case Top : {
                newDir = m_cube->getTop();
                newAxis = m_cube->getTopRot();
                break;
            }
            case Bottom : {
                newDir = m_cube->getBottom();
                newAxis = m_cube->getBottomRot();
                break;
            }
            case FrontTopLeft : {
                newDir = m_frameToStdDir.at("FrontTopLeft");
                newAxis = m_frameToStdRot.at("FrontTopLeft");
                break;
            }
            case FrontTopRight : {
                newDir = m_frameToStdDir.at("FrontTopRight");
                newAxis = m_frameToStdRot.at("FrontTopRight");
                break;
            }
            case FrontBottomLeft : {
                newDir = m_frameToStdDir.at("FrontBottomLeft");
                newAxis = m_frameToStdRot.at("FrontBottomLeft");
                break;
            }
            case FrontBottomRight : {
                newDir = m_frameToStdDir.at("FrontBottomRight");
                newAxis = m_frameToStdRot.at("FrontBottomRight");
                break;
            }
            default: {
                //TARFU invalid secondary type
                Base::Console().Message("ERROR - DPG::updateSecondaryDirs - invalid projection type\n");
                newDir = v->Direction.getValue();
                newAxis = v->OrientBasis.getValue();
            }
        }
        v->Direction.setValue(newDir);
        v->OrientBasis.setValue(newAxis);
        v->recomputeFeature();
    }
//    Base::Console().Message("TRACE - DPG::updateSecondaryDirs exits\n");
}


void DrawProjGroup::rotateRight()
{
//Front -> Right -> Rear -> Left -> Front
    //m_cube->dump("TRACE - DPG::rotR before");
    m_cube->rotateRight(); 
    //m_cube->dump("TRACE - DPG::rotR after");
    updateSecondaryDirs();
}

void DrawProjGroup::rotateLeft()
{
//Front -> Left -> Rear -> Right -> Front
    //m_cube->dump("TRACE - DPG::rotL before");
    m_cube->rotateLeft(); 
    //m_cube->dump("TRACE - DPG::rotL after");
    updateSecondaryDirs();
}

void DrawProjGroup::rotateUp()
{
//Front -> Top -> Rear -> Bottom -> Front
    //m_cube->dump("TRACE - DPG::rotU before");
    m_cube->rotateUp(); 
    //m_cube->dump("TRACE - DPG::rotU after");
    updateSecondaryDirs();
}
    
void DrawProjGroup::rotateDown()
{
//Front -> Bottom -> Rear -> Top -> Front
    m_cube->dump("TRACE - DPG::rotD before");
    m_cube->rotateDown(); 
    m_cube->dump("TRACE - DPG::rotD after");
    updateSecondaryDirs();
}

void DrawProjGroup::spinCW()
{
//Top -> Right -> Bottom -> Left -> Top
    //m_cube->dump("TRACE - DPG::spinCW before");
    m_cube->spinCW(); 
    //m_cube->dump("TRACE - DPG::spinCW after");
    updateSecondaryDirs();
}
    
void DrawProjGroup::spinCCW()
{
//Top -> Left -> Bottom -> Right -> Top
    //m_cube->dump("TRACE - DPG::spingCCW before");
    m_cube->spinCCW(); 
    //m_cube->dump("TRACE - DPG::spinCCW after");
    updateSecondaryDirs();
}

//*************************************

//! rebuild view direction map from existing DPGI's if possible or from Anchor
void DrawProjGroup::onDocumentRestored()
{
//    Base::Vector3d f,r,t;
//    Base::Vector3d fr,rr,tr;
//    bool ffound = false;
//    bool rfound = false;
//    bool tfound = false;
//    
//    for (auto& docObj: Views.getValues()) {
//        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
//        ProjItemType type = static_cast<ProjItemType>(v->Type.getValue());
//        switch (type) {
//            case Front : {
//                f = v->Direction.getValue();
//                fr = v->OrientBasis.getValue();
//                ffound = true;
//                break;
//            }
//            case Right : {
//                r  = v->Direction.getValue();
//                rr = v->OrientBasis.getValue();
//                rfound = true;
//                break;
//            }
//            case Top : {
//                t = v->Direction.getValue();
//                tr = v->OrientBasis.getValue();
//                tfound = true;
//                break;
//            }
//            default: {
//                break;
//            }
//        }
//    }
//    if (ffound && rfound && tfound) {
//        //m_cube->reinit(r,rr,f,fr,t,tr);
//        Base::Console().Log("LOG: - DPG::restore - found Right,Front,Top\n");
//    } else {
//        m_cube->initialize(r,f,t);
//        Base::Console().Log("LOG: - DPG::restore - didn't find Right,Front,Top\n");
//    }
//    DrawViewCollection::onDocumentRestored();
}

PyObject *DrawProjGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
