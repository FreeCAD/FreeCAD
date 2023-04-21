# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM Heat Elmer writer"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from .. import sifio
from .. import writer as general_writer
from femtools import membertools
from femmesh import meshtools
from . import heat


class Heatwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getHeatSolver(self, equation):
        # check if we need to update the equation
        self._updateHeatSolver(equation)
        # output the equation parameters
        s = self.write.createNonlinearSolver(equation)
        s["Equation"] = equation.Name
        s["Procedure"] = sifio.FileAttr("HeatSolve/HeatSolver")
        s["Bubbles"] = equation.Bubbles
        s["Exec Solver"] = "Always"
        s["Optimize Bandwidth"] = True
        s["Stabilize"] = equation.Stabilize
        s["Variable"] = self.write.getUniqueVarName("Temperature")
        return s

    def handleHeatConstants(self):
        self.write.constant(
            "Stefan Boltzmann",
            self.write.convert(self.write.constsdef["StefanBoltzmann"], "M/(O^4*T^3)")
        )

    def handleHeatEquation(self, bodies, equation):
        for b in bodies:
            if equation.Convection != "None":
                self.write.equation(b, "Convection", equation.Convection)
            if equation.PhaseChangeModel != "None":
                self.write.equation(b, "Phase Change Model", equation.PhaseChangeModel)

    def _updateHeatSolver(self, equation):
        # updates older Heat equations
        if not hasattr(equation, "Convection"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "Convection",
                "Equation",
                "Type of convection to be used"
            )
            equation.Convection = heat.CONVECTION_TYPE
            equation.Convection = "None"
        if not hasattr(equation, "PhaseChangeModel"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "PhaseChangeModel",
                "Equation",
                "Model for phase change"
            )
            equation.PhaseChangeModel = heat.PHASE_CHANGE_MODEL
            equation.PhaseChangeModel = "None"

    def handleHeatBndConditions(self):
        i = -1
        for obj in self.write.getMember("Fem::ConstraintTemperature"):
            i = i + 1
            femobjects = membertools.get_several_member(
                self.write.analysis,
                "Fem::ConstraintTemperature"
            )
            femobjects[i]["Nodes"] = meshtools.get_femnodes_by_femobj_with_references(
                self.write.getSingleMember("Fem::FemMeshObject").FemMesh,
                femobjects[i]
            )
            NumberOfNodes = len(femobjects[i]["Nodes"])
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Temperature":
                        temperature = float(obj.Temperature.getValueAs("K"))
                        self.write.boundary(name, "Temperature", temperature)
                    elif obj.ConstraintType == "CFlux":
                        flux = float(obj.CFlux.getValueAs("W"))
                        # CFLUX is the flux per mesh node
                        flux = flux / NumberOfNodes
                        self.write.boundary(name, "Temperature Load", flux)
                self.write.handled(obj)
        for obj in self.write.getMember("Fem::ConstraintHeatflux"):
            if obj.References:
                for name in obj.References[0][1]:
                    if obj.ConstraintType == "Convection":
                        film = self.write.getFromUi(obj.FilmCoef, "W/(m^2*K)", "M/(T^3*O)")
                        temp = self.write.getFromUi(obj.AmbientTemp, "K", "O")
                        self.write.boundary(name, "Heat Transfer Coefficient", film)
                        self.write.boundary(name, "External Temperature", temp)
                    elif obj.ConstraintType == "DFlux":
                        flux = self.write.getFromUi(obj.DFlux, "W/m^2", "M*T^-3")
                        self.write.boundary(name, "Heat Flux BC", True)
                        self.write.boundary(name, "Heat Flux", flux)
                self.write.handled(obj)

    def handleHeatInitial(self, bodies):
        tempObj = self.write.getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = float(tempObj.initialTemperature.getValueAs("K"))
            for name in bodies:
                self.write.initial(name, "Temperature", refTemp)
            self.write.handled(tempObj)

    def _outputHeatBodyForce(self, obj, name):
        heatSource = self.write.getFromUi(obj.HeatSource, "W/kg", "L^2*T^-3")
        if heatSource == 0.0:
            # a zero heat would break Elmer (division by zero)
            raise general_writer.WriteError("The body heat source must not be zero!")
        self.write.bodyForce(name, "Heat Source", heatSource)

    def handleHeatBodyForces(self, bodies):
        bodyHeats = self.write.getMember("Fem::ConstraintBodyHeatSource")
        for obj in bodyHeats:
            if obj.References:
                for name in obj.References[0][1]:
                    self._outputHeatBodyForce(obj, name)
                self.write.handled(obj)
            else:
                # if there is only one body heat without a reference
                # add it to all bodies
                if len(bodyHeats) == 1:
                    for name in bodies:
                        self._outputHeatBodyForce(obj, name)
                else:
                    raise general_writer.WriteError(
                        "Several body heat constraints found without reference to a body.\n"
                        "Please set a body for each body heat constraint."
                    )
            self.write.handled(obj)

    def handleHeatMaterial(self, bodies):
        tempObj = self.write.getSingleMember("Fem::ConstraintInitialTemperature")
        if tempObj is not None:
            refTemp = float(tempObj.initialTemperature.getValueAs("K"))
            for name in bodies:
                self.write.material(name, "Reference Temperature", refTemp)
        for obj in self.write.getMember("App::MaterialObject"):
            m = obj.Material
            refs = (
                obj.References[0][1]
                if obj.References
                else self.write.getAllBodies())
            for name in (n for n in refs if n in bodies):
                if "Density" not in m:
                    raise general_writer.WriteError(
                        "Used material does not specify the necessary 'Density'."
                    )
                self.write.material(name, "Name", m["Name"])
                self.write.material(
                    name, "Density",
                    self.write.getDensity(m))
                if "ThermalConductivity" not in m:
                    raise general_writer.WriteError(
                        "Used material does not specify the necessary 'Thermal Conductivity'."
                    )
                self.write.material(
                    name, "Heat Conductivity",
                    self.write.convert(m["ThermalConductivity"], "M*L/(T^3*O)"))
                if "SpecificHeat" not in m:
                    raise general_writer.WriteError(
                        "Used material does not specify the necessary 'Specific Heat'."
                    )
                self.write.material(
                    name, "Heat Capacity",
                    self.write.convert(m["SpecificHeat"], "L^2/(T^2*O)"))

##  @}
