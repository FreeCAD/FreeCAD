/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepFeat_SplitShape.hxx>
# include <BRepOffsetAPI_Sewing.hxx>
# include <BRepPrim_Wedge.hxx>
# include <BRepPrimAPI_MakeBox.hxx>
# include <BRepPrimAPI_MakeCone.hxx>
# include <BRepPrimAPI_MakeCylinder.hxx>
# include <BRepPrimAPI_MakeRevolution.hxx>
# include <BRepPrimAPI_MakeSphere.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepFill.hxx>
# include <BRepFill_Filling.hxx>
# include <BRepLib.hxx>
# include <BSplCLib.hxx>
# include <gp_Ax3.hxx>
# include <gp_Circ.hxx>
# include <gp_Pnt.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Plane.hxx>
# include <GeomFill_AppSurf.hxx>
# include <GeomFill_Generator.hxx>
# include <GeomFill_Line.hxx>
# include <GeomFill_SectionGenerator.hxx>
# include <Interface_Static.hxx>
# include <NCollection_List.hxx>
# include <Precision.hxx>
# include <ShapeFix.hxx>
# include <ShapeBuild_ReShape.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <Standard_DomainError.hxx>
# include <Standard_Version.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shell.hxx>
# include <TopoDS_Solid.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif
# include <BRepFill_Generator.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/ElementNamingUtils.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Interpreter.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "BSplineSurfacePy.h"
#include "edgecluster.h"
#include "FaceMaker.h"
#include "GeometryCurvePy.h"
#include "GeometryPy.h"
#include "ImportIges.h"
#include "ImportStep.h"
#include "Interface.h"
#include "modelRefine.h"
#include "OCCError.h"
#include "PartFeature.h"
#include "PartPyCXX.h"
#include "Tools.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeWirePy.h"

#ifdef FCUseFreeType
#  include "FT2FC.h"
#endif

extern const char* BRepBuilderAPI_FaceErrorText(BRepBuilderAPI_FaceError fe);

#ifndef M_PI
#define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

namespace Part {

PartExport void getPyShapes(PyObject *obj, std::vector<TopoShape> &shapes) {
    if(!obj)
        return;
    if(PyObject_TypeCheck(obj,&Part::TopoShapePy::Type))
        shapes.push_back(*static_cast<TopoShapePy*>(obj)->getTopoShapePtr());
    else if (PyObject_TypeCheck(obj, &GeometryPy::Type))
        shapes.emplace_back(static_cast<GeometryPy*>(obj)->getGeometryPtr()->toShape());
    else if(PySequence_Check(obj)) {
        Py::Sequence list(obj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type)))
                shapes.push_back(*static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr());
            else if (PyObject_TypeCheck((*it).ptr(), &GeometryPy::Type))
                shapes.emplace_back(static_cast<GeometryPy*>(
                                (*it).ptr())->getGeometryPtr()->toShape());
            else
                throw Py::TypeError("expect shape in sequence");
        }
    }else
        throw Py::TypeError("expect shape or sequence of shapes");
}

PartExport std::vector<TopoShape> getPyShapes(PyObject *obj) {
    std::vector<TopoShape> ret;
    getPyShapes(obj,ret);
    return ret;
}

struct EdgePoints {
    gp_Pnt v1, v2;
    std::list<TopoDS_Edge>::iterator it;
    TopoDS_Edge edge;
};

PartExport std::list<TopoDS_Edge> sort_Edges(double tol3d, std::list<TopoDS_Edge>& edges)
{
    tol3d = tol3d * tol3d;
    std::list<EdgePoints>  edge_points;
    TopExp_Explorer xp;
    for (std::list<TopoDS_Edge>::iterator it = edges.begin(); it != edges.end(); ++it) {
        EdgePoints ep;
        xp.Init(*it,TopAbs_VERTEX);
        ep.v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        ep.v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        ep.it = it;
        ep.edge = *it;
        edge_points.push_back(ep);
    }

    if (edge_points.empty())
        return {};

    std::list<TopoDS_Edge> sorted;
    gp_Pnt first, last;
    first = edge_points.front().v1;
    last  = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edges.erase(edge_points.front().it);
    edge_points.erase(edge_points.begin());

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edge_points.begin(); pEI != edge_points.end(); ++pEI) {
            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(pEI->edge);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v2.SquareDistance(first) <= tol3d) {
                first = pEI->v1;
                sorted.push_front(pEI->edge);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                Standard_Real first, last;
                const Handle(Geom_Curve) & curve = BRep_Tool::Curve(pEI->edge, first, last);
                first = curve->ReversedParameter(first);
                last = curve->ReversedParameter(last);
                TopoDS_Edge edgeReversed = BRepBuilderAPI_MakeEdge(curve->Reversed(), last, first);
                sorted.push_back(edgeReversed);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            else if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                Standard_Real first, last;
                const Handle(Geom_Curve) & curve = BRep_Tool::Curve(pEI->edge, first, last);
                first = curve->ReversedParameter(first);
                last = curve->ReversedParameter(last);
                TopoDS_Edge edgeReversed = BRepBuilderAPI_MakeEdge(curve->Reversed(), last, first);
                sorted.push_front(edgeReversed);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
        }

        if ((pEI == edge_points.end()) || (last.SquareDistance(first) <= tol3d)) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }

    return sorted;
}
}

namespace Part {
class BRepFeatModule : public Py::ExtensionModule<BRepFeatModule>
{
public:
    BRepFeatModule() : Py::ExtensionModule<BRepFeatModule>("BRepFeat")
    {
        initialize("This is a module working with the BRepFeat package."); // register with Python
    }
};

class BRepOffsetAPIModule : public Py::ExtensionModule<BRepOffsetAPIModule>
{
public:
    BRepOffsetAPIModule() : Py::ExtensionModule<BRepOffsetAPIModule>("BRepOffsetAPI")
    {
        initialize("This is a module working with the BRepOffsetAPI package."); // register with Python
    }
};

class Geom2dModule : public Py::ExtensionModule<Geom2dModule>
{
public:
    Geom2dModule() : Py::ExtensionModule<Geom2dModule>("Geom2d")
    {
        initialize("This is a module working with 2d geometries."); // register with Python
    }
};

class GeomPlateModule : public Py::ExtensionModule<GeomPlateModule>
{
public:
    GeomPlateModule() : Py::ExtensionModule<GeomPlateModule>("GeomPlate")
    {
        initialize("This is a module working with the GeomPlate framework."); // register with Python
    }
};

class HLRBRepModule : public Py::ExtensionModule<HLRBRepModule>
{
public:
    HLRBRepModule() : Py::ExtensionModule<HLRBRepModule>("HLRBRep")
    {
        initialize("This is a module working with the HLRBRep framework."); // register with Python
    }
};

class ShapeFixModule : public Py::ExtensionModule<ShapeFixModule>
{
public:
    ShapeFixModule() : Py::ExtensionModule<ShapeFixModule>("ShapeFix")
    {
        add_varargs_method("sameParameter",&ShapeFixModule::sameParameter,
            "sameParameter(shape, enforce, prec=0.0)"
        );
        add_varargs_method("encodeRegularity",&ShapeFixModule::encodeRegularity,
            "encodeRegularity(shape, tolerance = 1e-10)\n"
        );
        add_varargs_method("removeSmallEdges",&ShapeFixModule::removeSmallEdges,
            "removeSmallEdges(shape, tolerance, ReShapeContext)\n"
            "Removes edges which are less than given tolerance from shape"
        );
        add_varargs_method("fixVertexPosition",&ShapeFixModule::fixVertexPosition,
            "fixVertexPosition(shape, tolerance, ReShapeContext)\n"
            "Fix position of the vertices having tolerance more tnan specified one"
        );
        add_varargs_method("leastEdgeSize",&ShapeFixModule::leastEdgeSize,
            "leastEdgeSize(shape)\n"
            "Calculate size of least edge"
        );
        initialize("This is a module working with the ShapeFix framework."); // register with Python
    }

private:
    Py::Object sameParameter(const Py::Tuple& args)
    {
        PyObject* shape;
        PyObject* enforce;
        double prec = 0.0;
        if (!PyArg_ParseTuple(args.ptr(), "O!O!|d", &TopoShapePy::Type, &shape, &PyBool_Type, &enforce, &prec))
            throw Py::Exception();

        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        bool ok = ShapeFix::SameParameter(sh, Base::asBoolean(enforce), prec);
        return Py::Boolean(ok);
    }
    Py::Object encodeRegularity(const Py::Tuple& args)
    {
        PyObject* shape;
        double tolang = 1.0e-10;
        if (!PyArg_ParseTuple(args.ptr(), "O!|d", &TopoShapePy::Type, &shape, &tolang))
            throw Py::Exception();

        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        ShapeFix::EncodeRegularity(sh, tolang);
        return Py::None();
    }
    Py::Object removeSmallEdges(const Py::Tuple& args)
    {
        PyObject* shape;
        double tol;
        if (!PyArg_ParseTuple(args.ptr(), "O!d", &TopoShapePy::Type, &shape, &tol))
            throw Py::Exception();

        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        Handle(ShapeBuild_ReShape) reshape = new ShapeBuild_ReShape();
        TopoShape res = ShapeFix::RemoveSmallEdges(sh, tol, reshape);
        return Py::asObject(res.getPyObject());
    }
    Py::Object fixVertexPosition(const Py::Tuple& args)
    {
        PyObject* shape;
        double tol;
        if (!PyArg_ParseTuple(args.ptr(), "O!d", &TopoShapePy::Type, &shape, &tol))
            throw Py::Exception();

        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        Handle(ShapeBuild_ReShape) reshape = new ShapeBuild_ReShape();
        bool ok = ShapeFix::FixVertexPosition(sh, tol, reshape);
        return Py::Boolean(ok);
    }
    Py::Object leastEdgeSize(const Py::Tuple& args)
    {
        PyObject* shape;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &TopoShapePy::Type, &shape))
            throw Py::Exception();

        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        double len = ShapeFix::LeastEdgeSize(sh);
        return Py::Float(len);
    }
};

