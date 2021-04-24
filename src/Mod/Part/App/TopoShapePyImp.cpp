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
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRepBuilderAPI_Copy.hxx>
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
#if OCC_VERSION_HEX >= 0x060801
# include <BRepExtrema_ShapeProximity.hxx>
#endif
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
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <App/PropertyStandard.h>
#include <CXX/Extensions.hxx>

#include "TopoShape.h"
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

int TopoShapePy::PyInit(PyObject* args, PyObject*)
{
    PyObject *pcObj=0;
    if (!PyArg_ParseTuple(args, "|O", &pcObj))
        return -1;

    auto shapes = getPyShapes(pcObj);

    if (pcObj) {
        TopoShape shape;
        PY_TRY {
            if(PyObject_TypeCheck(pcObj,&TopoShapePy::Type)) {
                shape = *static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
            }else{
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
        }_PY_CATCH_OCC(return(-1))

        getTopoShapePtr()->setShape(shape.getShape());
    }

    return 0;
}

PyObject* TopoShapePy::copy(PyObject *args)
{
    PyObject* copyGeom = Py_True;
    PyObject* copyMesh = Py_False;
    if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &copyGeom, &PyBool_Type, &copyMesh))
        return NULL;

    const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return 0;
    }

    if (!shape.IsNull()) {
#if OCC_VERSION_HEX >= 0x070000
        BRepBuilderAPI_Copy c(shape,
                              PyObject_IsTrue(copyGeom) ? Standard_True : Standard_False,
                              PyObject_IsTrue(copyMesh) ? Standard_True : Standard_False);
#else
        BRepBuilderAPI_Copy c(shape,
                              PyObject_IsTrue(copyGeom) ? Standard_True : Standard_False);
#endif
        static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->setShape(c.Shape());
    }
    return cpy;
}

PyObject* TopoShapePy::cleaned(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return 0;
    }

    if (!shape.IsNull()) {
        BRepBuilderAPI_Copy c(shape);
        const TopoDS_Shape& copiedShape = c.Shape();
        BRepTools::Clean(copiedShape); // remove triangulation
        static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->setShape(c.Shape());
    }
    return cpy;
}

PyObject* TopoShapePy::replaceShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return NULL;

    try {
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
        PyObject* inst = type->tp_new(type, this, 0);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->replaceShape(shapes));
        return inst;
    }
    catch (const Py::Exception&) {
        return 0;
    }
    catch (...) {
        PyErr_SetString(PartExceptionOCCError, "failed to replace shape");
        return 0;
    }
}

PyObject* TopoShapePy::removeShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return NULL;

    try {
        Py::Sequence list(l);
        std::vector<TopoDS_Shape> shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::TopoShape sh(*it);
            shapes.push_back(
                sh.extensionObject()->getTopoShapePtr()->getShape()
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, 0);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->removeShape(shapes));
        return inst;
    }
    catch (...) {
        PyErr_SetString(PartExceptionOCCError, "failed to remove shape");
        return 0;
    }
}

PyObject*  TopoShapePy::read(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    getTopoShapePtr()->read(EncodedName.c_str());
    Py_Return;
}

PyObject* TopoShapePy::writeInventor(PyObject * args, PyObject * keywds)
{
    static char *kwlist[] = {"Mode", "Deviation", "Angle", "FaceColors", NULL};

    double dev=0.3, angle=0.4;
    int mode=2;
    PyObject* pylist=nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iddO", kwlist,
                                     &mode,&dev,&angle,&pylist))
        return NULL;

    std::vector<App::Color> faceColors;
    if (pylist) {
        App::PropertyColorList prop;
        prop.setPyObject(pylist);
        faceColors = prop.getValues();
    }

    std::stringstream result;
    BRepMesh_IncrementalMesh(getTopoShapePtr()->getShape(),dev);
    if (mode == 0)
        getTopoShapePtr()->exportFaceSet(dev, angle, faceColors, result);
    else if (mode == 1)
        getTopoShapePtr()->exportLineSet(result);
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
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write iges file
        getTopoShapePtr()->exportIges(EncodedName.c_str());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::exportStep(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write step file
        getTopoShapePtr()->exportStep(EncodedName.c_str());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }

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
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PartExceptionOCCError,e.what());
            return NULL;
        }

        Py_Return;
    }

    PyErr_Clear();

    PyObject* input;
    if (PyArg_ParseTuple(args, "O", &input)) {
        try {
            // write brep
            Base::PyStreambuf buf(input);
            std::ostream str(0);
            str.rdbuf(&buf);
            getTopoShapePtr()->exportBrep(str);
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PartExceptionOCCError,e.what());
            return NULL;
        }

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return NULL;
}

PyObject*  TopoShapePy::exportBinary(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return NULL;

    try {
        // read binary brep
        std::ofstream str(input, std::ios::out | std::ios::binary);
        getTopoShapePtr()->exportBinary(str);
        str.close();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::dumpToString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        std::stringstream str;
        getTopoShapePtr()->dump(str);
        return Py::new_reference_to(Py::String(str.str()));
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject*  TopoShapePy::exportBrepToString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        // write brep file
        std::stringstream str;
        getTopoShapePtr()->exportBrep(str);
        return Py::new_reference_to(Py::String(str.str()));
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
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
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PartExceptionOCCError,e.what());
            return NULL;
        }

        Py_Return;
    }

    PyErr_Clear();
    PyObject* input;
    if (PyArg_ParseTuple(args, "O", &input)) {
        try {
            // read brep
            Base::PyStreambuf buf(input);
            std::istream str(0);
            str.rdbuf(&buf);
            getTopoShapePtr()->importBrep(str);
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PartExceptionOCCError,e.what());
            return NULL;
        }

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return NULL;
}

