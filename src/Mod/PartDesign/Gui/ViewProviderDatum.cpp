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
# include <QMessageBox>
# include <QAction>
# include <QMenu>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include "ViewProviderDatum.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumFeature.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/Body.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatum,Gui::ViewProviderGeometryObject)

ViewProviderDatum::ViewProviderDatum()
{    
    pShapeSep = new SoSeparator();
    pShapeSep->ref();
}

ViewProviderDatum::~ViewProviderDatum()
{
    pShapeSep->unref();
}

void ViewProviderDatum::attach(App::DocumentObject *obj)
{
    ViewProviderDocumentObject::attach(obj);

    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(getObject());
    if (pcDatum->getTypeId() == PartDesign::Plane::getClassTypeId())
        datumType = QObject::tr("Plane");
    else if (pcDatum->getTypeId() == PartDesign::Line::getClassTypeId())
        datumType = QObject::tr("Line");
    else if (pcDatum->getTypeId() == PartDesign::Point::getClassTypeId())
        datumType = QObject::tr("Point");

    SoSeparator* sep = new SoSeparator();
    SoPickStyle* ps = new SoPickStyle();
    ps->style = SoPickStyle::SHAPE;
    SoShapeHints* hints = new SoShapeHints();
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
    SoBaseColor* color = new SoBaseColor();
    color->rgb.setValue(0.9, 0.9, 0.1);
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.9f, 0.9f, 0.1f);
    material->transparency.setValue(0.2);
    sep->addChild(hints);
    sep->addChild(color);
    sep->addChild(material);
    sep->addChild(pShapeSep);
    addDisplayMaskMode(sep, "Base");
}

std::vector<std::string> ViewProviderDatum::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderDatum::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderDatum::onChanged(const App::Property* prop)
{
    /*if (prop == &Shape) {
        updateData(prop);
    }
    else {*/
        ViewProviderDocumentObject::onChanged(prop);
    //}
}

void ViewProviderDatum::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit datum ") + datumType, receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDatum::setEdit(int ModNum)
{
    if (!ViewProvider::setEdit(ModNum))
        return false;

    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this datum feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumParameters *datumDlg = qobject_cast<TaskDlgDatumParameters *>(dlg);
        if (datumDlg && datumDlg->getDatumView() != this)
            datumDlg = 0; // another datum feature left open its task panel
        if (dlg && !datumDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (datumDlg)
            Gui::Control().showDialog(datumDlg);
        else
            Gui::Control().showDialog(new TaskDlgDatumParameters(this));

        return true;
    }
    else {
        return ViewProvider::setEdit(ModNum);
    }
}

void ViewProviderDatum::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderGeometryObject::unsetEdit(ModNum);
    }
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPoint,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPoint::ViewProviderDatumPoint()
{
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;
    points->numPoints = 0;
    pShapeSep->addChild(points);
}

ViewProviderDatumPoint::~ViewProviderDatumPoint()
{

}

void ViewProviderDatumPoint::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Point* pcDatum = static_cast<PartDesign::Point*>(this->getObject());

    if (strcmp(prop->getName(),"_Point") == 0) {
        Base::Vector3d p = pcDatum->_Point.getValue();
        SoMFVec3f v;
        v.setNum(1);
        v.set1Value(0, p.x, p.y, p.z);
        SoMarkerSet* points = static_cast<SoMarkerSet*>(pShapeSep->getChild(0));

        SoVertexProperty* vprop;
        if (points->vertexProperty.getValue() == NULL) {
            vprop = new SoVertexProperty();
            vprop->vertex = v;
            points->vertexProperty = vprop;
        } else {
            vprop = static_cast<SoVertexProperty*>(points->vertexProperty.getValue());
            vprop->vertex = v;
        }

        points->numPoints = 1;
    }

    ViewProviderDatum::updateData(prop);
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumLine,PartDesignGui::ViewProviderDatum)

ViewProviderDatumLine::ViewProviderDatumLine()
{
    SoLineSet* lineSet = new SoLineSet();
    pShapeSep->addChild(lineSet);
}

ViewProviderDatumLine::~ViewProviderDatumLine()
{

}

