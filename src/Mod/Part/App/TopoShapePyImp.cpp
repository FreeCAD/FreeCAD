/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <boost/regex.hpp>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <BRepOffsetAPI_MakePipe.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepProj_Projection.hxx>
# include <BRepTools.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepExtrema_ShapeProximity.hxx>
# include <BRepExtrema_SupportType.hxx>
# include <BRepBndLib.hxx>
# include <BRep_Tool.hxx>
# include <gp_Ax1.hxx>
# include <gp_Ax2.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Trsf.hxx>
# include <gp_Pln.hxx>
# include <Poly_Polygon3D.hxx>
# include <Poly_Triangulation.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopLoc_Location.hxx>
# include <TopExp.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <HLRAppli_ReflectLines.hxx>
# include <BRepGProp.hxx>
# include <GProp_GProps.hxx>
# include <BRepAlgo_NormalProjection.hxx>
# include <ShapeAnalysis_ShapeTolerance.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>
#include <Base/MatrixPy.h>
#include <Base/Stream.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <App/MappedElement.h>
#include <App/PropertyStandard.h>
#include <App/StringHasherPy.h>
#include <CXX/Extensions.hxx>

#include "TopoShape.h"
#include "TopoShapeOpCode.h"
#include "PartPyCXX.h"
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapePy.cpp>

#include "OCCError.h"
#include <Mod/Part/App/GeometryPy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Part/App/TopoShapeVertexPy.h>
#include <Mod/Part/App/TopoShapeSolidPy.h>
#include <Mod/Part/App/TopoShapeShellPy.h>
#include <Mod/Part/App/TopoShapeCompSolidPy.h>
#include <Mod/Part/App/TopoShapeCompoundPy.h>
#include <Mod/Part/App/PlanePy.h>

using namespace Part;

#ifndef M_PI
    #define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
    #define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

static Py_hash_t _TopoShapeHash(PyObject *self) {
    if (!self) {
        PyErr_SetString(PyExc_TypeError, "descriptor 'hash' of 'Part.TopoShape' object needs an argument");
        return 0;
    }
    if (!static_cast<Base::PyObjectBase*>(self)->isValid()) {
        PyErr_SetString(PyExc_ReferenceError, "This object is already deleted most likely through closing a document. This reference is no longer valid!");
        return 0;
    }
    return static_cast<TopoShapePy*>(self)->getTopoShapePtr()->getShape().HashCode(INT_MAX);
}

struct TopoShapePyInit {
    TopoShapePyInit() {
        TopoShapePy::Type.tp_hash = _TopoShapeHash;
    }
} _TopoShapePyInit;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapePy::representation(void) const
{
    std::stringstream str;
    str << "<Shape object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapePy and the Twin object
    return new TopoShapePy(new TopoShape);
}

int TopoShapePy::PyInit(PyObject* args, PyObject *keywds)
{
#ifndef FC_NO_ELEMENT_MAP
    static char *kwlist[] = {"shape", "op", "tag", "hasher", nullptr};
    long tag = 0;
    PyObject *pyHasher = nullptr;
    const char *op = nullptr;
    PyObject *pcObj = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OsiO!", 
                kwlist,&pcObj,&op,&tag,&App::StringHasherPy::Type,&pyHasher))
        return -1;
    auto &self = *getTopoShapePtr();
    self.Tag = tag;
    if(pyHasher)
        self.Hasher = static_cast<App::StringHasherPy*>(pyHasher)->getStringHasherPtr();
    auto shapes = getPyShapes(pcObj);
    PY_TRY {
        if(shapes.size()==1 && !op) {
            auto s = shapes.front();
            if(self.Tag) {
                if((s.Tag && self.Tag!=s.Tag)
                        || (self.Hasher 
                            && s.getElementMapSize() 
                            && self.Hasher!=s.Hasher))
                {
                    s.reTagElementMap(self.Tag,self.Hasher);
                }else{
                    s.Tag = self.Tag;
                    s.Hasher = self.Hasher;
                }
            }
            self = s;
        }else if(shapes.size()) {
            if(!op) op = Part::OpCodes::Fuse;
            self.makEBoolean(op,shapes);
        }
    }_PY_CATCH_OCC(return(-1))
#else
    PyObject *pcObj=nullptr;
    if (!PyArg_ParseTuple(args, "|O", &pcObj))
        return -1;

    if (pcObj) {
        TopoShape shape;
        PY_TRY {
            if(PyObject_TypeCheck(pcObj,&TopoShapePy::Type)) {
                shape = *static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
            }
            else {
                Py::Sequence list(pcObj);
                bool first = true;
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryPy::Type))) {
                        TopoDS_Shape sh = static_cast<GeometryPy*>((*it).ptr())->
                            getGeometryPtr()->toShape();
                        if (first) {
                            first = false;
                            shape.setShape(sh);
                        }
                        else {
                            shape.setShape(shape.fuse(sh));
                        }
                    }
                }
            }
        }
        _PY_CATCH_OCC(return(-1))

        getTopoShapePtr()->setShape(shape.getShape());
    }
#endif

    return 0;
}

PyObject* TopoShapePy::copy(PyObject *args)
{
    PyObject* copyGeom = Py_True;
    PyObject* copyMesh = Py_False;

#ifndef FC_NO_ELEMENT_MAP
    const char *op = nullptr;
    PyObject *pyHasher = nullptr;
    if (!PyArg_ParseTuple(args, "|sO!O!O!", &op,&App::StringHasherPy::Type,&pyHasher,
                &PyBool_Type,&copyGeom,&PyBool_Type,&copyMesh)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &copyGeom, &PyBool_Type, &copyMesh))
            return 0;
    }
    if(op && !op[0]) op = nullptr;
    App::StringHasherRef hasher;
    if(pyHasher)
        hasher = static_cast<App::StringHasherPy*>(pyHasher)->getStringHasherPtr();
    auto &self = *getTopoShapePtr();
    return Py::new_reference_to(shape2pyshape(
                TopoShape(self.Tag,hasher).makECopy(
                    self,op,PyObject_IsTrue(copyGeom),PyObject_IsTrue(copyMesh))));
#else
    PyObject* copyGeom = Py_True;
    PyObject* copyMesh = Py_False;
    if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &copyGeom, &PyBool_Type, &copyMesh))
        return nullptr;

    const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return nullptr;
    }

    if (!shape.IsNull()) {
        BRepBuilderAPI_Copy c(shape,
                              PyObject_IsTrue(copyGeom) ? Standard_True : Standard_False,
                              PyObject_IsTrue(copyMesh) ? Standard_True : Standard_False);
        static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->setShape(c.Shape());
    }
    return cpy;
#endif
}

PyObject* TopoShapePy::cleaned(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
#ifndef FC_NO_ELEMENT_MAP
    auto &self = *getTopoShapePtr();
    TopoShape copy(self.makECopy());
    if (!copy.isNull())
        BRepTools::Clean(copy.getShape()); // remove triangulation
    return Py::new_reference_to(shape2pyshape(copy));
#else

    const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return nullptr;
    }

    if (!shape.IsNull()) {
        BRepBuilderAPI_Copy c(shape);
        const TopoDS_Shape& copiedShape = c.Shape();
        BRepTools::Clean(copiedShape); // remove triangulation
        static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->setShape(c.Shape());
    }
    return cpy;
#endif
}

PyObject* TopoShapePy::replaceShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return nullptr;

    try {
#ifndef FC_NO_ELEMENT_MAP
        Py::Sequence list(l);
        std::vector< std::pair<TopoShape, TopoShape> > shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Tuple tuple(*it);
            Py::TopoShape sh1(tuple[0]);
            Py::TopoShape sh2(tuple[1]);
            shapes.push_back(std::make_pair(
                *sh1.extensionObject()->getTopoShapePtr(),
                *sh2.extensionObject()->getTopoShapePtr())
            );
        }
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->replacEShape(shapes)));
#else
        Py::Sequence list(l);
        std::vector< std::pair<TopoDS_Shape, TopoDS_Shape> > shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Tuple tuple(*it);
            Py::TopoShape sh1(tuple[0]);
            Py::TopoShape sh2(tuple[1]);
            shapes.emplace_back(
                sh1.extensionObject()->getTopoShapePtr()->getShape(),
                sh2.extensionObject()->getTopoShapePtr()->getShape()
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, nullptr);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->replaceShape(shapes));
        return inst;
#endif
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::removeShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return nullptr;

    try {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->removEShape(getPyShapes(l))));
#else
        Py::Sequence list(l);
        std::vector<TopoDS_Shape> shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::TopoShape sh(*it);
            shapes.push_back(sh.extensionObject()->getTopoShapePtr()->getShape());
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, nullptr);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->removeShape(shapes));
        return inst;
#endif
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::read(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return nullptr;

    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    getTopoShapePtr()->read(EncodedName.c_str());
    return IncRef();
}

PyObject* TopoShapePy::writeInventor(PyObject * args, PyObject * keywds)
{
    static char *kwlist[] = {"Mode", "Deviation", "Angle", "FaceColors", nullptr};

    double dev=0.3, angle=0.4;
    int mode=2;
    PyObject* pylist=nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iddO", kwlist,
                                     &mode,&dev,&angle,&pylist))
        return nullptr;

    std::vector<App::Color> faceColors;
    if (pylist) {
        App::PropertyColorList prop;
        prop.setPyObject(pylist);
        faceColors = prop.getValues();
    }

    std::stringstream result;
    BRepMesh_IncrementalMesh(getTopoShapePtr()->getShape(),dev);
    if (mode == 0) {
        getTopoShapePtr()->exportFaceSet(dev, angle, faceColors, result);
    }
    else if (mode == 1) {
        getTopoShapePtr()->exportLineSet(result);
    }
    else {
        getTopoShapePtr()->exportFaceSet(dev, angle, faceColors, result);
        getTopoShapePtr()->exportLineSet(result);
    }
    // NOTE: Cleaning the triangulation may cause problems on some algorithms like BOP
    //BRepTools::Clean(getTopoShapePtr()->getShape()); // remove triangulation
    return Py::new_reference_to(Py::String(result.str()));
}

PyObject*  TopoShapePy::exportIges(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return nullptr;

    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write iges file
        getTopoShapePtr()->exportIges(EncodedName.c_str());
    } PY_CATCH_OCC

    Py_Return;
}

PyObject*  TopoShapePy::exportStep(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return nullptr;

    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write step file
        getTopoShapePtr()->exportStep(EncodedName.c_str());
    } PY_CATCH_OCC

    Py_Return;
}

