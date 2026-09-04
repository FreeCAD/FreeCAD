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
const TopoShape* shapeOf(PyObject* obj)
{
    if (!obj || !PyObject_TypeCheck(obj, &(TopoShapePy::Type))) {
        return nullptr;
    }
    return static_cast<TopoShapePy*>(obj)->getTopoShapePtr();
}

bool isShapeList(PyObject* obj)
{
    return obj && PyObject_TypeCheck(obj, &(ShapeListPy::Type));
}

bool isListLike(PyObject* obj)
{
    return isShapeList(obj) || (obj && PyList_Check(obj));
}

bool sameShape(const TopoShape& left, const TopoShape& right)
{
    return (left.isNull() && right.isNull())
        || (!left.isNull() && !right.isNull() && left.getShape().IsSame(right.getShape()));
}

/// Return a real Python list of shape objects and validate its elements. This
/// is used before delegating a mutation to Python's own list implementation.
bool shapeObjectsOf(PyObject* obj, Py::List& out)
{
    PyObject* raw = PySequence_List(obj);
    if (!raw) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError, "expected an iterable of shapes");
        }
        return false;
    }
    out = Py::List(raw, true);
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(out.ptr()); ++i) {
        if (!shapeOf(PyList_GET_ITEM(out.ptr(), i))) {
            PyErr_SetString(PyExc_TypeError, "expected a sequence of shapes");
            return false;
        }
    }
    return true;
}

/// Turn a python index into a position in a list of \a count entries.
/// Raises IndexError and answers false when it is not one.
bool indexOf(Py_ssize_t given, int count, int& idx, bool allowEnd = false)
{
    Py_ssize_t value = given < 0 ? given + count : given;
    if (value < 0 || value > count || (!allowEnd && value == count)) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return false;
    }
    idx = static_cast<int>(value);
    return true;
}

struct ShapeListIterator
{
    PyObject_HEAD PyObject* owner;
    std::vector<TopoShape>* values;
    std::size_t index;
};

PyTypeObject* shapeListIteratorType = nullptr;

void shapeListIteratorDealloc(PyObject* self);
PyObject* shapeListIteratorIter(PyObject* self);
PyObject* shapeListIteratorNext(PyObject* self);

PyType_Slot shapeListIteratorSlots[] = {
    {Py_tp_dealloc, reinterpret_cast<void*>(shapeListIteratorDealloc)},
    {Py_tp_iter, reinterpret_cast<void*>(shapeListIteratorIter)},
    {Py_tp_iternext, reinterpret_cast<void*>(shapeListIteratorNext)},
    {0, nullptr},
};

PyType_Spec shapeListIteratorSpec = {
    "Part.ShapeListIterator",
    sizeof(ShapeListIterator),
    0,
    Py_TPFLAGS_DEFAULT,
    shapeListIteratorSlots,
};

void shapeListIteratorDealloc(PyObject* self)
{
    auto* iterator = reinterpret_cast<ShapeListIterator*>(self);
    delete iterator->values;
    Py_XDECREF(iterator->owner);
    Py_TYPE(self)->tp_free(self);
}

PyObject* shapeListIteratorIter(PyObject* self)
{
    Py_INCREF(self);
    return self;
}

PyObject* shapeListIteratorNext(PyObject* self)
{
    auto* iterator = reinterpret_cast<ShapeListIterator*>(self);
    if (iterator->index >= iterator->values->size()) {
        PyErr_SetNone(PyExc_StopIteration);
        return nullptr;
    }

    PY_TRY
    {
        auto* owner = static_cast<ShapeListPy*>(iterator->owner);
        const auto index = iterator->index;
        auto result
            = owner->elementObjectFromValue(static_cast<int>(index), (*iterator->values)[index]);
        ++iterator->index;
        return Py::new_reference_to(result);
    }
    PY_CATCH
}

