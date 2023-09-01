/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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

// inclusion of the generated files (generated out of QuantityPy.xml)
#include "QuantityPy.h"
#include "UnitPy.h"
#include "QuantityPy.cpp"


using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string QuantityPy::representation() const
{
    std::stringstream ret;

    double val= getQuantityPtr()->getValue();
    Unit unit = getQuantityPtr()->getUnit();

    // Use Python's implementation to repr() a float
    Py::Float flt(val);
    ret << static_cast<std::string>(flt.repr());
    if (!unit.isEmpty())
        ret << " " << unit.getString().toUtf8().constData();

    return ret.str();
}

PyObject* QuantityPy::toStr(PyObject* args)
{
    int prec = getQuantityPtr()->getFormat().precision;
    if (!PyArg_ParseTuple(args,"|i", &prec))
        return nullptr;

    double val= getQuantityPtr()->getValue();
    Unit unit = getQuantityPtr()->getUnit();

    std::stringstream ret;
    ret.precision(prec);
    ret.setf(std::ios::fixed, std::ios::floatfield);
    ret << val;
    if (!unit.isEmpty())
        ret << " " << unit.getString().toUtf8().constData();

    return Py_BuildValue("s", ret.str().c_str());
}

PyObject *QuantityPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of QuantityPy and the Twin object
    return new QuantityPy(new Quantity);
}

// constructor method
int QuantityPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    Quantity *self = getQuantityPtr();

    PyErr_Clear(); // set by PyArg_ParseTuple()
    PyObject *object{};
    if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
        *self = *(static_cast<Base::QuantityPy*>(object)->getQuantityPtr());
        return 0;
    }

    PyErr_Clear(); // set by PyArg_ParseTuple()
    double f = DOUBLE_MAX;
    if (PyArg_ParseTuple(args,"dO!",&f,&(Base::UnitPy::Type), &object)) {
        *self = Quantity(f,*(static_cast<Base::UnitPy*>(object)->getUnitPtr()));
        return 0;
    }

    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"dO!",&f,&(Base::QuantityPy::Type), &object)) {
        PyErr_SetString(PyExc_TypeError, "Second argument must be a Unit not a Quantity");
        return -1;
    }

    int i1=0;
    int i2=0;
    int i3=0;
    int i4=0;
    int i5=0;
    int i6=0;
    int i7=0;
    int i8=0;
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args, "|diiiiiiii", &f,&i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
        if (f < DOUBLE_MAX) {
            *self = Quantity(f,Unit{static_cast<int8_t>(i1),
                                    static_cast<int8_t>(i2),
                                    static_cast<int8_t>(i3),
                                    static_cast<int8_t>(i4),
                                    static_cast<int8_t>(i5),
                                    static_cast<int8_t>(i6),
                                    static_cast<int8_t>(i7),
                                    static_cast<int8_t>(i8)});
        }
        return 0;
    }

    PyErr_Clear(); // set by PyArg_ParseTuple()
    char* string{};
    if (PyArg_ParseTuple(args,"et", "utf-8", &string)) {
        QString qstr = QString::fromUtf8(string);
        PyMem_Free(string);
        try {
            *self = Quantity::parse(qstr);
        }
        catch(const Base::ParserError& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return -1;
        }

        return 0;
    }

    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"det", &f, "utf-8", &string)) {
        QString unit = QString::fromUtf8(string);
        PyMem_Free(string);
        try {
            *self = Quantity(f, unit);
        }
        catch(const Base::ParserError& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return -1;
        }

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Either quantity, float with units or string expected");
    return -1;
}

PyObject* QuantityPy::getUserPreferred(PyObject* /*args*/)
{
    QString uus;
    double factor{};
    Py::Tuple res(3);

    QString uss = getQuantityPtr()->getUserString(factor,uus);

    res[0] = Py::String(uss.toUtf8(),"utf-8");
    res[1] = Py::Float(factor);
    res[2] = Py::String(uus.toUtf8(),"utf-8");

    return Py::new_reference_to(res);
}

