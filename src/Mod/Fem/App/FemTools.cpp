/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QFileInfo>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Precision.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#endif

#include <App/Application.h>

#include "FemTools.h"


Base::Vector3d Fem::Tools::getDirectionFromShape(const TopoDS_Shape& shape)
{
    gp_XYZ dir(0, 0, 0);

    // "Direction must be a planar face or linear edge"
    //
    if (shape.ShapeType() == TopAbs_FACE) {
        if (isPlanar(TopoDS::Face(shape))) {
            dir = getDirection(TopoDS::Face(shape));
        }
    }
    else if (shape.ShapeType() == TopAbs_EDGE) {
        if (isLinear(TopoDS::Edge(shape))) {
            dir = getDirection(TopoDS::Edge(shape));
        }
    }

    Base::Vector3d the_direction(dir.X(), dir.Y(), dir.Z());
    return the_direction;
}

bool Fem::Tools::isPlanar(const TopoDS_Face& face)
{
    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Plane) {
        return true;
    }
    else if (surface.GetType() == GeomAbs_BSplineSurface) {
        Handle(Geom_BSplineSurface) spline = surface.BSpline();
        try {
            TColgp_Array2OfPnt poles(1, spline->NbUPoles(), 1, spline->NbVPoles());
            spline->Poles(poles);

            // get the plane from three control points
            gp_Pnt p1 = poles(poles.LowerRow(), poles.LowerCol());
            gp_Pnt p2 = poles(poles.UpperRow(), poles.LowerCol());
            gp_Pnt p3 = poles(poles.LowerRow(), poles.UpperCol());
            gp_Vec vec1(p1, p2);
            gp_Vec vec2(p1, p3);
            gp_Vec vec3 = vec1.Crossed(vec2);
            gp_Pln plane(p1, gp_Dir(vec3));

            for (int i = poles.LowerRow(); i <= poles.UpperRow(); i++) {
                for (int j = poles.LowerCol(); j < poles.UpperCol(); j++) {
                    // are control points coplanar?
                    const gp_Pnt& pole = poles(i, j);
                    Standard_Real dist = plane.Distance(pole);
                    if (dist > Precision::Confusion()) {
                        return false;
                    }
                }
            }

            return true;
        }
        catch (Standard_Failure&) {
            return false;
        }
    }
    else if (surface.GetType() == GeomAbs_BezierSurface) {
        Handle(Geom_BezierSurface) bezier = surface.Bezier();
        try {
            TColgp_Array2OfPnt poles(1, bezier->NbUPoles(), 1, bezier->NbVPoles());
            bezier->Poles(poles);

            // get the plane from three control points
            gp_Pnt p1 = poles(poles.LowerRow(), poles.LowerCol());
            gp_Pnt p2 = poles(poles.UpperRow(), poles.LowerCol());
            gp_Pnt p3 = poles(poles.LowerRow(), poles.UpperCol());
            gp_Vec vec1(p1, p2);
            gp_Vec vec2(p1, p3);
            gp_Vec vec3 = vec1.Crossed(vec2);
            gp_Pln plane(p1, gp_Dir(vec3));

            for (int i = poles.LowerRow(); i <= poles.UpperRow(); i++) {
                for (int j = poles.LowerCol(); j < poles.UpperCol(); j++) {
                    // are control points coplanar?
                    const gp_Pnt& pole = poles(i, j);
                    Standard_Real dist = plane.Distance(pole);
                    if (dist > Precision::Confusion()) {
                        return false;
                    }
                }
            }

            return true;
        }
        catch (Standard_Failure&) {
            return false;
        }
    }

    return false;
}

gp_XYZ Fem::Tools::getDirection(const TopoDS_Face& face)
{
    gp_XYZ dir(0, 0, 0);

    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Plane) {
        dir = surface.Plane().Axis().Direction().XYZ();
    }
    else if (surface.GetType() == GeomAbs_BSplineSurface) {
        Handle(Geom_BSplineSurface) spline = surface.BSpline();
        try {
            TColgp_Array2OfPnt poles(1, spline->NbUPoles(), 1, spline->NbVPoles());
            spline->Poles(poles);

            // get the plane from three control points
            gp_Pnt p1 = poles(poles.LowerRow(), poles.LowerCol());
            gp_Pnt p2 = poles(poles.UpperRow(), poles.LowerCol());
            gp_Pnt p3 = poles(poles.LowerRow(), poles.UpperCol());
            gp_Vec vec1(p1, p2);
            gp_Vec vec2(p1, p3);
            gp_Vec vec3 = vec1.Crossed(vec2);
            gp_Pln plane(p1, gp_Dir(vec3));
            dir = plane.Axis().Direction().XYZ();
        }
        catch (Standard_Failure&) {
        }
    }
    else if (surface.GetType() == GeomAbs_BezierSurface) {
        Handle(Geom_BezierSurface) bezier = surface.Bezier();
        try {
            TColgp_Array2OfPnt poles(1, bezier->NbUPoles(), 1, bezier->NbVPoles());
            bezier->Poles(poles);

            // get the plane from three control points
            gp_Pnt p1 = poles(poles.LowerRow(), poles.LowerCol());
            gp_Pnt p2 = poles(poles.UpperRow(), poles.LowerCol());
            gp_Pnt p3 = poles(poles.LowerRow(), poles.UpperCol());
            gp_Vec vec1(p1, p2);
            gp_Vec vec2(p1, p3);
            gp_Vec vec3 = vec1.Crossed(vec2);
            gp_Pln plane(p1, gp_Dir(vec3));
            dir = plane.Axis().Direction().XYZ();
        }
        catch (Standard_Failure&) {
        }
    }

    return dir;
}