PyObject*  TopoShapePy::exportBrep(PyObject *args)
{
    char* Name;
    if (PyArg_ParseTuple(args, "et","utf-8",&Name)) {
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            // write brep file
            getTopoShapePtr()->exportBrep(EncodedName.c_str());
        } PY_CATCH_OCC

        Py_Return;
    }

    PyErr_Clear();

    PyObject* input;
    if (PyArg_ParseTuple(args, "O", &input)) {
        try {
            // write brep
            Base::PyStreambuf buf(input);
            std::ostream str(nullptr);
            str.rdbuf(&buf);
            getTopoShapePtr()->exportBrep(str);
        } PY_CATCH_OCC

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return nullptr;
}

PyObject*  TopoShapePy::exportBinary(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return nullptr;

    try {
        // read binary brep
        std::ofstream str(input, std::ios::out | std::ios::binary);
        getTopoShapePtr()->exportBinary(str);
        str.close();
    } PY_CATCH_OCC

    Py_Return;
}

PyObject*  TopoShapePy::dumpToString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        std::stringstream str;
        getTopoShapePtr()->dump(str);
        return Py::new_reference_to(Py::String(str.str()));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::exportBrepToString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        // write brep file
        std::stringstream str;
        getTopoShapePtr()->exportBrep(str);
        return Py::new_reference_to(Py::String(str.str()));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::importBrep(PyObject *args)
{
    char* Name;
    if (PyArg_ParseTuple(args, "et","utf-8",&Name)) {
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            // write brep file
            getTopoShapePtr()->importBrep(EncodedName.c_str());
        } PY_CATCH_OCC

        Py_Return;
    }

    PyErr_Clear();
    PyObject* input;
    if (PyArg_ParseTuple(args, "O", &input)) {
        try {
            // read brep
            Base::PyStreambuf buf(input);
            std::istream str(nullptr);
            str.rdbuf(&buf);
            getTopoShapePtr()->importBrep(str);
        } PY_CATCH_OCC

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return nullptr;
}

PyObject*  TopoShapePy::importBinary(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return nullptr;

    try {
        // read binary brep
        std::ifstream str(input, std::ios::in | std::ios::binary);
        getTopoShapePtr()->importBinary(str);
        str.close();
    } PY_CATCH_OCC

    Py_Return;
}

PyObject*  TopoShapePy::importBrepFromString(PyObject *args)
{
    char* input;
    int indicator=1;
    if (!PyArg_ParseTuple(args, "s|i", &input, &indicator))
        return nullptr;

    try {
        // read brep
        std::stringstream str(input);
        getTopoShapePtr()->importBrep(str,indicator);
    } PY_CATCH_OCC

    Py_Return;
}

PyObject*  TopoShapePy::__getstate__(PyObject *args) {
    return exportBrepToString(args);
}


PyObject*  TopoShapePy::__setstate__(PyObject *args) {
    if (! getTopoShapePtr()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError,"no c++ object");
        return nullptr;
    }
    else {
        return importBrepFromString(args);
    }
}

PyObject*  TopoShapePy::exportStl(PyObject *args)
{
    double deflection = 0.01;
    char* Name;
    if (!PyArg_ParseTuple(args, "et|d","utf-8",&Name,&deflection))
        return nullptr;

    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write stl file
        getTopoShapePtr()->exportStl(EncodedName.c_str(), deflection);
    } PY_CATCH_OCC

    Py_Return;
}

PyObject* TopoShapePy::extrude(PyObject *args)
{
    PyObject *pVec;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pVec))
        return nullptr;

    try {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(pVec)->value();
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEPrism(gp_Vec(vec.x,vec.y,vec.z))));
#else
        TopoDS_Shape shape = this->getTopoShapePtr()->makePrism(gp_Vec(vec.x,vec.y,vec.z));
        TopAbs_ShapeEnum type = shape.ShapeType();
        switch (type) {
            case TopAbs_COMPOUND:
                return new TopoShapeCompoundPy(new TopoShape(shape));
            case TopAbs_COMPSOLID:
                return new TopoShapeCompSolidPy(new TopoShape(shape));
            case TopAbs_SOLID:
                return new TopoShapeSolidPy(new TopoShape(shape));
            case TopAbs_SHELL:
                return new TopoShapeShellPy(new TopoShape(shape));
            case TopAbs_FACE:
                return new TopoShapeFacePy(new TopoShape(shape));
            case TopAbs_WIRE:
                break;
            case TopAbs_EDGE:
                return new TopoShapeEdgePy(new TopoShape(shape));
            case TopAbs_VERTEX:
                break;
            case TopAbs_SHAPE:
                break;
            default:
                break;
        }

        PyErr_SetString(PartExceptionOCCError, "extrusion for this shape type not supported");
        return nullptr;
#endif
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::revolve(PyObject *args)
{
    PyObject *pPos,*pDir;
    double d=360;
    if (!PyArg_ParseTuple(args, "O!O!|d", &(Base::VectorPy::Type), &pPos, &(Base::VectorPy::Type), &pDir,&d))
        return nullptr;
    Base::Vector3d pos = static_cast<Base::VectorPy*>(pPos)->value();
    Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
    try {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makERevolve(
            gp_Ax1(gp_Pnt(pos.x,pos.y,pos.z), gp_Dir(dir.x,dir.y,dir.z)),d*(M_PI/180))));
#else
        const TopoDS_Shape& input = this->getTopoShapePtr()->getShape();
        if (input.IsNull()) {
            PyErr_SetString(PartExceptionOCCError, "empty shape cannot be revolved");
            return nullptr;
        }

        TopExp_Explorer xp;
        xp.Init(input,TopAbs_SOLID);
        if (xp.More()) {
            PyErr_SetString(PartExceptionOCCError, "shape must not contain solids");
            return nullptr;
        }
        xp.Init(input,TopAbs_COMPSOLID);
        if (xp.More()) {
            PyErr_SetString(PartExceptionOCCError, "shape must not contain compound solids");
            return nullptr;
        }

        Base::Vector3d pos = static_cast<Base::VectorPy*>(pPos)->value();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
        TopoDS_Shape shape = this->getTopoShapePtr()->revolve(
            gp_Ax1(gp_Pnt(pos.x,pos.y,pos.z), gp_Dir(dir.x,dir.y,dir.z)),d*(M_PI/180));
        TopAbs_ShapeEnum type = shape.ShapeType();

        switch (type) {
        case TopAbs_COMPOUND:
            return new TopoShapeCompoundPy(new TopoShape(shape));
        case TopAbs_COMPSOLID:
            return new TopoShapeCompSolidPy(new TopoShape(shape));
        case TopAbs_SOLID:
            return new TopoShapeSolidPy(new TopoShape(shape));
        case TopAbs_SHELL:
            return new TopoShapeShellPy(new TopoShape(shape));
        case TopAbs_FACE:
            return new TopoShapeFacePy(new TopoShape(shape));
        case TopAbs_WIRE:
            break;
        case TopAbs_EDGE:
            return new TopoShapeEdgePy(new TopoShape(shape));
        case TopAbs_VERTEX:
            break;
        case TopAbs_SHAPE:
            break;
        default:
            break;
        }

        PyErr_SetString(PartExceptionOCCError, "revolution for this shape type not supported");
        return nullptr;
#endif
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::check(PyObject *args)
{
    PyObject* runBopCheck = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &(PyBool_Type), &runBopCheck))
        return nullptr;

    if (!getTopoShapePtr()->getShape().IsNull()) {
        std::stringstream str;
        if (!getTopoShapePtr()->analyze(PyObject_IsTrue(runBopCheck) ? true : false, str)) {
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return nullptr;
        }
    }

    return IncRef();
}