PyObject* QuantityPy::getValueAs(PyObject *args)
{
    Quantity quant;
    quant.setInvalid();

    // first try Quantity
    if (!quant.isValid()) {
        PyObject *object{};
        if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
            quant = * static_cast<Base::QuantityPy*>(object)->getQuantityPtr();
        }
    }

    if (!quant.isValid()) {
        PyObject *object{};
        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::UnitPy::Type), &object)) {
            quant.setUnit(*static_cast<Base::UnitPy*>(object)->getUnitPtr());
            quant.setValue(1.0);
        }
    }

    if (!quant.isValid()) {
        PyObject *object{};
        double value{};
        PyErr_Clear();
        if (PyArg_ParseTuple(args,"dO!",&value, &(Base::UnitPy::Type), &object)) {
            quant.setUnit(*static_cast<Base::UnitPy*>(object)->getUnitPtr());
            quant.setValue(value);
        }
    }

    if (!quant.isValid()) {
        double f = DOUBLE_MAX;
        int i1=0;
        int i2=0;
        int i3=0;
        int i4=0;
        int i5=0;
        int i6=0;
        int i7=0;
        int i8=0;
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d|iiiiiiii", &f,&i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
            if (f < DOUBLE_MAX) {
                quant = Quantity(f,Unit{static_cast<int8_t>(i1),
                                        static_cast<int8_t>(i2),
                                        static_cast<int8_t>(i3),
                                        static_cast<int8_t>(i4),
                                        static_cast<int8_t>(i5),
                                        static_cast<int8_t>(i6),
                                        static_cast<int8_t>(i7),
                                        static_cast<int8_t>(i8)});
            }
        }
    }

    if (!quant.isValid()) {
        PyErr_Clear();
        char* string{};
        if (PyArg_ParseTuple(args,"et", "utf-8", &string)) {
            QString qstr = QString::fromUtf8(string);
            PyMem_Free(string);
            quant = Quantity::parse(qstr);
        }
    }

    if (!quant.isValid()) {
        PyErr_SetString(PyExc_TypeError, "Either quantity, string, float or unit expected");
        return nullptr;
    }

    if (getQuantityPtr()->getUnit() != quant.getUnit() && quant.isQuantity()) {
        PyErr_SetString(PyExc_ValueError, "Unit mismatch");
        return nullptr;
    }

    quant = Quantity(getQuantityPtr()->getValueAs(quant));
    return new QuantityPy(new Quantity(quant));
}

PyObject * QuantityPy::__round__ (PyObject *args)
{
    double val= getQuantityPtr()->getValue();
    Unit unit = getQuantityPtr()->getUnit();
    Py::Float flt(val);
    Py::Callable func(flt.getAttr("__round__"));
    double rnd = static_cast<double>(Py::Float(func.apply(args)));

    return new QuantityPy(new Quantity(rnd, unit));
}

PyObject * QuantityPy::number_float_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Arg must be Quantity");
        return nullptr;
    }

    QuantityPy* q = static_cast<QuantityPy*>(self);
    return PyFloat_FromDouble(q->getValue());
}

PyObject * QuantityPy::number_int_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Arg must be Quantity");
        return nullptr;
    }

    QuantityPy* q = static_cast<QuantityPy*>(self);
    return PyLong_FromLong(long(q->getValue()));
}

PyObject * QuantityPy::number_negative_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Arg must be Quantity");
        return nullptr;
    }

    Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
    double b = -1;
    return new QuantityPy(new Quantity(*a * b));
}

PyObject * QuantityPy::number_positive_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Arg must be Quantity");
        return nullptr;
    }

    Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
    return new QuantityPy(new Quantity(*a));
}

PyObject * QuantityPy::number_absolute_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Arg must be Quantity");
        return nullptr;
    }

    Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
    return new QuantityPy(new Quantity(fabs(a->getValue()), a->getUnit()));
}

static Quantity &pyToQuantity(Quantity &q, PyObject *pyobj) {
    if (PyObject_TypeCheck(pyobj, &Base::QuantityPy::Type))
        q = *static_cast<Base::QuantityPy*>(pyobj)->getQuantityPtr();
    else if (PyFloat_Check(pyobj))
        q = Quantity(PyFloat_AsDouble(pyobj));
    else if (PyLong_Check(pyobj))
        q = Quantity(PyLong_AsLong(pyobj));
    else {
        PyErr_Format(PyExc_TypeError,"Cannot convert %s to Quantity",Py_TYPE(pyobj)->tp_name);
        throw Py::Exception();
    }
    return q;
}

PyObject* QuantityPy::number_add_handler(PyObject *self, PyObject *other)
{
    Quantity *pa=nullptr, *pb=nullptr;
    Quantity a,b;
    PY_TRY {
        if (PyObject_TypeCheck(self, &(QuantityPy::Type)))
            pa = static_cast<QuantityPy*>(self)->getQuantityPtr();
        else
            pa = &pyToQuantity(a,self);

        if (PyObject_TypeCheck(other, &(QuantityPy::Type)))
            pb = static_cast<QuantityPy*>(other)->getQuantityPtr();
        else
            pb = &pyToQuantity(b,other);
        return new QuantityPy(new Quantity(*pa + *pb) );
    } PY_CATCH
}

PyObject* QuantityPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    Quantity *pa=nullptr, *pb=nullptr;
    Quantity a,b;
    PY_TRY {
        if (PyObject_TypeCheck(self, &(QuantityPy::Type)))
            pa = static_cast<QuantityPy*>(self)->getQuantityPtr();
        else
            pa = &pyToQuantity(a,self);

        if (PyObject_TypeCheck(other, &(QuantityPy::Type)))
            pb = static_cast<QuantityPy*>(other)->getQuantityPtr();
        else
            pb = &pyToQuantity(b,other);
        return new QuantityPy(new Quantity(*pa - *pb) );
    } PY_CATCH
}

