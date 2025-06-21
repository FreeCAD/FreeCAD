#include "PreCompiled.h"
#include "dxf/DxfExport.h"
#include "dxf/ImpExpDxf.h"
#include "dxf/DxfWriterProxy.h"
#include "SketchExportHelper.h"

#include <App/Annotation.h>
#include <App/FeaturePython.h>
#include <App/DocumentObjectPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PartFeaturePy.h>
#include <Mod/Part/App/TopoShapePy.h>  // For wrapping/unwrapping shapes
#include <Base/VectorPy.h>             // For wrapping/unwrapping vectors
#include <Mod/TechDraw/App/DrawPage.h>

// For mesh logic
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>


namespace Import
{

void executeDxfExport(PyObject* objectList, ImpExpDxfWrite& writer)
{
    PyObject* helperModule = PyImport_ImportModule("Draft.importDXF");
    if (!helperModule) {
        throw Py::ImportError("Could not import Draft.importDXF module.");
    }

    // Get the single master dispatcher function from Python
    PyObject* export_object_func = PyObject_GetAttrString(helperModule, "_export_object");
    if (!export_object_func || !PyCallable_Check(export_object_func)) {
        Py_XDECREF(helperModule);
        Py_XDECREF(export_object_func);
        throw Py::RuntimeError("Cannot find the _export_object helper function in importDXF.py");
    }

    Py::Sequence list(objectList);

    // Create the C++ writer proxy ONCE
    PyObject* writerProxyObj =
        DxfWriterProxy_Type.tp_new(&Import::DxfWriterProxy_Type, nullptr, nullptr);
    ((Import::DxfWriterProxy*)writerProxyObj)->writer_inst = &writer;

    // The entire loop is now just this:
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        PyObject* item = (*it).ptr();

        // Call the master dispatcher for every single object
        PyObject* result =
            PyObject_CallFunctionObjArgs(export_object_func, item, writerProxyObj, NULL);
        Py_XDECREF(result);  // We don't care about the return value

        if (PyErr_Occurred()) {
            // An error happened in Python, print it and clear it to continue
            PyErr_Print();
            PyErr_Clear();
        }
    }

    // Clean up
    Py_DECREF(writerProxyObj);
    Py_DECREF(export_object_func);
    Py_DECREF(helperModule);
}

}  // namespace Import