static PyObject *makeShape(const char *op,const TopoShape &shape, PyObject *args) {
    double tol=0;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O|d", &pcObj,&tol))
        return 0;
    PY_TRY {
        std::vector<TopoShape> shapes;
        shapes.push_back(shape);
        getPyShapes(pcObj,shapes);
        return Py::new_reference_to(shape2pyshape(TopoShape().makEBoolean(op,shapes,0,tol)));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::fuse(PyObject *args)
{
#if !defined(FC_NO_ELEMENT_MAP)
    return makeShape(Part::OpCodes::Fuse,*getTopoShapePtr(),args);
#else
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a fuse operation:
            TopoDS_Shape fusShape = this->getTopoShapePtr()->fuse(shape);
            return new TopoShapePy(new TopoShape(fusShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            // Let's call algorithm computing a fuse operation:
            TopoDS_Shape fuseShape = this->getTopoShapePtr()->fuse(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(fuseShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        Py::Sequence shapeSeq(pcObj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
            }
            else {
                PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
                return nullptr;
           }
        }
        try {
            TopoDS_Shape multiFusedShape = this->getTopoShapePtr()->fuse(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiFusedShape));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return nullptr;
#endif
}

PyObject*  TopoShapePy::multiFuse(PyObject *args)
{
#if !defined(FC_NO_ELEMENT_MAP)
    return makeShape(Part::OpCodes::Fuse,*getTopoShapePtr(),args);
#else
    double tolerance = 0.0;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance))
        return nullptr;

    std::vector<TopoDS_Shape> shapeVec;
    Py::Sequence shapeSeq(pcObj);
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
        }
        else {
            PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
            return nullptr;
       }
    }
    try {
        TopoDS_Shape multiFusedShape = this->getTopoShapePtr()->fuse(shapeVec,tolerance);
        return new TopoShapePy(new TopoShape(multiFusedShape));
    } PY_CATCH_OCC
#endif
}

PyObject*  TopoShapePy::oldFuse(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return nullptr;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    try {
        // Let's call algorithm computing a fuse operation:
        TopoDS_Shape fusShape = this->getTopoShapePtr()->oldFuse(shape);
        return new TopoShapePy(new TopoShape(fusShape));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::common(PyObject *args)
{
#if !defined(FC_NO_ELEMENT_MAP)
    return makeShape(Part::OpCodes::Common,*getTopoShapePtr(),args);
#else
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a common operation:
            TopoDS_Shape comShape = this->getTopoShapePtr()->common(shape);
            return new TopoShapePy(new TopoShape(comShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape commonShape = this->getTopoShapePtr()->common(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(commonShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        Py::Sequence shapeSeq(pcObj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
            }
            else {
                PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
                return nullptr;
            }
        }
        try {
            TopoDS_Shape multiCommonShape = this->getTopoShapePtr()->common(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiCommonShape));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return nullptr;
#endif
}

PyObject*  TopoShapePy::section(PyObject *args)
{
#if !defined(FC_NO_ELEMENT_MAP)
    return makeShape(Part::OpCodes::Section,*getTopoShapePtr(),args);
#else
    PyObject *pcObj;
    PyObject *approx = Py_False;
    if (PyArg_ParseTuple(args, "O!|O!", &(TopoShapePy::Type), &pcObj, &(PyBool_Type), &approx)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a section operation:
            TopoDS_Shape secShape = this->getTopoShapePtr()->section(shape,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(secShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d|O!", &(TopoShapePy::Type), &pcObj, &tolerance, &(PyBool_Type), &approx)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape sectionShape = this->getTopoShapePtr()->section(shapeVec,tolerance,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(sectionShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O|dO!", &pcObj, &tolerance, &(PyBool_Type), &approx)) {
        std::vector<TopoDS_Shape> shapeVec;
        Py::Sequence shapeSeq(pcObj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
            }
            else {
                PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
                return nullptr;
           }
        }
        try {
            TopoDS_Shape multiSectionShape = this->getTopoShapePtr()->section(shapeVec,tolerance,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(multiSectionShape));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return nullptr;
#endif
}

PyObject*  TopoShapePy::slice(PyObject *args)
{
    PyObject *dir;
    double d;
    if (!PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &dir, &d))
        return nullptr;

    Base::Vector3d vec = Py::Vector(dir, false).toVector();

    try {
#if !defined(FC_NO_ELEMENT_MAP)
        Py::List wires;
        for(auto &w : getTopoShapePtr()->makESlice(vec,d).getSubTopoShapes(TopAbs_WIRE))
            wires.append(shape2pyshape(w));
        return Py::new_reference_to(wires);
#else
        std::list<TopoDS_Wire> slice = this->getTopoShapePtr()->slice(vec, d);
        Py::List wire;
        for (std::list<TopoDS_Wire>::iterator it = slice.begin(); it != slice.end(); ++it) {
            wire.append(Py::asObject(new TopoShapeWirePy(new TopoShape(*it))));
        }

        return Py::new_reference_to(wire);
#endif
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::slices(PyObject *args)
{
    PyObject *dir, *dist;
    if (!PyArg_ParseTuple(args, "O!O", &(Base::VectorPy::Type), &dir, &dist))
        return nullptr;

    try {
        Base::Vector3d vec = Py::Vector(dir, false).toVector();
        Py::Sequence list(dist);
        std::vector<double> d;
        d.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it)
            d.push_back((double)Py::Float(*it));
#if !defined(FC_NO_ELEMENT_MAP)
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makESlices(vec,d)));
#else
        TopoDS_Compound slice = this->getTopoShapePtr()->slices(vec, d);
        return new TopoShapeCompoundPy(new TopoShape(slice));
#endif
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::cut(PyObject *args)
{
#if !defined(FC_NO_ELEMENT_MAP)
    return makeShape(Part::OpCodes::Cut,*getTopoShapePtr(),args);
#else
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a cut operation:
            TopoDS_Shape cutShape = this->getTopoShapePtr()->cut(shape);
            return new TopoShapePy(new TopoShape(cutShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape cutShape = this->getTopoShapePtr()->cut(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(cutShape));
        } PY_CATCH_OCC
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        Py::Sequence shapeSeq(pcObj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
                shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
            }
            else {
                PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
                return nullptr;
           }
        }
        try {
            TopoDS_Shape multiCutShape = this->getTopoShapePtr()->cut(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiCutShape));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return nullptr;
#endif
}

PyObject*  TopoShapePy::generalFuse(PyObject *args)
{
    double tolerance = 0.0;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance))
        return nullptr;

#if !defined(FC_NO_ELEMENT_MAP)
    std::vector<std::vector<TopoShape> > modifies;
    std::vector<TopoShape> shapes;
    shapes.push_back(*getTopoShapePtr());
    try {
        getPyShapes(pcObj,shapes);
        TopoShape res;
        res.makEGeneralFuse(shapes,modifies,tolerance);
        Py::List mapPy;
        for(auto &mod : modifies){
            Py::List shapesPy;
            for(auto &sh : mod)
                shapesPy.append(shape2pyshape(sh));
            mapPy.append(shapesPy);
        }
        Py::Tuple ret(2);
        ret[0] = shape2pyshape(res);
        ret[1] = mapPy;
        return Py::new_reference_to(ret);
    } PY_CATCH_OCC
#else
    std::vector<TopoDS_Shape> shapeVec;
    Py::Sequence shapeSeq(pcObj);
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
        }
        else {
            PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
            return nullptr;
       }
    }
    try {
        std::vector<TopTools_ListOfShape> map;
        TopoDS_Shape gfaResultShape = this->getTopoShapePtr()->generalFuse(shapeVec,tolerance,&map);
        Py::Object shapePy = shape2pyshape(gfaResultShape);
        Py::List mapPy;
        for(TopTools_ListOfShape &shapes: map){
            Py::List shapesPy;
            for(TopTools_ListIteratorOfListOfShape it(shapes); it.More(); it.Next()){
                shapesPy.append(shape2pyshape(it.Value()));
            }
            mapPy.append(shapesPy);
        }
        Py::Tuple ret(2);
        ret[0] = shapePy;
        ret[1] = mapPy;
        return Py::new_reference_to(ret);
    } PY_CATCH_OCC
#endif
}

PyObject*  TopoShapePy::sewShape(PyObject *args)
{
    double tolerance = 1.0e-06;
    if (!PyArg_ParseTuple(args, "|d", &tolerance))
        return nullptr;

    try {
        getTopoShapePtr()->sewShape();
        return IncRef();
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::childShapes(PyObject *args)
{
    PyObject* cumOri = Py_True;
    PyObject* cumLoc = Py_True;
    if (!PyArg_ParseTuple(args, "|O!O!", &(PyBool_Type), &cumOri, &(PyBool_Type), &cumLoc))
        return nullptr;
#ifndef FC_NO_ELEMENT_MAP
    TopoShape shape = *getTopoShapePtr();
    if(!PyObject_IsTrue(cumOri))
        shape.setShape(shape.getShape().Oriented(TopAbs_FORWARD), false);
    if (!PyObject_IsTrue(cumLoc))
        shape.setShape(shape.getShape().Located(TopLoc_Location()), false);
    Py::List list;
    PY_TRY {
        for(auto &s : shape.getSubTopoShapes())
            list.append(shape2pyshape(s));
        return Py::new_reference_to(list);
    } PY_CATCH_OCC
#else
    try {
        const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
        if (shape.IsNull()) {
            PyErr_SetString(PyExc_ValueError, "Shape is null");
            return nullptr;
        }
        TopoDS_Iterator it(shape,
            PyObject_IsTrue(cumOri) ? Standard_True : Standard_False,
            PyObject_IsTrue(cumLoc) ? Standard_True : Standard_False);
        Py::List list;
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            if (!aChild.IsNull()) {
                TopAbs_ShapeEnum type = aChild.ShapeType();
                PyObject* pyChild = nullptr;
                switch (type) {
                    case TopAbs_COMPOUND:
                        pyChild = new TopoShapeCompoundPy(new TopoShape(aChild));
                        break;
                    case TopAbs_COMPSOLID:
                        pyChild = new TopoShapeCompSolidPy(new TopoShape(aChild));
                        break;
                    case TopAbs_SOLID:
                        pyChild = new TopoShapeSolidPy(new TopoShape(aChild));
                        break;
                    case TopAbs_SHELL:
                        pyChild = new TopoShapeShellPy(new TopoShape(aChild));
                        break;
                    case TopAbs_FACE:
                        pyChild = new TopoShapeFacePy(new TopoShape(aChild));
                        break;
                    case TopAbs_WIRE:
                        pyChild = new TopoShapeWirePy(new TopoShape(aChild));
                        break;
                    case TopAbs_EDGE:
                        pyChild = new TopoShapeEdgePy(new TopoShape(aChild));
                        break;
                    case TopAbs_VERTEX:
                        pyChild = new TopoShapeVertexPy(new TopoShape(aChild));
                        break;
                    case TopAbs_SHAPE:
                        break;
                    default:
                        break;
                }

                if (pyChild) {
                    list.append(Py::Object(pyChild,true));
                }
            }
        }
        return Py::new_reference_to(list);
    } PY_CATCH_OCC
#endif
}

namespace Part {
// Containers to associate TopAbs_ShapeEnum values to each TopoShape*Py class
const std::vector<std::pair<PyTypeObject*, TopAbs_ShapeEnum>> vecTypeShape = {
    {&TopoShapeCompoundPy::Type, TopAbs_COMPOUND},
    {&TopoShapeCompSolidPy::Type, TopAbs_COMPSOLID},
    {&TopoShapeSolidPy::Type, TopAbs_SOLID},
    {&TopoShapeShellPy::Type, TopAbs_SHELL},
    {&TopoShapeFacePy::Type, TopAbs_FACE},
    {&TopoShapeWirePy::Type, TopAbs_WIRE},
    {&TopoShapeEdgePy::Type, TopAbs_EDGE},
    {&TopoShapeVertexPy::Type, TopAbs_VERTEX},
    {&TopoShapePy::Type, TopAbs_SHAPE}
};

const std::map<PyTypeObject*, TopAbs_ShapeEnum> mapTypeShape(
    vecTypeShape.begin(), vecTypeShape.end());

// Returns shape type of a Python type. Similar to TopAbs::ShapeTypeFromString.
// Returns TopAbs_SHAPE if pyType is not a subclass of any of the TopoShape*Py.
static TopAbs_ShapeEnum ShapeTypeFromPyType(PyTypeObject* pyType)
{
    for (auto it = vecTypeShape.begin(); it != vecTypeShape.end(); ++it) {
        if (PyType_IsSubtype(pyType, it->first))
            return it->second;
    }
    return TopAbs_SHAPE;
};
}

PyObject*  TopoShapePy::ancestorsOfType(PyObject *args)
{
    PyObject *pcObj;
    PyObject *type;
    if (!PyArg_ParseTuple(args, "O!O!", &(TopoShapePy::Type), &pcObj, &PyType_Type, &type))
        return nullptr;

    try {
        const TopoDS_Shape& model = getTopoShapePtr()->getShape();
        const TopoDS_Shape& shape = static_cast<TopoShapePy*>(pcObj)->
                getTopoShapePtr()->getShape();
        if (model.IsNull() || shape.IsNull()) {
            PyErr_SetString(PyExc_ValueError, "Shape is null");
            return nullptr;
        }

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
            PyErr_SetString(PyExc_TypeError, "type must be a Shape subtype");
            return nullptr;
        }

        TopTools_IndexedDataMapOfShapeListOfShape mapOfShapeShape;
        TopExp::MapShapesAndAncestors(model, shape.ShapeType(), shapetype, mapOfShapeShape);
        const TopTools_ListOfShape& ancestors = mapOfShapeShape.FindFromKey(shape);

        Py::List list;
        std::set<Standard_Integer> hashes;
        TopTools_ListIteratorOfListOfShape it(ancestors);
        for (; it.More(); it.Next()) {
            // make sure to avoid duplicates
            Standard_Integer code = it.Value().HashCode(INT_MAX);
            if (hashes.find(code) == hashes.end()) {
                list.append(shape2pyshape(it.Value()));
                hashes.insert(code);
            }
        }

        return Py::new_reference_to(list);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject*  TopoShapePy::removeInternalWires(PyObject *args)
{
    double minArea;
    if (!PyArg_ParseTuple(args, "d",&minArea))
        return nullptr;

    try {
        bool ok = getTopoShapePtr()->removeInternalWires(minArea);
        PyObject* ret = ok ? Py_True : Py_False;
        Py_INCREF(ret);
        return ret;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject*  TopoShapePy::mirror(PyObject *args)
{
    PyObject *v1, *v2;
    if (!PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type),&v1, &(Base::VectorPy::Type),&v2))
        return nullptr;

    Base::Vector3d base = Py::Vector(v1,false).toVector();
    Base::Vector3d norm = Py::Vector(v2,false).toVector();

    PY_TRY {
        gp_Ax2 ax2(gp_Pnt(base.x,base.y,base.z), gp_Dir(norm.x,norm.y,norm.z));
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEMirror(ax2)));
#else
        TopoDS_Shape shape = this->getTopoShapePtr()->mirror(ax2);
        return new TopoShapePy(new TopoShape(shape));
#endif
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::transformGeometry(PyObject *args)
{
    PyObject *obj;
    PyObject *cpy = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!", &(Base::MatrixPy::Type), &obj, &PyBool_Type, &cpy))
        return nullptr;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->transformGShape(mat, PyObject_IsTrue(cpy) ? true : false);
        return new TopoShapePy(new TopoShape(shape));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::transformShape(PyObject *args)
{
    PyObject *obj;
    PyObject *copy = Py_False;
    PyObject *checkScale = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!O!", &(Base::MatrixPy::Type),&obj,&(PyBool_Type), &copy, &(PyBool_Type), &checkScale))
        return nullptr;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    PY_TRY {
        this->getTopoShapePtr()->transformShape(mat, PyObject_IsTrue(copy) ? true : false, 
                PyObject_IsTrue(checkScale) ? true : false);
        return IncRef();
    }
    PY_CATCH_OCC
}

PyObject* TopoShapePy::transformed(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"matrix", "copy", "checkScale", "op", nullptr};
    PyObject* pymat;
    PyObject* copy = Py_False;
    PyObject* checkScale = Py_False;
    const char *op = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|O!O!s", kwlist,
                &Base::MatrixPy::Type, &pymat, &PyBool_Type, &copy, &PyBool_Type, &checkScale, &op))
        return nullptr;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(pymat)->value();
    PY_TRY {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makETransform(mat,op,
                        PyObject_IsTrue(checkScale) ? true : false,
                        PyObject_IsTrue(copy) ? true : false)));
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::translate(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return nullptr;

    Base::Vector3d vec;
    if (PyObject_TypeCheck(obj, &(Base::VectorPy::Type))) {
        vec = static_cast<Base::VectorPy*>(obj)->value();
    }
    else if (PyObject_TypeCheck(obj, &PyTuple_Type)) {
        vec = Base::getVectorFromTuple<double>(obj);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "either vector or tuple expected");
        return nullptr;
    }

    gp_Trsf mov;
    mov.SetTranslation(gp_Vec(vec.x,vec.y,vec.z));
    TopLoc_Location loc(mov);
    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Move(loc);
    getTopoShapePtr()->setShape(shape,false);
    return IncRef();
}

PyObject*  TopoShapePy::rotate(PyObject *args)
{
    PyObject *obj1, *obj2;
    double angle;
    if (!PyArg_ParseTuple(args, "OOd", &obj1, &obj2, &angle))
        return nullptr;

    PY_TRY {
        // Vector also supports sequence
        Py::Sequence p1(obj1), p2(obj2);
        // Convert into OCC representation
        gp_Pnt pos = gp_Pnt((double)Py::Float(p1[0]),
                            (double)Py::Float(p1[1]),
                            (double)Py::Float(p1[2]));
        gp_Dir dir = gp_Dir((double)Py::Float(p2[0]),
                            (double)Py::Float(p2[1]),
                            (double)Py::Float(p2[2]));

        gp_Ax1 axis(pos, dir);
        gp_Trsf mov;
        mov.SetRotation(axis, angle*(M_PI/180));
        TopLoc_Location loc(mov);
        TopoDS_Shape shape = getTopoShapePtr()->getShape();
        shape.Move(loc);
        getTopoShapePtr()->setShape(shape,false);
        return IncRef();
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::scale(PyObject *args)
{
    double factor;
    PyObject* p=nullptr;
    if (!PyArg_ParseTuple(args, "d|O!", &factor, &(Base::VectorPy::Type), &p))
        return nullptr;

    gp_Pnt pos(0,0,0);
    if (p) {
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(p)->value();
        pos.SetX(pnt.x);
        pos.SetY(pnt.y);
        pos.SetZ(pnt.z);
    }
    if (fabs(factor) < Precision::Confusion()) {
        PyErr_SetString(PyExc_ValueError, "scale factor too small");
        return nullptr;
    }

    PY_TRY {
        const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
        if (!shape.IsNull()) {
            gp_Trsf scl;
            scl.SetScale(pos, factor);
            BRepBuilderAPI_Transform BRepScale(scl);
            bool bCopy = true;
            BRepScale.Perform(shape, bCopy);
#ifndef FC_NO_ELEMENT_MAP
            TopoShape copy(*getTopoShapePtr());
            getTopoShapePtr()->makEShape(BRepScale,copy);
#else
            getTopoShapePtr()->setShape(BRepScale.Shape());
#endif
        }
        return IncRef();
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::translated(PyObject *args)
{
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->translate(args);
}

PyObject*  TopoShapePy::rotated(PyObject *args)
{
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->rotate(args);
}

PyObject*  TopoShapePy::scaled(PyObject *args)
{
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->scale(args);
}

PyObject* TopoShapePy::makeFillet(PyObject *args)
{
    // use two radii for all edges
    double radius1, radius2;
    PyObject *obj;
#ifndef FC_NO_ELEMENT_MAP
    if (!PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "dO", &radius1, &obj)) {
            PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
                    "-- one radius and a list of edges\n"
                    "-- two radii and a list of edges");
            return 0;
        }
        radius2 = radius1;
    }
    PY_TRY {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEFillet(
                        getPyShapes(obj),radius1,radius2)));
    }PY_CATCH_OCC
#else
    if (PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
        PY_TRY {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            BRepFilletAPI_MakeFillet mkFillet(shape);
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkFillet.Shape()));
        } PY_CATCH_OCC
    }
#endif

    PyErr_Clear();
    // use one radius for all edges
    double radius;
    if (PyArg_ParseTuple(args, "dO", &radius, &obj)) {
        PY_TRY {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            BRepFilletAPI_MakeFillet mkFillet(shape);
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        mkFillet.Add(radius, TopoDS::Edge(edge));
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkFillet.Shape()));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return nullptr;
}

