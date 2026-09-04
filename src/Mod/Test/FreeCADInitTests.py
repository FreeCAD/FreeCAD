# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Runtime contract tests for the App bootstrap script."""

from __future__ import annotations

from datetime import datetime
from enum import IntEnum
import sys
import unittest

import FreeCAD

Units = FreeCAD.Units

_EXPECTED_QUANTITY_CONSTANTS = frozenset("""
    NanoMetre MicroMetre MilliMetre CentiMetre DeciMetre Metre KiloMetre MilliLiter
    Liter Hertz KiloHertz MegaHertz GigaHertz TeraHertz MicroGram MilliGram Gram
    KiloGram Ton Second Minute Hour Ampere NanoAmpere MicroAmpere MilliAmpere
    KiloAmpere MegaAmpere Kelvin MilliKelvin MicroKelvin Mole NanoMole MicroMole
    MilliMole Candela Inch Foot Thou Yard Mile SquareFoot CubicFoot Pound Ounce
    Stone Hundredweights Newton MilliNewton KiloNewton MegaNewton NewtonPerMeter
    MilliNewtonPerMeter KiloNewtonPerMeter MegaNewtonPerMeter Pascal KiloPascal
    MegaPascal GigaPascal MilliBar Bar PoundForce Torr mTorr yTorr PSI KSI MPSI
    Watt NanoWatt MicroWatt MilliWatt KiloWatt VoltAmpere Volt MilliVolt KiloVolt
    MegaSiemens KiloSiemens Siemens MilliSiemens MicroSiemens Ohm KiloOhm MegaOhm
    Coulomb Tesla Gauss Weber PicoFarad NanoFarad MicroFarad MilliFarad Farad
    NanoHenry MicroHenry MilliHenry Henry Joule MilliJoule KiloJoule NewtonMeter
    VoltAmpereSecond WattSecond KiloWattHour ElectronVolt KiloElectronVolt
    MegaElectronVolt Calorie KiloCalorie MPH KMH Degree Radian Gon AngularMinute
    AngularSecond
    """.split())

_EXPECTED_UNIT_CONSTANTS = frozenset("""
    AmountOfSubstance ElectricCurrent Length LuminousIntensity Mass Temperature
    TimeSpan Acceleration Angle AngleOfFriction Area CompressiveStrength
    CurrentDensity Density DissipationRate DynamicViscosity Frequency
    MagneticFluxDensity Magnetization ElectricalCapacitance ElectricalConductance
    ElectricalConductivity ElectricalInductance ElectricalResistance ElectricCharge
    ElectricPotential Force HeatFlux InverseArea InverseLength InverseVolume
    KinematicViscosity Pressure Power ShearModulus SpecificEnergy SpecificHeat
    Stiffness Stress ThermalConductivity ThermalExpansionCoefficient
    ThermalTransferCoefficient UltimateTensileStrength Velocity VacuumPermittivity
    Volume VolumeFlowRate VolumetricThermalExpansionCoefficient Work YieldStrength
    YoungsModulus
    """.split())


class FreeCADInitContractTest(unittest.TestCase):
    """Protect observable bootstrap behavior while FreeCADInit is refactored."""

    def test_public_bootstrap_attributes(self) -> None:
        self.assertIsInstance(FreeCAD.Logger, type)
        logger = FreeCAD.Logger("FreeCADInitContractTest")
        self.assertTrue(logger.isEnabledFor("Error"))
        self.assertTrue(hasattr(logger, "info"))

        self.assertIn(FreeCAD.GuiUp, (0, 1))
        self.assertIs(FreeCAD.ScaleType.__mro__[1], IntEnum)
        self.assertIs(FreeCAD.PropertyType.__mro__[1], IntEnum)
        self.assertIs(FreeCAD.ReturnType.__mro__[1], IntEnum)
        self.assertIs(Units.Scheme.__mro__[1], IntEnum)
        self.assertIs(Units.NumberFormat.__mro__[1], IntEnum)

        self.assertEqual(
            {
                "Internal": 0,
                "MKS": 1,
                "Imperial": 2,
                "ImperialDecimal": 3,
                "Centimeter": 4,
                "ImperialBuilding": 5,
                "MmMin": 6,
                "ImperialCivil": 7,
                "FEM": 8,
                "MeterDecimal": 9,
            },
            {name: member.value for name, member in Units.Scheme.__members__.items()},
        )
        self.assertEqual(
            {"Default": 0, "Fixed": 1, "Scientific": 2},
            {name: member.value for name, member in Units.NumberFormat.__members__.items()},
        )

    def test_predefined_units_and_quantities(self) -> None:
        for name in _EXPECTED_QUANTITY_CONSTANTS:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(Units, name), Units.Quantity)

        for name in _EXPECTED_UNIT_CONSTANTS:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(Units, name), Units.Unit)

        self.assertFalse(hasattr(Units, "Oersted"))

    def test_startup_paths_and_discovery_attributes(self) -> None:
        self.assertIsInstance(FreeCAD.__cmake__, list)
        self.assertIsInstance(FreeCAD.__ModDirs__, list)
        self.assertTrue(all(isinstance(path, str) for path in FreeCAD.__ModDirs__))
        self.assertIsInstance(FreeCAD.__ModCache__, list)
        self.assertIsInstance(FreeCAD.__MacroDirs__, list)
        self.assertTrue(hasattr(FreeCAD, "__path__"))
        self.assertTrue(all(isinstance(path, str) for path in FreeCAD.__path__))

    def test_intentional_bootstrap_leaks_remain_available(self) -> None:
        main = sys.modules["__main__"]
        self.assertIs(getattr(main, "IntEnum"), IntEnum)
        self.assertIs(getattr(main, "datetime"), datetime)


if __name__ == "__main__":
    unittest.main()
