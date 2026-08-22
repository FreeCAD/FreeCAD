/***************************************************************************
 *   Copyright (c) 2026 Zheng, Lei <realthunder.dev@gmail.com>             *
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

#include <algorithm>
#include <sstream>

#include "ShapeList.h"
#include "PartPyCXX.h"
#include "TopoShapePy.h"

// inclusion of the generated files (generated out of ShapeListPy.xml)
#include "ShapeListPy.h"
#include "ShapeListPy.cpp"

using namespace Part;

namespace
{

/// What Shape.ShapeType would say of an element of this list; the type map
/// has no name for TopAbs_SHAPE, which is what a list of mixed sub shapes
/// carries
std::string elementTypeName(TopAbs_ShapeEnum type)
{
    if (type == TopAbs_SHAPE) {
        return "Shape";
    }
    return TopoShape::shapeName(type, true);
}

/// The shape a python object holds, or null if it holds none
const TopoShape *shapeOf(PyObject *obj)
{
    if (!obj || !PyObject_TypeCheck(obj, &(TopoShapePy::Type))) {
        return nullptr;
    }
    return static_cast<TopoShapePy *>(obj)->getTopoShapePtr();
}

/// Read a sequence of shapes, raising and answering false on anything else
bool shapesOf(PyObject *obj, std::vector<TopoShape> &out)
{
    if (!obj) {
        return false;
    }
    if (PyObject_TypeCheck(obj, &(ShapeListPy::Type))) {
        const ShapeList &other = static_cast<ShapeListPy *>(obj)->list();
        int count = other.size();
        out.reserve(out.size() + static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            out.push_back(other.get(i));
        }
        return true;
    }
    PyObject *iterator = PyObject_GetIter(obj);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "expected a sequence of shapes");
        return false;
    }
    Py::Object guard(iterator, true);
    while (PyObject *item = PyIter_Next(iterator)) {
        Py::Object hold(item, true);
        const TopoShape *shape = shapeOf(item);
        if (!shape) {
            PyErr_SetString(PyExc_TypeError, "expected a sequence of shapes");
            return false;
        }
        out.push_back(*shape);
    }
    return !PyErr_Occurred();
}

/// Turn a python index into a position in a list of \a count entries.
/// Raises IndexError and answers false when it is not one.
bool indexOf(Py_ssize_t given, int count, int &idx, bool allowEnd = false)
{
    Py_ssize_t value = given < 0 ? given + count : given;
    if (value < 0 || value > count || (!allowEnd && value == count)) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return false;
    }
    idx = static_cast<int>(value);
    return true;
}

/// A new python list of the elements, which is what a slice and every
/// operation that has to hand out several elements at once answers with
Py::List asPyList(const ShapeList &list, Py_ssize_t start, Py_ssize_t step, Py_ssize_t count)
{
    Py::List res;
    for (Py_ssize_t i = 0; i < count; ++i) {
        res.append(shape2pyshape(list.get(static_cast<int>(start + i * step))));
    }
    return res;
}

}  // namespace

std::string ShapeListPy::representation() const
{
    const ShapeList &values = list();
    std::ostringstream str;
    str << "<ShapeList: " << values.size() << " " << elementTypeName(values.getType());
    if (!values.isView()) {
        str << ", detached";
    }
    str << ">";
    return str.str();
}

PyObject *ShapeListPy::PyMake(PyTypeObject * /*type*/, PyObject * /*args*/, PyObject * /*kwds*/)
{
    return new ShapeListPy(new ShapeList);
}

