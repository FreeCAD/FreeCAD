/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <cstdlib>
# include <sstream>
# include <BRepLib.hxx>
# include <BSplCLib.hxx>
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepAdaptor_HCurve.hxx>
# include <BRepAdaptor_HCompCurve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgo_Fuse.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_NurbsConvert.hxx>
# include <BRepBuilderAPI_FaceError.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <BRepMesh.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRepMesh_Triangle.hxx>
# include <BRepMesh_Edge.hxx>
# include <BRepOffsetAPI_MakeThickSolid.hxx>
# include <BRepOffsetAPI_MakeOffsetShape.hxx>
# include <BRepOffsetAPI_MakePipe.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepOffsetAPI_Sewing.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ReShape.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <GeomLProp_SLProps.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomFill_CorrectedFrenet.hxx>
# include <GeomFill_CurveAndTrihedron.hxx>
# include <GeomFill_EvolvedSection.hxx>
# include <GeomFill_Pipe.hxx>
# include <GeomFill_SectionLaw.hxx>
# include <GeomFill_Sweep.hxx>
# include <Handle_Law_BSpFunc.hxx>
# include <Handle_Law_BSpline.hxx>
# include <Handle_TopTools_HSequenceOfShape.hxx>
# include <Law_BSpFunc.hxx>
# include <Law_Linear.hxx>
# include <Law_S.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <Interface_Static.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESControl_Writer.hxx>
# include <IGESControl_Reader.hxx>
# include <STEPControl_Writer.hxx>
# include <STEPControl_Reader.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <Geom2d_Ellipse.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_Circle.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Line.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Poly_Triangulation.hxx>
# include <Standard_Failure.hxx>
# include <StlAPI_Writer.hxx>
# include <Standard_Failure.hxx>
# include <gp_GTrsf.hxx>
# include <ShapeAnalysis_Shell.hxx>
# include <ShapeBuild_ReShape.hxx>
# include <ShapeFix_Edge.hxx>
# include <ShapeFix_Face.hxx>
# include <ShapeFix_Shell.hxx>
# include <ShapeFix_Solid.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <ShapeUpgrade_RemoveInternalWires.hxx>
# include <Standard_Version.hxx>
#endif
# include <Poly_Polygon3D.hxx>
# include <Poly_PolygonOnTriangulation.hxx>
# include <BRepMesh.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <ShapeFix_Shape.hxx>
# include <XSControl_WorkSession.hxx>
# include <Transfer_TransientProcess.hxx>
# include <Transfer_FinderProcess.hxx>
# include <APIHeaderSection_MakeHeader.hxx>

#include <Base/Builder3D.h>
#include <Base/FileInfo.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include "TopoShape.h"
#include "CrossSection.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeVertexPy.h"
#include "ProgressIndicator.h"
#include "modelRefine.h"
#include "Tools.h"

using namespace Part;

const char* BRepBuilderAPI_FaceErrorText(BRepBuilderAPI_FaceError et)
{
    switch (et)
    {
    case BRepBuilderAPI_FaceDone:
        return "Construction was successful";
    case BRepBuilderAPI_NoFace:
        return "No face";
    case BRepBuilderAPI_NotPlanar:
        return "Face is not planar";
    case BRepBuilderAPI_CurveProjectionFailed:
        return "Curve projection failed";
    case BRepBuilderAPI_ParametersOutOfRange:
        return "Parameters out of range";
#if OCC_HEX_VERSION < 0x060500
    case BRepBuilderAPI_SurfaceNotC2:
        return "Surface not C2-continous";
#endif
    default:
        return "Unknown creation error";
    }
}

// ------------------------------------------------

TYPESYSTEM_SOURCE(Part::ShapeSegment , Data::Segment);

std::string ShapeSegment::getName() const
{
    return std::string();
}

// ------------------------------------------------

TYPESYSTEM_SOURCE(Part::TopoShape , Data::ComplexGeoData);

TopoShape::TopoShape()
{
}

TopoShape::~TopoShape()
{
}

TopoShape::TopoShape(const TopoDS_Shape& shape)
  : _Shape(shape)
{
}

TopoShape::TopoShape(const TopoShape& shape)
  : _Shape(shape._Shape)
{
}

std::vector<const char*> TopoShape::getElementTypes(void) const
{
    std::vector<const char*> temp(3);
    temp.push_back("Vertex");
    temp.push_back("Edge");
    temp.push_back("Face");

    return temp;
}

unsigned long TopoShape::countSubElements(const char* Type) const
{
    return countSubShapes(Type);
}

Data::Segment* TopoShape::getSubElement(const char* Type, unsigned long n) const
{
    std::stringstream str;
    str << Type << n;
    std::string temp = str.str();
    return new ShapeSegment(getSubShape(temp.c_str()));
}

void TopoShape::getLinesFromSubelement(const Data::Segment* element,
                                       std::vector<Base::Vector3d> &Points,
                                       std::vector<Line> &lines) const
{
}

void TopoShape::getFacesFromSubelement(const Data::Segment* element,
                                       std::vector<Base::Vector3d> &Points,
                                       std::vector<Base::Vector3d> &PointNormals,
                                       std::vector<Facet> &faces) const
{
    if (element->getTypeId() == ShapeSegment::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const ShapeSegment*>(element)->Shape;
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
            return;
    
        TopLoc_Location aLoc;
        // doing the meshing and checking the result
        Handle(Poly_Triangulation) aPoly = BRep_Tool::Triangulation(TopoDS::Face(shape),aLoc);
        if (aPoly.IsNull())
            return;

        // geting the transformation of the shape/face
        gp_Trsf myTransf;
        Standard_Boolean identity = true;
        if (!aLoc.IsIdentity())  {
            identity = false;
            myTransf = aLoc.Transformation();
        }

        Standard_Integer i;
        // geting size and create the array
        int nbNodesInFace = aPoly->NbNodes();
        int nbTriInFace = aPoly->NbTriangles();
        Points.resize(nbNodesInFace);
        PointNormals.resize(nbNodesInFace); // fills up already the array
        faces.resize(nbTriInFace);

        // check orientation
        TopAbs_Orientation orient = shape.Orientation();

        // cycling through the poly mesh
        const Poly_Array1OfTriangle& Triangles = aPoly->Triangles();
        const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();
        for (i=1; i<=nbTriInFace; i++) {
            // Get the triangle
            Standard_Integer N1,N2,N3;
            Triangles(i).Get(N1,N2,N3);

            // change orientation of the triangles
            if (orient != TopAbs_FORWARD) {
                Standard_Integer tmp = N1;
                N1 = N2;
                N2 = tmp;
            }

            gp_Pnt V1 = Nodes(N1);
            gp_Pnt V2 = Nodes(N2);
            gp_Pnt V3 = Nodes(N3);

            // transform the vertices to the place of the face
            if (!identity) {
                V1.Transform(myTransf);
                V2.Transform(myTransf);
                V3.Transform(myTransf);
            }

            // Calculate triangle normal
            gp_Vec v1(V1.X(),V1.Y(),V1.Z()),v2(V2.X(),V2.Y(),V2.Z()),v3(V3.X(),V3.Y(),V3.Z());
            gp_Vec Normal = (v2-v1)^(v3-v1);

            //Standard_Real Area = 0.5 * Normal.Magnitude();

            // add the triangle normal to the vertex normal for all points of this triangle
            PointNormals[N1-1] += Base::Vector3d(Normal.X(),Normal.Y(),Normal.Z());
            PointNormals[N2-1] += Base::Vector3d(Normal.X(),Normal.Y(),Normal.Z());
            PointNormals[N3-1] += Base::Vector3d(Normal.X(),Normal.Y(),Normal.Z());

            Points[N1-1].Set(V1.X(),V1.Y(),V1.Z());
            Points[N2-1].Set(V2.X(),V2.Y(),V2.Z());
            Points[N3-1].Set(V3.X(),V3.Y(),V3.Z());

            int j = i - 1;
            N1--;
            N2--;
            N3--;
            faces[j].I1 = N1;
            faces[j].I2 = N2;
            faces[j].I3 = N3;
        }

        // normalize all vertex normals
        for (i=0; i < nbNodesInFace; i++) {
            gp_Dir clNormal;
            try {
                Handle_Geom_Surface Surface = BRep_Tool::Surface(TopoDS::Face(shape));

                gp_Pnt vertex(Base::convertTo<gp_Pnt>(Points[i]));
                GeomAPI_ProjectPointOnSurf ProPntSrf(vertex, Surface);
                Standard_Real fU, fV;
                ProPntSrf.Parameters(1, fU, fV);

                GeomLProp_SLProps clPropOfFace(Surface, fU, fV, 2, gp::Resolution());

                clNormal = clPropOfFace.Normal();
                Base::Vector3d temp = Base::convertTo<Base::Vector3d>(clNormal);
                if (temp * Points[i] < 0)
                    temp = -temp;
                Points[i] = temp;
            }
            catch (...) {
            }
            Points[i].Normalize();
        }
    }
}

TopoDS_Shape TopoShape::getSubShape(const char* Type) const
{
    if (!Type)
        Standard_Failure::Raise("No sub-shape type given");
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot get sub-shape from empty shape");

    std::string shapetype(Type);
    if (shapetype.size() > 4 && shapetype.substr(0,4) == "Face") {
        int index=std::atoi(&shapetype[4]);
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_FACE, anIndices);
        // To avoid a segmentation fault we have to check if container is empty
        if (anIndices.IsEmpty())
            Standard_Failure::Raise("Shape has no faces");
        return anIndices.FindKey(index);
    }
    else if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") {
        int index=std::atoi(&shapetype[4]);
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_EDGE, anIndices);
        // To avoid a segmentation fault we have to check if container is empty
        if (anIndices.IsEmpty())
            Standard_Failure::Raise("Shape has no edges");
        return anIndices.FindKey(index);
    }
    else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
        int index=std::atoi(&shapetype[6]);
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_VERTEX, anIndices);
        // To avoid a segmentation fault we have to check if container is empty
        if (anIndices.IsEmpty())
            Standard_Failure::Raise("Shape has no vertexes");
        return anIndices.FindKey(index);
    }

    Standard_Failure::Raise("Not supported sub-shape type");
    return TopoDS_Shape(); // avoid compiler warning
}

