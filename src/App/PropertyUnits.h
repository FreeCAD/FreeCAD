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


#ifndef APP_PROPERTYUNITS_H
#define APP_PROPERTYUNITS_H

#include <Base/Quantity.h>
#include <Base/Unit.h>

#include "PropertyStandard.h"

namespace Base {
class Writer;
}


namespace App
{

/** Float with Unit property
 * This is a property for float with a predefined Unit associated.
 */
class AppExport PropertyQuantity : public PropertyFloat
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyQuantity() = default;
    ~PropertyQuantity() override = default;

    Base::Quantity getQuantityValue() const;

    const char* getEditorName() const override;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void setUnit(const Base::Unit &u) {_Unit = u;}
    const Base::Unit &getUnit() const {return _Unit;}

    void setValue(double lValue) { PropertyFloat::setValue(lValue); }
    double getValue() const { return PropertyFloat::getValue(); }

    void setPathValue(const App::ObjectIdentifier &path, const boost::any &value) override;
    const boost::any getPathValue(const App::ObjectIdentifier &path) const override;

    bool isSame(const Property &other) const override {
        if (&other == this)
            return true;
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue()
            && _Unit == static_cast<decltype(this)>(&other)->_Unit;
    }

protected:
    Base::Quantity createQuantityFromPy(PyObject *value);
    Base::Unit _Unit;
};

/** Float with Unit property
 * This is a property for float with a predefined Unit associated.
 */
class AppExport PropertyQuantityConstraint : public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyQuantityConstraint():_ConstStruct(nullptr){}
    ~PropertyQuantityConstraint() override = default;

    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints {
        double LowerBound, UpperBound, StepSize;
    };
    /** setting the boundaries
     * This sets the constraint struct. It can be dynamically
     * allocated or set as an static in the class the property
     * belongs to:
     * \code
     * const Constraints percent = {0.0,100.0,1.0}
     * \endcode
     */
    void setConstraints(const Constraints* sConstrain);
    /// get the constraint struct
    const Constraints*  getConstraints() const;
    //@}

    double getMinimum() const;
    double getMaximum() const;
    double getStepSize() const;

    const char* getEditorName() const override;
    void setPyObject(PyObject *) override;


protected:
    const Constraints* _ConstStruct;
};

/** Acceleration property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyAcceleration: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyAcceleration();
    ~PropertyAcceleration() override = default;
};

/** Angle property
 * This is a property for representing angles. It basically a float
 * property. On the Gui it has a quantity like RAD.
 */
class AppExport PropertyAngle: public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyAngle();
    ~PropertyAngle() override = default;
    const char *getEditorName() const override { return "Gui::PropertyEditor::PropertyAngleItem"; }
};

/** Area property
 * This is a property for representing areas. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m^2 or mm^2.
 */
class AppExport PropertyArea: public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyArea();
    ~PropertyArea() override = default;
};

/** Distance property
 * This is a property for representing distances. It is basically a float
 * property. On the Gui it has a quantity like m or mm.
 */
class AppExport PropertyDistance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyDistance();
    ~PropertyDistance() override = default;
};

/** CurrentDensity property
 * This is a property for electric current densities. It is basically a float
 * property. On the Gui it has a quantity of Ampere/m^2.
 */
class AppExport PropertyCurrentDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyCurrentDensity();
    ~PropertyCurrentDensity() override = default;
};

/** ElectricPotential property
 * This is a property for electric potentials. It is basically a float
 * property. On the Gui it has a quantity of Volt.
 */
class AppExport PropertyElectricPotential: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricPotential();
    ~PropertyElectricPotential() override = default;
};

/** Frequency property
 * This is a property for representing frequency. It is basically a float
 * property. On the Gui it has a quantity like 1/s or Hz.
 */
class AppExport PropertyFrequency: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFrequency();
    ~PropertyFrequency() override = default;
};

/** Force property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyForce: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyForce();
    ~PropertyForce() override = default;
};

/** Length property
 * This is a property for representing lengths. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m or mm.
 */
