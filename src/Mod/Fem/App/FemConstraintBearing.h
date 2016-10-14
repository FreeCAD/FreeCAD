/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
#include <App/PropertyGeo.h>

#include "FemConstraint.h"

namespace Fem
{

class AppFemExport ConstraintBearing : public Fem::Constraint
{
    PROPERTY_HEADER(Fem::ConstraintBearing);

public:
    /// Constructor
    ConstraintBearing(void);

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
    virtual App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemConstraintBearing";
    }

protected:
    virtual void onChanged(const App::Property* prop);
};

} //namespace Fem


#endif // FEM_CONSTRAINTBEARING_H