unsigned long TopoShape::countSubShapes(const char* Type) const
{
    std::string shapetype(Type);
    if (shapetype == "Face") {
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_FACE, anIndices);
        return anIndices.Extent();
    }
    else if (shapetype == "Edge") {
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_EDGE, anIndices);
        return anIndices.Extent();
    }
    else if (shapetype == "Vertex") {
        TopTools_IndexedMapOfShape anIndices;
        TopExp::MapShapes(this->_Shape, TopAbs_VERTEX, anIndices);
        return anIndices.Extent();
    }

    return 0;
}

PyObject * TopoShape::getPySubShape(const char* Type) const
{
    // get the shape
    TopoDS_Shape Shape = getSubShape(Type);
    // destinquish the return type
    std::string shapetype(Type);
    if (shapetype.size() > 4 && shapetype.substr(0,4) == "Face") 
        return new TopoShapeFacePy(new TopoShape(Shape));
    else if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") 
        return new TopoShapeEdgePy(new TopoShape(Shape));
    else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") 
        return new TopoShapeVertexPy(new TopoShape(Shape));
    else 
        return 0;

}

void TopoShape::operator = (const TopoShape& sh)
{
    if (this != &sh) {
        this->_Shape = sh._Shape;
    }
}

void TopoShape::convertTogpTrsf(const Base::Matrix4D& mtrx, gp_Trsf& trsf)
{
    trsf.SetValues(mtrx[0][0],mtrx[0][1],mtrx[0][2],mtrx[0][3],
                   mtrx[1][0],mtrx[1][1],mtrx[1][2],mtrx[1][3],
                   mtrx[2][0],mtrx[2][1],mtrx[2][2],mtrx[2][3],
                   0.00001,0.00001);
}

void TopoShape::convertToMatrix(const gp_Trsf& trsf, Base::Matrix4D& mtrx)
{
    gp_Mat m = trsf._CSFDB_Getgp_Trsfmatrix();
    gp_XYZ p = trsf._CSFDB_Getgp_Trsfloc();
    Standard_Real scale = trsf._CSFDB_Getgp_Trsfscale();

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
}

void TopoShape::setTransform(const Base::Matrix4D& rclTrf)
{
    gp_Trsf mov;
    convertTogpTrsf(rclTrf, mov);
    TopLoc_Location loc(mov);
    _Shape.Location(loc);
}

Base::Matrix4D TopoShape::getTransform(void) const
{
    Base::Matrix4D mtrx;
    gp_Trsf Trf = _Shape.Location().Transformation();
    convertToMatrix(Trf, mtrx);
    return mtrx;
}

void TopoShape::read(const char *FileName)
{
    Base::FileInfo File(FileName);
  
    // checking on the file
    if (!File.isReadable())
        throw Base::FileException("File to load not existing or not readable", FileName);
    
    if (File.hasExtension("igs") || File.hasExtension("iges")) {
        // read iges file
        importIges(File.filePath().c_str());
    }
    else if (File.hasExtension("stp") || File.hasExtension("step")) {
        importStep(File.filePath().c_str());
    }
    else if (File.hasExtension("brp") || File.hasExtension("brep")) {
        // read brep-file
        importBrep(File.filePath().c_str());
    }
    else{
        throw Base::Exception("Unknown extension");
    }
}

