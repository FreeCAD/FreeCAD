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
    PropertyQuantityConstraint() = default;
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

private:
    const Constraints* _ConstStruct{nullptr};
};

// ------------------------------------------------------
// now all properties
// ------------------------------------------------------

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

/** AmountOfSubstance property
 * This is a property for representing number of molecules. It is basically a
 * float property. On the Gui it has a quantity like mole.
 */
class AppExport PropertyAmountOfSubstance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyAmountOfSubstance();
    ~PropertyAmountOfSubstance() override = default;
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

/** CompressiveStrength property
 * This is a property for representing compressive strength. It is basically a
 * float property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyCompressiveStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyCompressiveStrength();
    ~PropertyCompressiveStrength() override = default;
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

/** Density property
 * This is a property for representing density. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like
 * kg/m^3.
 */
class AppExport PropertyDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyDensity();
    ~PropertyDensity() override = default;
};

/** DissipationRate property
 * This is a property for representing turbulent dissipation rate. It basically
 * a float property. On the Gui it has a quantity like m^2/s^3.
 */
class AppExport PropertyDissipationRate: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyDissipationRate();
    ~PropertyDissipationRate() override = default;
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

/** DynamicViscosity property
 * This is a property for representing dynamic viscosity. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like Pa*s.
 */
class AppExport PropertyDynamicViscosity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyDynamicViscosity();
    ~PropertyDynamicViscosity() override = default;
};

/** ElectricalCapacitance property
 * This is a property for representing capacitance. It is basically a float
 * property. On the Gui it has a quantity like F.
 */
class AppExport PropertyElectricalCapacitance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricalCapacitance();
    ~PropertyElectricalCapacitance() override = default;
};

/** ElectricalConductance property
 * This is a property for representing electrical conductance. It is basically a
 * float property. On the Gui it has a quantity like S.
 */
class AppExport PropertyElectricalConductance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricalConductance();
    ~PropertyElectricalConductance() override = default;
};

/** ElectricalConductivity property
 * This is a property for representing electrical conductivity. It is basically
 * a float property. On the Gui it has a quantity like S/m.
 */
class AppExport PropertyElectricalConductivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricalConductivity();
    ~PropertyElectricalConductivity() override = default;
};

/** ElectricalInductance property
 * This is a property for representing electrical inductance. It is basically a
 * float property. On the Gui it has a quantity like H.
 */
class AppExport PropertyElectricalInductance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricalInductance();
    ~PropertyElectricalInductance() override = default;
};

/** ElectricalResistance property
 * This is a property for representing electrical resistance. It is basically a
 * float property. On the Gui it has a quantity like Ohm.
 */

class AppExport PropertyElectricalResistance: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricalResistance();
    ~PropertyElectricalResistance() override = default;
};

/** ElectricCharge property
 * This is a property for representing electric charge. It is basically a float
 * property. On the Gui it has a quantity like C.
 */
class AppExport PropertyElectricCharge: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricCharge();
    ~PropertyElectricCharge() override = default;
};

/** ElectricCurrent property
 * This is a property for representing electric currents. It is basically a
 * float property. On the Gui it has a quantity like A.
 */
class AppExport PropertyElectricCurrent: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyElectricCurrent();
    ~PropertyElectricCurrent() override = default;
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

/** HeatFlux property
 * This is a property for representing heat flux. It is basically a float
 * property. On the Gui it has a quantity like W/m^2.
 */
class AppExport PropertyHeatFlux: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyHeatFlux();
    ~PropertyHeatFlux() override = default;
};

/** InverseArea property
 * This is a property for representing the reciprocal of area. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like 1/m^2.
 */
class AppExport PropertyInverseArea: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyInverseArea();
    ~PropertyInverseArea() override = default;
};

/** InverseLength property
 * This is a property for representing the reciprocal of length. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like 1/m.
 */
class AppExport PropertyInverseLength: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyInverseLength();
    ~PropertyInverseLength() override = default;
};

/** InverseVolume property
 * This is a property for representing the reciprocal of volume. It is basically a float
*  property. which must not be negative. On the Gui it has a quantity like 1/m^3.
 */
class AppExport PropertyInverseVolume: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyInverseVolume();
    ~PropertyInverseVolume() override = default;
};

/** KinematicViscosity property
 * This is a property for representing kinematic viscosity. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m^2/s.
 */
class AppExport PropertyKinematicViscosity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyKinematicViscosity();
    ~PropertyKinematicViscosity() override = default;
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

/** LuminousIntensity property
 * This is a property for representing luminous intensity. It is basically a
 * float property. On the Gui it has a quantity like cd.
 */
class AppExport PropertyLuminousIntensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyLuminousIntensity();
    ~PropertyLuminousIntensity() override = default;
};

/** MagneticFieldStrength property
 * This is a property for representing magnetic field strength. It is basically
 * a float property. On the Gui it has a quantity like A/m.
 */
class AppExport PropertyMagneticFieldStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMagneticFieldStrength();
    ~PropertyMagneticFieldStrength() override = default;
};

/** MagneticFlux property
 * This is a property for representing magnetic flux. It is basically a float
 * property. On the Gui it has a quantity like Wb.
 */