PyObject* QuantityPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    Quantity *pa=nullptr, *pb=nullptr;
    Quantity a,b;
    PY_TRY {
        if (PyObject_TypeCheck(self, &(QuantityPy::Type)))
            pa = static_cast<QuantityPy*>(self)->getQuantityPtr();
        else
            pa = &pyToQuantity(a,self);

        if (PyObject_TypeCheck(other, &(QuantityPy::Type)))
            pb = static_cast<QuantityPy*>(other)->getQuantityPtr();
        else
            pb = &pyToQuantity(b,other);
        return new QuantityPy(new Quantity(*pa * *pb) );
    } PY_CATCH
}

PyObject * QuantityPy::number_divide_handler (PyObject *self, PyObject *other)
{
    Quantity *pa=nullptr, *pb=nullptr;
    Quantity a,b;
    PY_TRY {
        if (PyObject_TypeCheck(self, &(QuantityPy::Type)))
            pa = static_cast<QuantityPy*>(self)->getQuantityPtr();
        else
            pa = &pyToQuantity(a,self);

        if (PyObject_TypeCheck(other, &(QuantityPy::Type)))
            pb = static_cast<QuantityPy*>(other)->getQuantityPtr();
        else
            pb = &pyToQuantity(b,other);
        return new QuantityPy(new Quantity(*pa / *pb) );
    } PY_CATCH
}

PyObject * QuantityPy::number_remainder_handler (PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Quantity");
        return nullptr;
    }

    double d1{}, d2{};
    Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
    d1 = a->getValue();

    if (PyObject_TypeCheck(other, &(QuantityPy::Type))) {
        Base::Quantity *b = static_cast<QuantityPy*>(other)->getQuantityPtr();
        d2 = b->getValue();
    }
    else if (PyFloat_Check(other)) {
        d2 = PyFloat_AsDouble(other);
    }
    else if (PyLong_Check(other)) {
        d2 = (double)PyLong_AsLong(other);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected quantity or number");
        return nullptr;
    }

    PyObject* p1 = PyFloat_FromDouble(d1);
    PyObject* p2 = PyFloat_FromDouble(d2);
    PyObject* r = PyNumber_Remainder(p1, p2);
    Py_DECREF(p1);
    Py_DECREF(p2);
    if (!r)
        return nullptr;
    double q = PyFloat_AsDouble(r);
    Py_DECREF(r);
    return new QuantityPy(new Quantity(q,a->getUnit()));
}

PyObject * QuantityPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    //PyNumber_Divmod();
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * QuantityPy::number_power_handler (PyObject *self, PyObject *other, PyObject * /*modulo*/)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Quantity");
        return nullptr;
    }

    PY_TRY {
        if (PyObject_TypeCheck(other, &(QuantityPy::Type))) {
            Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
            Base::Quantity *b = static_cast<QuantityPy*>(other)->getQuantityPtr();
            Base::Quantity q(a->pow(*b)); // to prevent memory leak in case of exception

            return new QuantityPy(new Quantity(q));
        }
        else if (PyFloat_Check(other)) {
            Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
            double b = PyFloat_AsDouble(other);
            return new QuantityPy(new Quantity(a->pow(b)) );
        }
        else if (PyLong_Check(other)) {
            Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
            double b = (double)PyLong_AsLong(other);
            return new QuantityPy(new Quantity(a->pow(b)));
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Expected quantity or number");
            return nullptr;
        }
    }PY_CATCH
}

int QuantityPy::number_nonzero_handler (PyObject *self)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        return 1;
    }

    Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
    return a->getValue() != 0.0;
}

