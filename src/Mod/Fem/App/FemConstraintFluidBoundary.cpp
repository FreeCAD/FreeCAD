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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Lin.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Precision.hxx>
#endif

#include "FemConstraintFluidBoundary.h"

#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintFluidBoundary, Fem::Constraint);

// also defined in TaskFemConstraintFluidBoundary.cpp and foamcasebuilder/basicbuilder.py, 
// please update simultaneously
// the second (index 1) is the default enum, as index 0 causes compiling error
static const char* BoundaryTypes[] = {"inlet","wall","outlet","interface","freestream", NULL};
static const char* WallSubtypes[] = {"unspecific", "fixed", "slip", "moving", NULL};
static const char* InletSubtypes[] = {"unspecific","totalPressure","uniformVelocity","volumetricFlowRate","massFlowRate", NULL};
static const char* OutletSubtypes[] = {"unspecific","totalPressure","staticPressure","uniformVelocity", "outFlow", NULL};
static const char* InterfaceSubtypes[] = {"unspecific","symmetry","wedge","cyclic","empty", NULL};
static const char* FreestreamSubtypes[] = {"unspecific", "freestream",NULL};

// see Ansys fluet manual: Turbulence Specification method
static const char* TurbulenceSpecifications[] = {"Intensity&LengthScale","Intensity&HydraulicDiameter",NULL};
// activate the heat transfer and radiation model in Solver object explorer
/* only used in TaskPanel
static const char* TurbulenceSpecificationHelpTexts[] = {"see Ansys fluet manual: Turbulence Specification method", 
            "or fully devloped internal flow, Turbulence intensity (0-1.0) 0.05 typical", NULL};
*/

//HTC value type, not sure it is supported in OpenFOAM
static const char* ThermalBoundaryTypes[] = {"fixedValue","zeroGradient", "fixedGradient", "mixed",  "HTC","coupled", NULL};
/* only used in TaskPanel
static const char* ThermalBoundaryHelpTexts[] = {"fixed Temperature [K]", "no heat transfer ()", "fixed value heat flux [W/m2]", 
            "mixed fixedGradient and fixedValue", "Heat transfer coeff [W/(M2)/K]", "conjugate heat transfer with solid", NULL};
*/

ConstraintFluidBoundary::ConstraintFluidBoundary()
{
    // momemtum boundary: pressure and velocity
    ADD_PROPERTY_TYPE(BoundaryType,(1),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Basic boundary type like inlet, wall, outlet,etc");
    BoundaryType.setEnums(BoundaryTypes);
    ADD_PROPERTY_TYPE(Subtype,(1),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Subtype defines value type or more specific type");
    Subtype.setEnums(WallSubtypes);
    ADD_PROPERTY_TYPE(BoundaryValue,(0.0),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Scaler value for the specific value subtype, like pressure, velocity");
    ADD_PROPERTY_TYPE(Direction,(0),"FluidBoundary",(App::PropertyType)(App::Prop_None),
                      "Element giving vector direction of constraint");
    
    ADD_PROPERTY_TYPE(TurbulenceSpecification,(1),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Turbulence boundary type");
    TurbulenceSpecification.setEnums(TurbulenceSpecifications); //Turbulence Specification Method
    ADD_PROPERTY_TYPE(TurbulentIntensityValue,(0.0),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Scaler value for Turbulent intensity etc");
    ADD_PROPERTY_TYPE(TurbulentLengthValue,(0.0),"Turbulence",(App::PropertyType)(App::Prop_None),
                      "Scaler value for Turbulent length scale, hydraulic diameter etc");
    // consider the newly added Fem::ConstraintTemperature 
    ADD_PROPERTY_TYPE(ThermalBoundaryType,(1),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Thermal boundary type");
    ThermalBoundaryType.setEnums(ThermalBoundaryTypes);
    ADD_PROPERTY_TYPE(TemperatureValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Temperature value for thermal boundary condition");
    ADD_PROPERTY_TYPE(HeatFluxValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Heat flux value for thermal boundary condition");
    ADD_PROPERTY_TYPE(HTCoeffValue,(0.0),"HeatTransfer",(App::PropertyType)(App::Prop_None),
                      "Heat transfer coefficient for convective boundary condition");
    // geometry rendering related properties
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY_TYPE(Points,(Base::Vector3d()),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Points where arrows are drawn");
    ADD_PROPERTY_TYPE(DirectionVector,(Base::Vector3d(0,0,1)),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Direction of arrows");
    naturalDirectionVector = Base::Vector3d(0,0,0); // by default use the null vector to indication an invalid value
    Points.setValues(std::vector<Base::Vector3d>());
    // property from: FemConstraintFixed object
    ADD_PROPERTY_TYPE(Normals,(Base::Vector3d()),"FluidBoundary",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Normals where symbols are drawn");
    Normals.setValues(std::vector<Base::Vector3d>());
}

App::DocumentObjectExecReturn *ConstraintFluidBoundary::execute(void)
{
    return Constraint::execute();
}

void ConstraintFluidBoundary::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);
    
    if (prop == &BoundaryType)
    {
        std::string boundaryType = BoundaryType.getValueAsString();
        if (boundaryType == "wall")
        {
            Subtype.setEnums(WallSubtypes);
        }
        else if (boundaryType == "interface")
        {
            Subtype.setEnums(InterfaceSubtypes);
        }
        else if (boundaryType == "freestream")
        {
            Subtype.setEnums(FreestreamSubtypes);
        }
        else if(boundaryType == "inlet")
        {
            Subtype.setEnums(InletSubtypes);
        }
        else if(boundaryType == "outlet")
        {
            Subtype.setEnums(OutletSubtypes);
        }
        else
        {
            Base::Console().Message(boundaryType.c_str());
            Base::Console().Message(" Error: this boundaryType is not defined\n");
        }
        //need to trigger ViewProvider::updateData() for redraw in 3D view
    }

    if (prop == &References) {
        std::vector<Base::Vector3d> points;
        std::vector<Base::Vector3d> normals;
        int scale = 1; //OvG: Enforce use of scale
        if (getPoints(points, normals, &scale)) {
            Points.setValues(points);
            Normals.setValues(normals);
            Scale.setValue(scale); //OvG: Scale
            Points.touch(); // This triggers ViewProvider::updateData()
        }
    } else if (prop == &Direction) {
        Base::Vector3d direction = getDirection(Direction);
        if (direction.Length() < Precision::Confusion())
            return;
        naturalDirectionVector = direction;
        if (Reversed.getValue())
            direction = -direction;
        DirectionVector.setValue(direction);
    } else if (prop == &Reversed) {
        // if the direction is invalid try to compute it again
        if (naturalDirectionVector.Length() < Precision::Confusion()) {
            naturalDirectionVector = getDirection(Direction);
        }
        if (naturalDirectionVector.Length() >= Precision::Confusion()) {
            if (Reversed.getValue() && (DirectionVector.getValue() == naturalDirectionVector)) {
                DirectionVector.setValue(-naturalDirectionVector);
            } else if (!Reversed.getValue() && (DirectionVector.getValue() != naturalDirectionVector)) {
                DirectionVector.setValue(naturalDirectionVector);
            }
        }
    } else if (prop == &NormalDirection) {
        // Set a default direction if no direction reference has been given
        if (Direction.getValue() == NULL) {
            Base::Vector3d direction = NormalDirection.getValue();
            if (Reversed.getValue())
                direction = -direction;
            DirectionVector.setValue(direction);
            naturalDirectionVector = direction;
        }
    }
}