PyObject*  TopoShapePy::importBinary(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return NULL;

    try {
        // read binary brep
        std::ifstream str(input, std::ios::in | std::ios::binary);
        getTopoShapePtr()->importBinary(str);
        str.close();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::importBrepFromString(PyObject *args)
{
    char* input;
    int indicator=1;
    if (!PyArg_ParseTuple(args, "s|i", &input, &indicator))
        return NULL;

    try {
        // read brep
        std::stringstream str(input);
        getTopoShapePtr()->importBrep(str,indicator);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return NULL;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject*  TopoShapePy::__getstate__(PyObject *args) {
    return exportBrepToString(args);
}


PyObject*  TopoShapePy::__setstate__(PyObject *args) {
    if (! getTopoShapePtr()) {
        PyErr_SetString(Base::BaseExceptionFreeCADError,"no c++ object");
        return 0;
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
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        // write stl file
        getTopoShapePtr()->exportStl(EncodedName.c_str(), deflection);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PartExceptionOCCError,e.what());
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* TopoShapePy::extrude(PyObject *args)
{
    PyObject *pVec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pVec)) {
        try {
            Base::Vector3d vec = static_cast<Base::VectorPy*>(pVec)->value();
            TopoDS_Shape shape = this->getTopoShapePtr()->makePrism(gp_Vec(vec.x,vec.y,vec.z));
            TopAbs_ShapeEnum type = shape.ShapeType();
            switch (type)
            {
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
            return 0;
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject* TopoShapePy::revolve(PyObject *args)
{
    PyObject *pPos,*pDir;
    double d=360;
    if (PyArg_ParseTuple(args, "O!O!|d", &(Base::VectorPy::Type), &pPos, &(Base::VectorPy::Type), &pDir,&d)) {
        try {
            const TopoDS_Shape& input = this->getTopoShapePtr()->getShape();
            if (input.IsNull()) {
                PyErr_SetString(PartExceptionOCCError, "empty shape cannot be revolved");
                return 0;
            }

            TopExp_Explorer xp;
            xp.Init(input,TopAbs_SOLID);
            if (xp.More()) {
                PyErr_SetString(PartExceptionOCCError, "shape must not contain solids");
                return 0;
            }
            xp.Init(input,TopAbs_COMPSOLID);
            if (xp.More()) {
                PyErr_SetString(PartExceptionOCCError, "shape must not contain compound solids");
                return 0;
            }

            Base::Vector3d pos = static_cast<Base::VectorPy*>(pPos)->value();
            Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
            TopoDS_Shape shape = this->getTopoShapePtr()->revolve(
                gp_Ax1(gp_Pnt(pos.x,pos.y,pos.z), gp_Dir(dir.x,dir.y,dir.z)),d*(M_PI/180));
            TopAbs_ShapeEnum type = shape.ShapeType();
            switch (type)
            {
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
            return 0;
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject*  TopoShapePy::check(PyObject *args)
{
    PyObject* runBopCheck = Py_False;
    if (!PyArg_ParseTuple(args, "|O!", &(PyBool_Type), &runBopCheck))
        return NULL;
    if (!getTopoShapePtr()->getShape().IsNull()) {
        std::stringstream str;
        if (!getTopoShapePtr()->analyze(PyObject_IsTrue(runBopCheck) ? true : false, str)) {
            PyErr_SetString(PyExc_ValueError, str.str().c_str());
            return NULL;
        }
    }

    Py_Return;
}

PyObject*  TopoShapePy::fuse(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a fuse operation:
            TopoDS_Shape fusShape = this->getTopoShapePtr()->fuse(shape);
            return new TopoShapePy(new TopoShape(fusShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
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
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
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
                return 0;
           }
        }
        try {
            TopoDS_Shape multiFusedShape = this->getTopoShapePtr()->fuse(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiFusedShape));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoShapePy::multiFuse(PyObject *args)
{
    double tolerance = 0.0;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance))
        return NULL;
    std::vector<TopoDS_Shape> shapeVec;
    Py::Sequence shapeSeq(pcObj);
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
        }
        else {
            PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
            return 0;
       }
    }
    try {
        TopoDS_Shape multiFusedShape = this->getTopoShapePtr()->fuse(shapeVec,tolerance);
        return new TopoShapePy(new TopoShape(multiFusedShape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::oldFuse(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    try {
        // Let's call algorithm computing a fuse operation:
        TopoDS_Shape fusShape = this->getTopoShapePtr()->oldFuse(shape);
        return new TopoShapePy(new TopoShape(fusShape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::common(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a common operation:
            TopoDS_Shape comShape = this->getTopoShapePtr()->common(shape);
            return new TopoShapePy(new TopoShape(comShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape commonShape = this->getTopoShapePtr()->common(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(commonShape));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
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
                return 0;
           }
        }
        try {
            TopoDS_Shape multiCommonShape = this->getTopoShapePtr()->common(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiCommonShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoShapePy::section(PyObject *args)
{
    PyObject *pcObj;
    PyObject *approx = Py_False;
    if (PyArg_ParseTuple(args, "O!|O!", &(TopoShapePy::Type), &pcObj, &(PyBool_Type), &approx)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a section operation:
            TopoDS_Shape secShape = this->getTopoShapePtr()->section(shape,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(secShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d|O!", &(TopoShapePy::Type), &pcObj, &tolerance, &(PyBool_Type), &approx)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape sectionShape = this->getTopoShapePtr()->section(shapeVec,tolerance,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(sectionShape));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
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
                return 0;
           }
        }
        try {
            TopoDS_Shape multiSectionShape = this->getTopoShapePtr()->section(shapeVec,tolerance,PyObject_IsTrue(approx) ? true : false);
            return new TopoShapePy(new TopoShape(multiSectionShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoShapePy::slice(PyObject *args)
{
    PyObject *dir;
    double d;
    if (!PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &dir, &d))
        return NULL;

    try {
        Base::Vector3d vec = Py::Vector(dir, false).toVector();
        std::list<TopoDS_Wire> slice = this->getTopoShapePtr()->slice(vec, d);
        Py::List wire;
        for (std::list<TopoDS_Wire>::iterator it = slice.begin(); it != slice.end(); ++it) {
            wire.append(Py::asObject(new TopoShapeWirePy(new TopoShape(*it))));
        }

        return Py::new_reference_to(wire);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::slices(PyObject *args)
{
    PyObject *dir, *dist;
    if (!PyArg_ParseTuple(args, "O!O", &(Base::VectorPy::Type), &dir, &dist))
        return NULL;

    try {
        Base::Vector3d vec = Py::Vector(dir, false).toVector();
        Py::Sequence list(dist);
        std::vector<double> d;
        d.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it)
            d.push_back((double)Py::Float(*it));
        TopoDS_Compound slice = this->getTopoShapePtr()->slices(vec, d);
        return new TopoShapeCompoundPy(new TopoShape(slice));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::cut(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        try {
            // Let's call algorithm computing a cut operation:
            TopoDS_Shape cutShape = this->getTopoShapePtr()->cut(shape);
            return new TopoShapePy(new TopoShape(cutShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_Clear();
    double tolerance = 0.0;
    if (PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &pcObj, &tolerance)) {
        std::vector<TopoDS_Shape> shapeVec;
        shapeVec.push_back(static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape());
        try {
            TopoDS_Shape cutShape = this->getTopoShapePtr()->cut(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(cutShape));
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
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
                return 0;
           }
        }
        try {
            TopoDS_Shape multiCutShape = this->getTopoShapePtr()->cut(shapeVec,tolerance);
            return new TopoShapePy(new TopoShape(multiCutShape));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoShapePy::generalFuse(PyObject *args)
{
    double tolerance = 0.0;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O|d", &pcObj, &tolerance))
        return NULL;
    std::vector<TopoDS_Shape> shapeVec;
    Py::Sequence shapeSeq(pcObj);
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            shapeVec.push_back(static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->getShape());
        }
        else {
            PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
            return 0;
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::sewShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        getTopoShapePtr()->sewShape();
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::childShapes(PyObject *args)
{
    PyObject* cumOri = Py_True;
    PyObject* cumLoc = Py_True;
    if (!PyArg_ParseTuple(args, "|O!O!", &(PyBool_Type), &cumOri,
                                         &(PyBool_Type), &cumLoc))
        return NULL;

    try {
        const TopoDS_Shape& shape = getTopoShapePtr()->getShape();
        if (shape.IsNull()) {
            PyErr_SetString(PyExc_ValueError, "Shape is null");
            return NULL;
        }
        TopoDS_Iterator it(shape,
            PyObject_IsTrue(cumOri) ? Standard_True : Standard_False,
            PyObject_IsTrue(cumLoc) ? Standard_True : Standard_False);
        Py::List list;
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            if (!aChild.IsNull()) {
                TopAbs_ShapeEnum type = aChild.ShapeType();
                PyObject* pyChild = 0;
                switch (type)
                {
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

namespace Part {
std::vector<PyTypeObject*> buildShapeEnumTypeMap()
{
   std::vector<PyTypeObject*> typeMap;
   typeMap.push_back(&TopoShapeCompoundPy::Type);             //TopAbs_COMPOUND
   typeMap.push_back(&TopoShapeCompSolidPy::Type);            //TopAbs_COMPSOLID
   typeMap.push_back(&TopoShapeSolidPy::Type);                //TopAbs_SOLID
   typeMap.push_back(&TopoShapeShellPy::Type);                //TopAbs_SHELL
   typeMap.push_back(&TopoShapeFacePy::Type);                 //TopAbs_FACE
   typeMap.push_back(&TopoShapeWirePy::Type);                 //TopAbs_WIRE
   typeMap.push_back(&TopoShapeEdgePy::Type);                 //TopAbs_EDGE
   typeMap.push_back(&TopoShapeVertexPy::Type);               //TopAbs_VERTEX
   typeMap.push_back(&TopoShapePy::Type);                     //TopAbs_SHAPE
   return typeMap;
}
}

PyObject*  TopoShapePy::ancestorsOfType(PyObject *args)
{
    PyObject *pcObj;
    PyObject *type;
    if (!PyArg_ParseTuple(args, "O!O!", &(TopoShapePy::Type), &pcObj, &PyType_Type, &type))
        return NULL;

    try {
        const TopoDS_Shape& model = getTopoShapePtr()->getShape();
        const TopoDS_Shape& shape = static_cast<TopoShapePy*>(pcObj)->
                getTopoShapePtr()->getShape();
        if (model.IsNull() || shape.IsNull()) {
            PyErr_SetString(PyExc_ValueError, "Shape is null");
            return NULL;
        }

        static std::vector<PyTypeObject*> typeMap = buildShapeEnumTypeMap();
        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;
        for (auto it = typeMap.begin(); it != typeMap.end(); ++it) {
            if (PyType_IsSubtype(pyType, *it)) {
                auto index = std::distance(typeMap.begin(), it);
                shapetype = static_cast<TopAbs_ShapeEnum>(index);
                break;
            }
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
        return NULL;
    }
}

PyObject*  TopoShapePy::removeInternalWires(PyObject *args)
{
    double minArea;
    if (!PyArg_ParseTuple(args, "d",&minArea))
        return NULL;

    try {
        bool ok = getTopoShapePtr()->removeInternalWires(minArea);
        PyObject* ret = ok ? Py_True : Py_False;
        Py_INCREF(ret);
        return ret;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::mirror(PyObject *args)
{
    PyObject *v1, *v2;
    if (!PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type),&v1,
                                        &(Base::VectorPy::Type),&v2))
        return NULL;

    Base::Vector3d base = Py::Vector(v1,false).toVector();
    Base::Vector3d norm = Py::Vector(v2,false).toVector();

    try {
        gp_Ax2 ax2(gp_Pnt(base.x,base.y,base.z), gp_Dir(norm.x,norm.y,norm.z));
        TopoDS_Shape shape = this->getTopoShapePtr()->mirror(ax2);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::transformGeometry(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&obj))
        return NULL;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->transformGShape(mat);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::transformShape(PyObject *args)
{
    PyObject *obj;
    PyObject *copy = Py_False;
    PyObject *checkScale = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!O", &(Base::MatrixPy::Type),&obj,&(PyBool_Type), &copy,&checkScale))
        return NULL;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    PY_TRY {
        this->getTopoShapePtr()->transformShape(mat, PyObject_IsTrue(copy) ? true : false, 
                PyObject_IsTrue(checkScale));
        return IncRef();
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::transformed(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"matrix", "copy", "checkScale", "op", NULL};
    PyObject* pymat;
    PyObject* copy = Py_False;
    PyObject* checkScale = Py_False;
    const char *op = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O!|OOs", kwlist,
                &Base::MatrixPy::Type, &pymat,&copy,&checkScale,&op))
        return 0;
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(pymat)->value();
    (void)op;
    PY_TRY {
        TopoShape s(*getTopoShapePtr());
        s.transformShape(mat,PyObject_IsTrue(copy),PyObject_IsTrue(checkScale));
        return Py::new_reference_to(shape2pyshape(s));
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::translate(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return 0;

    Base::Vector3d vec;
    if (PyObject_TypeCheck(obj, &(Base::VectorPy::Type))) {
        vec = static_cast<Base::VectorPy*>(obj)->value();
    }
    else if (PyObject_TypeCheck(obj, &PyTuple_Type)) {
        vec = Base::getVectorFromTuple<double>(obj);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "either vector or tuple expected");
        return 0;
    }

    gp_Trsf mov;
    mov.SetTranslation(gp_Vec(vec.x,vec.y,vec.z));
    TopLoc_Location loc(mov);
    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Move(loc);
    getTopoShapePtr()->setShape(shape);
    return IncRef();
}

PyObject*  TopoShapePy::rotate(PyObject *args)
{
    PyObject *obj1, *obj2;
    double angle;
    if (!PyArg_ParseTuple(args, "OOd", &obj1, &obj2, &angle))
        return NULL;

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
        getTopoShapePtr()->setShape(shape);
        return IncRef();
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::scale(PyObject *args)
{
    double factor;
    PyObject* p=0;
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
            getTopoShapePtr()->setShape(BRepScale.Shape());
        }
        return IncRef();
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::translated(PyObject *args) {
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->translate(args);
}

PyObject*  TopoShapePy::rotated(PyObject *args) {
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->rotate(args);
}

PyObject*  TopoShapePy::scaled(PyObject *args) {
    Py::Object pyobj(shape2pyshape(*getTopoShapePtr()));
    return static_cast<TopoShapePy*>(pyobj.ptr())->scale(args);
}

PyObject* TopoShapePy::makeFillet(PyObject *args)
{
    // use two radii for all edges
    double radius1, radius2;
    PyObject *obj;
    if (PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
        try {
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
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
    }

    PyErr_Clear();
    // use one radius for all edges
    double radius;
    if (PyArg_ParseTuple(args, "dO", &radius, &obj)) {
        try {
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
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return NULL;
}

PyObject* TopoShapePy::makeChamfer(PyObject *args)
{
    // use two radii for all edges
    double radius1, radius2;
    PyObject *obj;
    if (PyArg_ParseTuple(args, "ddO", &radius1, &radius2, &obj)) {
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
                        mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), face);
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkChamfer.Shape()));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
    }

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
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return NULL;
}

PyObject* TopoShapePy::makeThickness(PyObject *args)
{
    PyObject *obj;
    double offset, tolerance;
    PyObject* inter = Py_False;
    PyObject* self_inter = Py_False;
    short offsetMode = 0, join = 0;
    if (!PyArg_ParseTuple(args, "Odd|O!O!hh",
        &obj,
        &offset, &tolerance,
        &(PyBool_Type), &inter,
        &(PyBool_Type), &self_inter,
        &offsetMode, &join))
        return 0;

    try {
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::makeOffsetShape(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"offset", "tolerance", "inter", "self_inter", "offsetMode", "join", "fill", NULL};
    double offset, tolerance;
    PyObject* inter = Py_False;
    PyObject* self_inter = Py_False;
    PyObject* fill = Py_False;
    short offsetMode = 0, join = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "dd|O!O!hhO!", kwlist,
        &offset, &tolerance,
        &(PyBool_Type), &inter,
        &(PyBool_Type), &self_inter,
        &offsetMode, &join,
        &(PyBool_Type), &fill))
        return 0;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->makeOffsetShape(offset, tolerance,
            PyObject_IsTrue(inter) ? true : false,
            PyObject_IsTrue(self_inter) ? true : false, offsetMode, join,
            PyObject_IsTrue(fill) ? true : false);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::makeOffset2D(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"offset", "join", "fill", "openResult", "intersection", NULL};
    double offset;
    PyObject* fill = Py_False;
    PyObject* openResult = Py_False;
    PyObject* inter = Py_False;
    short join = 0;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "d|hO!O!O!", kwlist,
        &offset,
        &join,
        &(PyBool_Type), &fill,
        &(PyBool_Type), &openResult,
        &(PyBool_Type), &inter))
        return 0;

    try {
        TopoDS_Shape resultShape = this->getTopoShapePtr()->makeOffset2D(offset, join,
            PyObject_IsTrue(fill) ? true : false,
            PyObject_IsTrue(openResult) ? true : false,
            PyObject_IsTrue(inter) ? true : false);
        return new_reference_to(shape2pyshape(resultShape));
    }
    PY_CATCH_OCC;
}

PyObject*  TopoShapePy::reverse(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Reverse();
    getTopoShapePtr()->setShape(shape);
    Py_Return;
}

PyObject*  TopoShapePy::reversed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape = shape.Reversed();

    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;

    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
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
        return NULL;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Complement();
    getTopoShapePtr()->setShape(shape);
    Py_Return;
}

PyObject*  TopoShapePy::nullify(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TopoDS_Shape shape = getTopoShapePtr()->getShape();
    shape.Nullify();
    getTopoShapePtr()->setShape(shape);
    Py_Return;
}

PyObject*  TopoShapePy::isNull(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool null = getTopoShapePtr()->isNull();
    return Py_BuildValue("O", (null ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        if (getTopoShapePtr()->getShape().IsNull())
            Standard_Failure::Raise("Cannot determine the 'Closed'' flag of an empty shape");
        return Py_BuildValue("O", (getTopoShapePtr()->isClosed() ? Py_True : Py_False));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "check failed, shape may be empty");
        return NULL;
    }
}

PyObject*  TopoShapePy::isEqual(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = (getTopoShapePtr()->getShape().IsEqual(shape));
    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isSame(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = getTopoShapePtr()->getShape().IsSame(shape);
    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isPartner(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
    Standard_Boolean test = getTopoShapePtr()->getShape().IsPartner(shape);
    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isValid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        return Py_BuildValue("O", (getTopoShapePtr()->isValid() ? Py_True : Py_False));
    } PY_CATCH_OCC
}

PyObject*  TopoShapePy::isCoplanar(PyObject *args)
{
    PyObject *pyObj;
    double tol = -1;
    if (!PyArg_ParseTuple(args, "O!|d", &TopoShapePy::Type, &pyObj, &tol))
        return NULL;
    PY_TRY {
        return Py::new_reference_to(Py::Boolean(getTopoShapePtr()->isCoplanar(
                    *static_cast<TopoShapePy*>(pyObj)->getTopoShapePtr(),tol)));
    }PY_CATCH_OCC
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

PyObject*  TopoShapePy::findPlane(PyObject *args)
{
    double tol = -1;
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return NULL;
    PY_TRY {
        gp_Pln pln;
        if(getTopoShapePtr()->findPlane(pln,tol))
            return new PlanePy(new GeomPlane(new Geom_Plane(pln)));
        Py_Return;
    }PY_CATCH_OCC
}

PyObject*  TopoShapePy::fix(PyObject *args)
{
    double prec, mintol, maxtol;
    if (!PyArg_ParseTuple(args, "ddd", &prec, &mintol, &maxtol))
        return NULL;
    try {
        return Py_BuildValue("O", (getTopoShapePtr()->fix(prec, mintol, maxtol) ? Py_True : Py_False));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "check failed, shape may be empty");
        return NULL;
    }
}

PyObject* TopoShapePy::hashCode(PyObject *args)
{
    int upper = IntegerLast();
    if (!PyArg_ParseTuple(args, "|i",&upper))
        return 0;
    int hc = getTopoShapePtr()->getShape().HashCode(upper);
    return Py_BuildValue("i", hc);
}

PyObject* TopoShapePy::tessellate(PyObject *args)
{
    try {
        float tolerance;
        PyObject* ok = Py_False;
        if (!PyArg_ParseTuple(args, "f|O!",&tolerance,&PyBool_Type,&ok))
            return 0;
        std::vector<Base::Vector3d> Points;
        std::vector<Data::ComplexGeoData::Facet> Facets;
        if (PyObject_IsTrue(ok))
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::project(PyObject *args)
{
    PyObject *obj;

    BRepAlgo_NormalProjection algo;
    algo.Init(this->getTopoShapePtr()->getShape());
    if (PyArg_ParseTuple(args, "O", &obj)) {
        try {
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
        }
        catch (Standard_Failure&) {
            PyErr_SetString(PartExceptionOCCError, "Failed to project shape");
            return NULL;
        }
    }

    return 0;
}

PyObject* TopoShapePy::makeParallelProjection(PyObject *args)
{
    PyObject *pShape, *pDir;
    if (PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapePy::Type), &pShape, &Base::VectorPy::Type, &pDir)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            const TopoDS_Shape& wire = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->getShape();
            Base::Vector3d vec = Py::Vector(pDir,false).toVector();
            BRepProj_Projection proj(wire, shape, gp_Dir(vec.x,vec.y,vec.z));
            TopoDS_Shape projected = proj.Shape();
            return new TopoShapePy(new TopoShape(projected));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject* TopoShapePy::makePerspectiveProjection(PyObject *args)
{
    PyObject *pShape, *pDir;
    if (PyArg_ParseTuple(args, "O!O!", &(Part::TopoShapePy::Type), &pShape, &Base::VectorPy::Type, &pDir)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
            const TopoDS_Shape& wire = static_cast<TopoShapePy*>(pShape)->getTopoShapePtr()->getShape();
            Base::Vector3d vec = Py::Vector(pDir,false).toVector();
            BRepProj_Projection proj(wire, shape, gp_Pnt(vec.x,vec.y,vec.z));
            TopoDS_Shape projected = proj.Shape();
            return new TopoShapePy(new TopoShape(projected));
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    return 0;
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
                                     &Base::VectorPy::Type, &pView,
                                     &Base::VectorPy::Type, &pPos,
                                     &Base::VectorPy::Type, &pUp,
                                     &type,
                                     &PyBool_Type, &vis,
                                     &PyBool_Type, &in3d))
        return nullptr;

    try {
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
        if (pPos) {
            p = Py::Vector(pPos,false).toVector();
        }
        Base::Vector3d u(0.0, 1.0, 0.0);
        if (pUp) {
            u = Py::Vector(pUp,false).toVector();
        }
        Base::Vector3d v = Py::Vector(pView,false).toVector();

        const TopoDS_Shape& shape = this->getTopoShapePtr()->getShape();
        HLRAppli_ReflectLines reflect(shape);
        reflect.SetAxes(v.x, v.y, v.z, p.x, p.y, p.z, u.x, u.y, u.z);
        reflect.Perform();
        TopoDS_Shape lines = reflect.GetCompoundOf3dEdges(t, PyObject_IsTrue(vis) ? Standard_True : Standard_False,
                                                          PyObject_IsTrue(in3d) ? Standard_True : Standard_False);
        return new TopoShapePy(new TopoShape(lines));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* TopoShapePy::makeShapeFromMesh(PyObject *args)
{
    PyObject *tup;
    float tolerance;
    if (!PyArg_ParseTuple(args, "O!f",&PyTuple_Type, &tup, &tolerance))
        return 0;

    try {
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

        getTopoShapePtr()->setFaces(Points, Facets,tolerance);
        Py_Return;
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::makeWires(PyObject *args) {
    const char *op = 0;
    if (!PyArg_ParseTuple(args, "s", &op))
        return NULL;
    PY_TRY {
        return Py::new_reference_to(shape2pyshape(getTopoShapePtr()->makEWires(op)));
    }PY_CATCH_OCC
}

PyObject* TopoShapePy::toNurbs(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        // Convert into nurbs
        TopoDS_Shape nurbs = this->getTopoShapePtr()->toNurbs();
        return new TopoShapePy(new TopoShape(nurbs));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::isInside(PyObject *args)
{
    PyObject *point;
    double tolerance;
    PyObject* checkFace = Py_False;
    TopAbs_State stateIn = TopAbs_IN;
    if (!PyArg_ParseTuple(args, "O!dO!", &(Base::VectorPy::Type), &point, &tolerance,  &PyBool_Type, &checkFace))
        return NULL;
    try {
        TopoDS_Shape shape = getTopoShapePtr()->getShape();
        BRepClass3d_SolidClassifier solidClassifier(shape);
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(point)->value();
        gp_Pnt vertex = gp_Pnt(pnt.x,pnt.y,pnt.z);
        solidClassifier.Perform(vertex, tolerance);
        Standard_Boolean test = (solidClassifier.State() == stateIn);
        if (PyObject_IsTrue(checkFace) && (solidClassifier.IsOnAFace()))
            test = Standard_True;
        return Py_BuildValue("O", (test ? Py_True : Py_False));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return NULL;
    }
}

PyObject* TopoShapePy::removeSplitter(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        // Remove redundant splitter
        TopoDS_Shape shape = this->getTopoShapePtr()->removeSplitter();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::getElement(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return NULL;
    std::string name(input);

    try {
        if (name.size() > 4 && name.substr(0,4) == "Face" && name[4]>=48 && name[4]<=57) {
            std::unique_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(input)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeFacePy(new TopoShape(Shape));
        }
        else if (name.size() > 4 && name.substr(0,4) == "Edge" && name[4]>=48 && name[4]<=57) {
            std::unique_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(input)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeEdgePy(new TopoShape(Shape));
        }
        else if (name.size() > 6 && name.substr(0,6) == "Vertex" && name[6]>=48 && name[6]<=57) {
            std::unique_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(input)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeVertexPy(new TopoShape(Shape));
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
    return 0;
}

PyObject* TopoShapePy::countElement(PyObject *args)
{
    char* input;
    if (!PyArg_ParseTuple(args, "s", &input))
        return NULL;
    PY_TRY {
        return Py::new_reference_to(Py::Int((long)getTopoShapePtr()->countSubShapes(input)));
    } PY_CATCH_OCC
}

PyObject* TopoShapePy::getTolerance(PyObject *args)
{
    int mode;
    PyObject* type=0;
    if (!PyArg_ParseTuple(args, "i|O!", &mode, &PyType_Type, &type))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        if (pyType == 0)
            shapetype = TopAbs_SHAPE;
        else if (PyType_IsSubtype(pyType, &TopoShapeShellPy::Type))
            shapetype = TopAbs_SHELL;
        else if (PyType_IsSubtype(pyType, &TopoShapeFacePy::Type))
            shapetype = TopAbs_FACE;
        else if (PyType_IsSubtype(pyType, &TopoShapeEdgePy::Type))
            shapetype = TopAbs_EDGE;
        else if (PyType_IsSubtype(pyType, &TopoShapeVertexPy::Type))
            shapetype = TopAbs_VERTEX;
        else if (pyType != &TopoShapePy::Type) {
            if (PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
                PyErr_SetString(PyExc_TypeError, "shape type must be Vertex, Edge, Face or Shell");
                return 0;
            }
            else {
                PyErr_SetString(PyExc_TypeError, "type must be a shape type");
                return 0;
            }
        }

        ShapeAnalysis_ShapeTolerance analysis;
        double tolerance = analysis.Tolerance(shape, mode, shapetype);
        return PyFloat_FromDouble(tolerance);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::overTolerance(PyObject *args)
{
    double value;
    PyObject* type=0;
    if (!PyArg_ParseTuple(args, "d|O!", &value, &PyType_Type, &type))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        if (pyType == 0)
            shapetype = TopAbs_SHAPE;
        else if (PyType_IsSubtype(pyType, &TopoShapeShellPy::Type))
            shapetype = TopAbs_SHELL;
        else if (PyType_IsSubtype(pyType, &TopoShapeFacePy::Type))
            shapetype = TopAbs_FACE;
        else if (PyType_IsSubtype(pyType, &TopoShapeEdgePy::Type))
            shapetype = TopAbs_EDGE;
        else if (PyType_IsSubtype(pyType, &TopoShapeVertexPy::Type))
            shapetype = TopAbs_VERTEX;
        else if (pyType != &TopoShapePy::Type) {
            if (PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
                PyErr_SetString(PyExc_TypeError, "shape type must be Vertex, Edge, Face or Shell");
                return 0;
            }
            else {
                PyErr_SetString(PyExc_TypeError, "type must be a shape type");
                return 0;
            }
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::inTolerance(PyObject *args)
{
    double valmin;
    double valmax;
    PyObject* type=0;
    if (!PyArg_ParseTuple(args, "dd|O!", &valmin, &valmax, &PyType_Type, &type))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        if (pyType == 0)
            shapetype = TopAbs_SHAPE;
        else if (PyType_IsSubtype(pyType, &TopoShapeShellPy::Type))
            shapetype = TopAbs_SHELL;
        else if (PyType_IsSubtype(pyType, &TopoShapeFacePy::Type))
            shapetype = TopAbs_FACE;
        else if (PyType_IsSubtype(pyType, &TopoShapeEdgePy::Type))
            shapetype = TopAbs_EDGE;
        else if (PyType_IsSubtype(pyType, &TopoShapeVertexPy::Type))
            shapetype = TopAbs_VERTEX;
        else if (pyType != &TopoShapePy::Type) {
            if (PyType_IsSubtype(pyType, &TopoShapePy::Type)) {
                PyErr_SetString(PyExc_TypeError, "shape type must be Vertex, Edge, Face or Shell");
                return 0;
            }
            else {
                PyErr_SetString(PyExc_TypeError, "type must be a shape type");
                return 0;
            }
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
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::globalTolerance(PyObject *args)
{
    int mode;
    if (!PyArg_ParseTuple(args, "i", &mode))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();

        ShapeAnalysis_ShapeTolerance analysis;
        analysis.Tolerance(shape, mode);
        double tolerance = analysis.GlobalTolerance(mode);
        return PyFloat_FromDouble(tolerance);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::fixTolerance(PyObject *args)
{
    double value;
    PyObject* type=0;
    if (!PyArg_ParseTuple(args, "d|O!", &value, &PyType_Type, &type))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        if (pyType == 0)
            shapetype = TopAbs_SHAPE;
        else if (PyType_IsSubtype(pyType, &TopoShapeWirePy::Type))
            shapetype = TopAbs_WIRE;
        else if (PyType_IsSubtype(pyType, &TopoShapeFacePy::Type))
            shapetype = TopAbs_FACE;
        else if (PyType_IsSubtype(pyType, &TopoShapeEdgePy::Type))
            shapetype = TopAbs_EDGE;
        else if (PyType_IsSubtype(pyType, &TopoShapeVertexPy::Type))
            shapetype = TopAbs_VERTEX;
        else if (PyType_IsSubtype(pyType, &TopoShapePy::Type))
            shapetype = TopAbs_SHAPE;
        else if (pyType != &TopoShapePy::Type) {
            PyErr_SetString(PyExc_TypeError, "type must be a shape type");
            return 0;
        }

        ShapeFix_ShapeTolerance fix;
        fix.SetTolerance(shape, value, shapetype);
        Py_Return;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::limitTolerance(PyObject *args)
{
    double tmin;
    double tmax=0;
    PyObject* type=0;
    if (!PyArg_ParseTuple(args, "d|dO!", &tmin, &tmax, &PyType_Type, &type))
        return NULL;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->getShape();
        TopAbs_ShapeEnum shapetype = TopAbs_SHAPE;

        PyTypeObject* pyType = reinterpret_cast<PyTypeObject*>(type);
        if (pyType == 0)
            shapetype = TopAbs_SHAPE;
        else if (PyType_IsSubtype(pyType, &TopoShapeWirePy::Type))
            shapetype = TopAbs_WIRE;
        else if (PyType_IsSubtype(pyType, &TopoShapeFacePy::Type))
            shapetype = TopAbs_FACE;
        else if (PyType_IsSubtype(pyType, &TopoShapeEdgePy::Type))
            shapetype = TopAbs_EDGE;
        else if (PyType_IsSubtype(pyType, &TopoShapeVertexPy::Type))
            shapetype = TopAbs_VERTEX;
        else if (PyType_IsSubtype(pyType, &TopoShapePy::Type))
            shapetype = TopAbs_SHAPE;
        else if (pyType != &TopoShapePy::Type) {
            PyErr_SetString(PyExc_TypeError, "type must be a shape type");
            return 0;
        }

        ShapeFix_ShapeTolerance fix;
        Standard_Boolean ok = fix.LimitTolerance(shape, tmin, tmax, shapetype);
        return PyBool_FromLong(ok ? 1 : 0);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
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
#if OCC_VERSION_HEX >= 0x060801
#if OCC_VERSION_HEX >= 0x060901
    typedef BRepExtrema_MapOfIntegerPackedMapOfInteger BRepExtrema_OverlappedSubShapes;
#endif
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
#else
    (void)args;
    PyErr_SetString(PyExc_NotImplementedError, "proximity requires OCCT >= 6.8.1");
    return 0;
#endif
}

PyObject* TopoShapePy::distToShape(PyObject *args)
{
    PyObject* ps2;
    gp_Pnt P1,P2;
    BRepExtrema_SupportType supportType1,supportType2;
    TopoDS_Shape suppS1,suppS2;
    Standard_Real minDist = -1, t1,t2,u1,v1,u2,v2;

    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type), &ps2))
        return 0;

    const TopoDS_Shape& s1 = getTopoShapePtr()->getShape();
    TopoShape* ts1 = getTopoShapePtr();
    const TopoDS_Shape& s2 = static_cast<Part::TopoShapePy*>(ps2)->getTopoShapePtr()->getShape();
    TopoShape* ts2 = static_cast<Part::TopoShapePy*>(ps2)->getTopoShapePtr();

    if (s2.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "distToShape: Shape parameter is invalid");
        return 0;
    }
    BRepExtrema_DistShapeShape extss;
    extss.LoadS1(s1);
    extss.LoadS2(s2);
    try {
        extss.Perform();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return 0;
    }
    if (!extss.IsDone()) {
        PyErr_SetString(PyExc_RuntimeError, "BRepExtrema_DistShapeShape failed");
        return 0;
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
        return 0;
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
        return 0;

    try {
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
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject* TopoShapePy::defeaturing(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return NULL;

    try {
        Py::Sequence list(l);
        std::vector<TopoDS_Shape> shapes;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::TopoShape sh(*it);
            shapes.push_back(
                sh.extensionObject()->getTopoShapePtr()->getShape()
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, 0);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->setShape
            (this->getTopoShapePtr()->defeaturing(shapes));
        return inst;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }
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
        throw Py::Exception(Base::BaseExceptionFreeCADError, "cannot determine type of null shape");
    TopAbs_ShapeEnum type = sh.ShapeType();
    std::string name;
    switch (type)
    {
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
        throw Py::Exception(Base::BaseExceptionFreeCADError, "cannot determine orientation of null shape");
    TopAbs_Orientation type = sh.Orientation();
    std::string name;
    switch (type)
    {
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
        throw Py::Exception(Base::BaseExceptionFreeCADError, "cannot determine orientation of null shape");
    std::string name = (std::string)arg;
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

Py::List TopoShapePy::getSubShapes(void) const
{
    Py::List ret;
    const TopoDS_Shape& shape = getTopoShapePtr()->getShape();

    if (!shape.IsNull()) {
        for(TopoDS_Iterator it(shape);it.More();it.Next())
            ret.append(shape2pyshape(it.Value()));
    }

    return ret;
}

Py::List TopoShapePy::getFaces(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_FACE);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* face = new TopoShapeFacePy(new TopoShape(shape));
        face->setNotTracking();
        ret.append(Py::asObject(face));
    }

    return ret;
}

Py::List TopoShapePy::getVertexes(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_VERTEX);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* vertex = new TopoShapeVertexPy(new TopoShape(shape));
        vertex->setNotTracking();
        ret.append(Py::asObject(vertex));
    }

    return ret;
}

Py::List TopoShapePy::getShells(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_SHELL);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* shell = new TopoShapeShellPy(new TopoShape(shape));
        shell->setNotTracking();
        ret.append(Py::asObject(shell));
    }

    return ret;
}

Py::List TopoShapePy::getSolids(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_SOLID);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* solid = new TopoShapeSolidPy(new TopoShape(shape));
        solid->setNotTracking();
        ret.append(Py::asObject(solid));
    }

    return ret;
}

Py::List TopoShapePy::getCompSolids(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_COMPSOLID);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* comps = new TopoShapeCompSolidPy(new TopoShape(shape));
        comps->setNotTracking();
        ret.append(Py::asObject(comps));
    }

    return ret;
}

Py::List TopoShapePy::getEdges(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_EDGE);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* edge = new TopoShapeEdgePy(new TopoShape(shape));
        edge->setNotTracking();
        ret.append(Py::asObject(edge));
    }

    return ret;
}

Py::List TopoShapePy::getWires(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_WIRE);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* wire = new TopoShapeWirePy(new TopoShape(shape));
        wire->setNotTracking();
        ret.append(Py::asObject(wire));
    }

    return ret;
}

Py::List TopoShapePy::getCompounds(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->getShape(),TopAbs_COMPOUND);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        Base::PyObjectBase* comp = new TopoShapeCompoundPy(new TopoShape(shape));
        comp->setNotTracking();
        ret.append(Py::asObject(comp));
    }

    return ret;
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

PyObject *TopoShapePy::getCustomAttributes(const char* attr) const
{
    if (!attr) return 0;
    PY_TRY {
        TopoDS_Shape res = getTopoShapePtr()->getSubShape(attr,true);
        if(!res.IsNull())
            return Py::new_reference_to(shape2pyshape(res));
    }PY_CATCH_OCC
    return 0;
}

int TopoShapePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
