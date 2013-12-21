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
# include <BRepAdaptor_Curve.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepPrimAPI_MakeBox.hxx>
# include <BRepPrimAPI_MakeCone.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepPrimAPI_MakeCylinder.hxx>
# include <BRepPrimAPI_MakeSphere.hxx>
# include <BRepPrimAPI_MakeRevolution.hxx>
# include <BRepPrim_Wedge.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <BRepBuilderAPI_MakeShell.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepOffsetAPI_Sewing.hxx>
# include <BRepFill.hxx>
# include <BRepLib.hxx>
# include <gp_Circ.hxx>
# include <gp_Pnt.hxx>
# include <gp_Lin.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom2d_Line.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_OffsetSurface.hxx>
# include <GeomAPI_PointsToBSplineSurface.hxx>
# include <Handle_Geom_Circle.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_TrimmedCurve.hxx>
# include <Interface_Static.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
# include <Standard_ConstructionError.hxx>
# include <Standard_DomainError.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Shell.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp_Explorer.hxx>
# include <TColgp_HArray2OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <Precision.hxx>
# include <Standard_Version.hxx>
#endif

#include <BRepOffsetAPI_ThruSections.hxx>
#include <BSplCLib.hxx>
#include <GeomFill_AppSurf.hxx>
#include <GeomFill_Line.hxx>
#include <GeomFill_Pipe.hxx>
#include <GeomFill_SectionGenerator.hxx>
#include <NCollection_List.hxx>
#include <BRepFill_Filling.hxx>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>

#include "TopoShape.h"
#include "TopoShapePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeVertexPy.h"
#include "GeometryPy.h"
#include "GeometryCurvePy.h"
#include "BSplineSurfacePy.h"
#include "FeaturePartBox.h"
#include "FeaturePartCut.h"
#include "FeaturePartImportStep.h"
#include "FeaturePartImportIges.h"
#include "FeaturePartImportBrep.h"
#include "ImportIges.h"
#include "ImportStep.h"
#include "edgecluster.h"

#ifdef FCUseFreeType
#  include "FT2FC.h"
#endif

using Base::Console;
using namespace Part;
using namespace std;

extern const char* BRepBuilderAPI_FaceErrorText(BRepBuilderAPI_FaceError fe);

#ifndef M_PI
    #define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
    #define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

/* module functions */
static PyObject * open(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;                         

    PY_TRY {
        //Base::Console().Log("Open in Part with %s",Name);
        Base::FileInfo file(Name);

        // extract ending
        if (file.extension() == "")
            Py_Error(PyExc_Exception,"no file ending");

        if (file.hasExtension("stp") || file.hasExtension("step")) {
            // create new document and add Import feature
            App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
#if 1
            ImportStepParts(pcDoc,Name);
#else
            Part::ImportStep *pcFeature = (Part::ImportStep *)pcDoc->addObject("Part::ImportStep",file.fileNamePure().c_str());
            pcFeature->FileName.setValue(Name);
#endif 
            pcDoc->recompute();
        }
#if 1
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
            App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
            ImportIgesParts(pcDoc,Name);
            pcDoc->recompute();
        }
#endif
        else {
            try {
                TopoShape shape;
                shape.read(Name);

                // create new document set loaded shape
                App::Document *pcDoc = App::GetApplication().newDocument(file.fileNamePure().c_str());
                Part::Feature *object = static_cast<Part::Feature *>(pcDoc->addObject
                    ("Part::Feature",file.fileNamePure().c_str()));
                object->Shape.setValue(shape);
                pcDoc->recompute();
            }
            catch (const Base::Exception& e) {
                Py_Error(PyExc_Exception, e.what());
            }
        }
    } PY_CATCH;

    Py_Return;
}

/* module functions */
static PyObject * insert(PyObject *self, PyObject *args)
{
    const char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "ss",&Name,&DocName))
        return NULL;                         

    PY_TRY {
        //Base::Console().Log("Insert in Part with %s",Name);
        Base::FileInfo file(Name);

        // extract ending
        if (file.extension() == "")
            Py_Error(PyExc_Exception,"no file ending");
        App::Document *pcDoc = App::GetApplication().getDocument(DocName);
        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        if (file.hasExtension("stp") || file.hasExtension("step")) {
#if 1
            ImportStepParts(pcDoc,Name);
#else
            // add Import feature
            Part::ImportStep *pcFeature = (Part::ImportStep *)pcDoc->addObject("Part::ImportStep",file.fileNamePure().c_str());
            pcFeature->FileName.setValue(Name);
#endif 
            pcDoc->recompute();
        }
#if 1
        else if (file.hasExtension("igs") || file.hasExtension("iges")) {
            ImportIgesParts(pcDoc,Name);
            pcDoc->recompute();
        }
#endif
        else {
            try {
                TopoShape shape;
                shape.read(Name);

                Part::Feature *object = static_cast<Part::Feature *>(pcDoc->addObject
                    ("Part::Feature",file.fileNamePure().c_str()));
                object->Shape.setValue(shape);
                pcDoc->recompute();
            }
            catch (const Base::Exception& e) {
                Py_Error(PyExc_Exception, e.what());
            }
        }
    } PY_CATCH;

    Py_Return;
}

/* module functions */
static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    PY_TRY {
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
        shape.write(filename);

    } PY_CATCH;

    Py_Return;
}