/*!
 Example code to get the labels for each face in an IGES file.
 \code
#include <Handle_XSControl_WorkSession.hxx>
#include <Handle_XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Handle_IGESData_IGESModel.hxx>
#include <IGESData_IGESModel.hxx>
#include <IGESData_IGESEntity.hxx>

IGESControl_Reader aReader;
...
// Gets the labels of all face items if defined in the IGES file
Handle_XSControl_WorkSession ws = aReader.WS();
Handle_XSControl_TransferReader tr = ws->TransferReader();

std::string name;
Handle(IGESData_IGESModel) aModel = aReader.IGESModel();
Standard_Integer all = aModel->NbEntities();

TopExp_Explorer ex;
for (ex.Init(this->_Shape, TopAbs_FACE); ex.More(); ex.Next())
{
    const TopoDS_Face& aFace = TopoDS::Face(ex.Current());
    Handle_Standard_Transient ent = tr->EntityFromShapeResult(aFace, 1);
    if (!ent.IsNull()) {
        int i = aModel->Number(ent);
        if (i > 0) {
            Handle_IGESData_IGESEntity ie = aModel->Entity(i);
            if (ie->HasShortLabel())
                name = ie->ShortLabel()->ToCString();
        }
    }
}
\endcode
*/
void TopoShape::importIges(const char *FileName)
{
    try {
        // read iges file
        // http://www.opencascade.org/org/forum/thread_20801/
        IGESControl_Controller::Init();
        Interface_Static::SetIVal("read.surfacecurve.mode",3);
        IGESControl_Reader aReader;
        if (aReader.ReadFile((const Standard_CString)FileName) != IFSelect_RetDone)
            throw Base::Exception("Error in reading IGES");

        Handle_Message_ProgressIndicator pi = new ProgressIndicator(100);
        pi->NewScope(100, "Reading IGES file...");
        pi->Show();
        aReader.WS()->MapReader()->SetProgress(pi);

        // make brep
        aReader.ClearShapes();
        aReader.TransferRoots();
        // one shape that contains all subshapes
        this->_Shape = aReader.OneShape();
        pi->EndScope();
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
}

void TopoShape::importStep(const char *FileName)
{
    try {
        STEPControl_Reader aReader;
        if (aReader.ReadFile((const Standard_CString)FileName) != IFSelect_RetDone)
            throw Base::Exception("Error in reading STEP");

        Handle_Message_ProgressIndicator pi = new ProgressIndicator(100);
        aReader.WS()->MapReader()->SetProgress(pi);
        pi->NewScope(100, "Reading STEP file...");
        pi->Show();

        // Root transfers
        aReader.TransferRoots();
        // one shape that contains all subshapes
        this->_Shape = aReader.OneShape();
        pi->EndScope();
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
}

void TopoShape::importBrep(const char *FileName)
{
    try {
        // read brep-file
        BRep_Builder aBuilder;
        TopoDS_Shape aShape;
    #if OCC_HEX_VERSION >= 0x060300
        Handle_Message_ProgressIndicator pi = new ProgressIndicator(100);
        pi->NewScope(100, "Reading BREP file...");
        pi->Show();
        BRepTools::Read(aShape,(const Standard_CString)FileName,aBuilder,pi);
        pi->EndScope();
    #else
        BRepTools::Read(aShape,(const Standard_CString)FileName,aBuilder);
    #endif
        this->_Shape = aShape;
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
}

void TopoShape::importBrep(std::istream& str)
{
    try {
        // read brep-file
        BRep_Builder aBuilder;
        TopoDS_Shape aShape;
#if OCC_HEX_VERSION >= 0x060300
        Handle_Message_ProgressIndicator pi = new ProgressIndicator(100);
        pi->NewScope(100, "Reading BREP file...");
        pi->Show();
        BRepTools::Read(aShape,str,aBuilder,pi);
        pi->EndScope();
#else
        BRepTools::Read(aShape,str,aBuilder);
#endif
        this->_Shape = aShape;
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
    catch (const std::exception& e) {
        throw Base::Exception(e.what());
    }
}

void TopoShape::write(const char *FileName) const
{
    Base::FileInfo File(FileName);
    
    if (File.hasExtension("igs") || File.hasExtension("iges")) {
        // write iges file
        exportIges(File.filePath().c_str());
    }
    else if (File.hasExtension("stp") || File.hasExtension("step")) {
        exportStep(File.filePath().c_str());
    }
    else if (File.hasExtension("brp") || File.hasExtension("brep")) {
        // read brep-file
        exportBrep(File.filePath().c_str());
    }
    else if (File.hasExtension("stl")) {
        // read brep-file
        exportStl(File.filePath().c_str());
    }
    else{
        throw Base::Exception("Unknown extension");
    }
}

void TopoShape::exportIges(const char *filename) const
{
    Interface_Static::SetCVal("write.iges.unit","IN");
    try {
        // write iges file
        IGESControl_Controller::Init();
        IGESControl_Writer aWriter;
        //IGESControl_Writer aWriter(Interface_Static::CVal("write.iges.unit"), 1);
        aWriter.AddShape(this->_Shape);
        aWriter.ComputeModel();
        if (aWriter.Write((const Standard_CString)filename) != IFSelect_RetDone)
            throw Base::Exception("Writing of IGES failed");
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
}

void TopoShape::exportStep(const char *filename) const
{
    try {
        // write step file
        STEPControl_Writer aWriter;

        Handle_Message_ProgressIndicator pi = new ProgressIndicator(100);
        aWriter.WS()->MapWriter()->SetProgress(pi);
        pi->NewScope(100, "Writing STEP file...");
        pi->Show();

        if (aWriter.Transfer(this->_Shape, STEPControl_AsIs) != IFSelect_RetDone)
            throw Base::Exception("Error in transferring STEP");

        APIHeaderSection_MakeHeader makeHeader(aWriter.Model());
        makeHeader.SetName(new TCollection_HAsciiString((const Standard_CString)filename));
        makeHeader.SetAuthorValue (1, new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetOrganizationValue (1, new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetOriginatingSystem(new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));

        if (aWriter.Write((const Standard_CString)filename) != IFSelect_RetDone)
            throw Base::Exception("Writing of STEP failed");
        pi->EndScope();
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) aFail = Standard_Failure::Caught();
        throw Base::Exception(aFail->GetMessageString());
    }
}

void TopoShape::exportBrep(const char *filename) const
{
    if (!BRepTools::Write(this->_Shape,(const Standard_CString)filename))
        throw Base::Exception("Writing of BREP failed");
}

void TopoShape::exportBrep(std::ostream& out)
{
    BRepTools::Write(this->_Shape, out);
}

void TopoShape::exportStl(const char *filename) const
{
    StlAPI_Writer writer;
    //writer.RelativeMode() = false;
    //writer.SetDeflection(0.1);
    writer.Write(this->_Shape,(const Standard_CString)filename);
}

void TopoShape::exportFaceSet(double dev, double ca, std::ostream& str) const
{
    Base::InventorBuilder builder(str);
    TopExp_Explorer ex;

    BRepMesh_IncrementalMesh MESH(this->_Shape,dev);
    for (ex.Init(this->_Shape, TopAbs_FACE); ex.More(); ex.Next()) {
        // get the shape and mesh it
        const TopoDS_Face& aFace = TopoDS::Face(ex.Current());
        Standard_Integer nbNodesInFace,nbTriInFace;
        std::vector<Base::Vector3f> vertices;
        std::vector<int> indices;

        // doing the meshing and checking the result
        TopLoc_Location aLoc;
        Handle(Poly_Triangulation) aPoly = BRep_Tool::Triangulation(aFace,aLoc);
        if (aPoly.IsNull()) continue;

        // getting the transformation of the shape/face
        gp_Trsf myTransf;
        Standard_Boolean identity = true;
        if (!aLoc.IsIdentity()) {
            identity = false;
            myTransf = aLoc.Transformation();
        }

        // getting size and create the array
        nbNodesInFace = aPoly->NbNodes();
        nbTriInFace = aPoly->NbTriangles();
        vertices.resize(nbNodesInFace);
        indices.resize(4*nbTriInFace);

        // check orientation
        TopAbs_Orientation orient = aFace.Orientation();

        // cycling through the poly mesh
        const Poly_Array1OfTriangle& Triangles = aPoly->Triangles();
        const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();
        for (int i=1;i<=nbTriInFace;i++) {
            // Get the triangle
            Standard_Integer N1,N2,N3;
            Triangles(i).Get(N1,N2,N3);

            // change orientation of the triangles
            if (orient != TopAbs_FORWARD) {
                Standard_Integer tmp = N1;
                N1 = N2;
                N2 = tmp;
            }

            gp_Pnt V1 = Nodes(N1);
            gp_Pnt V2 = Nodes(N2);
            gp_Pnt V3 = Nodes(N3);

            // transform the vertices to the place of the face
            if (!identity) {
                V1.Transform(myTransf);
                V2.Transform(myTransf);
                V3.Transform(myTransf);
            }

            vertices[N1-1].Set((float)(V1.X()),(float)(V1.Y()),(float)(V1.Z()));
            vertices[N2-1].Set((float)(V2.X()),(float)(V2.Y()),(float)(V2.Z()));
            vertices[N3-1].Set((float)(V3.X()),(float)(V3.Y()),(float)(V3.Z()));

            int j = i - 1;
            N1--; N2--; N3--;
            indices[4*j] = N1; indices[4*j+1] = N2; indices[4*j+2] = N3; indices[4*j+3] = -1;
        }

        builder.addIndexedFaceSet(vertices, indices, (float)ca);
    } // end of face loop
}

void TopoShape::exportLineSet(std::ostream& str) const
{
    Base::InventorBuilder builder(str);
    // get a indexed map of edges
    TopTools_IndexedMapOfShape M;
    TopExp::MapShapes(this->_Shape, TopAbs_EDGE, M);

    // build up map edge->face
    TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
    TopExp::MapShapesAndAncestors(this->_Shape, TopAbs_EDGE, TopAbs_FACE, edge2Face);
    for (int i=0; i<M.Extent(); i++)
    {
        const TopoDS_Edge& aEdge = TopoDS::Edge(M(i+1));
        gp_Trsf myTransf;
        TopLoc_Location aLoc;

        // try to triangulate the edge
        Handle(Poly_Polygon3D) aPoly = BRep_Tool::Polygon3D(aEdge, aLoc);

        std::vector<Base::Vector3f> vertices;
        Standard_Integer nbNodesInFace;

        // triangulation succeeded?
        if (!aPoly.IsNull()) {
            if (!aLoc.IsIdentity()) {
                myTransf = aLoc.Transformation();
            }
            nbNodesInFace = aPoly->NbNodes();
            vertices.resize(nbNodesInFace);

            const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();

            gp_Pnt V;
            for (Standard_Integer i=0;i < nbNodesInFace;i++) {
                V = Nodes(i+1);
                V.Transform(myTransf);
                vertices[i].Set((float)(V.X()),(float)(V.Y()),(float)(V.Z()));
            }
        }
        else {
            // the edge has not its own triangulation, but then a face the edge is attached to
            // must provide this triangulation

            // Look for one face in our map (it doesn't care which one we take)
            const TopoDS_Face& aFace = TopoDS::Face(edge2Face.FindFromKey(aEdge).First());

            // take the face's triangulation instead
            Handle(Poly_Triangulation) aPolyTria = BRep_Tool::Triangulation(aFace,aLoc);
            if (!aLoc.IsIdentity()) {
                myTransf = aLoc.Transformation();
            }

            if (aPolyTria.IsNull()) break;

            // this holds the indices of the edge's triangulation to the actual points
            Handle(Poly_PolygonOnTriangulation) aPoly = BRep_Tool::PolygonOnTriangulation(aEdge, aPolyTria, aLoc);
            if (aPoly.IsNull())
                continue; // polygon does not exist

            // getting size and create the array
            nbNodesInFace = aPoly->NbNodes();
            vertices.resize(nbNodesInFace);

            const TColStd_Array1OfInteger& indices = aPoly->Nodes();
            const TColgp_Array1OfPnt& Nodes = aPolyTria->Nodes();

            gp_Pnt V;
            int pos = 0;
            // go through the index array
            for (Standard_Integer i=indices.Lower();i <= indices.Upper();i++) {
                V = Nodes(indices(i));
                V.Transform(myTransf);
                vertices[pos++].Set((float)(V.X()),(float)(V.Y()),(float)(V.Z()));
            }
        }

        builder.addLineSet(vertices, 2, 0, 0, 0);
    }
}

Base::BoundBox3d TopoShape::getBoundBox(void) const
{
    Base::BoundBox3d box;
    try {
        // If the shape is empty an exception may be thrown
        Bnd_Box bounds;
        BRepBndLib::Add(_Shape, bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        box.MinX = xMin;
        box.MaxX = xMax;
        box.MinY = yMin;
        box.MaxY = yMax;
        box.MinZ = zMin;
        box.MaxZ = zMax;
    }
    catch (Standard_Failure) {
    }

    return box;
}

void TopoShape::Save (Base::Writer & writer) const
{
}

void TopoShape::Restore(Base::XMLReader &reader)
{
}

void TopoShape::SaveDocFile (Base::Writer &writer) const
{
}

void TopoShape::RestoreDocFile(Base::Reader &reader)
{
}

unsigned int TopoShape_RefCountShapes(const TopoDS_Shape& aShape)
{
    unsigned int size = 1; // this shape
    TopoDS_Iterator it;
    // go through all direct children
    for (it.Initialize(aShape, false, false);it.More(); it.Next()) {
        size += TopoShape_RefCountShapes(it.Value());
    }

    return size;
}

unsigned int TopoShape::getMemSize (void) const
{
    if (!_Shape.IsNull()) {
        // Count total amount of references of TopoDS_Shape objects
        unsigned int memsize = (sizeof(TopoDS_Shape)+sizeof(TopoDS_TShape)) * TopoShape_RefCountShapes(_Shape);

        // Now get a map of TopoDS_Shape objects without duplicates
        TopTools_IndexedMapOfShape M;
        TopExp::MapShapes(_Shape, M);
        for (int i=0; i<M.Extent(); i++) {
            const TopoDS_Shape& shape = M(i+1);
            // add the size of the underlying geomtric data
            Handle(TopoDS_TShape) tshape = shape.TShape();
            memsize += tshape->DynamicType()->Size();

            switch (shape.ShapeType())
            {
            case TopAbs_FACE:
                {
                    // first, last, tolerance
                    memsize += 5*sizeof(Standard_Real);
                    const TopoDS_Face& face = TopoDS::Face(shape);
                    BRepAdaptor_Surface surface(face);
                    switch (surface.GetType())
                    {
                    case GeomAbs_Plane:
                        memsize += sizeof(Geom_Plane);
                        break;
                    case GeomAbs_Cylinder:
                        memsize += sizeof(Geom_CylindricalSurface);
                        break;
                    case GeomAbs_Cone:
                        memsize += sizeof(Geom_ConicalSurface);
                        break;
                    case GeomAbs_Sphere:
                        memsize += sizeof(Geom_SphericalSurface);
                        break;
                    case GeomAbs_Torus:
                        memsize += sizeof(Geom_ToroidalSurface);
                        break;
                    case GeomAbs_BezierSurface:
                        memsize += sizeof(Geom_BezierSurface);
                        memsize += (surface.NbUKnots()+surface.NbVKnots()) * sizeof(Standard_Real);
                        memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_BSplineSurface:
                        memsize += sizeof(Geom_BSplineSurface);
                        memsize += (surface.NbUKnots()+surface.NbVKnots()) * sizeof(Standard_Real);
                        memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_SurfaceOfRevolution:
                        memsize += sizeof(Geom_SurfaceOfRevolution);
                        break;
                    case GeomAbs_SurfaceOfExtrusion:
                        memsize += sizeof(Geom_SurfaceOfLinearExtrusion);
                        break;
                    case GeomAbs_OtherSurface:
                        // What kind of surface should this be?
                        memsize += sizeof(Geom_Surface);
                        break;
                    default:
                        break;
                    }
                } break;
            case TopAbs_EDGE:
                {
                    // first, last, tolerance
                    memsize += 3*sizeof(Standard_Real);
                    const TopoDS_Edge& edge = TopoDS::Edge(shape);
                    BRepAdaptor_Curve curve(edge);
                    switch (curve.GetType())
                    {
                    case GeomAbs_Line:
                        memsize += sizeof(Geom_Line);
                        break;
                    case GeomAbs_Circle:
                        memsize += sizeof(Geom_Circle);
                        break;
                    case GeomAbs_Ellipse:
                        memsize += sizeof(Geom_Ellipse);
                        break;
                    case GeomAbs_Hyperbola:
                        memsize += sizeof(Geom_Hyperbola);
                        break;
                    case GeomAbs_Parabola:
                        memsize += sizeof(Geom_Parabola);
                        break;
                    case GeomAbs_BezierCurve:
                        memsize += sizeof(Geom_BezierCurve);
                        memsize += curve.NbKnots() * sizeof(Standard_Real);
                        memsize += curve.NbPoles() * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_BSplineCurve:
                        memsize += sizeof(Geom_BSplineCurve);
                        memsize += curve.NbKnots() * sizeof(Standard_Real);
                        memsize += curve.NbPoles() * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_OtherCurve:
                        // What kind of curve should this be?
                        memsize += sizeof(Geom_Curve);
                        break;
                    default:
                        break;
                    }
                } break;
            case TopAbs_VERTEX:
                {
                    // tolerance
                    memsize += sizeof(Standard_Real);
                    memsize += sizeof(Geom_CartesianPoint);
                } break;
            default:
                break;
            }
        }

        // estimated memory usage
        return memsize;
    }

    // in case the shape is invalid
    return sizeof(TopoDS_Shape);
}

bool TopoShape::isNull() const
{
    return this->_Shape.IsNull() ? true : false;
}

bool TopoShape::isValid() const
{
    BRepCheck_Analyzer aChecker(this->_Shape);
    return aChecker.IsValid() ? true : false;
}

bool TopoShape::analyze(std::ostream& str) const
{
    if (!this->_Shape.IsNull()) {
        BRepCheck_Analyzer aChecker(this->_Shape);
        if (!aChecker.IsValid()) {
            std::vector<TopoDS_Shape> shapes;

            TopTools_IndexedMapOfShape vertexOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_VERTEX, vertexOfShape);
            for (int i = 1; i <= vertexOfShape.Extent();++i)
                shapes.push_back(vertexOfShape(i));

            TopTools_IndexedMapOfShape edgeOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_EDGE, edgeOfShape);
            for (int i = 1; i <= edgeOfShape.Extent();++i)
                shapes.push_back(edgeOfShape(i));

            TopTools_IndexedMapOfShape wireOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_WIRE, wireOfShape);
            for (int i = 1; i <= wireOfShape.Extent();++i)
                shapes.push_back(wireOfShape(i));

            TopTools_IndexedMapOfShape faceOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_FACE, faceOfShape);
            for (int i = 1; i <= faceOfShape.Extent();++i)
                shapes.push_back(faceOfShape(i));

            TopTools_IndexedMapOfShape shellOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_SHELL, shellOfShape);
            for (int i = 1; i <= shellOfShape.Extent();++i)
                shapes.push_back(shellOfShape(i));

            TopTools_IndexedMapOfShape solidOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_SOLID, solidOfShape);
            for (int i = 1; i <= solidOfShape.Extent();++i)
                shapes.push_back(solidOfShape(i));

            TopTools_IndexedMapOfShape compOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_COMPOUND, compOfShape);
            for (int i = 1; i <= compOfShape.Extent();++i)
                shapes.push_back(compOfShape(i));

            TopTools_IndexedMapOfShape compsOfShape;
            TopExp::MapShapes(this->_Shape, TopAbs_COMPSOLID, compsOfShape);
            for (int i = 1; i <= compsOfShape.Extent();++i)
                shapes.push_back(compsOfShape(i));

            for (std::vector<TopoDS_Shape>::iterator xp = shapes.begin(); xp != shapes.end(); ++xp) {
                if (!aChecker.IsValid(*xp)) {
                    const Handle_BRepCheck_Result& result = aChecker.Result(*xp);
                    if (result.IsNull())
                        continue;
                    const BRepCheck_ListOfStatus& status = result->StatusOnShape(*xp);

                    BRepCheck_ListIteratorOfListOfStatus it(status);
                    while (it.More()) {
                        BRepCheck_Status& val = it.Value();
                        switch (val)
                        {
                        case BRepCheck_NoError:
                            str << "No error" << std::endl;
                            break;
                        case BRepCheck_InvalidPointOnCurve:
                            str << "Invalid point on curve" << std::endl;
                            break;
                        case BRepCheck_InvalidPointOnCurveOnSurface:
                            str << "Invalid point on curve on surface" << std::endl;
                            break;
                        case BRepCheck_InvalidPointOnSurface:
                            str << "Invalid point on surface" << std::endl;
                            break;
                        case BRepCheck_No3DCurve:
                            str << "No 3D curve" << std::endl;
                            break;
                        case BRepCheck_Multiple3DCurve:
                            str << "Multiple 3D curve" << std::endl;
                            break;
                        case BRepCheck_Invalid3DCurve:
                            str << "Invalid 3D curve" << std::endl;
                            break;
                        case BRepCheck_NoCurveOnSurface:
                            str << "No curve on surface" << std::endl;
                            break;
                        case BRepCheck_InvalidCurveOnSurface:
                            str << "Invalid curve on surface" << std::endl;
                            break;
                        case BRepCheck_InvalidCurveOnClosedSurface:
                            str << "Invalid curve on closed surface" << std::endl;
                            break;
                        case BRepCheck_InvalidSameRangeFlag:
                            str << "Invalid same-range flag" << std::endl;
                            break;
                        case BRepCheck_InvalidSameParameterFlag:
                            str << "Invalid same-parameter flag" << std::endl;
                            break;
                        case BRepCheck_InvalidDegeneratedFlag:
                            str << "Invalid degenerated flag" << std::endl;
                            break;
                        case BRepCheck_FreeEdge:
                            str << "Free edge" << std::endl;
                            break;
                        case BRepCheck_InvalidMultiConnexity:
                            str << "Invalid multi-connexity" << std::endl;
                            break;
                        case BRepCheck_InvalidRange:
                            str << "Invalid range" << std::endl;
                            break;
                        case BRepCheck_EmptyWire:
                            str << "Empty wire" << std::endl;
                            break;
                        case BRepCheck_RedundantEdge:
                            str << "Redundant edge" << std::endl;
                            break;
                        case BRepCheck_SelfIntersectingWire:
                            str << "Self-intersecting wire" << std::endl;
                            break;
                        case BRepCheck_NoSurface:
                            str << "No surface" << std::endl;
                            break;
                        case BRepCheck_InvalidWire:
                            str << "Invalid wires" << std::endl;
                            break;
                        case BRepCheck_RedundantWire:
                            str << "Redundant wires" << std::endl;
                            break;
                        case BRepCheck_IntersectingWires:
                            str << "Intersecting wires" << std::endl;
                            break;
                        case BRepCheck_InvalidImbricationOfWires:
                            str << "Invalid imbrication of wires" << std::endl;
                            break;
                        case BRepCheck_EmptyShell:
                            str << "Empty shell" << std::endl;
                            break;
                        case BRepCheck_RedundantFace:
                            str << "Redundant face" << std::endl;
                            break;
                        case BRepCheck_UnorientableShape:
                            str << "Unorientable shape" << std::endl;
                            break;
                        case BRepCheck_NotClosed:
                            str << "Not closed" << std::endl;
                            break;
                        case BRepCheck_NotConnected:
                            str << "Not connected" << std::endl;
                            break;
                        case BRepCheck_SubshapeNotInShape:
                            str << "Sub-shape not in shape" << std::endl;
                            break;
                        case BRepCheck_BadOrientation:
                            str << "Bad orientation" << std::endl;
                            break;
                        case BRepCheck_BadOrientationOfSubshape:
                            str << "Bad orientation of sub-shape" << std::endl;
                            break;
                        case BRepCheck_InvalidToleranceValue:
                            str << "Invalid tolerance value" << std::endl;
                            break;
                        case BRepCheck_CheckFail:
                            str << "Check failed" << std::endl;
                            break;
                        default:
                            str << "Undetermined error" << std::endl;
                            break;
                        }

                        it.Next();
                    }
                }
            }

            return false; // errors detected
        }
    }

    return true;
}