class AppExport PropertyLength: public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyLength();
    ~PropertyLength() override = default;
};

 /** Magnetization property
 * This is a property for representing magnetizations. It is basically a float
 * property. On the Gui it has a quantity like A/m.
 */
    class AppExport PropertyMagnetization: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMagnetization();
    ~PropertyMagnetization() override = default;
};

/** Pressure property
 * This is a property for representing pressure. It basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyPressure: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPressure();
    ~PropertyPressure() override = default;
};

/** Speed property
 * This is a property for representing speed. It is basically a float
 * property. On the Gui it has a quantity like m/s or km/h.
 */
class AppExport PropertySpeed: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertySpeed();
    ~PropertySpeed() override = default;
};

/** Stiffness property
 * This is a property for representing stiffness. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyStiffness: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyStiffness();
    ~PropertyStiffness() override = default;
};

/** Mass property
 * This is a property for representing mass. It is basically a float
 * property. On the Gui it has a quantity like kg.
 */
class AppExport PropertyMass: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyMass(void);
    virtual ~PropertyMass(){}
};

/** Density property
 * This is a property for representing density. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like
 * kg/m^3.
 */
class AppExport PropertyDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyDensity(void);
    virtual ~PropertyDensity(){}
};

/** TimeSpan property
 * This is a property for representing time intervals. It is basically a float
 * property. On the Gui it has a quantity like s.
 */
class AppExport PropertyTime: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyTime(void);
    virtual ~PropertyTime(){}
};

/** Velocity property
 * This is a property for representing velocities. It is basically a float
 * property. On the Gui it has a quantity like m/s.
 */
class AppExport PropertyVelocity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyVelocity(void);
    virtual ~PropertyVelocity(){}
};

/** Temperature property
 * This is a property for representing temperatures. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like K.
 */
class AppExport PropertyTemperature: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyTemperature(void);
    virtual ~PropertyTemperature(){}
};

/** ElectricCurrent property
 * This is a property for representing electric currents. It is basically a
 * float property. On the Gui it has a quantity like A.
 */
class AppExport PropertyElectricCurrent: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricCurrent(void);
    virtual ~PropertyElectricCurrent(){}
};

/** ElectricCharge property
 * This is a property for representing electric charge. It is basically a float
 * property. On the Gui it has a quantity like C.
 */
class AppExport PropertyElectricCharge: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricCharge(void);
    virtual ~PropertyElectricCharge(){}
};

/** MagneticFieldStrength property
 * This is a property for representing magnetic field strength. It is basically
 * a float property. On the Gui it has a quantity like Wb.
 */
class AppExport PropertyMagneticFieldStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyMagneticFieldStrength(void);
    virtual ~PropertyMagneticFieldStrength(){}
};

/** MagneticFlux property
 * This is a property for representing magnetic flux. It is basically a float
 * property. On the Gui it has a quantity like Wb.
 */
class AppExport PropertyMagneticFlux: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyMagneticFlux(void);
    virtual ~PropertyMagneticFlux(){}
};

/** MagneticFluxDensity property
 * This is a property for representing magnetic flux density. It is basically a
 * float property. On the Gui it has a quantity like G or T.
 */
class AppExport PropertyMagneticFluxDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyMagneticFluxDensity(void);
    virtual ~PropertyMagneticFluxDensity(){}
};

/** ElectricalCapacitance property
 * This is a property for representing capacitance. It is basically a float
 * property. On the Gui it has a quantity like uF.
 */
class AppExport PropertyElectricalCapacitance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricalCapacitance(void);
    virtual ~PropertyElectricalCapacitance(){}
};

/** ElectricalInductance property
 * This is a property for representing electrical inductance. It is basically a
 * float property. On the Gui it has a quantity like H.
 */
class AppExport PropertyElectricalInductance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricalInductance(void);
    virtual ~PropertyElectricalInductance(){}
};

