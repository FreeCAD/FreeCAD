/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
# include <boost/version.hpp>
# include <boost/filesystem/path.hpp>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/UnitsApi.h>

#include "PropertyUnits.h"
#include <Base/PyObjectBase.h>
#include <Base/QuantityPy.h>

#define new DEBUG_CLIENTBLOCK
using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyQuantity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyQuantity, App::PropertyFloat);

Base::Quantity PropertyQuantity::getQuantityValue(void) const
{
    return Quantity(_dValue,_Unit);
}

const char* PropertyQuantity::getEditorName(void) const
{
    return "Gui::PropertyEditor::PropertyUnitItem";
}

PyObject *PropertyQuantity::getPyObject(void)
{
    return new QuantityPy (new Quantity(_dValue,_Unit));
}

Base::Quantity PropertyQuantity::createQuantityFromPy(PyObject *value)
{
    Base::Quantity quant;

    if (PyString_Check(value))
        quant = Quantity::parse(QString::fromLatin1(PyString_AsString(value)));
    else if (PyUnicode_Check(value)){
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        std::string Str;
        Str = PyString_AsString(unicode);
        quant = Quantity::parse(QString::fromUtf8(Str.c_str()));
        Py_DECREF(unicode);
    }else if (PyFloat_Check(value))
        quant = Quantity(PyFloat_AsDouble(value),_Unit);
    else if (PyInt_Check(value))
        quant = Quantity((double)PyInt_AsLong(value),_Unit);
    else if (PyObject_TypeCheck(value, &(QuantityPy::Type))) {
        Base::QuantityPy  *pcObject = static_cast<Base::QuantityPy*>(value);
        quant = *(pcObject->getQuantityPtr());
    }
    else
        throw Base::Exception("Wrong type!");

    return quant;
}


void PropertyQuantity::setPyObject(PyObject *value)
{
    Base::Quantity quant= createQuantityFromPy(value);

    Unit unit = quant.getUnit();
    if (unit.isEmpty()){
        PropertyFloat::setValue(quant.getValue());
        return;
    }

    if (unit != _Unit)
        throw Base::Exception("Not matching Unit!");

    PropertyFloat::setValue(quant.getValue());
}

//**************************************************************************
//**************************************************************************
// PropertyQuantityConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyQuantityConstraint, App::PropertyQuantity);



void PropertyQuantityConstraint::setConstraints(const Constraints* sConstrain)
{
    _ConstStruct = sConstrain;
}

const PropertyQuantityConstraint::Constraints*  PropertyQuantityConstraint::getConstraints(void) const
{
    return _ConstStruct;
}

void PropertyQuantityConstraint::setPyObject(PyObject *value)
{
    Base::Quantity quant= createQuantityFromPy(value);

    Unit unit = quant.getUnit();
    double temp = quant.getValue();
    if (_ConstStruct) {
        if (temp > _ConstStruct->UpperBound)
            temp = _ConstStruct->UpperBound;
        else if (temp < _ConstStruct->LowerBound)
            temp = _ConstStruct->LowerBound;
    }
    quant.setValue(temp);

    if (unit.isEmpty()){
        PropertyFloat::setValue(quant.getValue());
        return;
    }

    if (unit != _Unit)
        throw Base::Exception("Not matching Unit!");

    PropertyFloat::setValue(quant.getValue());
}

//**************************************************************************
//**************************************************************************
// PropertyDistance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDistance, App::PropertyQuantity);

PropertyDistance::PropertyDistance()
{
    setUnit(Base::Unit::Length);
}

//**************************************************************************
//**************************************************************************
// PropertySpeed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertySpeed, App::PropertyQuantity);

PropertySpeed::PropertySpeed()
{
    setUnit(Base::Unit::Velocity);
}

//**************************************************************************
//**************************************************************************
// PropertyAcceleration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAcceleration, App::PropertyQuantity);

PropertyAcceleration::PropertyAcceleration()
{
    setUnit(Base::Unit::Acceleration);
}

//**************************************************************************
//**************************************************************************
// PropertyLength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLength, App::PropertyQuantity);

PropertyLength::PropertyLength()
{
    setUnit(Base::Unit::Length);
}

//**************************************************************************
//**************************************************************************
// PropertyAngle
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAngle, App::PropertyQuantityConstraint);

PropertyAngle::PropertyAngle()
{
    setUnit(Base::Unit::Angle);
}

//**************************************************************************
//**************************************************************************
// PropertyPressure
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPressure, App::PropertyQuantity);

PropertyPressure::PropertyPressure()
{
    setUnit(Base::Unit::Pressure);
}

//**************************************************************************
//**************************************************************************
// PropertyForce
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyForce, App::PropertyQuantity);

PropertyForce::PropertyForce()
{
    setUnit(Base::Unit::Force);
}

