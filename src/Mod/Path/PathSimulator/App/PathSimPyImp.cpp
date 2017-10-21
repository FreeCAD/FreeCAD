/***************************************************************************
*   Copyright (c) 2017 Shai Seger         <shaise at gmail>               *
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

#include "Mod/Path/PathSimulator/App/PathSim.h"
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Mesh/App/MeshPy.h>

// inclusion of the generated files (generated out of PathSimPy.xml)
#include "PathSimPy.h"
#include "PathSimPy.cpp"

using namespace PathSimulator;

// returns a string which represents the object e.g. when printed in python
std::string PathSimPy::representation(void) const
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
	static char *kwlist[] = { "stock", "resolution", NULL };
	PyObject *pObjStock;
	float resolution;
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!f", kwlist, &(Part::TopoShapePy::Type), &pObjStock, &resolution))
		return 0;
	PathSim *sim = getPathSimPtr();
	Part::TopoShape *stock = static_cast<Part::TopoShapePy*>(pObjStock)->getTopoShapePtr();
	sim->BeginSimulation(stock, resolution);
	Py_IncRef(Py_None);
	return Py_None;
}

PyObject* PathSimPy::SetCurrentTool(PyObject * /*args*/)
{
	PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
	return 0;
}

PyObject* PathSimPy::GetResultMesh(PyObject * args)
{
	if (!PyArg_ParseTuple(args, ""))
		return 0;
	cStock *stock = getPathSimPtr()->m_stock;

	Mesh::MeshObject *mesh = new Mesh::MeshObject();
	Mesh::MeshPy *meshpy = new Mesh::MeshPy(mesh);
	stock->Tesselate(*mesh);
	return meshpy;
}

/* test script 
import PathSimulator
sim = PathSimulator.PathSim()
stock = Part.makeBox(20,20,5)
sim.BeginSimulation(stock,0.1)
msh = sim.GetResultMesh()
Mesh.show(msh)

*/



Py::Object PathSimPy::getTool(void) const
{
    //return Py::Object();
    throw Py::AttributeError("Not yet implemented");
}

PyObject *PathSimPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PathSimPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


