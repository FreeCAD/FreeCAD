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
# include <array>
# include <cmath>
# include <cstdlib>
# include <sstream>
# include <boost/regex.hpp>

# include <APIHeaderSection_MakeHeader.hxx>
# include <BinTools.hxx>
# include <BinTools_ShapeSet.hxx>
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAdaptor_CompCurve.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_FaceError.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_NurbsConvert.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepFill_CompatibleWires.hxx>
# include <BRepGProp.hxx>
# include <BRepGProp_Face.hxx>
# include <BRepLib.hxx>
# include <BRepLib_FindSurface.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepOffsetAPI_MakeOffsetShape.hxx>
# include <BRepOffsetAPI_MakePipe.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepOffsetAPI_MakeThickSolid.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ReShape.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <BSplCLib.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <GCPnts_UniformAbscissa.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Geom_Circle.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Line.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Plane.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Geom2d_Ellipse.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <GeomFill_CorrectedFrenet.hxx>
# include <GeomFill_CurveAndTrihedron.hxx>
# include <GeomFill_EvolvedSection.hxx>
# include <GeomFill_Pipe.hxx>
# include <GeomFill_SectionLaw.hxx>
# include <GeomFill_Sweep.hxx>
# include <GeomLib.hxx>
# include <GeomLib_IsPlanarSurface.hxx>
# include <gp_Circ.hxx>
# include <gp_Pln.hxx>
# include <GProp_GProps.hxx>
# include <IGESControl_Controller.hxx>
# include <IGESControl_Reader.hxx>
# include <IGESControl_Writer.hxx>
# include <IGESData_GlobalSection.hxx>
# include <IGESData_IGESModel.hxx>
# include <Interface_Static.hxx>
# include <Law_BSpline.hxx>
# include <Law_BSpFunc.hxx>
# include <Law_Constant.hxx>
# include <ShapeAnalysis_FreeBoundsProperties.hxx>
# include <ShapeExtend_Explorer.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeUpgrade_RemoveInternalWires.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <STEPControl_Reader.hxx>
# include <STEPControl_Writer.hxx>
# include <StlAPI_Writer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <Transfer_FinderProcess.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_TransferWriter.hxx>
# include <XSControl_WorkSession.hxx>


# include <BOPAlgo_ArgumentAnalyzer.hxx>
# include <BOPAlgo_ListOfCheckResult.hxx>

# include <BRepAlgoAPI_Defeaturing.hxx>

#if OCC_VERSION_HEX < 0x070600
# include <BRepAdaptor_HCurve.hxx>
# include <BRepAdaptor_HCompCurve.hxx>
#endif

# include <boost/algorithm/string/predicate.hpp>
# include <boost/core/ignore_unused.hpp>
#endif // _PreComp_

#include <App/Material.h>
#include <App/ElementNamingUtils.h>
#include <Base/BoundBox.h>
#include <Base/Builder3D.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "TopoShape.h"
#include "BRepOffsetAPI_MakeOffsetFix.h"
#include "CrossSection.h"
#include "encodeFilename.h"
#include "FaceMakerBullseye.h"
#include "Interface.h"
#include "modelRefine.h"
#include "PartPyCXX.h"
#include "ProgressIndicator.h"
#include "Tools.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeWirePy.h"


FC_LOG_LEVEL_INIT("TopoShape",true,true)

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
    default:
        return "Unknown creation error";
    }
}

/**
 * Derive a roughly proportional default angular deflection from a linear
 * tolerance. This is a work-around for unreliable linear tolerance enforcement
 * in OCC, especially on nurbs surfaces. The intention is to provide sane
 * baseline (default) behavior with linear tolerances only, until OCC is fixed.
 * If needed for specific use cases, explicit angular deflection parameters can
 * still be exposed separately.
 */
inline double defaultAngularDeflection(double linearTolerance) {
    // Default OCC angular deflection is 0.5 radians, or about 28.6 degrees.
    // That is a bit coarser than necessary for performance, so we default to at
    // most 0.1 radians, or 5.7 degrees. We also do not go finer than 0.005, or
    // roughly 0.28 degree angular resolution, to avoid performance tanking
    // completely at very fine resolutions.
    return std::min(0.1, linearTolerance * 5 + 0.005);
}

// ------------------------------------------------

NullShapeException::NullShapeException()
  : ValueError()
{
}

NullShapeException::NullShapeException(const char * sMessage)
  : ValueError(sMessage)
{
}

NullShapeException::NullShapeException(const std::string& sMessage)
  : ValueError(sMessage)
{
}

// ------------------------------------------------

BooleanException::BooleanException()
  : CADKernelError()
{
}

BooleanException::BooleanException(const char * sMessage)
  : CADKernelError(sMessage)
{
}

BooleanException::BooleanException(const std::string& sMessage)
  : CADKernelError(sMessage)
{
}

// ------------------------------------------------

TYPESYSTEM_SOURCE(Part::ShapeSegment , Data::Segment)

std::string ShapeSegment::getName() const
{
    return {};
}

// ------------------------------------------------

TYPESYSTEM_SOURCE(Part::TopoShape , Data::ComplexGeoData)

TopoShape::TopoShape() = default;

TopoShape::~TopoShape() = default;

TopoShape::TopoShape(const TopoDS_Shape& shape)
  : _Shape(shape)
{
}

TopoShape::TopoShape(const TopoShape& shape)
  : _Shape(shape._Shape)
{
    Tag = shape.Tag;
}

std::pair<std::string, unsigned long> TopoShape::getElementTypeAndIndex(const char* Name)
{
    int index = 0;
    std::string element;
    boost::regex ex("^(Face|Edge|Vertex)([1-9][0-9]*)$");
    boost::cmatch what;

    if (Name && boost::regex_match(Name, what, ex)) {
        element = what[1].str();
        index = std::atoi(what[2].str().c_str());
    }

    return std::make_pair(element, index);
}

