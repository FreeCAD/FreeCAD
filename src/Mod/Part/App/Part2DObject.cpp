/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Curve.hxx>
# include <Geom2dAPI_InterCurveCurve.hxx>
# include <Geom2dAPI_ProjectPointOnCurve.hxx>
# include <GeomAPI.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif



#include "Part2DObject.h"
#include "Geometry.h"


using namespace Part;

const int Part2DObject::H_Axis = -1;
const int Part2DObject::V_Axis = -2;

PROPERTY_SOURCE(Part::Part2DObject, Part::Feature)


Part2DObject::Part2DObject()
{
     ADD_PROPERTY_TYPE(Support,(0),   "2D",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");
}


App::DocumentObjectExecReturn *Part2DObject::execute(void)
{
    return App::DocumentObject::StdReturn;
}

void Part2DObject::positionBySupport(void)
{
    // recalculate support:
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return;

    Base::Placement Place = part->Placement.getValue();
    const std::vector<std::string> &sub = Support.getSubValues();
    assert(sub.size()==1);
    // get the selected sub shape (a Face)
    const Part::TopoShape &shape = part->Shape.getShape();
    if (shape._Shape.IsNull())
        throw Base::Exception("Support shape is empty!");
    TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
    const TopoDS_Face &face = TopoDS::Face(sh);
    if (face.IsNull())
        throw Base::Exception("Null face in Part2DObject::positionBySupport()!");

    BRepAdaptor_Surface adapt(face);
    if (adapt.GetType() != GeomAbs_Plane)
        throw Base::Exception("No planar face in Part2DObject::positionBySupport()!");

    bool Reverse = false;
    if (face.Orientation() == TopAbs_REVERSED)
        Reverse = true;

    gp_Pln plane = adapt.Plane();
    Standard_Boolean ok = plane.Direct();
    if (!ok) {
        // toggle if plane has a left-handed coordinate system
        plane.UReverse();
        Reverse = !Reverse;
    }

    gp_Ax1 Normal = plane.Axis();
    if (Reverse)
        Normal.Reverse();

    gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);

    Handle (Geom_Plane) gPlane = new Geom_Plane(plane);
    GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
    gp_Pnt SketchBasePoint = projector.NearestPoint();

    gp_Dir dir = Normal.Direction();
    gp_Ax3 SketchPos;

    Base::Vector3d dX,dY,dZ;
    Place.getRotation().multVec(Base::Vector3d(1,0,0),dX);
    Place.getRotation().multVec(Base::Vector3d(0,1,0),dY);
    Place.getRotation().multVec(Base::Vector3d(0,0,1),dZ);
    gp_Dir dirX(dX.x, dX.y, dX.z);
    gp_Dir dirY(dY.x, dY.y, dY.z);
    gp_Dir dirZ(dZ.x, dZ.y, dZ.z);
    double cosNX = dir.Dot(dirX);
    double cosNY = dir.Dot(dirY);
    double cosNZ = dir.Dot(dirZ);
    std::vector<double> cosXYZ;
    cosXYZ.push_back(fabs(cosNX));
    cosXYZ.push_back(fabs(cosNY));
    cosXYZ.push_back(fabs(cosNZ));

    int pos = std::max_element(cosXYZ.begin(), cosXYZ.end()) - cosXYZ.begin();

    // +X/-X
    if (pos == 0) {
        if (cosNX > 0)
            SketchPos = gp_Ax3(SketchBasePoint, dir, dirY);
        else
            SketchPos = gp_Ax3(SketchBasePoint, dir, -dirY);
    }
    // +Y/-Y
    else if (pos == 1) {
        if (cosNY > 0)
            SketchPos = gp_Ax3(SketchBasePoint, dir, -dirX);
        else
            SketchPos = gp_Ax3(SketchBasePoint, dir, dirX);
    }
    // +Z/-Z
    else {
        SketchPos = gp_Ax3(SketchBasePoint, dir, dirX);
    }

    gp_Trsf Trf;
    Trf.SetTransformation(SketchPos);
    Trf.Invert();

    Base::Matrix4D mtrx;

    gp_Mat m = Trf._CSFDB_Getgp_Trsfmatrix();
    gp_XYZ p = Trf._CSFDB_Getgp_Trsfloc();
    Standard_Real scale = 1.0;

    // set Rotation matrix
    mtrx[0][0] = scale * m._CSFDB_Getgp_Matmatrix(0,0);
    mtrx[0][1] = scale * m._CSFDB_Getgp_Matmatrix(0,1);
    mtrx[0][2] = scale * m._CSFDB_Getgp_Matmatrix(0,2);

    mtrx[1][0] = scale * m._CSFDB_Getgp_Matmatrix(1,0);
    mtrx[1][1] = scale * m._CSFDB_Getgp_Matmatrix(1,1);
    mtrx[1][2] = scale * m._CSFDB_Getgp_Matmatrix(1,2);

    mtrx[2][0] = scale * m._CSFDB_Getgp_Matmatrix(2,0);
    mtrx[2][1] = scale * m._CSFDB_Getgp_Matmatrix(2,1);
    mtrx[2][2] = scale * m._CSFDB_Getgp_Matmatrix(2,2);

    // set pos vector
    mtrx[0][3] = p._CSFDB_Getgp_XYZx();
    mtrx[1][3] = p._CSFDB_Getgp_XYZy();
    mtrx[2][3] = p._CSFDB_Getgp_XYZz();

    // check the angle against the Z Axis
    //Standard_Real a = Normal.Angle(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)));

    Placement.setValue(Base::Placement(mtrx));
}