class ShapeUpgradeModule : public Py::ExtensionModule<ShapeUpgradeModule>
{
public:
    ShapeUpgradeModule() : Py::ExtensionModule<ShapeUpgradeModule>("ShapeUpgrade")
    {
        initialize("This is a module working with the ShapeUpgrade framework."); // register with Python
    }
};

class ChFi2dModule : public Py::ExtensionModule<ChFi2dModule>
{
public:
    ChFi2dModule() : Py::ExtensionModule<ChFi2dModule>("ChFi2d")
    {
        initialize("This is a module working with the ChFi2d framework."); // register with Python
    }
};

class Module : public Py::ExtensionModule<Module>
{
    BRepFeatModule brepFeat;
    BRepOffsetAPIModule brepOffsetApi;
    Geom2dModule geom2d;
    GeomPlateModule geomPlate;
    HLRBRepModule HLRBRep;
    ShapeFixModule shapeFix;
    ShapeUpgradeModule shapeUpgrade;
    ChFi2dModule chFi2d;
public:
    Module() : Py::ExtensionModule<Module>("Part")
    {
        add_varargs_method("open",&Module::open,
            "open(string) -- Create a new document and load the file into the document."
        );
        add_varargs_method("insert",&Module::insert,
            "insert(string,string) -- Insert the file into the given document."
        );
        add_varargs_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        add_varargs_method("read",&Module::read,
            "read(string) -- Load the file and return the shape."
        );
        add_varargs_method("show",&Module::show,
            "show(shape,[string]) -- Add the shape to the active document or create one if no document exists."
        );
        add_varargs_method("getFacets",&Module::getFacets,
            "getFacets(shape): simplified mesh generation"
        );
        add_varargs_method("makeCompound",&Module::makeCompound,
            "makeCompound(list) -- Create a compound out of a list of shapes."
        );
        add_varargs_method("makeShell",&Module::makeShell,
            "makeShell(list) -- Create a shell out of a list of faces."
        );
        add_varargs_method("makeFace",&Module::makeFace,
            "makeFace(list_of_shapes_or_compound, maker_class_name) -- Create a face (faces) using facemaker class.\n"
            "maker_class_name is a string like 'Part::FaceMakerSimple'."
        );
        add_varargs_method("makeFilledSurface",&Module::makeFilledSurface,
            "makeFilledSurface(list of curves, tolerance) -- Create a surface out of a list of curves."
        );
        add_varargs_method("makeFilledFace",&Module::makeFilledFace,
            "makeFilledFace(list) -- Create a face out of a list of edges."
        );
        add_varargs_method("makeSolid",&Module::makeSolid,
            "makeSolid(shape): Create a solid out of shells of shape. If shape is a compsolid, the overall volume solid is created."
        );
        add_varargs_method("makePlane",&Module::makePlane,
            "makePlane(length,width,[pnt,dirZ,dirX]) -- Make a plane\n"
            "By default pnt=Vector(0,0,0) and dirZ=Vector(0,0,1), dirX is ignored in this case"
        );
        add_varargs_method("makeBox",&Module::makeBox,
            "makeBox(length,width,height,[pnt,dir]) -- Make a box located\n"
            "in pnt with the dimensions (length,width,height)\n"
            "By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)"
        );
        add_varargs_method("makeWedge",&Module::makeWedge,
            "makeWedge(xmin, ymin, zmin, z2min, x2min,\n"
            "xmax, ymax, zmax, z2max, x2max,[pnt,dir])\n"
            " -- Make a wedge located in pnt\n"
            "By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)"
        );
        add_varargs_method("makeLine",&Module::makeLine,
            "makeLine(startpnt,endpnt) -- Make a line between two points\n"
            "\n"
            "Args:\n"
            "    startpnt (Vector or tuple): Vector or 3 element tuple \n"
            "        containing the x,y and z coordinates of the start point,\n"
            "        i.e. (x1,y1,z1).\n"
            "    endpnt (Vector or tuple): Vector or 3 element tuple \n"
            "        containing the x,y and z coordinates of the start point,\n"
            "        i.e. (x1,y1,z1).\n"
            "\n"
            "Returns:\n"
            "    Edge: Part.Edge object\n"
        );
        add_varargs_method("makePolygon",&Module::makePolygon,
            "makePolygon(pntslist) -- Make a polygon from a list of points\n"
            "\n"
            "Args:\n"
            "    pntslist (list(Vector)): list of Vectors representing the \n"
            "        points of the polygon.\n"
            "\n"
            "Returns:\n"
            "    Wire: Part.Wire object. If the last point in the list is \n"
            "        not the same as the first point, the Wire will not be \n"
            "        closed and cannot be used to create a face.\n"
        );
        add_varargs_method("makeCircle",&Module::makeCircle,
            "makeCircle(radius,[pnt,dir,angle1,angle2]) -- Make a circle with a given radius\n"
            "By default pnt=Vector(0,0,0), dir=Vector(0,0,1), angle1=0 and angle2=360"
        );
        add_varargs_method("makeSphere",&Module::makeSphere,
            "makeSphere(radius,[pnt, dir, angle1,angle2,angle3]) -- Make a sphere with a given radius\n"
            "By default pnt=Vector(0,0,0), dir=Vector(0,0,1), angle1=0, angle2=90 and angle3=360"
        );
        add_varargs_method("makeCylinder",&Module::makeCylinder,
            "makeCylinder(radius,height,[pnt,dir,angle]) -- Make a cylinder with a given radius and height\n"
            "By default pnt=Vector(0,0,0),dir=Vector(0,0,1) and angle=360"
        );
        add_varargs_method("makeCone",&Module::makeCone,
            "makeCone(radius1,radius2,height,[pnt,dir,angle]) -- Make a cone with given radii and height\n"
            "By default pnt=Vector(0,0,0), dir=Vector(0,0,1) and angle=360"
        );
        add_varargs_method("makeTorus",&Module::makeTorus,
            "makeTorus(radius1,radius2,[pnt,dir,angle1,angle2,angle]) -- Make a torus with a given radii and angles\n"
            "By default pnt=Vector(0,0,0),dir=Vector(0,0,1),angle1=0,angle1=360 and angle=360"
        );
        add_varargs_method("makeHelix",&Module::makeHelix,
            "makeHelix(pitch,height,radius,[angle]) -- Make a helix with a given pitch, height and radius\n"
            "By default a cylindrical surface is used to create the helix. If the fourth parameter is set\n"
            "(the apex given in degree) a conical surface is used instead"
        );
        add_varargs_method("makeLongHelix",&Module::makeLongHelix,
            "makeLongHelix(pitch,height,radius,[angle],[hand]) -- Make a (multi-edge) helix with a given pitch, height and radius\n"
            "By default a cylindrical surface is used to create the helix. If the fourth parameter is set\n"
            "(the apex given in degree) a conical surface is used instead."
        );
        add_varargs_method("makeThread",&Module::makeThread,
            "makeThread(pitch,depth,height,radius) -- Make a thread with a given pitch, depth, height and radius"
        );
        add_varargs_method("makeRevolution",&Module::makeRevolution,
            "makeRevolution(Curve or Edge,[vmin,vmax,angle,pnt,dir,shapetype]) -- Make a revolved shape\n"
            "by rotating the curve or a portion of it around an axis given by (pnt,dir).\n"
            "By default vmin/vmax=bounds of the curve, angle=360, pnt=Vector(0,0,0),\n"
            "dir=Vector(0,0,1) and shapetype=Part.Solid"
        );
        add_varargs_method("makeRuledSurface",&Module::makeRuledSurface,
            "makeRuledSurface(Edge|Wire,Edge|Wire) -- Make a ruled surface\n"
            "Create a ruled surface out of two edges or wires. If wires are used then"
            "these must have the same number of edges."
        );
        add_varargs_method("makeShellFromWires",&Module::makeShellFromWires,
            "makeShellFromWires(Wires) -- Make a shell from wires.\n"
            "The wires must have the same number of edges."
        );
        add_varargs_method("makeTube",&Module::makeTube,
            "makeTube(edge,radius,[continuity,max degree,max segments]) -- Create a tube.\n"
            "continuity is a string which must be 'C0','C1','C2','C3','CN','G1' or 'G1',"
        );
        add_varargs_method("makeSweepSurface",&Module::makeSweepSurface,
            "makeSweepSurface(edge(path),edge(profile),[float]) -- Create a profile along a path."
        );
        add_varargs_method("makeLoft",&Module::makeLoft,
            "makeLoft(list of wires,[solid=False,ruled=False,closed=False,maxDegree=5]) -- Create a loft shape."
        );
        add_varargs_method("makeWireString",&Module::makeWireString,
            "makeWireString(string,fontdir,fontfile,height,[track]) -- Make list of wires in the form of a string's characters."
        );
        add_varargs_method("makeSplitShape",&Module::makeSplitShape,
            "makeSplitShape(shape, list of shape pairs,[check Interior=True]) -> two lists of shapes.\n"
            "The following shape pairs are supported:\n"
            "* Wire, Face\n"
            "* Edge, Face\n"
            "* Compound, Face\n"
            "* Edge, Edge\n"
            "* The face must be part of the specified shape and the edge, wire or compound must\n"
            "lie on the face.\n"
            "Output:\n"
            "The first list contains the faces that are the left of the projected wires.\n"
            "The second list contains the left part on the shape.\n\n"
            "Example:\n"
            "face = ...\n"
            "edges = ...\n"
            "split = [(edges[0],face),(edges[1],face)]\n"
            "r = Part.makeSplitShape(face, split)\n"
            "Part.show(r[0][0])\n"
            "Part.show(r[1][0])\n"
        );
        add_varargs_method("exportUnits",&Module::exportUnits,
            "exportUnits([string=MM|M|INCH|FT|MI|KM|MIL|UM|CM|UIN]) -- Set units for exporting STEP/IGES files and returns the units."
        );
        add_varargs_method("setStaticValue",&Module::setStaticValue,
            "setStaticValue(string,string|int|float) -- Set a name to a value The value can be a string, int or float."
        );
        add_varargs_method("cast_to_shape",&Module::cast_to_shape,
            "cast_to_shape(shape) -- Cast to the actual shape type"
        );
        add_varargs_method("getSortedClusters",&Module::getSortedClusters,
            "getSortedClusters(list of edges) -- Helper method to sort and cluster a variety of edges"
        );
        add_varargs_method("__sortEdges__",&Module::sortEdges,
            "__sortEdges__(list of edges) -- list of edges\n"
            "Helper method to sort an unsorted list of edges so that afterwards\n"
            "the start and end vertex of two consecutive edges are geometrically coincident.\n"
            "It returns a single list of edges and the algorithm stops after the first set of\n"
            "connected edges which means that the output list can be smaller than the input list.\n"
            "The sorted list can be used to create a Wire."
        );
        add_varargs_method("sortEdges",&Module::sortEdges2,
            "sortEdges(list of edges, [tol3d]) -- list of lists of edges\n"
            "It does basically the same as __sortEdges__ but sorts all input edges and thus returns\n"
            "a list of lists of edges\n"
            "optional 3D tolerance defaults to Precision::Confusion"
        );
        add_varargs_method("__toPythonOCC__",&Module::toPythonOCC,
            "__toPythonOCC__(shape) -- Helper method to convert an internal shape to pythonocc shape"
        );
        add_varargs_method("__fromPythonOCC__",&Module::fromPythonOCC,
            "__fromPythonOCC__(occ) -- Helper method to convert a pythonocc shape to an internal shape"
        );
        add_varargs_method("clearShapeCache",&Module::clearShapeCache,
            "clearShapeCache() -- Clears internal shape cache"
        );
        add_keyword_method("getShape",&Module::getShape,
            "getShape(obj,subname=None,mat=None,needSubElement=False,transform=True,retType=0):\n"
            "Obtain the TopoShape of a given object with SubName reference\n\n"
            "* obj: the input object\n"
            "* subname: dot separated sub-object reference\n"
            "* mat: the current transformation matrix\n"
            "* needSubElement: if False, ignore the sub-element (e.g. Face1, Edge1) reference in 'subname'\n"
            "* transform: if False, then skip obj's transformation. Use this if mat already include obj's\n"
            "             transformation matrix\n"
            "* retType: 0: return TopoShape,\n"
            "           1: return (shape,subObj,mat), where subObj is the object referenced in 'subname',\n"
            "              and 'mat' is the accumulated transformation matrix of that sub-object.\n"
            "           2: same as 1, but make sure 'subObj' is resolved if it is a link.\n"
            "* refine: refine the returned shape"
        );
        add_varargs_method("splitSubname",&Module::splitSubname,
            "splitSubname(subname) -> list(sub,mapped,subElement)\n"
            "Split the given subname into a list\n\n"
            "sub: subname without any sub-element reference\n"
            "mapped: mapped element name, or '' if none\n"
            "subElement: old style element name, or '' if none"
        );
        add_varargs_method("joinSubname",&Module::joinSubname,
            "joinSubname(sub,mapped,subElement) -> subname\n"
        );
        initialize("This is a module working with shapes."); // register with Python

        PyModule_AddObject(m_module, "BRepFeat", brepFeat.module().ptr());
        PyModule_AddObject(m_module, "BRepOffsetAPI", brepOffsetApi.module().ptr());
        PyModule_AddObject(m_module, "Geom2d", geom2d.module().ptr());
        PyModule_AddObject(m_module, "GeomPlate", geomPlate.module().ptr());
        PyModule_AddObject(m_module, "HLRBRep", HLRBRep.module().ptr());
        PyModule_AddObject(m_module, "ShapeFix", shapeFix.module().ptr());
        PyModule_AddObject(m_module, "ShapeUpgrade", shapeUpgrade.module().ptr());
        PyModule_AddObject(m_module, "ChFi2d", chFi2d.module().ptr());
    }

private:
    Py::Object invoke_method_keyword( void *method_def,
            const Py::Tuple &args, const Py::Dict &keywords ) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_keyword(method_def, args, keywords);
        }
        catch (const Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            Base::Console().Error("%s\n", str.c_str());
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            Base::Console().Error("%s\n", str.c_str());
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        //Base::Console().Log("Open in Part with %s",Name);
        Base::FileInfo file(EncodedName.c_str());

        // extract ending
        if (file.extension().empty())
            throw Py::RuntimeError("No file extension");

        if (file.hasExtension({"stp", "step"})) {
            // create new document and add Import feature
            App::Document *pcDoc = App::GetApplication().newDocument();
            ImportStepParts(pcDoc,EncodedName.c_str());

            pcDoc->recompute();
        }
        else if (file.hasExtension({"igs", "iges"})) {
            App::Document *pcDoc = App::GetApplication().newDocument();
            ImportIgesParts(pcDoc,EncodedName.c_str());
            pcDoc->recompute();
        }
        else {
            TopoShape shape;
            shape.read(EncodedName.c_str());

            // create new document set loaded shape
            App::Document *pcDoc = App::GetApplication().newDocument(file.fileNamePure().c_str());
            Part::Feature *object = static_cast<Part::Feature *>(pcDoc->addObject
                ("Part::Feature",file.fileNamePure().c_str()));
            object->Shape.setValue(shape);
            pcDoc->recompute();
        }

        return Py::None();
    }
    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "ets","utf-8",&Name,&DocName))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(EncodedName.c_str());

        // extract ending
        if (file.extension().empty())
            throw Py::RuntimeError("No file extension");

        App::Document *pcDoc = App::GetApplication().getDocument(DocName);
        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        if (file.hasExtension({"stp", "step"})) {
            ImportStepParts(pcDoc,EncodedName.c_str());

            pcDoc->recompute();
        }
        else if (file.hasExtension({"igs", "iges"})) {
            ImportIgesParts(pcDoc,EncodedName.c_str());
            pcDoc->recompute();
        }
        else {
            TopoShape shape;
            shape.read(EncodedName.c_str());

            Part::Feature *object = static_cast<Part::Feature *>(pcDoc->addObject
                ("Part::Feature",file.fileNamePure().c_str()));
            object->Shape.setValue(shape);
            pcDoc->recompute();
        }

        return Py::None();
    }
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet",&object,"utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);

        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                    Part::Feature* part = static_cast<Part::Feature*>(obj);
                    const TopoDS_Shape& shape = part->Shape.getValue();
                    if (!shape.IsNull())
                        builder.Add(comp, shape);
                }
                else {
                    Base::Console().Message("'%s' is not a shape, export will be ignored.\n", obj->Label.getValue());
                }
            }
        }

        TopoShape shape(comp);
        shape.write(EncodedName.c_str());

        return Py::None();
    }
    Py::Object read(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        TopoShape* shape = new TopoShape();
        shape->read(EncodedName.c_str());
        return Py::asObject(new TopoShapePy(shape));
    }
    Py::Object show(const Py::Tuple& args)
    {
        PyObject *pcObj = nullptr;
        char *name = "Shape";
        if (!PyArg_ParseTuple(args.ptr(), "O!|s", &(TopoShapePy::Type), &pcObj, &name))
            throw Py::Exception();

        App::Document *pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObj);
        Part::Feature *pcFeature = static_cast<Part::Feature*>(pcDoc->addObject("Part::Feature", name));
        // copy the data
        pcFeature->Shape.setValue(pShape->getTopoShapePtr()->getShape());
        pcDoc->recompute();
        return Py::asObject(pcFeature->getPyObject());
    }
    Py::Object getFacets(const Py::Tuple& args)
    {
        PyObject *shape;
        Py::List list;
        if (!PyArg_ParseTuple(args.ptr(), "O", &shape))
            throw Py::Exception();
        auto theShape = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        for (TopExp_Explorer ex(theShape, TopAbs_FACE); ex.More(); ex.Next()) {
            TopoDS_Face currentFace = TopoDS::Face(ex.Current());

            std::vector<gp_Pnt> points;
            std::vector<Poly_Triangle> facets;
            if (Tools::getTriangulation(currentFace, points, facets)) {
                for (const auto& it : facets) {
                    Standard_Integer n1,n2,n3;
                    it.Get(n1, n2, n3);

                    gp_Pnt p1 = points[n1];
                    gp_Pnt p2 = points[n2];
                    gp_Pnt p3 = points[n3];

                    // TODO: verify if tolerance should be hard coded
                    if (!p1.IsEqual(p2, 0.01) && !p2.IsEqual(p3, 0.01) && !p3.IsEqual(p1, 0.01)) {
                        PyObject *t1 = PyTuple_Pack(3, PyFloat_FromDouble(p1.X()), PyFloat_FromDouble(p1.Y()), PyFloat_FromDouble(p1.Z()));
                        PyObject *t2 = PyTuple_Pack(3, PyFloat_FromDouble(p2.X()), PyFloat_FromDouble(p2.Y()), PyFloat_FromDouble(p2.Z()));
                        PyObject *t3 = PyTuple_Pack(3, PyFloat_FromDouble(p3.X()), PyFloat_FromDouble(p3.Y()), PyFloat_FromDouble(p3.Z()));
                        list.append(Py::asObject(PyTuple_Pack(3, t1, t2, t3)));
                    }
                }
            }
        }
        return list;
    }
    Py::Object makeCompound(const Py::Tuple& args)
    {
        PyObject *pcObj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &pcObj))
            throw Py::Exception();

        BRep_Builder builder;
        TopoDS_Compound Comp;
        builder.MakeCompound(Comp);

        PY_TRY {
            for(auto &s : getPyShapes(pcObj)) {
                const auto &sh = s.getShape();
                if (!sh.IsNull())
                    builder.Add(Comp, sh);
            }
        } _PY_CATCH_OCC(throw Py::Exception())
        return Py::asObject(new TopoShapeCompoundPy(new TopoShape(Comp)));
    }
    Py::Object makeShell(const Py::Tuple& args)
    {
        PyObject *obj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
            throw Py::Exception();

        BRep_Builder builder;
        TopoDS_Shape shape;
        TopoDS_Shell shell;
        //BRepOffsetAPI_Sewing mkShell;
        builder.MakeShell(shell);

        try {
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeFacePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapeFacePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    if (!sh.IsNull())
                        builder.Add(shell, sh);
                }
            }

            shape = shell;
            BRepCheck_Analyzer check(shell);
            if (!check.IsValid()) {
                ShapeUpgrade_ShellSewing sewShell;
                shape = sewShell.ApplySewing(shell);
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }

        return Py::asObject(new TopoShapeShellPy(new TopoShape(shape)));
    }
    Py::Object makeFace(const Py::Tuple& args)
    {
        try {
            char* className = nullptr;
            PyObject* pcPyShapeOrList = nullptr;
            PyErr_Clear();
            if (PyArg_ParseTuple(args.ptr(), "Os", &pcPyShapeOrList, &className)) {
                std::unique_ptr<FaceMaker> fm = Part::FaceMaker::ConstructFromType(className);

                //dump all supplied shapes to facemaker, no matter what type (let facemaker decide).
                if (PySequence_Check(pcPyShapeOrList)){
                    Py::Sequence list(pcPyShapeOrList);
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        PyObject* item = (*it).ptr();
                        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                            const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                            fm->addShape(sh);
                        } else {
                            throw Py::TypeError("Object is not a shape.");
                        }
                    }
                } else if (PyObject_TypeCheck(pcPyShapeOrList, &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(pcPyShapeOrList)->getTopoShapePtr()->getShape();
                    if (sh.IsNull())
                        throw NullShapeException("Shape is null!");
                    if (sh.ShapeType() == TopAbs_COMPOUND)
                        fm->useCompound(TopoDS::Compound(sh));
                    else
                        fm->addShape(sh);
                } else {
                    throw Py::Exception(PyExc_TypeError, "First argument is neither a shape nor list of shapes.");
                }

                fm->Build();

                TopoShape topo(fm->Shape());
                return Py::asObject(topo.getPyObject());
            }

            throw Py::TypeError(std::string("Argument type signature not recognized. Should be either (list, string), or (shape, string)"));

        } catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        } catch (Base::Exception &e){
            e.setPyException();
            throw Py::Exception();
        }
    }
    Py::Object makeFilledSurface(const Py::Tuple &args)
    {
        PyObject *obj;
        double tolerance;
        if (!PyArg_ParseTuple(args.ptr(), "Od", &obj, &tolerance))
            throw Py::Exception();

        try {
            GeomFill_Generator generator;
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryCurvePy::Type))) {
                    Handle(Geom_Curve) hCurve = Handle(Geom_Curve)::DownCast(static_cast<Part::GeometryCurvePy*>((*it).ptr())->getGeomCurvePtr()->handle());
                    if (!hCurve.IsNull()) {
                        generator.AddCurve(hCurve);
                    }
                }
            }

            generator.Perform(tolerance);
            Handle(Geom_Surface) hSurface = generator.Surface();
            if (!hSurface.IsNull()) {
                return Py::asObject(makeFromSurface(hSurface)->getPyObject());
            }
            else {
                throw Py::Exception(PartExceptionOCCError, "Failed to created surface by filling curves");
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeFilledFace(const Py::Tuple& args)
    {
        // TODO: BRepFeat_SplitShape
        PyObject *obj;
        PyObject *surf=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O|O!", &obj, &TopoShapeFacePy::Type, &surf))
            throw Py::Exception();

        // See also BRepOffsetAPI_MakeFilling
        BRepFill_Filling builder;
        try {
            if (surf) {
                const TopoDS_Shape& face = static_cast<TopoShapeFacePy*>(surf)->
                    getTopoShapePtr()->getShape();
                if (!face.IsNull() && face.ShapeType() == TopAbs_FACE) {
                    builder.LoadInitSurface(TopoDS::Face(face));
                }
            }
            Py::Sequence list(obj);
            int numConstraints = 0;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    if (!sh.IsNull()) {
                        if (sh.ShapeType() == TopAbs_EDGE) {
                            builder.Add(TopoDS::Edge(sh), GeomAbs_C0);
                            numConstraints++;
                        }
                        else if (sh.ShapeType() == TopAbs_FACE) {
                            builder.Add(TopoDS::Face(sh), GeomAbs_C0);
                            numConstraints++;
                        }
                        else if (sh.ShapeType() == TopAbs_VERTEX) {
                            const TopoDS_Vertex& v = TopoDS::Vertex(sh);
                            gp_Pnt pnt = BRep_Tool::Pnt(v);
                            builder.Add(pnt);
                            numConstraints++;
                        }
                    }
                }
            }

            if (numConstraints == 0) {
                throw Py::Exception(PartExceptionOCCError, "Failed to create face with no constraints");
            }

            builder.Build();
            if (builder.IsDone()) {
                return Py::asObject(new TopoShapeFacePy(new TopoShape(builder.Face())));
            }
            else {
                throw Py::Exception(PartExceptionOCCError, "Failed to created face by filling edges");
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeSolid(const Py::Tuple& args)
    {
        PyObject *obj;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(TopoShapePy::Type), &obj))
            throw Py::Exception();

        try {
            const TopoDS_Shape& shape = static_cast<TopoShapePy*>(obj)
                ->getTopoShapePtr()->getShape();
            //first, if we were given a compsolid, try making a solid out of it
            TopExp_Explorer CSExp (shape, TopAbs_COMPSOLID);
            TopoDS_CompSolid compsolid;
            int count=0;
            for (; CSExp.More(); CSExp.Next()) {
                ++count;
                compsolid = TopoDS::CompSolid(CSExp.Current());
                if (count > 1)
                    break;
            }
            if (count == 0) {
                //no compsolids. Get shells...
                BRepBuilderAPI_MakeSolid mkSolid;
                TopExp_Explorer anExp (shape, TopAbs_SHELL);
                count=0;
                for (; anExp.More(); anExp.Next()) {
                    ++count;
                    mkSolid.Add(TopoDS::Shell(anExp.Current()));
                }

                if (count == 0)//no shells?
                    Standard_Failure::Raise("No shells or compsolids found in shape");

                TopoDS_Solid solid = mkSolid.Solid();
                BRepLib::OrientClosedSolid(solid);
                return Py::asObject(new TopoShapeSolidPy(new TopoShape(solid)));
            } else if (count == 1) {
                BRepBuilderAPI_MakeSolid mkSolid(compsolid);
                TopoDS_Solid solid = mkSolid.Solid();
                return Py::asObject(new TopoShapeSolidPy(new TopoShape(solid)));
            } else { // if (count > 1)
                Standard_Failure::Raise("Only one compsolid can be accepted. Provided shape has more than one compsolid.");
                return Py::None(); //prevents compiler warning
            }
        }
        catch (Standard_Failure& err) {
            std::stringstream errmsg;
            errmsg << "Creation of solid failed: " << err.GetMessageString();
            throw Py::Exception(PartExceptionOCCError, errmsg.str().c_str());
        }
    }
    Py::Object makePlane(const Py::Tuple& args)
    {
        double length, width;
        PyObject *pPnt=nullptr, *pDirZ=nullptr, *pDirX=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "dd|O!O!O!", &length, &width,
                                                 &(Base::VectorPy::Type), &pPnt,
                                                 &(Base::VectorPy::Type), &pDirZ,
                                                 &(Base::VectorPy::Type), &pDirX))
            throw Py::Exception();

        if (length < Precision::Confusion()) {
            throw Py::ValueError("length of plane too small");
        }
        if (width < Precision::Confusion()) {
            throw Py::ValueError("width of plane too small");
        }

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDirZ) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDirZ)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            Handle(Geom_Plane) aPlane;
            if (pDirX) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDirX)->value();
                gp_Dir dx;
                dx.SetCoord(vec.x, vec.y, vec.z);
                aPlane = new Geom_Plane(gp_Ax3(p, d, dx));
            }
            else {
                aPlane = new Geom_Plane(p, d);
            }

            BRepBuilderAPI_MakeFace Face(aPlane, 0.0, length, 0.0, width, Precision::Confusion() );
            return Py::asObject(new TopoShapeFacePy(new TopoShape((Face.Face()))));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of plane failed");
        }
        catch (Standard_Failure&) {
            throw Py::Exception(PartExceptionOCCError, "creation of plane failed");
        }
    }
    Py::Object makeBox(const Py::Tuple& args)
    {
        double length, width, height;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "ddd|O!O!",
            &length, &width, &height,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir))
            throw Py::Exception();

        if (length < Precision::Confusion()) {
            throw Py::ValueError("length of box too small");
        }
        if (width < Precision::Confusion()) {
            throw Py::ValueError("width of box too small");
        }
        if (height < Precision::Confusion()) {
            throw Py::ValueError("height of box too small");
        }

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrimAPI_MakeBox mkBox(gp_Ax2(p,d), length, width, height);
            TopoDS_Shape ResultShape = mkBox.Shape();
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(ResultShape)));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of box failed");
        }
    }
    Py::Object makeWedge(const Py::Tuple& args)
    {
        double xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "dddddddddd|O!O!",
            &xmin, &ymin, &zmin, &z2min, &x2min, &xmax, &ymax, &zmax, &z2max, &x2max,
            &(Base::VectorPy::Type), &pPnt, &(Base::VectorPy::Type), &pDir))
            throw Py::Exception();

        double dx = xmax-xmin;
        double dy = ymax-ymin;
        double dz = zmax-zmin;
        double dz2 = z2max-z2min;
        double dx2 = x2max-x2min;
        if (dx < Precision::Confusion()) {
            throw Py::ValueError("delta x of wedge too small");
        }
        if (dy < Precision::Confusion()) {
            throw Py::ValueError("delta y of wedge too small");
        }
        if (dz < Precision::Confusion()) {
            throw Py::ValueError("delta z of wedge too small");
        }
        if (dz2 < 0) {
            throw Py::ValueError("delta z2 of wedge is negative");
        }
        if (dx2 < 0) {
            throw Py::ValueError("delta x2 of wedge is negative");
        }

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrim_Wedge mkWedge(gp_Ax2(p,d), xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max);
            BRepBuilderAPI_MakeSolid mkSolid;
            mkSolid.Add(mkWedge.Shell());
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(mkSolid.Solid())));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of wedge failed");
        }
    }
    Py::Object makeLine(const Py::Tuple& args)
    {
        PyObject *obj1, *obj2;
        if (!PyArg_ParseTuple(args.ptr(), "OO", &obj1, &obj2))
            throw Py::Exception();

        Base::Vector3d pnt1, pnt2;
        if (PyObject_TypeCheck(obj1, &(Base::VectorPy::Type))) {
            pnt1 = static_cast<Base::VectorPy*>(obj1)->value();
        }
        else if (PyObject_TypeCheck(obj1, &PyTuple_Type)) {
            pnt1 = Base::getVectorFromTuple<double>(obj1);
        }
        else {
            throw Py::TypeError("first argument must either be vector or tuple");
        }
        if (PyObject_TypeCheck(obj2, &(Base::VectorPy::Type))) {
            pnt2 = static_cast<Base::VectorPy*>(obj2)->value();
        }
        else if (PyObject_TypeCheck(obj2, &PyTuple_Type)) {
            pnt2 = Base::getVectorFromTuple<double>(obj2);
        }
        else {
            throw Py::TypeError("second argument must either be vector or tuple");
        }

        // Create directly the underlying line geometry
        BRepBuilderAPI_MakeEdge makeEdge(gp_Pnt(pnt1.x, pnt1.y, pnt1.z),
                                         gp_Pnt(pnt2.x, pnt2.y, pnt2.z));

        const char *error=nullptr;
        switch (makeEdge.Error())
        {
        case BRepBuilderAPI_EdgeDone:
            break; // ok
        case BRepBuilderAPI_PointProjectionFailed:
            error = "Point projection failed";
            break;
        case BRepBuilderAPI_ParameterOutOfRange:
            error = "Parameter out of range";
            break;
        case BRepBuilderAPI_DifferentPointsOnClosedCurve:
            error = "Different points on closed curve";
            break;
        case BRepBuilderAPI_PointWithInfiniteParameter:
            error = "Point with infinite parameter";
            break;
        case BRepBuilderAPI_DifferentsPointAndParameter:
            error = "Different point and parameter";
            break;
        case BRepBuilderAPI_LineThroughIdenticPoints:
            error = "Line through identical points";
            break;
        }
        // Error
        if (error) {
            throw Py::Exception(PartExceptionOCCError, error);
        }

        TopoDS_Edge edge = makeEdge.Edge();
        return Py::asObject(new TopoShapeEdgePy(new TopoShape(edge)));
    }
    Py::Object makePolygon(const Py::Tuple& args)
    {
        PyObject *pcObj;
        PyObject *pclosed=Py_False;
        if (!PyArg_ParseTuple(args.ptr(), "O|O!", &pcObj, &(PyBool_Type), &pclosed))
            throw Py::Exception();

        BRepBuilderAPI_MakePolygon mkPoly;
        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Base::VectorPy::Type))) {
                    Base::Vector3d v = static_cast<Base::VectorPy*>((*it).ptr())->value();
                    mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
                }
                else if (PyObject_TypeCheck((*it).ptr(), &PyTuple_Type)) {
                    Base::Vector3d v = Base::getVectorFromTuple<double>((*it).ptr());
                    mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
                }
            }

            if (!mkPoly.IsDone())
                Standard_Failure::Raise("Cannot create polygon because less than two vertices are given");

            // if the polygon should be closed
            if (Base::asBoolean(pclosed)) {
                if (!mkPoly.FirstVertex().IsSame(mkPoly.LastVertex())) {
                    mkPoly.Add(mkPoly.FirstVertex());
                }
            }

            return Py::asObject(new TopoShapeWirePy(new TopoShape(mkPoly.Wire())));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeCircle(const Py::Tuple& args)
    {
        double radius, angle1=0.0, angle2=360;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "d|O!O!dd",
            &radius,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir,
            &angle1, &angle2))
            throw Py::Exception();

        try {
            gp_Pnt loc(0,0,0);
            gp_Dir dir(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                loc.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                dir.SetCoord(vec.x, vec.y, vec.z);
            }
            gp_Ax1 axis(loc, dir);
            gp_Circ circle;
            circle.SetAxis(axis);
            circle.SetRadius(radius);

            Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
            BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, angle1*(M_PI/180), angle2*(M_PI/180));
            TopoDS_Edge edge = aMakeEdge.Edge();
            return Py::asObject(new TopoShapeEdgePy(new TopoShape(edge)));
        }
        catch (Standard_Failure&) {
            throw Py::Exception(PartExceptionOCCError, "creation of circle failed");
        }
    }
    Py::Object makeSphere(const Py::Tuple& args)
    {
        double radius, angle1=-90, angle2=90, angle3=360;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "d|O!O!ddd",
            &radius,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir,
            &angle1, &angle2, &angle3))
            throw Py::Exception();

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrimAPI_MakeSphere mkSphere(gp_Ax2(p,d), radius, angle1*(M_PI/180), angle2*(M_PI/180), angle3*(M_PI/180));
            TopoDS_Shape shape = mkSphere.Shape();
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(shape)));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of sphere failed");
        }
    }
    Py::Object makeCylinder(const Py::Tuple& args)
    {
        double radius, height, angle=360;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "dd|O!O!d",
            &radius, &height,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir,
            &angle))
            throw Py::Exception();

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrimAPI_MakeCylinder mkCyl(gp_Ax2(p,d),radius, height, angle*(M_PI/180));
            TopoDS_Shape shape = mkCyl.Shape();
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(shape)));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of cylinder failed");
        }
    }
    Py::Object makeCone(const Py::Tuple& args)
    {
        double radius1, radius2,  height, angle=360;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "ddd|O!O!d",
            &radius1, &radius2, &height,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir,
            &angle))
            throw Py::Exception();

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrimAPI_MakeCone mkCone(gp_Ax2(p,d),radius1, radius2, height, angle*(M_PI/180));
            TopoDS_Shape shape = mkCone.Shape();
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(shape)));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of cone failed");
        }
    }
    Py::Object makeTorus(const Py::Tuple& args)
    {
        double radius1, radius2, angle1=0.0, angle2=360, angle=360;
        PyObject *pPnt=nullptr, *pDir=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "dd|O!O!ddd",
            &radius1, &radius2,
            &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir,
            &angle1, &angle2, &angle))
            throw Py::Exception();

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }
            BRepPrimAPI_MakeTorus mkTorus(gp_Ax2(p,d), radius1, radius2, angle1*(M_PI/180), angle2*(M_PI/180), angle*(M_PI/180));
            const TopoDS_Shape& shape = mkTorus.Shape();
            return Py::asObject(new TopoShapeSolidPy(new TopoShape(shape)));
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of torus failed");
        }
    }
    Py::Object makeHelix(const Py::Tuple& args)
    {
        double pitch, height, radius, angle=-1.0;
        PyObject *pleft=Py_False;
        PyObject *pvertHeight=Py_False;
        if (!PyArg_ParseTuple(args.ptr(), "ddd|dO!O!",
            &pitch, &height, &radius, &angle,
            &(PyBool_Type), &pleft,
            &(PyBool_Type), &pvertHeight))
            throw Py::Exception();

        try {
            TopoShape helix;
            Standard_Boolean anIsLeft = Base::asBoolean(pleft);
            Standard_Boolean anIsVertHeight = Base::asBoolean(pvertHeight);
            TopoDS_Shape wire = helix.makeHelix(pitch, height, radius, angle,
                                                anIsLeft, anIsVertHeight);
            return Py::asObject(new TopoShapeWirePy(new TopoShape(wire)));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeLongHelix(const Py::Tuple& args)
    {
        double pitch, height, radius, angle=-1.0;
        PyObject *pleft=Py_False;
        if (!PyArg_ParseTuple(args.ptr(), "ddd|dO!", &pitch, &height, &radius, &angle,
                                               &(PyBool_Type), &pleft)) {
            throw Py::RuntimeError("Part.makeLongHelix fails on parms");
        }

        try {
            TopoShape helix;
            Standard_Boolean anIsLeft = Base::asBoolean(pleft);
            TopoDS_Shape wire = helix.makeLongHelix(pitch, height, radius, angle, anIsLeft);
            return Py::asObject(new TopoShapeWirePy(new TopoShape(wire)));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeThread(const Py::Tuple& args)
    {
        double pitch, depth, height, radius;
        if (!PyArg_ParseTuple(args.ptr(), "dddd", &pitch, &depth, &height, &radius))
            throw Py::Exception();

        try {
            TopoShape helix;
            TopoDS_Shape wire = helix.makeThread(pitch, depth, height, radius);
            return Py::asObject(new TopoShapeWirePy(new TopoShape(wire)));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeRevolution(const Py::Tuple& args)
    {
        double vmin = DBL_MAX, vmax=-DBL_MAX;
        double angle=360;
        PyObject *pPnt=nullptr, *pDir=nullptr, *pCrv;
        Handle(Geom_Curve) curve;
        PyObject* defaultType = Base::getTypeAsObject(&Part::TopoShapeSolidPy::Type);
        PyObject* type = defaultType;

        do {
            if (PyArg_ParseTuple(args.ptr(), "O!|dddO!O!O!", &(GeometryPy::Type), &pCrv,
                                                       &vmin, &vmax, &angle,
                                                       &(Base::VectorPy::Type), &pPnt,
                                                       &(Base::VectorPy::Type), &pDir,
                                                       &(PyType_Type), &type)) {
                GeometryPy* pcGeo = static_cast<GeometryPy*>(pCrv);
                curve = Handle(Geom_Curve)::DownCast
                    (pcGeo->getGeometryPtr()->handle());
                if (curve.IsNull()) {
                    throw Py::Exception(PyExc_TypeError, "geometry is not a curve");
                }
                if (vmin == DBL_MAX)
                    vmin = curve->FirstParameter();

                if (vmax == -DBL_MAX)
                    vmax = curve->LastParameter();
                break;
            }

            PyErr_Clear();
            if (PyArg_ParseTuple(args.ptr(), "O!|dddO!O!O!", &(TopoShapePy::Type), &pCrv,
                                                       &vmin, &vmax, &angle,
                                                       &(Base::VectorPy::Type), &pPnt,
                                                       &(Base::VectorPy::Type), &pDir,
                                                       &(PyType_Type), &type)) {
                const TopoDS_Shape& shape = static_cast<TopoShapePy*>(pCrv)->getTopoShapePtr()->getShape();
                if (shape.IsNull()) {
                    throw Py::Exception(PartExceptionOCCError, "shape is empty");
                }

                if (shape.ShapeType() != TopAbs_EDGE) {
                    throw Py::Exception(PartExceptionOCCError, "shape is not an edge");
                }

                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve adapt(edge);

                const Handle(Geom_Curve)& hCurve = adapt.Curve().Curve();
                // Apply placement of the shape to the curve
                TopLoc_Location loc = edge.Location();
                curve = Handle(Geom_Curve)::DownCast(hCurve->Transformed(loc.Transformation()));
                if (curve.IsNull()) {
                    throw Py::Exception(PartExceptionOCCError, "invalid curve in edge");
                }

                if (vmin == DBL_MAX)
                    vmin = adapt.FirstParameter();
                if (vmax == -DBL_MAX)
                    vmax = adapt.LastParameter();
                break;
            }

            // invalid arguments
            throw Py::TypeError("Expected arguments are:\n"
                                "Curve or Edge, [float, float, float, Vector, Vector, ShapeType]");
        }
        while(false);

        try {
            gp_Pnt p(0,0,0);
            gp_Dir d(0,0,1);
            if (pPnt) {
                Base::Vector3d pnt = static_cast<Base::VectorPy*>(pPnt)->value();
                p.SetCoord(pnt.x, pnt.y, pnt.z);
            }
            if (pDir) {
                Base::Vector3d vec = static_cast<Base::VectorPy*>(pDir)->value();
                d.SetCoord(vec.x, vec.y, vec.z);
            }

            PyObject* shellType = Base::getTypeAsObject(&Part::TopoShapeShellPy::Type);
            PyObject* faceType = Base::getTypeAsObject(&Part::TopoShapeFacePy::Type);

            BRepPrimAPI_MakeRevolution mkRev(gp_Ax2(p,d),curve, vmin, vmax, angle*(M_PI/180));
            if (type == defaultType) {
                TopoDS_Shape shape = mkRev.Solid();
                return Py::asObject(new TopoShapeSolidPy(new TopoShape(shape)));
            }
            else if (type == shellType) {
                TopoDS_Shape shape = mkRev.Shell();
                return Py::asObject(new TopoShapeShellPy(new TopoShape(shape)));
            }
            else if (type == faceType) {
                TopoDS_Shape shape = mkRev.Face();
                return Py::asObject(new TopoShapeFacePy(new TopoShape(shape)));
            }
            else {
                TopoDS_Shape shape = mkRev.Shape();
                return Py::asObject(new TopoShapePy(new TopoShape(shape)));
            }
        }
        catch (Standard_DomainError&) {
            throw Py::Exception(PartExceptionOCCDomainError, "creation of revolved shape failed");
        }
    }
    Py::Object makeRuledSurface(const Py::Tuple& args)
    {
        // http://opencascade.blogspot.com/2009/10/surface-modeling-part1.html
        PyObject *sh1, *sh2;
        if (!PyArg_ParseTuple(args.ptr(), "O!O!", &(TopoShapePy::Type), &sh1,
                                            &(TopoShapePy::Type), &sh2))
            throw Py::Exception();

        const TopoDS_Shape& shape1 = static_cast<TopoShapePy*>(sh1)->getTopoShapePtr()->getShape();
        const TopoDS_Shape& shape2 = static_cast<TopoShapePy*>(sh2)->getTopoShapePtr()->getShape();

        try {
            if (shape1.ShapeType() == TopAbs_EDGE && shape2.ShapeType() == TopAbs_EDGE) {
                TopoDS_Face face = BRepFill::Face(TopoDS::Edge(shape1), TopoDS::Edge(shape2));
                return Py::asObject(new TopoShapeFacePy(new TopoShape(face)));
            }
            else if (shape1.ShapeType() == TopAbs_WIRE && shape2.ShapeType() == TopAbs_WIRE) {
                TopoDS_Shell shell = BRepFill::Shell(TopoDS::Wire(shape1), TopoDS::Wire(shape2));
                return Py::asObject(new TopoShapeShellPy(new TopoShape(shell)));
            }
            else {
                throw Py::Exception(PartExceptionOCCError, "curves must either be edges or wires");
            }
        }
        catch (Standard_Failure&) {
            throw Py::Exception(PartExceptionOCCError, "creation of ruled surface failed");
        }
    }
    Py::Object makeShellFromWires(const Py::Tuple& args)
    {
        PyObject *pylist;
        if (!PyArg_ParseTuple(args.ptr(), "O", &pylist))
            throw Py::Exception();

        try {
            BRepFill_Generator fill;
            Py::Sequence list(pylist);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::TopoShape shape(*it);
                const TopoDS_Shape& s = shape.extensionObject()->getTopoShapePtr()->getShape();
                if (!s.IsNull() && s.ShapeType() == TopAbs_WIRE) {
                    fill.AddWire(TopoDS::Wire(s));
                }
            }

            fill.Perform();
            return Py::asObject(new TopoShapeShellPy(new TopoShape(fill.Shell())));
        }
        catch (Standard_Failure&) {
            throw Py::Exception(PartExceptionOCCError, "creation of shell failed");
        }
    }
    Py::Object makeTube(const Py::Tuple& args)
    {
        PyObject *pshape;
        double radius;
        double tolerance=0.001;
        char* scont = "C0";
        int maxdegree = 3;
        int maxsegment = 30;

        // Path + radius
        if (!PyArg_ParseTuple(args.ptr(), "O!d|sii", &(TopoShapePy::Type), &pshape, &radius, &scont, &maxdegree, &maxsegment))
            throw Py::Exception();

        std::string str_cont = scont;
        int cont;
        if (str_cont == "C0")
            cont = (int)GeomAbs_C0;
        else if (str_cont == "C1")
            cont = (int)GeomAbs_C1;
        else if (str_cont == "C2")
            cont = (int)GeomAbs_C2;
        else if (str_cont == "C3")
            cont = (int)GeomAbs_C3;
        else if (str_cont == "CN")
            cont = (int)GeomAbs_CN;
        else if (str_cont == "G1")
            cont = (int)GeomAbs_G1;
        else if (str_cont == "G2")
            cont = (int)GeomAbs_G2;
        else
            cont = (int)GeomAbs_C0;

        try {
            const TopoDS_Shape& path_shape = static_cast<TopoShapePy*>(pshape)->getTopoShapePtr()->getShape();
            TopoShape myShape(path_shape);
            TopoDS_Shape face = myShape.makeTube(radius, tolerance, cont, maxdegree, maxsegment);
            return Py::asObject(new TopoShapeFacePy(new TopoShape(face)));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeSweepSurface(const Py::Tuple& args)
    {
        PyObject *path, *profile;
        double tolerance=0.001;
        int fillMode = 0;

        // Path + profile
        if (!PyArg_ParseTuple(args.ptr(), "O!O!|di", &(TopoShapePy::Type), &path,
                                               &(TopoShapePy::Type), &profile,
                                               &tolerance, &fillMode))
            throw Py::Exception();

        try {
            const TopoDS_Shape& path_shape = static_cast<TopoShapePy*>(path)->getTopoShapePtr()->getShape();
            const TopoDS_Shape& prof_shape = static_cast<TopoShapePy*>(profile)->getTopoShapePtr()->getShape();

            TopoShape myShape(path_shape);
            TopoDS_Shape face = myShape.makeSweep(prof_shape, tolerance, fillMode);
            return Py::asObject(new TopoShapeFacePy(new TopoShape(face)));
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeLoft(const Py::Tuple& args)
    {
        PyObject *pcObj;
        PyObject *psolid=Py_False;
        PyObject *pruled=Py_False;
        PyObject *pclosed=Py_False;
        int degMax = 5;
        if (!PyArg_ParseTuple(args.ptr(), "O|O!O!O!i", &pcObj,
                                              &(PyBool_Type), &psolid,
                                              &(PyBool_Type), &pruled,
                                              &(PyBool_Type), &pclosed,
                                              &degMax)) {
            throw Py::Exception();
        }

        TopTools_ListOfShape profiles;
        Py::Sequence list(pcObj);

        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                    getTopoShapePtr()->getShape();
                profiles.Append(sh);
            }
        }

        TopoShape myShape;
        Standard_Boolean anIsSolid = Base::asBoolean(psolid);
        Standard_Boolean anIsRuled = Base::asBoolean(pruled);
        Standard_Boolean anIsClosed = Base::asBoolean(pclosed);
        TopoDS_Shape aResult = myShape.makeLoft(profiles, anIsSolid, anIsRuled, anIsClosed, degMax);
        return Py::asObject(new TopoShapePy(new TopoShape(aResult)));

    }
    Py::Object makeSplitShape(const Py::Tuple& args)
    {
        PyObject* shape;
        PyObject* list;
        PyObject* checkInterior = Py_True;
        if (!PyArg_ParseTuple(args.ptr(), "O!O|O!", &(TopoShapePy::Type), &shape, &list,
                                                    &PyBool_Type, &checkInterior))
            throw Py::Exception();

        try {
            TopoDS_Shape initShape = static_cast<TopoShapePy*>
                    (shape)->getTopoShapePtr()->getShape();
            BRepFeat_SplitShape splitShape(initShape);
            splitShape.SetCheckInterior(Base::asBoolean(checkInterior));

            Py::Sequence seq(list);
            for (Py::Sequence::iterator it = seq.begin(); it != seq.end(); ++it) {
                Py::Tuple tuple(*it);
                Py::TopoShape sh1(tuple[0]);
                Py::TopoShape sh2(tuple[1]);
                const TopoDS_Shape& shape1= sh1.extensionObject()->getTopoShapePtr()->getShape();
                const TopoDS_Shape& shape2= sh2.extensionObject()->getTopoShapePtr()->getShape();
                if (shape1.IsNull() || shape2.IsNull())
                    throw Py::RuntimeError("Cannot add null shape");
                if (shape2.ShapeType() == TopAbs_FACE) {
                    if (shape1.ShapeType() == TopAbs_EDGE) {
                        splitShape.Add(TopoDS::Edge(shape1), TopoDS::Face(shape2));
                    }
                    else if (shape1.ShapeType() == TopAbs_WIRE) {
                        splitShape.Add(TopoDS::Wire(shape1), TopoDS::Face(shape2));
                    }
                    else if (shape1.ShapeType() == TopAbs_COMPOUND) {
                        splitShape.Add(TopoDS::Compound(shape1), TopoDS::Face(shape2));
                    }
                    else {
                        throw Py::TypeError("First item in tuple must be Edge, Wire or Compound");
                    }
                }
                else if (shape2.ShapeType() == TopAbs_EDGE) {
                    if (shape1.ShapeType() == TopAbs_EDGE) {
                        splitShape.Add(TopoDS::Edge(shape1), TopoDS::Edge(shape2));
                    }
                    else {
                        throw Py::TypeError("First item in tuple must be Edge");
                    }
                }
                else {
                    throw Py::TypeError("Second item in tuple must be Face or Edge");
                }
            }

            splitShape.Build();
            const TopTools_ListOfShape& d = splitShape.DirectLeft();
            const TopTools_ListOfShape& l = splitShape.Left();

            Py::List list1;
            for (TopTools_ListIteratorOfListOfShape it(d); it.More(); it.Next()) {
                list1.append(shape2pyshape(it.Value()));
            }

            Py::List list2;
            for (TopTools_ListIteratorOfListOfShape it(l); it.More(); it.Next()) {
                list2.append(shape2pyshape(it.Value()));
            }

            Py::Tuple tuple(2);
            tuple.setItem(0, list1);
            tuple.setItem(1, list2);
            return tuple;
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(PartExceptionOCCError, e.GetMessageString());
        }
    }
    Py::Object makeWireString(const Py::Tuple& args)
    {
#ifdef FCUseFreeType
        PyObject *intext;
        const char* dir;
        const char* fontfile;
        const char* fontspec;
        bool useFontSpec = false;
        double height;
        double track = 0;

        Py_UNICODE *unichars = nullptr;
        Py_ssize_t pysize;

        PyObject *CharList;

        if (PyArg_ParseTuple(args.ptr(), "Ossd|d", &intext,                               // compatibility with old version
                                         &dir,
                                         &fontfile,
                                         &height,
                                         &track)) {
            useFontSpec = false;
        }
        else {
            PyErr_Clear();
            if (PyArg_ParseTuple(args.ptr(), "Osd|d", &intext,
                                            &fontspec,
                                            &height,
                                            &track)) {
                useFontSpec = true;
            }
            else {
                throw Py::TypeError("** makeWireString bad args.");
            }
        }

        //FIXME: Test this!
        if (PyBytes_Check(intext)) {
            PyObject *p = Base::PyAsUnicodeObject(PyBytes_AsString(intext));
            if (!p) {
                throw Py::TypeError("** makeWireString can't convert PyString.");
            }

            pysize = PyUnicode_GetLength(p);
#if PY_VERSION_HEX < 0x03090000
            unichars = PyUnicode_AS_UNICODE(p);
#else
#ifdef FC_OS_WIN32
            //PyUNICODE is only 16 bits on Windows (wchar_t), so passing 32 bit UCS4
            //will result in unknown glyph in even positions, and wrong characters in
            //odd positions.
            unichars = (Py_UNICODE*)PyUnicode_AsWideCharString(p, &pysize);
#else
            unichars = (Py_UNICODE *)PyUnicode_AsUCS4Copy(p);
#endif
#endif
        }
        else if (PyUnicode_Check(intext)) {
            pysize = PyUnicode_GetLength(intext);

#if PY_VERSION_HEX < 0x03090000
            unichars = PyUnicode_AS_UNICODE(intext);
#else
#ifdef FC_OS_WIN32
            //PyUNICODE is only 16 bits on Windows (wchar_t), so passing 32 bit UCS4
            //will result in unknown glyph in even positions, and wrong characters in
            //odd positions.
            unichars = (Py_UNICODE*)PyUnicode_AsWideCharString(intext, &pysize);
#else
            unichars = (Py_UNICODE *)PyUnicode_AsUCS4Copy(intext);
#endif
#endif
        }
        else {
            throw Py::TypeError("** makeWireString bad text parameter");
        }

        try {
            if (useFontSpec) {
                CharList = FT2FC(unichars,pysize,fontspec,height,track);
            }
            else {
                CharList = FT2FC(unichars,pysize,dir,fontfile,height,track);
            }
#if PY_VERSION_HEX >= 0x03090000
            if (unichars) {
                PyMem_Free(unichars);
            }
#endif
        }
        catch (Standard_DomainError&) {                                      // Standard_DomainError is OCC error.
            throw Py::Exception(PartExceptionOCCDomainError, "makeWireString failed - Standard_DomainError");
        }
        catch (std::runtime_error& e) {                                     // FT2 or FT2FC errors
            throw Py::Exception(PartExceptionOCCError, e.what());
        }

        return Py::asObject(CharList);
#else
        throw Py::RuntimeError("FreeCAD compiled without FreeType support! This method is disabled...");
#endif
    }
    Py::Object exportUnits(const Py::Tuple& args)
    {
        char* unit=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "|s", &unit))
            throw Py::Exception();

        if (unit) {
            if (!Interface::writeIgesUnit(unit)) {
                throw Py::RuntimeError("Failed to set 'write.iges.unit'");
            }
            if (!Interface::writeStepUnit(unit)) {
                throw Py::RuntimeError("Failed to set 'write.step.unit'");
            }
        }

        Py::Dict dict;
        dict.setItem("write.iges.unit", Py::String(Interface::writeIgesUnit()));
        dict.setItem("write.step.unit", Py::String(Interface::writeStepUnit()));
        return dict;
    }
    Py::Object setStaticValue(const Py::Tuple& args)
    {
        char *name, *cval;
        if (PyArg_ParseTuple(args.ptr(), "ss", &name, &cval)) {
            if (!Interface_Static::SetCVal(name, cval)) {
                std::stringstream str;
                str << "Failed to set '" << name << "'";
                throw Py::RuntimeError(str.str());
            }
            return Py::None();
        }

        PyErr_Clear();
        PyObject* index_or_value;
        if (PyArg_ParseTuple(args.ptr(), "sO", &name, &index_or_value)) {
            if (PyLong_Check(index_or_value)) {
                int ival = (int)PyLong_AsLong(index_or_value);
                if (!Interface_Static::SetIVal(name, ival)) {
                    std::stringstream str;
                    str << "Failed to set '" << name << "'";
                    throw Py::RuntimeError(str.str());
                }
                return Py::None();
            }
            else if (PyFloat_Check(index_or_value)) {
                double rval = PyFloat_AsDouble(index_or_value);
                if (!Interface_Static::SetRVal(name, rval)) {
                    std::stringstream str;
                    str << "Failed to set '" << name << "'";
                    throw Py::RuntimeError(str.str());
                }
                return Py::None();
            }
        }

        throw Py::TypeError("First argument must be string and must be either string, int or float");
    }
    Py::Object cast_to_shape(const Py::Tuple& args)
    {
        PyObject *object;
        if (PyArg_ParseTuple(args.ptr(),"O!",&(Part::TopoShapePy::Type), &object)) {
            TopoShape* ptr = static_cast<TopoShapePy*>(object)->getTopoShapePtr();
            return Py::asObject(ptr->getPyObject());
        }

        throw Py::Exception();
    }
    Py::Object getSortedClusters(const Py::Tuple& args)
    {
        PyObject *obj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &obj)) {
            throw Py::Exception(PartExceptionOCCError, "list of edges expected");
        }

        Py::Sequence list(obj);
        std::vector<TopoDS_Edge> edges;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                if (sh.ShapeType() == TopAbs_EDGE)
                    edges.push_back(TopoDS::Edge(sh));
                else {
                    throw Py::TypeError("shape is not an edge");
                }
            }
            else {
                throw Py::TypeError("item is not a shape");
            }
        }

        Edgecluster acluster(edges);
        tEdgeClusterVector aclusteroutput = acluster.GetClusters();

        Py::List root_list;
        for (const auto & it : aclusteroutput) {
            Py::List add_list;
            for (const auto& it1 : it) {
                add_list.append(Py::Object(new TopoShapeEdgePy(new TopoShape(it1)),true));
            }
            root_list.append(add_list);
        }

        return root_list;
    }
    Py::Object sortEdges(const Py::Tuple& args)
    {
        PyObject *obj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &obj)) {
            throw Py::TypeError("list of edges expected");
        }

        Py::Sequence list(obj);
        std::list<TopoDS_Edge> edges;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                if (sh.ShapeType() == TopAbs_EDGE)
                    edges.push_back(TopoDS::Edge(sh));
                else {
                    throw Py::TypeError("shape is not an edge");
                }
            }
            else {
                throw Py::TypeError("item is not a shape");
            }
        }

        std::list<TopoDS_Edge> sorted = sort_Edges(Precision::Confusion(), edges);
        Py::List sorted_list;
        for (const auto & it : sorted) {
            sorted_list.append(Py::Object(new TopoShapeEdgePy(new TopoShape(it)),true));
        }

        return sorted_list;
    }
    Py::Object sortEdges2(const Py::Tuple& args)
    {
        PyObject *obj;
        double tol3d = Precision::Confusion();
        if (!PyArg_ParseTuple(args.ptr(), "O|d", &obj, &tol3d)) {
            throw Py::Exception(PartExceptionOCCError, "list of edges expected");
        }

        Py::Sequence list(obj);
        std::list<TopoDS_Edge> edges;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape();
                if (sh.ShapeType() == TopAbs_EDGE)
                    edges.push_back(TopoDS::Edge(sh));
                else {
                    throw Py::TypeError("shape is not an edge");
                }
            }
            else {
                throw Py::TypeError("item is not a shape");
            }
        }

        Py::List root_list;
        while(!edges.empty()) {
            std::list<TopoDS_Edge> sorted = sort_Edges(tol3d, edges);
            Py::List sorted_list;
            for (const auto & it : sorted) {
                sorted_list.append(Py::Object(new TopoShapeEdgePy(new TopoShape(it)),true));
            }
            root_list.append(sorted_list);
        }
        return root_list;
    }
    Py::Object toPythonOCC(const Py::Tuple& args)
    {
        PyObject *pcObj;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(TopoShapePy::Type), &pcObj))
            throw Py::Exception();

        try {
            TopoDS_Shape* shape = new TopoDS_Shape();
            (*shape) = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
            PyObject* proxy = nullptr;
            proxy = Base::Interpreter().createSWIGPointerObj("OCC.TopoDS", "TopoDS_Shape *", static_cast<void*>(shape), 1);
            return Py::asObject(proxy);
        }
        catch (const Base::Exception& e) {
            throw Py::Exception(PartExceptionOCCError, e.what());
        }
    }
    Py::Object fromPythonOCC(const Py::Tuple& args)
    {
        PyObject *proxy;
        if (!PyArg_ParseTuple(args.ptr(), "O", &proxy))
            throw Py::Exception();

        void* ptr;
        try {
            TopoShape* shape = new TopoShape();
            Base::Interpreter().convertSWIGPointerObj("OCC.TopoDS","TopoDS_Shape *", proxy, &ptr, 0);
            TopoDS_Shape* s = static_cast<TopoDS_Shape*>(ptr);
            shape->setShape(*s);
            return Py::asObject(new TopoShapePy(shape));
        }
        catch (const Base::Exception& e) {
            throw Py::Exception(PartExceptionOCCError, e.what());
        }
    }

    Py::Object getShape(const Py::Tuple& args, const Py::Dict &kwds) {
        PyObject *pObj;
        const char *subname = nullptr;
        PyObject *pyMat = nullptr;
        PyObject *needSubElement = Py_False;
        PyObject *transform = Py_True;
        PyObject *noElementMap = Py_False;
        PyObject *refine = Py_False;
        short retType = 0;
        static const std::array<const char *, 9> kwd_list{"obj", "subname", "mat",
                                                          "needSubElement", "transform", "retType", "noElementMap",
                                                          "refine", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|sO!O!O!hO!O!", kwd_list,
                                                 &App::DocumentObjectPy::Type, &pObj, &subname, &Base::MatrixPy::Type,
                                                 &pyMat,
                                                 &PyBool_Type, &needSubElement, &PyBool_Type, &transform, &retType,
                                                 &PyBool_Type, &noElementMap, &PyBool_Type, &refine)) {
            throw Py::Exception();
        }

        App::DocumentObject *obj =
            static_cast<App::DocumentObjectPy*>(pObj)->getDocumentObjectPtr();
        App::DocumentObject *subObj = nullptr;
        Base::Matrix4D mat;
        if(pyMat)
            mat = *static_cast<Base::MatrixPy*>(pyMat)->getMatrixPtr();
        auto shape = Feature::getTopoShape(obj,subname,Base::asBoolean(needSubElement),
                &mat,&subObj,retType==2,Base::asBoolean(transform),
                Base::asBoolean(noElementMap));
        if (Base::asBoolean(refine)) {
            // shape = TopoShape(0,shape.Hasher).makERefine(shape);
            BRepBuilderAPI_RefineModel mkRefine(shape.getShape());
            shape.setShape(mkRefine.Shape());
        }
        Py::Object sret(shape2pyshape(shape));
        if(retType==0)
            return sret;

        return Py::TupleN(sret,Py::asObject(new Base::MatrixPy(new Base::Matrix4D(mat))),
                subObj?Py::Object(subObj->getPyObject(),true):Py::Object());
    }

    Py::Object clearShapeCache(const Py::Tuple &args) {
        if (!PyArg_ParseTuple(args.ptr(),""))
            throw Py::Exception();
        Part::Feature::clearShapeCache();
        return Py::Object();
    }

    Py::Object splitSubname(const Py::Tuple& args) {
        const char *subname;
        if (!PyArg_ParseTuple(args.ptr(), "s",&subname))
            throw Py::Exception();
        auto element = Data::findElementName(subname);
        std::string sub(subname,element-subname);
        Py::List list;
        list.append(Py::String(sub));
        const char *dot = strchr(element,'.');
        if(!dot)
            dot = element+strlen(element);
        const char *mapped = Data::isMappedElement(element);
        if(mapped)
            list.append(Py::String(std::string(mapped,dot-mapped)));
        else
            list.append(Py::String());
        if(*dot=='.')
            list.append(Py::String(dot+1));
        else if(!mapped)
            list.append(Py::String(element));
        else
            list.append(Py::String());
        return list;
    }

    Py::Object joinSubname(const Py::Tuple& args) {
        const char *sub;
        const char *mapped;
        const char *element;
        if (!PyArg_ParseTuple(args.ptr(), "sss",&sub,&mapped,&element))
            throw Py::Exception();
        std::string subname(sub);
        if (!subname.empty() && subname[subname.size()-1]!='.')
            subname += '.';
        if (mapped && mapped[0]) {
            if (!Data::isMappedElement(mapped))
                subname += Data::ELEMENT_MAP_PREFIX;
            subname += mapped;
        }
        if (element && element[0]) {
            if (!subname.empty() && subname[subname.size()-1]!='.')
                subname += '.';
            subname += element;
        }
        return Py::String(subname);
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Part