std::vector<const char*> TopoShape::getElementTypes() const
{
    static const std::vector<const char*> temp = {"Face","Edge","Vertex"};
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

TopoDS_Shape TopoShape::getSubShape(const char* Type, bool silent) const
{
    auto res = shapeTypeAndIndex(Type);
    return getSubShape(res.first,res.second,silent);
}

TopoDS_Shape TopoShape::getSubShape(TopAbs_ShapeEnum type, int index, bool silent) const
{
    if(index <= 0) {
        if(silent)
            return {};
        Standard_Failure::Raise("Unsupported sub-shape type");
    }

    if (this->_Shape.IsNull()) {
        if(silent)
            return {};
        Standard_Failure::Raise("Cannot get sub-shape from empty shape");
    }

    try {
        if(type == TopAbs_SHAPE) {
            int i=1;
            for(TopoDS_Iterator it(_Shape);it.More();it.Next(),++i) {
                if(i == index)
                    return it.Value();
            }
        } else {
            TopTools_IndexedMapOfShape anIndices;
            TopExp::MapShapes(this->_Shape, type, anIndices);
            if(index <= anIndices.Extent())
                return anIndices.FindKey(index);
        }
    } catch(Standard_Failure &) {
        if(silent)
            return {};
        throw;
    }
    if(!silent)
        Standard_Failure::Raise("Index out of bound");
    return {};
}

unsigned long TopoShape::countSubShapes(const char* Type) const
{
    if(!Type)
        return 0;
    if(strcmp(Type,"SubShape")==0)
        return countSubShapes(TopAbs_SHAPE);
    auto type = shapeType(Type,true);
    if(type == TopAbs_SHAPE)
        return 0;
    return countSubShapes(type);
}

unsigned long TopoShape::countSubShapes(TopAbs_ShapeEnum Type) const
{
    if(Type == TopAbs_SHAPE) {
        int count = 0;
        for(TopoDS_Iterator it(_Shape);it.More();it.Next())
            ++count;
        return count;
    }
    TopTools_IndexedMapOfShape anIndices;
    TopExp::MapShapes(this->_Shape, Type, anIndices);
    return anIndices.Extent();
}

bool TopoShape::hasSubShape(TopAbs_ShapeEnum type) const {
    if(type == TopAbs_SHAPE) {
        TopoDS_Iterator it(_Shape);
        return !!it.More();
    }
    TopExp_Explorer exp(_Shape,type);
    return !!exp.More();
}

bool TopoShape::hasSubShape(const char *Type) const {
    auto idx = shapeTypeAndIndex(Type);
    return idx.second>0 && idx.second<=(int)countSubShapes(idx.first);
}

template<class T>
static inline std::vector<T> _getSubShapes(const TopoDS_Shape &s, TopAbs_ShapeEnum type) {
    std::vector<T> shapes;
    if(s.IsNull())
        return shapes;

    if(type == TopAbs_SHAPE) {
        for(TopoDS_Iterator it(s);it.More();it.Next())
            shapes.emplace_back(it.Value());
        return shapes;
    }

    TopTools_IndexedMapOfShape anIndices;
    TopExp::MapShapes(s, type, anIndices);
    int count = anIndices.Extent();
    shapes.reserve(count);
    for(int i=1;i<=count;++i)
        shapes.emplace_back(anIndices.FindKey(i));
    return shapes;
}

std::vector<TopoShape> TopoShape::getSubTopoShapes(TopAbs_ShapeEnum type) const {
    return _getSubShapes<TopoShape>(_Shape,type);
}

std::vector<TopoDS_Shape> TopoShape::getSubShapes(TopAbs_ShapeEnum type) const {
    return _getSubShapes<TopoDS_Shape>(_Shape,type);
}

static std::array<std::string,TopAbs_SHAPE> _ShapeNames;

static void initShapeNameMap() {
    if(_ShapeNames[TopAbs_VERTEX].empty()) {
        _ShapeNames[TopAbs_VERTEX] = "Vertex";
        _ShapeNames[TopAbs_EDGE] = "Edge";
        _ShapeNames[TopAbs_FACE] = "Face";
        _ShapeNames[TopAbs_WIRE] = "Wire";
        _ShapeNames[TopAbs_SHELL] = "Shell";
        _ShapeNames[TopAbs_SOLID] = "Solid";
        _ShapeNames[TopAbs_COMPOUND] = "Compound";
        _ShapeNames[TopAbs_COMPSOLID] = "CompSolid";
    }
}

std::pair<TopAbs_ShapeEnum,int> TopoShape::shapeTypeAndIndex(const char *name) {
    int idx = 0;
    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    static const std::string _subshape("SubShape");
    if(boost::starts_with(name,_subshape)) {
        std::istringstream iss(name+_subshape.size());
        iss >> idx;
        if(!iss.eof())
            idx = 0;
    } else {
        type = shapeType(name,true);
        if(type != TopAbs_SHAPE) {
            std::istringstream iss(name+shapeName(type).size());
            iss >> idx;
            if(!iss.eof()) {
                idx = 0;
                type = TopAbs_SHAPE;
            }
        }
    }
    return std::make_pair(type,idx);
}

TopAbs_ShapeEnum TopoShape::shapeType(const char *type, bool silent) {
    if(type) {
        initShapeNameMap();
        for(size_t idx=0;idx<_ShapeNames.size();++idx) {
            if(!_ShapeNames[idx].empty() && boost::starts_with(type,_ShapeNames[idx]))
                return static_cast<TopAbs_ShapeEnum>(idx);
        }
    }
    if(!silent) {
        if(Data::hasMissingElement(type))
            FC_THROWM(Base::CADKernelError,"missing shape element: " << (type?type:"?"));
        FC_THROWM(Base::CADKernelError,"invalid shape type: " << (type?type:"?"));
    }
    return TopAbs_SHAPE;
}

TopAbs_ShapeEnum TopoShape::shapeType(char type, bool silent) {
    switch(type) {
    case 'E':
        return TopAbs_EDGE;
    case 'V':
        return TopAbs_VERTEX;
    case 'F':
        return TopAbs_FACE;
    default:
        if(!silent)
            FC_THROWM(Base::CADKernelError, "invalid shape type '" << type << "'");
        return TopAbs_SHAPE;
    }
}

TopAbs_ShapeEnum TopoShape::shapeType(bool silent) const {
    if(isNull()) {
        if(!silent)
            FC_THROWM(NullShapeException, "Input shape is null");
        return TopAbs_SHAPE;
    }
    return getShape().ShapeType();
}

const std::string &TopoShape::shapeName(TopAbs_ShapeEnum type, bool silent) {
    initShapeNameMap();
    if(type>=0 && type<_ShapeNames.size() && !_ShapeNames[type].empty())
        return _ShapeNames[type];
    if(!silent)
        FC_THROWM(Base::CADKernelError, "invalid shape type '" << type << "'");
    static std::string ret;
    return ret;
}

const std::string &TopoShape::shapeName(bool silent) const {
    return shapeName(shapeType(silent),silent);
}

PyObject * TopoShape::getPySubShape(const char* Type, bool silent) const
{
    return Py::new_reference_to(shape2pyshape(getSubShape(Type,silent)));
}

PyObject * TopoShape::getPyObject()
{
    Base::PyObjectBase* prop = nullptr;
    if (_Shape.IsNull()) {
        prop = new TopoShapePy(new TopoShape(_Shape));
    }
    else {
        TopAbs_ShapeEnum type = _Shape.ShapeType();
        switch (type)
        {
        case TopAbs_COMPOUND:
            prop = new TopoShapeCompoundPy(new TopoShape(_Shape));
            break;
        case TopAbs_COMPSOLID:
            prop = new TopoShapeCompSolidPy(new TopoShape(_Shape));
            break;
        case TopAbs_SOLID:
            prop = new TopoShapeSolidPy(new TopoShape(_Shape));
            break;
        case TopAbs_SHELL:
            prop = new TopoShapeShellPy(new TopoShape(_Shape));
            break;
        case TopAbs_FACE:
            prop = new TopoShapeFacePy(new TopoShape(_Shape));
            break;
        case TopAbs_WIRE:
            prop = new TopoShapeWirePy(new TopoShape(_Shape));
            break;
        case TopAbs_EDGE:
            prop = new TopoShapeEdgePy(new TopoShape(_Shape));
            break;
        case TopAbs_VERTEX:
            prop = new TopoShapeVertexPy(new TopoShape(_Shape));
            break;
        case TopAbs_SHAPE:
        default:
            prop = new TopoShapePy(new TopoShape(_Shape));
            break;
        }
    }

    prop->setNotTracking();
    return prop;
}

void TopoShape::setPyObject(PyObject* obj)
{
    if (PyObject_TypeCheck(obj, &TopoShapePy::Type)) {
        this->_Shape = static_cast<TopoShapePy*>(obj)->getTopoShapePtr()->getShape();
    }
    else {
        std::string error = std::string("type must be 'Shape', not ");
        error += obj->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void TopoShape::operator = (const TopoShape& sh)
{
    if (this != &sh) {
        this->Tag = sh.Tag;
        this->_Shape = sh._Shape;
    }
}

void TopoShape::convertTogpTrsf(const Base::Matrix4D& mtrx, gp_Trsf& trsf)
{
    trsf.SetValues(mtrx[0][0],mtrx[0][1],mtrx[0][2],mtrx[0][3],
                   mtrx[1][0],mtrx[1][1],mtrx[1][2],mtrx[1][3],
                   mtrx[2][0],mtrx[2][1],mtrx[2][2],mtrx[2][3]);
}

void TopoShape::convertToMatrix(const gp_Trsf& trsf, Base::Matrix4D& mtrx)
{
    // https://www.opencascade.com/doc/occt-7.0.0/refman/html/classgp___trsf.html
    // VectorialPart() already includes the scale factor
    gp_Mat m = trsf.VectorialPart();
    gp_XYZ p = trsf.TranslationPart();

    // set Rotation matrix
    mtrx[0][0] = m(1,1);
    mtrx[0][1] = m(1,2);
    mtrx[0][2] = m(1,3);

    mtrx[1][0] = m(2,1);
    mtrx[1][1] = m(2,2);
    mtrx[1][2] = m(2,3);

    mtrx[2][0] = m(3,1);
    mtrx[2][1] = m(3,2);
    mtrx[2][2] = m(3,3);

    // set pos vector
    mtrx[0][3] = p.X();
    mtrx[1][3] = p.Y();
    mtrx[2][3] = p.Z();
}

Base::Matrix4D TopoShape::convert(const gp_Trsf& trsf) {
    Base::Matrix4D mat;
    convertToMatrix(trsf,mat);
    return mat;
}

gp_Trsf TopoShape::convert(const Base::Matrix4D& mtrx) {
    gp_Trsf trsf;
    convertTogpTrsf(mtrx,trsf);
    return trsf;
}

void TopoShape::setTransform(const Base::Matrix4D& rclTrf)
{
    gp_Trsf mov;
    convertTogpTrsf(rclTrf, mov);
    TopLoc_Location loc(mov);
    _Shape.Location(loc);
}

Base::Matrix4D TopoShape::getTransform() const
{
    Base::Matrix4D mtrx;
    gp_Trsf Trf = _Shape.Location().Transformation();
    convertToMatrix(Trf, mtrx);
    return mtrx;
}

/*!
 * \obsolete
 */
void TopoShape::setShapePlacement(const Base::Placement& rclTrf)
{
    const Base::Vector3d& pos = rclTrf.getPosition();
    Base::Vector3d axis;
    double angle;
    rclTrf.getRotation().getValue(axis, angle);

    gp_Trsf trsf;
    trsf.SetRotation(gp_Ax1(gp_Pnt(0.,0.,0.), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trsf.SetTranslationPart(gp_Vec(pos.x, pos.y, pos.z));
    TopLoc_Location loc(trsf);
    _Shape.Location(loc);
}

/*!
 * \obsolete
 */
Base::Placement TopoShape::getShapePlacement() const
{
    TopLoc_Location loc = _Shape.Location();
    gp_Trsf trsf = loc.Transformation();
    gp_XYZ pos = trsf.TranslationPart();

    gp_XYZ axis;
    Standard_Real angle;
    trsf.GetRotation(axis, angle);

    Base::Rotation rot(Base::Vector3d(axis.X(), axis.Y(), axis.Z()), angle);
    Base::Placement placement(Base::Vector3d(pos.X(), pos.Y(), pos.Z()), rot);

    return placement;
}

void TopoShape::read(const char *FileName)
{
    Base::FileInfo File(FileName);

    // checking on the file
    if (!File.isReadable())
        throw Base::FileException("File to load not existing or not readable", FileName);

    if (File.hasExtension({"igs", "iges"})) {
        // read iges file
        importIges(File.filePath().c_str());
    }
    else if (File.hasExtension({"stp", "step"})) {
        importStep(File.filePath().c_str());
    }
    else if (File.hasExtension({"brp", "brep"})) {
        // read brep-file
        importBrep(File.filePath().c_str());
    }
    else{
        throw Base::FileException("Unknown extension");
    }
}

/*!
 Example code to get the labels for each face in an IGES file.
 \code
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <IGESData_IGESModel.hxx>
#include <IGESData_IGESEntity.hxx>

IGESControl_Reader aReader;
...
// Gets the labels of all face items if defined in the IGES file
Handle(XSControl_WorkSession) ws = aReader.WS();
Handle(XSControl_TransferReader) tr = ws->TransferReader();

std::string name;
Handle(IGESData_IGESModel) aModel = aReader.IGESModel();
Standard_Integer all = aModel->NbEntities();

TopExp_Explorer ex;
for (ex.Init(this->_Shape, TopAbs_FACE); ex.More(); ex.Next())
{
    const TopoDS_Face& aFace = TopoDS::Face(ex.Current());
    Handle(Standard_Transient) ent = tr->EntityFromShapeResult(aFace, 1);
    if (!ent.IsNull()) {
        int i = aModel->Number(ent);
        if (i > 0) {
            Handle(IGESData_IGESEntity) ie = aModel->Entity(i);
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
        IGESControl_Controller::Init();
        IGESControl_Reader aReader;
        // Ignore construction elements
        // http://www.opencascade.org/org/forum/thread_20603/?forum=3
        aReader.SetReadVisible(Standard_True);
        if (aReader.ReadFile(encodeFilename(FileName).c_str()) != IFSelect_RetDone)
            throw Base::FileException("Error in reading IGES");

#if OCC_VERSION_HEX < 0x070500
        Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
        pi->NewScope(100, "Reading IGES file...");
        pi->Show();
        aReader.WS()->MapReader()->SetProgress(pi);
#endif

        // make brep
        aReader.ClearShapes();
        aReader.TransferRoots();
        // one shape that contains all subshapes
        this->_Shape = aReader.OneShape();
#if OCC_VERSION_HEX < 0x070500
        pi->EndScope();
#endif
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void TopoShape::importStep(const char *FileName)
{
    try {
        STEPControl_Reader aReader;
        if (aReader.ReadFile(encodeFilename(FileName).c_str()) != IFSelect_RetDone)
            throw Base::FileException("Error in reading STEP");

#if OCC_VERSION_HEX < 0x070500
        Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
        aReader.WS()->MapReader()->SetProgress(pi);
        pi->NewScope(100, "Reading STEP file...");
        pi->Show();
#endif

        // Root transfers
        aReader.TransferRoots();
        // one shape that contains all subshapes
        this->_Shape = aReader.OneShape();
#if OCC_VERSION_HEX < 0x070500
        pi->EndScope();
#endif
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void TopoShape::importBrep(const char *FileName)
{
    try {
        // read brep-file
        BRep_Builder aBuilder;
        TopoDS_Shape aShape;
#if OCC_VERSION_HEX < 0x070500
        Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
        pi->NewScope(100, "Reading BREP file...");
        pi->Show();
        BRepTools::Read(aShape,encodeFilename(FileName).c_str(),aBuilder,pi);
        pi->EndScope();
#else
        BRepTools::Read(aShape,static_cast<Standard_CString>(FileName),aBuilder);
#endif
        this->_Shape = aShape;
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void TopoShape::importBrep(std::istream& str, int indicator)
{
    try {
        // read brep-file
        BRep_Builder aBuilder;
        TopoDS_Shape aShape;
#if OCC_VERSION_HEX < 0x070500
        if (indicator) {
            Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
            pi->NewScope(100, "Reading BREP file...");
            pi->Show();
            BRepTools::Read(aShape,str,aBuilder,pi);
            pi->EndScope();
        }
        else {
            BRepTools::Read(aShape,str,aBuilder);
        }
#else
        (void)indicator;
        BRepTools::Read(aShape,str,aBuilder);
#endif
        this->_Shape = aShape;
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
    catch (const std::exception& e) {
        throw Base::CADKernelError(e.what());
    }
}

void TopoShape::importBinary(std::istream& str)
{
    BinTools_ShapeSet theShapeSet;
    theShapeSet.Read(str);
    Standard_Integer shapeId=0, locId=0, orient=0;
    BinTools::GetInteger(str, shapeId);
    if (shapeId <= 0 || shapeId > theShapeSet.NbShapes())
        return;

    BinTools::GetInteger(str, locId);
    BinTools::GetInteger(str, orient);
    TopAbs_Orientation anOrient = static_cast<TopAbs_Orientation>(orient);

    try {
        this->_Shape = theShapeSet.Shape(shapeId);
        this->_Shape.Location(theShapeSet.Locations().Location (locId));
        this->_Shape.Orientation (anOrient);
    }
    catch (Standard_Failure&) {
        throw Base::RuntimeError("Failed to read shape from binary stream");
    }
}

void TopoShape::write(const char *FileName) const
{
    Base::FileInfo File(FileName);

    if (File.hasExtension({"igs", "iges"})) {
        // write iges file
        exportIges(File.filePath().c_str());
    }
    else if (File.hasExtension({"stp", "step"})) {
        exportStep(File.filePath().c_str());
    }
    else if (File.hasExtension({"brp", "brep"})) {
        // read brep-file
        exportBrep(File.filePath().c_str());
    }
    else if (File.hasExtension("stl")) {
        // read brep-file
        exportStl(File.filePath().c_str(), 0.01);
    }
    else{
        throw Base::FileException("Unknown extension");
    }
}

void TopoShape::exportIges(const char *filename) const
{
    try {
        // write iges file
        IGESControl_Controller::Init();
        IGESControl_Writer aWriter;
        IGESData_GlobalSection header = aWriter.Model()->GlobalSection();
        header.SetAuthorName(new TCollection_HAsciiString(Interface::writeIgesHeaderAuthor()));
        header.SetCompanyName(new TCollection_HAsciiString(Interface::writeIgesHeaderCompany()));
        header.SetSendName(new TCollection_HAsciiString(Interface::writeIgesHeaderProduct()));
        aWriter.Model()->SetGlobalSection(header);
        aWriter.AddShape(this->_Shape);
        aWriter.ComputeModel();
        if (aWriter.Write(encodeFilename(filename).c_str()) != IFSelect_RetDone)
            throw Base::FileException("Writing of IGES failed");
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void TopoShape::exportStep(const char *filename) const
{
    try {
        // Fixes issue #6282
        // Do not write out any assembly information when using the simplified STEP export
        Interface::writeStepAssembly(Interface::Assembly::Off);

        // write step file
        STEPControl_Writer aWriter;

        const Handle(XSControl_TransferWriter)& hTransferWriter = aWriter.WS()->TransferWriter();
        Handle(Transfer_FinderProcess) hFinder = hTransferWriter->FinderProcess();

#if OCC_VERSION_HEX < 0x070500
        Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
        hFinder->SetProgress(pi);
        pi->NewScope(100, "Writing STEP file...");
        pi->Show();
#endif

        if (aWriter.Transfer(this->_Shape, STEPControl_AsIs) != IFSelect_RetDone)
            throw Base::FileException("Error in transferring STEP");

        APIHeaderSection_MakeHeader makeHeader(aWriter.Model());
        // Don't set name because STEP doesn't support UTF-8
        // https://forum.freecad.org/viewtopic.php?f=8&t=52967
        makeHeader.SetAuthorValue (1, new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetOrganizationValue (1, new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetOriginatingSystem(new TCollection_HAsciiString("FreeCAD"));
        makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));

        if (aWriter.Write(encodeFilename(filename).c_str()) != IFSelect_RetDone)
            throw Base::FileException("Writing of STEP failed");
#if OCC_VERSION_HEX < 0x070500
        pi->EndScope();
#endif
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void TopoShape::exportBrep(const char *filename) const
{
#if OCC_VERSION_HEX >= 0x070600
    if (!BRepTools::Write(this->_Shape,encodeFilename(filename).c_str(), Standard_False, Standard_False, TopTools_FormatVersion_VERSION_1))
        throw Base::FileException("Writing of BREP failed");
#else
    if (!BRepTools::Write(this->_Shape,encodeFilename(filename).c_str()))
        throw Base::FileException("Writing of BREP failed");
#endif
}

void TopoShape::exportBrep(std::ostream& out) const
{
    // See TopTools_FormatVersion of OCCT 7.6
    enum {
        VERSION_1 = 1,
        VERSION_2 = 2,
        VERSION_3 = 3
    };
    BRepTools_ShapeSet SS(Standard_False);
    SS.SetFormatNb(VERSION_1);
    SS.Add(this->_Shape);
    SS.Write(out);
    SS.Write(this->_Shape, out);
}

void TopoShape::exportBinary(std::ostream& out) const
{
    // See BinTools_FormatVersion of OCCT 7.6
    enum {
        VERSION_1 = 1,
        VERSION_2 = 2,
        VERSION_3 = 3,
        VERSION_4 = 4
    };

    // An example how to use BinTools_ShapeSet can be found in BinMNaming_NamedShapeDriver.cxx
    BinTools_ShapeSet theShapeSet;
    theShapeSet.SetFormatNb(VERSION_3);
    if (this->_Shape.IsNull()) {
        theShapeSet.Add(this->_Shape);
        theShapeSet.Write(out);
        BinTools::PutInteger(out, -1);
        BinTools::PutInteger(out, -1);
        BinTools::PutInteger(out, -1);
    }
    else {
        Standard_Integer shapeId = theShapeSet.Add(this->_Shape);
        Standard_Integer locId = theShapeSet.Locations().Index(this->_Shape.Location());
        Standard_Integer orient = static_cast<int>(this->_Shape.Orientation());

        theShapeSet.Write(out);
        BinTools::PutInteger(out, shapeId);
        BinTools::PutInteger(out, locId);
        BinTools::PutInteger(out, orient);
    }
}

void TopoShape::dump(std::ostream& out) const
{
    BRepTools::Dump(this->_Shape, out);
}

void TopoShape::exportStl(const char *filename, double deflection) const
{
    StlAPI_Writer writer;
    BRepMesh_IncrementalMesh aMesh(this->_Shape, deflection,
                                   /*isRelative*/ Standard_False,
                                   /*theAngDeflection*/
                                   defaultAngularDeflection(deflection),
                                   /*isInParallel*/ true);
    writer.Write(this->_Shape,encodeFilename(filename).c_str());
}

void TopoShape::exportFaceSet(double dev, double ca,
                              const std::vector<App::Color>& colors,
                              std::ostream& str) const
{
    Base::InventorBuilder builder(str);
    builder.beginSeparator();
    TopExp_Explorer ex;
    std::size_t numFaces = 0;
    for (ex.Init(this->_Shape, TopAbs_FACE); ex.More(); ex.Next()) {
        numFaces++;
    }

    bool supportFaceColors = (numFaces == colors.size());

    std::size_t index=0;
    BRepMesh_IncrementalMesh MESH(this->_Shape, dev,
                                  /*isRelative*/ Standard_False,
                                  /*theAngDeflection*/
                                  defaultAngularDeflection(dev),
                                  /*isInParallel*/ true);
    for (ex.Init(this->_Shape, TopAbs_FACE); ex.More(); ex.Next(), index++) {
        // get the shape and mesh it
        const TopoDS_Face& aFace = TopoDS::Face(ex.Current());
        std::vector<gp_Pnt> points;
        std::vector<Poly_Triangle> facets;
        if (!Tools::getTriangulation(aFace, points, facets))
            continue;

        std::vector<Base::Vector3f> vertices;
        std::vector<int> indices;
        vertices.resize(points.size());
        indices.resize(4 * facets.size());

        for (std::size_t i = 0; i < points.size(); i++) {
            vertices[i] = Base::convertTo<Base::Vector3f>(points[i]);
        }

        for (std::size_t i = 0; i < facets.size(); i++) {
            Standard_Integer n1,n2,n3;
            facets[i].Get(n1, n2, n3);
            indices[4 * i    ] = n1;
            indices[4 * i + 1] = n2;
            indices[4 * i + 2] = n3;
            indices[4 * i + 3] = -1;
        }

        builder.beginSeparator();
        Base::ShapeHintsItem shapeHints{static_cast<float>(ca)};
        builder.addNode(shapeHints);
        if (supportFaceColors) {
            App::Color c = colors[index];
            Base::MaterialItem material;
            material.setDiffuseColor({Base::ColorRGB{c.r, c.g, c.b}});
            material.setTransparency({c.a});
            builder.addNode(material);
        }

        Base::Coordinate3Item coords{vertices};
        builder.addNode(coords);
        Base::IndexedFaceSetItem faceSet{indices};
        builder.addNode(faceSet);
        builder.endSeparator();
    }
    builder.endSeparator();
}

void TopoShape::exportLineSet(std::ostream& str) const
{
    Base::InventorBuilder builder(str);
    builder.beginSeparator();
    // get a indexed map of edges
    TopTools_IndexedMapOfShape M;
    TopExp::MapShapes(this->_Shape, TopAbs_EDGE, M);

    // build up map edge->face
    TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
    TopExp::MapShapesAndAncestors(this->_Shape, TopAbs_EDGE, TopAbs_FACE, edge2Face);

    for (int i=0; i<M.Extent(); i++) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(M(i+1));
        std::vector<gp_Pnt> points;

        if (!Tools::getPolygon3D(aEdge, points)) {
            // the edge has not its own triangulation, but then a face the edge is attached to
            // must provide this triangulation

            // Look for one face in our map (it doesn't care which one we take)
            const TopoDS_Face& aFace = TopoDS::Face(edge2Face.FindFromKey(aEdge).First());
            if (!Tools::getPolygonOnTriangulation(aEdge, aFace, points))
                continue;
        }

        std::vector<Base::Vector3f> vertices;
        vertices.reserve(points.size());
        std::for_each(points.begin(), points.end(), [&vertices](const gp_Pnt& p) {
            vertices.push_back(Base::convertTo<Base::Vector3f>(p));
        });

        Base::DrawStyle drawStyle;
        drawStyle.lineWidth = 2.0F;
        builder.addNode(Base::MultiLineItem{vertices, drawStyle, Base::ColorRGB{0, 0, 0}});
    }

    builder.endSeparator();
}

Base::BoundBox3d TopoShape::getBoundBox() const
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
    catch (Standard_Failure&) {
    }

    return box;
}

namespace {
bool getShapeProperties(const TopoDS_Shape& shape, GProp_GProps& prop)
{
    TopExp_Explorer xpSolid(shape, TopAbs_SOLID);
    if (xpSolid.More()) {
        BRepGProp::VolumeProperties(shape, prop);
        return true;
    }

    TopExp_Explorer xpFace(shape, TopAbs_FACE);
    if (xpFace.More()) {
        BRepGProp::SurfaceProperties(shape, prop);
        return true;
    }

    TopExp_Explorer xpEdge(shape, TopAbs_EDGE);
    if (xpEdge.More()) {
        BRepGProp::LinearProperties(shape, prop);
        return true;
    }

    TopExp_Explorer xpVert(shape, TopAbs_VERTEX);
    if (xpVert.More()) {
        gp_Pnt pnts;
        int count = 0;
        for (; xpVert.More(); xpVert.Next()) {
            count++;
            gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(xpVert.Current()));
            pnts.SetX(pnts.X() + pnt.X());
            pnts.SetY(pnts.Y() + pnt.Y());
            pnts.SetZ(pnts.Z() + pnt.Z());
        }

        pnts.SetX(pnts.X() / count);
        pnts.SetY(pnts.Y() / count);
        pnts.SetZ(pnts.Z() / count);
        prop = GProp_GProps(pnts);

        return true;
    }

    return false;
}
}