PyObject* TopoShapePy::makeChamfer(PyObject *args)
{
    // use two radii for all edges
    double radius1, radius2;
    PyObject *obj;
#ifndef FC_NO_ELEMENT_MAP
    if (!PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
        if (!PyArg_ParseTuple(args, "dO", &radius1, &obj)) {
            PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
                    "-- one radius and a list of edges\n"
                    "-- two radii and a list of edges");
            return 0;
        }
        PyErr_Clear();
        radius2 = radius1;
    }
    PY_TRY {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEChamfer(
                        getPyShapes(obj),radius1,radius2)));
    }PY_CATCH_OCC
#else
    if (PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
        PY_TRY {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            BRepFilletAPI_MakeChamfer mkChamfer(shape);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
            TopExp::MapShapes(shape, TopAbs_EDGE, mapOfEdges);
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
                        mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), face);
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkChamfer.Shape()));
        } PY_CATCH_OCC
    }
#endif

    PyErr_Clear();
    // use one radius for all edges
    double radius;
    if (PyArg_ParseTuple(args, "dO", &radius, &obj)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            BRepFilletAPI_MakeChamfer mkChamfer(shape);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
            TopExp::MapShapes(shape, TopAbs_EDGE, mapOfEdges);
            Py::Sequence list(obj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
#if OCC_VERSION_HEX > 0x070300
                        mkChamfer.Add(radius, radius, TopoDS::Edge(edge), face);
#else
                        mkChamfer.Add(radius, TopoDS::Edge(edge), face);
#endif
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkChamfer.Shape()));
        } PY_CATCH_OCC
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return nullptr;
}

PyObject* TopoShapePy::makeThickness(PyObject *args)
{
    PyObject *obj;
    double offset, tolerance;
    PyObject* inter = Py_False;
    PyObject* self_inter = Py_False;
    short offsetMode = 0, join = 0;
    if (!PyArg_ParseTuple(args, "Odd|O!O!hh", &obj, &offset, &tolerance,
        &(PyBool_Type), &inter, &(PyBool_Type), &self_inter, &offsetMode, &join))
        return nullptr;

    PY_TRY {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEThickSolid(
                        getPyShapes(obj),offset,tolerance, PyObject_IsTrue(inter) ? true : false, 
                        PyObject_IsTrue(self_inter) ? true : false, offsetMode, static_cast<TopoShape::JoinType>(join))));
#else
        TopTools_ListOfShape facesToRemove;
        Py::Sequence list(obj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                facesToRemove.Append(shape);
            }
        }

        TopoDS_Shape shape = this->getTopoShapePtr()->makeThickSolid(facesToRemove, offset, tolerance,
            PyObject_IsTrue(inter) ? true : false, PyObject_IsTrue(self_inter) ? true : false, offsetMode, join);
        return new TopoShapeSolidPy(new TopoShape(shape));
#endif
    }PY_CATCH_OCC
}

PyObject* TopoShapePy::makeOffsetShape(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"offset", "tolerance", "inter", "self_inter", "offsetMode", "join", "fill", nullptr};
    double offset, tolerance;
    PyObject* inter = Py_False;
    PyObject* self_inter = Py_False;
    PyObject* fill = Py_False;
    short offsetMode = 0, join = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "dd|O!O!hhO!", kwlist, &offset, &tolerance,
        &(PyBool_Type), &inter, &(PyBool_Type), &self_inter, &offsetMode, &join, &(PyBool_Type), &fill))
        return nullptr;

    PY_TRY {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEOffset(
                        offset, tolerance, PyObject_IsTrue(inter) ? true : false,
                        PyObject_IsTrue(self_inter) ? true : false, offsetMode, static_cast<TopoShape::JoinType>(join),
                        PyObject_IsTrue(fill) ? true : false)));
#else
        TopoDS_Shape shape = this->getTopoShapePtr()->makeOffsetShape(offset, tolerance,
            PyObject_IsTrue(inter) ? true : false,
            PyObject_IsTrue(self_inter) ? true : false, offsetMode, join,
            PyObject_IsTrue(fill) ? true : false);
        return new TopoShapePy(new TopoShape(shape));