int ensureShapeListIteratorType()
{
    if (shapeListIteratorType) {
        return 0;
    }

    PyObject* type = PyType_FromSpec(&shapeListIteratorSpec);
    if (!type) {
        return -1;
    }
    shapeListIteratorType = reinterpret_cast<PyTypeObject*>(type);
    return 0;
}

PyObject* shapeListIter(PyObject* self)
{
    auto* owner = static_cast<ShapeListPy*>(self);
    PY_TRY
    {
        auto values = std::make_unique<std::vector<TopoShape>>(owner->effectiveValues());
        if (ensureShapeListIteratorType() < 0) {
            return nullptr;
        }
        auto* iterator = PyObject_New(ShapeListIterator, shapeListIteratorType);
        if (!iterator) {
            return nullptr;
        }
        iterator->owner = self;
        Py_INCREF(self);
        iterator->values = values.release();
        iterator->index = 0;
        return reinterpret_cast<PyObject*>(iterator);
    }
    PY_CATCH
}

/// A new python list of the elements, which is what a slice and every
/// operation that has to hand out several elements at once answers with
Py::List asPyList(ShapeListPy& list)
{
    const auto values = list.effectiveValues();
    Py::List res;
    for (std::size_t i = 0; i < values.size(); ++i) {
        res.append(list.elementObjectFromValue(static_cast<int>(i), values[i]));
    }
    return res;
}

}  // namespace

Py::Object ShapeListPy::elementObject(int index)
{
    if (index < 0 || index >= size()) {
        throw Py::IndexError("shape list index out of range");
    }
    return elementObjectUnchecked(index);
}

Py::Object ShapeListPy::elementObjectUnchecked(int index)
{
    if (_materialised) {
        return Py::Object(PyList_GET_ITEM(_materialised->ptr(), index));
    }
    return elementObjectFromValue(index, list().getUnchecked(index));
}

Py::Object ShapeListPy::elementObjectFromValue(int index, const TopoShape& value)
{
    if (_materialised && index >= 0 && index < PyList_GET_SIZE(_materialised->ptr())) {
        return Py::Object(PyList_GET_ITEM(_materialised->ptr(), index));
    }
    if (_materialised) {
        // An iterator owns a snapshot of the C++ values. If the source list
        // shrank after the iterator was created, the snapshot still has one
        // more value to yield and there is no current list slot to reuse.
        return shape2pyshape(value);
    }
    auto found = _touched.find(index);
    if (found == _touched.end()) {
        auto object = std::make_shared<Py::Object>(shape2pyshape(value));
        found = _touched.emplace(index, std::move(object)).first;
    }
    return *found->second;
}

TopoShape ShapeListPy::effectiveElement(int index) const
{
    if (index < 0 || index >= size()) {
        return TopoShape();
    }
    return effectiveElementUnchecked(index);
}

TopoShape ShapeListPy::effectiveElementUnchecked(int index) const
{
    if (_materialised) {
        const TopoShape* shape = shapeOf(PyList_GET_ITEM(_materialised->ptr(), index));
        if (shape) {
            return *shape;
        }
        throw Py::TypeError("expected a shape");
    }
    auto found = _touched.find(index);
    if (found != _touched.end()) {
        if (const TopoShape* shape = shapeOf(found->second->ptr())) {
            return *shape;
        }
    }
    return list().getUnchecked(index);
}

void ShapeListPy::overlayCachedElements(std::vector<TopoShape>& values) const
{
    for (const auto& [index, object] : _touched) {
        if (index < 0 || index >= static_cast<int>(values.size()) || !object) {
            continue;
        }
        if (const TopoShape* shape = shapeOf(object->ptr())) {
            values[static_cast<std::size_t>(index)] = *shape;
        }
    }
}

void ShapeListPy::materialisePythonList()
{
    if (_materialised) {
        return;
    }

    const auto values = effectiveValues();
    auto result = std::make_unique<Py::List>();
    for (std::size_t i = 0; i < values.size(); ++i) {
        auto found = _touched.find(static_cast<int>(i));
        if (found != _touched.end()) {
            result->append(*found->second);
        }
        else {
            result->append(shape2pyshape(values[i]));
        }
    }
    _materialised = std::move(result);
    _touched.clear();
    list().materialise();
}

