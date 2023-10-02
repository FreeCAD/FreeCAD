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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Precision.hxx>
#endif

#include <Base/Console.h>

#include "FemConstraintFluidBoundary.h"


using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintFluidBoundary, Fem::Constraint)

// clang-format off
// see forum topic: https://forum.freecad.org/viewtopic.php?&p=407901

// also defined in TaskFemConstraintFluidBoundary.cpp and FoamCaseBuilder/BasicBuilder.py, update simultaneously
// the second (index 1) item is the default enum, as index 0 causes compiling error
static const char* BoundaryTypes[] = {"inlet","wall","outlet","interface","freestream", nullptr};
static const char* WallSubtypes[] = {"unspecific", "fixed", "slip", "partialSlip", "moving", nullptr};
static const char* InletSubtypes[] = {"unspecific","totalPressure","uniformVelocity","volumetricFlowRate","massFlowRate", nullptr};
static const char* OutletSubtypes[] = {"unspecific","totalPressure","staticPressure","uniformVelocity", "outFlow", nullptr};
static const char* InterfaceSubtypes[] = {"unspecific","symmetry","wedge","cyclic","empty", nullptr};
static const char* FreestreamSubtypes[] = {"unspecific", "freestream",nullptr};

// see Ansys fluet manual: Turbulence Specification method, if not specified, solver will guess a value based e.g. 0.05 for inlet length geometry",
static const char* TurbulenceSpecifications[] = {"intensity&DissipationRate", "intensity&LengthScale","intensity&ViscosityRatio","intensity&HydraulicDiameter",nullptr};
/* only used in TaskFemConstraintFluidBoundary.cpp */

// activate the heat transfer and radiation model in Solver object explorer
// also defined in FoamCaseBuilder/HeatTransferBuilder.py, update simultaneously, heatFlux is not a standard OpenFOAM patch type
static const char* ThermalBoundaryTypes[] = {"fixedValue","zeroGradient", "fixedGradient", "mixed", "heatFlux", "HTC","coupled", nullptr};
/* only used in TaskFemConstraintFluidBoundary.cpp
static const char* ThermalBoundaryHelpTexts[] = {"fixed Temperature [K]", "no heat transfer ()", "fixed value heat flux [K/m]",
            "mixed fixedGradient and fixedValue", "fixed heat flux [W/m2]", "Heat transfer coeff [W/(M2)/K]", "conjugate heat transfer with solid", NULL};
*/
// clang-format on

