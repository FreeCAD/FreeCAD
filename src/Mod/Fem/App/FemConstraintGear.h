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

#ifndef FEM_CONSTRAINTGear_H
#define FEM_CONSTRAINTGear_H

#include "FemConstraintBearing.h"


namespace Fem
{

class FemExport ConstraintGear: public Fem::ConstraintBearing
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::ConstraintGear);

public:
    /// Constructor
    ConstraintGear();

    App::PropertyFloat Diameter;
    App::PropertyFloat Force;
    App::PropertyFloat ForceAngle;
    App::PropertyLinkSub Direction;
    App::PropertyBool Reversed;
    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyVector DirectionVector;

    /// recalculate the object
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemConstraintGear";
    }

protected:
    void onChanged(const App::Property* prop) override;

private:
    Base::Vector3d naturalDirectionVector;
};

}  // namespace Fem


#endif  // FEM_CONSTRAINTGear_H