/* module functions */
static PyObject * read(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;                         
    PY_TRY {
        TopoShape* shape = new TopoShape();
        shape->read(Name);
        return new TopoShapePy(shape); 
    } PY_CATCH;
}

static PyObject * 
show(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        App::Document *pcDoc = App::GetApplication().getActiveDocument(); 	 
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObj);
        Part::Feature *pcFeature = (Part::Feature *)pcDoc->addObject("Part::Feature", "Shape");
        // copy the data
        //TopoShape* shape = new MeshObject(*pShape->getTopoShapeObjectPtr());
        pcFeature->Shape.setValue(pShape->getTopoShapePtr()->_Shape);
        pcDoc->recompute();
    } PY_CATCH;

    Py_Return;
}


#ifdef FCUseFreeType

static PyObject * makeWireString(PyObject *self, PyObject *args)
{
    PyObject *intext;
    const char* dir;                      
    const char* fontfile;
    float height;
    int track = 0;

    Py_UNICODE *unichars;
    Py_ssize_t pysize;
   
    PyObject *CharList;
   
    if (!PyArg_ParseTuple(args, "Ossf|i", &intext, 
                                          &dir,
                                          &fontfile,
                                          &height,
                                          &track))  {
        Base::Console().Message("** makeWireString bad args.\n");                                           
        return NULL;
    }

    if (PyString_Check(intext)) {
        PyObject *p = Base::PyAsUnicodeObject(PyString_AsString(intext));    
        if (!p) {
            Base::Console().Message("** makeWireString can't convert PyString.\n");
            return NULL;
        }
        pysize = PyUnicode_GetSize(p);    
        unichars = PyUnicode_AS_UNICODE(p);
    }
    else if (PyUnicode_Check(intext)) {        
        pysize = PyUnicode_GetSize(intext);   
        unichars = PyUnicode_AS_UNICODE(intext);
    }
    else {
        Base::Console().Message("** makeWireString bad text parameter.\n");                                           
        return NULL;
    }

    try {        
        CharList = FT2FC(unichars,pysize,dir,fontfile,height,track);         // get list of wire chars
    }
    catch (Standard_DomainError) {                                      // Standard_DomainError is OCC error.
        PyErr_SetString(PyExc_Exception, "makeWireString failed - Standard_DomainError");
        return NULL;
    }
    catch (std::runtime_error& e) {                                     // FT2 or FT2FC errors
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }

    return (CharList);
}
#else

static PyObject * makeWireString(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_Exception, "FreeCAD compiled without FreeType support! This method is disabled...");
    return NULL;
}

#endif //#ifdef FCUseFreeType
static PyObject * 
makeCompound(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        BRep_Builder builder;
        TopoDS_Compound Comp;
        builder.MakeCompound(Comp);
        
        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->_Shape;
                    if (!sh.IsNull())
                        builder.Add(Comp, sh);
                }
            }
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }

        return new TopoShapeCompoundPy(new TopoShape(Comp));
    } PY_CATCH;
}

static PyObject * makeFilledFace(PyObject *self, PyObject *args)
{
    // http://opencascade.blogspot.com/2010/03/surface-modeling-part6.html
    // TODO: GeomPlate_BuildPlateSurface
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    PY_TRY {
        BRepFill_Filling builder;
        
        try {
            Py::Sequence list(obj);
            int countEdges = 0;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeEdgePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapeEdgePy*>((*it).ptr())->
                        getTopoShapePtr()->_Shape;
                    if (!sh.IsNull()) {
                        builder.Add(TopoDS::Edge(sh), GeomAbs_C0);
                        countEdges++;
                    }
                }
            }

            if (countEdges == 0) {
                PyErr_SetString(PyExc_Exception, "Failed to created face with no edges");
                return 0;
            }

            builder.Build();
            if (builder.IsDone()) {
                return new TopoShapeFacePy(new TopoShape(builder.Face()));
            }
            else {
                PyErr_SetString(PyExc_Exception, "Failed to created face by filling edges");
                return 0;
            }
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }
    } PY_CATCH;
}

static PyObject * makeShell(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    PY_TRY {
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
                        getTopoShapePtr()->_Shape;
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
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }

        return new TopoShapeShellPy(new TopoShape(shape));
    } PY_CATCH;
}

static PyObject * makeSolid(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj))
        return NULL;

    try {
        BRepBuilderAPI_MakeSolid mkSolid;
        const TopoDS_Shape& shape = static_cast<TopoShapePy*>(obj)
            ->getTopoShapePtr()->_Shape;
        TopExp_Explorer anExp (shape, TopAbs_SHELL);
        int count=0;
        for (; anExp.More(); anExp.Next()) {
            ++count;
            mkSolid.Add(TopoDS::Shell(anExp.Current()));
        }

        if (count == 0)
            Standard_Failure::Raise("No shells found in shape");

        const TopoDS_Solid& solid = mkSolid.Solid();
        return new TopoShapeSolidPy(new TopoShape(solid));
    }
    catch (Standard_Failure) {
        PyErr_SetString(PyExc_Exception, "creation of solid failed");
        return NULL;
    }
}

static PyObject * makePlane(PyObject *self, PyObject *args)
{
    double length, width;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "dd|O!O!", &length, &width,
                                           &(Base::VectorPy::Type), &pPnt,
                                           &(Base::VectorPy::Type), &pDir))
        return NULL;

    if (length < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "length of plane too small");
        return NULL;
    }
    if (width < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "width of plane too small");
        return NULL;
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
        Handle_Geom_Plane aPlane = new Geom_Plane(p, d);
        BRepBuilderAPI_MakeFace Face(aPlane, 0.0, length, 0.0, width
#if OCC_VERSION_HEX >= 0x060502
          , Precision::Confusion()
#endif
        );
        return new TopoShapeFacePy(new TopoShape((Face.Face()))); 
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of plane failed");
        return NULL;
    }
}

