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
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include <QRectF>

#include <Mod/Part/App/PartFeature.h>
#include "DrawPage.h"
#include "DrawProjGroup.h"

#include "DrawProjGroupPy.h"  // generated from DrawProjGroupPy.xml

using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"Document",
                                                              "First Angle",
                                                              "Third Angle",
                                                              NULL};

PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup(void)
{
    static const char *group = "Drawing view";

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
    //TODO: DrawView already has DrawPage* findParentPage()
    return findParentPage();
    //TechDraw::DrawPage *ret = NULL;

    //std::vector<App::DocumentObject*> parent = getInList();
    //for (std::vector<App::DocumentObject*>::iterator it = parent.begin(); it != parent.end(); ++it) {
    //    if ((*it)->getTypeId().isDerivedFrom(DrawPage::getClassTypeId())) {
    //        ret = static_cast<DrawPage *>(*it);
    //    }
    //}

    //return ret;
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

    float working_scale = std::min(scale_x, scale_y);

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
    if ( prop == &ProjectionType ||
         prop == &ScaleType ||
         prop == &viewOrientationMatrix ||
         prop == &Scale ) {
        if (!isRestoring()) {
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
    const std::vector<App::DocumentObject *> &views = Views.getValues();
    for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {

        DrawView *view = dynamic_cast<DrawView *>(*it);
        if(view->getTypeId() == DrawProjGroupItem::getClassTypeId()) {
            DrawProjGroupItem *projPtr = dynamic_cast<DrawProjGroupItem *>(*it);

            if( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 )
                return *it;
        }
    }

    return 0;
}

bool DrawProjGroup::hasProjection(const char *viewProjType) const
{
    const std::vector<App::DocumentObject *> &views = Views.getValues();

    for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {

        TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);
        if(view->getTypeId() == TechDraw::DrawProjGroupItem::getClassTypeId()) {
            TechDraw::DrawProjGroupItem *projPtr = dynamic_cast<TechDraw::DrawProjGroupItem *>(*it);

            if( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
                return true;
            }
        }
    }
    return false;
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

App::DocumentObject * DrawProjGroup::addProjection(const char *viewProjType)
{
    DrawProjGroupItem *view = NULL;

    if ( checkViewProjType(viewProjType) && !hasProjection(viewProjType) ) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        App::DocumentObject *docObj = getDocument()->addObject("TechDraw::DrawProjGroupItem",
                                                               FeatName.c_str());

        view = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );
        view->Source.setValue( Source.getValue() );
        view->ScaleType.setValue( ScaleType.getValue() );
        view->Scale.setValue( Scale.getValue() );
        view->Type.setValue( viewProjType );
        view->Label.setValue( viewProjType );
        setViewOrientation( view, viewProjType );

        addView(view);         //from DrawViewCollection
        moveToCentre();
    }

    return view;
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

int DrawProjGroup::removeProjection(const char *viewProjType)
{
    if ( checkViewProjType(viewProjType) ) {
        if(!hasProjection(viewProjType)) {
            throw Base::Exception("The projection doesn't exist in the group");
        }

        // Iterate through the child views and find the projection type
        const std::vector<App::DocumentObject *> &views = Views.getValues();
        for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {

            TechDraw::DrawView *view = dynamic_cast<TechDraw::DrawView *>(*it);
            if(view->getTypeId() == TechDraw::DrawProjGroupItem::getClassTypeId()) {
                TechDraw::DrawProjGroupItem *projPtr = dynamic_cast<TechDraw::DrawProjGroupItem *>(*it);

                if ( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
                    // Remove from the document
                    getDocument()->remObject((*it)->getNameInDocument());
                    moveToCentre();
                    return views.size();
                }
            }
        }
    }

    return -1;
}

void DrawProjGroup::arrangeViewPointers(DrawProjGroupItem *viewPtrs[10]) const
{
    for (int i=0; i<10; ++i) {
        viewPtrs[i] = NULL;
    }

    DrawProjGroupItem *anchorView = dynamic_cast<DrawProjGroupItem *>(Anchor.getValue());

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
    DrawProjGroupItem* oView;
    std::vector<App::DocumentObject *> views = Views.getValues();
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
        for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(DrawProjGroupItem::getClassTypeId())) {
                oView = dynamic_cast<DrawProjGroupItem *>(*it);

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
                    viewPtrs[thirdAngle ? 6 : 6] = oView;
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
    double xSpacing = spacingX.getValue();    //in mm
    double ySpacing = spacingY.getValue();    //in mm

    if (viewPtrs[0]) {
        double displace = std::max(bboxes[0].LengthX() + bboxes[4].LengthX(),
                                   bboxes[0].LengthY() + bboxes[4].LengthY());
        viewPtrs[0]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[0]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[1]) {
        viewPtrs[1]->Y.setValue((bboxes[1].LengthY() + bboxes[4].LengthY()) / 2.0 + ySpacing);
    }
    if (viewPtrs[2]) {
        double displace = std::max(bboxes[2].LengthX() + bboxes[4].LengthX(),
                                   bboxes[2].LengthY() + bboxes[4].LengthY());
        viewPtrs[2]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[2]->Y.setValue(displace / 2.0 + ySpacing);
    }
    if (viewPtrs[3]) {
        viewPtrs[3]->X.setValue((bboxes[3].LengthX() + bboxes[4].LengthX()) / -2.0 - xSpacing);
    }
    if (viewPtrs[4]) {  // TODO: Move this check above, and figure out a sane bounding box based on other existing views
    }
    if (viewPtrs[5]) {
        viewPtrs[5]->X.setValue((bboxes[5].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
    }
    if (viewPtrs[6]) {
        if (viewPtrs[5])
            viewPtrs[6]->X.setValue((bboxes[6].LengthX() + bboxes[4].LengthX()) / 2.0 + bboxes[5].LengthX() + 2 * xSpacing);
        else
            viewPtrs[6]->X.setValue((bboxes[6].LengthX() + bboxes[4].LengthX()) / 2.0 + xSpacing);
    }
    if (viewPtrs[7]) {
        double displace = std::max(bboxes[7].LengthX() + bboxes[4].LengthX(),
                                   bboxes[7].LengthY() + bboxes[4].LengthY());
        viewPtrs[7]->X.setValue(displace / -2.0 - xSpacing);
        viewPtrs[7]->Y.setValue(displace / -2.0 - ySpacing);
    }
    if (viewPtrs[8]) {
        viewPtrs[8]->Y.setValue((bboxes[8].LengthY() + bboxes[4].LengthY()) / -2.0 - ySpacing);
    }
    if (viewPtrs[9]) {
        double displace = std::max(bboxes[9].LengthX() + bboxes[4].LengthX(),
                                   bboxes[9].LengthY() + bboxes[4].LengthY());
        viewPtrs[9]->X.setValue(displace / 2.0 + xSpacing);
        viewPtrs[9]->Y.setValue(displace / -2.0 - ySpacing);
    }

    return true;
}

//TODO: Turn this into a command so it can be issued from python
void DrawProjGroup::setFrontViewOrientation(const Base::Matrix4D &newMat)
{
    viewOrientationMatrix.setValue(newMat);

    DrawProjGroupItem *view;
    std::vector<App::DocumentObject *> views = Views.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(DrawProjGroupItem::getClassTypeId())) {
            view = dynamic_cast<DrawProjGroupItem *>(*it);
            setViewOrientation(view, view->Type.getValueAsString());
            view->touch();
        }
    }
}

App::DocumentObjectExecReturn *DrawProjGroup::execute(void)
{
    if (ScaleType.isValue("Automatic")) {

        //Recalculate scale
        double autoScale = calculateAutomaticScale();

        if(std::abs(Scale.getValue() - autoScale) > FLT_EPSILON) {
            // Set this Scale
            Scale.setValue(autoScale);

            //Rebuild the view
            const std::vector<App::DocumentObject *> &views = Views.getValues();
            for(std::vector<App::DocumentObject *>::const_iterator it = views.begin(); it != views.end(); ++it) {
                App::DocumentObject *docObj = *it;
                if(docObj->getTypeId().isDerivedFrom(DrawView::getClassTypeId())) {
                    DrawView *view = dynamic_cast<DrawView *>(*it);

                    //Set scale factor of each view
                    view->ScaleType.setValue("Custom");
                    view->Scale.setValue(autoScale);
                    view->Scale.touch();
                    view->Scale.setStatus(App::Property::ReadOnly,true);
                    view->touch();
                }
            }
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
    execute();
}

PyObject *DrawProjGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
