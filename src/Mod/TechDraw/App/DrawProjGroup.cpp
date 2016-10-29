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
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "DrawPage.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroup.h"

#include <Mod/TechDraw/App/DrawProjGroupPy.h>  // generated from DrawProjGroupPy.xml

using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"Document",
                                                              "First Angle",
                                                              "Third Angle",
                                                              NULL};

PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup(void)
{
    static const char *group = "ProjGroup";

    ADD_PROPERTY_TYPE(Anchor, (0), group, App::Prop_None, "The root view to align projections with");

    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY(ProjectionType, ((long)0));

    ADD_PROPERTY_TYPE(spacingX, (15), group, App::Prop_None, "Horizontal spacing between views");
    ADD_PROPERTY_TYPE(spacingY, (15), group, App::Prop_None, "Vertical spacing between views");

    ADD_PROPERTY(viewOrientationMatrix, (Base::Matrix4D()));
}

DrawProjGroup::~DrawProjGroup()
{
}

short DrawProjGroup::mustExecute() const
{
    if(Views.isTouched() ||
       Source.isTouched()) {
        return 1;
     }

    if (ProjectionType.isTouched())
        return 1;
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

    if(!page->hasValidTemplate())
        throw Base::Exception("Page template isn't valid");

    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);

    // C++ Standard says casting bool to int gives 0 or 1
    int numVertSpaces = (viewPtrs[0] || viewPtrs[3] || viewPtrs[7]) +
                        (viewPtrs[2] || viewPtrs[5] || viewPtrs[9]) +
                        (viewPtrs[6] != NULL);
    int numHorizSpaces = (viewPtrs[0] || viewPtrs[1] || viewPtrs[2]) +
                         (viewPtrs[7] || viewPtrs[8] || viewPtrs[9]);

    double availableX = page->getPageWidth() - spacingX.getValue() * (numVertSpaces + 1);
    double availableY = page->getPageHeight() - spacingY.getValue() * (numHorizSpaces + 1);

    double scale_x = availableX / width;
    double scale_y = availableY / height;

    double fudgeFactor = 0.90;
    float working_scale = fudgeFactor * std::min(scale_x, scale_y);

    //which gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239...
    //eg if working_scale = 0.115, then we want to use 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = std::floor(std::log10(working_scale));                  //if working_scale = a * 10^b, what is b?
    working_scale *= std::pow(10, -exponent);                                //now find what 'a' is.

    float valid_scales[2][8] = {{1.0, 1.25, 2.0, 2.5, 3.75, 5.0, 7.5, 10.0},   //equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                {1.0, 1.5 , 2.0, 3.0, 4.0 , 5.0, 8.0, 10.0}};  //equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1

    int i = 7;
    while (valid_scales[(exponent >= 0)][i] > working_scale)                 //choose closest value smaller than 'a' from list.
        i -= 1;                                                              //choosing top list if exponent -ve, bottom list for +ve exponent

    //now have the appropriate scale, reapply the *10^b
    return valid_scales[(exponent >= 0)][i] * pow(10, exponent);
}

QRectF DrawProjGroup::getRect() const
{
    DrawProjGroupItem *viewPtrs[10];
    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);    //min space for 1:1 drawing
    return QRectF(0,0,Scale.getValue() * width,Scale.getValue() * height);
}

void DrawProjGroup::minimumBbViews(DrawProjGroupItem *viewPtrs[10],
                                            double &width, double &height) const
{
    // Get bounding boxes in object scale
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes, false);

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

void DrawProjGroup::onChanged(const App::Property* prop)
{
    //TODO: For some reason, when the projection type is changed, the isometric views show change appropriately, but the orthographic ones dont... Or vice-versa.
    if (!isRestoring()) {
        if ( prop == &ProjectionType ) {
            resetPositions();
            execute();
        } else if (prop == &ScaleType ||
            prop == &viewOrientationMatrix ||
            prop == &Scale ||
            prop == &Views) {
            execute();
        } else if (prop == &spacingX ||
            prop == &spacingY) {
            execute();
        }
    }
    TechDraw::DrawViewCollection::onChanged(prop);
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
    DrawProjGroupItem *view( nullptr );

    if ( checkViewProjType(viewProjType) && !hasProjection(viewProjType) ) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        auto docObj( getDocument()->addObject( "TechDraw::DrawProjGroupItem",     //add to Document
                                               FeatName.c_str() ) );

        view = static_cast<TechDraw::DrawProjGroupItem *>( docObj );
        view->Source.setValue( Source.getValue() );
        view->ScaleType.setValue( ScaleType.getValue() );
        view->Scale.setValue( Scale.getValue() );
        view->Type.setValue( viewProjType );
        view->Label.setValue( viewProjType );
        setViewOrientation( view, viewProjType );

        addView(view);         //from DrawViewCollection - add to ProjGroup Views
        moveToCentre();
    }

    return view;
}

