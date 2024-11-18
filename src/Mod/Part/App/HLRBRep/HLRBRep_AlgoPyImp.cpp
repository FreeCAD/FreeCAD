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

# include <boost/math/special_functions/fpclassify.hpp>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "HLRBRep/HLRBRep_AlgoPy.h"
#include "HLRBRep/HLRBRep_AlgoPy.cpp"
#include "TopoShapePy.h"
#include "Tools.h"


using namespace Part;

PyObject *HLRBRep_AlgoPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of HLRBRep_AlgoPy
    return new HLRBRep_AlgoPy(nullptr);
}

// constructor method
int HLRBRep_AlgoPy::PyInit(PyObject* /*args*/, PyObject* /*kwds*/)
{
    // The lifetime of the HLRBRep_Algo is controlled by the hAlgo handle
    HLRBRep_Algo* algo = new HLRBRep_Algo();
    hAlgo = algo;
    setTwinPointer(algo);
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string HLRBRep_AlgoPy::representation() const
{
    return {"<HLRBRep_Algo object>"};
}

PyObject* HLRBRep_AlgoPy::add(PyObject *args)
{
    PyObject* shape;
    int nbIso = 0;
    if (!PyArg_ParseTuple(args, "O!|i", &Part::TopoShapePy::Type, &shape, &nbIso))
        return nullptr;

    TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    getHLRBRep_AlgoPtr()->Add(input, nbIso);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::remove(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    getHLRBRep_AlgoPtr()->Remove(index);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::index(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    int value = getHLRBRep_AlgoPtr()->Index(input);
    return Py_BuildValue("i", value);
}

PyObject* HLRBRep_AlgoPy::outLinedShapeNullify(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_AlgoPtr()->OutLinedShapeNullify();
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::setProjector(PyObject *args, PyObject *kwds)
{
    PyObject* ps = nullptr;
    PyObject* zd = nullptr;
    PyObject* xd = nullptr;
    double focus = std::numeric_limits<double>::quiet_NaN();

    static const std::array<const char *, 5> kwlist {"Origin", "ZDir", "XDir", "focus", nullptr};
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
            getHLRBRep_AlgoPtr()->Projector(HLRAlgo_Projector(ax2));
        else
            getHLRBRep_AlgoPtr()->Projector(HLRAlgo_Projector(ax2, focus));
        Py_Return;
    }

    return nullptr;
}

PyObject* HLRBRep_AlgoPy::nbShapes(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    int num = getHLRBRep_AlgoPtr()->NbShapes();
    return Py_BuildValue("i", num);
}

PyObject* HLRBRep_AlgoPy::showAll(PyObject *args)
{
    int i=-1;
    if (!PyArg_ParseTuple(args, "|i", &i))
        return nullptr;

    if (i < 1)
        getHLRBRep_AlgoPtr()->ShowAll();
    else
        getHLRBRep_AlgoPtr()->ShowAll(i);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::hide(PyObject *args)
{
    int i=-1, j=-1;
    if (!PyArg_ParseTuple(args, "|ii", &i, &j))
        return nullptr;

    if (i < 1)
        getHLRBRep_AlgoPtr()->Hide();
    else if (j < 1)
        getHLRBRep_AlgoPtr()->Hide(i);
    else
        getHLRBRep_AlgoPtr()->Hide(i, j);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::hideAll(PyObject *args)
{
    int i=-1;
    if (!PyArg_ParseTuple(args, "|i", &i))
        return nullptr;

    if (i < 1)
        getHLRBRep_AlgoPtr()->HideAll();
    else
        getHLRBRep_AlgoPtr()->HideAll(i);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::partialHide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_AlgoPtr()->PartialHide();
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::select(PyObject *args)
{
    int i=-1;
    if (!PyArg_ParseTuple(args, "|i", &i))
        return nullptr;

    if (i < 1)
        getHLRBRep_AlgoPtr()->Select();
    else
        getHLRBRep_AlgoPtr()->Select(i);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::selectEdge(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    getHLRBRep_AlgoPtr()->SelectEdge(index);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::selectFace(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    getHLRBRep_AlgoPtr()->SelectFace(index);
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::initEdgeStatus(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_AlgoPtr()->InitEdgeStatus();
    Py_Return;
}

PyObject* HLRBRep_AlgoPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_AlgoPtr()->Update();
    Py_Return;
}

PyObject *HLRBRep_AlgoPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int HLRBRep_AlgoPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
