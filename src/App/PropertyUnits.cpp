/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <cfloat>
#endif

#include <Base/QuantityPy.h>
#include <Base/UnitPy.h>
#include "Base/Units.h"

#include "PropertyUnits.h"
#include "Expression.h"


using namespace App;
using namespace Base;
using namespace std;


const PropertyQuantityConstraint::Constraints LengthStandard = {0.0, DBL_MAX, 1.0};
const PropertyQuantityConstraint::Constraints AngleStandard = {-360, 360, 1.0};

//**************************************************************************
// PropertyQuantity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyQuantity, App::PropertyFloat)

Quantity PropertyQuantity::getQuantityValue() const
{
    Quantity quantity(_dValue, _Unit);
    quantity.setFormat(_Format);
    return quantity;
}

const char* PropertyQuantity::getEditorName() const
{
    return "Gui::PropertyEditor::PropertyUnitItem";
}

PyObject* PropertyQuantity::getPyObject()
{
    return new QuantityPy(new Quantity(_dValue, _Unit));
}

Quantity PropertyQuantity::createQuantityFromPy(PyObject* value)
{
    Quantity quant;

    if (PyUnicode_Check(value)) {
        quant = Quantity::parse(PyUnicode_AsUTF8(value));
    }
    else if (PyFloat_Check(value)) {
        quant = Quantity(PyFloat_AsDouble(value), _Unit);
    }
    else if (PyLong_Check(value)) {
        quant = Quantity(double(PyLong_AsLong(value)), _Unit);
    }
    else if (PyObject_TypeCheck(value, &(QuantityPy::Type))) {
        QuantityPy* pcObject = static_cast<QuantityPy*>(value);
        quant = *(pcObject->getQuantityPtr());
    }
    else {
        std::string error = std::string("wrong type as quantity: ");
        error += value->ob_type->tp_name;
        throw TypeError(error);
    }

    return quant;
}

void PropertyQuantity::setPyObject(PyObject* value)
{
    // Set the unit if Unit object supplied, else check the unit
    // and set the value

    if (PyObject_TypeCheck(value, &(UnitPy::Type))) {
        const auto* pcObject = static_cast<UnitPy*>(value);
        const Unit unit = *pcObject->getUnitPtr();
        aboutToSetValue();
        _Unit = unit;
        hasSetValue();
        return;
    }
    const Quantity quant = createQuantityFromPy(value);

    const Unit unit = quant.getUnit();
    if (unit == Units::NullUnit) {
        PropertyFloat::setValue(quant.getValue());
        return;
    }

    if (unit != _Unit) {
        throw UnitsMismatchError("Not matching Unit!");
    }

    PropertyFloat::setValue(quant.getValue());
}

void PropertyQuantity::setPathValue(const ObjectIdentifier& /*path*/, const boost::any& value)
{
    const auto q = anyToQuantity(value);
    aboutToSetValue();
    if (q.getUnit() != Units::NullUnit) {
        _Unit = q.getUnit();
    }
    _dValue = q.getValue();
    setValue(q.getValue());
}

const boost::any PropertyQuantity::getPathValue(const ObjectIdentifier& /*path*/) const
{
    Quantity quantity(_dValue, _Unit);
    quantity.setFormat(_Format);
    return quantity;
}

//**************************************************************************
// PropertyQuantityConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyQuantityConstraint, App::PropertyQuantity)


void PropertyQuantityConstraint::setConstraints(const Constraints* sConstrain)
{
    _ConstStruct = sConstrain;
}

const char* PropertyQuantityConstraint::getEditorName() const
{
    return "Gui::PropertyEditor::PropertyUnitConstraintItem";
}

const PropertyQuantityConstraint::Constraints* PropertyQuantityConstraint::getConstraints() const
{
    return _ConstStruct;
}

double PropertyQuantityConstraint::getMinimum() const
{
    if (_ConstStruct) {
        return _ConstStruct->LowerBound;
    }
    return std::numeric_limits<double>::min();
}

double PropertyQuantityConstraint::getMaximum() const
{
    if (_ConstStruct) {
        return _ConstStruct->UpperBound;
    }
    return std::numeric_limits<double>::max();
}

double PropertyQuantityConstraint::getStepSize() const
{
    if (_ConstStruct) {
        return _ConstStruct->StepSize;
    }
    return 1.0;
}