std::vector<TopoShape> ShapeListPy::effectiveValues() const
{
    if (_materialised) {
        std::vector<TopoShape> values;
        values.reserve(static_cast<std::size_t>(PyList_GET_SIZE(_materialised->ptr())));
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(_materialised->ptr()); ++i) {
            const TopoShape* shape = shapeOf(PyList_GET_ITEM(_materialised->ptr(), i));
            if (!shape) {
                throw Py::TypeError("expected a shape");
            }
            values.push_back(*shape);
        }
        return values;
    }
    std::vector<TopoShape> values = list().values();
    overlayCachedElements(values);
    return values;
}

std::string ShapeListPy::representation() const
{
    const ShapeList& values = list();
    std::ostringstream str;
    str << "<ShapeList: " << size() << " " << elementTypeName(values.getType());
    if (isMaterialised() || !values.isView()) {
        str << ", detached";
    }
    str << ">";
    return str.str();
}

PyObject* ShapeListPy::PyMake(PyTypeObject* /*type*/, PyObject* /*args*/, PyObject* /*kwds*/)
{
    return new ShapeListPy(new ShapeList);
}

int ShapeListPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }
    PyErr_Clear();

    PyObject* source = nullptr;
    const char* type = nullptr;
    const char* avoid = nullptr;
    if (PyArg_ParseTuple(args, "O!s|s", &(TopoShapePy::Type), &source, &type, &avoid)) {
        PY_TRY
        {
            *getShapeListPtr() = ShapeList(
                *static_cast<TopoShapePy*>(source)->getTopoShapePtr(),
                TopoShape::shapeType(type),
                avoid && avoid[0] ? TopoShape::shapeType(avoid) : TopAbs_SHAPE
            );
            return 0;
        }
        _PY_CATCH(return -1)
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "O", &source)) {
        Py::List items;
        if (!shapeObjectsOf(source, items)) {
            return -1;
        }
        PY_TRY
        {
            std::vector<TopoShape> values;
            values.reserve(static_cast<std::size_t>(PyList_GET_SIZE(items.ptr())));
            for (Py_ssize_t i = 0; i < PyList_GET_SIZE(items.ptr()); ++i) {
                values.push_back(*shapeOf(PyList_GET_ITEM(items.ptr(), i)));
            }
            *getShapeListPtr() = ShapeList(std::move(values));
            _materialised = std::make_unique<Py::List>(items);
            return 0;
        }
        _PY_CATCH(return -1)
    }

    PyErr_SetString(
        PyExc_TypeError,
        "supported signatures:\n"
        "ShapeList()\n"
        "ShapeList(sequence of Shape)\n"
        "ShapeList(Shape, type, [avoid])"
    );
    return -1;
}

// ---------------------------------------------------------------------
// Attributes

Py::Long ShapeListPy::getCount() const
{
    return Py::Long(size());
}

int ShapeListPy::size() const
{
    return _materialised ? static_cast<int>(PyList_GET_SIZE(_materialised->ptr())) : list().size();
}

Py::String ShapeListPy::getElementType() const
{
    const TopAbs_ShapeEnum type = list().getType();
    if (type == TopAbs_SHAPE) {
        return Py::String(elementTypeName(type));
    }

    auto changedType = [&](const TopoShape* shape) {
        return !shape || shape->isNull() || shape->getShape().ShapeType() != type;
    };
    if (_materialised) {
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(_materialised->ptr()); ++i) {
            if (changedType(shapeOf(PyList_GET_ITEM(_materialised->ptr(), i)))) {
                return Py::String(elementTypeName(TopAbs_SHAPE));
            }
        }
    }
    else {
        for (const auto& [index, object] : _touched) {
            (void)index;
            if (object && changedType(shapeOf(object->ptr()))) {
                return Py::String(elementTypeName(TopAbs_SHAPE));
            }
        }
    }
    return Py::String(elementTypeName(type));
}

