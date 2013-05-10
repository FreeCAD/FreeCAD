/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include "ViewProviderDatumPlane.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.9f, 0.9f, 0.13);
    material->transparency.setValue(0.2);
    pShapeSep->addChild(material);
}

ViewProviderDatumPlane::~ViewProviderDatumPlane()
{

}

void ViewProviderDatumPlane::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Plane* pcDatum = static_cast<PartDesign::Plane*>(this->getObject());

    if (strcmp(prop->getName(),"Placement") == 0) {
        Base::Placement plm = pcDatum->Placement.getValue();
        plm.invert();
        Base::Vector3d base(0,0,0);
        Base::Vector3d normal(0,0,1);

        // Get limits of the plane from bounding box of the body
        PartDesign::Body* body = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(this->getObject()));
        if (body == NULL)
            return;
        Base::BoundBox3d bbox = body->getBoundBox();
        bbox = bbox.Transformed(plm.toMatrix());
        double dlength = bbox.CalcDiagonalLength();
        bbox.Enlarge(0.1 * dlength);

        // Calculate intersection of plane with bounding box edges
        // TODO: This can be a lot more efficient if we do the maths ourselves, e.g.
        // http://cococubed.asu.edu/code_pages/raybox.shtml
        // http://www.fho-emden.de/~hoffmann/cubeplane12112006.pdf
        Handle_Geom_Plane plane = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
        std::vector<Base::Vector3d> points;

        for (int i = 0; i < 12; i++) {
            // Get the edge of the bounding box
            Base::Vector3d p1, p2;
            bbox.CalcEdge(i, p1, p2);
            Base::Vector3d ldir = p2 - p1;
            Handle_Geom_Line line = new Geom_Line(gp_Pnt(p1.x, p1.y, p1.z), gp_Dir(ldir.x, ldir.y, ldir.z));
            GeomAPI_IntCS intersector(line, plane);
            if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                continue;
            gp_Pnt pnt = intersector.Point(1);
            Base::Vector3d point(pnt.X(), pnt.Y(), pnt.Z());

            // Check whether intersection is on the bbox edge (bbox.IsInside() always tests false)
            double edgeLength = (p1 - p2).Length();
            double l1 = (p1 - point).Length();
            double l2 = (p2 - point).Length();
            if (fabs(edgeLength - l1 - l2) > 0.001)
                continue;

            // Check for duplicates
            bool duplicate = false;
            for (std::vector<Base::Vector3d>::const_iterator p = points.begin(); p != points.end(); p++) {
                if ((point - *p).Sqr() < Precision::Confusion()) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
                points.push_back(point);
        }

        if (points.size() < 3)
            return;

        // Sort the points to get a proper polygon, see http://www.fho-emden.de/~hoffmann/cubeplane12112006.pdf p.5
        if (points.size() > 3) {
            // Longest component of normal vector
            int longest;
            if (normal.x > normal.y)
                if (normal.x > normal.z)
                    longest = 0; // x is longest
                else
                    longest = 2; // z is longest
            else
                if (normal.y > normal.z)
                    longest = 1; // y is longest
                else
                    longest = 2; // z is longest

            // mean value for intersection points
            Base::Vector3d m;
            for (std::vector<Base::Vector3d>::iterator p = points.begin(); p != points.end(); p++)
                m += *p;
            m /= points.size();

            // Sort by angles
            std::vector<double> a(points.size());
            for (int i = 0; i < points.size() - 1; i++) {
                if (longest == 0)
                    a[i] = atan2(points[i].z - m.z, points[i].y - m.y);
                else if (longest == 1)
                    a[i] = atan2(points[i].z - m.z, points[i].x - m.x);
                else
                    a[i] = atan2(points[i].y - m.y, points[i].x - m.x);

                for (int k = i+1; k < points.size(); k++) {
                    if (longest == 0)
                        a[k] = atan2(points[k].z - m.z, points[k].y - m.y);
                    else if (longest == 1)
                        a[k] = atan2(points[k].z - m.z, points[k].x - m.x);
                    else
                        a[k] = atan2(points[k].y - m.y, points[k].x - m.x);

                    if (a[k] < a[i]) {
                        Base::Vector3d temp = points[i];
                        points[i] = points[k];
                        points[k] = temp;
                        a[i] = a[k];
                    }
                }
            }
        }

        // Display the plane
        SoMFVec3f v;
        v.setNum(points.size());
        for (int p = 0; p < points.size(); p++)
            v.set1Value(p, points[p].x, points[p].y, points[p].z);
        SoMFInt32 idx;
        idx.setNum(1);
        idx.set1Value(0, points.size());

        SoFaceSet* faceSet;
        SoLineSet* lineSet;
        SoVertexProperty* vprop;        

        if (pShapeSep->getNumChildren() == 1) {
            faceSet = new SoFaceSet();
            vprop = new SoVertexProperty();
            vprop->vertex = v;
            faceSet->vertexProperty = vprop;
            faceSet->numVertices = idx;
            pShapeSep->addChild(faceSet);
            lineSet = new SoLineSet();
            lineSet->vertexProperty = vprop;
            lineSet->numVertices = idx;
            pShapeSep->addChild(lineSet);
        } else {
            faceSet = static_cast<SoFaceSet*>(pShapeSep->getChild(1));
            vprop = static_cast<SoVertexProperty*>(faceSet->vertexProperty.getValue());
            vprop->vertex = v;
            faceSet->numVertices = idx;
            lineSet = static_cast<SoLineSet*>(pShapeSep->getChild(2));
            vprop = static_cast<SoVertexProperty*>(lineSet->vertexProperty.getValue());
            vprop->vertex = v;
            lineSet->numVertices = idx;
        }
    }

    ViewProviderDatum::updateData(prop);
}


