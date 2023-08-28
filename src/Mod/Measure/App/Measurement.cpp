/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <gp_Circ.hxx>
# include <gp_Lin.hxx>
# include <GProp_GProps.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Measurement.h"
#include "MeasurementPy.h"


#ifndef M_PI
# define M_PI 3.14159265358979323846
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

Measurement::~Measurement() = default;

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
  subElements.emplace_back(subName);

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

    MeasureType mode;

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
            mode = Edges;
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
//    Base::Console().Message("Meas::getShape(%s, %s)\n", obj->getNameInDocument(), subName);
    //temporary fix to get "Vertex7" from "Body003.Pocket020.Vertex7"
    //when selected, Body features are provided as featureName and subNameAndIndex
    //other sources provide the full extended name with index
    std::string workingSubName(subName);
    size_t lastDot = workingSubName.rfind('.');
    if (lastDot != std::string::npos) {
        workingSubName = workingSubName.substr(lastDot + 1);
    }

    try {
        Part::TopoShape partShape = Part::Feature::getTopoShape(obj);
        App::GeoFeature* geoFeat = dynamic_cast<App::GeoFeature*>(obj);
        if (geoFeat) {
            partShape.setPlacement(geoFeat->globalPlacement());
        }
        TopoDS_Shape shape = partShape.getSubShape(workingSubName.c_str());
        if(shape.IsNull()) {
            throw Part::NullShapeException("null shape in measurement");
        }
        return shape;
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
    catch (...) {
        throw Base::RuntimeError("Measurement: Unknown error retrieving shape");
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
                    throw Base::RuntimeError("Measurement - length - Curve type not currently handled");
                    }
                }  //end switch
            }  //end for
        } //end measureType == Edges
    }
    return result;
}

double Measurement::angle(const Base::Vector3d & /*param*/) const
{
    //TODO: do these references arrive as obj+sub pairs or as a struct of obj + [subs]?
    const std::vector<App::DocumentObject*> &objects = References3D.getValues();
    const std::vector<std::string> &subElements = References3D.getSubValues();
    int numRefs = objects.size();
    if(numRefs == 0) {
        throw Base::RuntimeError("No references available for angle measurement");
    } else if (measureType == Invalid) {
        throw Base::RuntimeError("MeasureType is Invalid for angle measurement");
    } else {
        if(measureType == Edges) {
            //Only case that is supported is edge to edge
            //The angle between two skew lines is measured by the angle between one line (A)
            //and a line (B) with the direction of the second through a point on the first line.
            //Since we don't know if the directions of the lines point in the same general direction
            //we could get the angle we want or the supplementary angle.
            if(numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
                BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

                if(curve1.GetType() == GeomAbs_Line &&
                   curve2.GetType() == GeomAbs_Line) {

                    gp_Pnt pnt1First = curve1.Value(curve1.FirstParameter());
                    gp_Dir dir1 = curve1.Line().Direction();
                    gp_Dir dir2 = curve2.Line().Direction();
                    gp_Dir dir2r = curve2.Line().Direction().Reversed();

                    gp_Lin l1 = gp_Lin(pnt1First, dir1); // (A)
                    gp_Lin l2 = gp_Lin(pnt1First, dir2); // (B)
                    gp_Lin l2r = gp_Lin(pnt1First, dir2r);  // (B')
                    Standard_Real aRad = l1.Angle(l2);
                    double aRadr = l1.Angle(l2r);
                    return std::min(aRad, aRadr) * 180  / M_PI;
                } else {
                    throw Base::RuntimeError("Measurement references must both be lines");
                }
            } else {
                throw Base::RuntimeError("Can not compute angle measurement - too many references");
            }
        } else if (measureType == Points) {
            //NOTE: we are calculating the 3d angle here, not the projected angle
            //ASSUMPTION: the references are in end-apex-end order
            if(numRefs == 3) {
                TopoDS_Shape shape0 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape1 = getShape(objects.at(1), subElements.at(1).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(2).c_str());
                if (shape0.ShapeType() != TopAbs_VERTEX ||
                    shape1.ShapeType() != TopAbs_VERTEX ||
                    shape2.ShapeType() != TopAbs_VERTEX) {
                    throw Base::RuntimeError("Measurement references for 3 point angle are not Vertex");
                }
                gp_Pnt gEnd0 = BRep_Tool::Pnt(TopoDS::Vertex(shape0));
                gp_Pnt gApex = BRep_Tool::Pnt(TopoDS::Vertex(shape1));
                gp_Pnt gEnd1 = BRep_Tool::Pnt(TopoDS::Vertex(shape2));
                gp_Dir gDir0 = gp_Dir(gEnd0.XYZ() - gApex.XYZ());
                gp_Dir gDir1 = gp_Dir(gEnd1.XYZ() - gApex.XYZ());
                gp_Lin line0 = gp_Lin(gEnd0, gDir0);
                gp_Lin line1 = gp_Lin(gEnd1, gDir1);
                double radians = line0.Angle(line1);
                return radians * 180  / M_PI;
            }
        }
    }
    throw Base::RuntimeError("Unexpected error for angle measurement");
}

double Measurement::radius() const
{
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        throw Base::RuntimeError("Measurement - radius - No References3D provided");
    } else {
        if(numRefs == 1 || measureType == Edges) {
            const std::vector<App::DocumentObject*> &objects = References3D.getValues();
            const std::vector<std::string> &subElements = References3D.getSubValues();

            TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
            const TopoDS_Edge& edge = TopoDS::Edge(shape);

            BRepAdaptor_Curve curve(edge);
            if(curve.GetType() == GeomAbs_Circle) {
                return (double) curve.Circle().Radius();
            }
        }
    }
    throw Base::RuntimeError("Measurement - radius - Invalid References3D Provided");
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
                return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
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

unsigned int Measurement::getMemSize() const
{
    return 0;
}

PyObject *Measurement::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeasurementPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