Py::Boolean ShapeListPy::getIsView() const
{
    return Py::Boolean(!isMaterialised() && list().isView());
}

Py::Object ShapeListPy::getShape() const
{
    if (isMaterialised() || !list().isView()) {
        return Py::None();
    }
    return shape2pyshape(list().getParent());
}

PyObject* ShapeListPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeListPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

// ---------------------------------------------------------------------
// Reading methods

PyObject* ShapeListPy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        return Py::new_reference_to(asPyList(*this));
    }
    PY_CATCH
}

PyObject* ShapeListPy::index(PyObject* args)
{
    PyObject* obj = nullptr;
    PyObject* startObj = nullptr;
    PyObject* stopObj = nullptr;
    if (!PyArg_ParseTuple(args, "O|OO:index", &obj, &startObj, &stopObj)) {
        return nullptr;
    }
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;
    if (startObj) {
        start = PyNumber_AsSsize_t(startObj, nullptr);
        if (start == -1 && PyErr_Occurred()) {
            return nullptr;
        }
    }
    if (stopObj) {
        stop = PyNumber_AsSsize_t(stopObj, nullptr);
        if (stop == -1 && PyErr_Occurred()) {
            return nullptr;
        }
    }
    PY_TRY
    {
        const TopoShape* wanted = shapeOf(obj);
        const Py_ssize_t count = size();
        if (start < 0) {
            start = std::max<Py_ssize_t>(0, start + count);
        }
        if (stop < 0) {
            stop = std::max<Py_ssize_t>(0, stop + count);
        }
        int idx = -1;
        if (wanted) {
            const Py_ssize_t end = std::min(stop, count);
            for (Py_ssize_t i = start; i < end; ++i) {
                if (sameShape(effectiveElementUnchecked(static_cast<int>(i)), *wanted)) {
                    idx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (idx < 0) {
            PyErr_Format(PyExc_ValueError, "%R is not in list", obj);
            return nullptr;
        }
        return Py::new_reference_to(Py::Long(idx));
    }
    PY_CATCH
}

PyObject* ShapeListPy::count(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O:count", &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape* wanted = shapeOf(obj);
        if (!wanted) {
            return Py::new_reference_to(Py::Long(0));
        }
        int found = 0;
        int total = size();
        for (int i = 0; i < total; ++i) {
            if (sameShape(effectiveElementUnchecked(i), *wanted)) {
                ++found;
            }
        }
        return Py::new_reference_to(Py::Long(found));
    }
    PY_CATCH
}

// ---------------------------------------------------------------------
// Writing methods. The first one materialises a real Python list and all
// subsequent operations delegate to that list.

PyObject* ShapeListPy::append(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape added = *static_cast<TopoShapePy*>(obj)->getTopoShapePtr();
        materialisePythonList();
        if (PyList_Append(_materialised->ptr(), obj) < 0) {
            return nullptr;
        }
        list().noteType(added);
        Py_Return;
    }
    PY_CATCH
}

PyObject* ShapeListPy::extend(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }
    Py::List added;
    if (!shapeObjectsOf(obj, added)) {
        return nullptr;
    }
    PY_TRY
    {
        materialisePythonList();
        const Py_ssize_t count = PyList_GET_SIZE(_materialised->ptr());
        if (PyList_SetSlice(_materialised->ptr(), count, count, added.ptr()) < 0) {
            return nullptr;
        }
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(added.ptr()); ++i) {
            list().noteType(*shapeOf(PyList_GET_ITEM(added.ptr(), i)));
        }
        Py_Return;
    }
    PY_CATCH
}

