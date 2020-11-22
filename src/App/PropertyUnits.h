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

// Std. configurations


#include <string>
#include <list>
#include <vector>
#include <boost/filesystem/path.hpp>

#include <Base/Unit.h>
#include <Base/Quantity.h>
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
    PropertyQuantity(void){}
    virtual ~PropertyQuantity(){}

    Base::Quantity getQuantityValue(void) const;

    virtual const char* getEditorName(void) const;

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    void setUnit(const Base::Unit &u) {_Unit = u;}
    const Base::Unit &getUnit(void) const {return _Unit;}

    void setValue(double lValue) { PropertyFloat::setValue(lValue); }
    double getValue(void) const { return PropertyFloat::getValue(); }

    virtual void setPathValue(const App::ObjectIdentifier &path, const boost::any &value);
    virtual const boost::any getPathValue(const App::ObjectIdentifier &path) const;

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
    PropertyQuantityConstraint(void):_ConstStruct(0){}
    virtual ~PropertyQuantityConstraint(){}

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
    const Constraints*  getConstraints(void) const;
    //@}

    virtual const char* getEditorName(void) const;
    virtual void setPyObject(PyObject *);


protected:
    const Constraints* _ConstStruct;
};

/** Distance property
 * This is a property for representing distances. It is basically a float
 * property. On the Gui it has a quantity like m or mm.
 */
class AppExport PropertyDistance: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyDistance(void);
    virtual ~PropertyDistance(){}
};

/** Length property
 * This is a property for representing lengths. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m or mm.
 */
class AppExport PropertyLength : public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER();
public:
    PropertyLength(void);
    virtual ~PropertyLength(){}
};

/** Area property
 * This is a property for representing areas. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m^2 or mm^2.
 */
class AppExport PropertyArea : public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER();
public:
    PropertyArea(void);
    virtual ~PropertyArea(){}
};

/** Volume property
 * This is a property for representing volumes. It is basically a float
 * property which must not be negative. On the Gui it has a quantity like m^3 or mm^3.
 */
class AppExport PropertyVolume : public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER();
public:
    PropertyVolume(void);
    virtual ~PropertyVolume(){}
};

/** Angle property
 * This is a property for representing angles. It basically a float
 * property. On the Gui it has a quantity like RAD.
 */
class AppExport PropertyAngle: public PropertyQuantityConstraint
{
    TYPESYSTEM_HEADER();
public:
    PropertyAngle(void);
    virtual ~PropertyAngle(){}
    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyAngleItem"; }
};

/** Frequency property
 * This is a property for representing frequency. It is basically a float
 * property. On the Gui it has a quantity like 1/s or Hz.
 */
class AppExport PropertyFrequency: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyFrequency(void);
    virtual ~PropertyFrequency(){}
};

/** Speed property
 * This is a property for representing speed. It is basically a float
 * property. On the Gui it has a quantity like m/s or km/h.
 */
class AppExport PropertySpeed: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertySpeed(void);
    virtual ~PropertySpeed(){}
};

/** Acceleration property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyAcceleration: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyAcceleration(void);
    virtual ~PropertyAcceleration(){}
};

/** Pressure property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyPressure: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyPressure(void);
    virtual ~PropertyPressure(){}
};

/** Force property
 * This is a property for representing acceleration. It is basically a float
 * property. On the Gui it has a quantity like m/s^2.
 */
class AppExport PropertyForce: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyForce(void);
    virtual ~PropertyForce(){}
};

/** VacuumPermittivity property
 * This is a property for representing vacuum permittivity. It is basically a float
 * property. On the Gui it has a quantity like s^4*A^2 / (m^3*kg).
 */
class AppExport PropertyVacuumPermittivity: public PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertyVacuumPermittivity(void);
    virtual ~PropertyVacuumPermittivity(){}
};

} // namespace App

#endif // APP_PROPERTYUNITS_H
