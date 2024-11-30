/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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

#ifndef FEM_CONSTRAINTBEARING_H
#define FEM_CONSTRAINTBEARING_H

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include "FemConstraint.h"


namespace Fem
{

class FemExport ConstraintBearing: public Fem::Constraint
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::ConstraintBearing);

public:
    /// Constructor
    ConstraintBearing();

    /// Location reference
    App::PropertyLinkSub Location;
    /// Distance from location reference
    App::PropertyFloat Dist;
    /// Is the bearing free to move in axial direction?
    App::PropertyBool AxialFree;
    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyFloat Radius;
    App::PropertyFloat Height;
    App::PropertyVector BasePoint;
    App::PropertyVector Axis;

    /// recalculate the object
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemConstraintBearing";
    }

protected:
    void onChanged(const App::Property* prop) override;
};

}  // namespace Fem


#endif  // FEM_CONSTRAINTBEARING_H