PyObject* ShapeListPy::insert(PyObject* args)
{
    Py_ssize_t where = 0;
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "nO!", &where, &(TopoShapePy::Type), &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape added = *static_cast<TopoShapePy*>(obj)->getTopoShapePtr();
        materialisePythonList();
        if (PyList_Insert(_materialised->ptr(), where, obj) < 0) {
            return nullptr;
        }
        list().noteType(added);
        Py_Return;
    }
    PY_CATCH
}

PyObject* ShapeListPy::pop(PyObject* args)
{
    Py_ssize_t where = -1;
    if (!PyArg_ParseTuple(args, "|n", &where)) {
        return nullptr;
    }
    const int count = size();
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
        materialisePythonList();
        Py::Object removed(PySequence_GetItem(_materialised->ptr(), idx), true);
        if (removed.isNull()) {
            return nullptr;
        }
        if (PySequence_DelItem(_materialised->ptr(), idx) < 0) {
            return nullptr;
        }
        return Py::new_reference_to(removed);
    }
    PY_CATCH
}

PyObject* ShapeListPy::remove(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O:remove", &obj)) {
        return nullptr;
    }
    PY_TRY
    {
        const TopoShape* wanted = shapeOf(obj);
        int idx = -1;
        if (wanted) {
            for (int i = 0; i < size(); ++i) {
                if (sameShape(effectiveElementUnchecked(i), *wanted)) {
                    idx = i;
                    break;
                }
            }
        }
        if (idx < 0) {
            PyErr_Format(PyExc_ValueError, "%R is not in list", obj);
            return nullptr;
        }
        materialisePythonList();
        if (PySequence_DelItem(_materialised->ptr(), idx) < 0) {
            return nullptr;
        }
        Py_Return;
    }
    PY_CATCH
}

PyObject* ShapeListPy::reverse(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        materialisePythonList();
        Py::Object result(PyObject_CallMethod(_materialised->ptr(), "reverse", nullptr), true);
        if (result.isNull()) {
            return nullptr;
        }
        Py_Return;
    }
    PY_CATCH
}

PyObject* ShapeListPy::clear(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    PY_TRY
    {
        materialisePythonList();
        if (PyList_SetSlice(_materialised->ptr(), 0, PyList_GET_SIZE(_materialised->ptr()), nullptr)
            < 0) {
            return nullptr;
        }
        Py_Return;
    }
    PY_CATCH
}

// clang-format off
PyObject* ShapeListPy::sort(PyObject* args, PyObject* kwds)
{
    // Hand the actual materialised list to list.sort(), so key callbacks,
    // re-entrant mutation, and error recovery have Python's normal behavior.
    PY_TRY
    {
        materialisePythonList();
        Py::Object sorter(PyObject_GetAttrString(_materialised->ptr(), "sort"), true);
        PyObject* res = PyObject_Call(sorter.ptr(), args, kwds);
        if (!res) {
            return nullptr;
        }
        Py_DECREF(res);
        Py_Return;
    }
    PY_CATCH
}
// clang-format on

// ---------------------------------------------------------------------
// Sequence protocol

Py_ssize_t ShapeListPy::sequence_length(PyObject* self)
{
    return static_cast<ShapeListPy*>(self)->size();
}

PyObject* ShapeListPy::sequence_item(PyObject* self, Py_ssize_t index)
{
    auto* py = static_cast<ShapeListPy*>(self);
    int idx = 0;
    if (!indexOf(index, py->size(), idx)) {
        return nullptr;
    }
    PY_TRY
    {
        return Py::new_reference_to(py->elementObjectUnchecked(idx));
    }
    PY_CATCH
}