int ShapeListPy::PyInit(PyObject *args, PyObject * /*kwds*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }
    PyErr_Clear();

    PyObject *source = nullptr;
    const char *type = nullptr;
    const char *avoid = nullptr;
    if (PyArg_ParseTuple(args, "O!s|s", &(TopoShapePy::Type), &source, &type, &avoid)) {
        PY_TRY
        {
            *getShapeListPtr() =
                ShapeList(*static_cast<TopoShapePy *>(source)->getTopoShapePtr(),
                          TopoShape::shapeType(type),
                          avoid && avoid[0] ? TopoShape::shapeType(avoid) : TopAbs_SHAPE);
            return 0;
        }
        _PY_CATCH(return -1)
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "O", &source)) {
        std::vector<TopoShape> shapes;
        if (!shapesOf(source, shapes)) {
            return -1;
        }
        PY_TRY
        {
            *getShapeListPtr() = ShapeList(std::move(shapes));
            return 0;
        }
        _PY_CATCH(return -1)
    }

    PyErr_SetString(PyExc_TypeError,
                    "supported signatures:\n"
                    "ShapeList()\n"
                    "ShapeList(sequence of Shape)\n"
                    "ShapeList(Shape, type, [avoid])");
    return -1;
}

// ---------------------------------------------------------------------
// Attributes

Py::Int ShapeListPy::getCount() const
{
    return Py::Int(list().size());
}

Py::String ShapeListPy::getElementType() const
{
    return Py::String(elementTypeName(list().getType()));
}

Py::Boolean ShapeListPy::getIsView() const
{
    return Py::Boolean(list().isView());
}

Py::Object ShapeListPy::getShape() const
{
    if (!list().isView()) {
        return Py::None();
    }
    return shape2pyshape(list().getParent());
}

PyObject *ShapeListPy::getCustomAttributes(const char * /*attr*/) const
{
    return nullptr;
}

int ShapeListPy::setCustomAttributes(const char * /*attr*/, PyObject * /*obj*/)
{
    return 0;
}

// ---------------------------------------------------------------------
// Reading methods

PyObject *ShapeListPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        return new ShapeListPy(new ShapeList(list()));
    }
    PY_CATCH
}

PyObject *ShapeListPy::index(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        int idx = list().find(*static_cast<TopoShapePy *>(obj)->getTopoShapePtr());
        if (idx < 0) {
            PyErr_SetString(PyExc_ValueError, "shape is not in the list");
            return nullptr;
        }
        return Py::new_reference_to(Py::Int(idx));
    }
    PY_CATCH
}

PyObject *ShapeListPy::count(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape &wanted = *static_cast<TopoShapePy *>(obj)->getTopoShapePtr();
        int found = 0;
        const ShapeList &values = list();
        int total = values.size();
        for (int i = 0; i < total; ++i) {
            TopoShape element = values.get(i);
            if (!element.isNull() && !wanted.isNull()
                && element.getShape().IsSame(wanted.getShape())) {
                ++found;
            }
        }
        return Py::new_reference_to(Py::Int(found));
    }
    PY_CATCH
}

// ---------------------------------------------------------------------
// Writing methods, each of which makes the list a value of its own