void ViewProviderDatumLine::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Line* pcDatum = static_cast<PartDesign::Line*>(this->getObject());

    if (strcmp(prop->getName(),"_Base") == 0) {
        Base::Vector3d base = pcDatum->_Base.getValue();
        Base::Vector3d dir = pcDatum->_Direction.getValue();

        // Get limits of the line from bounding box of the body
        PartDesign::Body* body = PartDesign::Body::findBodyOf(this->getObject());
        if (body == NULL)
            return;
        Part::Feature* tipSolid = static_cast<Part::Feature*>(body->getPrevSolidFeature());
        if (tipSolid == NULL)
            return;
        Base::BoundBox3d bbox = tipSolid->Shape.getShape().getBoundBox();
        bbox.Enlarge(0.1 * bbox.CalcDiagonalLength());
        Base::Vector3d p1, p2;
        if (bbox.IsInBox(base)) {
            bbox.IntersectionPoint(base, dir, p1, Precision::Confusion());
            bbox.IntersectionPoint(base, -dir, p2, Precision::Confusion());
        } else {
            bbox.IntersectWithLine(base, dir, p1, p2);
            if ((p1 == Base::Vector3d(0,0,0)) && (p2 == Base::Vector3d(0,0,0)))
                bbox.IntersectWithLine(base, -dir, p1, p2);
        }

        // Display the line
        SoMFVec3f v;
        v.setNum(2);
        v.set1Value(0, p1.x, p1.y, p1.z);
        v.set1Value(1, p2.x, p2.y, p2.z);
        SoLineSet* lineSet = static_cast<SoLineSet*>(pShapeSep->getChild(0));

        SoVertexProperty* vprop;
        if (lineSet->vertexProperty.getValue() == NULL) {
            vprop = new SoVertexProperty();
            vprop->vertex = v;
            lineSet->vertexProperty = vprop;
        } else {
            vprop = static_cast<SoVertexProperty*>(lineSet->vertexProperty.getValue());
            vprop->vertex = v;
        }

        SoMFInt32 idx;
        idx.setNum(1);
        idx.set1Value(0, 2);
        lineSet->numVertices = idx;
    }

    ViewProviderDatum::updateData(prop);
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    SoFaceSet* faceSet = new SoFaceSet();
    pShapeSep->addChild(faceSet);
}

ViewProviderDatumPlane::~ViewProviderDatumPlane()
{

}

void ViewProviderDatumPlane::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Plane* pcDatum = static_cast<PartDesign::Plane*>(this->getObject());

    if (strcmp(prop->getName(),"_Base") == 0) {
        Base::Vector3d base = pcDatum->_Base.getValue();
        Base::Vector3d normal = pcDatum->_Normal.getValue();

        // Get limits of the plane from bounding box of the body
        PartDesign::Body* body = PartDesign::Body::findBodyOf(this->getObject());
        if (body == NULL)
            return;
        Part::Feature* tipSolid = static_cast<Part::Feature*>(body->getPrevSolidFeature());
        if (tipSolid == NULL)
            return;
        Base::BoundBox3d bbox = tipSolid->Shape.getShape().getBoundBox();
        bbox.Enlarge(0.1 * bbox.CalcDiagonalLength());

        // Calculate intersection of plane with bounding box edges
        // TODO: This can be a lot more efficient if we do the maths ourselves, e.g.
        // http://cococubed.asu.edu/code_pages/raybox.shtml
        // http://www.fho-emden.de/~hoffmann/cubeplane12112006.pdf
        Handle_Geom_Plane plane = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
        std::vector<Base::Vector3d> points;

        for (int i = 0; i < 12; i++) {
            // Get the edge of the bounding box
            Base::Vector3d p1, p2;
            bbox.CalcDistance(i, p1, p2);
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
            double a[points.size()];
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
        SoFaceSet* faceSet = static_cast<SoFaceSet*>(pShapeSep->getChild(0));

        SoVertexProperty* vprop;
        if (faceSet->vertexProperty.getValue() == NULL) {
            vprop = new SoVertexProperty();
            vprop->vertex = v;
            faceSet->vertexProperty = vprop;
        } else {
            vprop = static_cast<SoVertexProperty*>(faceSet->vertexProperty.getValue());
            vprop->vertex = v;
        }

        SoMFInt32 idx;
        idx.setNum(1);
        idx.set1Value(0, points.size());
        faceSet->numVertices = idx;
    }

    ViewProviderDatum::updateData(prop);
}


