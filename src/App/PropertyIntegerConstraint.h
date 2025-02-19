/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef APP_PROPERTYINTEGERCONSTRAINT_H
#define APP_PROPERTYINTEGERCONSTRAINT_H

#include "PropertyInteger.h"


namespace App {

/** Constraint integer properties
 * This property fulfills the need of a constraint integer. It holds basically a
 * state (integer) and a struct of boundaries. If the boundaries
 * is not set it acts basically like an IntegerProperty and does no checking.
 * The constraints struct can be created on the heap or build in.
 */
class AppExport PropertyIntegerConstraint: public PropertyInteger
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Standard constructor
    PropertyIntegerConstraint();

    /// destructor
    ~PropertyIntegerConstraint() override;

    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints
    {
        long LowerBound, UpperBound, StepSize;
        Constraints()
            : LowerBound(0)
            , UpperBound(0)
            , StepSize(0)
            , candelete(false)
        {}
        Constraints(long l, long u, long s)
            : LowerBound(l)
            , UpperBound(u)
            , StepSize(s)
            , candelete(false)
        {}
        ~Constraints() = default;
        void setDeletable(bool on)
        {
            candelete = on;
        }
        bool isDeletable() const
        {
            return candelete;
        }

    private:
        bool candelete;
    };
    /** setting the boundaries
    * This sets the constraint struct. It can be dynamically
    * allocated or set as a static in the class the property
    * belongs to:
    * \code
    * const Constraints percent = {0,100,1}
    * \endcode
    */
    void setConstraints(const Constraints* sConstraint);
    /// get the constraint struct
    const Constraints* getConstraints() const;
    //@}

    long getMinimum() const;
    long getMaximum() const;
    long getStepSize() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyIntegerConstraintItem";
    }
    void setPyObject(PyObject* py) override;

protected:
    const Constraints* _ConstStruct {nullptr};
};

}  // namespace

#endif