PyObject* QuantityPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(QuantityPy::Type)) &&
        PyObject_TypeCheck(w, &(QuantityPy::Type))) {
        const Quantity * u1 = static_cast<QuantityPy*>(v)->getQuantityPtr();
        const Quantity * u2 = static_cast<QuantityPy*>(w)->getQuantityPtr();

        PyObject *res=nullptr;
        if (op == Py_NE) {
            res = (!(*u1 == *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LT) {
            res = (*u1 < *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LE) {
            res = (*u1 < *u2)||(*u1 == *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GT) {
            res = (!(*u1 < *u2))&&(!(*u1 == *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GE) {
            res = (!(*u1 < *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_EQ) {
            res = (*u1 == *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
    }
    else if (PyNumber_Check(v) && PyNumber_Check(w)) {
        // Try to get floating numbers
        double u1 = PyFloat_AsDouble(v);
        double u2 = PyFloat_AsDouble(w);
        PyObject *res=nullptr;
        if (op == Py_NE) {
            res = (u1 != u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LT) {
            res = (u1 < u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LE) {
            res = (u1 <= u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GT) {
            res = (u1 > u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GE) {
            res = (u1 >= u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_EQ) {
            res = (u1 == u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
    }

    // This always returns False
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
}

Py::Float QuantityPy::getValue() const
{
    return Py::Float(getQuantityPtr()->getValue());
}

void QuantityPy::setValue(Py::Float arg)
{
    getQuantityPtr()->setValue(arg);
}

Py::Object QuantityPy::getUnit() const
{
    return Py::asObject(new UnitPy(new Unit(getQuantityPtr()->getUnit())));
}

void QuantityPy::setUnit(Py::Object arg)
{
    Py::Type UnitType(Base::getTypeAsObject(&Base::UnitPy::Type));
    if(!arg.isType(UnitType))
        throw Py::AttributeError("Not yet implemented");

    getQuantityPtr()->setUnit(*static_cast<Base::UnitPy*>((*arg))->getUnitPtr());
}

Py::String QuantityPy::getUserString() const
{
    return {getQuantityPtr()->getUserString().toUtf8(),"utf-8"};
}

Py::Dict QuantityPy::getFormat() const
{
    QuantityFormat fmt = getQuantityPtr()->getFormat();

    Py::Dict dict;
    dict.setItem("Precision", Py::Int (fmt.precision));
    dict.setItem("NumberFormat", Py::Char(fmt.toFormat()));
    dict.setItem("Denominator", Py::Int(fmt.denominator));
    return dict;
}

void  QuantityPy::setFormat(Py::Dict arg)
{
    QuantityFormat fmt = getQuantityPtr()->getFormat();

    if (arg.hasKey("Precision")) {
        Py::Int  prec(arg.getItem("Precision"));
        fmt.precision = static_cast<int>(prec);
    }

    if (arg.hasKey("NumberFormat")) {
        Py::Object item = arg.getItem("NumberFormat");
        if (item.isNumeric()) {
            int format = static_cast<int>(Py::Int(item));
            if (format < 0 || format > QuantityFormat::Scientific)
                throw Py::ValueError("Invalid format value");
            fmt.format = static_cast<QuantityFormat::NumberFormat>(format);
        }
        else {
            Py::Char form(item);
            std::string fmtstr = static_cast<std::string>(Py::String(form));
            if (fmtstr.size() != 1)
                throw Py::ValueError("Invalid format character");

            bool ok = false;
            fmt.format = Base::QuantityFormat::toFormat(fmtstr[0], &ok);
            if (!ok)
                throw Py::ValueError("Invalid format character");
        }
    }

    if (arg.hasKey("Denominator")) {
        Py::Int  denom(arg.getItem("Denominator"));
        int fracInch = static_cast<int>(denom);
        // check that the value is positive and a power of 2
        if (fracInch <= 0)
            throw Py::ValueError("Denominator must be higher than zero");
        // bitwise check
        if (fracInch & (fracInch - 1))
            throw Py::ValueError("Denominator must be a power of two");
        fmt.denominator = fracInch;
    }

    getQuantityPtr()->setFormat(fmt);
}

PyObject *QuantityPy::getCustomAttributes(const char* attr) const
{
    QuantityPy* py = nullptr;
    if (strcmp(attr, "Torr") == 0) {
        py = new QuantityPy(new Quantity(Quantity::Torr));
    }
    else if (strcmp(attr, "mTorr") == 0) {
        py = new QuantityPy(new Quantity(Quantity::mTorr));
    }
    else if (strcmp(attr, "yTorr") == 0) {
        py = new QuantityPy(new Quantity(Quantity::yTorr));
    }
    else if (strcmp(attr, "PoundForce") == 0) {
        py = new QuantityPy(new Quantity(Quantity::PoundForce));
    }
    else if (strcmp(attr, "AngularMinute") == 0) {
        py = new QuantityPy(new Quantity(Quantity::AngMinute));
    }
    else if (strcmp(attr, "AngularSecond") == 0) {
        py = new QuantityPy(new Quantity(Quantity::AngSecond));
    }

    if (py) {
        py->setNotTracking();
    }

    return py;
}

int QuantityPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject * QuantityPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_TypeError, "bad operand type for unary ~");
    return nullptr;
}

PyObject * QuantityPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_TypeError, "unsupported operand type(s) for <<");
    return nullptr;
}

PyObject * QuantityPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_TypeError, "unsupported operand type(s) for >>");
    return nullptr;
}

PyObject * QuantityPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_TypeError, "unsupported operand type(s) for &");
    return nullptr;
}

PyObject * QuantityPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_TypeError, "unsupported operand type(s) for ^");
    return nullptr;
}

PyObject * QuantityPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_TypeError, "unsupported operand type(s) for |");
    return nullptr;
}