bool TopoShape::isClosed() const
{
    return BRep_Tool::IsClosed(this->_Shape) ? true : false;
}

TopoDS_Shape TopoShape::cut(TopoDS_Shape shape) const
{
    BRepAlgoAPI_Cut mkCut(this->_Shape, shape);
    return mkCut.Shape();
}

TopoDS_Shape TopoShape::common(TopoDS_Shape shape) const
{
    BRepAlgoAPI_Common mkCommon(this->_Shape, shape);
    return mkCommon.Shape();
}

TopoDS_Shape TopoShape::fuse(TopoDS_Shape shape) const
{
    BRepAlgoAPI_Fuse mkFuse(this->_Shape, shape);
    return mkFuse.Shape();
}

TopoDS_Shape TopoShape::oldFuse(TopoDS_Shape shape) const
{
    BRepAlgo_Fuse mkFuse(this->_Shape, shape);
    return mkFuse.Shape();
}

TopoDS_Shape TopoShape::section(TopoDS_Shape shape) const
{
    BRepAlgoAPI_Section mkSection(this->_Shape, shape);
    return mkSection.Shape();
}

std::list<TopoDS_Wire> TopoShape::slice(const Base::Vector3d& dir, double d) const
{
    CrossSection cs(dir.x, dir.y, dir.z, this->_Shape);
    return cs.slice(d);
}