int ShapeListPy::sequence_ass_item(PyObject* self, Py_ssize_t index, PyObject* value)
{
    auto* py = static_cast<ShapeListPy*>(self);
    int idx = 0;
    if (!indexOf(index, py->size(), idx)) {
        return -1;
    }
    PY_TRY
    {
        if (!value) {
            py->materialisePythonList();
            if (PySequence_DelItem(py->materialisedList(), idx) < 0) {
                return -1;
            }
            return 0;
        }
        const TopoShape* shape = shapeOf(value);
        if (!shape) {
            PyErr_SetString(PyExc_TypeError, "expected a shape");
            return -1;
        }
        const TopoShape written = *shape;
        py->materialisePythonList();
        PyObject* key = PyLong_FromSsize_t(idx);
        if (!key) {
            return -1;
        }
        const int result = PyObject_SetItem(py->materialisedList(), key, value);
        Py_DECREF(key);
        if (result < 0) {
            return -1;
        }
        py->list().noteType(written);
        return 0;
    }
    _PY_CATCH(return -1)
}

PyObject* ShapeListPy::mapping_subscript(PyObject* self, PyObject* item)
{
    auto* py = static_cast<ShapeListPy*>(self);
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
            if (PySlice_GetIndicesEx(item, py->size(), &start, &stop, &step, &count) < 0) {
                return nullptr;
            }
            Py::List result;
            for (Py_ssize_t i = 0; i < count; ++i) {
                result.append(py->elementObjectUnchecked(static_cast<int>(start + i * step)));
            }
            return Py::new_reference_to(result);
        }
        PY_CATCH
    }
    PyErr_SetString(PyExc_TypeError, "index must be an integer or a slice");
    return nullptr;
}

int ShapeListPy::mapping_ass_subscript(PyObject* self, PyObject* item, PyObject* value)
{
    auto* py = static_cast<ShapeListPy*>(self);
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
        Py::List written;
        if (value && !shapeObjectsOf(value, written)) {
            return -1;
        }
        py->materialisePythonList();
        const int result = value ? PyObject_SetItem(py->materialisedList(), item, written.ptr())
                                 : PyObject_DelItem(py->materialisedList(), item);
        if (result < 0) {
            return -1;
        }
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(written.ptr()); ++i) {
            py->list().noteType(*shapeOf(PyList_GET_ITEM(written.ptr(), i)));
        }
        return 0;
    }
    _PY_CATCH(return -1)
}

int ShapeListPy::sequence_contains(PyObject* self, PyObject* value)
{
    const TopoShape* shape = shapeOf(value);
    if (!shape) {
        return 0;
    }
    PY_TRY
    {
        auto* py = static_cast<ShapeListPy*>(self);
        for (int i = 0; i < py->size(); ++i) {
            if (sameShape(py->effectiveElementUnchecked(i), *shape)) {
                return 1;
            }
        }
        return 0;
    }
    _PY_CATCH(return -1)
}

PyObject* ShapeListPy::sequence_concat(PyObject* self, PyObject* other)
{
    if (!isListLike(self) || !isListLike(other)) {
        PyErr_Format(
            PyExc_TypeError,
            "can only concatenate list (not \"%.200s\") to list",
            Py_TYPE(isListLike(self) ? other : self)->tp_name
        );
        return nullptr;
    }
    PY_TRY
    {
        Py::List result;
        auto append = [&](PyObject* source) {
            if (isShapeList(source)) {
                Py::List values = asPyList(*static_cast<ShapeListPy*>(source));
                for (Py_ssize_t i = 0; i < PyList_GET_SIZE(values.ptr()); ++i) {
                    result.append(Py::Object(PyList_GET_ITEM(values.ptr(), i)));
                }
            }
            else {
                for (Py_ssize_t i = 0; i < PyList_GET_SIZE(source); ++i) {
                    result.append(Py::Object(PyList_GET_ITEM(source, i)));
                }
            }
        };
        append(self);
        append(other);
        return Py::new_reference_to(result);
    }
    PY_CATCH
}

// clang-format off
PyObject* ShapeListPy::sequence_repeat(PyObject* self, Py_ssize_t times)
{
    PY_TRY
    {
        Py::List source = asPyList(*static_cast<ShapeListPy*>(self));
        Py::List result;
        for (Py_ssize_t n = 0; n < times; ++n) {
            for (Py_ssize_t i = 0; i < PyList_GET_SIZE(source.ptr()); ++i) {
                result.append(Py::Object(PyList_GET_ITEM(source.ptr(), i)));
            }
        }
        return Py::new_reference_to(result);
    }
    PY_CATCH
}
// clang-format on

