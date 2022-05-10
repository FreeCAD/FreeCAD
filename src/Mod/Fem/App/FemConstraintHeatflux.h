/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
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


#ifndef FEM_CONSTRAINTHEATFLUX_H
#define FEM_CONSTRAINTHEATFLUX_H

#include "FemConstraint.h"

namespace Fem {

class FemExport ConstraintHeatflux : public Fem::Constraint
{
    PROPERTY_HEADER(Fem::ConstraintHeatflux);

public:
    ConstraintHeatflux(void);

    App::PropertyFloat AmbientTemp;
    /*App::PropertyFloat FaceTemp;*/
    App::PropertyFloat FilmCoef;
    App::PropertyFloat DFlux;
    App::PropertyEnumeration ConstraintType;

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

#endif // FEM_CONSTRAINTHEATFLUX_H
