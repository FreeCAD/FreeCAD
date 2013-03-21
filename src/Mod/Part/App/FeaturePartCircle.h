/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_FEATUREPARTCIRCLE_H
#define PART_FEATUREPARTCIRCLE_H

#include <App/PropertyUnits.h>
#include "PrimitiveFeature.h"

namespace Part
{
class Circle : public Part::Primitive
{
    PROPERTY_HEADER(Part::Circle);

public:
    Circle();
    virtual ~Circle();

    App::PropertyFloat Radius;
    App::PropertyAngle Angle0;
    App::PropertyAngle Angle1;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    void onChanged(const App::Property*);
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderCircleParametric";
    }

private:
    static App::PropertyFloatConstraint::Constraints angleRange;
    //@}
};

} //namespace Part

#endif // PART_FEATUREPARTCIRCLE_H