PyObject* ShapeListPy::sequence_inplace_concat(PyObject* self, PyObject* other)
{
    Py::List added;
    if (!shapeObjectsOf(other, added)) {
        return nullptr;
    }
    PY_TRY
    {
        auto* py = static_cast<ShapeListPy*>(self);
        py->materialisePythonList();
        const Py_ssize_t count = PyList_GET_SIZE(py->materialisedList());
        if (PyList_SetSlice(py->materialisedList(), count, count, added.ptr()) < 0) {
            return nullptr;
        }
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(added.ptr()); ++i) {
            py->list().noteType(*shapeOf(PyList_GET_ITEM(added.ptr(), i)));
        }
        Py_INCREF(self);
        return self;
    }
    PY_CATCH
}

PyObject* ShapeListPy::sequence_inplace_repeat(PyObject* self, Py_ssize_t times)
{
    auto* py = static_cast<ShapeListPy*>(self);
    PY_TRY
    {
        py->materialisePythonList();
        Py::Object result(PySequence_InPlaceRepeat(py->materialisedList(), times), true);
        if (result.isNull()) {
            return nullptr;
        }
        Py_INCREF(self);
        return self;
    }
    PY_CATCH
}

// ---------------------------------------------------------------------
// Comparison

PyObject* ShapeListPy::richCompare(PyObject* v, PyObject* w, int op)
{
    if (!isListLike(v) || !isListLike(w)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PY_TRY
    {
        Py::List left = isShapeList(v) ? asPyList(*static_cast<ShapeListPy*>(v)) : Py::List(v);
        Py::List right = isShapeList(w) ? asPyList(*static_cast<ShapeListPy*>(w)) : Py::List(w);
        return PyObject_RichCompare(left.ptr(), right.ptr(), op);
    }
    PY_CATCH
}

// A ShapeList needs only these three numeric slots: Python's list type does
// not expose reflected sequence operations, so the slots make list +
// ShapeList, ShapeList + list, and both multiplication orders return a real
// list. All other numeric operations are deliberately absent.
PyObject* shapeListNumberAdd(PyObject* left, PyObject* right)
{
    if (!isListLike(left) || !isListLike(right)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return ShapeListPy::sequence_concat(left, right);
}

PyObject* shapeListNumberMultiply(PyObject* left, PyObject* right)
{
    PyObject* list = isShapeList(left) ? left : right;
    PyObject* number = list == left ? right : left;
    if (!isShapeList(list) || !PyIndex_Check(number)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    const Py_ssize_t times = PyNumber_AsSsize_t(number, PyExc_OverflowError);
    if (times == -1 && PyErr_Occurred()) {
        return nullptr;
    }
    return ShapeListPy::sequence_repeat(list, times);
}

PyObject* shapeListNumberInplaceMultiply(PyObject* self, PyObject* other)
{
    if (!isShapeList(self) || !PyIndex_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    const Py_ssize_t times = PyNumber_AsSsize_t(other, PyExc_OverflowError);
    if (times == -1 && PyErr_Occurred()) {
        return nullptr;
    }
    return ShapeListPy::sequence_inplace_repeat(self, times);
}

PyNumberMethods shapeListNumberMethods {};

struct ShapeListNumberSlots
{
    ShapeListNumberSlots()
    {
        ShapeListPy::Type.tp_iter = shapeListIter;
        shapeListNumberMethods.nb_add = shapeListNumberAdd;
        shapeListNumberMethods.nb_multiply = shapeListNumberMultiply;
        shapeListNumberMethods.nb_inplace_multiply = shapeListNumberInplaceMultiply;
        ShapeListPy::Type.tp_as_number = &shapeListNumberMethods;
    }
};

ShapeListNumberSlots shapeListNumberSlots;
