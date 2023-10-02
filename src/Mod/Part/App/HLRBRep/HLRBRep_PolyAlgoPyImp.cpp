/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <limits>

# include <gp_Ax2.hxx>
# include <gp_Pnt.hxx>
# include <HLRAlgo_Projector.hxx>
# include <Standard_Version.hxx>

# include <boost/math/special_functions/fpclassify.hpp>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "HLRBRep/HLRBRep_PolyAlgoPy.h"
#include "HLRBRep/HLRBRep_PolyAlgoPy.cpp"
#include "TopoShapePy.h"
#include "Tools.h"


using namespace Part;

PyObject *HLRBRep_PolyAlgoPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of HLRBRep_AlgoPy
    return new HLRBRep_PolyAlgoPy(nullptr);
}

// constructor method
int HLRBRep_PolyAlgoPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return -1;

    // The lifetime of the HLRBRep_PolyAlgo is controlled by the hAlgo handle
    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        HLRBRep_PolyAlgo* algo = new HLRBRep_PolyAlgo(input);
        hAlgo = algo;
        setTwinPointer(algo);
    }
    else {
        HLRBRep_PolyAlgo* algo = new HLRBRep_PolyAlgo();
        hAlgo = algo;
        setTwinPointer(algo);
    }

    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string HLRBRep_PolyAlgoPy::representation() const
{
    return {"<HLRBRep_PolyAlgo object>"};
}

PyObject* HLRBRep_PolyAlgoPy::setProjector(PyObject *args, PyObject *kwds)
{
    PyObject* ps = nullptr;
    PyObject* zd = nullptr;
    PyObject* xd = nullptr;
    double focus = std::numeric_limits<double>::quiet_NaN();

    static const std::array<const char *, 4> kwlist {"Origin", "ZDir", "XDir", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|O!O!O!d", kwlist,
                                            &Base::VectorPy::Type, &ps,
                                            &Base::VectorPy::Type, &zd,
                                            &Base::VectorPy::Type, &xd,
                                            &focus)) {
        gp_Ax2 ax2;
        if (ps && zd && xd) {
            Base::Vector3d p = Py::Vector(ps,false).toVector();
            Base::Vector3d z = Py::Vector(zd,false).toVector();
            Base::Vector3d x = Py::Vector(xd,false).toVector();
            ax2.SetLocation(Base::convertTo<gp_Pnt>(p));
            ax2.SetDirection(Base::convertTo<gp_Dir>(z));
            ax2.SetXDirection(Base::convertTo<gp_Dir>(x));
        }
        else if (ps && zd) {
            Base::Vector3d p = Py::Vector(ps,false).toVector();
            Base::Vector3d z = Py::Vector(zd,false).toVector();
            ax2.SetLocation(Base::convertTo<gp_Pnt>(p));
            ax2.SetDirection(Base::convertTo<gp_Dir>(z));
        }

        if (boost::math::isnan(focus))
            getHLRBRep_PolyAlgoPtr()->Projector(HLRAlgo_Projector(ax2));
        else
            getHLRBRep_PolyAlgoPtr()->Projector(HLRAlgo_Projector(ax2, focus));
        Py_Return;
    }

    return nullptr;
}

PyObject* HLRBRep_PolyAlgoPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->Update();
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::load(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    getHLRBRep_PolyAlgoPtr()->Load(input);
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::remove(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->Remove(index);
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::nbShapes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    int num = getHLRBRep_PolyAlgoPtr()->NbShapes();
    return Py_BuildValue("i", num);
}

PyObject* HLRBRep_PolyAlgoPy::shape(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    TopoDS_Shape result = getHLRBRep_PolyAlgoPtr()->Shape(index);
    return new TopoShapePy(new TopoShape(result));
}

PyObject* HLRBRep_PolyAlgoPy::index(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    int value = getHLRBRep_PolyAlgoPtr()->Index(input);
    return Py_BuildValue("i", value);
}

PyObject* HLRBRep_PolyAlgoPy::initHide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->InitHide();
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::moreHide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean more = getHLRBRep_PolyAlgoPtr()->MoreHide();
    return Py_BuildValue("O", (more ? Py_True : Py_False));
}

PyObject* HLRBRep_PolyAlgoPy::nextHide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->NextHide();
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::initShow(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->InitShow();
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::moreShow(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean more = getHLRBRep_PolyAlgoPtr()->MoreShow();
    return Py_BuildValue("O", (more ? Py_True : Py_False));
}

PyObject* HLRBRep_PolyAlgoPy::nextShow(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyAlgoPtr()->NextShow();
    Py_Return;
}

PyObject* HLRBRep_PolyAlgoPy::outLinedShape(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    TopoDS_Shape result = getHLRBRep_PolyAlgoPtr()->OutLinedShape(input);
    return new TopoShapePy(new TopoShape(result));
}

Py::Float HLRBRep_PolyAlgoPy::getAngle() const
{
#if OCC_VERSION_HEX <= 0x070400
    return Py::Float(getHLRBRep_PolyAlgoPtr()->Angle());
#else
    throw Py::RuntimeError("Function has been removed with OCC 7.5");
#endif
}

void  HLRBRep_PolyAlgoPy::setAngle(Py::Float arg)
{
#if OCC_VERSION_HEX <= 0x070400
    getHLRBRep_PolyAlgoPtr()->Angle(static_cast<double>(arg));
#else
    (void)arg;
#endif
}

Py::Float HLRBRep_PolyAlgoPy::getTolAngular() const
{
    return Py::Float(getHLRBRep_PolyAlgoPtr()->TolAngular());
}

void  HLRBRep_PolyAlgoPy::setTolAngular(Py::Float arg)
{
    getHLRBRep_PolyAlgoPtr()->TolAngular(static_cast<double>(arg));
}

Py::Float HLRBRep_PolyAlgoPy::getTolCoef() const
{
    return Py::Float(getHLRBRep_PolyAlgoPtr()->TolCoef());
}

void  HLRBRep_PolyAlgoPy::setTolCoef(Py::Float arg)
{
    getHLRBRep_PolyAlgoPtr()->TolCoef(static_cast<double>(arg));
}

PyObject *HLRBRep_PolyAlgoPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int HLRBRep_PolyAlgoPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