static PyObject * makeBox(PyObject *self, PyObject *args)
{
    double length, width, height;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "ddd|O!O!", &length, &width, &height,
                                            &(Base::VectorPy::Type), &pPnt,
                                            &(Base::VectorPy::Type), &pDir))
        return NULL;

    if (length < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "length of box too small");
        return NULL;
    }
    if (width < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "width of box too small");
        return NULL;
    }
    if (height < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "height of box too small");
        return NULL;
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
        return new TopoShapeSolidPy(new TopoShape(ResultShape)); 
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of box failed");
        return NULL;
    }
}

static PyObject * makeWedge(PyObject *self, PyObject *args)
{
    double xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "dddddddddd|O!O!",
        &xmin, &ymin, &zmin, &z2min, &x2min, &xmax, &ymax, &zmax, &z2max, &x2max,
        &(Base::VectorPy::Type), &pPnt, &(Base::VectorPy::Type), &pDir))
        return NULL;

    double dx = xmax-xmin;
    double dy = ymax-ymin;
    double dz = zmax-zmin;
    double dz2 = z2max-z2min;
    double dx2 = x2max-x2min;
    if (dx < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "delta x of wedge too small");
        return NULL;
    }
    if (dy < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "delta y of wedge too small");
        return NULL;
    }
    if (dz < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "delta z of wedge too small");
        return NULL;
    }
    if (dz2 < 0) {
        PyErr_SetString(PyExc_Exception, "delta z2 of wedge is negative");
        return NULL;
    }
    if (dx2 < 0) {
        PyErr_SetString(PyExc_Exception, "delta x2 of wedge is negative");
        return NULL;
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
        return new TopoShapeSolidPy(new TopoShape(mkSolid.Solid())); 
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of wedge failed");
        return NULL;
    }
}

static PyObject * makeCircle(PyObject *self, PyObject *args)
{
    double radius, angle1=0.0, angle2=360;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "d|O!O!dd", &radius,
                                            &(Base::VectorPy::Type), &pPnt,
                                            &(Base::VectorPy::Type), &pDir,
                                            &angle1, &angle2))
        return NULL;

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

        Handle_Geom_Circle hCircle = new Geom_Circle (circle);
        BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, angle1*(M_PI/180), angle2*(M_PI/180));
        TopoDS_Edge edge = aMakeEdge.Edge();
        return new TopoShapeEdgePy(new TopoShape(edge)); 
    }
    catch (Standard_Failure) {
        PyErr_SetString(PyExc_Exception, "creation of circle failed");
        return NULL;
    }
}

static PyObject * makeSphere(PyObject *self, PyObject *args)
{
    double radius, angle1=-90, angle2=90, angle3=360;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "d|O!O!ddd", &radius,
                                             &(Base::VectorPy::Type), &pPnt,
                                             &(Base::VectorPy::Type), &pDir,
                                             &angle1, &angle2, &angle3))
        return NULL;

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
        return new TopoShapeSolidPy(new TopoShape(shape));
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of sphere failed");
        return NULL;
    }
}

static PyObject * makeCylinder(PyObject *self, PyObject *args)
{
    double radius, height, angle=360;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "dd|O!O!d", &radius, &height,
                                            &(Base::VectorPy::Type), &pPnt,
                                            &(Base::VectorPy::Type), &pDir,
                                            &angle))
        return NULL;

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
        return new TopoShapeSolidPy(new TopoShape(shape));
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of cylinder failed");
        return NULL;
    }
}

static PyObject * makeCone(PyObject *self, PyObject *args)
{
    double radius1, radius2,  height, angle=360;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "ddd|O!O!d", &radius1, &radius2, &height,
                                             &(Base::VectorPy::Type), &pPnt,
                                             &(Base::VectorPy::Type), &pDir,
                                             &angle))
        return NULL;

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
        return new TopoShapeSolidPy(new TopoShape(shape));
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of cone failed");
        return NULL;
    }
}

static PyObject * makeTorus(PyObject *self, PyObject *args)
{
    double radius1, radius2, angle1=0.0, angle2=360, angle=360;
    PyObject *pPnt=0, *pDir=0;
    if (!PyArg_ParseTuple(args, "dd|O!O!ddd", &radius1, &radius2,
                                              &(Base::VectorPy::Type), &pPnt,
                                              &(Base::VectorPy::Type), &pDir,
                                              &angle1, &angle2, &angle))
        return NULL;

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
        return new TopoShapeSolidPy(new TopoShape(shape));
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of torus failed");
        return NULL;
    }
}

