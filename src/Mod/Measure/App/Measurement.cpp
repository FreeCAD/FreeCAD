/***************************************************************************
 *   Copyright (c) 2013      Luke Parry <l.parry@warwick.ac.uk>            *
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
  #include <BRep_Builder.hxx>
  #include <TopoDS_Compound.hxx>
  # include <TopoDS_Shape.hxx>
  # include <TopoDS_Face.hxx>
  # include <TopoDS.hxx>
  # include <TopExp_Explorer.hxx>
  # include <gp_Pln.hxx>
  # include <gp_Ax3.hxx>
  # include <gp_Circ.hxx>
  # include <gp_Elips.hxx>
  # include <GCPnts_AbscissaPoint.hxx>
  # include <BRepAdaptor_Surface.hxx>
  # include <BRepAdaptor_Curve.hxx>
  # include <BRepExtrema_DistShapeShape.hxx>
  # include <GProp_GProps.hxx>
  # include <GeomAPI_ExtremaCurveCurve.hxx>
  # include <BRepGProp.hxx>
#endif


#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/VectorPy.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Measurement.h"
#include "MeasurementPy.h"

#ifndef M_PI
    #define M_PI    3.14159265358979323846 /* pi */
#endif
using namespace Measure;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Measure::Measurement, Base::BaseClass)

Measurement::Measurement()
{
    measureType = Invalid;
    References3D.setScope(App::LinkScope::Global);
}

Measurement::~Measurement()
{

}

void Measurement::clear()
{
    std::vector<App::DocumentObject*> Objects;
    std::vector<std::string> SubElements;
    References3D.setValues(Objects, SubElements);
    measureType = Invalid;
}

bool Measurement::has3DReferences()
{
    return (References3D.getSize() > 0);
}

//add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject *obj, const std::string& subName)
{
    return addReference3D(obj,subName.c_str());
}

///add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject *obj, const char* subName)
{
  std::vector<App::DocumentObject*> objects = References3D.getValues();
  std::vector<std::string> subElements = References3D.getSubValues();

  objects.push_back(obj);
  subElements.push_back(subName);

  References3D.setValues(objects, subElements);

  measureType = getType();
  return References3D.getSize();
}

MeasureType Measurement::getType()
{
    const std::vector<App::DocumentObject*> &objects = References3D.getValues();
    const std::vector<std::string> &subElements = References3D.getSubValues();

    std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
    std::vector<std::string>::const_iterator subEl = subElements.begin();

    //
    //int dims = -1;
    MeasureType mode;

    // Type of References3D
    int verts = 0;
    int edges = 0;
    int faces = 0;
    int vols = 0;

    for (;obj != objects.end(); ++obj, ++subEl) {

        // Check if solid object
        if(strcmp((*subEl).c_str(), "") == 0) {
            vols++;
        } else {

            TopoDS_Shape refSubShape;
            try {
                refSubShape = Part::Feature::getShape(*obj,(*subEl).c_str(),true);
                if(refSubShape.IsNull())
                    return Invalid;
            }
            catch (Standard_Failure& e) {
                std::stringstream errorMsg;
        
                errorMsg << "Measurement - getType - " << e.GetMessageString() << std::endl;
                throw Base::CADKernelError(e.GetMessageString());
            }

            switch (refSubShape.ShapeType()) {
              case TopAbs_VERTEX:
                {
                    verts++;
                }
                break;
              case TopAbs_EDGE:
                {
                    edges++;
                }
                break;
              case TopAbs_FACE:
                {
                    faces++;
                }
                break;
              default:
                break;
            }
        }
    }

    if(vols > 0) {
        if(verts > 0 || edges > 0 || faces > 0) {
            mode = Invalid;
        } else {
            mode = Volumes;
        }
    } else if(faces > 0) {
        if(verts > 0 || edges > 0) {
            if(faces > 1 && verts > 1 && edges > 0) {
                mode = Invalid;
            } else {
                // One Surface and One Point
                mode = PointToSurface;
            }
        } else {
            mode = Surfaces;
        }
    } else if(edges > 0) {
        if(verts > 0) {
            if(verts > 1 && edges > 0) {
                mode = Invalid;
            } else {
                mode = PointToEdge;
            }
        } else {
//             if(edges == 2) {
//                 mode = EdgeToEdge;
//             } else {
                mode = Edges;
//             }
        }
    } else if (verts > 0) {
        mode = Points;
    } else {
        mode = Invalid;
    }

    return mode;
}

