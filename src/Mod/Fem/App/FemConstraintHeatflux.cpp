/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
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

#include "PreCompiled.h"

#include "FemConstraintHeatflux.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintHeatflux, Fem::Constraint)

static const char* ConstraintTypes[] = {"DFlux", "Convection", "Radiation", nullptr};

ConstraintHeatflux::ConstraintHeatflux()
{
    ADD_PROPERTY_TYPE(AmbientTemp,
                      (0.0),
                      "ConstraintHeatflux",
                      App::Prop_None,
                      "Ambient temperature");
    /*ADD_PROPERTY(FaceTemp,(0.0));*/
    ADD_PROPERTY_TYPE(FilmCoef, (0.0), "ConstraintHeatflux", App::Prop_None, "Film coefficient");
    ADD_PROPERTY_TYPE(Emissivity, (0.0), "ConstraintHeatflux", App::Prop_None, "Emissivity");
    ADD_PROPERTY_TYPE(DFlux, (0.0), "ConstraintHeatflux", App::Prop_None, "Distributed heat flux");
    ADD_PROPERTY_TYPE(ConstraintType,
                      (1),
                      "ConstraintHeatflux",
                      App::Prop_None,
                      "Type of constraint, surface convection, radiation or surface heat flux");
    ConstraintType.setEnums(ConstraintTypes);
}

App::DocumentObjectExecReturn* ConstraintHeatflux::execute()
{
    return Constraint::execute();
}

const char* ConstraintHeatflux::getViewProviderName() const
{
    return "FemGui::ViewProviderFemConstraintHeatflux";
}

void ConstraintHeatflux::handleChangedPropertyType(Base::XMLReader& reader,
                                                   const char* typeName,
                                                   App::Property* prop)
{
    if (prop == &FilmCoef && strcmp(typeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat filmCoefProperty;
        filmCoefProperty.Restore(reader);
        FilmCoef.setValue(filmCoefProperty.getValue());
    }
    else if (prop == &DFlux && strcmp(typeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat dFluxProperty;
        dFluxProperty.Restore(reader);
        DFlux.setValue(dFluxProperty.getValue());
    }
    else if (prop == &AmbientTemp && strcmp(typeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat tempProperty;
        tempProperty.Restore(reader);
        AmbientTemp.setValue(tempProperty.getValue());
    }
    else {
        Constraint::handleChangedPropertyType(reader, typeName, prop);
    }
}

void ConstraintHeatflux::onChanged(const App::Property* prop)
{
    Constraint::onChanged(prop);
}