PyObject *ShapeListPy::append(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape added = *static_cast<TopoShapePy *>(obj)->getTopoShapePtr();
        list().edit([&](std::vector<TopoShape> &values) { values.push_back(added); });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::extend(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }
    std::vector<TopoShape> added;
    if (!shapesOf(obj, added)) {
        return nullptr;
    }
    PY_TRY
    {
        list().edit([&](std::vector<TopoShape> &values) {
            values.insert(values.end(), added.begin(), added.end());
        });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::insert(PyObject *args)
{
    Py_ssize_t where = 0;
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "nO!", &where, &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape added = *static_cast<TopoShapePy *>(obj)->getTopoShapePtr();
        // list.insert() clamps rather than raising, and so does this
        const int count = list().size();
        Py_ssize_t value = where < 0 ? where + count : where;
        if (value < 0) {
            value = 0;
        }
        if (value > count) {
            value = count;
        }
        list().edit([&](std::vector<TopoShape> &values) {
            values.insert(values.begin() + static_cast<std::ptrdiff_t>(value), added);
        });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::pop(PyObject *args)
{
    Py_ssize_t where = -1;
    if (!PyArg_ParseTuple(args, "|n", &where)) {
        return nullptr;
    }
    const int count = list().size();
    if (!count) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty list");
        return nullptr;
    }
    int idx = 0;
    if (!indexOf(where, count, idx)) {
        return nullptr;
    }
    PY_TRY
    {
        TopoShape removed;
        list().edit([&](std::vector<TopoShape> &values) {
            removed = values[idx];
            values.erase(values.begin() + idx);
        });
        return Py::new_reference_to(shape2pyshape(removed));
    }
    PY_CATCH
}

PyObject *ShapeListPy::remove(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        int idx = list().find(*static_cast<TopoShapePy *>(obj)->getTopoShapePtr());
        if (idx < 0) {
            PyErr_SetString(PyExc_ValueError, "shape is not in the list");
            return nullptr;
        }
        list().edit([&](std::vector<TopoShape> &values) {
            values.erase(values.begin() + idx);
        });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::reverse(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        list().edit([](std::vector<TopoShape> &values) {
            std::reverse(values.begin(), values.end());
        });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        list().edit([](std::vector<TopoShape> &values) { values.clear(); });
        Py_Return;
    }
    PY_CATCH
}

PyObject *ShapeListPy::sort(PyObject *args, PyObject *kwds)
{
    // Handed to list.sort() rather than reimplemented, so that key= and
    // reverse= mean exactly what they mean everywhere else
    Py::List ordered;
    PY_TRY
    {
        const ShapeList &values = list();
        int count = values.size();
        for (int i = 0; i < count; ++i) {
            ordered.append(shape2pyshape(values.get(i)));
        }
    }
    PY_CATCH

    Py::Object sorter = ordered.getAttr("sort");
    PyObject *res = PyObject_Call(sorter.ptr(), args, kwds);
    if (!res) {
        return nullptr;
    }
    Py_DECREF(res);

    std::vector<TopoShape> sorted;
    if (!shapesOf(ordered.ptr(), sorted)) {
        return nullptr;
    }
    PY_TRY
    {
        list().edit([&](std::vector<TopoShape> &values) { values = sorted; });
        Py_Return;
    }
    PY_CATCH
}

// ---------------------------------------------------------------------
// Sequence protocol

Py_ssize_t ShapeListPy::sequence_length(PyObject *self)
{
    return static_cast<ShapeListPy *>(self)->list().size();
}

PyObject *ShapeListPy::sequence_item(PyObject *self, Py_ssize_t index)
{
    auto *py = static_cast<ShapeListPy *>(self);
    int idx = 0;
    if (!indexOf(index, py->list().size(), idx)) {
        return nullptr;
    }
    PY_TRY
    {
        return Py::new_reference_to(shape2pyshape(py->list().get(idx)));
    }
    PY_CATCH
}

int ShapeListPy::sequence_ass_item(PyObject *self, Py_ssize_t index, PyObject *value)
{
    auto *py = static_cast<ShapeListPy *>(self);
    int idx = 0;
    if (!indexOf(index, py->list().size(), idx)) {
        return -1;
    }
    PY_TRY
    {
        if (!value) {
            py->list().edit([&](std::vector<TopoShape> &values) {
                values.erase(values.begin() + idx);
            });
            return 0;
        }
        const TopoShape *shape = shapeOf(value);
        if (!shape) {
            PyErr_SetString(PyExc_TypeError, "expected a shape");
            return -1;
        }
        const TopoShape written = *shape;
        py->list().edit([&](std::vector<TopoShape> &values) { values[idx] = written; });
        return 0;
    }
    _PY_CATCH(return -1)
}

PyObject *ShapeListPy::mapping_subscript(PyObject *self, PyObject *item)
{
    auto *py = static_cast<ShapeListPy *>(self);
    if (PyIndex_Check(item)) {
        const Py_ssize_t index = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (index == -1 && PyErr_Occurred()) {
            return nullptr;
        }
        return sequence_item(self, index);
    }
    if (PySlice_Check(item)) {
        PY_TRY
        {
            Py_ssize_t start = 0;
            Py_ssize_t stop = 0;
            Py_ssize_t step = 0;
            Py_ssize_t count = 0;
            if (PySlice_GetIndicesEx(item, py->list().size(), &start, &stop, &step, &count) < 0) {
                return nullptr;
            }
            // A plain list, because that is what slicing a list gives and
            // what every caller that slices one goes on to do with it
            return Py::new_reference_to(asPyList(py->list(), start, step, count));
        }
        PY_CATCH
    }
    PyErr_SetString(PyExc_TypeError, "index must be an integer or a slice");
    return nullptr;
}

int ShapeListPy::mapping_ass_subscript(PyObject *self, PyObject *item, PyObject *value)
{
    auto *py = static_cast<ShapeListPy *>(self);
    if (PyIndex_Check(item)) {
        const Py_ssize_t index = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (index == -1 && PyErr_Occurred()) {
            return -1;
        }
        return sequence_ass_item(self, index, value);
    }
    if (!PySlice_Check(item)) {
        PyErr_SetString(PyExc_TypeError, "index must be an integer or a slice");
        return -1;
    }
    PY_TRY
    {
        Py_ssize_t start = 0;
        Py_ssize_t stop = 0;
        Py_ssize_t step = 0;
        Py_ssize_t count = 0;
        if (PySlice_GetIndicesEx(item, py->list().size(), &start, &stop, &step, &count) < 0) {
            return -1;
        }
        if (step != 1) {
            PyErr_SetString(PyExc_ValueError, "only a contiguous slice can be assigned to");
            return -1;
        }
        std::vector<TopoShape> written;
        if (value && !shapesOf(value, written)) {
            return -1;
        }
        py->list().edit([&](std::vector<TopoShape> &values) {
            auto first = values.begin() + start;
            values.erase(first, first + count);
            values.insert(values.begin() + start, written.begin(), written.end());
        });
        return 0;
    }
    _PY_CATCH(return -1)
}

int ShapeListPy::sequence_contains(PyObject *self, PyObject *value)
{
    const TopoShape *shape = shapeOf(value);
    if (!shape) {
        return 0;
    }
    PY_TRY
    {
        return static_cast<ShapeListPy *>(self)->list().find(*shape) >= 0 ? 1 : 0;
    }
    _PY_CATCH(return -1)
}

PyObject *ShapeListPy::sequence_concat(PyObject *self, PyObject *other)
{
    PY_TRY
    {
        std::vector<TopoShape> values;
        if (!shapesOf(self, values) || !shapesOf(other, values)) {
            return nullptr;
        }
        return new ShapeListPy(new ShapeList(std::move(values)));
    }
    PY_CATCH
}

PyObject *ShapeListPy::sequence_repeat(PyObject *self, Py_ssize_t times)
{
    PY_TRY
    {
        const ShapeList &source = static_cast<ShapeListPy *>(self)->list();
        const int count = source.size();
        std::vector<TopoShape> values;
        if (times > 0) {
            values.reserve(static_cast<std::size_t>(count) * static_cast<std::size_t>(times));
        }
        for (Py_ssize_t n = 0; n < times; ++n) {
            for (int i = 0; i < count; ++i) {
                values.push_back(source.get(i));
            }
        }
        return new ShapeListPy(new ShapeList(std::move(values)));
    }
    PY_CATCH
}

PyObject *ShapeListPy::sequence_inplace_concat(PyObject *self, PyObject *other)
{
    std::vector<TopoShape> added;
    if (!shapesOf(other, added)) {
        return nullptr;
    }
    PY_TRY
    {
        static_cast<ShapeListPy *>(self)->list().edit([&](std::vector<TopoShape> &values) {
            values.insert(values.end(), added.begin(), added.end());
        });
        Py_INCREF(self);
        return self;
    }
    PY_CATCH
}

// ---------------------------------------------------------------------
// Comparison

PyObject *ShapeListPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (op != Py_EQ && op != Py_NE) {
        PyErr_SetString(PyExc_TypeError, "shape lists have no ordering");
        return nullptr;
    }
    if (!PyObject_TypeCheck(v, &(ShapeListPy::Type)) || !PySequence_Check(w)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    const ShapeList &left = static_cast<ShapeListPy *>(v)->list();
    std::vector<TopoShape> right;
    if (!shapesOf(w, right)) {
        PyErr_Clear();
        Py_RETURN_NOTIMPLEMENTED;
    }
    bool equal = static_cast<int>(right.size()) == left.size();
    for (std::size_t i = 0; equal && i < right.size(); ++i) {
        TopoShape element = left.get(static_cast<int>(i));
        equal = !element.isNull() && !right[i].isNull()
            && element.getShape().IsSame(right[i].getShape());
    }
    if (op == Py_NE) {
        equal = !equal;
    }
    return Py::new_reference_to(Py::Boolean(equal));
}

// ---------------------------------------------------------------------
// Number protocol
//
// Only three of these mean anything for a list. They are here because
// "+" has to work with a plain list on EITHER side: python asks both
// operands' nb_add before it falls back to the left one's sq_concat, and
// list has no nb_add, so [edge] + shape.Edges reaches this and nothing
// else. The rest answer NotImplemented, which is what python turns into
// the TypeError a wrong operand deserves.

PyObject *ShapeListPy::number_add_handler(PyObject *self, PyObject *other)
{
    PyObject *list = PyObject_TypeCheck(self, &(ShapeListPy::Type)) ? self : other;
    if (!PyObject_TypeCheck(list, &(ShapeListPy::Type))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (!PySequence_Check(self) || !PySequence_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PY_TRY
    {
        std::vector<TopoShape> values;
        if (!shapesOf(self, values) || !shapesOf(other, values)) {
            PyErr_Clear();
            Py_RETURN_NOTIMPLEMENTED;
        }
        return new ShapeListPy(new ShapeList(std::move(values)));
    }
    PY_CATCH
}

PyObject *ShapeListPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    PyObject *list = PyObject_TypeCheck(self, &(ShapeListPy::Type)) ? self : other;
    PyObject *number = list == self ? other : self;
    if (!PyObject_TypeCheck(list, &(ShapeListPy::Type)) || !PyIndex_Check(number)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    const Py_ssize_t times = PyNumber_AsSsize_t(number, PyExc_OverflowError);
    if (times == -1 && PyErr_Occurred()) {
        return nullptr;
    }
    return sequence_repeat(list, times);
}

int ShapeListPy::number_nonzero_handler(PyObject *self)
{
    return static_cast<ShapeListPy *>(self)->list().empty() ? 0 : 1;
}

#define SHAPELIST_NO_NUMBER(_name)                                                                 \
    PyObject *ShapeListPy::_name(PyObject * /*self*/, PyObject * /*other*/)                        \
    {                                                                                              \
        Py_RETURN_NOTIMPLEMENTED;                                                                  \
    }

SHAPELIST_NO_NUMBER(number_subtract_handler)
SHAPELIST_NO_NUMBER(number_divide_handler)
SHAPELIST_NO_NUMBER(number_remainder_handler)
SHAPELIST_NO_NUMBER(number_divmod_handler)
SHAPELIST_NO_NUMBER(number_lshift_handler)
SHAPELIST_NO_NUMBER(number_rshift_handler)
SHAPELIST_NO_NUMBER(number_and_handler)
SHAPELIST_NO_NUMBER(number_xor_handler)
SHAPELIST_NO_NUMBER(number_or_handler)

#undef SHAPELIST_NO_NUMBER

#define SHAPELIST_NO_UNARY(_name)                                                                  \
    PyObject *ShapeListPy::_name(PyObject * /*self*/)                                              \
    {                                                                                              \
        Py_RETURN_NOTIMPLEMENTED;                                                                  \
    }

SHAPELIST_NO_UNARY(number_negative_handler)
SHAPELIST_NO_UNARY(number_positive_handler)
SHAPELIST_NO_UNARY(number_absolute_handler)
SHAPELIST_NO_UNARY(number_invert_handler)
SHAPELIST_NO_UNARY(number_int_handler)
SHAPELIST_NO_UNARY(number_float_handler)

#undef SHAPELIST_NO_UNARY

PyObject *ShapeListPy::number_power_handler(PyObject * /*self*/,
                                            PyObject * /*other*/,
                                            PyObject * /*modulo*/)
{
    Py_RETURN_NOTIMPLEMENTED;
}