int Part2DObject::getAxisCount(void) const
{
    return 0;
}

Base::Axis Part2DObject::getAxis(int axId) const
{
    return Base::Axis();
}

bool Part2DObject::seekTrimPoints(const std::vector<Geometry *> &geomlist,
                                  int GeoId, const Base::Vector3d &point,
                                  int &GeoId1, Base::Vector3d &intersect1,
                                  int &GeoId2, Base::Vector3d &intersect2)
{
    if (GeoId >= int(geomlist.size()))
        return false;

    gp_Pln plane(gp_Pnt(0,0,0),gp_Dir(0,0,1));

    Standard_Boolean periodic=Standard_False;
    double period;
    Handle_Geom2d_Curve primaryCurve;
    Handle_Geom_Geometry geom = (geomlist[GeoId])->handle();
    Handle_Geom_Curve curve3d = Handle_Geom_Curve::DownCast(geom);
    if (curve3d.IsNull())
        return false;
    else {
        primaryCurve = GeomAPI::To2d(curve3d, plane);
        periodic = primaryCurve->IsPeriodic();
        if (periodic)
            period = primaryCurve->Period();
    }

    // create the intersector and projector functions
    Geom2dAPI_InterCurveCurve Intersector;
    Geom2dAPI_ProjectPointOnCurve Projector;

    // find the parameter of the picked point on the primary curve
    Projector.Init(gp_Pnt2d(point.x, point.y), primaryCurve);
    double pickedParam = Projector.LowerDistanceParameter();

    // find intersection points
    GeoId1 = -1;
    GeoId2 = -1;
    double param1=-1e10,param2=1e10;
    gp_Pnt2d p1,p2;
    Handle_Geom2d_Curve secondaryCurve;
    for (int id=0; id < int(geomlist.size()); id++) {
        if (id != GeoId && !geomlist[id]->Construction) {
            geom = (geomlist[id])->handle();
            curve3d = Handle_Geom_Curve::DownCast(geom);
            if (!curve3d.IsNull()) {
                secondaryCurve = GeomAPI::To2d(curve3d, plane);
                // perform the curves intersection
                Intersector.Init(primaryCurve, secondaryCurve, 1.0e-12);
                for (int i=1; i <= Intersector.NbPoints(); i++) {
                    gp_Pnt2d p = Intersector.Point(i);
                    // get the parameter of the intersection point on the primary curve
                    Projector.Init(p, primaryCurve);
                    double param = Projector.LowerDistanceParameter();
                    if (periodic) {
                        // transfer param into the interval (pickedParam-period pickedParam]
                        param = param - period * ceil((param-pickedParam) / period);
                        if (param > param1) {
                            param1 = param;
                            p1 = p;
                            GeoId1 = id;
                        }
                        param -= period; // transfer param into the interval (pickedParam pickedParam+period]
                        if (param < param2) {
                            param2 = param;
                            p2 = p;
                            GeoId2 = id;
                        }
                    }
                    else if (param < pickedParam && param > param1) {
                        param1 = param;
                        p1 = p;
                        GeoId1 = id;
                    }
                    else if (param > pickedParam && param < param2) {
                        param2 = param;
                        p2 = p;
                        GeoId2 = id;
                    }
                }
            }
        }
    }

    if (periodic) {
        // in case both points coincide, cancel the selection of one of both
        if (abs(param2-param1-period) < 1e-10) {
            if (param2 - pickedParam >= pickedParam - param1)
                GeoId2 = -1;
            else
                GeoId1 = -1;
        }
    }

   if (GeoId1 < 0 && GeoId2 < 0)
       return false;

   if (GeoId1 >= 0)
       intersect1 = Base::Vector3d(p1.X(),p1.Y(),0.f);
   if (GeoId2 >= 0)
       intersect2 = Base::Vector3d(p2.X(),p2.Y(),0.f);
   return true;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
  PROPERTY_SOURCE_TEMPLATE(Part::Part2DObjectPython, Part::Part2DObject)
  template<> const char* Part::Part2DObjectPython::getViewProviderName(void) const {
    return "PartGui::ViewProvider2DObjectPython";
}
/// @endcond

// explicit template instantiation
  template class PartExport FeaturePythonT<Part::Part2DObject>;
}

