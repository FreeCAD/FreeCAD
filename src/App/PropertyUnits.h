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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

public:
    PropertyDistance();
    ~PropertyDistance() override = default;
};

/** ElectricPotential property
 * This is a property for electric potentials. It is basically a float
 * property. On the Gui it has a quantity of Volt.
 */
class AppExport PropertyElectricPotential: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

public:
    PropertyLength();
    ~PropertyLength() override = default;
};

/** Pressure property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyPressure: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

public:
    PropertyStiffness();
    ~PropertyStiffness() override = default;
};

/** VacuumPermittivity property
 * This is a property for representing vacuum permittivity. It is basically a float
 * property. On the Gui it has a quantity like s^4*A^2 / (m^3*kg).
 */
class AppExport PropertyVacuumPermittivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();

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
    TYPESYSTEM_HEADER();

public:
    PropertyVolume();
    ~PropertyVolume() override = default;
};

}// namespace App

#endif// APP_PROPERTYUNITS_H