bool TopoShape::getCenterOfGravity(Base::Vector3d& center) const
{
    if (_Shape.IsNull())
        return false;

    // Computing of CentreOfMass
    GProp_GProps prop;
    if (getShapeProperties(_Shape, prop)) {
        if (prop.Mass() > Precision::Infinite()) {
            return false;
        }
        gp_Pnt pnt = prop.CentreOfMass();
        center.Set(pnt.X(), pnt.Y(), pnt.Z());
        return true;
    }

    return false;
}

void TopoShape::Save (Base::Writer& writer) const
{
    if(!writer.isForceXML()) {
        //See SaveDocFile(), RestoreDocFile()
        // add a filename to the writer's list.  Each file on the list is eventually
        // processed by SaveDocFile().
        if (writer.getMode("BinaryBrep")) {
            writer.Stream() << writer.ind() << "<TopoShape file=\""
                            << writer.addFile("TopoShape.bin", this)
                            << "\"/>" << std::endl;
        }
        else {
            writer.Stream() << writer.ind() << "<TopoShape file=\""
                            << writer.addFile("TopoShape.brp", this)
                            << "\"/>" << std::endl;
        }
    }}

void TopoShape::Restore(Base::XMLReader& reader)
{
    reader.readElement("TopoShape");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // add a filename to the writer's list.  Each file on the list is eventually
        // processed by RestoreDocFile().
        reader.addFile(file.c_str(),this);
    }
}

void TopoShape::SaveDocFile (Base::Writer& writer) const
{
    if (getShape().IsNull()) {
        return;
    }
    //the writer has already opened a stream with the appropriate filename
    if (writer.getMode("BinaryBrep")) {
        exportBinary(writer.Stream());
    } else {
        exportBrep(writer.Stream());
    }
}