class AppExport PropertyMagneticFlux: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMagneticFlux();
    ~PropertyMagneticFlux() override = default;
};

/** MagneticFluxDensity property
 * This is a property for representing magnetic flux density. It is basically a
 * float property. On the Gui it has a quantity like G or T.
 */
class AppExport PropertyMagneticFluxDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMagneticFluxDensity();
    ~PropertyMagneticFluxDensity() override = default;
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

/** Mass property
 * This is a property for representing mass. It is basically a float
 * property. On the Gui it has a quantity like kg.
 */
class AppExport PropertyMass: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMass();
    ~PropertyMass() override = default;
};

/** Moment property
 * This is a property for representing moment. It is basically a float
 * property. On the Gui it has a quantity like N*m.
 */
class AppExport PropertyMoment: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMoment();
    ~PropertyMoment() override = default;
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

/** Power property
 * This is a property for representing power. It is basically a float
 * property. On the Gui it has a quantity like W.
 */
class AppExport PropertyPower: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPower();
    ~PropertyPower() override = default;
};

/** ShearModulus property
 * This is a property for representing shear modulus. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyShearModulus: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyShearModulus();
    ~PropertyShearModulus() override = default;
};

/** SpecificEnergy property
 * This is a property for representing specific energy. It is basically a float
 * property. On the Gui it has a quantity like m^2/s^2 or J/kg.
 */
class AppExport PropertySpecificEnergy: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertySpecificEnergy();
    ~PropertySpecificEnergy() override = default;
};

/** SpecificHeat property
 * This is a property for representing specific heat capacity. It is basically a
 * float property. On the Gui it has a quantity like J/kg/K.
 */
class AppExport PropertySpecificHeat: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertySpecificHeat();
    ~PropertySpecificHeat() override = default;
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
 * property. On the Gui it has a quantity like N/m.
 */
class AppExport PropertyStiffness: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyStiffness();
    ~PropertyStiffness() override = default;
};

/** StiffnessDensity property
 * This is a property for representing stiffness per area unit. It is basically a float
 * property. On the Gui it has a quantity like Pa/m.
 */
class AppExport PropertyStiffnessDensity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyStiffnessDensity();
    ~PropertyStiffnessDensity() override = default;
};

/** Stress property
 * This is a property for representing stress. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyStress: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyStress();
    ~PropertyStress() override = default;
};

/** Temperature property
 * This is a property for representing temperatures. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like K.
 */
class AppExport PropertyTemperature: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyTemperature();
    ~PropertyTemperature() override = default;
};

/** ThermalConductivity property
 * This is a property for representing thermal conductivity. It is basically a
 * float property. On the Gui it has a quantity like W/m/K.
 */
class AppExport PropertyThermalConductivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyThermalConductivity();
    ~PropertyThermalConductivity() override = default;
};

/** ThermalExpansionCoefficient property
 * This is a property for representing a coefficient of thermal expansion. It
 * basically a float property. On the Gui it has a quantity like 1/K.
 */
class AppExport PropertyThermalExpansionCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyThermalExpansionCoefficient();
    ~PropertyThermalExpansionCoefficient() override = default;
};

/** ThermalTransferCoefficient property
 * This is a property for representing heat transfer coefficient. It is
 * basically a float property. On the Gui it has a quantity like W/m^2/K.
 */
class AppExport PropertyThermalTransferCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyThermalTransferCoefficient();
    ~PropertyThermalTransferCoefficient() override = default;
};

/** TimeSpan property
 * This is a property for representing time intervals. It is basically a float
 * property. On the Gui it has a quantity like s.
 */
class AppExport PropertyTime: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyTime();
    ~PropertyTime() override = default;
};

/** UltimateTensileStrength property
 * This is a property for representing ultimate tensile strength. It is
 * basically a float property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyUltimateTensileStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyUltimateTensileStrength();
    ~PropertyUltimateTensileStrength() override = default;
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

/** Velocity property
 * This is a property for representing velocities. It is basically a float
 * property. On the Gui it has a quantity like m/s.
 */
class AppExport PropertyVelocity: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyVelocity();
    ~PropertyVelocity() override = default;
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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyVolumeFlowRate();
    ~PropertyVolumeFlowRate() override = default;
};

/** VolumetricThermalExpansionCoefficient property
 * This is a property for representing . It is basically a float
 * property. On the Gui it has a quantity like 1/K.
 */
class AppExport PropertyVolumetricThermalExpansionCoefficient: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyVolumetricThermalExpansionCoefficient();
    ~PropertyVolumetricThermalExpansionCoefficient() override = default;
};

/** Work property
 * This is a property for representing work. It is basically a float
 * property. On the Gui it has a quantity like Nm.
 */
class AppExport PropertyWork: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyWork();
    ~PropertyWork() override = default;
};

/** YieldStrength property
 * This is a property for representing yield strength. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyYieldStrength: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyYieldStrength();
    ~PropertyYieldStrength() override = default;
};

/** YoungsModulus property
 * This is a property for representing Young's modulus. It is basically a float
 * property. On the Gui it has a quantity like Pa.
 */
class AppExport PropertyYoungsModulus: public PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyYoungsModulus();
    ~PropertyYoungsModulus() override = default;
};

}// namespace App

#endif// APP_PROPERTYUNITS_H
