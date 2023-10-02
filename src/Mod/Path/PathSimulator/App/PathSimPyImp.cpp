/**************************************************************************
*   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
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

#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Path/App/CommandPy.h>
#include <Mod/Part/App/TopoShapePy.h>

#include "PathSim.h"
// inclusion of the generated files (generated out of PathSimPy.xml)
#include "PathSimPy.h"
#include "PathSimPy.cpp"


using namespace PathSimulator;

// returns a string which represents the object e.g. when printed in python
std::string PathSimPy::representation() const
{
    return std::string("<PathSim object>");
}

PyObject *PathSimPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PathSimPy and the Twin object
    return new PathSimPy(new PathSim);
}

// constructor method
int PathSimPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* PathSimPy::BeginSimulation(PyObject * args, PyObject * kwds)
{
	static const std::array<const char *, 3> kwlist { "stock", "resolution", nullptr };
	PyObject *pObjStock;
	float resolution;
	if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!f", kwlist, &(Part::TopoShapePy::Type), &pObjStock, &resolution))
		return nullptr;
	PathSim *sim = getPathSimPtr();
	Part::TopoShape *stock = static_cast<Part::TopoShapePy*>(pObjStock)->getTopoShapePtr();
	sim->BeginSimulation(stock, resolution);
	Py_IncRef(Py_None);
	return Py_None;
}

PyObject* PathSimPy::SetToolShape(PyObject * args)
{
	PyObject *pObjToolShape;
	float resolution;
	if (!PyArg_ParseTuple(args, "O!f", &(Part::TopoShapePy::Type), &pObjToolShape, &resolution))
		return nullptr;
	PathSim *sim = getPathSimPtr();
	const TopoDS_Shape& toolShape = static_cast<Part::TopoShapePy*>(pObjToolShape)->getTopoShapePtr()->getShape();
	sim->SetToolShape(toolShape, resolution);
	Py_IncRef(Py_None);
	return Py_None;
}

PyObject* PathSimPy::GetResultMesh(PyObject * args)
{
	if (!PyArg_ParseTuple(args, ""))
		return nullptr;
	cStock *stock = getPathSimPtr()->m_stock.get();
	if (!stock)
	{
		PyErr_SetString(PyExc_RuntimeError, "Simulation has stock object");
		return nullptr;
	}

	Mesh::MeshObject *meshOuter = new Mesh::MeshObject();
	Mesh::MeshPy *meshOuterpy = new Mesh::MeshPy(meshOuter);
	Mesh::MeshObject *meshInner = new Mesh::MeshObject();
	Mesh::MeshPy *meshInnerpy = new Mesh::MeshPy(meshInner);
	stock->Tessellate(*meshOuter, *meshInner);
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SetItem(tuple, 0, meshOuterpy);
	PyTuple_SetItem(tuple, 1, meshInnerpy);
	return tuple;
}


PyObject* PathSimPy::ApplyCommand(PyObject * args, PyObject * kwds)
{
    static const std::array<const char *, 3> kwlist { "position", "command", nullptr };
	PyObject *pObjPlace;
	PyObject *pObjCmd;
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!", kwlist, &(Base::PlacementPy::Type), &pObjPlace,
                                             &(Path::CommandPy::Type), &pObjCmd)) {
        return nullptr;
    }
	PathSim *sim = getPathSimPtr();
	Base::Placement *pos = static_cast<Base::PlacementPy*>(pObjPlace)->getPlacementPtr();
	Path::Command *cmd = static_cast<Path::CommandPy*>(pObjCmd)->getCommandPtr();
	Base::Placement *newpos = sim->ApplyCommand(pos, cmd);
	//Base::Console().Log("Done...\n");
	//Base::Console().Refresh();
	Base::PlacementPy *newposPy = new Base::PlacementPy(newpos);
	return newposPy;
}

Py::Object PathSimPy::getTool() const
{
    //return Py::Object();
    throw Py::AttributeError("Not yet implemented");
}

PyObject *PathSimPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PathSimPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


