#include "PreCompiled.h"
#include "dxf/DxfExport.h"
#include "dxf/ImpExpDxf.h"
#include "dxf/DxfWriterProxy.h"
#include "SketchExportHelper.h"

#include <App/Annotation.h>
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

    Py::Sequence list(objectList);

    bool pageExported = false;
    if (list.size() == 1) {
        PyObject* item = list.getItem(0).ptr();
        App::DocumentObject* obj =
            static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
        if (strcmp(obj->getTypeId().getName(), "TechDraw::DrawPage") == 0) {
            PyObject* export_page_func =
                PyObject_GetAttrString(helperModule, "_export_techdraw_page");
            if (export_page_func && PyCallable_Check(export_page_func)) {
                PyObject* writerProxyObj =
                    DxfWriterProxy_Type.tp_new(&Import::DxfWriterProxy_Type, nullptr, nullptr);
                ((Import::DxfWriterProxy*)writerProxyObj)->writer_inst = &writer;
                PyObject* result =
                    PyObject_CallFunctionObjArgs(export_page_func, item, writerProxyObj, NULL);
                Py_XDECREF(result);
                Py_DECREF(writerProxyObj);
            }
            Py_XDECREF(export_page_func);
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
            pageExported = true;
        }
    }

    if (!pageExported) {
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            App::DocumentObject* obj =
                static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();

            std::string layerName = "0";
            int aciColor = 256;

            PyObject* get_layer_func = PyObject_GetAttrString(helperModule, "_get_layer_name");
            if (get_layer_func && PyCallable_Check(get_layer_func)) {
                PyObject* pyLayerName = PyObject_CallFunctionObjArgs(get_layer_func, item, NULL);
                if (pyLayerName && PyUnicode_Check(pyLayerName)) {
                    layerName = PyUnicode_AsUTF8(pyLayerName);
                }
                Py_XDECREF(pyLayerName);
            }
            Py_XDECREF(get_layer_func);

            PyObject* get_aci_func = PyObject_GetAttrString(helperModule, "_get_aci_color");
            if (get_aci_func && PyCallable_Check(get_aci_func)) {
                PyObject* pyAciColor = PyObject_CallFunctionObjArgs(get_aci_func, item, NULL);
                if (pyAciColor && PyLong_Check(pyAciColor)) {
                    aciColor = PyLong_AsLong(pyAciColor);
                }
                Py_XDECREF(pyAciColor);
            }
            Py_XDECREF(get_aci_func);
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }

            writer.setLayerName(layerName);
            writer.setColor(aciColor);

            if (obj->isDerivedFrom(App::Annotation::getClassTypeId())) {
                PyObject* get_text_data_func =
                    PyObject_GetAttrString(helperModule, "_get_text_data");
                if (get_text_data_func && PyCallable_Check(get_text_data_func)) {
                    PyObject* text_data_list =
                        PyObject_CallFunctionObjArgs(get_text_data_func, item, NULL);
                    if (text_data_list && PyList_Check(text_data_list)) {
                        Py_ssize_t size = PyList_Size(text_data_list);
                        for (Py_ssize_t i = 0; i < size; ++i) {
                            PyObject* text_tuple = PyList_GetItem(text_data_list, i);
                            char* text_str;
                            double p1[3], p2[3], height, rotation;
                            int justification;
                            if (PyArg_ParseTuple(text_tuple,
                                                 "s(ddd)(ddd)did",
                                                 &text_str,
                                                 &p1[0],
                                                 &p1[1],
                                                 &p1[2],
                                                 &p2[0],
                                                 &p2[1],
                                                 &p2[2],
                                                 &height,
                                                 &justification,
                                                 &rotation)) {
                                writer.writeText(text_str, p1, p2, height, justification);
                            }
                        }
                    }
                    Py_XDECREF(text_data_list);
                }
                Py_XDECREF(get_text_data_func);
            }
            else if (obj->getPropertyByName("Dimline") != nullptr) {
                PyObject* get_dim_data_func =
                    PyObject_GetAttrString(helperModule, "_get_dimension_data");
                if (get_dim_data_func && PyCallable_Check(get_dim_data_func)) {
                    PyObject* dim_tuple =
                        PyObject_CallFunctionObjArgs(get_dim_data_func, item, NULL);
                    if (dim_tuple && PyTuple_Check(dim_tuple)) {
                        const char* dim_text;
                        double text_mid[3], line_def[3], p1[3], p2[3];
                        int dim_type;
                        if (PyArg_ParseTuple(dim_tuple,
                                             "(ddd)(ddd)(ddd)(ddd)si",
                                             &text_mid[0],
                                             &text_mid[1],
                                             &text_mid[2],
                                             &line_def[0],
                                             &line_def[1],
                                             &line_def[2],
                                             &p1[0],
                                             &p1[1],
                                             &p1[2],
                                             &p2[0],
                                             &p2[1],
                                             &p2[2],
                                             &dim_text,
                                             &dim_type)) {
                            writer.writeLinearDim(text_mid, line_def, p1, p2, dim_text, dim_type);
                        }
                    }
                    Py_XDECREF(dim_tuple);
                }
                Py_XDECREF(get_dim_data_func);
            }
            else if (auto* part = dynamic_cast<Part::Feature*>(obj)) {
                TopoDS_Shape shapeToExport = part->Shape.getValue();
                if (shapeToExport.IsNull()) {
                    continue;
                }

                if (writer.optionMesh && shapeToExport.ShapeType() > TopAbs_FACE) {
                    writer.writePolyFaceMesh(shapeToExport);
                }
                else if (writer.optionProject && shapeToExport.ShapeType() > TopAbs_FACE) {
                    // Call the Python helper to perform the projection
                    PyObject* project_func = PyObject_GetAttrString(helperModule, "_project_shape");
                    if (project_func && PyCallable_Check(project_func)) {
                        Part::TopoShapePy* shapePy =
                            new Part::TopoShapePy(new Part::TopoShape(shapeToExport));
                        const Base::Vector3d& projectionDir = writer.getProjectionDir();
                        Base::VectorPy* dirPy = new Base::VectorPy(projectionDir);

                        PyObject* resultShapePy = PyObject_CallFunctionObjArgs(project_func,
                                                                               (PyObject*)shapePy,
                                                                               (PyObject*)dirPy,
                                                                               NULL);

                        if (resultShapePy
                            && PyObject_TypeCheck(resultShapePy, &(Part::TopoShapePy::Type))) {
                            Part::TopoShape* ts =
                                static_cast<Part::TopoShapePy*>(resultShapePy)->getTopoShapePtr();
                            writer.exportShape(ts->getShape());
                        }

                        Py_XDECREF(shapePy);
                        Py_XDECREF(dirPy);
                        Py_XDECREF(resultShapePy);
                    }
                    Py_XDECREF(project_func);
                }
                else if (SketchExportHelper::isSketch(obj)) {
                    writer.exportShape(SketchExportHelper::getFlatSketchXY(obj));
                }
                else {
                    writer.exportShape(shapeToExport);
                }
            }
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
        }
    }
    Py_DECREF(helperModule);
}

}  // namespace Import