void TopoShape::RestoreDocFile(Base::Reader& reader)
{
    Base::FileInfo brep(reader.getFileName());
    if (brep.hasExtension("bin")) {
        importBinary(reader);
    } else {
        importBrep(reader);
    }
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

unsigned int TopoShape::getMemSize () const
{
    if (!_Shape.IsNull()) {
        // Count total amount of references of TopoDS_Shape objects
        unsigned int memsize = (sizeof(TopoDS_Shape)+sizeof(TopoDS_TShape)) * TopoShape_RefCountShapes(_Shape);

        // Now get a map of TopoDS_Shape objects without duplicates
        TopTools_IndexedMapOfShape M;
        TopExp::MapShapes(_Shape, M);
        for (int i=0; i<M.Extent(); i++) {
            const TopoDS_Shape& shape = M(i+1);
            if (shape.IsNull())
                continue;

            // add the size of the underlying geometric data
            Handle(TopoDS_TShape) tshape = shape.TShape();
            memsize += tshape->DynamicType()->Size();

            switch (shape.ShapeType())
            {
            case TopAbs_FACE:
                {
                    // first, last, tolerance
                    memsize += 5*sizeof(Standard_Real);
                    const TopoDS_Face& face = TopoDS::Face(shape);
                    // if no geometry is attached to a face an exception is raised
                    BRepAdaptor_Surface surface;
                    try {
                        surface.Initialize(face);
                    }
                    catch (const Standard_Failure&) {
                        continue;
                    }

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
                        memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Standard_Real);
                        memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_BSplineSurface:
                        memsize += sizeof(Geom_BSplineSurface);
                        memsize += (surface.NbUKnots()+surface.NbVKnots()) * sizeof(Standard_Real);
                        memsize += (surface.NbUPoles()*surface.NbVPoles()) * sizeof(Standard_Real);
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
                    // if no geometry is attached to an edge an exception is raised
                    BRepAdaptor_Curve curve;
                    try {
                        curve.Initialize(edge);
                    }
                    catch (const Standard_Failure&) {
                        continue;
                    }

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
                        memsize += curve.NbPoles() * sizeof(Standard_Real);
                        memsize += curve.NbPoles() * sizeof(Geom_CartesianPoint);
                        break;
                    case GeomAbs_BSplineCurve:
                        memsize += sizeof(Geom_BSplineCurve);
                        memsize += curve.NbKnots() * sizeof(Standard_Real);
                        memsize += curve.NbPoles() * sizeof(Standard_Real);
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

namespace Part {
std::vector<std::string> buildShapeEnumVector()
{
   std::vector<std::string> names;
   names.emplace_back("Compound");             //TopAbs_COMPOUND
   names.emplace_back("Compound Solid");       //TopAbs_COMPSOLID
   names.emplace_back("Solid");                //TopAbs_SOLID
   names.emplace_back("Shell");                //TopAbs_SHELL
   names.emplace_back("Face");                 //TopAbs_FACE
   names.emplace_back("Wire");                 //TopAbs_WIRE
   names.emplace_back("Edge");                 //TopAbs_EDGE
   names.emplace_back("Vertex");               //TopAbs_VERTEX
   names.emplace_back("Shape");                //TopAbs_SHAPE
   return names;
}

std::vector<std::string> buildBOPCheckResultVector()
{
  std::vector<std::string> results;
  results.emplace_back("BOPAlgo CheckUnknown");               //BOPAlgo_CheckUnknown
  results.emplace_back("BOPAlgo BadType");                    //BOPAlgo_BadType
  results.emplace_back("BOPAlgo SelfIntersect");              //BOPAlgo_SelfIntersect
  results.emplace_back("BOPAlgo TooSmallEdge");               //BOPAlgo_TooSmallEdge
  results.emplace_back("BOPAlgo NonRecoverableFace");         //BOPAlgo_NonRecoverableFace
  results.emplace_back("BOPAlgo IncompatibilityOfVertex");    //BOPAlgo_IncompatibilityOfVertex
  results.emplace_back("BOPAlgo IncompatibilityOfEdge");      //BOPAlgo_IncompatibilityOfEdge
  results.emplace_back("BOPAlgo IncompatibilityOfFace");      //BOPAlgo_IncompatibilityOfFace
  results.emplace_back("BOPAlgo OperationAborted");           //BOPAlgo_OperationAborted
  results.emplace_back("BOPAlgo GeomAbs_C0");                 //BOPAlgo_GeomAbs_C0
  results.emplace_back("BOPAlgo_InvalidCurveOnSurface");      //BOPAlgo_InvalidCurveOnSurface
  results.emplace_back("BOPAlgo NotValid");                   //BOPAlgo_NotValid
  return results;
}
}

bool TopoShape::analyze(bool runBopCheck, std::ostream& str) const
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

            for (const auto & shape : shapes) {
                if (!aChecker.IsValid(shape)) {
                    const Handle(BRepCheck_Result)& result = aChecker.Result(shape);
                    if (result.IsNull())
                        continue;
                    const BRepCheck_ListOfStatus& status = result->StatusOnShape(shape);

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
        else if (runBopCheck) {

            TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(this->_Shape).Shape();
            BOPAlgo_ArgumentAnalyzer BOPCheck;
            BOPCheck.SetShape1(BOPCopy);
            //all settings are false by default. so only turn on what we want.
            BOPCheck.ArgumentTypeMode() = true;
            BOPCheck.SelfInterMode() = true;
            BOPCheck.SmallEdgeMode() = true;
            BOPCheck.RebuildFaceMode() = true;
            BOPCheck.ContinuityMode() = true;
            BOPCheck.SetParallelMode(true); //this doesn't help for speed right now(occt 6.9.1).
            BOPCheck.SetRunParallel(true); //performance boost, use all available cores
            BOPCheck.TangentMode() = true; //these 4 new tests add about 5% processing time.
            BOPCheck.MergeVertexMode() = true;
            BOPCheck.CurveOnSurfaceMode() = true;
            BOPCheck.MergeEdgeMode() = true;
            BOPCheck.Perform();
            if (!BOPCheck.HasFaulty())
                return true;

            str << "BOP check found the following errors:" << std::endl;
            static std::vector<std::string> shapeEnumToString = buildShapeEnumVector();
            static std::vector<std::string> bopEnumToString = buildBOPCheckResultVector();
            const BOPAlgo_ListOfCheckResult &BOPResults = BOPCheck.GetCheckResult();
            BOPAlgo_ListIteratorOfListOfCheckResult BOPResultsIt(BOPResults);
            for (; BOPResultsIt.More(); BOPResultsIt.Next()) {
                const BOPAlgo_CheckResult &current = BOPResultsIt.Value();

                const TopTools_ListOfShape &faultyShapes1 = current.GetFaultyShapes1();
                TopTools_ListIteratorOfListOfShape faultyShapes1It(faultyShapes1);
                for (;faultyShapes1It.More(); faultyShapes1It.Next()) {
                    const TopoDS_Shape &faultyShape = faultyShapes1It.Value();
                    str << "Error in " << shapeEnumToString[faultyShape.ShapeType()] << ": ";
                    str << bopEnumToString[current.GetCheckStatus()] << std::endl;
                }
            }
            return false;
        }
    }
    return true;
}

bool TopoShape::isClosed() const
{
    if (this->_Shape.IsNull())
        return false;
    bool closed = false;
    switch (this->_Shape.ShapeType()) {
    case TopAbs_SHELL:
    case TopAbs_WIRE:
    case TopAbs_EDGE:
        closed = BRep_Tool::IsClosed(this->_Shape) ? true : false;
        break;
    case TopAbs_COMPSOLID:
    case TopAbs_SOLID:
        {
            closed = true;
            TopExp_Explorer xp(this->_Shape, TopAbs_SHELL);
            while (xp.More()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
                xp.Next();
            }
        }
        break;
    case TopAbs_COMPOUND:
        {
            closed = true;
            TopExp_Explorer xp;
            for (xp.Init(this->_Shape, TopAbs_SHELL); xp.More(); xp.Next()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
            }
            for (xp.Init(this->_Shape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
            }
            for (xp.Init(this->_Shape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
            }
            for (xp.Init(this->_Shape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
            }
            for (xp.Init(this->_Shape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
                closed &= BRep_Tool::IsClosed(xp.Current()) ? true : false;
            }
        }
        break;
    case TopAbs_FACE:
    case TopAbs_VERTEX:
    case TopAbs_SHAPE:
        closed = BRep_Tool::IsClosed(this->_Shape) ? true : false;
        break;
    }
    return closed;
}

TopoDS_Shape TopoShape::cut(TopoDS_Shape shape) const
{
    if (this->_Shape.IsNull())
        return this->_Shape;
    if (shape.IsNull())
        return this->_Shape;
    BRepAlgoAPI_Cut mkCut(this->_Shape, shape);
    return makeShell(mkCut.Shape());
}

TopoDS_Shape TopoShape::cut(const std::vector<TopoDS_Shape>& shapes, Standard_Real tolerance) const
{
    if (this->_Shape.IsNull())
        return this->_Shape;
    BRepAlgoAPI_Cut mkCut;
    mkCut.SetRunParallel(true);
    TopTools_ListOfShape shapeArguments,shapeTools;
    shapeArguments.Append(this->_Shape);
    for (const auto & shape : shapes) {
        if (shape.IsNull())
            throw Base::ValueError("Tool shape is null");
        if (tolerance > 0.0)
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shapeTools.Append(BRepBuilderAPI_Copy(shape).Shape());
        else
            shapeTools.Append(shape);
    }

    mkCut.SetArguments(shapeArguments);
    mkCut.SetTools(shapeTools);
    if (tolerance > 0.0)
        mkCut.SetFuzzyValue(tolerance);
    mkCut.Build();
    if (!mkCut.IsDone())
        throw Base::RuntimeError("Multi cut failed");

    TopoDS_Shape resShape = mkCut.Shape();
    return makeShell(resShape);
}

TopoDS_Shape TopoShape::common(TopoDS_Shape shape) const
{
    if (this->_Shape.IsNull())
        return this->_Shape;
    if (shape.IsNull())
        return shape;
    BRepAlgoAPI_Common mkCommon(this->_Shape, shape);
    return makeShell(mkCommon.Shape());
}

TopoDS_Shape TopoShape::common(const std::vector<TopoDS_Shape>& shapes, Standard_Real tolerance) const
{
    if (this->_Shape.IsNull())
        return this->_Shape;
    BRepAlgoAPI_Common mkCommon;
    mkCommon.SetRunParallel(true);
    TopTools_ListOfShape shapeArguments,shapeTools;
    shapeArguments.Append(this->_Shape);
    for (const auto & shape : shapes) {
        if (shape.IsNull())
            throw Base::ValueError("Tool shape is null");
        if (tolerance > 0.0)
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shapeTools.Append(BRepBuilderAPI_Copy(shape).Shape());
        else
            shapeTools.Append(shape);
    }

    mkCommon.SetArguments(shapeArguments);
    mkCommon.SetTools(shapeTools);
    if (tolerance > 0.0)
        mkCommon.SetFuzzyValue(tolerance);
    mkCommon.Build();
    if (!mkCommon.IsDone())
        throw Base::RuntimeError("Multi common failed");

    TopoDS_Shape resShape = mkCommon.Shape();
    return makeShell(resShape);
}

TopoDS_Shape TopoShape::fuse(TopoDS_Shape shape) const
{
    if (this->_Shape.IsNull())
        return shape;
    if (shape.IsNull())
        return this->_Shape;
    BRepAlgoAPI_Fuse mkFuse(this->_Shape, shape);
    return makeShell(mkFuse.Shape());
}

TopoDS_Shape TopoShape::fuse(const std::vector<TopoDS_Shape>& shapes, Standard_Real tolerance) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");

    BRepAlgoAPI_Fuse mkFuse;
    mkFuse.SetRunParallel(true);
    TopTools_ListOfShape shapeArguments,shapeTools;
    shapeArguments.Append(this->_Shape);
    for (const auto & shape : shapes) {
        if (shape.IsNull())
            throw NullShapeException("Tool shape is null");
        if (tolerance > 0.0)
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shapeTools.Append(BRepBuilderAPI_Copy(shape).Shape());
        else
            shapeTools.Append(shape);
    }
    mkFuse.SetArguments(shapeArguments);
    mkFuse.SetTools(shapeTools);
    if (tolerance > 0.0)
        mkFuse.SetFuzzyValue(tolerance);
    mkFuse.Build();
    if (!mkFuse.IsDone())
        throw Base::RuntimeError("Multi fuse failed");

    TopoDS_Shape resShape = mkFuse.Shape();
    return makeShell(resShape);
}

TopoDS_Shape TopoShape::oldFuse(TopoDS_Shape shape) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");
    if (shape.IsNull())
        Standard_Failure::Raise("Tool shape is null");

    throw Standard_Failure("BRepAlgo_Fuse is deprecated since OCCT 7.3");
}

TopoDS_Shape TopoShape::section(TopoDS_Shape shape, Standard_Boolean approximate) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");
    if (shape.IsNull())
        Standard_Failure::Raise("Tool shape is null");
    BRepAlgoAPI_Section mkSection;
    mkSection.Init1(this->_Shape);
    mkSection.Init2(shape);
    mkSection.Approximation(approximate);
    mkSection.Build();
    if (!mkSection.IsDone())
        throw Base::RuntimeError("Section failed");
    return mkSection.Shape();
}

TopoDS_Shape TopoShape::section(const std::vector<TopoDS_Shape>& shapes,
                                Standard_Real tolerance,
                                Standard_Boolean approximate) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");

    BRepAlgoAPI_Section mkSection;
    mkSection.SetRunParallel(true);
    mkSection.Approximation(approximate);
    TopTools_ListOfShape shapeArguments,shapeTools;
    shapeArguments.Append(this->_Shape);
    for (const auto & shape : shapes) {
        if (shape.IsNull())
            throw Base::ValueError("Tool shape is null");
        if (tolerance > 0.0)
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            shapeTools.Append(BRepBuilderAPI_Copy(shape).Shape());
        else
            shapeTools.Append(shape);
    }

    mkSection.SetArguments(shapeArguments);
    mkSection.SetTools(shapeTools);
    if (tolerance > 0.0)
        mkSection.SetFuzzyValue(tolerance);
    mkSection.Build();
    if (!mkSection.IsDone())
        throw Base::RuntimeError("Multi section failed");

    TopoDS_Shape resShape = mkSection.Shape();
    return resShape;
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
    for (double jt : d) {
        wire_list.push_back(cs.slice(jt));
    }

    std::vector< std::list<TopoDS_Wire> >::const_iterator ft;
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);

    for (ft = wire_list.begin(); ft != wire_list.end(); ++ft) {
        const std::list<TopoDS_Wire>& w = *ft;
        for (const auto & wt : w) {
            if (!wt.IsNull())
                builder.Add(comp, wt);
        }
    }

    return comp;
}

TopoDS_Shape TopoShape::generalFuse(const std::vector<TopoDS_Shape> &sOthers, Standard_Real tolerance,
                                    std::vector<TopTools_ListOfShape>* mapInOut) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");

    BRepAlgoAPI_BuilderAlgo mkGFA;
    mkGFA.SetRunParallel(true);
    TopTools_ListOfShape GFAArguments;
    GFAArguments.Append(this->_Shape);
    for (const TopoDS_Shape &it: sOthers) {
        if (it.IsNull())
            throw NullShapeException("Tool shape is null");
        if (tolerance > 0.0)
            // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
            GFAArguments.Append(BRepBuilderAPI_Copy(it).Shape());
        else
            GFAArguments.Append(it);
    }
    mkGFA.SetArguments(GFAArguments);
    if (tolerance > 0.0)
        mkGFA.SetFuzzyValue(tolerance);
    mkGFA.SetNonDestructive(Standard_True);
    mkGFA.Build();
    if (!mkGFA.IsDone())
        throw BooleanException("MultiFusion failed");
    TopoDS_Shape resShape = mkGFA.Shape();
    if (mapInOut){
        for(TopTools_ListIteratorOfListOfShape it(GFAArguments); it.More(); it.Next()){
            mapInOut->push_back(mkGFA.Modified(it.Value()));
        }
    }
    return resShape;
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

    if (!mkPipeShell.IsReady())
        throw Standard_Failure("shape is not ready to build");

    mkPipeShell.Build();

    if (make_solid)
        mkPipeShell.MakeSolid();

    return mkPipeShell.Shape();
}

static Handle(Law_Function) CreateBsFunction (const Standard_Real theFirst, const Standard_Real theLast, const Standard_Real theRadius)
{
    (void)theRadius;
    //Handle(Law_BSpline) aBs;
    //Handle(Law_BSpFunc) aFunc = new Law_BSpFunc (aBs, theFirst, theLast);
    Handle(Law_Constant) aFunc = new Law_Constant();
    aFunc->Set(1, theFirst, theLast);
    return aFunc;
}

TopoDS_Shape TopoShape::makeTube(double radius, double tol, int cont, int maxdegree, int maxsegm) const
{
    // http://opencascade.blogspot.com/2009/11/surface-modeling-part3.html
    Standard_Real theTol = tol;
    Standard_Real theRadius = radius;
    //Standard_Boolean theIsPolynomial = Standard_True;
    Standard_Boolean myIsElem = Standard_True;
    GeomAbs_Shape theContinuity = GeomAbs_Shape(cont);
    Standard_Integer theMaxDegree = maxdegree;
    Standard_Integer theMaxSegment = maxsegm;

    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot sweep along empty spine");

#if OCC_VERSION_HEX >= 0x070600

    Handle(Adaptor3d_Curve) myPath;

    if (this->_Shape.ShapeType() == TopAbs_EDGE) {
        const TopoDS_Edge& path_edge = TopoDS::Edge(this->_Shape);
        myPath = new BRepAdaptor_Curve(path_edge);
    }
#else
    Handle(Adaptor3d_HCurve) myPath;
    if (this->_Shape.ShapeType() == TopAbs_EDGE) {
        const TopoDS_Edge& path_edge = TopoDS::Edge(this->_Shape);
        BRepAdaptor_Curve path_adapt(path_edge);
        myPath = new BRepAdaptor_HCurve(path_adapt);
    }
#endif

    else {
        Standard_Failure::Raise("Spine shape is not an edge");
    }

    //circular profile
    Handle(Geom_Circle) aCirc = new Geom_Circle (gp::XOY(), theRadius);
    aCirc->Rotate (gp::OZ(), M_PI/2.);

    //perpendicular section
    Handle(Law_Function) myEvol = ::CreateBsFunction (myPath->FirstParameter(), myPath->LastParameter(), theRadius);
    Handle(GeomFill_SectionLaw) aSec = new GeomFill_EvolvedSection(aCirc, myEvol);
    Handle(GeomFill_LocationLaw) aLoc = new GeomFill_CurveAndTrihedron(new GeomFill_CorrectedFrenet);
    aLoc->SetCurve (myPath);

    GeomFill_Sweep mkSweep (aLoc, myIsElem);
    mkSweep.SetTolerance (theTol);
    mkSweep.Build (aSec, GeomFill_Location, theContinuity, theMaxDegree, theMaxSegment);
    if (mkSweep.IsDone()) {
        Handle(Geom_Surface) mySurface = mkSweep.Surface();
        //Standard_Real myError = mkSweep.ErrorOnSurface();

        Standard_Real u1,u2,v1,v2;
        mySurface->Bounds(u1,u2,v1,v2);
        BRepBuilderAPI_MakeFace mkBuilder(mySurface, u1, u2, v1, v2 , Precision::Confusion()
        );
        return mkBuilder.Shape();
    }

    return {};
}


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
    Handle(Geom_Curve) hPath = path_adapt.Curve().Curve();

    // Apply placement of the shape to the curve
    TopLoc_Location loc1 = path_edge.Location();
    hPath = Handle(Geom_Curve)::DownCast(hPath->Transformed(loc1.Transformation()));

    if (hPath.IsNull())
        Standard_Failure::Raise("invalid curve in path edge");

    BRepAdaptor_Curve prof_adapt(prof_edge);
    double vmin = prof_adapt.FirstParameter();
    double vmax = prof_adapt.LastParameter();
    Handle(Geom_Curve) hProfile = prof_adapt.Curve().Curve();

    // Apply placement of the shape to the curve
    TopLoc_Location loc2 = prof_edge.Location();
    hProfile = Handle(Geom_Curve)::DownCast(hProfile->Transformed(loc2.Transformation()));

    if (hProfile.IsNull())
        Standard_Failure::Raise("invalid curve in profile edge");

    GeomFill_Pipe mkSweep(hPath, hProfile, static_cast<GeomFill_Trihedron>(fillMode));
    mkSweep.GenerateParticularCase(Standard_True);
    mkSweep.Perform(tol, Standard_False, GeomAbs_C1, BSplCLib::MaxDegree(), 1000);

    const Handle(Geom_Surface)& surf = mkSweep.Surface();
    BRepBuilderAPI_MakeFace mkBuilder(surf, umin, umax, vmin, vmax , Precision::Confusion());
    return mkBuilder.Face();
}

TopoDS_Shape TopoShape::makeTorus(Standard_Real radius1, Standard_Real radius2,
                                  Standard_Real angle1, Standard_Real angle2,
                                  Standard_Real angle3, Standard_Boolean isSolid) const
{
    // https://forum.freecad.org/viewtopic.php?f=3&t=1445
    // https://forum.freecad.org/viewtopic.php?f=3&t=52719
    // Build a torus
    gp_Circ circle;
    circle.SetRadius(radius2);
    gp_Pnt pos(radius1,0,0);
    gp_Dir dir(0,1,0);
    circle.SetAxis(gp_Ax1(pos, dir));

    BRepBuilderAPI_MakeEdge mkEdge(circle, Base::toRadians<double>(angle1),
                                           Base::toRadians<double>(angle2));
    BRepBuilderAPI_MakeWire mkWire;
    mkWire.Add(mkEdge.Edge());

    if ((angle1 > -180.0 || angle2 < 180.0) && isSolid) {
        BRepBuilderAPI_MakeVertex mkVertex(pos);
        BRepBuilderAPI_MakeEdge mkEdge1(mkVertex.Vertex(), mkEdge.Vertex1());
        BRepBuilderAPI_MakeEdge mkEdge2(mkVertex.Vertex(), mkEdge.Vertex2());
        mkWire.Add(mkEdge1.Edge());
        mkWire.Add(mkEdge2.Edge());
    }

    BRepBuilderAPI_MakeFace mkFace(mkWire.Wire());
    BRepPrimAPI_MakeRevol mkRevol(mkFace.Face(), gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)),
        Base::toRadians<double>(angle3), Standard_True);
    return mkRevol.Shape();
}

TopoDS_Shape TopoShape::makeHelix(Standard_Real pitch, Standard_Real height,
                                  Standard_Real radius, Standard_Real angle,
                                  Standard_Boolean leftHanded,
                                  Standard_Boolean newStyle) const
{
    if (fabs(pitch) < Precision::Confusion())
        Standard_Failure::Raise("Pitch of helix too small");

    if (fabs(height) < Precision::Confusion())
        Standard_Failure::Raise("Height of helix too small");

    if ((height > 0 && pitch < 0) || (height < 0 && pitch > 0))
        Standard_Failure::Raise("Pitch and height of helix not compatible");

    gp_Ax2 cylAx2(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
    Handle(Geom_Surface) surf;
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
    Standard_Real coneDir = 1.0;
    if (leftHanded) {
        aDir.SetCoord(-2. * M_PI, pitch);
        coneDir = -1.0;
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
            Standard_Real u = coneDir * (height/pitch) * 2.0 * M_PI;
            gp_Pnt2d cend(u, v);
            end = cend;
        }
    }

    Handle(Geom2d_TrimmedCurve) segm = GCE2d_MakeSegment(beg , end);

    TopoDS_Edge edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edgeOnSurf);
    BRepLib::BuildCurves3d(wire);
    return TopoDS_Shape(std::move(wire));
}