#endif
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makeOffset2D(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"offset", "join", "fill", "openResult", "intersection", nullptr};
    double offset;
    PyObject* fill = Py_False;
    PyObject* openResult = Py_False;
    PyObject* inter = Py_False;
    short join = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "d|hO!O!O!", kwlist,  &offset, &join,
        &(PyBool_Type), &fill, &(PyBool_Type), &openResult, &(PyBool_Type), &inter))
        return nullptr;

    try {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEOffset2D(
            offset, static_cast<TopoShape::JoinType>(join), PyObject_IsTrue(fill) ? true : false,
            PyObject_IsTrue(openResult) ? true : false,
            PyObject_IsTrue(inter) ? true : false)));
#else
        TopoDS_Shape resultShape = this->getTopoShapePtr()->makeOffset2D(offset, join,
            PyObject_IsTrue(fill) ? true : false,
            PyObject_IsTrue(openResult) ? true : false,
            PyObject_IsTrue(inter) ? true : false);
        return new_reference_to(shape2pyshape(resultShape));
#endif
    }
    PY_CATCH_OCC;
}

PyObject*  TopoShapePy::reverse(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Reverse();
    getTopoShapePtr()->setShape(shape,false);
    return IncRef();
}

PyObject*  TopoShapePy::reversed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape = shape.Reversed();

    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;

    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return nullptr;
    }

    if (!shape.IsNull()) {
        static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->setShape(shape);
    }
    return cpy;
}

PyObject*  TopoShapePy::complement(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Complement();
    getTopoShapePtr()->setShape(shape,false);
    return IncRef();
}

PyObject*  TopoShapePy::nullify(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Nullify();
    getTopoShapePtr()->setShape(shape);
    return IncRef();
}

PyObject*  TopoShapePy::isNull(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    bool null = getTopoShapePtr()->isNull();
    return Py_BuildValue("O", (null ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    PY_TRY {
        if (getTopoShapePtr()->getShape().IsNull())
            Standard_Failure::Raise("Cannot determine the 'Closed'' flag of an empty shape");
        return Py_BuildValue("O", (getTopoShapePtr()->isClosed() ? Py_True : Py_False));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::isEqual(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return nullptr;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = (getTopoShapePtr()->getShape().IsEqual(shape));

    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isSame(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return nullptr;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = getTopoShapePtr()->getShape().IsSame(shape);

    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isPartner(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return nullptr;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = getTopoShapePtr()->getShape().IsPartner(shape);

    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isValid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        return Py_BuildValue("O", (getTopoShapePtr()->isValid() ? Py_True : Py_False));
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::isCoplanar(PyObject *args)
{
    PyObject *pyObj;
    double tol = -1;
    if (!PyArg_ParseTuple(args, "O!|d", &TopoShapePy::Type, &pyObj, &tol))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getTopoShapePtr()->isCoplanar(
                    *static_cast<TopoShapePy*>(pyObj)->getTopoShapePtr(),tol)));
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::isInfinite(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getTopoShapePtr()->isInfinite()));
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::isLinearEdge(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getTopoShapePtr()->isLinearEdge()));
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::isPlanarFace(PyObject *args)
{
    double tol = 1e-7;
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return NULL;
    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getTopoShapePtr()->isPlanarFace(tol)));
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::findPlane(PyObject *args)
{
    double tol = -1;
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return nullptr;

    PY_TRY {
        gp_Pln pln;
        if(getTopoShapePtr()->findPlane(pln,tol))
            return new PlanePy(new GeomPlane(new Geom_Plane(pln)));
        Py_Return;
    }
    PY_CATCH_OCC
}

PyObject*  TopoShapePy::fix(PyObject *args)
{
    double prec, mintol, maxtol;
    if (!PyArg_ParseTuple(args, "ddd", &prec, &mintol, &maxtol))
        return nullptr;
    PY_TRY {
        return Py_BuildValue("O", (getTopoShapePtr()->fix(prec, mintol, maxtol) ? Py_True : Py_False));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::hashCode(PyObject *args)
{
    int upper = IntegerLast();
    if (!PyArg_ParseTuple(args, "|i",&upper))
        return nullptr;

    int hc = getTopoShapePtr()->getShape().HashCode(upper);
    return Py_BuildValue("i", hc);
}

PyObject* TopoShapePy::tessellate(PyObject *args)
{
    float tolerance;
    PyObject* ok = Py_False;
    if (!PyArg_ParseTuple(args, "f|O!",&tolerance,&PyBool_Type,&ok))
        return nullptr;

    PY_TRY {
        std::vector<Base::Vector3d> Points;
        std::vector<Data::ComplexGeoData::Facet> Facets;
        if (PyObject_IsTrue(ok) ? true : false)
            BRepTools::Clean(getTopoShapePtr()->getShape());
        getTopoShapePtr()->getFaces(Points, Facets,tolerance);
        Py::Tuple tuple(2);
        Py::List vertex;
        for (std::vector<Base::Vector3d>::const_iterator it = Points.begin();
            it != Points.end(); ++it)
            vertex.append(Py::asObject(new Base::VectorPy(*it)));
        tuple.setItem(0, vertex);
        Py::List facet;
        for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
            it = Facets.begin(); it != Facets.end(); ++it) {
            Py::Tuple f(3);
            f.setItem(0,Py::Long((long)it->I1));
            f.setItem(1,Py::Long((long)it->I2));
            f.setItem(2,Py::Long((long)it->I3));
            facet.append(f);
        }
        tuple.setItem(1, facet);
        return Py::new_reference_to(tuple);
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::project(PyObject *args)
{
    PyObject *obj;

    BRepAlgo_NormalProjection algo;
    algo.Init(this->getTopoShapePtr()->getShape());
    if (!PyArg_ParseTuple(args, "O", &obj))
        return nullptr;

    PY_TRY {
        Py::Sequence list(obj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->getShape();
                algo.Add(shape);
            }
        }

        algo.Compute3d(Standard_True);
        algo.SetLimit(Standard_True);
        algo.SetParams(1.e-6, 1.e-6, GeomAbs_C1, 14, 16);
        //algo.SetDefaultParams();
        algo.Build();
        return new TopoShapePy(new TopoShape(algo.Projection()));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makeParallelProjection(PyObject *args)
{
    PyObject *pShape, *pDir;
    if (!PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapePy::Type), &pShape, &Base::VectorPy::Type, &pDir))
        return nullptr;

    try {
        const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
        const TopoDS_Shape& wire = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->getShape();
        Base::Vector3d vec = Py::Vector(pDir,false).toVector();
        BRepProj_Projection proj(wire, shape, gp_Dir(vec.x,vec.y,vec.z));
        TopoDS_Shape projected = proj.Shape();
        return new TopoShapePy(new TopoShape(projected));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makePerspectiveProjection(PyObject *args)
{
    PyObject *pShape, *pDir;
    if (!PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapePy::Type), &pShape, &Base::VectorPy::Type, &pDir))
        return nullptr;

    try {
        const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
        const TopoDS_Shape& wire = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->getShape();
        Base::Vector3d vec = Py::Vector(pDir,false).toVector();
        BRepProj_Projection proj(wire, shape, gp_Pnt(vec.x,vec.y,vec.z));
        TopoDS_Shape projected = proj.Shape();
        return new TopoShapePy(new TopoShape(projected));
    } PY_CATCH_OCC
}

/*!
from pivy import coin

rot=Gui.ActiveDocument.ActiveView.getCameraOrientation()
vdir=App.Vector(0,0,-1)
vdir=rot.multVec(vdir)
udir=App.Vector(0,1,0)
udir=rot.multVec(udir)

pos=Gui.ActiveDocument.ActiveView.getCameraNode().position.getValue().getValue()
pos=App.Vector(*pos)

shape=App.ActiveDocument.ActiveObject.Shape
reflect=shape.reflectLines(ViewDir=vdir, ViewPos=pos, UpDir=udir, EdgeType="Sharp", Visible=True, OnShape=False)
Part.show(reflect)
 */
PyObject* TopoShapePy::reflectLines(PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"ViewDir", "ViewPos", "UpDir", "EdgeType", "Visible", "OnShape", nullptr};

    char* type="OutLine";
    PyObject* vis = Py_True;
    PyObject* in3d = Py_False;
    PyObject* pPos = nullptr;
    PyObject* pUp = nullptr;
    PyObject *pView;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!O!sO!O!", kwlist,
        &Base::VectorPy::Type, &pView, &Base::VectorPy::Type, &pPos, &Base::VectorPy::Type,
        &pUp, &type, &PyBool_Type, &vis, &PyBool_Type, &in3d))
        return nullptr;

    PY_TRY {
        HLRBRep_TypeOfResultingEdge t;
        std::string str = type;
        if (str == "IsoLine")
            t = HLRBRep_IsoLine;
        else if (str == "Rg1Line")
            t = HLRBRep_Rg1Line;
        else if (str == "RgNLine")
            t = HLRBRep_RgNLine;
        else if (str == "Sharp")
            t = HLRBRep_Sharp;
        else
            t = HLRBRep_OutLine;

        Base::Vector3d p(0.0, 0.0, 0.0);
        if (pPos)
            p = Py::Vector(pPos,false).toVector();
        Base::Vector3d u(0.0, 1.0, 0.0);
        if (pUp)
            u = Py::Vector(pUp,false).toVector();

        Base::Vector3d v = Py::Vector(pView,false).toVector();
        const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
        HLRAppli_ReflectLines reflect(shape);
        reflect.SetAxes(v.x, v.y, v.z, p.x, p.y, p.z, u.x, u.y, u.z);
        reflect.Perform();
        TopoDS_Shape lines = reflect.GetCompoundOf3dEdges(t, PyObject_IsTrue(vis) ? Standard_True : Standard_False,
                                                          PyObject_IsTrue(in3d) ? Standard_True : Standard_False);
        return new TopoShapePy(new TopoShape(lines));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makeShapeFromMesh(PyObject *args)
{
    PyObject *tup;
    double tolerance = 1.0e-06;
    PyObject* sewShape = Py_True;
    if (!PyArg_ParseTuple(args, "O!|dO!",&PyTuple_Type, &tup, &tolerance, &PyBool_Type, &sewShape))
        return nullptr;

    PY_TRY {
        Py::Tuple tuple(tup);
        Py::Sequence vertex(tuple[0]);
        Py::Sequence facets(tuple[1]);

        std::vector<Base::Vector3d> Points;
        for (Py::Sequence::iterator it = vertex.begin(); it != vertex.end(); ++it) {
            Py::Vector vec(*it);
            Points.push_back(vec.toVector());
        }
        std::vector<Data::ComplexGeoData::Facet> Facets;
        for (Py::Sequence::iterator it = facets.begin(); it != facets.end(); ++it) {
            Data::ComplexGeoData::Facet face;
            Py::Tuple f(*it);
            face.I1 = (int)Py::Long(f[0]);
            face.I2 = (int)Py::Long(f[1]);
            face.I3 = (int)Py::Long(f[2]);
            Facets.push_back(face);
        }

        getTopoShapePtr()->setFaces(Points, Facets, tolerance);
        if (PyObject_IsTrue(sewShape) ? true : false)
            getTopoShapePtr()->sewShape(tolerance);

        Py_Return;
    }
    PY_CATCH_OCC
}

/*
import PartEnums
v = App.Vector
profile = Part.makePolygon([v(0.,0.,0.), v(-60.,-60.,-100.), v(-60.,-60.,-140.)])
spine = Part.makePolygon([v(0.,0.,0.), v(100.,0.,0.), v(100.,100.,0.), v(0.,100.,0.), v(0.,0.,0.)])
evolve = spine.makeEvolved(Profile=profile, Join=PartEnums.JoinType.Arc)
*/
PyObject* TopoShapePy::makeEvolved(PyObject *args, PyObject *kwds)
{
    PyObject* Profile;
    PyObject* AxeProf = Py_True;
    PyObject* Solid = Py_False;
    PyObject* ProfOnSpine = Py_False;
    auto JoinType = TopoShape::JoinType::Arc;
    double Tolerance = 0.0000001;

    static char* kwds_evolve[] = {"Profile", "Join", "AxeProf", "Solid", "ProfOnSpine", "Tolerance", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|iO!O!O!d", kwds_evolve,
                                     &TopoShapePy::Type, &Profile, &JoinType,
                                     &PyBool_Type, &AxeProf, &PyBool_Type, &Solid,
                                     &PyBool_Type, &ProfOnSpine, &Tolerance))
        return nullptr;

    try {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEEvolve(
                        *static_cast<TopoShapePy*>(Profile)->getTopoShapePtr(), JoinType,
                        PyObject_IsTrue(AxeProf) ? true : false,
                        PyObject_IsTrue(Solid) ? true : false,
                        PyObject_IsTrue(ProfOnSpine) ? true : false,
                        Tolerance)));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makeWires(PyObject *args) {
    const char *op = nullptr;
    if (!PyArg_ParseTuple(args, "s", &op))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEWires(op)));
    }
    PY_CATCH_OCC
}

