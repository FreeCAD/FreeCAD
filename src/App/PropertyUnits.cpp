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
// PropertyFloatUnit
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyQuantity, App::PropertyFloat);

Base::Quantity PropertyQuantity::getQuantityValue(void) const
{
	return Quantity( _dValue,_Unit);
}

void PropertyQuantity::setValue(const Base::Quantity &quant)
{
    aboutToSetValue();
    _dValue = quant.getValue();
    _Unit   = quant.getUnit();
    hasSetValue();
}

const char* PropertyQuantity::getEditorName(void) const
{ 

    return "Gui::PropertyEditor::PropertyUnitItem";

}

PyObject *PropertyQuantity::getPyObject(void)
{
    return new QuantityPy (new Quantity( _dValue,_Unit));
}

void PropertyQuantity::setPyObject(PyObject *value)
{
	Base::Quantity quant;

    if (PyString_Check(value)) 
        quant = Quantity::parse(QString::fromLatin1(PyString_AsString(value)));
    else if (PyFloat_Check(value))
        quant = Quantity(PyFloat_AsDouble(value),_Unit);
    else if (PyInt_Check(value))
        quant = Quantity((double)PyInt_AsLong(value),_Unit);
    else if (PyObject_TypeCheck(value, &(QuantityPy::Type))) {
        Base::QuantityPy  *pcObject = static_cast<Base::QuantityPy*>(value);
		quant = *(pcObject->getQuantityPtr());
	}else
        throw Base::Exception("Wrong type!");

	Unit unit = quant.getUnit();
	if(unit.isEmpty()){
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

TYPESYSTEM_SOURCE(App::PropertyDistance, App::PropertyFloat);

//**************************************************************************
//**************************************************************************
// PropertySpeed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertySpeed, App::PropertyFloat);

//**************************************************************************
//**************************************************************************
// PropertyAcceleration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAcceleration, App::PropertyFloat);

//**************************************************************************
//**************************************************************************
// PropertyLength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLength, App::PropertyFloat);

const char* PropertyLength::getEditorName(void) const
{ 
#ifdef UseUnitsInGui
    return "Gui::PropertyEditor::PropertyUnitItem";
#else
    return "Gui::PropertyEditor::PropertyFloatItem"; 
#endif
}


void PropertyLength::setPyObject(PyObject *value)
{
#ifdef UseUnitsInGui
    setValue(UnitsApi::toDblWithUserPrefs(Length,value));
 
#else   
   double val=0.0f;
    if (PyFloat_Check(value)) {
        val = PyFloat_AsDouble(value);
    }
    else if(PyInt_Check(value)) {
        val = (double) PyInt_AsLong(value);
    }
    else {
        std::string error = std::string("type must be float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    if (val < 0.0f)
        throw Base::ValueError("value must be nonnegative");

    setValue(val);
#endif
}

//**************************************************************************
//**************************************************************************
// PropertyAngle
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAngle, App::PropertyFloatConstraint);




