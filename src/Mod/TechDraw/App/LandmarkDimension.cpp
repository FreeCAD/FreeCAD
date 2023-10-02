/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

//*
//* Notes:
//* LandmarkDimension doesn't need References2D as points are always calculated from
//* Reference3D features and DVP Vertices are always accessed by tag.
//* References2D is only used to store the parent DVP

#include "PreCompiled.h"

#ifndef _PreComp_
# include <gp_Ax2.hxx>
# include <gp_Pnt.hxx>
# include <gp_Pnt2d.hxx>
# include <HLRAlgo_Projector.hxx>
#endif

#include <App/Document.h>
#include <Base/Console.h>

#include "LandmarkDimension.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "ShapeExtractor.h"


using namespace TechDraw;

//===========================================================================
// LandmarkDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::LandmarkDimension, TechDraw::DrawViewDimension)


LandmarkDimension::LandmarkDimension()
{
    static const char *group = "Landmark";
    //this leaves a blank entry in position 1.
    ADD_PROPERTY_TYPE(ReferenceTags, ("") , group, App::Prop_Output, "Tags of Dimension Endpoints");
    std::vector<std::string> noTags;
    ReferenceTags.setValues(noTags);
}

LandmarkDimension::~LandmarkDimension()
{
}

void LandmarkDimension::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //this is only good for new RD creation, not restore from disk?
        if (prop == &References3D) {
            //handled in base class
        } else if (prop == &ReferenceTags) {
            //???
        }
    }

    DrawViewDimension::onChanged(prop);
}

short LandmarkDimension::mustExecute() const
{
    return DrawViewDimension::mustExecute();
}

App::DocumentObjectExecReturn *LandmarkDimension::execute()
{
    Base::Console().Message("LD::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    DrawViewPart* dvp = getViewPart();
    if (!dvp) {
        return App::DocumentObject::StdReturn;
    }
    References2D.setValue(dvp);

    std::vector<DocumentObject*> features = References3D.getValues();
    Base::Console().Message("LD::execute - features: %d\n", features.size());
    //if distance, required size = 2
    //if angle, required size = 3;    //not implemented yet
    unsigned int requiredSize = 2;
    if (features.size() < requiredSize) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<Base::Vector3d> points;
    std::vector<std::string> reprs = ReferenceTags.getValues();
    if (reprs.empty()) {
        //add verts to dvp & vert tags to RD
        for (auto& f: features) {
            Base::Vector3d loc3d = ShapeExtractor::getLocation3dFromFeat(f);
            Base::Vector3d loc2d = projectPoint(loc3d, dvp) * dvp->getScale();
            points.push_back(loc2d);
            std::string tag = dvp->addReferenceVertex(loc2d);
            reprs.push_back(tag);
        }
        ReferenceTags.setValues(reprs);
    } else {
        //update dvp referenceverts locations
        int index = 0;
        for (auto& f: features) {
            Base::Vector3d loc3d = ShapeExtractor::getLocation3dFromFeat(f);
            Base::Vector3d loc2d = projectPoint(loc3d, dvp) * dvp->getScale();
            points.push_back(loc2d);
            dvp->updateReferenceVert(reprs.at(index), loc2d);  //sb by tag
            index++;
        }
    }
    Base::Console().Message("LD::execute - front: %s back: %s\n",
                            DrawUtil::formatVector(points.front()).c_str(),
                            DrawUtil::formatVector(points.back()).c_str());
    setLinearPoints(points.front(), points.back());

    App::DocumentObjectExecReturn* dvdResult = DrawViewDimension::execute();

    dvp->addReferencesToGeom();
    dvp->requestPaint();

    overrideKeepUpdated(false);
    return dvdResult;
}

Base::Vector3d LandmarkDimension::projectPoint(const Base::Vector3d& pt, DrawViewPart* dvp) const
{
    Base::Vector3d stdOrg(0.0, 0.0, 0.0);
    gp_Ax2 viewAxis = dvp->getProjectionCS(stdOrg);
    Base::Vector3d alignedPt = pt - dvp->getOriginalCentroid();
    gp_Pnt gPt(alignedPt.x, alignedPt.y, alignedPt.z);

    HLRAlgo_Projector projector( viewAxis );
    gp_Pnt2d prjPnt;
    projector.Project(gPt, prjPnt);
    Base::Vector3d result(prjPnt.X(), prjPnt.Y(), 0.0);
    return DrawUtil::invertY(result);
}

int LandmarkDimension::getRefType() const
{
    //TODO: need changes here when other reference dim types added
    return DrawViewDimension::RefType::twoVertex;
}

DrawViewPart* LandmarkDimension::getViewPart() const
{
    std::vector<App::DocumentObject*> refs2d = References2D.getValues();
    App::DocumentObject* obj = refs2d.front();
    DrawViewPart* dvp = dynamic_cast<DrawViewPart*>(obj);
    return dvp;
}

void LandmarkDimension::onDocumentRestored()
{
    DrawViewPart* dvp = getViewPart();

    std::vector<DocumentObject*> features = References3D.getValues();
    std::vector<Base::Vector3d> points;
    std::vector<std::string> tags;
    //add verts to dvp & vert tags to RD
    for (auto& f: features) {
        Base::Vector3d loc3d = ShapeExtractor::getLocation3dFromFeat(f);
        Base::Vector3d loc2d = projectPoint(loc3d, dvp) * dvp->getScale();
        points.push_back(loc2d);
        std::string tag = dvp->addReferenceVertex(loc2d);
        tags.push_back(tag);
    }
    ReferenceTags.setValues(tags);

    setLinearPoints(points.front(), points.back());

    DrawViewDimension::onDocumentRestored();
}

void LandmarkDimension::unsetupObject()
{
    TechDraw::DrawViewPart* dvp = getViewPart();

    std::vector<std::string> tags = ReferenceTags.getValues();
    for (auto& t: tags) {
        dvp->removeReferenceVertex(t);
    }
    dvp->resetReferenceVerts();
    dvp->requestPaint();
}

