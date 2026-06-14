# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Final


@export(
    PythonName="Material.UUIDs",
    Twin="ModelUUIDs",
    TwinPointer="ModelUUIDs",
    Include="Mod/Material/App/ModelUuids.h",
    Namespace="Materials",
    Constructor=True,
    Delete=True,
)
class UUIDs(BaseClass):
    """
    Material model UUID identifiers.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Father: Final[str] = ...
    """UUID for model System:Legacy/Father"""

    MaterialStandard: Final[str] = ...
    """UUID for model System:Legacy/MaterialStandard"""

    ArrudaBoyce: Final[str] = ...
    """UUID for model System:Mechanical/ArrudaBoyce"""

    Density: Final[str] = ...
    """UUID for model System:Mechanical/Density"""

    Hardness: Final[str] = ...
    """UUID for model System:Mechanical/Hardness"""

    IsotropicLinearElastic: Final[str] = ...
    """UUID for model System:Mechanical/IsotropicLinearElastic"""

    LinearElastic: Final[str] = ...
    """UUID for model System:Mechanical/LinearElastic"""

    Machinability: Final[str] = ...
    """UUID for model System:Machining/Machinability"""

    MooneyRivlin: Final[str] = ...
    """UUID for model System:Mechanical/MooneyRivlin"""

    NeoHooke: Final[str] = ...
    """UUID for model System:Mechanical/NeoHooke"""

    OgdenN1: Final[str] = ...
    """UUID for model System:Mechanical/OgdenN1"""

    OgdenN2: Final[str] = ...
    """UUID for model System:Mechanical/OgdenN2"""

    OgdenN3: Final[str] = ...
    """UUID for model System:Mechanical/OgdenN3"""

    OgdenYld2004p18: Final[str] = ...
    """UUID for model System:Mechanical/OgdenYld2004p18"""

    OrthotropicLinearElastic: Final[str] = ...
    """UUID for model System:Mechanical/OrthotropicLinearElastic"""

    PolynomialN1: Final[str] = ...
    """UUID for model System:Mechanical/PolynomialN1"""

    PolynomialN2: Final[str] = ...
    """UUID for model System:Mechanical/PolynomialN2"""

    PolynomialN3: Final[str] = ...
    """UUID for model System:Mechanical/PolynomialN3"""

    ReducedPolynomialN1: Final[str] = ...
    """UUID for model System:Mechanical/ReducedPolynomialN1"""

    ReducedPolynomialN2: Final[str] = ...
    """UUID for model System:Mechanical/ReducedPolynomialN2"""

    ReducedPolynomialN3: Final[str] = ...
    """UUID for model System:Mechanical/ReducedPolynomialN3"""

    Yeoh: Final[str] = ...
    """UUID for model System:Mechanical/Yeoh"""

    Fluid: Final[str] = ...
    """UUID for model System:Fluid/Fluid"""

    Thermal: Final[str] = ...
    """UUID for model System:Thermal/Thermal"""

    Electromagnetic: Final[str] = ...
    """UUID for model System:Electromagnetic/Electromagnetic"""

    Architectural: Final[str] = ...
    """UUID for model System:Architectural/Architectural"""

    ArchitecturalRendering: Final[str] = ...
    """UUID for model System:Architectural/ArchitecturalRendering"""

    Costs: Final[str] = ...
    """UUID for model System:Costs/Costs"""

    BasicRendering: Final[str] = ...
    """UUID for model System:Rendering/BasicRendering"""

    TextureRendering: Final[str] = ...
    """UUID for model System:Rendering/TextureRendering"""

    AdvancedRendering: Final[str] = ...
    """UUID for model System:Rendering/AdvancedRendering"""

    VectorRendering: Final[str] = ...
    """UUID for model System:Rendering/VectorRendering"""

    RenderAppleseed: Final[str] = ...
    """UUID for model System:Rendering/RenderAppleseed"""

    RenderCarpaint: Final[str] = ...
    """UUID for model System:Rendering/RenderCarpaint"""

    RenderCycles: Final[str] = ...
    """UUID for model System:Rendering/RenderCycles"""

    RenderDiffuse: Final[str] = ...
    """UUID for model System:Rendering/RenderDiffuse"""

    RenderDisney: Final[str] = ...
    """UUID for model System:Rendering/RenderDisney"""

    RenderEmission: Final[str] = ...
    """UUID for model System:Rendering/RenderEmission"""

    RenderGlass: Final[str] = ...
    """UUID for model System:Rendering/RenderGlass"""

    RenderLuxcore: Final[str] = ...
    """UUID for model System:Rendering/RenderLuxcore"""

    RenderLuxrender: Final[str] = ...
    """UUID for model System:Rendering/RenderLuxrender"""

    RenderMixed: Final[str] = ...
    """UUID for model System:Rendering/RenderMixed"""

    RenderOspray: Final[str] = ...
    """UUID for model System:Rendering/RenderOspray"""

    RenderPbrt: Final[str] = ...
    """UUID for model System:Rendering/RenderPbrt"""

    RenderPovray: Final[str] = ...
    """UUID for model System:Rendering/RenderPovray"""

    RenderSubstancePBR: Final[str] = ...
    """UUID for model System:Rendering/RenderSubstancePBR"""

    RenderTexture: Final[str] = ...
    """UUID for model System:Rendering/RenderTexture"""

    RenderWB: Final[str] = ...
    """UUID for model System:Rendering/RenderWB"""

    TestModel: Final[str] = ...
    """UUID for model System:Test/Test Model"""