int DrawProjGroup::removeProjection(const char *viewProjType)
{
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

void DrawProjGroup::setViewOrientation(DrawProjGroupItem *v, const char *projType) const
{
    Base::Vector3d dir, xDir;

    // Traditional orthographic
    if(strcmp(projType, "Front") == 0) {
        dir.Set(0, 1, 0);
        xDir.Set(1, 0, 0);

    } else if(strcmp(projType, "Rear") == 0) {
        dir.Set(0, -1, 0);
        xDir.Set(-1, 0, 0);

    } else if(strcmp(projType, "Right") == 0) {
        dir.Set(1, 0, 0);
        xDir.Set(0, -1, 0);

    } else if(strcmp(projType, "Left") == 0) {
        dir.Set(-1, 0, 0);
        xDir.Set(0, 1, 0);

    } else if(strcmp(projType, "Top") == 0) {
        dir.Set(0, 0, 1);
        xDir.Set(1, 0, 0);

    } else if(strcmp(projType, "Bottom") == 0) {
        dir.Set(0, 0, -1);
        xDir.Set(1, 0, 0);

    // Isometric
    } else if(strcmp(projType, "FrontTopLeft") == 0) {
        dir.Set(-1/sqrt(3), 1/sqrt(3), 1/sqrt(3));
        xDir.Set(sqrt(2)/2.0, sqrt(2.0)/2.0, 0);

    } else if(strcmp(projType, "FrontTopRight") == 0) {
        dir.Set(1/sqrt(3), 1/sqrt(3), 1/sqrt(3));
        xDir.Set(sqrt(2)/2.0, -sqrt(2.0)/2.0, 0);

    } else if(strcmp(projType, "FrontBottomRight") == 0) {
        dir.Set(1/sqrt(3), 1/sqrt(3), -1/sqrt(3));
        xDir.Set(sqrt(2)/2.0, -sqrt(2.0)/2.0, 0);

    } else if(strcmp(projType, "FrontBottomLeft") == 0) {
        dir.Set(-1/sqrt(3), 1/sqrt(3), -1/sqrt(3));
        xDir.Set(sqrt(2)/2.0, sqrt(2.0)/2.0, 0);

    } else {
        throw Base::Exception("Unknown view type in DrawProjGroup::setViewOrientation()");
    }

    dir = viewOrientationMatrix.getValue() * dir;
    xDir = viewOrientationMatrix.getValue() * xDir;

    v->Direction.setValue(dir);
    v->XAxisDirection.setValue(xDir);
}

void DrawProjGroup::arrangeViewPointers(DrawProjGroupItem *viewPtrs[10]) const
{
    for (int i=0; i<10; ++i) {
        viewPtrs[i] = NULL;
    }

    auto anchorView( dynamic_cast<DrawProjGroupItem *>(Anchor.getValue()) );

    if (!anchorView) {  //TODO: Consider not requiring an anchor view, or allowing ones other than "Front"
        throw Base::Exception("No anchor view set in DrawProjGroup::arrangeViewPointers()");
    }

    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    if (ProjectionType.isValue("Document")) {
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
                    viewPtrs[thirdAngle ? 4 : 4] = oView;
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
                double scale = 1.0 / viewPtrs[i]->Scale.getValue();
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
    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);

    // TODO: Work on not requiring the front view...
    if (!viewPtrs[4]) {
        return false;
    }

    // Calculate bounding boxes for each displayed view
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes);

    // Now that things are setup, do the spacing
    double scale = Scale.getValue();
    double xSpacing = scale * spacingX.getValue();    //in mm
    double ySpacing = scale * spacingY.getValue();    //in mm

    if (viewPtrs[0] &&
        viewPtrs[0]->allowAutoPos() &&
        bboxes[0].IsValid()) {
        double displace = std::max(bboxes[0].LengthX() + bboxes[4].LengthX(),
                                   bboxes[0].LengthY() + bboxes[4].LengthY());
        viewPtrs[0]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[0]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[1] &&
        viewPtrs[1]->allowAutoPos() &&
        bboxes[1].IsValid()) {
        viewPtrs[1]->Y.setValue((bboxes[1].LengthY() + bboxes[4].LengthY()) / 2.0 + ySpacing);
    }
    if (viewPtrs[2] && viewPtrs[2]->allowAutoPos()) {
        double displace = std::max(bboxes[2].LengthX() + bboxes[4].LengthX(),
                                   bboxes[2].LengthY() + bboxes[4].LengthY());
        viewPtrs[2]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[2]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[3] &&
        viewPtrs[3]->allowAutoPos() &&
        bboxes[3].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[3]->X.setValue((bboxes[3].LengthX() + bboxes[4].LengthX()) / -2.0 - xSpacing);
    }
    if (viewPtrs[4]) {  // TODO: Move this check above, and figure out a sane bounding box based on other existing views
    }
    if (viewPtrs[5] &&
        viewPtrs[5]->allowAutoPos() &&
        bboxes[5].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[5]->X.setValue((bboxes[5].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
    }
    if (viewPtrs[6] &&
        viewPtrs[6]->allowAutoPos() &&
        bboxes[6].IsValid()) {    //"Rear"
        if (viewPtrs[5] &&
            bboxes[5].IsValid()) {
            viewPtrs[6]->X.setValue(viewPtrs[5]->X.getValue() + bboxes[5].LengthX()/2.0 + xSpacing + bboxes[6].LengthX() / 2.0 );
        }else if (viewPtrs[4] &&
            bboxes[4].IsValid()) {
            viewPtrs[6]->X.setValue((bboxes[6].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
        }
    }
    if (viewPtrs[7] &&
        viewPtrs[7]->allowAutoPos() &&
        bboxes[7].IsValid()) {
        double displace = std::max(bboxes[7].LengthX() + bboxes[4].LengthX(),
                                   bboxes[7].LengthY() + bboxes[4].LengthY());
        viewPtrs[7]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[7]->Y.setValue(displace / -2.0 - ySpacing);
    }
    if (viewPtrs[8] &&
        viewPtrs[8]->allowAutoPos() &&
        bboxes[8].IsValid() &&
        bboxes[4].IsValid()) {
        viewPtrs[8]->Y.setValue((bboxes[8].LengthY() + bboxes[4].LengthY()) / -2.0 - ySpacing);
    }
    if (viewPtrs[9] &&
        viewPtrs[9]->allowAutoPos() &&
        bboxes[9].IsValid()) {
        double displace = std::max(bboxes[9].LengthX() + bboxes[4].LengthX(),
                                   bboxes[9].LengthY() + bboxes[4].LengthY());
        viewPtrs[9]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[9]->Y.setValue(displace / -2.0 - ySpacing);
    }

    return true;
}

//!allow child DPGI's to be automatically positioned
void DrawProjGroup::resetPositions(void)
{
    for( auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if( view ) {
            view->setAutoPos(true);
            //X,Y == 0??
        }
     }
}

//TODO: Turn this into a command so it can be issued from python
void DrawProjGroup::setFrontViewOrientation(const Base::Matrix4D &newMat)
{
    viewOrientationMatrix.setValue(newMat);

    for( auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if( view ) {
            setViewOrientation(view, view->Type.getValueAsString());
            // TODO: Seems we should ensure that modifying the view triggers this automatically? IR
            view->touch();
        }
    }
}

App::DocumentObjectExecReturn *DrawProjGroup::execute(void)
{
    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!page) {
        return DrawViewCollection::execute();
    }

    if (ScaleType.isValue("Automatic")) {
        //Recalculate scale if Group is too big
        if (!checkFit(page)) {
            double newScale = calculateAutomaticScale();
            if(std::abs(Scale.getValue() - newScale) > FLT_EPSILON) {
                Scale.setValue(newScale);
                //Rebuild the DPGI's
                for( const auto it : Views.getValues() ) {
                    auto view( dynamic_cast<DrawProjGroupItem *>(it) );
                    if( view ) {
                        view->ScaleType.setValue("Custom");
                        view->Scale.setValue(newScale);             //not sure we have to set scale here. DVP will do it based on ScaleType
                        view->Scale.setStatus(App::Property::ReadOnly,true);
                        //view->touch();
                    }
                }
                resetPositions();
            }
        }
    } else if (ScaleType.isValue("Document")) {
        double docScale = page->Scale.getValue();
        if(std::abs(Scale.getValue() - docScale) > FLT_EPSILON) {
            Scale.setValue(docScale);
            //Rebuild the DPGI's
            const std::vector<App::DocumentObject *> &views = Views.getValues();
            for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
                App::DocumentObject *docObj = *it;
                if(docObj->getTypeId().isDerivedFrom(DrawProjGroupItem::getClassTypeId())) {
                    DrawProjGroupItem *view = dynamic_cast<DrawProjGroupItem *>(*it);
                    view->ScaleType.setValue("Document");
                    view->Scale.setValue(docScale);
                    view->Scale.setStatus(App::Property::ReadOnly,true);
                    //view->touch();
                }
            }
            resetPositions();
        }
    }


    // recalculate positions for children
    if (Views.getSize()) {
        distributeProjections();
    }
    //touch();

    return DrawViewCollection::execute();
}

App::Enumeration DrawProjGroup::usedProjectionType(void)
{
    //TODO: Would've been nice to have an Enumeration(const PropertyEnumeration &) constructor
    App::Enumeration ret(ProjectionTypeEnums, ProjectionType.getValueAsString());
    if (ret.isValue("Document")) {
        TechDraw::DrawPage * page = getPage();
        if ( page != NULL ) {
            ret.setValue(page->ProjectionType.getValueAsString());
        }
    }
    return ret;
}

void DrawProjGroup::onDocumentRestored()
{
    DrawViewCollection::onDocumentRestored();
}

PyObject *DrawProjGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