static PyObject * makeHelix(PyObject *self, PyObject *args)
{
    double pitch, height, radius, angle=-1.0;
    if (!PyArg_ParseTuple(args, "ddd|d", &pitch, &height, &radius, &angle))
        return 0;

    try {
        TopoShape helix;
        TopoDS_Shape wire = helix.makeHelix(pitch, height, radius, angle);
        return new TopoShapeWirePy(new TopoShape(wire));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

static PyObject * makeThread(PyObject *self, PyObject *args)
{
    double pitch, depth, height, radius;
    if (!PyArg_ParseTuple(args, "dddd", &pitch, &depth, &height, &radius))
        return 0;

    try {
        TopoShape helix;
        TopoDS_Shape wire = helix.makeThread(pitch, depth, height, radius);
        return new TopoShapeWirePy(new TopoShape(wire));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

static PyObject * makeLine(PyObject *self, PyObject *args)
{
    PyObject *obj1, *obj2;
    if (!PyArg_ParseTuple(args, "OO", &obj1, &obj2))
        return NULL;

    Base::Vector3d pnt1, pnt2;
    if (PyObject_TypeCheck(obj1, &(Base::VectorPy::Type))) {
        pnt1 = static_cast<Base::VectorPy*>(obj1)->value();
    }
    else if (PyObject_TypeCheck(obj1, &PyTuple_Type)) {
        try {
            pnt1 = Base::getVectorFromTuple<double>(obj1);
        }
        catch (const Py::Exception&) {
            return NULL;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "first argument must either be vector or tuple");
        return 0;
    }
    if (PyObject_TypeCheck(obj2, &(Base::VectorPy::Type))) {
        pnt2 = static_cast<Base::VectorPy*>(obj2)->value();
    }
    else if (PyObject_TypeCheck(obj2, &PyTuple_Type)) {
        try {
            pnt2 = Base::getVectorFromTuple<double>(obj2);
        }
        catch (const Py::Exception&) {
            return NULL;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "second argument must either be vector or tuple");
        return 0;
    }

    // Create directly the underlying line geometry
    BRepBuilderAPI_MakeEdge makeEdge(gp_Pnt(pnt1.x, pnt1.y, pnt1.z),
                                     gp_Pnt(pnt2.x, pnt2.y, pnt2.z));

    const char *error=0;
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
        error = "Line through identic points";
        break;
    }
    // Error 
    if (error) {
        PyErr_SetString(PyExc_RuntimeError, error);
        return NULL;
    }

    TopoDS_Edge edge = makeEdge.Edge();
    return new TopoShapeEdgePy(new TopoShape(edge)); 
}

static PyObject * makePolygon(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        BRepBuilderAPI_MakePolygon mkPoly;
        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Base::VectorPy::Type))) {
                    Base::Vector3d v = static_cast<Base::VectorPy*>((*it).ptr())->value();
                    mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
                }
                else if (PyObject_TypeCheck((*it).ptr(), &PyTuple_Type)) {
                    try {
                        Base::Vector3d v = Base::getVectorFromTuple<double>((*it).ptr());
                        mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
                    }
                    catch (const Py::Exception&) {
                        return 0;
                    }
                }
            }

            if (!mkPoly.IsDone())
                Standard_Failure::Raise("Cannot create polygon because less than two vertices are given");

            return new TopoShapeWirePy(new TopoShape(mkPoly.Wire()));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }
    } PY_CATCH;
}

static PyObject * makeRevolution(PyObject *self, PyObject *args)
{
    double vmin = DBL_MAX, vmax=-DBL_MAX;
    double angle=360;
    PyObject *pPnt=0, *pDir=0, *pCrv;
    Handle_Geom_Curve curve;
    union PyType_Object defaultType = {&Part::TopoShapeSolidPy::Type};
    PyObject* type = defaultType.o;
    if (PyArg_ParseTuple(args, "O!|dddO!O!O!", &(GeometryPy::Type), &pCrv,
                                               &vmin, &vmax, &angle,
                                               &(Base::VectorPy::Type), &pPnt,
                                               &(Base::VectorPy::Type), &pDir,
                                               &(PyType_Type), &type)) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(pCrv);
        curve = Handle_Geom_Curve::DownCast
            (pcGeo->getGeometryPtr()->handle());
        if (curve.IsNull()) {
            PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
            return 0;
        }
        if (vmin == DBL_MAX)
            vmin = curve->FirstParameter();

        if (vmax == -DBL_MAX)
            vmax = curve->LastParameter();
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "O!|dddO!O!", &(TopoShapePy::Type), &pCrv,
            &vmin, &vmax, &angle, &(Base::VectorPy::Type), &pPnt,
            &(Base::VectorPy::Type), &pDir)) {
            return 0;
        }
        const TopoDS_Shape& shape = static_cast<TopoShapePy*>(pCrv)->getTopoShapePtr()->_Shape;
        if (shape.IsNull()) {
            PyErr_SetString(PyExc_Exception, "shape is empty");
            return 0;
        }

        if (shape.ShapeType() != TopAbs_EDGE) {
            PyErr_SetString(PyExc_Exception, "shape is not an edge");
            return 0;
        }

        const TopoDS_Edge& edge = TopoDS::Edge(shape);
        BRepAdaptor_Curve adapt(edge);

        const Handle_Geom_Curve& hCurve = adapt.Curve().Curve();
        // Apply placement of the shape to the curve
        TopLoc_Location loc = edge.Location();
        curve = Handle_Geom_Curve::DownCast(hCurve->Transformed(loc.Transformation()));
        if (curve.IsNull()) {
            PyErr_SetString(PyExc_Exception, "invalid curve in edge");
            return 0;
        }

        if (vmin == DBL_MAX)
            vmin = adapt.FirstParameter();
        if (vmax == -DBL_MAX)
            vmax = adapt.LastParameter();
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

        union PyType_Object shellType = {&Part::TopoShapeShellPy::Type};
        union PyType_Object faceType = {&Part::TopoShapeFacePy::Type};

        BRepPrimAPI_MakeRevolution mkRev(gp_Ax2(p,d),curve, vmin, vmax, angle*(M_PI/180));
        if (type == defaultType.o) {
            TopoDS_Shape shape = mkRev.Solid();
            return new TopoShapeSolidPy(new TopoShape(shape));
        }
        else if (type == shellType.o) {
            TopoDS_Shape shape = mkRev.Shell();
            return new TopoShapeShellPy(new TopoShape(shape));
        }
        else if (type == faceType.o) {
            TopoDS_Shape shape = mkRev.Face();
            return new TopoShapeFacePy(new TopoShape(shape));
        }
        else {
            TopoDS_Shape shape = mkRev.Shape();
            return new TopoShapePy(new TopoShape(shape));
        }
    }
    catch (Standard_DomainError) {
        PyErr_SetString(PyExc_Exception, "creation of revolved shape failed");
        return NULL;
    }
}

