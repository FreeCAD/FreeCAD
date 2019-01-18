/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel (juergen.riegel@web.de)              *
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

#include "Mod/Robot/App/Robot6Axis.h"
#include <Base/PlacementPy.h>
#include <Base/MatrixPy.h>
#include <Base/Exception.h>
#include <sstream>

// inclusion of the generated files (generated out of Robot6AxisPy.xml)
#include "Robot6AxisPy.h"
#include "Robot6AxisPy.cpp"

using namespace Robot;

// returns a string which represents the object e.g. when printed in python
std::string Robot6AxisPy::representation(void) const
{
	std::stringstream str;

    str.precision(5);
	str << "<Robot6Axis "
		<< "Tcp:(" 
		  << getRobot6AxisPtr()->getTcp().getPosition().x << ","
		  << getRobot6AxisPtr()->getTcp().getPosition().y << ","
		  << getRobot6AxisPtr()->getTcp().getPosition().z << ") "
		<< "Axis:(" 
		  << "1:" << getRobot6AxisPtr()->getAxis(0) << " "
		  << "2:" << getRobot6AxisPtr()->getAxis(1) << " "
		  << "3:" << getRobot6AxisPtr()->getAxis(2) << " "
		  << "4:" << getRobot6AxisPtr()->getAxis(3) << " "
		  << "5:" << getRobot6AxisPtr()->getAxis(4) << " "
		  << "6:" << getRobot6AxisPtr()->getAxis(5) << ")";

	return str.str();
}

PyObject *Robot6AxisPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Robot6AxisPy and the Twin object 
    return new Robot6AxisPy(new Robot6Axis);
}

// constructor method
int Robot6AxisPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* Robot6AxisPy::check(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}



Py::Float Robot6AxisPy::getAxis1(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(0));
}

void Robot6AxisPy::setAxis1(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(0,(float)arg.operator double());
}

Py::Float Robot6AxisPy::getAxis2(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(1));
}

void Robot6AxisPy::setAxis2(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(1,(float)arg.operator double());
}

Py::Float Robot6AxisPy::getAxis3(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(2));
}

void Robot6AxisPy::setAxis3(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(2,(float)arg.operator double());
}

Py::Float Robot6AxisPy::getAxis4(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(3));
}

void Robot6AxisPy::setAxis4(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(3,(float)arg.operator double());
}

Py::Float Robot6AxisPy::getAxis5(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(4));
}

void Robot6AxisPy::setAxis5(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(4,(float)arg.operator double());
}

Py::Float Robot6AxisPy::getAxis6(void) const
{
    return Py::Float(getRobot6AxisPtr()->getAxis(5));
}

void Robot6AxisPy::setAxis6(Py::Float arg)
{
	getRobot6AxisPtr()->setAxis(5,(float)arg.operator double());
}

Py::Object Robot6AxisPy::getTcp(void) const
{
	return Py::Object(new Base::PlacementPy(new Base::Placement(getRobot6AxisPtr()->getTcp())));
}

void Robot6AxisPy::setTcp(Py::Object value)
{
   if (PyObject_TypeCheck(*value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy  *pcObject = (Base::MatrixPy*)*value;
        Base::Matrix4D mat = pcObject->value();
        Base::Placement p;
        p.fromMatrix(mat);
		getRobot6AxisPtr()->setTo(p);
    }
    else if (PyObject_TypeCheck(*value, &(Base::PlacementPy::Type))) {
        if(! getRobot6AxisPtr()->setTo(*static_cast<Base::PlacementPy*>(*value)->getPlacementPtr()))
			throw Base::RuntimeError("Can not reach Point");
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Placement', not ");
        error += (*value)->ob_type->tp_name;
        throw Py::TypeError(error);
    }

}

Py::Object Robot6AxisPy::getBase(void) const
{
    return Py::Object();
}

void Robot6AxisPy::setBase(Py::Object /*arg*/)
{

}

PyObject *Robot6AxisPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Robot6AxisPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


