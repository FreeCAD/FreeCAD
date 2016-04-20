/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2016     *
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
# include <QObject>
#endif
#include "AttacherTexts.h"

using namespace Attacher;

namespace AttacherGui {

TextSet TwoStrings(QString str1, QString str2)
{
    std::vector<QString> v;
    v.resize(2);
    v[0] = str1;
    v[1] = str2;
    return v;
}

AttacherGui::TextSet AttacherGui::getUIStrings(Base::Type attacherType, Attacher::eMapMode mmode)
{
    if (attacherType.isDerivedFrom(Attacher::AttachEngine3D::getClassTypeId())){
        //---- coordinate system attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(QObject::tr("Deactivated","Attachment3D mode caption"),
                              QObject::tr("Attachment is disabled. CS can be moved by editing Placement property.","Attachment3D mode tooltip"));
        break;
        case mmTranslate:
            return TwoStrings(QObject::tr("Translate origin","Attachment3D mode caption"),
                              QObject::tr("Origin is aligned to match Vertex. Orientation is controlled by Placement property.","Attachment3D mode tooltip"));
        break;
        case mmObjectXY:
            return TwoStrings(QObject::tr("Object's  X Y Z","Attachment3D mode caption"),
                              QObject::tr("Placement is made equal to Placement of linked object.","Attachment3D mode tooltip"));
        break;
        case mmObjectXZ:
            return TwoStrings(QObject::tr("Object's  X Z-Y","Attachment3D mode caption"),
                              QObject::tr("X', Y', Z' axes are matched with object's local X, Z, -Y, respectively.","Attachment3D mode tooltip"));
        break;
        case mmObjectYZ:
            return TwoStrings(QObject::tr("Object's  Y Z X","Attachment3D mode caption"),
                              QObject::tr("X', Y', Z' axes are matched with object's local Y, Z, X, respectively.","Attachment3D mode tooltip"));
        break;
        case mmFlatFace:
            return TwoStrings(QObject::tr("XY on plane","Attachment3D mode caption"),
                              QObject::tr("X' Y' plane is aligned to coincide planar face.","Attachment3D mode tooltip"));
        break;
        case mmTangentPlane:
            return TwoStrings(QObject::tr("XY tangent to surface","Attachment3D mode caption"),
                              QObject::tr("X' Y' plane is made tangent to surface at vertex.","Attachment3D mode tooltip"));
        break;
        case mmNormalToPath:
            return TwoStrings(QObject::tr("Z tangent to edge","Attachment3D mode caption"),
                              QObject::tr("Z' axis is aligned to be tangent to edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmFrenetNB:
            return TwoStrings(QObject::tr("Frenet NBT","Attachment3D mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmFrenetTN:
            return TwoStrings(QObject::tr("Frenet TNB","Attachment3D mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmFrenetTB:
            return TwoStrings(QObject::tr("Frenet TBN","Attachment3D mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmConcentric:
            return TwoStrings(QObject::tr("Concentric","Attachment3D mode caption"),
                              QObject::tr("Align XY plane to osculating circle of an edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmRevolutionSection:
            return TwoStrings(QObject::tr("Revolution Section","Attachment3D mode caption"),
                              QObject::tr("Align Y' axis to match axis of osculating circle of an edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        break;
        case mmThreePointsPlane:
            return TwoStrings(QObject::tr("XY plane by 3 points","Attachment3D mode caption"),
                              QObject::tr("Align XY plane to pass through three vertices.","Attachment3D mode tooltip"));
        break;
        case mmThreePointsNormal:
            return TwoStrings(QObject::tr("XZ plane by 3 points","Attachment3D mode caption"),
                              QObject::tr("Align XZ plane to pass through 3 points; X axis will pass through two first points.","Attachment3D mode tooltip"));
        break;
        case mmFolding:
            return TwoStrings(QObject::tr("Folding","Attachment3D mode caption"),
                              QObject::tr("Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. XY plane will be aligned to folding the first edge.","Attachment3D mode tooltip"));
        break;
        }
    } else if (attacherType.isDerivedFrom(Attacher::AttachEnginePlane::getClassTypeId())){
        //---- Plane/sketch attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(QObject::tr("Deactivated","AttachmentPlane mode caption"),
                              QObject::tr("Attachment is disabled. Plane can be moved by editing Placement property.","AttachmentPlane mode tooltip"));
        break;
        case mmTranslate:
            return TwoStrings(QObject::tr("Translate origin","AttachmentPlane mode caption"),
                              QObject::tr("Origin is aligned to match Vertex. Orientation is controlled by Placement property.","AttachmentPlane mode tooltip"));
        break;
        case mmObjectXY:
            return TwoStrings(QObject::tr("Object's XY","AttachmentPlane mode caption"),
                              QObject::tr("Plane is aligned to XY local plane of linked object.","AttachmentPlane mode tooltip"));
        break;
        case mmObjectXZ:
            return TwoStrings(QObject::tr("Object's XZ","AttachmentPlane mode caption"),
                              QObject::tr("Plane is aligned to XZ local plane of linked object.","AttachmentPlane mode tooltip"));
        break;
        case mmObjectYZ:
            return TwoStrings(QObject::tr("Object's  YZ","AttachmentPlane mode caption"),
                              QObject::tr("Plane is aligned to YZ local plane of linked object.","AttachmentPlane mode tooltip"));
        break;
        case mmFlatFace:
            return TwoStrings(QObject::tr("Plane face","AttachmentPlane mode caption"),
                              QObject::tr("Plane is aligned to coincide planar face.","AttachmentPlane mode tooltip"));
        break;
        case mmTangentPlane:
            return TwoStrings(QObject::tr("Tangent to surface","AttachmentPlane mode caption"),
                              QObject::tr("Plane is made tangent to surface at vertex.","AttachmentPlane mode tooltip"));
        break;
        case mmNormalToPath:
            return TwoStrings(QObject::tr("Normal to edge","AttachmentPlane mode caption"),
                              QObject::tr("Plane is made tangent to edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmFrenetNB:
            return TwoStrings(QObject::tr("Frenet NB","AttachmentPlane mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmFrenetTN:
            return TwoStrings(QObject::tr("Frenet TN","AttachmentPlane mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmFrenetTB:
            return TwoStrings(QObject::tr("Frenet TB","AttachmentPlane mode caption"),
                              QObject::tr("Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmConcentric:
            return TwoStrings(QObject::tr("Concentric","AttachmentPlane mode caption"),
                              QObject::tr("Align to plane to osculating circle of an edge. Origin is aligned to point of curvature. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmRevolutionSection:
            return TwoStrings(QObject::tr("Revolution Section","AttachmentPlane mode caption"),
                              QObject::tr("Plane is prependicular to edge, and Y axis is matched with axis of osculating circle. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        break;
        case mmThreePointsPlane:
            return TwoStrings(QObject::tr("Plane by 3 points","AttachmentPlane mode caption"),
                              QObject::tr("Align plane to pass through three vertices.","AttachmentPlane mode tooltip"));
        break;
        case mmThreePointsNormal:
            return TwoStrings(QObject::tr("Normal to 3 points","AttachmentPlane mode caption"),
                              QObject::tr("Plane will pass through first to vertices, and perpendicular to plane that passes through three vertices.","AttachmentPlane mode tooltip"));
        break;
        case mmFolding:
            return TwoStrings(QObject::tr("Folding","AttachmentPlane mode caption"),
                              QObject::tr("Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. Plane will be aligned to folding the first edge.","AttachmentPlane mode tooltip"));
        break;
        }
    } else if (attacherType.isDerivedFrom(Attacher::AttachEngineLine::getClassTypeId())){
        //---- Line attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(QObject::tr("Deactivated","AttachmentLine mode caption"),
                              QObject::tr("Attachment is disabled. Line can be moved by editing Placement property.","AttachmentLine mode tooltip"));
        break;
        case mm1AxisX:
            return TwoStrings(QObject::tr("Object's X","AttachmentLine mode caption"),
                              QObject::tr("Line is aligned along local X axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        break;
        case mm1AxisY:
            return TwoStrings(QObject::tr("Object's Y","AttachmentLine mode caption"),
                              QObject::tr("Line is aligned along local Y axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        break;
        case mm1AxisZ:
            return TwoStrings(QObject::tr("Object's Z","AttachmentLine mode caption"),
                              QObject::tr("Line is aligned along local X axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        break;
        case mm1AxisCurv:
            return TwoStrings(QObject::tr("Axis of curvature","AttachmentLine mode caption"),
                              QObject::tr("Line that is an axis of osculating circle of curved edge. Optional vertex defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1Directrix1:
            return TwoStrings(QObject::tr("Directrix1","AttachmentLine mode caption"),
                              QObject::tr("Directrix line for ellipse, parabola, hyperbola.","AttachmentLine mode tooltip"));
        break;
        case mm1Directrix2:
            return TwoStrings(QObject::tr("Directrix2","AttachmentLine mode caption"),
                              QObject::tr("Second directrix line for ellipse and hyperbola.","AttachmentLine mode tooltip"));
        break;
        case mm1Asymptote1:
            return TwoStrings(QObject::tr("Asymptote1","AttachmentLine mode caption"),
                              QObject::tr("Asymptote of a hyperbola.","AttachmentLine mode tooltip"));
        break;
        case mm1Asymptote2:
            return TwoStrings(QObject::tr("Asymptote2","AttachmentLine mode caption"),
                              QObject::tr("Second asymptote of hyperbola.","AttachmentLine mode tooltip"));
        break;
        case mm1Tangent:
            return TwoStrings(QObject::tr("Tangent","AttachmentLine mode caption"),
                              QObject::tr("Line tangent to an edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1Normal:
            return TwoStrings(QObject::tr("Normal","AttachmentLine mode caption"),
                              QObject::tr("Align to N vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1Binormal:
            return TwoStrings(QObject::tr("Binormal","AttachmentLine mode caption"),
                              QObject::tr("Align to B vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1TangentU:
            return TwoStrings(QObject::tr("Tangent to surface (U)","AttachmentLine mode caption"),
                              QObject::tr("Tangent to surface, along U parameter. Vertex link defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1TangentV:
            return TwoStrings(QObject::tr("Tangent to surface (V)","AttachmentLine mode caption"),
                              QObject::tr("Tangent to surface, along U parameter. Vertex link defines where.","AttachmentLine mode tooltip"));
        break;
        case mm1TwoPoints:
            return TwoStrings(QObject::tr("Through two points","AttachmentLine mode caption"),
                              QObject::tr("Line that passes through two vertices.","AttachmentLine mode tooltip"));
        break;
        case mm1Intersection:
            return TwoStrings(QObject::tr("Intersection","AttachmentLine mode caption"),
                              QObject::tr("Not implemented.","AttachmentLine mode tooltip"));
        break;
        case mm1Proximity:
            return TwoStrings(QObject::tr("Proximity line","AttachmentLine mode caption"),
                              QObject::tr("Line that spans the shortest distance between shapes.","AttachmentLine mode tooltip"));
        break;
        }
    } else if (attacherType.isDerivedFrom(Attacher::AttachEnginePoint::getClassTypeId())){
        //---- Point attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(QObject::tr("Deactivated","AttachmentPoint mode caption"),
                              QObject::tr("Attachment is disabled. Point can be moved by editing Placement property.","AttachmentPoint mode tooltip"));
        break;
        case mm0Origin:
            return TwoStrings(QObject::tr("Object's origin","AttachmentPoint mode caption"),
                              QObject::tr("Point is put at object's Placement.Position. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentPoint mode tooltip"));
        break;
        case mm0Focus1:
            return TwoStrings(QObject::tr("Focus1","AttachmentPoint mode caption"),
                              QObject::tr("Focus of ellipse, parabola, hyperbola.","AttachmentPoint mode tooltip"));
        break;
        case mm0Focus2:
            return TwoStrings(QObject::tr("Focus2","AttachmentPoint mode caption"),
                              QObject::tr("Second focus of ellipse and hyperbola.","AttachmentPoint mode tooltip"));
        break;
        case mm0OnEdge:
            return TwoStrings(QObject::tr("On edge","AttachmentPoint mode caption"),
                              QObject::tr("Point is put on edge, MapPathParametr controls where. Additionally, vertex can be linked in for making a projection.","AttachmentPoint mode tooltip"));
        break;
        case mm0CenterOfCurvature:
            return TwoStrings(QObject::tr("Center of curvature","AttachmentPoint mode caption"),
                              QObject::tr("Center of osculating circle of an edge. Optinal vertex link defines where.","AttachmentPoint mode tooltip"));
        break;
        case mm0CenterOfMass:
            return TwoStrings(QObject::tr("Center of mass","AttachmentPoint mode caption"),
                              QObject::tr("Not implemented","AttachmentPoint mode tooltip"));
        break;
        case mm0Intersection:
            return TwoStrings(QObject::tr("Intersection","AttachmentPoint mode caption"),
                              QObject::tr("Not implemented","AttachmentPoint mode tooltip"));
        break;
        case mm0Vertex:
            return TwoStrings(QObject::tr("Vertex","AttachmentPoint mode caption"),
                              QObject::tr("Put Datum point coincident with another vertex.","AttachmentPoint mode tooltip"));
        break;
        case mm0ProximityPoint1:
            return TwoStrings(QObject::tr("Proximity point 1","AttachmentPoint mode caption"),
                              QObject::tr("Point on first reference that is closest to second reference.","AttachmentPoint mode tooltip"));
        break;
        case mm0ProximityPoint2:
            return TwoStrings(QObject::tr("Proximity point 2","AttachmentPoint mode caption"),
                              QObject::tr("Point on second reference that is closest to first reference.","AttachmentPoint mode tooltip"));
        break;
        }
    }

    assert("No user-friendly string defined for this attachment mode."=="");
    return TwoStrings(QString::fromLatin1(Attacher::AttachEngine::getModeName(mmode).c_str()),QString());
}


QString getShapeTypeText(eRefType type)
{
    //get rid of flags in type
    type = eRefType(type & (rtFlagHasPlacement - 1));

    const char* eRefTypeStrings[] = {
        QT_TR_NOOP("Any"),
        QT_TR_NOOP("Vertex"),
        QT_TR_NOOP("Edge"),
        QT_TR_NOOP("Face"),

        QT_TR_NOOP("Line"),
        QT_TR_NOOP("Curve"),
        QT_TR_NOOP("Circle"),
        QT_TR_NOOP("Conic"),
        QT_TR_NOOP("Ellipse"),
        QT_TR_NOOP("Parabola"),
        QT_TR_NOOP("Hyperbola"),

        QT_TR_NOOP("Plane"),
        QT_TR_NOOP("Sphere"),
        QT_TR_NOOP("Revolve"),
        QT_TR_NOOP("Cylinder"),
        QT_TR_NOOP("Torus"),
        QT_TR_NOOP("Cone"),
        //
        QT_TR_NOOP("Object"),
        QT_TR_NOOP("Solid"),
        QT_TR_NOOP("Wire"),
        NULL
    };

    if (type >= 0 && type < rtDummy_numberOfShapeTypes)
        if (eRefTypeStrings[int(type)])
            return QObject::tr(eRefTypeStrings[int(type)]);
    throw Base::Exception("getShTypeText: type value is wrong, or a string is missing in the list");
}

} //namespace AttacherGui
