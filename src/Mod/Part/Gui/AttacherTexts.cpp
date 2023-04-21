/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
# include <QApplication>
#endif

#include <Base/Console.h>

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

TextSet getUIStrings(Base::Type attacherType, eMapMode mmode)
{
    if (attacherType.isDerivedFrom(AttachEngine3D::getClassTypeId())){
        //---- coordinate system attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(qApp->translate("Attacher3D", "Deactivated","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Attachment is disabled. Object can be moved by editing Placement property.","Attachment3D mode tooltip"));
        case mmTranslate:
            return TwoStrings(qApp->translate("Attacher3D", "Translate origin","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Origin is aligned to match Vertex. Orientation is controlled by Placement property.","Attachment3D mode tooltip"));
        case mmObjectXY:
            return TwoStrings(qApp->translate("Attacher3D", "Object's X Y Z","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Placement is made equal to Placement of linked object.","Attachment3D mode tooltip"));
        case mmObjectXZ:
            return TwoStrings(qApp->translate("Attacher3D", "Object's X Z Y","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "X', Y', Z' axes are matched with object's local X, Z, -Y, respectively.","Attachment3D mode tooltip"));
        case mmObjectYZ:
            return TwoStrings(qApp->translate("Attacher3D", "Object's Y Z X","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "X', Y', Z' axes are matched with object's local Y, Z, X, respectively.","Attachment3D mode tooltip"));
        case mmFlatFace:
            return TwoStrings(qApp->translate("Attacher3D", "XY on plane","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "X' Y' plane is aligned to coincide planar face.","Attachment3D mode tooltip"));
        case mmTangentPlane:
            return TwoStrings(qApp->translate("Attacher3D", "XY tangent to surface","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "X' Y' plane is made tangent to surface at vertex.","Attachment3D mode tooltip"));
        case mmNormalToPath:
            return TwoStrings(qApp->translate("Attacher3D", "Z tangent to edge","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Z' axis is aligned to be tangent to edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmFrenetNB:
            return TwoStrings(qApp->translate("Attacher3D", "Frenet NBT","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmFrenetTN:
            return TwoStrings(qApp->translate("Attacher3D", "Frenet TNB","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmFrenetTB:
            return TwoStrings(qApp->translate("Attacher3D", "Frenet TBN","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmConcentric:
            return TwoStrings(qApp->translate("Attacher3D", "Concentric","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align XY plane to osculating circle of an edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmRevolutionSection:
            return TwoStrings(qApp->translate("Attacher3D", "Revolution Section","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align Y' axis to match axis of osculating circle of an edge. Optional vertex link defines where.","Attachment3D mode tooltip"));
        case mmThreePointsPlane:
            return TwoStrings(qApp->translate("Attacher3D", "XY plane by 3 points","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align XY plane to pass through three vertices.","Attachment3D mode tooltip"));
        case mmThreePointsNormal:
            return TwoStrings(qApp->translate("Attacher3D", "XZ plane by 3 points","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Align XZ plane to pass through 3 points; X axis will pass through two first points.","Attachment3D mode tooltip"));
        case mmFolding:
            return TwoStrings(qApp->translate("Attacher3D", "Folding","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. XY plane will be aligned to folding the first edge.","Attachment3D mode tooltip"));
        case mmInertialCS:
            return TwoStrings(qApp->translate("Attacher3D", "Inertial CS","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Inertial coordinate system, constructed on principal axes of inertia and center of mass.","Attachment3D mode tooltip"));
        case mmOZX:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Z-X","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align Z' and X' axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOZY:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Z-Y","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align Z' and Y' axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOXY:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-X-Y","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align X' and Y' axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOXZ:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-X-Z","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align X' and Z' axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOYZ:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Y-Z","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align Y' and Z' axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOYX:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Y-X","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align Y' and X' axes towards vertex/along line.","Attachment3D mode tooltip"));
        default:
            break;
        }
    } else if (attacherType.isDerivedFrom(AttachEnginePlane::getClassTypeId())){
        //---- Plane/sketch attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(qApp->translate("Attacher2D", "Deactivated","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Attachment is disabled. Object can be moved by editing Placement property.","AttachmentPlane mode tooltip"));
        case mmTranslate:
            return TwoStrings(qApp->translate("Attacher2D", "Translate origin","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Origin is aligned to match Vertex. Orientation is controlled by Placement property.","AttachmentPlane mode tooltip"));
        case mmObjectXY:
            return TwoStrings(qApp->translate("Attacher2D", "Object's XY","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is aligned to XY local plane of linked object.","AttachmentPlane mode tooltip"));
        case mmObjectXZ:
            return TwoStrings(qApp->translate("Attacher2D", "Object's XZ","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is aligned to XZ local plane of linked object.","AttachmentPlane mode tooltip"));
        case mmObjectYZ:
            return TwoStrings(qApp->translate("Attacher2D", "Object's YZ","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is aligned to YZ local plane of linked object.","AttachmentPlane mode tooltip"));
        case mmFlatFace:
            return TwoStrings(qApp->translate("Attacher2D", "Plane face","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is aligned to coincide planar face.","AttachmentPlane mode tooltip"));
        case mmTangentPlane:
            return TwoStrings(qApp->translate("Attacher2D", "Tangent to surface","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is made tangent to surface at vertex.","AttachmentPlane mode tooltip"));
        case mmNormalToPath:
            return TwoStrings(qApp->translate("Attacher2D", "Normal to edge","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is made tangent to edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmFrenetNB:
            return TwoStrings(qApp->translate("Attacher2D", "Frenet NB","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmFrenetTN:
            return TwoStrings(qApp->translate("Attacher2D", "Frenet TN","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmFrenetTB:
            return TwoStrings(qApp->translate("Attacher2D", "Frenet TB","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmConcentric:
            return TwoStrings(qApp->translate("Attacher2D", "Concentric","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Align to plane to osculating circle of an edge. Origin is aligned to point of curvature. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmRevolutionSection:
            return TwoStrings(qApp->translate("Attacher2D", "Revolution Section","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane is perpendicular to edge, and Y axis is matched with axis of osculating circle. Optional vertex link defines where.","AttachmentPlane mode tooltip"));
        case mmThreePointsPlane:
            return TwoStrings(qApp->translate("Attacher2D", "Plane by 3 points","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Align plane to pass through three vertices.","AttachmentPlane mode tooltip"));
        case mmThreePointsNormal:
            return TwoStrings(qApp->translate("Attacher2D", "Normal to 3 points","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane will pass through first two vertices, and perpendicular to plane that passes through three vertices.","AttachmentPlane mode tooltip"));
        case mmFolding:
            return TwoStrings(qApp->translate("Attacher2D", "Folding","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. Plane will be aligned to folding the first edge.","AttachmentPlane mode tooltip"));
        case mmInertialCS:
            return TwoStrings(qApp->translate("Attacher2D", "Inertia 2-3","AttachmentPlane mode caption"),
                              qApp->translate("Attacher2D", "Plane constructed on second and third principal axes of inertia (passes through center of mass).","AttachmentPlane mode tooltip"));
        case mmOZX:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-N-X","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align normal and horizontal plane axis towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOZY:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-N-Y","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align normal and vertical plane axis towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOXY:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-X-Y","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align horizontal and vertical plane axes towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOXZ:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-X-N","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align horizontal plane axis and normal towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOYZ:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Y-N","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align vertical plane axis and normal towards vertex/along line.","Attachment3D mode tooltip"));
        case mmOYX:
            return TwoStrings(qApp->translate("Attacher3D", "Align O-Y-X","Attachment3D mode caption"),
                              qApp->translate("Attacher3D", "Match origin with first Vertex. Align vertical and horizontal plane axes towards vertex/along line.","Attachment3D mode tooltip"));
        default:
            break;
        }
    } else if (attacherType.isDerivedFrom(AttachEngineLine::getClassTypeId())){
        //---- Line attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(qApp->translate("Attacher1D", "Deactivated","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Attachment is disabled. Line can be moved by editing Placement property.","AttachmentLine mode tooltip"));
        case mm1AxisX:
            return TwoStrings(qApp->translate("Attacher1D", "Object's X","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line is aligned along local X axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        case mm1AxisY:
            return TwoStrings(qApp->translate("Attacher1D", "Object's Y","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line is aligned along local Y axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        case mm1AxisZ:
            return TwoStrings(qApp->translate("Attacher1D", "Object's Z","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line is aligned along local X axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentLine mode tooltip"));
        case mm1AxisCurv:
            return TwoStrings(qApp->translate("Attacher1D", "Axis of curvature","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line that is an axis of osculating circle of curved edge. Optional vertex defines where.","AttachmentLine mode tooltip"));
        case mm1Directrix1:
            return TwoStrings(qApp->translate("Attacher1D", "Directrix1","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Directrix line for ellipse, parabola, hyperbola.","AttachmentLine mode tooltip"));
        case mm1Directrix2:
            return TwoStrings(qApp->translate("Attacher1D", "Directrix2","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Second directrix line for ellipse and hyperbola.","AttachmentLine mode tooltip"));
        case mm1Asymptote1:
            return TwoStrings(qApp->translate("Attacher1D", "Asymptote1","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Asymptote of a hyperbola.","AttachmentLine mode tooltip"));
        case mm1Asymptote2:
            return TwoStrings(qApp->translate("Attacher1D", "Asymptote2","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Second asymptote of hyperbola.","AttachmentLine mode tooltip"));
        case mm1Tangent:
            return TwoStrings(qApp->translate("Attacher1D", "Tangent","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line tangent to an edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        case mm1Normal:
            return TwoStrings(qApp->translate("Attacher1D", "Normal to edge","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Align to N vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        case mm1Binormal:
            return TwoStrings(qApp->translate("Attacher1D", "Binormal","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Align to B vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.","AttachmentLine mode tooltip"));
        case mm1TangentU:
            return TwoStrings(qApp->translate("Attacher1D", "Tangent to surface (U)","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Tangent to surface, along U parameter. Vertex link defines where.","AttachmentLine mode tooltip"));
        case mm1TangentV:
            return TwoStrings(qApp->translate("Attacher1D", "Tangent to surface (V)","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Tangent to surface, along U parameter. Vertex link defines where.","AttachmentLine mode tooltip"));
        case mm1TwoPoints:
            return TwoStrings(qApp->translate("Attacher1D", "Through two points","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line that passes through two vertices.","AttachmentLine mode tooltip"));
        case mm1Intersection:
            return TwoStrings(qApp->translate("Attacher1D", "Intersection","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Not implemented.","AttachmentLine mode tooltip"));
        case mm1Proximity:
            return TwoStrings(qApp->translate("Attacher1D", "Proximity line","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line that spans the shortest distance between shapes.","AttachmentLine mode tooltip"));
        case mm1AxisInertia1:
            return TwoStrings(qApp->translate("Attacher1D", "1st principal axis","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line follows first principal axis of inertia.","AttachmentLine mode tooltip"));
        case mm1AxisInertia2:
            return TwoStrings(qApp->translate("Attacher1D", "2nd principal axis","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line follows second principal axis of inertia.","AttachmentLine mode tooltip"));
        case mm1AxisInertia3:
            return TwoStrings(qApp->translate("Attacher1D", "3rd principal axis","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line follows third principal axis of inertia.","AttachmentLine mode tooltip"));
        case mm1FaceNormal:
            return TwoStrings(qApp->translate("Attacher1D", "Normal to surface","AttachmentLine mode caption"),
                              qApp->translate("Attacher1D", "Line perpendicular to surface at point set by vertex.","AttachmentLine mode tooltip"));
        default:
            break;
        }
    } else if (attacherType.isDerivedFrom(AttachEnginePoint::getClassTypeId())){
        //---- Point attacher ----
        switch (mmode){
        case mmDeactivated:
            return TwoStrings(qApp->translate("Attacher0D", "Deactivated","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Attachment is disabled. Point can be moved by editing Placement property.","AttachmentPoint mode tooltip"));
        case mm0Origin:
            return TwoStrings(qApp->translate("Attacher0D", "Object's origin","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Point is put at object's Placement.Position. Works on objects with placements, and ellipse/parabola/hyperbola edges.","AttachmentPoint mode tooltip"));
        case mm0Focus1:
            return TwoStrings(qApp->translate("Attacher0D", "Focus1","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Focus of ellipse, parabola, hyperbola.","AttachmentPoint mode tooltip"));
        case mm0Focus2:
            return TwoStrings(qApp->translate("Attacher0D", "Focus2","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Second focus of ellipse and hyperbola.","AttachmentPoint mode tooltip"));
        case mm0OnEdge:
            return TwoStrings(qApp->translate("Attacher0D", "On edge","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Point is put on edge, MapPathParameter controls where. Additionally, vertex can be linked in for making a projection.","AttachmentPoint mode tooltip"));
        case mm0CenterOfCurvature:
            return TwoStrings(qApp->translate("Attacher0D", "Center of curvature","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Center of osculating circle of an edge. Optional vertex link defines where.","AttachmentPoint mode tooltip"));
        case mm0CenterOfMass:
            return TwoStrings(qApp->translate("Attacher0D", "Center of mass","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Center of mass of all references (equal densities are assumed).","AttachmentPoint mode tooltip"));
        case mm0Intersection:
            return TwoStrings(qApp->translate("Attacher0D", "Intersection","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Not implemented","AttachmentPoint mode tooltip"));
        case mm0Vertex:
            return TwoStrings(qApp->translate("Attacher0D", "Vertex","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Put Datum point coincident with another vertex.","AttachmentPoint mode tooltip"));
        case mm0ProximityPoint1:
            return TwoStrings(qApp->translate("Attacher0D", "Proximity point 1","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Point on first reference that is closest to second reference.","AttachmentPoint mode tooltip"));
        case mm0ProximityPoint2:
            return TwoStrings(qApp->translate("Attacher0D", "Proximity point 2","AttachmentPoint mode caption"),
                              qApp->translate("Attacher0D", "Point on second reference that is closest to first reference.","AttachmentPoint mode tooltip"));
        default:
            break;
        }
    }

    Base::Console().Warning("No user-friendly string defined for this attachment mode and attacher type: %s %s \n",AttachEngine::getModeName(mmode).c_str(), attacherType.getName());
    return TwoStrings(QString::fromStdString(AttachEngine::getModeName(mmode)), QString());
}

//Note: this list must be in sync with eRefType enum
static struct { const char *txt; const char *comment; } eRefTypeStrings[] = {
    QT_TRANSLATE_NOOP3("Attacher", "Any", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Vertex", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Edge", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Face", "Attacher reference type"),

    QT_TRANSLATE_NOOP3("Attacher", "Line", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Curve", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Circle", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Conic", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Ellipse", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Parabola", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Hyperbola", "Attacher reference type"),

    QT_TRANSLATE_NOOP3("Attacher", "Plane", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Sphere", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Revolve", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Cylinder", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Torus", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Cone", "Attacher reference type"),

    QT_TRANSLATE_NOOP3("Attacher", "Object", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Solid", "Attacher reference type"),
    QT_TRANSLATE_NOOP3("Attacher", "Wire", "Attacher reference type"),
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr}
};


QString getShapeTypeText(eRefType type)
{
    //get rid of flags in type
    type = eRefType(type & (rtFlagHasPlacement - 1));

    if (type >= 0 && type < rtDummy_numberOfShapeTypes){
        if (eRefTypeStrings[int(type)].txt){
            return qApp->translate("Attacher", eRefTypeStrings[int(type)].txt, eRefTypeStrings[int(type)].comment);
        }
    }

    throw Base::TypeError("getShTypeText: type value is wrong, or a string is missing in the list");
}

QStringList getRefListForMode(AttachEngine &attacher, eMapMode mmode)
{
    refTypeStringList list = attacher.modeRefTypes[mmode];
    QStringList strlist;
    for(refTypeString &rts : list){
        QStringList buf;
        for(eRefType rt : rts){
            buf.append(getShapeTypeText(rt));
        }
        strlist.append(buf.join(QString::fromLatin1(", ")));
    }
    return strlist;
}


// --------------------Py interface---------------------

PyObject* AttacherGuiPy::sGetModeStrings(PyObject * /*self*/, PyObject *args)
{
    int modeIndex = 0;
    char* attacherType;
    if (!PyArg_ParseTuple(args, "si", &attacherType, &modeIndex))
        return nullptr;

    try {
        Base::Type t = Base::Type::fromName(attacherType);
        if (! t.isDerivedFrom(AttachEngine::getClassTypeId())){
            std::stringstream ss;
            ss << "Object of this type is not derived from AttachEngine: ";
            ss << attacherType;
            throw Py::TypeError(ss.str());
        }
        TextSet strs = getUIStrings(t,eMapMode(modeIndex));
        Py::List result;
        for(QString &s : strs){
            QByteArray ba_utf8 = s.toUtf8();
            result.append(Py::String(ba_utf8.data(), "utf-8"));
        }

        return Py::new_reference_to(result);
    } catch (const Py::Exception&) {
        return nullptr;
    } catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject* AttacherGuiPy::sGetRefTypeUserFriendlyName(PyObject * /*self*/, PyObject *args)
{
    int refTypeIndex = 0;
    if (!PyArg_ParseTuple(args, "i", &refTypeIndex))
        return nullptr;

    try {
        QByteArray ba_utf8 = getShapeTypeText(eRefType(refTypeIndex)).toUtf8();
        return Py::new_reference_to(Py::String(ba_utf8.data(), "utf-8"));
    } catch (const Py::Exception&) {
        return nullptr;
    } catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}


PyMethodDef AttacherGuiPy::Methods[] = {
    {"getModeStrings",             (PyCFunction) AttacherGuiPy::sGetModeStrings, METH_VARARGS,
     "getModeStrings(attacher_type, mode_index) - gets mode user-friendly name and brief description."},
    {"getRefTypeUserFriendlyName", (PyCFunction) AttacherGuiPy::sGetRefTypeUserFriendlyName, METH_VARARGS,
     "getRefTypeUserFriendlyName(type_index) - gets user-friendly name of AttachEngine's shape type."},
    {nullptr, nullptr, 0, nullptr}  /* Sentinel */
};


} //namespace AttacherGui