void PropertyQuantityConstraint::setPyObject(PyObject* value)
{
    Quantity quant = createQuantityFromPy(value);

    const Unit unit = quant.getUnit();
    double temp = quant.getValue();
    if (_ConstStruct) {
        if (temp > _ConstStruct->UpperBound) {
            temp = _ConstStruct->UpperBound;
        }
        else if (temp < _ConstStruct->LowerBound) {
            temp = _ConstStruct->LowerBound;
        }
    }
    quant.setValue(temp);

    if (unit == Units::NullUnit) {
        PropertyFloat::setValue(quant.getValue());  // clazy:exclude=skipped-base-method
        return;
    }

    if (unit != _Unit) {
        throw UnitsMismatchError("Not matching Unit!");
    }

    PropertyFloat::setValue(quant.getValue());  // clazy:exclude=skipped-base-method
}

// ------------------------------------------------------
// now all properties
// ------------------------------------------------------

//**************************************************************************
// PropertyAcceleration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAcceleration, App::PropertyQuantity)

PropertyAcceleration::PropertyAcceleration()
{
    setUnit(Units::Acceleration);
}

//**************************************************************************
// PropertyAmountOfSubstance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAmountOfSubstance, App::PropertyQuantity)

PropertyAmountOfSubstance::PropertyAmountOfSubstance()
{
    setUnit(Units::AmountOfSubstance);
}

//**************************************************************************
// PropertyAngle
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyAngle, App::PropertyQuantityConstraint)

PropertyAngle::PropertyAngle()
{
    setUnit(Units::Angle);
    setConstraints(&AngleStandard);
}

//**************************************************************************
// PropertyArea
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyArea, App::PropertyQuantityConstraint)

PropertyArea::PropertyArea()
{
    setUnit(Units::Area);
    setConstraints(&LengthStandard);
}

//**************************************************************************
// PropertyCompressiveStrength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyCompressiveStrength, App::PropertyQuantity)

PropertyCompressiveStrength::PropertyCompressiveStrength()
{
    setUnit(Units::CompressiveStrength);
}

//**************************************************************************
// PropertyCurrentDensity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyCurrentDensity, App::PropertyQuantity)

PropertyCurrentDensity::PropertyCurrentDensity()
{
    setUnit(Units::CurrentDensity);
}

//**************************************************************************
// PropertyDensity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDensity, App::PropertyQuantity)

PropertyDensity::PropertyDensity()
{
    setUnit(Units::Density);
}

//**************************************************************************
// PropertyDissipationRate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDissipationRate, App::PropertyQuantity)

PropertyDissipationRate::PropertyDissipationRate()
{
    setUnit(Units::DissipationRate);
}

//**************************************************************************
// PropertyDistance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDistance, App::PropertyQuantity)

PropertyDistance::PropertyDistance()
{
    setUnit(Units::Length);
}

//**************************************************************************
// PropertyDynamicViscosity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDynamicViscosity, App::PropertyQuantity)

PropertyDynamicViscosity::PropertyDynamicViscosity()
{
    setUnit(Units::DynamicViscosity);
}

//**************************************************************************
// PropertyElectricalCapacitance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricalCapacitance, App::PropertyQuantity)

PropertyElectricalCapacitance::PropertyElectricalCapacitance()
{
    setUnit(Units::ElectricalCapacitance);
}

//**************************************************************************
// PropertyElectricalInductance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricalInductance, App::PropertyQuantity)

PropertyElectricalInductance::PropertyElectricalInductance()
{
    setUnit(Units::ElectricalInductance);
}

//**************************************************************************
// PropertyElectricalConductance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricalConductance, App::PropertyQuantity)

PropertyElectricalConductance::PropertyElectricalConductance()
{
    setUnit(Units::ElectricalConductance);
}

//**************************************************************************
// PropertyElectricalConductivity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricalConductivity, App::PropertyQuantity)

PropertyElectricalConductivity::PropertyElectricalConductivity()
{
    setUnit(Units::ElectricalConductivity);
}

//**************************************************************************
// PropertyElectricalResistance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricalResistance, App::PropertyQuantity)

PropertyElectricalResistance::PropertyElectricalResistance()
{
    setUnit(Units::ElectricalResistance);
}

//**************************************************************************
// PropertyElectricCharge
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricCharge, App::PropertyQuantity)

PropertyElectricCharge::PropertyElectricCharge()
{
    setUnit(Units::ElectricCharge);
}

//**************************************************************************
// PropertyElectricCurrent
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricCurrent, App::PropertyQuantity)

