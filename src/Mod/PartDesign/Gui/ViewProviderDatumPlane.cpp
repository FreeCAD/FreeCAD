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
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLineSet.h>
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
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    sPixmap = "PartDesign_Plane.svg";
    
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.9f, 0.9f, 0.13f);
    material->transparency.setValue(0.5f);
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
        if (dlength < Precision::Confusion())
            return;
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
            for (unsigned int i = 0; i < points.size() - 1; i++) {
                if (longest == 0)
                    a[i] = atan2(points[i].z - m.z, points[i].y - m.y);
                else if (longest == 1)
                    a[i] = atan2(points[i].z - m.z, points[i].x - m.x);
                else
                    a[i] = atan2(points[i].y - m.y, points[i].x - m.x);

                for (unsigned int k = i+1; k < points.size(); k++) {
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
        // Note: To achieve different colours on the two sides of the plane, see:
        // http://doc.coin3d.org/Coin/classSoIndexedFaceSet.html
        SoCoordinate3* coord;
        PartGui::SoBrepFaceSet* faceSet;
        SoIndexedLineSet* lineSet;

        if (pShapeSep->getNumChildren() == 1) {
            // The polygon must be split up into triangles because the SoBRepFaceSet only handles those
            if (points.size() < 3)
                return;
            coord = new SoCoordinate3();
            coord->point.setNum(points.size());
            for (unsigned int p = 0; p < points.size(); p++)
                coord->point.set1Value(p, points[p].x, points[p].y, points[p].z);
            pShapeSep->addChild(coord);

            faceSet = new PartGui::SoBrepFaceSet();            
            faceSet->partIndex.setNum(1); // One face
            faceSet->partIndex.set1Value(0, points.size()-3 + 1); // with this many triangles
            faceSet->coordIndex.setNum(4 + 4*(points.size()-3));
            // The first triangle
            faceSet->coordIndex.set1Value(0, 0);
            faceSet->coordIndex.set1Value(1, 1);
            faceSet->coordIndex.set1Value(2, 2);
            faceSet->coordIndex.set1Value(3, SO_END_FACE_INDEX);
            // One more triangle for every extra polygon point
            for (unsigned int p = 3; p < points.size(); p++) {
                faceSet->coordIndex.set1Value(4 + 4*(p-3), 0);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 1, p-1);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 2, p);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 3, SO_END_FACE_INDEX);
            }
            pShapeSep->addChild(faceSet);

            lineSet = new SoIndexedLineSet();
            lineSet->coordIndex.setNum(points.size()+2);
            for (unsigned int p = 0; p < points.size(); p++)
                lineSet->coordIndex.set1Value(p, p);
            lineSet->coordIndex.set1Value(points.size(), 0);
            lineSet->coordIndex.set1Value(points.size()+1, SO_END_LINE_INDEX);
            pShapeSep->addChild(lineSet);
        } else {
            coord = static_cast<SoCoordinate3*>(pShapeSep->getChild(1));
            coord->point.setNum(points.size());
            for (unsigned int p = 0; p < points.size(); p++)
                coord->point.set1Value(p, points[p].x, points[p].y, points[p].z);
            faceSet = static_cast<PartGui::SoBrepFaceSet*>(pShapeSep->getChild(2));
            faceSet->partIndex.setNum(1); // One face
            faceSet->partIndex.set1Value(0, points.size()-3 + 1); // with this many triangles
            faceSet->coordIndex.setNum(4 + 4*(points.size()-3));
            // The first triangle
            faceSet->coordIndex.set1Value(0, 0);
            faceSet->coordIndex.set1Value(1, 1);
            faceSet->coordIndex.set1Value(2, 2);
            faceSet->coordIndex.set1Value(3, SO_END_FACE_INDEX);
            // One more triangle for every extra polygon point
            for (unsigned int p = 3; p < points.size(); p++) {
                faceSet->coordIndex.set1Value(4 + 4*(p-3), 0);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 1, p-1);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 2, p);
                faceSet->coordIndex.set1Value(4 + 4*(p-3) + 3, SO_END_FACE_INDEX);
            }
            lineSet = static_cast<SoIndexedLineSet*>(pShapeSep->getChild(3));
            lineSet->coordIndex.setNum(points.size()+2);
            for (unsigned int p = 0; p < points.size(); p++)
                lineSet->coordIndex.set1Value(p, p);
            lineSet->coordIndex.set1Value(points.size(), 0);
            lineSet->coordIndex.set1Value(points.size()+1, SO_END_LINE_INDEX);
        }
    }

    ViewProviderDatum::updateData(prop);
}