//***********
// makeLongHelix is a workaround for an OCC problem found in helices with more than
// some magic number of turns.  See Mantis #0954.
//***********
TopoDS_Shape TopoShape::makeLongHelix(Standard_Real pitch, Standard_Real height,
                                      Standard_Real radius, Standard_Real angle,
                                      Standard_Boolean leftHanded) const
{
    if (pitch < Precision::Confusion())
        Standard_Failure::Raise("Pitch of helix too small");

    if (height < Precision::Confusion())
        Standard_Failure::Raise("Height of helix too small");

    gp_Ax2 cylAx2(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
    Handle(Geom_Surface) surf;
    Standard_Boolean isCylinder;

    if (std::fabs(angle) < Precision::Confusion()) {                           // Cylindrical helix
        if (radius < Precision::Confusion())
            Standard_Failure::Raise("Radius of helix too small");
        surf= new Geom_CylindricalSurface(cylAx2, radius);
        isCylinder = true;
    }
    else {                                                                     // Conical helix
        angle = Base::toRadians(angle);
        surf = new Geom_ConicalSurface(gp_Ax3(cylAx2), angle, radius);
        isCylinder = false;
    }

    Standard_Real turns = height/pitch;
    unsigned long wholeTurns = floor(turns);
    Standard_Real partTurn = turns - wholeTurns;

    gp_Pnt2d aPnt(0, 0);
    gp_Dir2d aDir(2. * M_PI, pitch);
    Standard_Real coneDir = 1.0;
    if (leftHanded) {
        aDir.SetCoord(-2. * M_PI, pitch);
        coneDir = -1.0;
    }
    gp_Ax2d aAx2d(aPnt, aDir);
    Handle(Geom2d_Line) line = new Geom2d_Line(aAx2d);
    gp_Pnt2d beg = line->Value(0);
    gp_Pnt2d end;
    Standard_Real u,v;
    BRepBuilderAPI_MakeWire mkWire;
    Handle(Geom2d_TrimmedCurve) segm;
    TopoDS_Edge edgeOnSurf;

    for (unsigned long i = 0; i < wholeTurns; i++) {
        if (isCylinder) {
            end = line->Value(sqrt(4.0*M_PI*M_PI+pitch*pitch)*(i+1));
        }
        else {
            u = coneDir * (i+1) * 2.0 * M_PI;
            v = ((i+1) * pitch) / cos(angle);
            end = gp_Pnt2d(u, v);
        }
        segm = GCE2d_MakeSegment(beg , end);
        edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
        mkWire.Add(edgeOnSurf);
        beg = end;
    }

    if (partTurn > Precision::Confusion()) {
        if (isCylinder) {
            end = line->Value(sqrt(4.0*M_PI*M_PI+pitch*pitch)*turns);
        }
        else {
            u = coneDir * turns * 2.0 * M_PI;
            v = height / cos(angle);
            end = gp_Pnt2d(u, v);
        }
        segm = GCE2d_MakeSegment(beg , end);
        edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
        mkWire.Add(edgeOnSurf);
    }

    TopoDS_Wire wire = mkWire.Wire();
    BRepLib::BuildCurves3d(wire);
    return TopoDS_Shape(std::move(wire));
}

TopoDS_Shape TopoShape::makeSpiralHelix(Standard_Real radiusbottom, Standard_Real radiustop,
                                  Standard_Real height, Standard_Real nbturns,
                                  Standard_Real breakperiod, Standard_Boolean leftHanded) const
{
    // 1000 periods is an OCCT limit. The 3D curve gets truncated
    // if the 2D curve spans beyond this limit.
    if ((breakperiod < 0) || (breakperiod > 1000))
        Standard_Failure::Raise("Break period must be in [0, 1000]");
    if (breakperiod == 0)
        breakperiod = 1000;
    if (nbturns <= 0)
        Standard_Failure::Raise("Number of turns must be greater than 0");

    Standard_Real nbPeriods = nbturns/breakperiod;
    Standard_Real nbFullPeriods = floor(nbPeriods);
    Standard_Real partPeriod = nbPeriods - nbFullPeriods;

    // A Bezier curve is used below, to get a periodic surface also for spirals.
    TColgp_Array1OfPnt poles(1,2);
    poles(1) = gp_Pnt(radiusbottom, 0, 0);
    poles(2) = gp_Pnt(radiustop, 0, height);
    Handle(Geom_BezierCurve) meridian = new Geom_BezierCurve(poles);

    gp_Ax1 axis(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
    Handle(Geom_Surface) surf = new Geom_SurfaceOfRevolution(meridian, axis);

    gp_Pnt2d beg(0, 0);
    gp_Pnt2d end(0, 0);
    gp_Vec2d dir(breakperiod * 2.0 * M_PI, 1 / nbPeriods);
    if (leftHanded == Standard_True)
        dir = gp_Vec2d(-breakperiod * 2.0 * M_PI, 1 / nbPeriods);
    Handle(Geom2d_TrimmedCurve) segm;
    TopoDS_Edge edgeOnSurf;
    BRepBuilderAPI_MakeWire mkWire;
    for (unsigned long i = 0; i < nbFullPeriods; i++) {
        end = beg.Translated(dir);
        segm = GCE2d_MakeSegment(beg , end);
        edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
        mkWire.Add(edgeOnSurf);
        beg = end;
    }
    if (partPeriod > Precision::Confusion()) {
        dir.Scale(partPeriod);
        end = beg.Translated(dir);
        segm = GCE2d_MakeSegment(beg , end);
        edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
        mkWire.Add(edgeOnSurf);
    }

    TopoDS_Wire wire = mkWire.Wire();
    BRepLib::BuildCurves3d(wire, Precision::Confusion(), GeomAbs_Shape::GeomAbs_C1, 14, 10000);
    return TopoDS_Shape(std::move(wire));
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
                                 Standard_Boolean isRuled,
                                 Standard_Boolean isClosed,
                                 Standard_Integer maxDegree) const
{
    // http://opencascade.blogspot.com/2010/01/surface-modeling-part5.html
    BRepOffsetAPI_ThruSections aGenerator (isSolid,isRuled);
    aGenerator.SetMaxDegree(maxDegree);

    TopTools_ListIteratorOfListOfShape it;
    int countShapes = 0;
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

    if (countShapes < 2) {
        Standard_Failure::Raise("Need at least two vertices, edges or wires to create loft face");
    }
    else {
        // close loft by duplicating initial profile as last profile.  not perfect.
        if (isClosed) {
        /* can only close loft in certain combinations of Vertex/Wire(Edge):
            - V1-W1-W2-W3-V2  ==> V1-W1-W2-W3-V2-V1  invalid closed
            - V1-W1-W2-W3     ==> V1-W1-W2-W3-V1     valid closed
            - W1-W2-W3-V1     ==> W1-W2-W3-V1-W1     invalid closed
            - W1-W2-W3        ==> W1-W2-W3-W1        valid closed*/
            if (profiles.Last().ShapeType() == TopAbs_VERTEX) {
                Base::Console().Message("TopoShape::makeLoft: can't close Loft with Vertex as last profile. 'Closed' ignored.\n");
            }
            else {
                // repeat Add logic above for first profile
                const TopoDS_Shape& firstProfile = profiles.First();
                if (firstProfile.ShapeType() == TopAbs_VERTEX)  {
                    aGenerator.AddVertex(TopoDS::Vertex (firstProfile));
                    countShapes++;
                }
                else if (firstProfile.ShapeType() == TopAbs_EDGE)  {
                    aGenerator.AddWire(TopoDS::Wire (firstProfile));
                    countShapes++;
                }
                else if (firstProfile.ShapeType() == TopAbs_WIRE)  {
                    aGenerator.AddWire(TopoDS::Wire (firstProfile));
                    countShapes++;
                }
            }
        }
    }

    Standard_Boolean anIsCheck = Standard_True;
    aGenerator.CheckCompatibility (anIsCheck);   // use BRepFill_CompatibleWires on profiles. force #edges, orientation, "origin" to match.
    aGenerator.Build();
    if (!aGenerator.IsDone())
        Standard_Failure::Raise("Failed to create loft face");

    //Base::Console().Message("DEBUG: TopoShape::makeLoft returns.\n");
    return aGenerator.Shape();
}

TopoDS_Shape TopoShape::makePrism(const gp_Vec& vec) const
{
    if (this->_Shape.IsNull()) Standard_Failure::Raise("cannot sweep empty shape");
    BRepPrimAPI_MakePrism mkPrism(this->_Shape, vec);
    return mkPrism.Shape();
}

TopoDS_Shape TopoShape::revolve(const gp_Ax1& axis, double d, Standard_Boolean isSolid) const
{
    if (this->_Shape.IsNull()) Standard_Failure::Raise("cannot revolve empty shape");

    TopoDS_Face f;
    TopoDS_Wire w;
    TopoDS_Edge e;
    Standard_Boolean convertFailed = false;

    TopoDS_Shape base = this->_Shape;
    if ((isSolid) && (BRep_Tool::IsClosed(base)) &&
        ((base.ShapeType() == TopAbs_EDGE) || (base.ShapeType() == TopAbs_WIRE))) {
        if (base.ShapeType() == TopAbs_EDGE) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(base));
            if (mkWire.IsDone()) {
                w = mkWire.Wire(); }
            else {
                convertFailed = true; }
        }
        else {
             w = TopoDS::Wire(base);}
        if (!convertFailed) {
            BRepBuilderAPI_MakeFace mkFace(w);
            if (mkFace.IsDone()) {
                f = mkFace.Face();
                base = f; }
            else {
                convertFailed = true; }
        }
    }

    if (convertFailed) {
        Base::Console().Message("TopoShape::revolve could not make Solid from Wire/Edge.\n");}

    BRepPrimAPI_MakeRevol mkRevol(base, axis,d);
    return mkRevol.Shape();
}

TopoDS_Shape TopoShape::makeOffsetShape(double offset, double tol, bool intersection,
                                        bool selfInter, short offsetMode, short join,
                                        bool fill) const
{
    // If the input shape is a compound with a single solid then the offset
    // algorithm creates only a shell instead of a solid which causes errors
    // when using it e.g. for boolean operations. (#0003571)
    // But when extracting the solid and passing it to the algorithm the output
    // shape is a solid.
    TopoDS_Shape inputShape = this->_Shape;
    TopExp_Explorer xp;
    xp.Init(inputShape, TopAbs_VERTEX, TopAbs_SOLID);
    if (!xp.More()) {
        xp.Init(inputShape, TopAbs_SOLID);
        if (xp.More()) {
            // If exactly one solid then get it
            TopoDS_Shape inputSolid = xp.Current();
            xp.Next();
            if (xp.More() == Standard_False)
                inputShape = inputSolid;
        }
    }

    BRepOffsetAPI_MakeOffsetShape mkOffset;
    mkOffset.PerformByJoin(inputShape, offset, tol, BRepOffset_Mode(offsetMode),
                           intersection ? Standard_True : Standard_False,
                           selfInter ? Standard_True : Standard_False,
                           GeomAbs_JoinType(join));

    if (!mkOffset.IsDone())
        Standard_Failure::Raise("BRepOffsetAPI_MakeOffsetShape not done");
    const TopoDS_Shape& res = mkOffset.Shape();
    if (!fill)
        return res;

    //get perimeter wire of original shape.
    //Wires returned seem to have edges in connection order.
    ShapeAnalysis_FreeBoundsProperties freeCheck(this->_Shape);
    freeCheck.Perform();
    if (freeCheck.NbClosedFreeBounds() < 1)
    {
        Standard_Failure::Raise("no closed bounds");
    }

    BRep_Builder builder;
    TopoDS_Compound perimeterCompound;
    builder.MakeCompound(perimeterCompound);
    for (int index = 1; index <= freeCheck.NbClosedFreeBounds(); ++index)
    {
        TopoDS_Wire originalWire = freeCheck.ClosedFreeBound(index)->FreeBound();
        const BRepAlgo_Image& img = mkOffset.MakeOffset().OffsetEdgesFromShapes();

        //build offset wire.
        TopoDS_Wire offsetWire;
        builder.MakeWire(offsetWire);
        TopExp_Explorer xp;
        for (xp.Init(originalWire, TopAbs_EDGE); xp.More(); xp.Next())
        {
            if (!img.HasImage(xp.Current()))
            {
                Standard_Failure::Raise("no image for shape");
            }
            const TopTools_ListOfShape& currentImage = img.Image(xp.Current());
            TopTools_ListIteratorOfListOfShape listIt;
            int edgeCount(0);
            TopoDS_Edge mappedEdge;
            for (listIt.Initialize(currentImage); listIt.More(); listIt.Next())
            {
                if (listIt.Value().ShapeType() != TopAbs_EDGE)
                    continue;
                edgeCount++;
                mappedEdge = TopoDS::Edge(listIt.Value());
            }

            if (edgeCount != 1)
            {
                std::ostringstream stream;
                stream << "wrong edge count: " << edgeCount << std::endl;
                Standard_Failure::Raise(stream.str().c_str());
            }
            builder.Add(offsetWire, mappedEdge);
        }

        //It would be nice if we could get thruSections to build planar faces
        //in all areas possible, so we could run through refine. I tried setting
        //ruled to standard_true, but that didn't have the desired affect.
        BRepOffsetAPI_ThruSections aGenerator;
        aGenerator.AddWire(originalWire);
        aGenerator.AddWire(offsetWire);
        aGenerator.Build();
        if (!aGenerator.IsDone())
        {
            Standard_Failure::Raise("ThruSections failed");
        }

        builder.Add(perimeterCompound, aGenerator.Shape());
    }

    //still had to sew. not using the passed in parameter for sew.
    //Sew has it's own default tolerance. Opinions?
    BRepBuilderAPI_Sewing sewTool;
    sewTool.Add(this->_Shape);
    sewTool.Add(perimeterCompound);
    sewTool.Add(res);
    sewTool.Perform(); //Perform Sewing

    TopoDS_Shape outputShape = sewTool.SewedShape();
    if ((outputShape.ShapeType() == TopAbs_SHELL) && (outputShape.Closed()))
    {
        BRepBuilderAPI_MakeSolid solidMaker(TopoDS::Shell(outputShape));
        if (solidMaker.IsDone())
        {
            TopoDS_Solid temp = solidMaker.Solid();
            //contrary to the occ docs the return value OrientCloseSolid doesn't
            //indicate whether the shell was open or not. It returns true with an
            //open shell and we end up with an invalid solid.
            if (BRepLib::OrientClosedSolid(temp))
                outputShape = temp;
        }
    }

    return outputShape;
}