PyObject* TopoShapePy::toNurbs(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        // Convert into nurbs
        TopoDS_Shape nurbs = this->getTopoShapePtr()->toNurbs();
        return new TopoShapePy(new TopoShape(nurbs));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::isInside(PyObject *args)
{
    PyObject *point;
    double tolerance;
    PyObject* checkFace = Py_False;
    TopAbs_State stateIn = TopAbs_IN;
    if (!PyArg_ParseTuple(args, "O!dO!", &(Base::VectorPy::Type), &point, &tolerance,  &PyBool_Type, &checkFace))
        return nullptr;

    try {
        TopoDS_Shape shape = getTopoShapePtr()->getShape();
        if (shape.IsNull()) {
            PyErr_SetString(PartExceptionOCCError, "Cannot handle null shape");
            return nullptr;
        }

        Base::Vector3d pnt = static_cast<Base::VectorPy*>(point)->value();
        gp_Pnt vertex = gp_Pnt(pnt.x,pnt.y,pnt.z);
        if (shape.ShapeType() == TopAbs_VERTEX ||
            shape.ShapeType() == TopAbs_EDGE ||
            shape.ShapeType() == TopAbs_WIRE) {

            BRepBuilderAPI_MakeVertex mkVertex(vertex);
            BRepExtrema_DistShapeShape extss;
            extss.LoadS1(mkVertex.Vertex());
            extss.LoadS2(shape);
            if (!extss.Perform()) {
                PyErr_SetString(PartExceptionOCCError, "Failed to determine distance to shape");
                return nullptr;
            }
            Standard_Boolean test = (extss.Value() <= tolerance);
            return Py_BuildValue("O", (test ? Py_True : Py_False));
        }
        else {
            BRepClass3d_SolidClassifier solidClassifier(shape);
            solidClassifier.Perform(vertex, tolerance);
            Standard_Boolean test = (solidClassifier.State() == stateIn);

            if ((PyObject_IsTrue(checkFace) ? true : false) && (solidClassifier.IsOnAFace()))
                test = Standard_True;
            return Py_BuildValue("O", (test ? Py_True : Py_False));
        }
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::removeSplitter(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
#ifndef FC_NO_ELEMENT_MAP
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makERefine()));
#else
        // Remove redundant splitter
        TopoDS_Shape shape = this->getTopoShapePtr()->removeSplitter();
        return new TopoShapePy(new TopoShape(shape));
#endif
    }PY_CATCH_OCC
}

PyObject* TopoShapePy::getElement(PyObject *args)
{
    char* input;
    PyObject *silent = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &input, &silent))
        return nullptr;
    PY_TRY {
        PyObject *res = getTopoShapePtr()->getPySubShape(input, PyObject_IsTrue(silent));
        if(!res)
            Py_Return;
        return res;
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::countElement(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(Py::Int((long)getTopoShapePtr()->countSubShapes(input)));
    }
    PY_CATCH_OCC
}

PyObject* TopoShapePy::getTolerance(PyObject *args)
{
    int mode;
    PyObject* type = reinterpret_cast<PyObject*>(&TopoShapePy::Type);
    if (!PyArg_ParseTuple(args, "i|O!", &mode, &PyType_Type, &type))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type) ||
            (shapetype != TopAbs_SHAPE && shapetype != TopAbs_VERTEX &&
            shapetype != TopAbs_EDGE && shapetype != TopAbs_FACE && shapetype != TopAbs_SHELL)) {
            PyErr_SetString(PyExc_TypeError, "shape type must be Shape, Vertex, Edge, Face or Shell");
            return nullptr;
        }

        ShapeAnalysis_ShapeTolerance analysis;
        double tolerance = analysis.Tolerance(shape, mode, shapetype);
        return PyFloat_FromDouble(tolerance);
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::overTolerance(PyObject *args)
{
    double value;
    PyObject* type = reinterpret_cast<PyObject*>(&TopoShapePy::Type);
    if (!PyArg_ParseTuple(args, "d|O!", &value, &PyType_Type, &type))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type) ||
            (shapetype != TopAbs_SHAPE && shapetype != TopAbs_VERTEX &&
            shapetype != TopAbs_EDGE && shapetype != TopAbs_FACE && shapetype != TopAbs_SHELL)) {
            PyErr_SetString(PyExc_TypeError, "shape type must be Shape, Vertex, Edge, Face or Shell");
            return nullptr;
        }

        ShapeAnalysis_ShapeTolerance analysis;
        Handle(TopTools_HSequenceOfShape) seq = analysis.OverTolerance(shape, value, shapetype);
        Py::Tuple tuple(seq->Length());
        std::size_t index=0;
        for (int i=1; i <= seq->Length(); i++) {
            TopoDS_Shape item = seq->Value(i);
            tuple.setItem(index++, shape2pyshape(item));
        }
        return Py::new_reference_to(tuple);
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::inTolerance(PyObject *args)
{
    double valmin;
    double valmax;
    PyObject* type = reinterpret_cast<PyObject*>(&TopoShapePy::Type);
    if (!PyArg_ParseTuple(args, "dd|O!", &valmin, &valmax, &PyType_Type, &type))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type) ||
            (shapetype != TopAbs_SHAPE && shapetype != TopAbs_VERTEX &&
            shapetype != TopAbs_EDGE && shapetype != TopAbs_FACE && shapetype != TopAbs_SHELL)) {
            PyErr_SetString(PyExc_TypeError, "shape type must be Shape, Vertex, Edge, Face or Shell");
            return nullptr;
        }

        ShapeAnalysis_ShapeTolerance analysis;
        Handle(TopTools_HSequenceOfShape) seq = analysis.InTolerance(shape, valmin, valmax, shapetype);
        Py::Tuple tuple(seq->Length());
        std::size_t index=0;
        for (int i=1; i <= seq->Length(); i++) {
            TopoDS_Shape item = seq->Value(i);
            tuple.setItem(index++, shape2pyshape(item));
        }
        return Py::new_reference_to(tuple);
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::globalTolerance(PyObject *args)
{
    int mode;
    if (!PyArg_ParseTuple(args, "i", &mode))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        ShapeAnalysis_ShapeTolerance analysis;
        analysis.Tolerance(shape, mode);
        double tolerance = analysis.GlobalTolerance(mode);

        return PyFloat_FromDouble(tolerance);
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::fixTolerance(PyObject *args)
{
    double value;
    PyObject* type = reinterpret_cast<PyObject*>(&TopoShapePy::Type);
    if (!PyArg_ParseTuple(args, "d|O!", &value, &PyType_Type, &type))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
            PyErr_SetString(PyExc_TypeError, "type must be a Shape subtype");
            return nullptr;
        }

        ShapeFix_ShapeTolerance fix;
        fix.SetTolerance(shape, value, shapetype);
        Py_Return;
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::limitTolerance(PyObject *args)
{
    double tmin;
    double tmax=0;
    PyObject* type = reinterpret_cast<PyObject*>(&TopoShapePy::Type);
    if (!PyArg_ParseTuple(args, "d|dO!", &tmin, &tmax, &PyType_Type, &type))
        return nullptr;

    PY_TRY {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = ShapeTypeFromPyType(pyType);
        if (!PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
            PyErr_SetString(PyExc_TypeError, "type must be a Shape subtype");
            return nullptr;
        }

        ShapeFix_ShapeTolerance fix;
        Standard_Boolean ok = fix.LimitTolerance(shape, tmin, tmax, shapetype);
        return PyBool_FromLong(ok ? 1 : 0);
    } PY_CATCH_OCC
}

PyObject* _getSupportIndex(const char* suppStr, TopoShape* ts, TopoDS_Shape suppShape) {
    std::stringstream ss;
    TopoDS_Shape subShape;

    unsigned long nSubShapes = ts->countSubShapes(suppStr);
    long supportIndex = -1;
    for (unsigned long j=1; j<=nSubShapes; j++){
        ss.str("");
        ss << suppStr << j;
        subShape = ts->getSubShape(ss.str().c_str());
        if (subShape.IsEqual(suppShape)) {
            supportIndex = j-1;
            break;
        }
    }
    return PyLong_FromLong(supportIndex);
}

PyObject* TopoShapePy::proximity(PyObject *args)
{
    typedef BRepExtrema_MapOfIntegerPackedMapOfInteger BRepExtrema_OverlappedSubShapes;

    PyObject* ps2;
    Standard_Real tol = Precision::Confusion();
    if (!PyArg_ParseTuple(args, "O!|d",&(TopoShapePy::Type), &ps2, &tol))
        return nullptr;

    const TopoDS_Shape& s1 = getTopoShapePtr()->getShape();
    const TopoDS_Shape& s2 = static_cast<Part::TopoShapePy*>(ps2)->getTopoShapePtr()->getShape();
    if (s1.IsNull()) {
        PyErr_SetString(PyExc_ValueError, "proximity: Shape object is invalid");
        return nullptr;
    }
    if (s2.IsNull()) {
        PyErr_SetString(PyExc_ValueError, "proximity: Shape parameter is invalid");
        return nullptr;
    }

    BRepExtrema_ShapeProximity proximity;
    proximity.LoadShape1 (s1);
    proximity.LoadShape2 (s2);
    if (tol > 0.0)
        proximity.SetTolerance (tol);
    proximity.Perform();
    if (!proximity.IsDone()) {
        // the proximity failed, maybe it's because the shapes are not yet mesh
        TopLoc_Location aLoc;
        TopExp_Explorer xp(s1, TopAbs_FACE);
        while (xp.More()) {
            const Handle(Poly_Triangulation)& aTriangulation =
              BRep_Tool::Triangulation(TopoDS::Face(xp.Current()), aLoc);
            if (aTriangulation.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "BRepExtrema_ShapeProximity not done, call 'tessellate' beforehand");
                return nullptr;
            }
        }

        xp.Init(s2, TopAbs_FACE);
        while (xp.More()) {
            const Handle(Poly_Triangulation)& aTriangulation =
              BRep_Tool::Triangulation(TopoDS::Face(xp.Current()), aLoc);
            if (aTriangulation.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "BRepExtrema_ShapeProximity not done, call 'tessellate' beforehand");
                return nullptr;
            }
        }

        // check also for free edges
        xp.Init(s1, TopAbs_EDGE, TopAbs_FACE);
        while (xp.More()) {
            const Handle(Poly_Polygon3D)& aPoly3D =
              BRep_Tool::Polygon3D(TopoDS::Edge(xp.Current()), aLoc);
            if (aPoly3D.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "BRepExtrema_ShapeProximity not done, call 'tessellate' beforehand");
                return nullptr;
            }
        }

        xp.Init(s2, TopAbs_EDGE, TopAbs_FACE);
        while (xp.More()) {
            const Handle(Poly_Polygon3D)& aPoly3D =
              BRep_Tool::Polygon3D(TopoDS::Edge(xp.Current()), aLoc);
            if (aPoly3D.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "BRepExtrema_ShapeProximity not done, call 'tessellate' beforehand");
                return nullptr;
            }
        }

        // another problem must have occurred
        PyErr_SetString(PartExceptionOCCError, "BRepExtrema_ShapeProximity not done");
        return nullptr;
    }

    Py::List overlappssindex1;
    Py::List overlappssindex2;

    for (BRepExtrema_OverlappedSubShapes::Iterator anIt1 (proximity.OverlapSubShapes1()); anIt1.More(); anIt1.Next()) {
        overlappssindex1.append(Py::Long(anIt1.Key() + 1));
    }
    for (BRepExtrema_OverlappedSubShapes::Iterator anIt2 (proximity.OverlapSubShapes2()); anIt2.More(); anIt2.Next()) {
        overlappssindex2.append(Py::Long(anIt2.Key() + 1));
    }

    Py::Tuple tuple(2);
    tuple.setItem(0, overlappssindex1);
    tuple.setItem(1, overlappssindex2);
    return Py::new_reference_to(tuple); //face indexes

}

