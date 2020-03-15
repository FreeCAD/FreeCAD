#include "PreCompiled.h"

#include "G2D/VectorPy.h"
#include "G2D/VectorPy.cpp"

#include <Base/DualNumberPy.h>
#include <Base/VectorPy.h>

using namespace FCS;

PyObject *VectorPy::PyMake(struct _typeobject *, PyObject* args, PyObject *)  // Python wrapper
{
    // create a new instance of VectorPy
    UnsafePyHandle<Vector> self ( new VectorPy(nullptr), true);

    {//no args
        if (PyArg_ParseTuple(args, "")){
            return Py::new_reference_to(self.getHandledObject());
        }
        PyErr_Clear();
    }

    {//double,double
        double x = 0.0;
        double y = 0.0;
        if (PyArg_ParseTuple(args, "dd", &x, &y)){
            self->x = x;
            self->y = y;
            return Py::new_reference_to(self.getHandledObject());
        }
        PyErr_Clear();
    }

    {//dualnumber,dualnumber
        PyObject* pcx = nullptr;
        PyObject* pcy = nullptr;
        if (PyArg_ParseTuple(args, "O!O!", &Base::DualNumberPy::Type, &pcx, &Base::DualNumberPy::Type, &pcy)){
            self->x = static_cast<Base::DualNumberPy*>(pcx)->value;
            self->y = static_cast<Base::DualNumberPy*>(pcy)->value;
            return Py::new_reference_to(self.getHandledObject());
        }
        PyErr_Clear();
    }

    {//vector,vector
        PyObject* pcvec = nullptr;
        PyObject* pcvecdu = nullptr;
        if (PyArg_ParseTuple(args, "O!|O!", &Base::VectorPy::Type, &pcvec, &Base::VectorPy::Type, &pcvecdu)){
            {
                Base::Vector3d vec = * static_cast<Base::VectorPy*>(pcvec)->getVectorPtr();
                self->x = vec.x;
                self->y = vec.y;
            }
            if (pcvecdu){
                Base::Vector3d vec = * static_cast<Base::VectorPy*>(pcvecdu)->getVectorPtr();
                self->x.du = vec.x;
                self->y.du = vec.y;
            }
            return Py::new_reference_to(self.getHandledObject());
        }
        PyErr_Clear();
    }

    PyErr_SetString(PyExc_TypeError,
        "wrong constructor argumrnts. Expect:"
        "\n"
        "\n () -> zero vector"
        "\n (float,float) -> vector with zero dual part"
        "\n (dualnumber, dualnumber) -> vector with x and y filled with these dual numbers"
        "\n (vec, vec = 0) -> fill real and dual parts from two FreeCAD.Vector's"
    );
    return nullptr;

}

// constructor method
int VectorPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    (void)args;
    return 0;
}

int VectorPy::initialization()
{
    assert(_pcTwinPointer == nullptr);
    _pcTwinPointer = &value;
    return 0;
}

int VectorPy::finalization()
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string VectorPy::representation(void) const
{
    return value.repr();
}



PyObject* VectorPy::dot(PyObject *args)
{
    (void)args;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* VectorPy::cross(PyObject *args)
{
    (void)args;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* VectorPy::normalized(PyObject *args)
{
    (void)args;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* VectorPy::number_add_handler(PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject* VectorPy::number_subtract_handler(PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject* VectorPy::number_multiply_handler(PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_power_handler (PyObject* /*self*/, PyObject* /*other*/, PyObject* /*modulo*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_negative_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_positive_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

int VectorPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * VectorPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_int_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * VectorPy::number_float_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

Py_ssize_t VectorPy::sequence_length(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return -1;
}

PyObject * VectorPy::sequence_item(PyObject *, Py_ssize_t)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject * VectorPy::mapping_subscript(PyObject *, PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

int VectorPy::sequence_ass_item(PyObject *, Py_ssize_t, PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return -1;
}

PyObject* VectorPy::richCompare(PyObject *v, PyObject *w, int op)
{
    (void)v;
    (void)w;
    (void)op;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

Py::Tuple VectorPy::getLength(void) const
{
    //return Py::Tuple();
    throw Py::AttributeError("Not yet implemented");
}

Py::Object VectorPy::getx(void) const
{
    return Py::asObject(value.x.getPyObject());
}

void  VectorPy::setx(Py::Object arg)
{
    (void)arg;
    throw Py::AttributeError("Not yet implemented");
}

Py::Object VectorPy::gety(void) const
{
    return Py::asObject(value.y.getPyObject());
}

void  VectorPy::sety(Py::Object arg)
{
    (void)arg;
    throw Py::AttributeError("Not yet implemented");
}

Py::Object VectorPy::getre(void) const
{
    Base::Vector3d v(value.x.re, value.y.re, 0);
    return Py::asObject(new Base::VectorPy(new Base::Vector3d(v)));
}

PyObject *VectorPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int VectorPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