TopoDS_Shape TopoShape::makeOffset2D(double offset, short joinType, bool fill, bool allowOpenResult, bool intersection) const
{
    if (_Shape.IsNull())
        throw Base::ValueError("makeOffset2D: input shape is null!");

    // OUTLINE OF MAKEOFFSET2D
    // * Prepare shapes to process
    // ** if _Shape is a compound, recursively call this routine for all subcompounds
    // ** if intrsection, dump all non-compound children into shapes to process; otherwise call this routine recursively for all children
    // ** if _shape isn't a compound, dump it straight to shapes to process
    // * Test for shape types, and convert them all to wires
    // * find plane
    // * OCC call (BRepBuilderAPI_MakeOffset)
    // * postprocessing (facemaking):
    // ** convert offset result back to faces, if inputs were faces
    // ** OR do offset filling:
    // *** for closed wires, simply feed source wires + offset wires to smart facemaker
    // *** for open wires, try to connect source anf offset result by creating new edges (incomplete implementation)
    // ** actual call to FaceMakerBullseye, unified for all facemaking.

    std::vector<TopoDS_Shape> shapesToProcess;
    std::vector<TopoDS_Shape> shapesToReturn;
    bool forceOutputCompound = false;

    if (this->_Shape.ShapeType() == TopAbs_COMPOUND) {
        if (!intersection) {
            //simply recursively process the children, independently
            TopoDS_Iterator it(_Shape);
            for( ; it.More() ; it.Next()) {
                shapesToReturn.push_back( TopoShape(it.Value()).makeOffset2D(offset, joinType, fill, allowOpenResult, intersection) );
                forceOutputCompound = true;
            }
        }
        else {
            //collect non-compounds from this compound for collective offset. Process other shapes independently.
            TopoDS_Iterator it(_Shape);
            for( ; it.More() ; it.Next()) {
                if(it.Value().ShapeType() == TopAbs_COMPOUND) {
                    //recursively process subcompounds
                    shapesToReturn.push_back( TopoShape(it.Value()).makeOffset2D(offset, joinType, fill, allowOpenResult, intersection) );
                    forceOutputCompound = true;
                }
                else {
                    shapesToProcess.push_back(it.Value());
                }
            }
        }
    }
    else {
        shapesToProcess.push_back(this->_Shape);
    }

    if(!shapesToProcess.empty()) {

        //although 2d offset supports offsetting a face directly, it seems there is
        //no way to do a collective offset of multiple faces. So, we are doing it
        //by getting all wires from the faces, and applying offsets to them, and
        //reassembling the faces later.
        std::vector<TopoDS_Wire> sourceWires;
        bool haveWires = false;
        bool haveFaces = false;
        for(TopoDS_Shape &sh : shapesToProcess){
            switch (sh.ShapeType()) {
                case TopAbs_EDGE:
                case TopAbs_WIRE:{
                    //convert edge to a wire if necessary...
                    TopoDS_Wire sourceWire;
                    if (sh.ShapeType() == TopAbs_WIRE){
                        sourceWire = TopoDS::Wire(sh);
                    } else { //edge
                        sourceWire = BRepBuilderAPI_MakeWire(TopoDS::Edge(sh)).Wire();
                    }
                    sourceWires.push_back(sourceWire);
                    haveWires = true;
                }break;
                case TopAbs_FACE:{
                    //get all wires of the face
                    TopoDS_Iterator it(sh);
                    for(; it.More(); it.Next()){
                        sourceWires.push_back(TopoDS::Wire(it.Value()));
                    }
                    haveFaces = true;
                }break;
                default:
                    throw Base::TypeError("makeOffset2D: input shape is not an edge, wire or face or compound of those.");
                break;
            }
        }
        if (haveWires && haveFaces)
            throw Base::TypeError("makeOffset2D: collective offset of a mix of wires and faces is not supported");
        if (haveFaces)
            allowOpenResult = false;

        //find plane.
        gp_Pln workingPlane;
        TopoDS_Compound compoundSourceWires;
        {
            BRep_Builder builder;
            builder.MakeCompound(compoundSourceWires);
            for(TopoDS_Wire &w : sourceWires)
                builder.Add(compoundSourceWires, w);
            BRepLib_FindSurface planefinder(compoundSourceWires, -1, Standard_True);
            if (!planefinder.Found())
                throw Base::CADKernelError("makeOffset2D: wires are nonplanar or noncoplanar");
            if (haveFaces){
                //extract plane from first face (useful for preserving the plane of face precisely if dealing with only one face)
                workingPlane = BRepAdaptor_Surface(TopoDS::Face(shapesToProcess[0])).Plane();
            } else {
                workingPlane = GeomAdaptor_Surface(planefinder.Surface()).Plane();
            }
        }

        //do the offset..
        TopoDS_Shape offsetShape;
        BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType), allowOpenResult);
        for (TopoDS_Wire &w : sourceWires) {
            mkOffset.AddWire(w);
        }

        if (fabs(offset) > Precision::Confusion()) {
            try {
    #if defined(__GNUC__) && defined (FC_OS_LINUX)
                Base::SignalException se;
    #endif
                mkOffset.Perform(offset);
            }
            catch (Standard_Failure &){
                throw;
            }
            catch (...) {
                throw Base::CADKernelError("BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
            }
            offsetShape = mkOffset.Shape();

            // Replace OffsetCurve with B-Spline
            if (!offsetShape.IsNull()) {
                offsetShape = mkOffset.Replace(GeomAbs_OffsetCurve, offsetShape);
            }

            if (offsetShape.IsNull())
                throw Base::CADKernelError("makeOffset2D: result of offsetting is null!");

            //Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
            // http://www.freecad.org/tracker/view.php?id=2699
            offsetShape = BRepBuilderAPI_Copy(offsetShape).Shape();
        }
        else {
            offsetShape = sourceWires.size()>1 ? TopoDS_Shape(compoundSourceWires) : sourceWires[0];
        }

        std::list<TopoDS_Wire> offsetWires;
        //interestingly, if wires are removed, empty compounds are returned by MakeOffset (as of OCC 7.0.0)
        //so, we just extract all nesting
        Handle(TopTools_HSequenceOfShape) seq = ShapeExtend_Explorer().SeqFromCompound(offsetShape, Standard_True);
        TopoDS_Iterator it(offsetShape);
        for(int i = 0; i < seq->Length(); ++i){
            offsetWires.push_back(TopoDS::Wire(seq->Value(i+1)));
        }

        if (offsetWires.empty())
            throw Base::CADKernelError("makeOffset2D: offset result has no wires.");

        std::list<TopoDS_Wire> wiresForMakingFaces;
        if (!fill){
            if (haveFaces){
                wiresForMakingFaces = offsetWires;
            }
            else {
                for(TopoDS_Wire &w : offsetWires)
                    shapesToReturn.push_back(w);
            }
        }
        else {
            //fill offset
            if (fabs(offset) < Precision::Confusion())
                throw Base::ValueError("makeOffset2D: offset distance is zero. Can't fill offset.");

            //filling offset. There are three major cases to consider:
            // 1. source wires and result wires are closed (simplest) -> make face
            // from source wire + offset wire
            //
            // 2. source wire is open, but offset wire is closed (if not
            // allowOpenResult). -> throw away source wire and make face right from
            // offset result.
            //
            // 3. source and offset wire are both open (note that there may be
            // closed islands in offset result) -> need connecting offset result to
            // source wire with new edges

            //first, lets split apart closed and open wires.
            std::list<TopoDS_Wire> closedWires;
            std::list<TopoDS_Wire> openWires;
            for(TopoDS_Wire &w : sourceWires)
                if (BRep_Tool::IsClosed(w))
                    closedWires.push_back(w);
                else
                    openWires.push_back(w);
            for(TopoDS_Wire &w : offsetWires)
                if (BRep_Tool::IsClosed(w))
                    closedWires.push_back(w);
                else
                    openWires.push_back(w);

            wiresForMakingFaces = closedWires;
            if (!allowOpenResult || openWires.empty()){
                //just ignore all open wires
            }
            else {
                //We need to connect open wires to form closed wires.

                //for now, only support offsetting one open wire -> there should be exactly two open wires for connecting
                if (openWires.size() != 2)
                    throw Base::CADKernelError("makeOffset2D: collective offset with filling of multiple wires is not supported yet.");

                TopoDS_Wire openWire1 = openWires.front();
                TopoDS_Wire openWire2 = openWires.back();

                //find open vertices
                BRepTools_WireExplorer xp;
                xp.Init(openWire1);
                TopoDS_Vertex v1 = xp.CurrentVertex();
                for(;xp.More();xp.Next()){};
                TopoDS_Vertex v2 = xp.CurrentVertex();

                //find open vertices
                xp.Init(openWire2);
                TopoDS_Vertex v3 = xp.CurrentVertex();
                for(;xp.More();xp.Next()){};
                TopoDS_Vertex v4 = xp.CurrentVertex();

                //check
                if (v1.IsNull())  throw NullShapeException("v1 is null");
                if (v2.IsNull())  throw NullShapeException("v2 is null");
                if (v3.IsNull())  throw NullShapeException("v3 is null");
                if (v4.IsNull())  throw NullShapeException("v4 is null");

                //assemble new wire

                //we want the connection order to be
                //v1 -> openWire1 -> v2 -> (new edge) -> v4 -> openWire2(rev) -> v3 -> (new edge) -> v1
                //let's check if it's the case. If not, we reverse one wire and swap its endpoints.

                if (fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v3)).Magnitude() - fabs(offset)) <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v3)){
                    openWire2.Reverse();
                    std::swap(v3, v4);
                    v3.Reverse();
                    v4.Reverse();
                }
                else if ((fabs(gp_Vec(BRep_Tool::Pnt(v2), BRep_Tool::Pnt(v4)).Magnitude() - fabs(offset)) <= BRep_Tool::Tolerance(v2) + BRep_Tool::Tolerance(v4))){
                    //orientation is as expected, nothing to do
                }
                else {
                    throw Base::CADKernelError("makeOffset2D: fill offset: failed to establish open vertex relationship.");
                }

                //now directions of open wires are aligned. Finally. make new wire!
                BRepBuilderAPI_MakeWire mkWire;
                //add openWire1
                BRepTools_WireExplorer it;
                for(it.Init(openWire1); it.More(); it.Next()){
                    mkWire.Add(it.Current());
                }
                //add first joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v2,v4).Edge());
                //add openWire2, in reverse order
                openWire2.Reverse();
                for(it.Init(TopoDS::Wire(openWire2)); it.More(); it.Next()){
                    mkWire.Add(it.Current());
                }
                //add final joining edge
                mkWire.Add(BRepBuilderAPI_MakeEdge(v3,v1).Edge());

                mkWire.Build();

                wiresForMakingFaces.push_front(mkWire.Wire());
            }
        }

        //make faces
        if (!wiresForMakingFaces.empty()){
            FaceMakerBullseye mkFace;
            mkFace.setPlane(workingPlane);
            for(TopoDS_Wire &w : wiresForMakingFaces){
                mkFace.addWire(w);
            }
            mkFace.Build();
            if (mkFace.Shape().IsNull())
                throw Base::CADKernelError("makeOffset2D: making face failed (null shape returned).");
            TopoDS_Shape result = mkFace.Shape();
            if (haveFaces && shapesToProcess.size() == 1)
                result.Orientation(shapesToProcess[0].Orientation());

            ShapeExtend_Explorer xp;
            Handle(TopTools_HSequenceOfShape) result_leaves = xp.SeqFromCompound(result, Standard_True);
            for(int i = 0; i < result_leaves->Length(); ++i)
                shapesToReturn.push_back(result_leaves->Value(i+1));
        }
    }

    //assemble output compound
    if (shapesToReturn.empty())
        return {}; //failure
    if (shapesToReturn.size() > 1 || forceOutputCompound){
        TopoDS_Compound result;
        BRep_Builder builder;
        builder.MakeCompound(result);
        for(TopoDS_Shape &sh : shapesToReturn) {
            if (!sh.IsNull()) {
                builder.Add(result, sh);
            }
        }
        return TopoDS_Shape(std::move(result));
    }
    else {
        return shapesToReturn[0];
    }
}

TopoDS_Shape TopoShape::makeThickSolid(const TopTools_ListOfShape& remFace,
                                       double offset, double tol, bool intersection,
                                       bool selfInter, short offsetMode, short join) const
{
    BRepOffsetAPI_MakeThickSolid mkThick;
    mkThick.MakeThickSolidByJoin(this->_Shape, remFace, offset, tol, BRepOffset_Mode(offsetMode),
        intersection ? Standard_True : Standard_False,
        selfInter ? Standard_True : Standard_False,
        GeomAbs_JoinType(join));
    return mkThick.Shape();
}

void TopoShape::transformGeometry(const Base::Matrix4D &rclMat)
{
    try {
        if (this->_Shape.IsNull())
            Standard_Failure::Raise("Cannot transform null shape");

        *this = makeGTransform(rclMat);
    }
    catch (const Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

TopoDS_Shape TopoShape::transformGShape(const Base::Matrix4D& rclTrf, bool copy) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot transform null shape");

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
    BRepBuilderAPI_GTransform mkTrf(this->_Shape, mat, copy);
    return mkTrf.Shape();
}

bool TopoShape::transformShape(const Base::Matrix4D& rclTrf, bool copy, bool checkScale)
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot transform null shape");

    return _makeTransform(TopoShape(*this),rclTrf,nullptr,checkScale,copy);
}

TopoDS_Shape TopoShape::mirror(const gp_Ax2& ax2) const
{
    gp_Trsf mat;
    mat.SetMirror(ax2);
    BRepBuilderAPI_Transform mkTrf(this->_Shape, mat);
    return mkTrf.Shape();
}

TopoDS_Shape TopoShape::toNurbs() const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Cannot convert null shape to NURBS");

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
    for (const auto & it : s)
        reshape.Remove(it);
    return reshape.Apply(this->_Shape, TopAbs_SHAPE);
}

void TopoShape::sewShape(double tolerance)
{
    BRepBuilderAPI_Sewing sew(tolerance);
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

        return TopoDS_Shape(std::move(comp));
    }

    return _Shape;
}

void TopoShape::getDomains(std::vector<Domain>& domains) const
{
    std::size_t countFaces = 0;
    for (TopExp_Explorer xp(this->_Shape, TopAbs_FACE); xp.More(); xp.Next()) {
        ++countFaces;
    }
    domains.reserve(countFaces);

    for (TopExp_Explorer xp(this->_Shape, TopAbs_FACE); xp.More(); xp.Next()) {
        TopoDS_Face face = TopoDS::Face(xp.Current());

        std::vector<gp_Pnt> points;
        std::vector<Poly_Triangle> facets;
        if (!Tools::getTriangulation(face, points, facets)) {
            // For a face that cannot be meshed append an empty domain.
            // It's important for some algorithms (e.g. color mapping) that the numbers of
            // faces and domains match
            Domain domain;
            domains.push_back(domain);
        }
        else {
            Domain domain;
            // copy the points
            domain.points.reserve(points.size());
            for (const auto& it : points) {
                Standard_Real X, Y, Z;
                it.Coord (X, Y, Z);
                domain.points.emplace_back(X, Y, Z);
            }

            // copy the triangles
            domain.facets.reserve(facets.size());
            for (const auto& it : facets) {
                Standard_Integer N1, N2, N3;
                it.Get(N1, N2, N3);

                Facet tria;
                tria.I1 = N1;
                tria.I2 = N2;
                tria.I3 = N3;
                domain.facets.push_back(tria);
            }

            domains.push_back(domain);
        }
    }
}