static PyObject * makeRuledSurface(PyObject *self, PyObject *args)
{
    // http://opencascade.blogspot.com/2009/10/surface-modeling-part1.html
    PyObject *sh1, *sh2;
    if (!PyArg_ParseTuple(args, "O!O!", &(TopoShapePy::Type), &sh1,
                                        &(TopoShapePy::Type), &sh2))
        return 0;

    const TopoDS_Shape& shape1 = static_cast<TopoShapePy*>(sh1)->getTopoShapePtr()->_Shape;
    const TopoDS_Shape& shape2 = static_cast<TopoShapePy*>(sh2)->getTopoShapePtr()->_Shape;

    try {
        if (shape1.ShapeType() == TopAbs_EDGE && shape2.ShapeType() == TopAbs_EDGE) {
            TopoDS_Face face = BRepFill::Face(TopoDS::Edge(shape1), TopoDS::Edge(shape2));
            return new TopoShapeFacePy(new TopoShape(face));
        }
        else if (shape1.ShapeType() == TopAbs_WIRE && shape2.ShapeType() == TopAbs_WIRE) {
            TopoDS_Shell shell = BRepFill::Shell(TopoDS::Wire(shape1), TopoDS::Wire(shape2));
            return new TopoShapeShellPy(new TopoShape(shell));
        }
        else {
            PyErr_SetString(PyExc_Exception, "curves must either be edges or wires");
            return 0;
        }
    }
    catch (Standard_Failure) {
        PyErr_SetString(PyExc_Exception, "creation of ruled surface failed");
        return 0;
    }
}

static PyObject * makeSweepSurface(PyObject *self, PyObject *args)
{
    PyObject *path, *profile;
    double tolerance=0.001;
    int fillMode = 0;

    // Path + profile
    if (!PyArg_ParseTuple(args, "O!O!|di", &(TopoShapePy::Type), &path,
                                           &(TopoShapePy::Type), &profile,
                                           &tolerance, &fillMode))
        return 0;

    try {
        const TopoDS_Shape& path_shape = static_cast<TopoShapePy*>(path)->getTopoShapePtr()->_Shape;
        const TopoDS_Shape& prof_shape = static_cast<TopoShapePy*>(profile)->getTopoShapePtr()->_Shape;

        TopoShape myShape(path_shape);
        TopoDS_Shape face = myShape.makeSweep(prof_shape, tolerance, fillMode);
        return new TopoShapeFacePy(new TopoShape(face));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

static PyObject * makeTube(PyObject *self, PyObject *args)
{
    PyObject *pshape;
    double radius;
    double tolerance=0.001;
    char* scont = "C0";
    int maxdegree = 3;
    int maxsegment = 30;

    // Path + radius
    if (!PyArg_ParseTuple(args, "O!d|sii", &(TopoShapePy::Type), &pshape, &radius, &scont, &maxdegree, &maxsegment))
        return 0;
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
        const TopoDS_Shape& path_shape = static_cast<TopoShapePy*>(pshape)->getTopoShapePtr()->_Shape;
        TopoShape myShape(path_shape);
        TopoDS_Shape face = myShape.makeTube(radius, tolerance, cont, maxdegree, maxsegment);
        return new TopoShapeFacePy(new TopoShape(face));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

static PyObject * makeLoft(PyObject *self, PyObject *args)
{
#if 0
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    NCollection_List<Handle_Geom_Curve> theSections;
    Py::Sequence list(pcObj);
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryCurvePy::Type))) {
            Handle_Geom_Curve hCurve = Handle_Geom_Curve::DownCast(
                static_cast<GeometryCurvePy*>((*it).ptr())->getGeomCurvePtr()->handle());
            theSections.Append(hCurve);
        }
    }

    //populate section generator
    GeomFill_SectionGenerator aSecGenerator;
    for (NCollection_List<Handle_Geom_Curve>::Iterator anIt(theSections); anIt.More(); anIt.Next()) {
        const Handle_Geom_Curve& aCurve = anIt.Value();
        aSecGenerator.AddCurve (aCurve);
    }
    aSecGenerator.Perform (Precision::PConfusion());

    Handle_GeomFill_Line aLine = new GeomFill_Line (theSections.Size());

    //parameters
    const Standard_Integer aMinDeg = 1, aMaxDeg = BSplCLib::MaxDegree(), aNbIt = 0;
    Standard_Real aTol3d = 1e-4, aTol2d = Precision::Parametric (aTol3d);

    //algorithm
    GeomFill_AppSurf anAlgo (aMinDeg, aMaxDeg, aTol3d, aTol2d, aNbIt);
    anAlgo.Perform (aLine, aSecGenerator);

    if (!anAlgo.IsDone()) {
        PyErr_SetString(PyExc_Exception, "Failed to create loft surface");
        return 0;
    }

    Handle_Geom_BSplineSurface aRes;
    aRes = new Geom_BSplineSurface(anAlgo.SurfPoles(), anAlgo.SurfWeights(),
        anAlgo.SurfUKnots(), anAlgo.SurfVKnots(), anAlgo.SurfUMults(), anAlgo.SurfVMults(),
        anAlgo.UDegree(), anAlgo.VDegree());
    return new BSplineSurfacePy(new GeomBSplineSurface(aRes));
