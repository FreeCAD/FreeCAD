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

#pragma once

#include "FemConstraintGear.h"


namespace Fem
{

class FemExport ConstraintPulley: public Fem::ConstraintGear
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::ConstraintPulley);

public:
    /// Constructor
    ConstraintPulley();

    /// Other pulley diameter
    App::PropertyFloat OtherDiameter;
    /// Center distance between the pulleys
    App::PropertyFloat CenterDistance;
    /// Driven pulley or driving pulley?
    App::PropertyBool IsDriven;
    /// Belt tension force
    App::PropertyFloat TensionForce;
    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyFloat BeltAngle;
    App::PropertyFloat BeltForce1;
    App::PropertyFloat BeltForce2;

    /// recalculate the object
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemConstraintPulley";
    }

protected:
    void onChanged(const App::Property* prop) override;
};

}  // namespace Fem