ConstraintFluidBoundary::ConstraintFluidBoundary()
{
    // clang-format off
    /// momentum boundary: pressure and velocity
    ADD_PROPERTY_TYPE(BoundaryType,(1),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Basic boundary type like inlet, wall, outlet,etc");
    BoundaryType.setEnums(BoundaryTypes);
    ADD_PROPERTY_TYPE(Subtype,(1),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Subtype defines more specific boundary types");
    Subtype.setEnums(WallSubtypes);
    ADD_PROPERTY_TYPE(BoundaryValue,(0.0),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Scaler value for the specific value subtype, like pressure, velocity magnitude");
    /// Direction should be allowed to edit in property editor, if no edge is available in CAD model
    ADD_PROPERTY_TYPE(Direction,(nullptr),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Vector direction of BoundaryValue");
    ADD_PROPERTY_TYPE(Reversed,(0),"FluidBoundary",(App::PropertyType)(App::Prop_ReadOnly|App::Prop_Output),
                      "To distinguish inlet (flow outward from solid) or outlet boundary condition");
    /// turbulence model setup for boundary
    ADD_PROPERTY_TYPE(TurbulenceSpecification,(1),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Method to specify burbulence magnitude on the boundary");
    TurbulenceSpecification.setEnums(TurbulenceSpecifications); // Turbulence Specification Method
    ADD_PROPERTY_TYPE(TurbulentIntensityValue,(0.0),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Scaler value for Turbulent intensity etc");
    ADD_PROPERTY_TYPE(TurbulentLengthValue,(0.0),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Scaler value for Turbulent length scale, hydraulic diameter etc");
    /// consider using the newly added Fem::ConstraintTemperature, but it is too hard to export the settings
    ADD_PROPERTY_TYPE(ThermalBoundaryType,(1),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Thermal boundary type");
    ThermalBoundaryType.setEnums(ThermalBoundaryTypes);
    ADD_PROPERTY_TYPE(TemperatureValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Temperature value for thermal boundary condition");
    ADD_PROPERTY_TYPE(HeatFluxValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Heat flux value for thermal boundary condition");
    ADD_PROPERTY_TYPE(HTCoeffValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Heat transfer coefficient for convective boundary condition");
    /// geometry rendering related properties
    ADD_PROPERTY_TYPE(Points,(Base::Vector3d()),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Points where arrows are drawn");
    Points.setValues(std::vector<Base::Vector3d>());
    ADD_PROPERTY_TYPE(DirectionVector,(Base::Vector3d(0,0,1)),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Direction of arrows");
    naturalDirectionVector = Base::Vector3d(0,0,0); // by default use the null vector to indicate an invalid value
    // property from: FemConstraintFixed object
    ADD_PROPERTY_TYPE(Normals,(Base::Vector3d()),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Normals where symbols are drawn");
    Normals.setValues(std::vector<Base::Vector3d>());
    // clang-format on
}

App::DocumentObjectExecReturn* ConstraintFluidBoundary::execute()
{
    return Constraint::execute();
}

void ConstraintFluidBoundary::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);

    if (prop == &BoundaryType) {
        std::string boundaryType = BoundaryType.getValueAsString();
        if (boundaryType == "wall") {
            Subtype.setEnums(WallSubtypes);
        }
        else if (boundaryType == "interface") {
            Subtype.setEnums(InterfaceSubtypes);
        }
        else if (boundaryType == "freestream") {
            Subtype.setEnums(FreestreamSubtypes);
        }
        else if (boundaryType == "inlet") {
            Subtype.setEnums(InletSubtypes);
        }
        else if (boundaryType == "outlet") {
            Subtype.setEnums(OutletSubtypes);
        }
        else {
            Base::Console().Message(boundaryType.c_str());
            Base::Console().Message(" Error: this boundaryType is not defined\n");
        }

        // must set a default (0 or 1) as freestream has only 2 subtypes
        Subtype.setValue(1);
        // need to trigger ViewProvider::updateData() for redraw in 3D view after this method
    }

    // naturalDirectionVector is a private member of this class
    if (prop == &References) {
        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        int scale = 1;  // OvG: Enforce use of scale
        if (getPoints(points, normals, &scale)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Scale.setValue(scale);  // OvG: Scale
            Points.touch();         // This triggers ViewProvider::updateData()
        }
    }
    else if (prop == &Direction) {
        Base::Vector3d direction = getDirection(Direction);
        // if Direct has no link provided return Base::Vector3d(0,0,0);
        if (direction.Length() < Precision::Confusion()) {
            return;
        }
        naturalDirectionVector = direction;
        if (Reversed.getValue()) {
            direction = -direction;
        }
        DirectionVector.setValue(direction);
    }
    else if (prop == &Reversed) {
        // if the direction is invalid try to compute it again
        if (naturalDirectionVector.Length() < Precision::Confusion()) {
            naturalDirectionVector = getDirection(Direction);
        }
        if (naturalDirectionVector.Length() >= Precision::Confusion()) {
            if (Reversed.getValue() && (DirectionVector.getValue() == naturalDirectionVector)) {
                DirectionVector.setValue(-naturalDirectionVector);
            }
            else if (!Reversed.getValue()
                     && (DirectionVector.getValue() != naturalDirectionVector)) {
                DirectionVector.setValue(naturalDirectionVector);
            }
        }
    }
    else if (prop == &NormalDirection) {
        // Set a default direction if no direction reference has been given
        if (!Direction.getValue()) {
            Base::Vector3d direction = NormalDirection.getValue();
            if (Reversed.getValue()) {
                direction = -direction;
            }
            DirectionVector.setValue(direction);
            naturalDirectionVector = direction;
        }
    }
}