namespace Part {
struct MeshVertex
{
    Standard_Real x,y,z;
    Standard_Integer i;

    MeshVertex(Standard_Real X, Standard_Real Y, Standard_Real Z)
        : x(X),y(Y),z(Z),i(0)
    {
    }
    explicit MeshVertex(const Base::Vector3d& p)
        : x(p.x),y(p.y),z(p.z),i(0)
    {
    }

    Base::Vector3d toPoint() const
    { return Base::Vector3d(x,y,z); }

    bool operator < (const MeshVertex &v) const
    {
        if (fabs ( this->x - v.x) >= MESH_MIN_PT_DIST)
            return this->x < v.x;
        if (fabs ( this->y - v.y) >= MESH_MIN_PT_DIST)
            return this->y < v.y;
        if (fabs ( this->z - v.z) >= MESH_MIN_PT_DIST)
            return this->z < v.z;
        return false; // points are considered to be equal
    }

private:
    // use the same value as used inside the Mesh module
    static const double MESH_MIN_PT_DIST;
};
}

//const double Vertex::MESH_MIN_PT_DIST = 1.0e-6;
const double MeshVertex::MESH_MIN_PT_DIST = gp::Resolution();

void TopoShape::getFacesFromDomains(const std::vector<Domain>& domains,
                                    std::vector<Base::Vector3d>& points,
                                    std::vector<Facet>& faces) const
{
    std::set<MeshVertex> vertices;
    Standard_Real x1, y1, z1;
    Standard_Real x2, y2, z2;
    Standard_Real x3, y3, z3;

    for (const auto & domain : domains) {
        for (std::vector<Facet>::const_iterator jt = domain.facets.begin(); jt != domain.facets.end(); ++jt) {
            x1 = domain.points[jt->I1].x;
            y1 = domain.points[jt->I1].y;
            z1 = domain.points[jt->I1].z;

            x2 = domain.points[jt->I2].x;
            y2 = domain.points[jt->I2].y;
            z2 = domain.points[jt->I2].z;

            x3 = domain.points[jt->I3].x;
            y3 = domain.points[jt->I3].y;
            z3 = domain.points[jt->I3].z;

            TopoShape::Facet face;
            std::set<MeshVertex>::iterator vIt;

            // 1st vertex
            MeshVertex v1(x1,y1,z1);
            vIt = vertices.find(v1);
            if (vIt == vertices.end()) {
                v1.i = vertices.size();
                face.I1 = v1.i;
                vertices.insert(v1);
            }
            else {
                face.I1 = vIt->i;
            }

            // 2nd vertex
            MeshVertex v2(x2,y2,z2);
            vIt = vertices.find(v2);
            if (vIt == vertices.end()) {
                v2.i = vertices.size();
                face.I2 = v2.i;
                vertices.insert(v2);
            }
            else {
                face.I2 = vIt->i;
            }

            // 3rd vertex
            MeshVertex v3(x3,y3,z3);
            vIt = vertices.find(v3);
            if (vIt == vertices.end()) {
                v3.i = vertices.size();
                face.I3 = v3.i;
                vertices.insert(v3);
            }
            else {
                face.I3 = vIt->i;
            }

            // make sure that we don't insert invalid facets
            if (face.I1 != face.I2 &&
                face.I2 != face.I3 &&
                face.I3 != face.I1)
                faces.push_back(face);
        }
    }

    std::vector<Base::Vector3d> meshPoints;
    meshPoints.resize(vertices.size());
    for (const auto & vertex : vertices)
        meshPoints[vertex.i] = vertex.toPoint();
    points.swap(meshPoints);
}

double TopoShape::getAccuracy() const
{
    double deviation = 0.2;
    Base::BoundBox3d bbox = getBoundBox();
    if (bbox.IsValid())
        return ((bbox.LengthX() + bbox.LengthY() + bbox.LengthZ())/300.0 * deviation);
    return ComplexGeoData::getAccuracy();
}

void TopoShape::getFaces(std::vector<Base::Vector3d> &aPoints,
                         std::vector<Facet> &aTopo,
                         double accuracy, uint16_t /*flags*/) const
{
    if (this->_Shape.IsNull())
        return;

    // get the meshes of all faces and then merge them
    BRepMesh_IncrementalMesh aMesh(this->_Shape, accuracy,
                                   /*isRelative*/ Standard_False,
                                   /*theAngDeflection*/
                                   defaultAngularDeflection(accuracy),
                                   /*isInParallel*/ true);
    std::vector<Domain> domains;
    getDomains(domains);
    getFacesFromDomains(domains, aPoints, aTopo);
}

void TopoShape::setFaces(const std::vector<Base::Vector3d> &Points,
                         const std::vector<Facet> &Topo, double tolerance)
{
    gp_XYZ p1, p2, p3;
    std::vector<TopoDS_Vertex> Vertexes;
    std::map<std::pair<uint32_t, uint32_t>, TopoDS_Edge> Edges;
    TopoDS_Face newFace;
    TopoDS_Wire newWire;
    Standard_Real x1, y1, z1;
    Standard_Real x2, y2, z2;
    Standard_Real x3, y3, z3;

    TopoDS_Compound aComp;
    BRep_Builder BuildTool;
    BuildTool.MakeCompound(aComp);

    uint32_t ctPoints = Points.size();
    Vertexes.resize(ctPoints);

    // Create array of vertexes
    auto CreateVertex = [](const Base::Vector3d& v) {
        gp_XYZ p(v.x, v.y, v.z);
        return BRepBuilderAPI_MakeVertex(p);
    };
    for (const auto& it : Topo) {
        if (it.I1 < ctPoints) {
            if (Vertexes[it.I1].IsNull())
                Vertexes[it.I1] = CreateVertex(Points[it.I1]);
        }
        if (it.I2 < ctPoints) {
            if (Vertexes[it.I2].IsNull())
                Vertexes[it.I2] = CreateVertex(Points[it.I2]);
        }
        if (it.I3 < ctPoints) {
            if (Vertexes[it.I3].IsNull())
                Vertexes[it.I3] = CreateVertex(Points[it.I3]);
        }
    }

    // Create map of edges
    auto CreateEdge = [&Vertexes, &Edges](uint32_t p1, uint32_t p2) {
        // First check if the edge of a neighbour facet already exists
        // The point indices must be flipped.
        auto key1 = std::make_pair(p2, p1);
        auto key2 = std::make_pair(p1, p2);
        auto it = Edges.find(key1);
        if (it != Edges.end()) {
            TopoDS_Edge edge = it->second;
            edge.Reverse();
            Edges[key2] = edge;
        }
        else {
            BRepBuilderAPI_MakeEdge mkEdge(Vertexes[p1], Vertexes[p2]);
            if (mkEdge.IsDone())
                Edges[key2] = mkEdge.Edge();
        }
    };
    auto GetEdge = [&Edges](uint32_t p1, uint32_t p2) {
        auto key = std::make_pair(p1, p2);
        return Edges[key];
    };
    for (const auto& it : Topo) {
        CreateEdge(it.I1, it.I2);
        CreateEdge(it.I2, it.I3);
        CreateEdge(it.I3, it.I1);
    }

    for (const auto& it : Topo) {
        if (it.I1 >= ctPoints || it.I2 >= ctPoints || it.I3 >= ctPoints)
            continue;
        x1 = Points[it.I1].x; y1 = Points[it.I1].y; z1 = Points[it.I1].z;
        x2 = Points[it.I2].x; y2 = Points[it.I2].y; z2 = Points[it.I2].z;
        x3 = Points[it.I3].x; y3 = Points[it.I3].y; z3 = Points[it.I3].z;

        p1.SetCoord(x1,y1,z1);
        p2.SetCoord(x2,y2,z2);
        p3.SetCoord(x3,y3,z3);

        // Avoid very tiny edges as this may result into broken faces. The tolerance is Approximation
        // because Confusion might be too tight.
        if ((!(p1.IsEqual(p2, Precision::Approximation()))) && (!(p1.IsEqual(p3, Precision::Approximation())))) {
            const TopoDS_Edge& e1 = GetEdge(it.I1, it.I2);
            const TopoDS_Edge& e2 = GetEdge(it.I2, it.I3);
            const TopoDS_Edge& e3 = GetEdge(it.I3, it.I1);
            if (e1.IsNull() || e2.IsNull() || e3.IsNull())
                continue;

            newWire = BRepBuilderAPI_MakeWire(e1, e2, e3);
            if (!newWire.IsNull()) {
                newFace = BRepBuilderAPI_MakeFace(newWire);
                if (!newFace.IsNull())
                    BuildTool.Add(aComp, newFace);
            }
        }
    }

    // If performSewing is true BRepBuilderAPI_Sewing creates a compound of
    // shells. Since the resulting shape isn't very usable in most use cases
    // it's fine to set it to false so the algorithm only performs some control
    // checks and creates a compound of faces.
    // However, the computing time can be reduced by 90%.
    // If a shell is needed then the sewShape() function should be called explicitly.
    BRepBuilderAPI_Sewing aSewingTool;
    Standard_Boolean performSewing = Standard_False;
    aSewingTool.Init(tolerance, performSewing);
    aSewingTool.Load(aComp);

#if OCC_VERSION_HEX < 0x070500
    Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
    pi->NewScope(100, "Create shape from mesh...");
    pi->Show();

    aSewingTool.Perform(pi);
#else
    aSewingTool.Perform();
#endif

    _Shape = aSewingTool.SewedShape();
#if OCC_VERSION_HEX < 0x070500
    pi->EndScope();
#endif
    if (_Shape.IsNull())
        _Shape = aComp;
}

void TopoShape::getLinesFromSubShape(const TopoDS_Shape& shape,
                                     std::vector<Base::Vector3d> &vertices,
                                     std::vector<Line> &lines) const
{
    if (shape.IsNull())
        return;

    // build up map edge->face
    TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
    TopExp::MapShapesAndAncestors(this->_Shape, TopAbs_EDGE, TopAbs_FACE, edge2Face);

    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge aEdge = TopoDS::Edge(exp.Current());
        std::vector<gp_Pnt> points;

        if (!Tools::getPolygon3D(aEdge, points)) {
            // the edge has not its own triangulation, but then a face the edge is attached to
            // must provide this triangulation

            // Look for one face in our map (it doesn't care which one we take)
            int index = edge2Face.FindIndex(aEdge);
            if (index < 1)
                continue;

            const auto &faces = edge2Face.FindFromIndex(index);
            if (faces.IsEmpty())
                continue;

            const TopoDS_Face& aFace = TopoDS::Face(faces.First());
            if (!Part::Tools::getPolygonOnTriangulation(aEdge, aFace, points))
                continue;
        }

        auto line_start = vertices.size();
        vertices.reserve(vertices.size() + points.size());
        std::for_each(points.begin(), points.end(), [&vertices](const gp_Pnt& p) {
            vertices.push_back(Base::convertTo<Base::Vector3d>(p));
        });

        if (line_start+1 < vertices.size()) {
            lines.emplace_back();
            lines.back().I1 = line_start;
            lines.back().I2 = vertices.size()-1;
        }
    }
}

void TopoShape::getLines(std::vector<Base::Vector3d> &vertices,
                         std::vector<TopoShape::Line> &lines,
                         double /*Accuracy*/, uint16_t /*flags*/) const
{
    getLinesFromSubShape(_Shape, vertices, lines);
}

void TopoShape::getPoints(std::vector<Base::Vector3d> &Points,
                          std::vector<Base::Vector3d> &Normals,
                          double Accuracy, uint16_t /*flags*/) const
{
    if (_Shape.IsNull())
        return;

    const int minPointsPerEdge = 30;
    const double lateralDistance = Accuracy;

    // get all 3d points from free vertices
    for (TopExp_Explorer xp(_Shape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
        gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        Points.push_back(Base::convertTo<Base::Vector3d>(p));
        Normals.emplace_back(0,0,0);
    }

    // sample inner points of all free edges
    for (TopExp_Explorer xp(_Shape, TopAbs_EDGE, TopAbs_FACE); xp.More(); xp.Next()) {
        BRepAdaptor_Curve curve(TopoDS::Edge(xp.Current()));
        GCPnts_UniformAbscissa discretizer(curve, lateralDistance, curve.FirstParameter(), curve.LastParameter());
        if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
            int nbPoints = discretizer.NbPoints();
            for (int i=1; i<=nbPoints; i++) {
                gp_Pnt p = curve.Value (discretizer.Parameter(i));
                Points.push_back(Base::convertTo<Base::Vector3d>(p));
                Normals.emplace_back(0,0,0);
            }
        }
    }

    // sample inner points of all faces
    BRepClass_FaceClassifier classifier;
    bool hasFaces = false;
    for (TopExp_Explorer xp(_Shape, TopAbs_FACE); xp.More(); xp.Next()) {
        hasFaces = true;
        int pointsPerEdge = minPointsPerEdge;
        TopoDS_Face face = TopoDS::Face(xp.Current());
        BRepAdaptor_Surface surface(face);
        Handle(Geom_Surface) aSurf = BRep_Tool::Surface(face);

        // parameter ranges
        Standard_Real uFirst = surface.FirstUParameter();
        Standard_Real uLast = surface.LastUParameter();
        Standard_Real uMid = (uFirst+uLast)/2;
        Standard_Real vFirst = surface.FirstVParameter();
        Standard_Real vLast = surface.LastVParameter();
        Standard_Real vMid = (vFirst+vLast)/2;

        // get geometrical length and width of the surface
        //
        gp_Pnt p1, p2;
        Standard_Real fLengthU = 0.0, fLengthV = 0.0;
        for (int i = 1; i <= pointsPerEdge; i++) {
            double u1 = static_cast<double>(i-1)/static_cast<double>(pointsPerEdge);
            double s1 = (1.0-u1)*uFirst + u1*uLast;
            p1 = surface.Value(s1,vMid);

            double u2 = static_cast<double>(i)/static_cast<double>(pointsPerEdge);
            double s2 = (1.0-u2)*uFirst + u2*uLast;
            p2 = surface.Value(s2,vMid);

            fLengthU += p1.Distance(p2);
        }

        for (int i = 1; i <= pointsPerEdge; i++) {
            double v1 = static_cast<double>(i-1)/static_cast<double>(pointsPerEdge);
            double t1 = (1.0-v1)*vFirst + v1*vLast;
            p1 = surface.Value(uMid,t1);

            double v2 = static_cast<double>(i)/static_cast<double>(pointsPerEdge);
            double t2 = (1.0-v2)*vFirst + v2*vLast;
            p2 = surface.Value(uMid,t2);

            fLengthV += p1.Distance(p2);
        }

        int uPointsPerEdge = static_cast<int>(fLengthU / lateralDistance);
        int vPointsPerEdge = static_cast<int>(fLengthV / lateralDistance);
        uPointsPerEdge = std::max(uPointsPerEdge, 1);
        vPointsPerEdge = std::max(vPointsPerEdge, 1);

        for (int i = 0; i <= uPointsPerEdge; i++) {
            double u = static_cast<double>(i)/static_cast<double>(uPointsPerEdge);
            double s = (1.0-u)*uFirst + u*uLast;

            for (int j = 0; j <= vPointsPerEdge; j++) {
                double v = static_cast<double>(j)/static_cast<double>(vPointsPerEdge);
                double t = (1.0-v)*vFirst + v*vLast;

                gp_Pnt2d p2d(s,t);
                classifier.Perform(face,p2d,1.0e-4);
                if (classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON) {
                    gp_Pnt p = surface.Value(s,t);
                    Points.push_back(Base::convertTo<Base::Vector3d>(p));
                    gp_Dir normal;
                    if (GeomLib::NormEstim(aSurf, p2d, Precision::Confusion(), normal) <= 1) {
                        if (face.Orientation() == TopAbs_REVERSED)
                            normal.Reverse();
                        Normals.push_back(Base::convertTo<Base::Vector3d>(normal));
                    }
                    else {
                        Normals.emplace_back(0,0,0);
                    }
                }
            }
        }
    }

    // if no faces are found then the normals can be cleared
    if (!hasFaces)
        Normals.clear();
}