TopoDS_Compound TopoShape::slices(const Base::Vector3d& dir, const std::vector<double>& d) const
{
    std::vector< std::list<TopoDS_Wire> > wire_list;
    CrossSection cs(dir.x, dir.y, dir.z, this->_Shape);
    for (std::vector<double>::const_iterator jt = d.begin(); jt != d.end(); ++jt) {
        wire_list.push_back(cs.slice(*jt));
    }

    std::vector< std::list<TopoDS_Wire> >::const_iterator ft;
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);

    for (ft = wire_list.begin(); ft != wire_list.end(); ++ft) {
        const std::list<TopoDS_Wire>& w = *ft;
        for (std::list<TopoDS_Wire>::const_iterator wt = w.begin(); wt != w.end(); ++wt) {
            if (!wt->IsNull())
                builder.Add(comp, *wt);
        }
    }

    return comp;
}

TopoDS_Shape TopoShape::makePipe(const TopoDS_Shape& profile) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");
    if (this->_Shape.ShapeType() != TopAbs_WIRE)
        Standard_Failure::Raise("Spine shape is not a wire");
    if (profile.IsNull())
        Standard_Failure::Raise("Cannot sweep empty profile");
    BRepOffsetAPI_MakePipe mkPipe(TopoDS::Wire(this->_Shape), profile);
    return mkPipe.Shape();
}

TopoDS_Shape TopoShape::makePipeShell(const TopTools_ListOfShape& profiles,
                                      const Standard_Boolean make_solid,
                                      const Standard_Boolean isFrenet,
                                      int transition) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");
    if (this->_Shape.ShapeType() != TopAbs_WIRE)
        Standard_Failure::Raise("Spine shape is not a wire");

    BRepOffsetAPI_MakePipeShell mkPipeShell(TopoDS::Wire(this->_Shape));
    BRepBuilderAPI_TransitionMode transMode;
    switch (transition) {
        case 1: transMode = BRepBuilderAPI_RightCorner;
            break;
        case 2: transMode = BRepBuilderAPI_RoundCorner;
            break;
        default: transMode = BRepBuilderAPI_Transformed;
            break;
    }
    mkPipeShell.SetMode(isFrenet);
    mkPipeShell.SetTransitionMode(transMode);
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(profiles); it.More(); it.Next()) {
        mkPipeShell.Add(TopoDS_Shape(it.Value()));
    }

    if (!mkPipeShell.IsReady()) Standard_Failure::Raise("shape is not ready to build");
    else mkPipeShell.Build();

    if (make_solid)	mkPipeShell.MakeSolid();

    return mkPipeShell.Shape();
}

#if 0
TopoDS_Shape TopoShape::makeTube() const
{
    // http://opencascade.blogspot.com/2009/11/surface-modeling-part3.html
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");
    if (this->_Shape.ShapeType() != TopAbs_EDGE)
        Standard_Failure::Raise("Spine shape is not an edge");

    const TopoDS_Edge& path_edge = TopoDS::Edge(this->_Shape);
    BRepAdaptor_Curve path_adapt(path_edge);
    double umin = path_adapt.FirstParameter();
    double umax = path_adapt.LastParameter();
    Handle_Geom_Curve hPath = path_adapt.Curve().Curve();

    // Apply placement of the shape to the curve
    TopLoc_Location loc1 = path_edge.Location();
    hPath = Handle_Geom_Curve::DownCast(hPath->Transformed(loc1.Transformation()));

    if (hPath.IsNull())
        Standard_Failure::Raise("Invalid curve in path edge");

    GeomFill_Pipe mkTube(hPath, radius);
    mkTube.Perform(tol, Standard_False, GeomAbs_C1, BSplCLib::MaxDegree(), 1000);

    const Handle_Geom_Surface& surf = mkTube.Surface();
    double u1,u2,v1,v2;
    surf->Bounds(u1,u2,v1,v2);

    BRepBuilderAPI_MakeFace mkBuilder(surf, umin, umax, v1, v2
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );
    return mkBuilder.Face();
}
#else 
static Handle(Law_Function) CreateBsFunction (const Standard_Real theFirst, const Standard_Real theLast, const Standard_Real theRadius)
{
    //Handle_Law_BSpline aBs;
    //Handle_Law_BSpFunc aFunc = new Law_BSpFunc (aBs, theFirst, theLast);
    Handle_Law_Linear aFunc = new Law_Linear();
    aFunc->Set(theFirst, theRadius, theLast, theRadius);
    return aFunc;
}

TopoDS_Shape TopoShape::makeTube(double radius, double tol, int cont, int maxdegree, int maxsegm) const
{
    // http://opencascade.blogspot.com/2009/11/surface-modeling-part3.html
    Standard_Real theTol = tol;
    //Standard_Boolean theIsPolynomial = Standard_True;
    Standard_Boolean myIsElem = Standard_True;
    GeomAbs_Shape theContinuity = GeomAbs_Shape(cont);
    Standard_Integer theMaxDegree = maxdegree;
    Standard_Integer theMaxSegment = maxsegm;

    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");

    Handle(Adaptor3d_HCurve) myPath;
    if (this->_Shape.ShapeType() == TopAbs_EDGE) {
        const TopoDS_Edge& path_edge = TopoDS::Edge(this->_Shape);
        BRepAdaptor_Curve path_adapt(path_edge);
        myPath = new BRepAdaptor_HCurve(path_adapt);
    }
    //else if (this->_Shape.ShapeType() == TopAbs_WIRE) {
    //    const TopoDS_Wire& path_wire = TopoDS::Wire(this->_Shape);
    //    BRepAdaptor_CompCurve path_adapt(path_wire);
    //    myPath = new BRepAdaptor_HCompCurve(path_adapt);
    //}
    //else {
    //    Standard_Failure::Raise("Spine shape is neither an edge nor a wire");
    //}
    else {
        Standard_Failure::Raise("Spine shape is not an edge");
    }

    //circular profile
    Handle(Geom_Circle) aCirc = new Geom_Circle (gp::XOY(), radius);
    aCirc->Rotate (gp::OZ(), M_PI/2.);

    //perpendicular section
    Handle(Law_Function) myEvol = ::CreateBsFunction (myPath->FirstParameter(), myPath->LastParameter(), radius);
    Handle(GeomFill_SectionLaw) aSec = new GeomFill_EvolvedSection(aCirc, myEvol);
    Handle(GeomFill_LocationLaw) aLoc = new GeomFill_CurveAndTrihedron(new GeomFill_CorrectedFrenet);
    aLoc->SetCurve (myPath);

    GeomFill_Sweep mkSweep (aLoc, myIsElem);
    mkSweep.SetTolerance (theTol);
    mkSweep.Build (aSec, GeomFill_Location, theContinuity, theMaxDegree, theMaxSegment);
    if (mkSweep.IsDone()) {
        Handle_Geom_Surface mySurface = mkSweep.Surface();
        //Standard_Real myError = mkSweep.ErrorOnSurface();

        Standard_Real u1,u2,v1,v2;
        mySurface->Bounds(u1,u2,v1,v2);
        BRepBuilderAPI_MakeFace mkBuilder(mySurface, u1, u2, v1, v2
#if OCC_VERSION_HEX >= 0x060502
          , Precision::Confusion()
#endif
        );
        return mkBuilder.Shape();
    }

    return TopoDS_Shape();
}
#endif

TopoDS_Shape TopoShape::makeSweep(const TopoDS_Shape& profile, double tol, int fillMode) const
{
    // http://opencascade.blogspot.com/2009/10/surface-modeling-part2.html
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");
    if (this->_Shape.ShapeType() != TopAbs_EDGE)
        Standard_Failure::Raise("Spine shape is not an edge");

    if (profile.IsNull())
        Standard_Failure::Raise("Cannot sweep with empty profile");
    if (profile.ShapeType() != TopAbs_EDGE)
        Standard_Failure::Raise("Profile shape is not an edge");

    const TopoDS_Edge& path_edge = TopoDS::Edge(this->_Shape);
    const TopoDS_Edge& prof_edge = TopoDS::Edge(profile);

    BRepAdaptor_Curve path_adapt(path_edge);
    double umin = path_adapt.FirstParameter();
    double umax = path_adapt.LastParameter();
    Handle_Geom_Curve hPath = path_adapt.Curve().Curve();

    // Apply placement of the shape to the curve
    TopLoc_Location loc1 = path_edge.Location();
    hPath = Handle_Geom_Curve::DownCast(hPath->Transformed(loc1.Transformation()));

    if (hPath.IsNull())
        Standard_Failure::Raise("invalid curve in path edge");

    BRepAdaptor_Curve prof_adapt(prof_edge);
    double vmin = prof_adapt.FirstParameter();
    double vmax = prof_adapt.LastParameter();
    Handle_Geom_Curve hProfile = prof_adapt.Curve().Curve();

    // Apply placement of the shape to the curve
    TopLoc_Location loc2 = prof_edge.Location();
    hProfile = Handle_Geom_Curve::DownCast(hProfile->Transformed(loc2.Transformation()));

    if (hProfile.IsNull())
        Standard_Failure::Raise("invalid curve in profile edge");

    GeomFill_Pipe mkSweep(hPath, hProfile, (GeomFill_Trihedron)fillMode);
    mkSweep.GenerateParticularCase(Standard_True);
    mkSweep.Perform(tol, Standard_False, GeomAbs_C1, BSplCLib::MaxDegree(), 1000);

    const Handle_Geom_Surface& surf = mkSweep.Surface();
    BRepBuilderAPI_MakeFace mkBuilder(surf, umin, umax, vmin, vmax
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );
    return mkBuilder.Face();
}

