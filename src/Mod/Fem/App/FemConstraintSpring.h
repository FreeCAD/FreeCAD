/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Preslav Aleksandrov <preslav.aleksandrov@protonmail.com>      *
 *   Based on Force constraint by Jan Rheinländer                          *
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


#ifndef FEM_CONSTRAINTPSPRING_H
#define FEM_CONSTRAINTPSPRING_H

#include "FemConstraint.h"

namespace Fem {

class FemExport ConstraintSpring : public Fem::Constraint
{
    PROPERTY_HEADER(Fem::ConstraintSpring);

public:
    ConstraintSpring(void);

    App::PropertyFloat normalStiffness;
    App::PropertyFloat tangentialStiffness;
    App::PropertyVectorList Points;
    App::PropertyVectorList Normals;

    /// recalculate the object
    virtual App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const;

protected:
    virtual void onChanged(const App::Property* prop);
};

}

#endif // FEM_CONSTRAINTPSPRING_H