#else
    PyObject *pcObj;
    PyObject *psolid=Py_False;
    PyObject *pruled=Py_False;
    if (!PyArg_ParseTuple(args, "O|O!O!", &pcObj,
                                          &(PyBool_Type), &psolid,
                                          &(PyBool_Type), &pruled))
        return NULL;

    try {
        TopTools_ListOfShape profiles;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                    getTopoShapePtr()->_Shape;
                profiles.Append(sh);
            }
        }

        TopoShape myShape;
        Standard_Boolean anIsSolid = PyObject_IsTrue(psolid) ? Standard_True : Standard_False;
        Standard_Boolean anIsRuled = PyObject_IsTrue(pruled) ? Standard_True : Standard_False;
        TopoDS_Shape aResult = myShape.makeLoft(profiles, anIsSolid, anIsRuled);
        return new TopoShapePy(new TopoShape(aResult));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
#endif
}

static PyObject* setStaticValue(PyObject *self, PyObject *args)
{
    char *name, *cval;
    if (PyArg_ParseTuple(args, "ss", &name, &cval)) {
        if (!Interface_Static::SetCVal(name, cval)) {
            PyErr_Format(PyExc_RuntimeError, "Failed to set '%s'", name);
            return 0;
        }
        Py_Return;
    }

    PyErr_Clear();
    PyObject* index_or_value;
    if (PyArg_ParseTuple(args, "sO", &name, &index_or_value)) {
        if (PyInt_Check(index_or_value)) {
            int ival = (int)PyInt_AsLong(index_or_value);
            if (!Interface_Static::SetIVal(name, ival)) {
                PyErr_Format(PyExc_RuntimeError, "Failed to set '%s'", name);
                return 0;
            }
            Py_Return;
        }
        else if (PyFloat_Check(index_or_value)) {
            double rval = PyFloat_AsDouble(index_or_value);
            if (!Interface_Static::SetRVal(name, rval)) {
                PyErr_Format(PyExc_RuntimeError, "Failed to set '%s'", name);
                return 0;
            }
            Py_Return;
        }
    }

    PyErr_SetString(PyExc_TypeError, "First argument must be string and must be either string, int or float");
    return 0;
}

static PyObject * exportUnits(PyObject *self, PyObject *args)
{
    char* unit=0;
    if (!PyArg_ParseTuple(args, "|s", &unit))
        return NULL;
    if (unit) {
        if (strcmp(unit,"M") == 0 || strcmp(unit,"MM") == 0 || strcmp(unit,"IN") == 0) {
            if (!Interface_Static::SetCVal("write.iges.unit",unit)) {
                PyErr_SetString(PyExc_RuntimeError, "Failed to set 'write.iges.unit'");
                return 0;
            }
            if (!Interface_Static::SetCVal("write.step.unit",unit)) {
                PyErr_SetString(PyExc_RuntimeError, "Failed to set 'write.step.unit'");
                return 0;
            }
        }
        else {
            PyErr_SetString(PyExc_ValueError, "Wrong unit");
            return 0;
        }
    }

    Py::Dict dict;
    dict.setItem("write.iges.unit", Py::String(Interface_Static::CVal("write.iges.unit")));
    dict.setItem("write.step.unit", Py::String(Interface_Static::CVal("write.step.unit")));
    return Py::new_reference_to(dict);
}