TopoDS_Shape TopoShape::makeHelix(Standard_Real pitch, Standard_Real height,
                                  Standard_Real radius, Standard_Real angle,
                                  Standard_Boolean leftHanded,
                                  Standard_Boolean newStyle) const
{
    if (pitch < Precision::Confusion())
        Standard_Failure::Raise("Pitch of helix too small");

    if (height < Precision::Confusion())
        Standard_Failure::Raise("Height of helix too small");

    gp_Ax2 cylAx2(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
    Handle_Geom_Surface surf;
    if (angle < Precision::Confusion()) {
        if (radius < Precision::Confusion())
            Standard_Failure::Raise("Radius of helix too small");
        surf = new Geom_CylindricalSurface(cylAx2, radius);
    }
    else {
        angle = Base::toRadians(angle);
        if (angle < Precision::Confusion())
            Standard_Failure::Raise("Angle of helix too small");
        surf = new Geom_ConicalSurface(gp_Ax3(cylAx2), angle, radius);
    }

    gp_Pnt2d aPnt(0, 0);
    gp_Dir2d aDir(2. * M_PI, pitch);
    if (leftHanded) {
        //aPnt.SetCoord(0.0, height);
        //aDir.SetCoord(2.0 * PI, -pitch);
        aPnt.SetCoord(2. * M_PI, 0.0);
        aDir.SetCoord(-2. * M_PI, pitch);
    }
    gp_Ax2d aAx2d(aPnt, aDir);

    Handle(Geom2d_Line) line = new Geom2d_Line(aAx2d);
    gp_Pnt2d beg = line->Value(0);
    gp_Pnt2d end = line->Value(sqrt(4.0*M_PI*M_PI+pitch*pitch)*(height/pitch));

    if (newStyle) {
        // See discussion at 0001247: Part Conical Helix Height/Pitch Incorrect
        if (angle >= Precision::Confusion()) {
            // calculate end point for conical helix
            Standard_Real v = height / cos(angle);
            Standard_Real u = (height/pitch) * 2.0 * M_PI;
            gp_Pnt2d cend(u, v);
            end = cend;
        }
    }

    Handle(Geom2d_TrimmedCurve) segm = GCE2d_MakeSegment(beg , end);

    TopoDS_Edge edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edgeOnSurf);
    BRepLib::BuildCurves3d(wire);
    return wire;
}

TopoDS_Shape TopoShape::makeThread(Standard_Real pitch,
                                   Standard_Real depth,
                                   Standard_Real height,
                                   Standard_Real radius) const
{
    if (pitch < Precision::Confusion())
        Standard_Failure::Raise("Pitch of thread too small");

    if (depth < Precision::Confusion())
        Standard_Failure::Raise("Depth of thread too small");

    if (height < Precision::Confusion())
        Standard_Failure::Raise("Height of thread too small");

    if (radius < Precision::Confusion())
        Standard_Failure::Raise("Radius of thread too small");

    //Threading : Create Surfaces
    gp_Ax2 cylAx2(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
    Handle(Geom_CylindricalSurface) aCyl1 = new Geom_CylindricalSurface(cylAx2 , radius);
    Handle(Geom_CylindricalSurface) aCyl2 = new Geom_CylindricalSurface(cylAx2 , radius+depth);

    //Threading : Define 2D Curves
    gp_Pnt2d aPnt(2. * M_PI , height / 2.);
    gp_Dir2d aDir(2. * M_PI , height / 4.);
    gp_Ax2d aAx2d(aPnt , aDir);

    Standard_Real aMajor = 2. * M_PI;
    Standard_Real aMinor = pitch;

    Handle(Geom2d_Ellipse) anEllipse1 = new Geom2d_Ellipse(aAx2d , aMajor , aMinor);
    Handle(Geom2d_Ellipse) anEllipse2 = new Geom2d_Ellipse(aAx2d , aMajor , aMinor / 4);

    Handle(Geom2d_TrimmedCurve) aArc1 = new Geom2d_TrimmedCurve(anEllipse1 , 0 , M_PI);
    Handle(Geom2d_TrimmedCurve) aArc2 = new Geom2d_TrimmedCurve(anEllipse2 , 0 , M_PI);

    gp_Pnt2d anEllipsePnt1 = anEllipse1->Value(0);
    gp_Pnt2d anEllipsePnt2 = anEllipse1->Value(M_PI);

    Handle(Geom2d_TrimmedCurve) aSegment = GCE2d_MakeSegment(anEllipsePnt1 , anEllipsePnt2);

    //Threading : Build Edges and Wires
    TopoDS_Edge aEdge1OnSurf1 = BRepBuilderAPI_MakeEdge(aArc1 , aCyl1);
    TopoDS_Edge aEdge2OnSurf1 = BRepBuilderAPI_MakeEdge(aSegment , aCyl1);
    TopoDS_Edge aEdge1OnSurf2 = BRepBuilderAPI_MakeEdge(aArc2 , aCyl2);
    TopoDS_Edge aEdge2OnSurf2 = BRepBuilderAPI_MakeEdge(aSegment , aCyl2);

    TopoDS_Wire threadingWire1 = BRepBuilderAPI_MakeWire(aEdge1OnSurf1 , aEdge2OnSurf1);
    TopoDS_Wire threadingWire2 = BRepBuilderAPI_MakeWire(aEdge1OnSurf2 , aEdge2OnSurf2);

    BRepLib::BuildCurves3d(threadingWire1);
    BRepLib::BuildCurves3d(threadingWire2);

    BRepOffsetAPI_ThruSections aTool(Standard_True);

    aTool.AddWire(threadingWire1);
    aTool.AddWire(threadingWire2);
    aTool.CheckCompatibility(Standard_False);

    return aTool.Shape();
}

TopoDS_Shape TopoShape::makeLoft(const TopTools_ListOfShape& profiles, 
                                 Standard_Boolean isSolid,
                                 Standard_Boolean isRuled) const
{
    // http://opencascade.blogspot.com/2010/01/surface-modeling-part5.html
    BRepOffsetAPI_ThruSections aGenerator (isSolid,isRuled);

    int countShapes = 0;
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(profiles); it.More(); it.Next()) {
        const TopoDS_Shape& item = it.Value();
        if (!item.IsNull() && item.ShapeType() == TopAbs_VERTEX) {
            aGenerator.AddVertex(TopoDS::Vertex (item));
            countShapes++;
        }
        else if (!item.IsNull() && item.ShapeType() == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(item));
            aGenerator.AddWire(mkWire.Wire());
            countShapes++;
        }
        else if (!item.IsNull() && item.ShapeType() == TopAbs_WIRE) {
            aGenerator.AddWire(TopoDS::Wire (item));
            countShapes++;
        }
    }

    if (countShapes < 2)
        Standard_Failure::Raise("Need at least two vertices, edges or wires to create loft face");

    Standard_Boolean anIsCheck = Standard_True;
    aGenerator.CheckCompatibility (anIsCheck);
    aGenerator.Build();
    if (!aGenerator.IsDone())
        Standard_Failure::Raise("Failed to create loft face");

    return aGenerator.Shape();
}

TopoDS_Shape TopoShape::makePrism(const gp_Vec& vec) const
{
    if (this->_Shape.IsNull()) Standard_Failure::Raise("cannot sweep empty shape");
    BRepPrimAPI_MakePrism mkPrism(this->_Shape, vec);
    return mkPrism.Shape();
}

TopoDS_Shape TopoShape::revolve(const gp_Ax1& axis, double d) const
{
    if (this->_Shape.IsNull()) Standard_Failure::Raise("cannot sweep empty shape");
    BRepPrimAPI_MakeRevol mkRevol(this->_Shape, axis,d);
    return mkRevol.Shape();
}

//#include <BRepFill.hxx>
//#include <BRepTools_Quilt.hxx>