TopoDS_Shape Measurement::getShape(App::DocumentObject *obj , const char *subName) const
{
    try {
        auto shape = Part::Feature::getShape(obj,subName,true);
        if(shape.IsNull())
            throw Part::NullShapeException("null shape");
        return shape;
    } catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

//TODO:: add lengthX, lengthY (and lengthZ??) support
// Methods for distances (edge length, two points, edge and a point
double Measurement::length() const
{
    double result = 0.0;
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        Base::Console().Error("Measurement::length - No 3D references available\n");
    } else if (measureType == Invalid) {
        Base::Console().Error("Measurement::length - measureType is Invalid\n");
    } else {
        const std::vector<App::DocumentObject*> &objects = References3D.getValues();
        const std::vector<std::string> &subElements = References3D.getSubValues();

        if(measureType == Points ||
           measureType == PointToEdge ||
           measureType == PointToSurface)  {

            Base::Vector3d diff = this->delta();
            //return diff.Length();
            result = diff.Length();
        } else if(measureType == Edges) {

            // Iterate through edges and calculate each length
            std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
            std::vector<std::string>::const_iterator subEl = subElements.begin();

            for (;obj != objects.end(); ++obj, ++subEl) {
                //const Part::Feature *refObj = static_cast<const Part::Feature*>((*obj));
                //const Part::TopoShape& refShape = refObj->Shape.getShape();
                // Get the length of one edge
                TopoDS_Shape shape = getShape(*obj, (*subEl).c_str());
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve(edge);

                switch(curve.GetType()) {
                    case GeomAbs_Line : {
                        gp_Pnt P1 = curve.Value(curve.FirstParameter());
                        gp_Pnt P2 = curve.Value(curve.LastParameter());
                        gp_XYZ diff = P2.XYZ() - P1.XYZ();
                        result += diff.Modulus();
                        break;
                    }
                    case GeomAbs_Circle : {
                        double u = curve.FirstParameter();
                        double v = curve.LastParameter();
                        double radius = curve.Circle().Radius();
                        if (u > v) // if arc is reversed
                            std::swap(u, v);

                        double range = v-u;
                        result += radius * range;
                        break;
                    }
                    case GeomAbs_Ellipse:
                    case GeomAbs_BSplineCurve:
                    case GeomAbs_Hyperbola:
                    case GeomAbs_BezierCurve: {
                        result += GCPnts_AbscissaPoint::Length(curve);
                        break;
                    }
                    default: {
                        Base::Console().Error("Measurement::length - curve type: %d not implemented\n");
//                    throw Base::ValueError("Measurement - length - Curve type not currently handled");
                    }
                }  //end switch
            }  //end for
        } //end measureType == Edges
    }
    return result;
}

double Measurement::angle(const Base::Vector3d & /*param*/) const
{
    double result = 0;
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        Base::Console().Error("Measurement::angle - No 3D references available\n");
    } else if (measureType == Invalid) {
        Base::Console().Error("Measurement::angle - measureType is Invalid\n");
    } else {
        if(measureType == Edges) {
            // Only case that is supported is edge to edge
            // TODO: handle 3 pt angle
            if(numRefs == 2) {
                const std::vector<App::DocumentObject*> &objects = References3D.getValues();
                const std::vector<std::string> &subElements = References3D.getSubValues();

                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
                BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

                if(curve1.GetType() == GeomAbs_Line &&
                   curve2.GetType() == GeomAbs_Line) {

                    gp_Pnt pnt1 = curve1.Value(curve1.FirstParameter());
                    gp_Pnt pnt2 = curve1.Value(curve1.LastParameter());
                    gp_Dir dir1 = curve1.Line().Direction();
                    gp_Dir dir2 = curve2.Line().Direction();

                    gp_Lin l1 = gp_Lin(pnt1,dir1);
                    gp_Lin l2 = gp_Lin(pnt2,dir2);
                    Standard_Real aRad = l1.Angle(l2);
                    result = aRad * 180  / M_PI;
//                    return aRad * 180  / M_PI;
                } else {
                    Base::Console().Error("Measurement::angle - Need 2 lines to make angle measure\n");
//                    throw Base::ValueError("Objects must both be lines");
                }
            } else {
                Base::Console().Error("Measurement::angle - Can not compute angle. Too many lines referenced\n");
//                throw Base::ValueError("Can not compute angle. Too many References3D");
            }
        } else if (measureType == Points) {
            if(numRefs == 3) {
                Base::Console().Error("Measurement::angle - 3 point angle not implemented yet\n");
            //TODO: 3 point angle
            }
        }
    }
    return result;
//    throw Base::ValueError("References3D are not Edges");
}

double Measurement::radius() const
{
    double result = 0.0;
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        Base::Console().Error("Measurement::radius - No 3D references available\n");
//        throw Base::ValueError("Measurement - radius - No References3D provided");
    } else {
        if(numRefs == 1 || measureType == Edges) {
            const std::vector<App::DocumentObject*> &objects = References3D.getValues();
            const std::vector<std::string> &subElements = References3D.getSubValues();

            TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
            const TopoDS_Edge& edge = TopoDS::Edge(shape);

            BRepAdaptor_Curve curve(edge);
            if(curve.GetType() == GeomAbs_Circle) {
                result = (double) curve.Circle().Radius();
//                return (double) curve.Circle().Radius();
            }
        }
    }