PropertyElectricCurrent::PropertyElectricCurrent()
{
    setUnit(Units::ElectricCurrent);
}

//**************************************************************************
// PropertyElectricPotential
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectricPotential, App::PropertyQuantity)

PropertyElectricPotential::PropertyElectricPotential()
{
    setUnit(Units::ElectricPotential);
}

//**************************************************************************
// PropertyFrequency
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFrequency, App::PropertyQuantity)

PropertyFrequency::PropertyFrequency()
{
    setUnit(Units::Frequency);
}

//**************************************************************************
// PropertyForce
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyForce, App::PropertyQuantity)

PropertyForce::PropertyForce()
{
    setUnit(Units::Force);
}

//**************************************************************************
// PropertyHeatFlux
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyHeatFlux, App::PropertyQuantity)

PropertyHeatFlux::PropertyHeatFlux()
{
    setUnit(Units::HeatFlux);
}

//**************************************************************************
// PropertyInverseArea
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyInverseArea, App::PropertyQuantity)

PropertyInverseArea::PropertyInverseArea()
{
    setUnit(Units::InverseArea);
}

//**************************************************************************
// PropertyInverseLength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyInverseLength, App::PropertyQuantity)

PropertyInverseLength::PropertyInverseLength()
{
    setUnit(Units::InverseLength);
}

//**************************************************************************
// PropertyInverseVolume
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyInverseVolume, App::PropertyQuantity)

PropertyInverseVolume::PropertyInverseVolume()
{
    setUnit(Units::InverseVolume);
}

//**************************************************************************
// PropertyKinematicViscosity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyKinematicViscosity, App::PropertyQuantity)

PropertyKinematicViscosity::PropertyKinematicViscosity()
{
    setUnit(Units::KinematicViscosity);
}

//**************************************************************************
// PropertyLength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLength, App::PropertyQuantityConstraint)

PropertyLength::PropertyLength()
{
    setUnit(Units::Length);
    setConstraints(&LengthStandard);
}

//**************************************************************************
// PropertyLuminousIntensity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLuminousIntensity, App::PropertyQuantity)

PropertyLuminousIntensity::PropertyLuminousIntensity()
{
    setUnit(Units::LuminousIntensity);
}

//**************************************************************************
// PropertyMagneticFieldStrength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMagneticFieldStrength, App::PropertyQuantity)

PropertyMagneticFieldStrength::PropertyMagneticFieldStrength()
{
    setUnit(Units::MagneticFieldStrength);
}

//**************************************************************************
// PropertyMagneticFlux
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMagneticFlux, App::PropertyQuantity)

PropertyMagneticFlux::PropertyMagneticFlux()
{
    setUnit(Units::MagneticFlux);
}

//**************************************************************************
// PropertyMagneticFluxDensity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMagneticFluxDensity, App::PropertyQuantity)

PropertyMagneticFluxDensity::PropertyMagneticFluxDensity()
{
    setUnit(Units::MagneticFluxDensity);
}

//**************************************************************************
// PropertyMagnetization
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMagnetization, App::PropertyQuantity)

PropertyMagnetization::PropertyMagnetization()
{
    setUnit(Units::Magnetization);
}

//**************************************************************************
// PropertyElectromagneticPotential
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyElectromagneticPotential, App::PropertyQuantity)

PropertyElectromagneticPotential::PropertyElectromagneticPotential()
{
    setUnit(Units::ElectromagneticPotential);
}

//**************************************************************************
// PropertyMass
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMass, App::PropertyQuantity)

PropertyMass::PropertyMass()
{
    setUnit(Units::Mass);
}

//**************************************************************************
// PropertyMoment
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMoment, App::PropertyQuantity)

PropertyMoment::PropertyMoment()
{
    setUnit(Units::Moment);
}

//**************************************************************************
// PropertyPressure
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPressure, App::PropertyQuantity)

PropertyPressure::PropertyPressure()
{
    setUnit(Units::Pressure);
}

//**************************************************************************
// PropertyPower
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPower, App::PropertyQuantity)

PropertyPower::PropertyPower()
{
    setUnit(Units::Power);
}

//**************************************************************************
// PropertySpecificEnergy
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertySpecificEnergy, App::PropertyQuantity)

PropertySpecificEnergy::PropertySpecificEnergy()
{
    setUnit(Units::SpecificEnergy);
}

//**************************************************************************
// PropertySpecificHeat
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertySpecificHeat, App::PropertyQuantity)

