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
#include <Mod/CAM/App/CommandPy.h>
#include <Mod/Part/App/TopoShapePy.h>

// inclusion of the generated files (generated out of CAMSimPy.xml)
#include "CAMSimPy.h"
#include "CAMSimPy.cpp"


using namespace CAMSimulator;

// returns a string which represents the object e.g. when printed in python
std::string CAMSimPy::representation() const
{
    return std::string("<CAMSim object>");
}

PyObject *CAMSimPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CAMSimPy and the Twin object
    return new CAMSimPy(new CAMSim);
}

// constructor method
int CAMSimPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* CAMSimPy::BeginSimulation(PyObject * args, PyObject * kwds)
{
	static const std::array<const char *, 3> kwlist { "stock", "resolution", nullptr };
	PyObject *pObjStock;
	float resolution;
	if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!f", kwlist, &(Part::TopoShapePy::Type), &pObjStock, &resolution))
		return nullptr;
	CAMSim *sim = getCAMSimPtr();
	Part::TopoShape *stock = static_cast<Part::TopoShapePy*>(pObjStock)->getTopoShapePtr();
	sim->BeginSimulation(stock, resolution);
	Py_IncRef(Py_None);
	return Py_None;
}

PyObject* CAMSimPy::SetToolShape(PyObject * args)
{
	PyObject *pObjToolShape;
	float resolution;
	if (!PyArg_ParseTuple(args, "O!f", &(Part::TopoShapePy::Type), &pObjToolShape, &resolution))
		return nullptr;
	CAMSim *sim = getCAMSimPtr();
	const TopoDS_Shape& toolShape = static_cast<Part::TopoShapePy*>(pObjToolShape)->getTopoShapePtr()->getShape();
	sim->SetToolShape(toolShape, resolution);
	Py_IncRef(Py_None);
	return Py_None;
}

PyObject* CAMSimPy::AddCommand(PyObject* args, PyObject* kwds)
{
    static const std::array<const char *, 3> kwlist { "position", "command", nullptr };
	PyObject *pObjPlace;
	PyObject *pObjCmd;
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!", kwlist, &(Base::PlacementPy::Type), &pObjPlace,
                                             &(Path::CommandPy::Type), &pObjCmd)) {
        return nullptr;
    }
	CAMSim *sim = getCAMSimPtr();
	Base::Placement *pos = static_cast<Base::PlacementPy*>(pObjPlace)->getPlacementPtr();
	Path::Command *cmd = static_cast<Path::CommandPy*>(pObjCmd)->getCommandPtr();
	Base::Placement *newpos = sim->ApplyCommand(pos, cmd);
	//Base::Console().Log("Done...\n");
	//Base::Console().Refresh();
	Base::PlacementPy *newposPy = new Base::PlacementPy(newpos);
	return newposPy;
}

PyObject *CAMSimPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CAMSimPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


