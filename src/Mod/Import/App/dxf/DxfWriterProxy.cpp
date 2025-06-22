#include "PreCompiled.h"
#include "dxf/DxfWriterProxy.h"
#include <Base/VectorPy.h>
#include <Mod/Part/App/TopoShapePy.h>

namespace Import
{

// --- Forward declarations of our static C functions ---
static void DxfWriterProxy_dealloc(DxfWriterProxy* self);
static PyObject* DxfWriterProxy_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static PyObject* DxfWriterProxy_writeBlock(DxfWriterProxy* self, PyObject* args);
static PyObject* DxfWriterProxy_writeEndBlock(DxfWriterProxy* self, PyObject* args);
static PyObject* DxfWriterProxy_writeInsert(DxfWriterProxy* self, PyObject* args);
static PyObject* DxfWriterProxy_exportShape(DxfWriterProxy* self, PyObject* args);
static PyObject* DxfWriterProxy_setLayerName(DxfWriterProxy* self, PyObject* args);
static PyObject* DxfWriterProxy_setColor(DxfWriterProxy* self, PyObject* args);

// --- Method Table ---
static PyMethodDef DxfWriterProxy_methods[] = {
    {"writeBlock",
     (PyCFunction)DxfWriterProxy_writeBlock,
     METH_VARARGS,
     "writeBlock(name, base_point_tuple)"},
    {"writeEndBlock",
     (PyCFunction)DxfWriterProxy_writeEndBlock,
     METH_VARARGS,
     "writeEndBlock(name)"},
    {"writeInsert",
     (PyCFunction)DxfWriterProxy_writeInsert,
     METH_VARARGS,
     "writeInsert(name, insertion_point_tuple, scale, rotation)"},
    {"exportShape",
     (PyCFunction)DxfWriterProxy_exportShape,
     METH_VARARGS,
     "exportShape(shape_object)"},
    {"setLayerName", (PyCFunction)DxfWriterProxy_setLayerName, METH_VARARGS, "setLayerName(name)"},
    {"setColor", (PyCFunction)DxfWriterProxy_setColor, METH_VARARGS, "setColor(aci_index)"},
    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

// --- Type Definition ---
PyTypeObject DxfWriterProxy_Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "Import.DxfWriterProxy", /* tp_name */
    sizeof(DxfWriterProxy),                                    /* tp_basicsize */
    0,                                                         /* tp_itemsize */
    (destructor)DxfWriterProxy_dealloc,                        /* tp_dealloc */
    0,                                                         /* tp_vectorcall_offset */
    nullptr,                                                   /* tp_getattr */
    nullptr,                                                   /* tp_setattr */
    nullptr,                                                   /* tp_as_async */
    nullptr,                                                   /* tp_repr */
    nullptr,                                                   /* tp_as_number */
    nullptr,                                                   /* tp_as_sequence */
    nullptr,                                                   /* tp_as_mapping */
    nullptr,                                                   /* tp_hash */
    nullptr,                                                   /* tp_call */
    nullptr,                                                   /* tp_str */
    PyObject_GenericGetAttr,                                   /* tp_getattro */
    nullptr,                                                   /* tp_setattro */
    nullptr,                                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                        /* tp_flags */
    "A proxy for the C++ DXF writer.",                         /* tp_doc */
    nullptr,                                                   /* tp_traverse */
    nullptr,                                                   /* tp_clear */
    nullptr,                                                   /* tp_richcompare */
    0,                                                         /* tp_weaklistoffset */
    nullptr,                                                   /* tp_iter */
    nullptr,                                                   /* tp_iternext */
    DxfWriterProxy_methods,                                    /* tp_methods */
    nullptr,                                                   /* tp_members */
    nullptr,                                                   /* tp_getset */
    nullptr,                                                   /* tp_base */
    nullptr,                                                   /* tp_dict */
    nullptr,                                                   /* tp_descr_get */
    nullptr,                                                   /* tp_descr_set */
    0,                                                         /* tp_dictoffset */
    (initproc) nullptr,                                        /* tp_init */
    nullptr,                                                   /* tp_alloc */
    DxfWriterProxy_new,                                        /* tp_new */
    nullptr,                                                   /* tp_free */
    nullptr,                                                   /* tp_is_gc */
    nullptr,                                                   /* tp_bases */
    nullptr,                                                   /* tp_mro */
    nullptr,                                                   /* tp_cache */
    nullptr,                                                   /* tp_subclasses */
    nullptr,                                                   /* tp_weaklist */
    nullptr,                                                   /* tp_del */
    0,                                                         /* tp_version_tag */
    nullptr,                                                   /* tp_finalize */
    nullptr,                                                   /* tp_vectorcall */
};

// --- C Function Implementations ---
void DxfWriterProxy_dealloc(DxfWriterProxy* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* DxfWriterProxy_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    DxfWriterProxy* self = (DxfWriterProxy*)type->tp_alloc(type, 0);
    if (self != nullptr) {
        self->writer_inst = nullptr;
    }
    return (PyObject*)self;
}

PyObject* DxfWriterProxy_writeBlock(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    double p[3];
    if (!PyArg_ParseTuple(args, "s(ddd)", &name, &p[0], &p[1], &p[2])) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeBlock(name, p);
    }
    Py_RETURN_NONE;
}

PyObject* DxfWriterProxy_writeEndBlock(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeEndBlock(name);
    }
    Py_RETURN_NONE;
}

PyObject* DxfWriterProxy_writeInsert(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    double p[3];
    double scale, rotation;
    if (!PyArg_ParseTuple(args, "s(ddd)dd", &name, &p[0], &p[1], &p[2], &scale, &rotation)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeInsert(name, p, scale, rotation);
    }
    Py_RETURN_NONE;
}

PyObject* DxfWriterProxy_exportShape(DxfWriterProxy* self, PyObject* args)
{
    PyObject* shapeObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &shapeObj)) {
        return nullptr;
    }
    if (self->writer_inst) {
        Part::TopoShape* ts = static_cast<Part::TopoShapePy*>(shapeObj)->getTopoShapePtr();
        self->writer_inst->exportShape(ts->getShape());
    }
    Py_RETURN_NONE;
}

PyObject* DxfWriterProxy_setLayerName(DxfWriterProxy* self, PyObject* args)
{
    char* layer_name;
    if (!PyArg_ParseTuple(args, "s", &layer_name)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->setLayerName(layer_name);
    }
    Py_RETURN_NONE;
}

PyObject* DxfWriterProxy_setColor(DxfWriterProxy* self, PyObject* args)
{
    int aci_color;
    if (!PyArg_ParseTuple(args, "i", &aci_color)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->setColor(aci_color);
    }
    Py_RETURN_NONE;
}

}  // namespace Import