PropertySpecificHeat::PropertySpecificHeat()
{
    setUnit(Units::SpecificHeat);
}

//**************************************************************************
// PropertySpeed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertySpeed, App::PropertyQuantity)

PropertySpeed::PropertySpeed()
{
    setUnit(Units::Velocity);
}

//**************************************************************************
// PropertyStiffness
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStiffness, App::PropertyQuantity)

PropertyStiffness::PropertyStiffness()
{
    setUnit(Units::Stiffness);
}

//**************************************************************************
// PropertyStiffnessDensity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStiffnessDensity, App::PropertyQuantity)

PropertyStiffnessDensity::PropertyStiffnessDensity()
{
    setUnit(Units::StiffnessDensity);
}

//**************************************************************************
// PropertyTemperature
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyTemperature, App::PropertyQuantity)

PropertyTemperature::PropertyTemperature()
{
    setUnit(Units::Temperature);
}

//**************************************************************************
// PropertyThermalConductivity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyThermalConductivity, App::PropertyQuantity)

PropertyThermalConductivity::PropertyThermalConductivity()
{
    setUnit(Units::ThermalConductivity);
}

//**************************************************************************
// PropertyThermalExpansionCoefficient
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyThermalExpansionCoefficient, App::PropertyQuantity)

PropertyThermalExpansionCoefficient::PropertyThermalExpansionCoefficient()
{
    setUnit(Units::ThermalExpansionCoefficient);
}


//**************************************************************************
// PropertyThermalTransferCoefficient
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyThermalTransferCoefficient, App::PropertyQuantity)

PropertyThermalTransferCoefficient::PropertyThermalTransferCoefficient()
{
    setUnit(Units::ThermalTransferCoefficient);
}

//**************************************************************************
// PropertyTime
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyTime, App::PropertyQuantity)

PropertyTime::PropertyTime()
{
    setUnit(Units::TimeSpan);
}

//**************************************************************************
// PropertyShearModulus
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyShearModulus, App::PropertyQuantity)

PropertyShearModulus::PropertyShearModulus()
{
    setUnit(Units::ShearModulus);
}

//**************************************************************************
// PropertyStress
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStress, App::PropertyQuantity)

PropertyStress::PropertyStress()
{
    setUnit(Units::Stress);
}

//**************************************************************************
// PropertyUltimateTensileStrength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyUltimateTensileStrength, App::PropertyQuantity)

PropertyUltimateTensileStrength::PropertyUltimateTensileStrength()
{
    setUnit(Units::UltimateTensileStrength);
}

//**************************************************************************
// PropertyVacuumPermittivity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVacuumPermittivity, App::PropertyQuantity)

PropertyVacuumPermittivity::PropertyVacuumPermittivity()
{
    setUnit(Units::VacuumPermittivity);
}

//**************************************************************************
// PropertyVelocity
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVelocity, App::PropertyQuantity)

PropertyVelocity::PropertyVelocity()
{
    setUnit(Units::Velocity);
}

//**************************************************************************
// PropertyVolume
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVolume, App::PropertyQuantityConstraint)

PropertyVolume::PropertyVolume()
{
    setUnit(Units::Volume);
    setConstraints(&LengthStandard);
}

//**************************************************************************
// PropertyVolumeFlowRate
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVolumeFlowRate, App::PropertyQuantity)

PropertyVolumeFlowRate::PropertyVolumeFlowRate()
{
    setUnit(Units::VolumeFlowRate);
}

//**************************************************************************
// PropertyVolumetricThermalExpansionCoefficient
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVolumetricThermalExpansionCoefficient, App::PropertyQuantity)

PropertyVolumetricThermalExpansionCoefficient::PropertyVolumetricThermalExpansionCoefficient()
{
    setUnit(Units::VolumetricThermalExpansionCoefficient);
}

//**************************************************************************
// PropertyWork
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyWork, App::PropertyQuantity)

PropertyWork::PropertyWork()
{
    setUnit(Units::Work);
}

//**************************************************************************
// PropertyYieldStrength
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyYieldStrength, App::PropertyQuantity)

PropertyYieldStrength::PropertyYieldStrength()
{
    setUnit(Units::YieldStrength);
}

//**************************************************************************
// PropertyYoungsModulus
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyYoungsModulus, App::PropertyQuantity)

PropertyYoungsModulus::PropertyYoungsModulus()
{
    setUnit(Units::YoungsModulus);
}