//    throw Base::ValueError("Measurement - radius - Invalid References3D Provided");
    return result;
}

Base::Vector3d Measurement::delta() const
{
    Base::Vector3d result;
    int numRefs =  References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::delta - No 3D references available\n");
    } else if (measureType == Invalid) {
        Base::Console().Error("Measurement::delta - measureType is Invalid\n");
    } else {
        const std::vector<App::DocumentObject*> &objects = References3D.getValues();
        const std::vector<std::string> &subElements = References3D.getSubValues();

        if(measureType == Points) {
            if(numRefs == 2) {
                // Keep separate case for two points to reduce need for complex algorithm
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                const TopoDS_Vertex& vert1 = TopoDS::Vertex(shape1);
                const TopoDS_Vertex& vert2 = TopoDS::Vertex(shape2);

                gp_Pnt P1 = BRep_Tool::Pnt(vert1);
                gp_Pnt P2 = BRep_Tool::Pnt(vert2);
                gp_XYZ diff = P2.XYZ() - P1.XYZ();
                result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
//                return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
            }
        } else if(measureType == PointToEdge ||
                  measureType == PointToSurface) {
            // BrepExtema can calculate minimum distance between any set of topology sets.
            if(numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepExtrema_DistShapeShape extrema(shape1, shape2);

                if(extrema.IsDone()) {
                    // Found the nearest point between point and curve
                    // NOTE we will assume there is only 1 solution (cyclic topology will create multiple solutions.
                    gp_Pnt P1 = extrema.PointOnShape1(1);
                    gp_Pnt P2 = extrema.PointOnShape2(1);
                    gp_XYZ diff = P2.XYZ() - P1.XYZ();
                    result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
//                    return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            }
        } else if(measureType == Edges) {
            // Only case that is supported is straight line edge
            if(numRefs == 1) {
                TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve(edge);

                if(curve.GetType() == GeomAbs_Line) {
                      gp_Pnt P1 = curve.Value(curve.FirstParameter());
                      gp_Pnt P2 = curve.Value(curve.LastParameter());
                      gp_XYZ diff = P2.XYZ() - P1.XYZ();
                      result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
//                      return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            } else if(numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
                BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

                // Only permit line to line distance
                if(curve1.GetType() == GeomAbs_Line &&
                   curve2.GetType() == GeomAbs_Line) {
                    BRepExtrema_DistShapeShape extrema(shape1, shape2);

                    if(extrema.IsDone()) {
                        // Found the nearest point between point and curve
                        // NOTE we will assume there is only 1 solution (cyclic topology will create multiple solutions.
                        gp_Pnt P1 = extrema.PointOnShape1(1);
                        gp_Pnt P2 = extrema.PointOnShape2(1);
                        gp_XYZ diff = P2.XYZ() - P1.XYZ();
                        result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
//                        return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                    }
                }
            }
        } else {
            Base::Console().Error("Measurement::delta - measureType is not recognized\n");
        }
    }
//    throw Base::ValueError("An invalid selection was made");
    return result;
}


Base::Vector3d Measurement::massCenter() const
{
    Base::Vector3d result;
    int numRefs =  References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::massCenter - No 3D references available\n");
    } else if (measureType == Invalid) {
        Base::Console().Error("Measurement::massCenter - measureType is Invalid\n");
    } else {
        const std::vector<App::DocumentObject*> &objects = References3D.getValues();
        const std::vector<std::string> &subElements = References3D.getSubValues();
        GProp_GProps gprops = GProp_GProps();

        if(measureType == Volumes) {
            // Iterate through edges and calculate each length
            std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
            std::vector<std::string>::const_iterator subEl = subElements.begin();

            for (;obj != objects.end(); ++obj, ++subEl) {
                //const Part::Feature *refObj = static_cast<const Part::Feature*>((*obj));
                //const Part::TopoShape& refShape = refObj->Shape.getShape();

                // Compute inertia properties

                GProp_GProps props = GProp_GProps();
                BRepGProp::VolumeProperties(getShape((*obj), ""), props);
                gprops.Add(props);
                // Get inertia properties
            }

            //double mass = gprops.Mass();
            gp_Pnt cog = gprops.CentreOfMass();

            return Base::Vector3d(cog.X(), cog.Y(), cog.Z());
        } else {
            Base::Console().Error("Measurement::massCenter - measureType is not recognized\n");
//          throw Base::ValueError("Measurement - massCenter - Invalid References3D Provided");
        }
    }
    return result;
}

unsigned int Measurement::getMemSize(void) const
{
    return 0;
}

PyObject *Measurement::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeasurementPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
