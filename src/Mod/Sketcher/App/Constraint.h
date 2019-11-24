/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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


#ifndef SKETCHER_CONSTRAINT_H
#define SKETCHER_CONSTRAINT_H


#include <Base/Persistence.h>
#include <Base/Quantity.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace Sketcher
{
/*!
 Important note: New constraint types must be always added at the end but before 'NumConstraintTypes'.
 This is mandatory in order to keep the handling of constraint types upward compatible which means that
 this program version ignores later introduced constraint types when reading them from a project file.
 */
enum ConstraintType {
    None = 0,
    Coincident = 1,
    Horizontal = 2,
    Vertical = 3,
    Parallel = 4,
    Tangent = 5,
    Distance = 6,
    DistanceX = 7,
    DistanceY = 8,
    Angle = 9,
    Perpendicular = 10,
    Radius = 11,
    Equal = 12,
    PointOnObject = 13,
    Symmetric = 14,
    InternalAlignment = 15,
    SnellsLaw = 16,
    Block = 17,
    Diameter = 18,
    NumConstraintTypes // must be the last item!
};

enum InternalAlignmentType {
    Undef                   = 0,
    EllipseMajorDiameter    = 1,
    EllipseMinorDiameter    = 2,
    EllipseFocus1           = 3,
    EllipseFocus2           = 4,
    HyperbolaMajor          = 5,
    HyperbolaMinor          = 6,
    HyperbolaFocus          = 7,
    ParabolaFocus           = 8,
    BSplineControlPoint     = 9,
    BSplineKnotPoint        = 10,
};

/// define if you want to use the end or start point
enum PointPos { none, start, end, mid };

class SketcherExport Constraint : public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Constraint();
    // PVS V690: It is perfectly fine to use copy operator only internally

    // Constraints objects explicitly not copiable with standard methods
    // Copy constructor is private for internal use only
    Constraint &operator =(const Constraint &a) = delete;

    // Constraints objects explicitly not movable
    Constraint(Constraint&&) = delete;
    Constraint& operator=(Constraint&&) = delete;

    virtual ~Constraint() = default;

    Constraint *clone(void) const; // does copy the tag, it will be treated as a rename by the expression engine.
    Constraint *copy(void) const; // does not copy the tag, but generates a new one

    static const int GeoUndef;

    // from base class
    virtual unsigned int getMemSize(void) const override;
    virtual void Save(Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    virtual PyObject *getPyObject(void) override;

    Base::Quantity getPresentationValue() const;
    inline void setValue(double newValue) {
        Value = newValue;
    }
    inline double getValue() const {
        return Value;
    }

    inline bool isDimensional() const {
        return Type == Distance || Type == DistanceX || Type == DistanceY ||
               Type == Radius || Type == Diameter || Type == Angle || Type == SnellsLaw;
    }

    friend class PropertyConstraintList;

private:
    Constraint(const Constraint&) = default; // only for internal use

private:
    double Value;
public:
    ConstraintType Type;
    InternalAlignmentType AlignmentType;
    std::string Name;
    int First;
    PointPos FirstPos;
    int Second;
    PointPos SecondPos;
    int Third;
    PointPos ThirdPos;
    float LabelDistance;
    float LabelPosition;
    bool isDriving;
    int InternalAlignmentIndex; // Note: for InternalAlignment Type this index indexes equal internal geometry elements (e.g. index of pole in a bspline). It is not a GeoId!!
    bool isInVirtualSpace;

    bool isActive;

protected:
    boost::uuids::uuid tag;
};

} //namespace Sketcher


#endif // SKETCHER_CONSTRAINT_H