PyObject* TopoShapePy::distToShape(PyObject *args)
{
    PyObject* ps2;
    gp_Pnt P1,P2;
    BRepExtrema_SupportType supportType1,supportType2;
    TopoDS_Shape suppS1,suppS2;
    Standard_Real minDist = -1, t1,t2,u1,v1,u2,v2;

    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type), &ps2))
        return nullptr;

    const TopoDS_Shape& s1 = getTopoShapePtr()->getShape();
    TopoShape* ts1 = getTopoShapePtr();
    const TopoDS_Shape& s2 = static_cast<Part::TopoShapePy*>(ps2)->getTopoShapePtr()->getShape();
    TopoShape* ts2 = static_cast<Part::TopoShapePy*>(ps2)->getTopoShapePtr();

    if (s2.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "distToShape: Shape parameter is invalid");
        return nullptr;
    }
    BRepExtrema_DistShapeShape extss;
    extss.LoadS1(s1);
    extss.LoadS2(s2);
    try {
        extss.Perform();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
    if (!extss.IsDone()) {
        PyErr_SetString(PyExc_RuntimeError, "BRepExtrema_DistShapeShape failed");
        return nullptr;
    }
    Py::List solnPts;
    Py::List solnGeom;
    int count = extss.NbSolution();
    if (count != 0) {
        minDist = extss.Value();
        //extss.Dump(std::cout);
        for (int i=1; i<= count; i++) {
            Py::Object pt1, pt2;
            Py::String suppType1, suppType2;
            Py::Long suppIndex1, suppIndex2;
            Py::Object param1, param2;

            P1 = extss.PointOnShape1(i);
            pt1 = Py::asObject( new Base::VectorPy(new Base::Vector3d(P1.X(),P1.Y(),P1.Z())));
            supportType1 = extss.SupportTypeShape1(i);
            suppS1 = extss.SupportOnShape1(i);
            switch (supportType1) {
                case BRepExtrema_IsVertex:
                    suppType1 = Py::String("Vertex");
                    suppIndex1 = Py::asObject(_getSupportIndex("Vertex",ts1,suppS1));
                    param1 = Py::None();
                    break;
                case BRepExtrema_IsOnEdge:
                    suppType1 = Py::String("Edge");
                    suppIndex1 = Py::asObject(_getSupportIndex("Edge",ts1,suppS1));
                    extss.ParOnEdgeS1(i,t1);
                    param1 = Py::Float(t1);
                    break;
                case BRepExtrema_IsInFace:
                    suppType1 = Py::String("Face");
                    suppIndex1 = Py::asObject(_getSupportIndex("Face",ts1,suppS1));
                    extss.ParOnFaceS1(i,u1,v1);
                    {
                        Py::Tuple tup(2);
                        tup[0] = Py::Float(u1);
                        tup[1] = Py::Float(v1);
                        param1 = tup;
                    }
                    break;
                default:
                    Base::Console().Message("distToShape: supportType1 is unknown: %d \n",supportType1);
                    suppType1 = Py::String("Unknown");
                    suppIndex1 = -1;
                    param1 = Py::None();
            }

            P2 = extss.PointOnShape2(i);
            pt2 = Py::asObject(new Base::VectorPy(new Base::Vector3d(P2.X(),P2.Y(),P2.Z())));
            supportType2 = extss.SupportTypeShape2(i);
            suppS2 = extss.SupportOnShape2(i);
            switch (supportType2) {
                case BRepExtrema_IsVertex:
                    suppType2 = Py::String("Vertex");
                    suppIndex2 = Py::asObject(_getSupportIndex("Vertex",ts2,suppS2));
                    param2 = Py::None();
                    break;
                case BRepExtrema_IsOnEdge:
                    suppType2 = Py::String("Edge");
                    suppIndex2 = Py::asObject(_getSupportIndex("Edge",ts2,suppS2));
                    extss.ParOnEdgeS2(i,t2);
                    param2 = Py::Float(t2);
                    break;
                case BRepExtrema_IsInFace:
                    suppType2 = Py::String("Face");
                    suppIndex2 = Py::asObject(_getSupportIndex("Face",ts2,suppS2));
                    extss.ParOnFaceS2(i,u2,v2);
                    {
                        Py::Tuple tup(2);
                        tup[0] = Py::Float(u2);
                        tup[1] = Py::Float(v2);
                        param2 = tup;
                    }
                    break;
                default:
                    Base::Console().Message("distToShape: supportType2 is unknown: %d \n",supportType2);
                    suppType2 = Py::String("Unknown");
                    suppIndex2 = -1;
                    param2 = Py::None();
            }
            Py::Tuple pts(2);
            pts[0] = pt1;
            pts[1] = pt2;
            solnPts.append(pts);

            Py::Tuple geom(6);
            geom[0] = suppType1;
            geom[1] = suppIndex1;
            geom[2] = param1;
            geom[3] = suppType2;
            geom[4] = suppIndex2;
            geom[5] = param2;

            solnGeom.append(geom);
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "distToShape: No Solutions Found.");
        return nullptr;
    }
    Py::Tuple ret(3);
    ret[0] = Py::Float(minDist);
    ret[1] = solnPts;
    ret[2] = solnGeom;
    return Py::new_reference_to(ret);
}

PyObject* TopoShapePy::optimalBoundingBox(PyObject *args)
{
    PyObject* useT = Py_True;
    PyObject* useS = Py_False;
    if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &PyBool_Type, &useT, &useS))
        return nullptr;

    PY_TRY {
#if OCC_VERSION_HEX >= 0x070200
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        Bnd_Box bounds;
        BRepBndLib::AddOptimal(shape, bounds,
                               PyObject_IsTrue(useT) ? Standard_True : Standard_False,
                               PyObject_IsTrue(useS) ? Standard_True : Standard_False);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        Base::BoundBox3d box;
        box.MinX = xMin;
        box.MaxX = xMax;
        box.MinY = yMin;
        box.MaxY = yMax;
        box.MinZ = zMin;
        box.MaxZ = zMax;

        Py::BoundingBox pybox(box);
        return Py::new_reference_to(pybox);
#else
        throw Py::RuntimeError("Need OCCT 7.2.0 or higher");
#endif
    } PY_CATCH_OCC
}

PyObject *TopoShapePy::clearCache(PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    getTopoShapePtr()->initCache(1);
    return IncRef();
}

PyObject* TopoShapePy::defeaturing(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return nullptr;

    PY_TRY {
        Py::Sequence list(l);
        std::vector<TopoDS_Shape> shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::TopoShape sh(*it);
            shapes.push_back(
                sh.extensionObject()->getTopoShapePtr()->getShape()
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, nullptr);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->defeaturing(shapes));
        return inst;
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::findSubShape(PyObject *args)
{
    PyObject *pyobj;
    if (!PyArg_ParseTuple(args, "O", &pyobj))
        return nullptr;

    PY_TRY {
        Py::List res;
        for (auto & s : getPyShapes(pyobj)) {
            int index = getTopoShapePtr()->findShape(s.getShape());
            if (index > 0)
                res.append(Py::TupleN(Py::String(s.shapeName()), Py::Int(index)));
            else
                res.append(Py::TupleN(Py::Object(), Py::Int(0)));
        }
        if (PySequence_Check(pyobj))
            return Py::new_reference_to(res);
        return Py::new_reference_to(Py::Object(res[0].ptr()));
    } PY_CATCH_OCC
}

PyObject *TopoShapePy::searchSubShape(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"shape", "needName", "checkGeometry", "tol", "atol", nullptr};
    PyObject *pyobj;
    PyObject *needName = Py_False;
    PyObject *checkGeometry = Py_True;
    double tol=1e-7;
    double atol=1e-12;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|OOdd", kwlist,
                                    &Type,&pyobj,&needName,&checkGeometry,&tol,&atol))
        return nullptr;

    PY_TRY {
        Py::List res;
        const TopoShape &shape = *static_cast<TopoShapePy*>(pyobj)->getTopoShapePtr();
        if(PyObject_IsTrue(needName)) {
            std::vector<std::string> names;
            auto shapes = getTopoShapePtr()->searchSubShape(
                    shape,&names,PyObject_IsTrue(checkGeometry),tol,atol);
            for(std::size_t i=0; i<shapes.size(); ++i)
                res.append(Py::TupleN(Py::String(names[i]), shape2pyshape(shapes[i])));
        } else {
            for(auto &s : getTopoShapePtr()->searchSubShape(
                        shape,nullptr,PyObject_IsTrue(checkGeometry),tol,atol))
            {
                res.append(shape2pyshape(s));
            }
        }
        return Py::new_reference_to(res);
    } PY_CATCH_OCC
}