static PyObject * toPythonOCC(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    try {
        TopoDS_Shape* shape = new TopoDS_Shape();
        (*shape) = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("OCC.TopoDS", "TopoDS_Shape *", (void*)shape, 1);
        return proxy;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

static PyObject * fromPythonOCC(PyObject *self, PyObject *args)
{
    PyObject *proxy;
    if (!PyArg_ParseTuple(args, "O", &proxy))
        return NULL;

    void* ptr;
    try {
        TopoShape* shape = new TopoShape();
        Base::Interpreter().convertSWIGPointerObj("OCC.TopoDS","TopoDS_Shape *", proxy, &ptr, 0);
        TopoDS_Shape* s = reinterpret_cast<TopoDS_Shape*>(ptr);
        shape->_Shape = (*s);
        return new TopoShapePy(shape);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

namespace Part {
struct EdgePoints {
    gp_Pnt v1, v2;
    TopoDS_Edge edge;
};

static std::list<TopoDS_Edge> sort_Edges(double tol3d, const std::vector<TopoDS_Edge>& edges)
{
    tol3d = tol3d * tol3d;
    std::list<EdgePoints>  edge_points;
    TopExp_Explorer xp;
    for (std::vector<TopoDS_Edge>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        EdgePoints ep;
        xp.Init(*it,TopAbs_VERTEX);
        ep.v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        ep.v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        ep.edge = *it;
        edge_points.push_back(ep);
    }

    if (edge_points.empty())
        return std::list<TopoDS_Edge>();

    std::list<TopoDS_Edge> sorted;
    gp_Pnt first, last;
    first = edge_points.front().v1;
    last  = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edge_points.erase(edge_points.begin());

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edge_points.begin(); pEI != edge_points.end(); ++pEI) {
            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v2.SquareDistance(first) <= tol3d) {
                first = pEI->v1;
                sorted.push_front(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                sorted.push_back(pEI->edge);
                edge_points.erase(pEI);
                break;
            }
            else if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                sorted.push_front(pEI->edge);
                edge_points.erase(pEI);
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

static PyObject * getSortedClusters(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        PyErr_SetString(PyExc_Exception, "list of edges expected");
        return 0;
    }

    Py::Sequence list(obj);
    std::vector<TopoDS_Edge> edges;
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->_Shape;
            if (sh.ShapeType() == TopAbs_EDGE)
                edges.push_back(TopoDS::Edge(sh));
            else {
                PyErr_SetString(PyExc_TypeError, "shape is not an edge");
                return 0;
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError, "item is not a shape");
            return 0;
        }
    }

    Edgecluster acluster(edges);
    tEdgeClusterVector aclusteroutput = acluster.GetClusters();

    Py::List root_list;
    for (tEdgeClusterVector::iterator it=aclusteroutput.begin(); it != aclusteroutput.end();++it) {
        Py::List add_list;
        for (tEdgeVector::iterator it1=(*it).begin();it1 != (*it).end();++it1) {
            add_list.append(Py::Object(new TopoShapeEdgePy(new TopoShape(*it1)),true));
        }
        root_list.append(add_list);
    }

    return Py::new_reference_to(root_list);
}


static PyObject * sortEdges(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        PyErr_SetString(PyExc_Exception, "list of edges expected");
        return 0;
    }


    Py::Sequence list(obj);
    std::vector<TopoDS_Edge> edges;
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        PyObject* item = (*it).ptr();
        if (PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            const TopoDS_Shape& sh = static_cast<Part::TopoShapePy*>(item)->getTopoShapePtr()->_Shape;
            if (sh.ShapeType() == TopAbs_EDGE)
                edges.push_back(TopoDS::Edge(sh));
            else {
                PyErr_SetString(PyExc_TypeError, "shape is not an edge");
                return 0;
            }
        }
        else {
            PyErr_SetString(PyExc_TypeError, "item is not a shape");
            return 0;
        }
    }

    try {
        std::list<TopoDS_Edge> sorted = sort_Edges(Precision::Confusion(), edges);

        Py::List sorted_list;
        for (std::list<TopoDS_Edge>::iterator it = sorted.begin(); it != sorted.end(); ++it) {
            sorted_list.append(Py::Object(new TopoShapeEdgePy(new TopoShape(*it)),true));
        }

        return Py::new_reference_to(sorted_list);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

static PyObject * cast_to_shape(PyObject *self, PyObject *args)
{
    PyObject *object;
    if (PyArg_ParseTuple(args,"O!",&(Part::TopoShapePy::Type), &object)) {
        TopoShape* ptr = static_cast<TopoShapePy*>(object)->getTopoShapePtr();
        TopoDS_Shape shape = ptr->_Shape;
        if (!shape.IsNull()) {
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
                return new TopoShapeWirePy(new TopoShape(shape));
            case TopAbs_EDGE:
                return new TopoShapeEdgePy(new TopoShape(shape));
            case TopAbs_VERTEX:
                return new TopoShapeVertexPy(new TopoShape(shape));
            case TopAbs_SHAPE:
                return new TopoShapePy(new TopoShape(shape));
            default:
                break;
            }
        }
        else {
            PyErr_SetString(PyExc_Exception, "empty shape");
        }
    }

    return 0;
}

/* registration table  */
struct PyMethodDef Part_methods[] = {
    {"open"       ,open      ,METH_VARARGS,
     "open(string) -- Create a new document and load the file into the document."},

    {"insert"     ,insert    ,METH_VARARGS,
     "insert(string,string) -- Insert the file into the given document."},

    {"export"     ,exporter  ,METH_VARARGS,
     "export(list,string) -- Export a list of objects into a single file."},

    {"read"       ,read      ,METH_VARARGS,
     "read(string) -- Load the file and return the shape."},

    {"show"       ,show      ,METH_VARARGS,
     "show(shape) -- Add the shape to the active document or create one if no document exists."},

    {"makeCompound"  ,makeCompound ,METH_VARARGS,
     "makeCompound(list) -- Create a compound out of a list of shapes."},

    {"makeShell"  ,makeShell ,METH_VARARGS,
     "makeShell(list) -- Create a shell out of a list of faces."},

    {"makeFilledFace"  ,makeFilledFace ,METH_VARARGS,
     "makeFilledFace(list) -- Create a face out of a list of edges."},

    {"makeSolid"  ,makeSolid ,METH_VARARGS,
     "makeSolid(shape) -- Create a solid out of the shells inside a shape."},

    {"makePlane"  ,makePlane ,METH_VARARGS,
     "makePlane(length,width,[pnt,dir]) -- Make a plane\n"
     "By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)"},

    {"makeBox"    ,makeBox ,METH_VARARGS,
     "makeBox(length,width,height,[pnt,dir]) -- Make a box located\n"
     "in pnt with the dimensions (length,width,height)\n"
     "By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)"},

    {"makeWedge"    ,makeWedge ,METH_VARARGS,
     "makeWedge(xmin, ymin, zmin, z2min, x2min,\n"
     "xmax, ymax, zmax, z2max, x2max,[pnt,dir])\n"
     " -- Make a wedge located in pnt\n"
     "By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)"},

    {"makeLine"   ,makeLine  ,METH_VARARGS,
     "makeLine((x1,y1,z1),(x2,y2,z2)) -- Make a line of two points"},

    {"makePolygon"   ,makePolygon  ,METH_VARARGS,
     "makePolygon(list) -- Make a polygon of a list of points"},

    {"makeCircle" ,makeCircle,METH_VARARGS,
     "makeCircle(radius,[pnt,dir,angle1,angle2]) -- Make a circle with a given radius\n"
     "By default pnt=Vector(0,0,0), dir=Vector(0,0,1), angle1=0 and angle2=360"},

    {"makeSphere" ,makeSphere,METH_VARARGS,
     "makeSphere(radius,[pnt, dir, angle1,angle2,angle3]) -- Make a sphere with a given radius\n"
     "By default pnt=Vector(0,0,0), dir=Vector(0,0,1), angle1=0, angle2=90 and angle3=360"},

    {"makeCylinder" ,makeCylinder,METH_VARARGS,
     "makeCylinder(radius,height,[pnt,dir,angle]) -- Make a cylinder with a given radius and height\n"
     "By default pnt=Vector(0,0,0),dir=Vector(0,0,1) and angle=360"},

    {"makeCone" ,makeCone,METH_VARARGS,
     "makeCone(radius1,radius2,height,[pnt,dir,angle]) -- Make a cone with given radii and height\n"
     "By default pnt=Vector(0,0,0), dir=Vector(0,0,1) and angle=360"},

    {"makeTorus" ,makeTorus,METH_VARARGS,
     "makeTorus(radius1,radius2,[pnt,dir,angle1,angle2,angle]) -- Make a torus with a given radii and angles\n"
     "By default pnt=Vector(0,0,0),dir=Vector(0,0,1),angle1=0,angle1=360 and angle=360"},

    {"makeHelix" ,makeHelix,METH_VARARGS,
     "makeHelix(pitch,height,radius,[angle]) -- Make a helix with a given pitch, height and radius\n"
     "By default a cylindrical surface is used to create the helix. If the fourth parameter is set\n"
     "(the apex given in degree) a conical surface is used instead"},

    {"makeThread" ,makeThread,METH_VARARGS,
     "makeThread(pitch,depth,height,radius) -- Make a thread with a given pitch, depth, height and radius"},

    {"makeRevolution" ,makeRevolution,METH_VARARGS,
     "makeRevolution(Curve,[vmin,vmax,angle,pnt,dir,shapetype]) -- Make a revolved shape\n"
     "by rotating the curve or a portion of it around an axis given by (pnt,dir).\n"
     "By default vmin/vmax=bounds of the curve,angle=360,pnt=Vector(0,0,0) and\n"
     "dir=Vector(0,0,1) and shapetype=Part.Solid"},

    {"makeRuledSurface" ,makeRuledSurface,METH_VARARGS,
     "makeRuledSurface(Edge|Wire,Edge|Wire) -- Make a ruled surface\n"
     "Create a ruled surface out of two edges or wires. If wires are used then"
     "these must have the same number of edges."},

    {"makeTube" ,makeTube,METH_VARARGS,
     "makeTube(edge,radius,[continuity,max degree,max segments]) -- Create a tube.\n"
     "continuity is a string which must be 'C0','C1','C2','C3','CN','G1' or 'G1',"},

    {"makeSweepSurface" ,makeSweepSurface,METH_VARARGS,
     "makeSweepSurface(edge(path),edge(profile),[float]) -- Create a profile along a path."},

    {"makeLoft" ,makeLoft,METH_VARARGS,
     "makeLoft(list of wires) -- Create a loft shape."},

    {"makeWireString" ,makeWireString ,METH_VARARGS,
     "makeWireString(string,fontdir,fontfile,height,[track]) -- Make list of wires in the form of a string's characters."},

    {"exportUnits" ,exportUnits ,METH_VARARGS,
     "exportUnits([string=MM|M|IN]) -- Set units for exporting STEP/IGES files and returns the units."},

    {"setStaticValue" ,setStaticValue ,METH_VARARGS,
     "setStaticValue(string,string|int|float) -- Set a name to a value The value can be a string, int or float."},

    {"cast_to_shape" ,cast_to_shape,METH_VARARGS,
     "cast_to_shape(shape) -- Cast to the actual shape type"},

    {"getSortedClusters" ,getSortedClusters,METH_VARARGS,
    "getSortedClusters(list of edges) -- Helper method to sort and cluster a variety of edges"},

    {"__sortEdges__" ,sortEdges,METH_VARARGS,
     "__sortEdges__(list of edges) -- Helper method to sort an unsorted list of edges so that afterwards\n"
     "two adjacent edges share a common vertex"},

    {"__toPythonOCC__" ,toPythonOCC,METH_VARARGS,
     "__toPythonOCC__(shape) -- Helper method to convert an internal shape to pythonocc shape"},

    {"__fromPythonOCC__" ,fromPythonOCC,METH_VARARGS,
     "__fromPythonOCC__(occ) -- Helper method to convert a pythonocc shape to an internal shape"},
    {NULL, NULL}        /* end of table marker */
};