/** ElectricalConductance property
 * This is a property for representing electrical conductance. It is basically a
 * float property. On the Gui it has a quantity like S.
 */
class AppExport PropertyElectricalConductance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricalConductance(void);
    virtual ~PropertyElectricalConductance(){}
};

/** ElectricalResistance property
 * This is a property for representing electrical resistance. It is basically a
 * float property. On the Gui it has a quantity like Ohm.
 */

class AppExport PropertyElectricalResistance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricalResistance(void);
    virtual ~PropertyElectricalResistance(){}
};

/** ElectricalConductivity property
 * This is a property for representing electrical conductivity. It is basically
 * a float property. On the Gui it has a quantity like S/m.
 */
class AppExport PropertyElectricalConductivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyElectricalConductivity(void);
    virtual ~PropertyElectricalConductivity(){}
};

/** AmountOfSubstance property
 * This is a property for representing number of molecules. It is basically a
 * float property. On the Gui it has a quantity like mole.
 */
class AppExport PropertyAmountOfSubstance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyAmountOfSubstance(void);
    virtual ~PropertyAmountOfSubstance(){}
};

/** LuminousIntensity property
 * This is a property for representing luminous intensity. It is basically a
 * float property. On the Gui it has a quantity like cd.
 */
class AppExport PropertyLuminousIntensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyLuminousIntensity(void);
    virtual ~PropertyLuminousIntensity(){}
};

/** CompressiveStrength property
 * This is a property for representing compressive strength. It is basically a
 * float property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyCompressiveStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyCompressiveStrength(void);
    virtual ~PropertyCompressiveStrength(){}
};

/** ShearModulus property
 * This is a property for representing shear modulus. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyShearModulus: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyShearModulus(void);
    virtual ~PropertyShearModulus(){}
};

/** Stress property
 * This is a property for representing . It is basically a float
 * property. On the Gui it has a quantity like .
 */
class AppExport PropertyStress: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyStress(void);
    virtual ~PropertyStress(){}
};

/** UltimateTensileStrength property
 * This is a property for representing ultimate tensile strength. It is
 * basically a float property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyUltimateTensileStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyUltimateTensileStrength(void);
    virtual ~PropertyUltimateTensileStrength(){}
};

/** YieldStrength property
 * This is a property for representing yield strength. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyYieldStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyYieldStrength(void);
    virtual ~PropertyYieldStrength(){}
};

/** YoungsModulus property
 * This is a property for representing Young's modulus. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyYoungsModulus: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyYoungsModulus(void);
    virtual ~PropertyYoungsModulus(){}
};

/** Work property
 * This is a property for representing work. It is basically a float
 * property. On the Gui it has a quantity like Nm.
 */
class AppExport PropertyWork: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyWork(void);
    virtual ~PropertyWork(){}
};

/** Power property
 * This is a property for representing power. It is basically a float
 * property. On the Gui it has a quantity like W.
 */
class AppExport PropertyPower: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyPower(void);
    virtual ~PropertyPower(){}
};

/** SpecificEnergy property
 * This is a property for representing specific energy. It is basically a float
 * property. On the Gui it has a quantity like m^2/s^2.
 */
class AppExport PropertySpecificEnergy: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertySpecificEnergy(void);
    virtual ~PropertySpecificEnergy(){}
};

/** ThermalConductivity property
 * This is a property for representing thermal conductivity. It is basically a
 * float property. On the Gui it has a quantity like W/m/K.
 */
class AppExport PropertyThermalConductivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyThermalConductivity(void);
    virtual ~PropertyThermalConductivity(){}
};

/** ThermalExpansionCoefficient property
 * This is a property for representing a coefficient of thermal expansion. It
 * basically a float property. On the Gui it has a quantity like 1/K.
 */
class AppExport PropertyThermalExpansionCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyThermalExpansionCoefficient(void);
    virtual ~PropertyThermalExpansionCoefficient(){}
};