// End of Methods, Start of Attributes

#if 0 // see ComplexGeoDataPy::Matrix which does the same
Py::Object TopoShapePy::getLocation(void) const
{
    const TopLoc_Location& loc = getTopoShapePtr()->getShape().Location();
    gp_Trsf trf = (gp_Trsf)loc;
    Base::Matrix4D mat;
    mat[0][0] = trf.Value(1,1);
    mat[0][1] = trf.Value(1,2);
    mat[0][2] = trf.Value(1,3);
    mat[0][3] = trf.Value(1,4);

    mat[1][0] = trf.Value(2,1);
    mat[1][1] = trf.Value(2,2);
    mat[1][2] = trf.Value(2,3);
    mat[1][3] = trf.Value(2,4);

    mat[2][0] = trf.Value(3,1);
    mat[2][1] = trf.Value(3,2);
    mat[2][2] = trf.Value(3,3);
    mat[2][3] = trf.Value(3,4);
    return Py::asObject(new Base::MatrixPy(mat));
}

void TopoShapePy::setLocation(Py::Object o)
{
    PyObject* p = o.ptr();
    if (PyObject_TypeCheck(p, &(Base::MatrixPy::Type))) {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(p)->value();
        Base::Rotation rot(mat);
        Base::Vector3d axis;
        double angle;
        rot.getValue(axis, angle);
        gp_Trsf trf;
        trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
        trf.SetTranslationPart(gp_Vec(mat[0][3],mat[1][3],mat[2][3]));
        TopLoc_Location loc(trf);
        getTopoShapePtr()->getShape().Location(loc);
    }
    else {
        std::string error = std::string("type must be 'Matrix', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}
#endif

Py::String TopoShapePy::getShapeType(void) const
{
    TopoDS_Shape sh = getTopoShapePtr()->getShape();
    if (sh.IsNull())
        throw Py::Exception(Base::PyExc_FC_GeneralError, "cannot determine type of null shape");

    TopAbs_ShapeEnum type = sh.ShapeType();
    std::string name;
    switch (type) {
        case TopAbs_COMPOUND:
            name = "Compound";
            break;
        case TopAbs_COMPSOLID:
            name = "CompSolid";
            break;
        case TopAbs_SOLID:
            name = "Solid";
            break;
        case TopAbs_SHELL:
            name = "Shell";
            break;
        case TopAbs_FACE:
            name = "Face";
            break;
        case TopAbs_WIRE:
            name = "Wire";
            break;
        case TopAbs_EDGE:
            name = "Edge";
            break;
        case TopAbs_VERTEX:
            name = "Vertex";
            break;
        case TopAbs_SHAPE:
            name = "Shape";
            break;
    }

    return Py::String(name);
}

Py::String TopoShapePy::getOrientation(void) const
{
    TopoDS_Shape sh = getTopoShapePtr()->getShape();
    if (sh.IsNull())
        throw Py::Exception(Base::PyExc_FC_GeneralError, "cannot determine orientation of null shape");

    TopAbs_Orientation type = sh.Orientation();
    std::string name;
    switch (type) {
        case TopAbs_FORWARD:
            name = "Forward";
            break;
        case TopAbs_REVERSED:
            name = "Reversed";
            break;
        case TopAbs_INTERNAL:
            name = "Internal";
            break;
        case TopAbs_EXTERNAL:
            name = "External";
            break;
    }

    return Py::String(name);
}

void TopoShapePy::setOrientation(Py::String arg)
{
    TopoDS_Shape sh = getTopoShapePtr()->getShape();
    if (sh.IsNull())
        throw Py::Exception(Base::PyExc_FC_GeneralError, "cannot determine orientation of null shape");

    std::string name = static_cast<std::string>(arg);
    TopAbs_Orientation type;
    if (name == "Forward") {
        type = TopAbs_FORWARD;
    }
    else if (name == "Reversed") {
        type = TopAbs_REVERSED;
    }
    else if (name == "Internal") {
        type = TopAbs_INTERNAL;
    }
    else if (name == "External") {
        type = TopAbs_EXTERNAL;
    }
    else {
        throw Py::AttributeError("Invalid orientation type");
    }

    sh.Orientation(type);
    getTopoShapePtr()->setShape(sh);
}

static Py::List getElements(const TopoShape &sh,
                            TopAbs_ShapeEnum type,
                            TopAbs_ShapeEnum avoid = TopAbs_SHAPE)
{
    Py::List ret;
    for(auto &shape : sh.getSubTopoShapes(type, avoid))
        ret.append(shape2pyshape(shape));
    return ret;
}

PyObject *TopoShapePy::getChildShapes(PyObject *args)
{
    const char *type;
    const char *avoid = nullptr;
    if (!PyArg_ParseTuple(args, "s|s", &type, &avoid))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(
                getElements(*getTopoShapePtr(),
                    TopoShape::shapeType(type),
                    avoid && avoid[0] ? TopoShape::shapeType(avoid) : TopAbs_SHAPE));
    }PY_CATCH_OCC;
}

Py::List TopoShapePy::getSubShapes(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_SHAPE);
}

Py::List TopoShapePy::getFaces(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_FACE);
}

Py::List TopoShapePy::getVertexes(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_VERTEX);
}

Py::List TopoShapePy::getShells(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_SHELL);
}

Py::List TopoShapePy::getSolids(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_SOLID);
}

Py::List TopoShapePy::getCompSolids(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_COMPSOLID);
}

Py::List TopoShapePy::getEdges(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_EDGE);
}

Py::List TopoShapePy::getWires(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_WIRE);
}

Py::List TopoShapePy::getCompounds(void) const
{
    return getElements(*getTopoShapePtr(),TopAbs_COMPOUND);
}

Py::Float TopoShapePy::getLength(void) const
{
    const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
    if (shape.IsNull())
        throw Py::RuntimeError("shape is invalid");
    GProp_GProps props;
    BRepGProp::LinearProperties(shape, props);
    return Py::Float(props.Mass());
}

Py::Float TopoShapePy::getArea(void) const
{
    const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
    if (shape.IsNull())
        throw Py::RuntimeError("shape is invalid");
    GProp_GProps props;
    BRepGProp::SurfaceProperties(shape, props);
    return Py::Float(props.Mass());
}

Py::Float TopoShapePy::getVolume(void) const
{
    const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
    if (shape.IsNull())
        throw Py::RuntimeError("shape is invalid");
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return Py::Float(props.Mass());
}

PyObject *TopoShapePy::getElementHistory(PyObject *args) {
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return 0;

    PY_TRY {
        Data::MappedName original;
        std::vector<Data::MappedName> history;
        long tag = getTopoShapePtr()->getElementHistory(name,&original,&history);
        if(!tag)
            Py_Return;
        Py::Tuple ret(3);
        ret.setItem(0,Py::Int(tag));
        std::string tmp;
        ret.setItem(1,Py::String(original.toString(tmp)));
        Py::List pyHistory;
        for(auto &h : history) {
            tmp.clear();
            pyHistory.append(Py::String(h.toString(tmp)));
        }
        ret.setItem(2,pyHistory);
        return Py::new_reference_to(ret);
    }PY_CATCH_OCC
}

struct PyShapeMapper: Part::ShapeMapper {
    bool populate(bool generated, PyObject *pyobj) {
        if(!pyobj || pyobj == Py_None)
            return true;
        try {
            Py::Sequence seq(pyobj);
            for(size_t i=0, count=seq.size(); i<count; ++i) {
                Py::Sequence item(seq[i].ptr());
                if(item.size() != 2)
                    return false;

                Part::ShapeMapper::populate(generated,
                        getPyShapes(item[0].ptr()), getPyShapes(item[1].ptr()));
            }
        } catch (Py::Exception &) {
            PyErr_Clear();
            return false;
        }
        return true;
    }

    void init(PyObject *g, PyObject *m) {
        const char *msg = "Expect input mapping to be a list of tuple(srcShape|shapeList, dstShape|shapeList)";
        if(!populate(true,g) || !populate(false,m))
            throw Py::TypeError(msg);
    }
};

PyObject *TopoShapePy::mapShapes(PyObject *args) {
    PyObject *generated;
    PyObject *modified;
    const char *op = nullptr;
    if (!PyArg_ParseTuple(args, "OO|s", &generated, &modified, &op))
        return 0;
    PY_TRY {
        PyShapeMapper mapper;
        mapper.init(generated, modified);
        TopoShape &self = *getTopoShapePtr();
        TopoShape s(self.Tag, self.Hasher);
        s.makESHAPE(self.getShape(), mapper, mapper.shapes, op);
        self = s;
        return IncRef();
    }PY_CATCH_OCC
}

PyObject *TopoShapePy::mapSubElement(PyObject *args) {
    const char *op = nullptr;
    PyObject *sh;
    if (!PyArg_ParseTuple(args, "O|s", &sh,&op))
        return 0;
    PY_TRY {
        getTopoShapePtr()->mapSubElement(getPyShapes(sh),op);
        return IncRef();
    }PY_CATCH_OCC
}

PyObject* TopoShapePy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(TopoShapePy::Type)) &&
        PyObject_TypeCheck(w, &(TopoShapePy::Type))) {

        const auto &s1 = static_cast<TopoShapePy*>(v)->getTopoShapePtr()->getShape();
        const auto &s2 = static_cast<TopoShapePy*>(w)->getTopoShapePtr()->getShape();

        PyObject *res=nullptr;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Shape");
            return 0;
        }
        else if (op == Py_EQ) {
            res = (s1.IsEqual(s2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (!s1.IsEqual(s2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
    }
    else {
        // This always returns False
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

PyObject *TopoShapePy::getCustomAttributes(const char* attr) const
{
    if (!attr)
        return nullptr;
    PY_TRY {
        return getTopoShapePtr()->getPySubShape(attr,true);
    } PY_CATCH_OCC
    return 0;
}

int TopoShapePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