bool Fem::Tools::isLinear(const TopoDS_Edge& edge)
{
    BRepAdaptor_Curve curve(edge);
    if (curve.GetType() == GeomAbs_Line) {
        return true;
    }
    else if (curve.GetType() == GeomAbs_BSplineCurve) {
        Handle(Geom_BSplineCurve) spline = curve.BSpline();
        try {
            gp_Pnt s1 = spline->Pole(1);
            gp_Pnt sn = spline->Pole(spline->NbPoles());
            gp_Vec vec(s1, sn);
            gp_Lin line(s1, gp_Dir(vec));

            for (int i = 2; i < spline->NbPoles(); i++) {
                // are control points collinear?
                Standard_Real dist = line.Distance(spline->Pole(i));
                if (dist > Precision::Confusion()) {
                    return false;
                }
            }

            return true;
        }
        catch (Standard_Failure&) {
            return false;
        }
    }
    else if (curve.GetType() == GeomAbs_BezierCurve) {
        Handle(Geom_BezierCurve) bezier = curve.Bezier();
        try {
            gp_Pnt s1 = bezier->Pole(1);
            gp_Pnt sn = bezier->Pole(bezier->NbPoles());
            gp_Vec vec(s1, sn);
            gp_Lin line(s1, gp_Dir(vec));

            for (int i = 2; i < bezier->NbPoles(); i++) {
                // are control points collinear?
                Standard_Real dist = line.Distance(bezier->Pole(i));
                if (dist > Precision::Confusion()) {
                    return false;
                }
            }

            return true;
        }
        catch (Standard_Failure&) {
            return false;
        }
    }

    return false;
}

gp_XYZ Fem::Tools::getDirection(const TopoDS_Edge& edge)
{
    gp_XYZ dir(0, 0, 0);

    BRepAdaptor_Curve curve(edge);
    if (curve.GetType() == GeomAbs_Line) {
        dir = curve.Line().Direction().XYZ();
    }
    else if (curve.GetType() == GeomAbs_BSplineCurve) {
        Handle(Geom_BSplineCurve) spline = curve.BSpline();
        try {
            gp_Pnt s1 = spline->Pole(1);
            gp_Pnt sn = spline->Pole(spline->NbPoles());
            gp_Vec vec(s1, sn);
            gp_Lin line(s1, gp_Dir(vec));
            dir = line.Direction().XYZ();
        }
        catch (Standard_Failure&) {
        }
    }
    else if (curve.GetType() == GeomAbs_BezierCurve) {
        Handle(Geom_BezierCurve) bezier = curve.Bezier();
        try {
            gp_Pnt s1 = bezier->Pole(1);
            gp_Pnt sn = bezier->Pole(bezier->NbPoles());
            gp_Vec vec(s1, sn);
            gp_Lin line(s1, gp_Dir(vec));
            dir = line.Direction().XYZ();
        }
        catch (Standard_Failure&) {
        }
    }

    return dir;
}

// function to determine 3rd-party binaries used by the FEM WB
std::string Fem::Tools::checkIfBinaryExists(std::string prefSection,
                                            std::string prefBinaryName,
                                            std::string binaryName)
{
    // if "Search in known binary directories" is set in the preferences, we ignore custom path
    auto paramPath = "User parameter:BaseApp/Preferences/Mod/Fem/" + prefSection;
    auto knownDirectoriesString = "UseStandard" + prefSection + "Location";
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(paramPath.c_str());
    bool knownDirectories = hGrp->GetBool(knownDirectoriesString.c_str(), true);

    if (knownDirectories) {
#if defined(FC_OS_WIN32)
        binaryName = binaryName + ".exe";
#endif
        // first check the environment paths by QFileInfo
        if (QFileInfo::exists(QString::fromLatin1(binaryName.c_str()))) {
            return binaryName;
        }
        // check the folder of the FreeCAD binary
        else {
            auto homePathBinary = App::Application::getHomePath() + "bin/" + binaryName;
            if (QFileInfo::exists(QString::fromLatin1(homePathBinary.c_str()))) {
                return binaryName;
            }
        }
    }
    else {
        auto binaryPathString = prefBinaryName + "BinaryPath";
        ParameterGrp::handle hGrp =
            App::GetApplication().GetParameterGroupByPath(paramPath.c_str());
        auto binaryPath = hGrp->GetASCII(binaryPathString.c_str(), "");
        QFileInfo::exists(QString::fromLatin1(binaryPath.c_str()));
        if (QFileInfo::exists(QString::fromLatin1(binaryPath.c_str()))) {
            return binaryPath;
        }
    }
    return "";
}
