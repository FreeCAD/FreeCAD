/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com                 *
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
#endif
#include <TopoDS_Shape.hxx>

#include <App/GeoFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "DrawViewPart.h"
#include "DrawUtil.h"
#include "Geometry.h"
#include "DimensionReferences.h"

using namespace TechDraw;

TopoDS_Shape ReferenceEntry::getGeometry() const
{
    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        std::string gType = geomType();
        if (gType == "Vertex") {
            auto vgeom = dvp->getVertex(getSubName());
            return vgeom->occVertex;
        } else if (gType == "Edge") {
            auto egeom = dvp->getEdge(getSubName());
            return egeom->occEdge;
        } else if (gType == "Face") {
            auto fgeom = dvp->getFace(getSubName());
            return fgeom->toOccFace();
        }
        return TopoDS_Shape();
    }

    Part::TopoShape shape = Part::Feature::getTopoShape(getObject());
    App::GeoFeature* geoFeat = dynamic_cast<App::GeoFeature*>(getObject());
    if (geoFeat) {
        shape.setPlacement(geoFeat->globalPlacement());
    }
    return shape.getSubShape(getSubName().c_str());
}

std::string ReferenceEntry::geomType() const
{
    return DrawUtil::getGeomTypeFromName(getSubName());
}