/** VolumetricThermalExpansionCoefficient property
 * This is a property for representing . It is basically a float
 * property. On the Gui it has a quantity like 1/K.
 */
class AppExport PropertyVolumetricThermalExpansionCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyVolumetricThermalExpansionCoefficient(void);
    virtual ~PropertyVolumetricThermalExpansionCoefficient(){}
};

/** SpecificHeat property
 * This is a property for representing specific heat capacity. It is basically a
 * float property. On the Gui it has a quantity like J/kg/K.
 */
class AppExport PropertySpecificHeat: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertySpecificHeat(void);
    virtual ~PropertySpecificHeat(){}
};

/** ThermalTransferCoefficient property
 * This is a property for representing heat transfer coefficient. It is
 * basically a float property. On the Gui it has a quantity like W/m^2/K.
 */
class AppExport PropertyThermalTransferCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyThermalTransferCoefficient(void);
    virtual ~PropertyThermalTransferCoefficient(){}
};

/** HeatFlux property
 * This is a property for representing heat flux. It is basically a float
 * property. On the Gui it has a quantity like W/m^2.
 */
class AppExport PropertyHeatFlux: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyHeatFlux(void);
    virtual ~PropertyHeatFlux(){}
};

/** DynamicViscosity property
 * This is a property for representing dynamic viscosity. It is basically a
 * float property which must not be negative. On the Gui it has a quantity like
 * Pa*s.
 */
class AppExport PropertyDynamicViscosity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyDynamicViscosity(void);
    virtual ~PropertyDynamicViscosity(){}
};

/** KinematicViscosity property
 * This is a property for representing kinematic viscosity. It is basically a
 * float property which must not be negative. On the Gui it has a quantity like
 * m^2/s^2.
 */
class AppExport PropertyKinematicViscosity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyKinematicViscosity(void);
    virtual ~PropertyKinematicViscosity(){}
};

/** VacuumPermittivity property
 * This is a property for representing vacuum permittivity. It is basically a float
 * property. On the Gui it has a quantity like s^4*A^2 / (m^3*kg).
 */
class AppExport PropertyVacuumPermittivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyVacuumPermittivity();
    ~PropertyVacuumPermittivity() override = default;
};

/** Volume property
 * This is a property for representing volumes. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m^3 or mm^3.
 */
class AppExport PropertyVolume: public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyVolume();
    ~PropertyVolume() override = default;
};

/** VolumeFlowRate property
 * This is a property for representing volumetric flow rate. It is basically a
 * float property. On the Gui it has a quantity like l/s.
 */
class AppExport PropertyVolumeFlowRate: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyVolumeFlowRate(void);
    virtual ~PropertyVolumeFlowRate(){}
};

/** DissipationRate property
 * This is a property for representing turbulent dissipation rate. It basically
 * a float property. On the Gui it has a quantity like m^2/s^3.
 */
class AppExport PropertyDissipationRate: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyDissipationRate(void);
    virtual ~PropertyDissipationRate(){}
};

/** InverseLength property
 * This is a property for representing the reciprocal of length. It is basically
 * a float property which must not be negative. On the Gui it has a quantity
 * like 1/m.
 */
class AppExport PropertyInverseLength: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyInverseLength(void);
    virtual ~PropertyInverseLength(){}
};

/** InverseArea property
 * This is a property for representing the reciprocal of area. It is basically a
 * float property which must not be negative. On the Gui it has a quantity like
 * 1/m^2.
 */
class AppExport PropertyInverseArea: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyInverseArea(void);
    virtual ~PropertyInverseArea(){}
};

/** InverseVolume property
 * This is a property for representing the reciprocal of volume. It is basically
 * a float property. which must not be negative. On the Gui it has a quantity
 * like 1/m^3.
 */
class AppExport PropertyInverseVolume: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

public:
    PropertyInverseVolume(void);
    virtual ~PropertyInverseVolume(){}
};


}// namespace App

#endif// APP_PROPERTYUNITS_H