TopoDS_Shape TopoShape::makeOffsetShape(double offset, double tol, bool intersection,
                                        bool selfInter, short offsetMode, short join,
                                        bool fill) const
{
    BRepOffsetAPI_MakeOffsetShape mkOffset(this->_Shape, offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
    const TopoDS_Shape& res = mkOffset.Shape();
    if (!fill)
        return res;
#if 1
    //s=Part.makePlane(10,10)
    //s.makeOffsetShape(1.0,0.01,False,False,0,0,True)
    const BRepOffset_MakeOffset& off = mkOffset.MakeOffset();
    const BRepAlgo_Image& img = off.OffsetEdgesFromShapes();

    // build up map edge->face
    TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
    TopExp::MapShapesAndAncestors(this->_Shape, TopAbs_EDGE, TopAbs_FACE, edge2Face);
    TopTools_IndexedMapOfShape mapOfShape;
    TopExp::MapShapes(this->_Shape, TopAbs_EDGE, mapOfShape);

    TopoDS_Shell shell;
    BRep_Builder builder;
    TopExp_Explorer xp;
    builder.MakeShell(shell);

    for (xp.Init(this->_Shape,TopAbs_FACE); xp.More(); xp.Next()) {
        builder.Add(shell, xp.Current());
    } 

    for (int i=1; i<= edge2Face.Extent(); ++i) {
        const TopTools_ListOfShape& los = edge2Face.FindFromIndex(i);
        if (los.Extent() == 1) {
            // set the index value as user data to use it in accept()
            const TopoDS_Shape& edge = edge2Face.FindKey(i);
            Standard_Boolean ok = img.HasImage(edge);
            if (ok) {
                const TopTools_ListOfShape& edges = img.Image(edge);
                TopTools_ListIteratorOfListOfShape it;
                it.Initialize(edges);
                BRepOffsetAPI_ThruSections aGenerator (0,0);
                aGenerator.AddWire(BRepBuilderAPI_MakeWire(TopoDS::Edge(edge)).Wire());
                aGenerator.AddWire(BRepBuilderAPI_MakeWire(TopoDS::Edge(it.Value())).Wire());
                aGenerator.Build();
                for (xp.Init(aGenerator.Shape(),TopAbs_FACE); xp.More(); xp.Next()) {
                    builder.Add(shell, xp.Current());
                }
                //TopoDS_Face face = BRepFill::Face(TopoDS::Edge(edge), TopoDS::Edge(it.Value()));
                //builder.Add(shell, face);
            }
        }
    }

    for (xp.Init(mkOffset.Shape(),TopAbs_FACE); xp.More(); xp.Next()) {
        builder.Add(shell, xp.Current());
    } 

    //BRepBuilderAPI_Sewing sew(offset);
    //sew.Load(this->_Shape);
    //sew.Add(mkOffset.Shape());
    //sew.Perform();

    //shell.Closed(Standard_True);

    return shell;
#else
    TopoDS_Solid    Res;
    TopExp_Explorer exp;
    BRep_Builder    B;
    B.MakeSolid(Res);

    BRepTools_Quilt Glue;
    for (exp.Init(this->_Shape,TopAbs_FACE); exp.More(); exp.Next()) {
      Glue.Add (exp.Current());
    } 
    for (exp.Init(mkOffset.Shape(),TopAbs_FACE);exp.More(); exp.Next()) {
      Glue.Add (exp.Current().Reversed());
    }
    TopoDS_Shape S = Glue.Shells();
    for (exp.Init(S,TopAbs_SHELL); exp.More(); exp.Next()) {
      B.Add(Res,exp.Current());
    }
    Res.Closed(Standard_True);
    return Res;
#endif
}

TopoDS_Shape TopoShape::makeThickSolid(const TopTools_ListOfShape& remFace,
                                       double offset, double tol, bool intersection,
                                       bool selfInter, short offsetMode, short join) const
{
    BRepOffsetAPI_MakeThickSolid mkThick(this->_Shape, remFace, offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
    return mkThick.Shape();
}

void TopoShape::transformGeometry(const Base::Matrix4D &rclMat)
{
    this->_Shape = transformGShape(rclMat);
}

TopoDS_Shape TopoShape::transformGShape(const Base::Matrix4D& rclTrf) const
{
    gp_GTrsf mat;
    mat.SetValue(1,1,rclTrf[0][0]);
    mat.SetValue(2,1,rclTrf[1][0]);
    mat.SetValue(3,1,rclTrf[2][0]);
    mat.SetValue(1,2,rclTrf[0][1]);
    mat.SetValue(2,2,rclTrf[1][1]);
    mat.SetValue(3,2,rclTrf[2][1]);
    mat.SetValue(1,3,rclTrf[0][2]);
    mat.SetValue(2,3,rclTrf[1][2]);
    mat.SetValue(3,3,rclTrf[2][2]);
    mat.SetValue(1,4,rclTrf[0][3]);
    mat.SetValue(2,4,rclTrf[1][3]);
    mat.SetValue(3,4,rclTrf[2][3]);

    // geometric transformation
    BRepBuilderAPI_GTransform mkTrf(this->_Shape, mat);
    return mkTrf.Shape();
}

void TopoShape::transformShape(const Base::Matrix4D& rclTrf, bool copy)
{
    gp_Trsf mat;
    mat.SetValues(rclTrf[0][0],rclTrf[0][1],rclTrf[0][2],rclTrf[0][3],
                  rclTrf[1][0],rclTrf[1][1],rclTrf[1][2],rclTrf[1][3],
                  rclTrf[2][0],rclTrf[2][1],rclTrf[2][2],rclTrf[2][3],
                  0.00001,0.00001);

    // location transformation
    BRepBuilderAPI_Transform mkTrf(this->_Shape, mat, copy ? Standard_True : Standard_False);
    this->_Shape = mkTrf.Shape();
}

TopoDS_Shape TopoShape::mirror(const gp_Ax2& ax2) const
{
    gp_Trsf mat;
    mat.SetMirror(ax2);
    TopLoc_Location loc = this->_Shape.Location();
    gp_Trsf placement = loc.Transformation();
    BRepBuilderAPI_Transform mkTrf(this->_Shape, mat);
    return mkTrf.Shape();
}

TopoDS_Shape TopoShape::toNurbs() const
{
    BRepBuilderAPI_NurbsConvert mkNurbs(this->_Shape);
    return mkNurbs.Shape();
}

TopoDS_Shape TopoShape::replaceShape(const std::vector< std::pair<TopoDS_Shape,TopoDS_Shape> >& s) const
{
    BRepTools_ReShape reshape;
    std::vector< std::pair<TopoDS_Shape,TopoDS_Shape> >::const_iterator it;
    for (it = s.begin(); it != s.end(); ++it)
        reshape.Replace(it->first, it->second);
    return reshape.Apply(this->_Shape, TopAbs_SHAPE);
}

TopoDS_Shape TopoShape::removeShape(const std::vector<TopoDS_Shape>& s) const
{
    BRepTools_ReShape reshape;
    for (std::vector<TopoDS_Shape>::const_iterator it = s.begin(); it != s.end(); ++it)
        reshape.Remove(*it);
    return reshape.Apply(this->_Shape, TopAbs_SHAPE);
}

void TopoShape::sewShape()
{
    BRepBuilderAPI_Sewing sew;
    sew.Load(this->_Shape);
    sew.Perform();

    this->_Shape = sew.SewedShape();
}

bool TopoShape::fix(double precision, double mintol, double maxtol)
{
    if (this->_Shape.IsNull())
        return false;

    TopAbs_ShapeEnum type = this->_Shape.ShapeType();

    ShapeFix_Shape fix(this->_Shape);
    fix.SetPrecision(precision);
    fix.SetMinTolerance(mintol);
    fix.SetMaxTolerance(maxtol);

    fix.Perform();

    if (type == TopAbs_SOLID) {
        //fix.FixEdgeTool();
        fix.FixWireTool()->Perform();
        fix.FixFaceTool()->Perform();
        fix.FixShellTool()->Perform();
        fix.FixSolidTool()->Perform();
        this->_Shape = fix.FixSolidTool()->Shape();
    }
    else if (type == TopAbs_SHELL) {
        fix.FixWireTool()->Perform();
        fix.FixFaceTool()->Perform();
        fix.FixShellTool()->Perform();
        this->_Shape = fix.FixShellTool()->Shape();
    }
    else if (type == TopAbs_FACE) {
        fix.FixWireTool()->Perform();
        fix.FixFaceTool()->Perform();
        this->_Shape = fix.Shape();
    }
    else if (type == TopAbs_WIRE) {
        fix.FixWireTool()->Perform();
        this->_Shape = fix.Shape();
    }
    else {
        this->_Shape = fix.Shape();
    }

    return isValid();
}

bool TopoShape::removeInternalWires(double minArea)
{
    ShapeUpgrade_RemoveInternalWires fix(this->_Shape);
    fix.MinArea() = minArea;
    bool ok = fix.Perform() ? true : false;
    this->_Shape = fix.GetResult();
    return ok;
}

TopoDS_Shape TopoShape::removeSplitter() const
{
    if (_Shape.IsNull())
        Standard_Failure::Raise("Cannot remove splitter from empty shape");

    if (_Shape.ShapeType() == TopAbs_SOLID) {
        const TopoDS_Solid &solid = TopoDS::Solid(_Shape);
        BRepBuilderAPI_MakeSolid mkSolid;
        TopExp_Explorer it;
        for (it.Init(solid, TopAbs_SHELL); it.More(); it.Next()) {
            const TopoDS_Shell &currentShell = TopoDS::Shell(it.Current());
            ModelRefine::FaceUniter uniter(currentShell);
            if (uniter.process()) {
                if (uniter.isModified()) {
                    const TopoDS_Shell &newShell = uniter.getShell();
                    mkSolid.Add(newShell);
                }
                else {
                    mkSolid.Add(currentShell);
                }
            }
            else {
                Standard_Failure::Raise("Removing splitter failed");
                return _Shape;
            }
        }
        return mkSolid.Solid();
    }
    else if (_Shape.ShapeType() == TopAbs_SHELL) {
        const TopoDS_Shell& shell = TopoDS::Shell(_Shape);
        ModelRefine::FaceUniter uniter(shell);
        if (uniter.process()) {
            return uniter.getShell();
        }
        else {
            Standard_Failure::Raise("Removing splitter failed");
        }
    }
    else if (_Shape.ShapeType() == TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);

        TopExp_Explorer xp;
        // solids
        for (xp.Init(_Shape, TopAbs_SOLID); xp.More(); xp.Next()) {
            const TopoDS_Solid &solid = TopoDS::Solid(xp.Current());
            BRepTools_ReShape reshape;
            TopExp_Explorer it;
            for (it.Init(solid, TopAbs_SHELL); it.More(); it.Next()) {
                const TopoDS_Shell &currentShell = TopoDS::Shell(it.Current());
                ModelRefine::FaceUniter uniter(currentShell);
                if (uniter.process()) {
                    if (uniter.isModified()) {
                        const TopoDS_Shell &newShell = uniter.getShell();
                        reshape.Replace(currentShell, newShell);
                    }
                }
            }
            builder.Add(comp, reshape.Apply(solid));
        }
        // free shells
        for (xp.Init(_Shape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next()) {
            const TopoDS_Shell& shell = TopoDS::Shell(xp.Current());
            ModelRefine::FaceUniter uniter(shell);
            if (uniter.process()) {
                builder.Add(comp, uniter.getShell());
            }
        }
        // the rest
        for (xp.Init(_Shape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(_Shape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(_Shape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }
        for (xp.Init(_Shape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
            if (!xp.Current().IsNull())
                builder.Add(comp, xp.Current());
        }

        return comp;
    }

    return _Shape;
}

namespace Part {
struct MeshVertex
{
    Standard_Real x,y,z;
    Standard_Integer i;

    MeshVertex(Standard_Real X, Standard_Real Y, Standard_Real Z)
        : x(X),y(Y),z(Z)
    {
    }
    MeshVertex(const gp_Pnt& p)
        : x(p.X()),y(p.Y()),z(p.Z())
    {
    }

    gp_Pnt toPoint() const
    { return gp_Pnt(x,y,z); }

    bool operator < (const MeshVertex &rclPt) const
    {
        if (fabs ( this->x - rclPt.x) >= MESH_MIN_PT_DIST)
            return this->x < rclPt.x;
        if (fabs ( this->y - rclPt.y) >= MESH_MIN_PT_DIST)
            return this->y < rclPt.y;
        if (fabs ( this->z - rclPt.z) >= MESH_MIN_PT_DIST)
            return this->z < rclPt.z;
        return false; // points are considered to be equal
    }

private:
    // use the same value as used inside the Mesh module
    static const double MESH_MIN_PT_DIST;
};
}

//const double Vertex::MESH_MIN_PT_DIST = 1.0e-6;
const double MeshVertex::MESH_MIN_PT_DIST = gp::Resolution();

#include <StlTransfer.hxx>
#include <StlMesh_Mesh.hxx>
#include <StlMesh_MeshExplorer.hxx>

void TopoShape::getFaces(std::vector<Base::Vector3d> &aPoints,
                         std::vector<Facet> &aTopo,
                         float accuracy, uint16_t flags) const
{
#if 1
    if (this->_Shape.IsNull())
        return;
    std::set<MeshVertex> vertices;
    Standard_Real x1, y1, z1;
    Standard_Real x2, y2, z2;
    Standard_Real x3, y3, z3;

    Handle_StlMesh_Mesh aMesh = new StlMesh_Mesh();
    StlTransfer::BuildIncrementalMesh(this->_Shape, accuracy,
#if OCC_VERSION_HEX >= 0x060503
        Standard_True,
#endif
        aMesh);
    StlMesh_MeshExplorer xp(aMesh);
    for (Standard_Integer nbd=1;nbd<=aMesh->NbDomains();nbd++) {
        for (xp.InitTriangle (nbd); xp.MoreTriangle (); xp.NextTriangle ()) {
            xp.TriangleVertices (x1,y1,z1,x2,y2,z2,x3,y3,z3);
            Data::ComplexGeoData::Facet face;
            std::set<MeshVertex>::iterator it;

            // 1st vertex
            MeshVertex v1(x1,y1,z1);
            it = vertices.find(v1);
            if (it == vertices.end()) {
                v1.i = vertices.size();
                face.I1 = v1.i;
                vertices.insert(v1);
            }
            else {
                face.I1 = it->i;
            }

            // 2nd vertex
            MeshVertex v2(x2,y2,z2);
            it = vertices.find(v2);
            if (it == vertices.end()) {
                v2.i = vertices.size();
                face.I2 = v2.i;
                vertices.insert(v2);
            }
            else {
                face.I2 = it->i;
            }

            // 3rd vertex
            MeshVertex v3(x3,y3,z3);
            it = vertices.find(v3);
            if (it == vertices.end()) {
                v3.i = vertices.size();
                face.I3 = v3.i;
                vertices.insert(v3);
            }
            else {
                face.I3 = it->i;
            }

            // make sure that we don't insert invalid facets
            if (face.I1 != face.I2 &&
                face.I2 != face.I3 &&
                face.I3 != face.I1)
                aTopo.push_back(face);
        }
    }

    std::vector<gp_Pnt> points;
    points.resize(vertices.size());
    for (std::set<MeshVertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
        points[it->i] = it->toPoint();
    for (std::vector<gp_Pnt>::iterator it = points.begin(); it != points.end(); ++it)
        aPoints.push_back(Base::Vector3d(it->X(),it->Y(),it->Z()));
#endif
#if 0
    BRepMesh::Mesh (this->_Shape, accuracy);
    std::set<MeshVertex> vertices;
    for (TopExp_Explorer xp(this->_Shape,TopAbs_FACE); xp.More(); xp.Next()) {
        TopoDS_Face face = TopoDS::Face(xp.Current());
        TopAbs_Orientation orient = face.Orientation();
        // change orientation of the triangles
        Standard_Boolean reversed = false;
        if (orient != TopAbs_FORWARD) {
            reversed = true;
        }
        TopLoc_Location aLoc;
        Handle(Poly_Triangulation) aPoly = BRep_Tool::Triangulation(face, aLoc);
        if (aPoly.IsNull()) continue;

        // getting the transformation of the shape/face
        gp_Trsf myTransf;
        Standard_Boolean identity = true;
        if(!aLoc.IsIdentity())  {
            identity = false;
            myTransf = aLoc.Transformation();
        }

        // cycling through the poly mesh
        const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();
        for (Standard_Integer i=1;i<=Nodes.Length();i++) {
            Standard_Real X1, Y1, Z1;
            gp_Pnt p = Nodes.Value(i);
            p.Transform(myTransf);
            p.Coord (X1, Y1, Z1);
        }

        const Poly_Array1OfTriangle& Triangles = aPoly->Triangles();
        try {
            for (Standard_Integer i=1;i<=Triangles.Length();i++) {
                Standard_Integer V1, V2, V3;
                Poly_Triangle triangle = Triangles.Value(i);
                triangle.Get(V1, V2, V3);
                if (reversed)
                    std::swap(V1,V2);
                gp_Pnt P1, P2, P3;
                Data::ComplexGeoData::Facet face;
                std::set<MeshVertex>::iterator it;

                // 1st vertex
                P1 = Nodes(V1);
                P1.Transform(myTransf);
                MeshVertex v1(P1);
                it = vertices.find(v1);
                if (it == vertices.end()) {
                    v1.i = vertices.size();
                    face.I1 = v1.i;
                    vertices.insert(v1);
                }
                else {
                    face.I1 = it->i;
                }

                // 2nd vertex
                P2 = Nodes(V2);
                P2.Transform(myTransf);
                MeshVertex v2(P2);
                it = vertices.find(v2);
                if (it == vertices.end()) {
                    v2.i = vertices.size();
                    face.I2 = v2.i;
                    vertices.insert(v2);
                }
                else {
                    face.I2 = it->i;
                }
                
                // 3rd vertex
                P3 = Nodes(V3);
                P3.Transform(myTransf);
                MeshVertex v3(P3);
                it = vertices.find(v3);
                if (it == vertices.end()) {
                    v3.i = vertices.size();
                    face.I3 = v3.i;
                    vertices.insert(v3);
                }
                else {
                    face.I3 = it->i;
                }

                // make sure that we don't insert invalid facets
                if (face.I1 != face.I2 &&
                    face.I2 != face.I3 &&
                    face.I3 != face.I1)
                    aTopo.push_back(face);
            }
        }
        catch(Standard_Failure) {
        }
    }

    std::map<Standard_Integer,gp_Pnt> points;
    for (std::set<MeshVertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
        points[it->i] = it->toPoint();
    for (std::map<Standard_Integer,gp_Pnt>::iterator it = points.begin(); it != points.end(); ++it)
        aPoints.push_back(Base::Vector3d(it->second.X(),it->second.Y(),it->second.Z()));
#endif
}

void TopoShape::setFaces(const std::vector<Base::Vector3d> &Points,
                         const std::vector<Facet> &Topo, float Accuracy)
{
    gp_XYZ p1, p2, p3;
    TopoDS_Vertex Vertex1, Vertex2, Vertex3;
    TopoDS_Face newFace;
    TopoDS_Wire newWire;
    BRepBuilderAPI_Sewing aSewingTool;
    Standard_Real x1, y1, z1;
    Standard_Real x2, y2, z2;
    Standard_Real x3, y3, z3;

    aSewingTool.Init(Accuracy,Standard_True);

    TopoDS_Compound aComp;
    BRep_Builder BuildTool;
    BuildTool.MakeCompound(aComp);

    unsigned int ctPoints = Points.size();
    for (std::vector<Facet>::const_iterator it = Topo.begin(); it != Topo.end(); ++it) {
        if (it->I1 >= ctPoints || it->I2 >= ctPoints || it->I3 >= ctPoints)
            continue;
        x1 = Points[it->I1].x; y1 = Points[it->I1].y; z1 = Points[it->I1].z;
        x2 = Points[it->I2].x; y2 = Points[it->I2].y; z2 = Points[it->I2].z;
        x3 = Points[it->I3].x; y3 = Points[it->I3].y; z3 = Points[it->I3].z;

        p1.SetCoord(x1,y1,z1);
        p2.SetCoord(x2,y2,z2);
        p3.SetCoord(x3,y3,z3);

        if ((!(p1.IsEqual(p2,0.0))) && (!(p1.IsEqual(p3,0.0)))) {
            Vertex1 = BRepBuilderAPI_MakeVertex(p1);
            Vertex2 = BRepBuilderAPI_MakeVertex(p2);
            Vertex3 = BRepBuilderAPI_MakeVertex(p3);

            newWire = BRepBuilderAPI_MakePolygon(Vertex1, Vertex2, Vertex3, Standard_True);
            if (!newWire.IsNull()) {
                newFace = BRepBuilderAPI_MakeFace(newWire);
                if (!newFace.IsNull())
                    BuildTool.Add(aComp, newFace);
            }
        }
    }

    aSewingTool.Load(aComp);
    aSewingTool.Perform();
    _Shape = aSewingTool.SewedShape();
    // TopAbs_Orientation o = _Shape.Orientation();
    _Shape.Reverse(); // seems that we have to reverse the orientation
    if (_Shape.IsNull())
        _Shape = aComp;
}