void TopoShape::getLinesFromSubElement(const Data::Segment* element,
                                       std::vector<Base::Vector3d> &vertices,
                                       std::vector<Line> &lines) const
{
    if (element->getTypeId() == ShapeSegment::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const ShapeSegment*>(element)->Shape;
        if (shape.IsNull())
            return;

        getLinesFromSubShape(shape, vertices, lines);
    }
}

void TopoShape::getFacesFromSubElement(const Data::Segment* element,
                                       std::vector<Base::Vector3d> &points,
                                       std::vector<Base::Vector3d> &pointNormals,
                                       std::vector<Facet> &faces) const
{
    if (element->getTypeId() == ShapeSegment::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const ShapeSegment*>(element)->Shape;
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
            return;

        // get the meshes of all faces and then merge them
        std::vector<Domain> domains;
        TopoShape(shape).getDomains(domains);
        getFacesFromDomains(domains, points, faces);

        (void)pointNormals; // leave this empty
    }
}

TopoDS_Shape TopoShape::defeaturing(const std::vector<TopoDS_Shape>& s) const
{
    if (this->_Shape.IsNull())
        Standard_Failure::Raise("Base shape is null");
    BRepAlgoAPI_Defeaturing defeat;
    defeat.SetRunParallel(true);
    defeat.SetShape(this->_Shape);
    for (const auto & it : s)
        defeat.AddFaceToRemove(it);
    defeat.Build();
    if (!defeat.IsDone()) {
        // error treatment
        Standard_SStream aSStream;
        defeat.DumpErrors(aSStream);
        const std::string& resultstr = aSStream.str();
        const char* cstr2 = resultstr.c_str();
        throw Base::RuntimeError(cstr2);
    }
    return defeat.Shape();
}

/**
 * @brief TopoShape::makeShell
 * If the input shape is a compound with faces not being part of a shell
 * it tries to make a shell.
 * If this operation fails or if the input shape is not a compound or a compound
 * with not only faces the input shape is returned.
 * @return Shell or passed shape
 */
TopoDS_Shape TopoShape::makeShell(const TopoDS_Shape& input) const
{
    // For comparison see also:
    // GEOMImpl_BooleanDriver::makeCompoundShellFromFaces
    if (input.IsNull())
        return input;
    if (input.ShapeType() != TopAbs_COMPOUND)
        return input;

    // we need a compound that consists of only faces
    TopExp_Explorer it;
    // no shells
    it.Init(input, TopAbs_SHELL);
    if (it.More())
        return input;

    // no wires outside a face
    it.Init(input, TopAbs_WIRE, TopAbs_FACE);
    if (it.More())
        return input;

    // no edges outside a wire
    it.Init(input, TopAbs_EDGE, TopAbs_WIRE);
    if (it.More())
        return input;

    // no vertexes outside an edge
    it.Init(input, TopAbs_VERTEX, TopAbs_EDGE);
    if (it.More())
        return input;

    BRep_Builder builder;
    TopoDS_Shape shape;
    TopoDS_Shell shell;
    builder.MakeShell(shell);

    try {
        for (it.Init(input, TopAbs_FACE); it.More(); it.Next()) {
            if (!it.Current().IsNull())
                builder.Add(shell, it.Current());
        }

        shape = shell;
        BRepCheck_Analyzer check(shell);
        if (!check.IsValid()) {
            ShapeUpgrade_ShellSewing sewShell;
            shape = sewShell.ApplySewing(shell);
        }

        if (shape.IsNull())
            return input;

        if (shape.ShapeType() != TopAbs_SHELL)
            return input;

        return shape; // success
    }
    catch (Standard_Failure&) {
        return input;
    }
}

#define _HANDLE_NULL_SHAPE(_msg,_throw) do {\
    if(_throw) {\
        FC_THROWM(NullShapeException,_msg);\
    }\
    FC_WARN(_msg);\
}while(0)

#define HANDLE_NULL_SHAPE _HANDLE_NULL_SHAPE("Null shape",true)
#define HANDLE_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",true)
#define WARN_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",false)

TopoShape &TopoShape::makeWires(const TopoShape &shape, const char *op, bool fix, double tol)
{
    _Shape.Nullify();

    if(shape.isNull())
        HANDLE_NULL_INPUT;

    if(tol<Precision::Confusion()) tol = Precision::Confusion();

    (void)op;
    (void)fix;
    std::vector<TopoShape> edges;
    std::list<TopoShape> edge_list;
    std::vector<TopoShape> wires;

    TopTools_IndexedMapOfShape anIndices;
    TopExp::MapShapes(shape.getShape(), TopAbs_EDGE, anIndices);
    for(int i=1;i<=anIndices.Extent();++i)
        edge_list.emplace_back(anIndices.FindKey(i));

    edges.reserve(edge_list.size());
    wires.reserve(edge_list.size());

    // sort them together to wires
    while (!edge_list.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        edges.push_back(edge_list.front());
        edge_list.pop_front();
        mkWire.Add(TopoDS::Edge(edges.back().getShape()));
        edges.back().setShape(mkWire.Edge());

        TopoDS_Wire new_wire = mkWire.Wire(); // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are connectible
        bool found = false;
        do {
            found = false;
            for (auto it=edge_list.begin();it!=edge_list.end();++it) {
                mkWire.Add(TopoDS::Edge(it->getShape()));
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edges.push_back(*it);
                    edges.back().setShape(mkWire.Edge());
                    edge_list.erase(it);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        } while (found);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(tol);
        aFix.Load(new_wire);

        aFix.FixReorder();
        // Assuming FixReorder() just reorder and don't change the underlying
        // edges, we get the wire and do a name mapping now, as the following
        // two operations (FixConnected and FixClosed) may change the edges.
        wires.emplace_back(aFix.Wire());

        aFix.FixConnected();
        aFix.FixClosed();
        // Now retrieve the shape and set it without touching element map
        wires.back().setShape(aFix.Wire());
    }
    return makeCompound(wires,nullptr,false);
}

TopoShape &TopoShape::makeCompound(const std::vector<TopoShape> &shapes, const char *op, bool force)
{
    (void)op;
    _Shape.Nullify();

    if(shapes.empty())
        HANDLE_NULL_INPUT;

    if(!force && shapes.size()==1) {
        *this = shapes[0];
        return *this;
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    int count = 0;
    for(auto &s : shapes) {
        if(s.isNull()) {
            WARN_NULL_INPUT;
            continue;
        }
        builder.Add(comp,s.getShape());
        ++count;
    }
    if(!count)
        HANDLE_NULL_SHAPE;
    _Shape = comp;
    return *this;
}

TopoShape &TopoShape::makeFace(const TopoShape &shape, const char *op, const char *maker)
{
    std::vector<TopoShape> shapes;
    if(shape.shapeType() == TopAbs_COMPOUND) {
        for(TopoDS_Iterator it(shape.getShape());it.More();it.Next())
            shapes.emplace_back(it.Value());
    } else
        shapes.push_back(shape);
    return makeFace(shapes,op,maker);
}

TopoShape &TopoShape::makeFace(const std::vector<TopoShape> &shapes, const char *op, const char *maker)
{
    (void)op;
    _Shape.Nullify();

    if(!maker || !maker[0])
        maker = "Part::FaceMakerBullseye";

    std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(maker);
    for(auto &s : shapes) {
        if (s.getShape().ShapeType() == TopAbs_COMPOUND)
            mkFace->useCompound(TopoDS::Compound(s.getShape()));
        else if (s.getShape().ShapeType() != TopAbs_VERTEX)
            mkFace->addShape(s.getShape());
    }
    mkFace->Build();
    _Shape = mkFace->Shape();
    return *this;
}

TopoShape &TopoShape::makeRefine(const TopoShape &shape, const char *op, bool no_fail) {
    (void)op;
    _Shape.Nullify();
    if(shape.isNull()) {
        if(!no_fail)
            HANDLE_NULL_SHAPE;
        return *this;
    }
    try {
        BRepBuilderAPI_RefineModel mkRefine(shape.getShape());
        _Shape = mkRefine.Shape();
        return *this;
    }catch (Standard_Failure &) {
        if(!no_fail) throw;
    }
    *this = shape;
    return *this;
}

bool TopoShape::findPlane(gp_Pln &pln, double tol) const {
    if(_Shape.IsNull())
        return false;
    TopoDS_Shape shape = _Shape;
    TopExp_Explorer exp(_Shape,TopAbs_EDGE);
    if(exp.More()) {
        TopoDS_Shape edge = exp.Current();
        exp.Next();
        if (!exp.More()) {
            // To deal with OCCT bug of wrong edge transformation
            shape = BRepBuilderAPI_Copy(_Shape).Shape();
        }
    }
    try {
        BRepLib_FindSurface finder(shape,tol,Standard_True);
        if (!finder.Found())
            return false;
        pln = GeomAdaptor_Surface(finder.Surface()).Plane();

        // To make the returned plane normal more stable, if the shape has any
        // face, use the normal of the first face.
        TopExp_Explorer exp(shape, TopAbs_FACE);
        if(exp.More()) {
            BRepAdaptor_Surface adapt(TopoDS::Face(exp.Current()));
            double u = adapt.FirstUParameter()
                + (adapt.LastUParameter() - adapt.FirstUParameter())/2.;
            double v = adapt.FirstVParameter()
                + (adapt.LastVParameter() - adapt.FirstVParameter())/2.;
            BRepLProp_SLProps prop(adapt,u,v,2,Precision::Confusion());
            if(prop.IsNormalDefined()) {
                gp_Pnt pnt; gp_Vec vec;
                // handles the orientation state of the shape
                BRepGProp_Face(TopoDS::Face(exp.Current())).Normal(u,v,pnt,vec);
                pln = gp_Pln(pnt, gp_Dir(vec));
            }
        }
        return true;
    }catch (Standard_Failure &e) {
        // For some reason the above BRepBuilderAPI_Copy failed to copy
        // the geometry of some edge, causing exception with message
        // BRepAdaptor_Curve::No geometry. However, without the above
        // copy, circular edges often have the wrong transformation!
        FC_LOG("failed to find surface: " << e.GetMessageString());
        return false;
    }
}

bool TopoShape::isInfinite() const
{
    if (_Shape.IsNull())
        return false;

    try {
        // If the shape is empty an exception may be thrown
        Bnd_Box bounds;
        BRepBndLib::Add(_Shape, bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        if (Precision::IsInfinite(xMax - xMin))
            return true;
        if (Precision::IsInfinite(yMax - yMin))
            return true;
        if (Precision::IsInfinite(zMax - zMin))
            return true;

        return false;
    }
    catch (Standard_Failure&) {
        return false;
    }
}

bool TopoShape::isPlanar(double tol) const
{
    if (_Shape.IsNull() || _Shape.ShapeType() != TopAbs_FACE) {
        return false;
    }

    BRepAdaptor_Surface adapt(TopoDS::Face(_Shape));
    if (adapt.GetType() == GeomAbs_Plane) {
        return true;
    }

    TopLoc_Location loc;
    Handle(Geom_Surface) surf = BRep_Tool::Surface(TopoDS::Face(_Shape), loc);
    if (surf.IsNull()) {
        return false;
    }

    GeomLib_IsPlanarSurface check(surf, tol);
    return check.IsPlanar();
}

bool TopoShape::isCoplanar(const TopoShape &other, double tol) const {
    if(isNull() || other.isNull())
        return false;
    if(_Shape.IsEqual(other._Shape))
        return true;
    gp_Pln pln1,pln2;
    if(!findPlane(pln1,tol) || !other.findPlane(pln2,tol))
        return false;
    if(tol<0.0)
        tol = Precision::Confusion();
    return pln1.Position().IsCoplanar(pln2.Position(),tol,tol);
}

bool TopoShape::_makeTransform(const TopoShape &shape,
        const Base::Matrix4D &rclTrf, const char *op, bool checkScale, bool copy)
{
    if(checkScale) {
        try {
            auto type = rclTrf.hasScale();
            if (type != Base::ScaleType::Uniform && type != Base::ScaleType::NoScaling) {
                makeGTransform(shape,rclTrf,op,copy);
                return true;
            }
        }
        catch (const Standard_Failure& e) {
            Base::Console().Warning("TopoShape::makeGTransform failed: %s\n", e.GetMessageString());
        }
    }
    makeTransform(shape,convert(rclTrf),op,copy);
    return false;
}

TopoShape &TopoShape::makeTransform(const TopoShape &shape, const gp_Trsf &trsf, const char *op, bool copy) {
    // resetElementMap();

    if(!copy) {
        // OCCT checks the ScaleFactor against gp::Resolution() which is DBL_MIN!!!
        copy = trsf.ScaleFactor()*trsf.HVectorialPart().Determinant() < 0. ||
               Abs(Abs(trsf.ScaleFactor()) - 1) > Precision::Confusion();
    }
    TopoShape tmp(shape);
    if(copy) {
        BRepBuilderAPI_Transform mkTrf(shape.getShape(), trsf, Standard_True);
        // TODO: calling Moved() is to make sure the shape has some Location,
        // which is necessary for STEP export to work. However, if we reach
        // here, it porabably means BRepBuilderAPI_Transform has modified
        // underlying shapes (because of scaling), it will break compound child
        // parent relationship anyway. In short, STEP import/export will most
        // likely break badly if there is any scaling involved
        tmp._Shape = mkTrf.Shape().Moved(gp_Trsf());
    }else
        tmp._Shape.Move(trsf);
    if(op || (shape.Tag && shape.Tag!=Tag)) {
        _Shape = tmp._Shape;
        // tmp.initCache(1);
        // mapSubElement(tmp,op);
    } else
        *this = tmp;
    return *this;
}

TopoShape &TopoShape::makeGTransform(const TopoShape &shape, const Base::Matrix4D &rclTrf, const char *op, bool copy)
{
    boost::ignore_unused(op);
    _Shape = shape.transformGShape(rclTrf, copy);
    return *this;
}
