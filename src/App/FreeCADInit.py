# SPDX-License-Identifier: LGPL-2.1-or-later

#***************************************************************************
#*   Copyright (c) 2001,2002 Jürgen Riegel <juergen.riegel@web.de>         *
#*   Copyright (c) 2025 Frank Martínez <mnesarco at gmail dot com>         *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software  you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation  either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY  without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD  if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# FreeCAD init module - App
#
# Gathering all the information to start FreeCAD.
# This is the second of four init scripts:
# +------+------------------+-----------------------------+
# | This | Script           | Runs                        |
# +------+------------------+-----------------------------+
# |      | CMakeVariables   | always                      |
# | >>>> | FreeCADInit      | always                      |
# |      | FreeCADTest      | only if test and not Gui    |
# |      | FreeCADGuiInit   | only if Gui is up           |
# +------+------------------+-----------------------------+

import FreeCAD

App = FreeCAD

App.Console.PrintLog("Init: starting App::FreeCADInit.py\n")
App.Console.PrintLog("░░░▀█▀░█▀█░▀█▀░▀█▀░░░█▀█░█▀█░█▀█░░\n")
App.Console.PrintLog("░░░░█░░█░█░░█░░░█░░░░█▀█░█▀▀░█▀▀░░\n")
App.Console.PrintLog("░░░▀▀▀░▀░▀░▀▀▀░░▀░░░░▀░▀░▀░░░▀░░░░\n")

try:
    import sys
    import os
    import traceback
    import inspect
    from collections.abc import Callable
    from enum import IntEnum  # Leak to globals (backwards compat)
    from datetime import datetime  # Leak to globals (backwards compat)
    from pathlib import Path  # Removed manually
    import dataclasses
    import collections
    import collections.abc as coll_abc
    import platform
    import types
    import importlib.resources as resources
    import importlib
    import functools
    import re
    import pkgutil
except ImportError:
    App.Console.PrintError("\n\nSeems the python standard libs are not installed, bailing out!\n\n")
    raise

# ┌────────────────────────────────────────────────┐
# │ Logging Frameworks                             │
# └────────────────────────────────────────────────┘

def __logger(fn):
    __logger.sep = "\n"
    def wrapper(text: object, *, sep: str | None = None) -> None:
        fn(f"{text!s}{__logger.sep if sep is None else sep}")
    return wrapper

Log = __logger(App.Console.PrintLog)
Msg = __logger(App.Console.PrintMessage)
Err = __logger(App.Console.PrintError)
Wrn = __logger(App.Console.PrintWarning)
Crt = __logger(App.Console.PrintCritical)
Ntf = __logger(App.Console.PrintNotification)
Tnf = __logger(App.Console.PrintTranslatedNotification)


class FCADLogger:
    """
    Convenient class for tagged logging.

    Example usage:
        >>> logger = FreeCAD.Logger('MyModule')
        >>> logger.info('log test {}',1)
        24.36053 <MyModule> <input>(1): test log 1

    The default output format is:
        <timestamp> <tag> <source file>(line number): message

    The message is formatted using new style Python string formatting, e.g.
    'test {}'.format(1). It is strongly recommended to not directly use
    Python string formatting, but pass additional argument indirectly through
    various logger print function, because the logger can skip string
    evaluation in case the logging level is disabled. For more options,
    please consult the docstring of __init__(), catch() and report().

    To set/get logger level:
        >>> FreeCAD.setLogLevel('MyModule','Trace')
        >>> FreeCAD.getLogLevel('MyModule')
        4

    There are five predefined logger level, each corresponding to an integer
    value, as shown below together with the corresponding logger print
    method,
        0: Error, Logger.error()
        1: Warning, Logger.warn()
        2: Message, Logger.msg() or info()
        3: Log, Logger.log() or debug()
        4: Trace, Logger.trace()

    FreeCAD.setLogLevel() supports both text and integer value, which allows
    you to define your own levels. The level set is persisted to user
    configuration file.

    By default any tag has a log level of 2 for release, and 3 for debug
    build.
    """

    _levels = {
        'Error': 0,
        'error': 0,
        'Warning': 1,
        'warn': 1,
        'Message': 2,
        'msg': 2,
        'info': 2,
        'Log': 3,
        'log': 3,
        'debug': 3,
        'Trace': 4,
        'trace': 4,
    }

    _printer = (
        App.Console.PrintError,
        App.Console.PrintWarning,
        App.Console.PrintMessage,
        App.Console.PrintLog,
        App.Console.PrintLog
    )

    _defaults = (
        ('printTag', True),
        ('noUpdateUI', True),
        ('timing', True),
        ('lineno', True),
        ('parent', None),
        ('title', 'FreeCAD'),
    )

    printTag: bool
    noUpdateUI: bool
    timing: bool
    lineno: bool
    parent: "FCADLogger | None"
    title: str
    tag: str
    laststamp: datetime

    def __init__(self, tag: str, **kwargs: object) -> None:
        """
        Construct a logger instance.

        Supported arguments are their default values are,

        * tag: a string tag for this logger. The log level of this logger can be
               accessed using FreeCAD.getLogLevel(tag)/setLogLevel(tag,level).
               All logger instance with the same tag shares the same level
               setting.

        * printTag (True): whether to print tag

        * noUpdateUI (True): whether to update GUI when printing. This is useful
                             to show log output on lengthy operations. Be
                             careful though, this may allow unexpected user
                             interaction when the application is busy, which may
                             lead to crash

        * timing (True): whether to print time stamp

        * lineno (True): whether to print source file and line number

        * parent (None): provide a parent logger, so that the log printing will
                         check for parent's log level in addition of its own

        * title ('FreeCAD'): message box title used by report()
        """
        self.tag = tag
        self.laststamp = datetime.now()
        for key, default in self._defaults:
            setattr(self, key, kwargs.get(key, default))

    def _isEnabledFor(self, level: int) -> bool:
        """
        Internal function to check for an integer log level.

        * level: integer log level
        """
        if self.parent and not self.parent._isEnabledFor(level):
            return False
        return App.getLogLevel(self.tag) >= level

    def isEnabledFor(self, level: int | str) -> bool:
        """
        To check for an integer or text log level.

        * level: integer or text log level
        """
        if not isinstance(level, int):
            level = self.__class__._levels[level]
        return self._isEnabledFor(level)

    def _logger_method(name: str, level: int, level_name: str):  # pylint: disable=no-self-argument
        """
        Create level logger.
        """
        docstring = f"""
            "{level_name}" level log printer

            * msg: message string. May contain new style Python string formatter.

            This function accepts additional positional and keyword arguments,
            which are forward to string.format() to generate the logging
            message. It is strongly recommended to not directly use Python
            string formatting, but pass additional arguments here, because the
            printer can skip string evaluation in case the logging level is
            disabled.
            """

        def log_fn(self, msg: str, *args, **kwargs) -> None:
            if self._isEnabledFor(level):
                frame = kwargs.pop('frame', 0) + 1
                self._log(level, msg, frame, args, kwargs)

        log_fn.__doc__ = docstring
        log_fn.__name__ = name
        return log_fn

    def _log(
            self,
            level: int,
            msg: str,
            frame: int = 0,
            args: tuple = (),
            kwargs: dict | None = None,
        ) -> None:
        """
        Internal log printing function.

        * level: integer log level

        * msg: message, may contain new style string format specifier

        * frame (0): the calling frame for printing source file and line
                        number.  For example, in case you have your own logging
                        function, and you want to show the callers source
                        location, then set frame to one.

        * args: tuple for positional arguments to be passed to
                string.format()

        * kwargs: dictionary for keyword arguments to be passed to
                    string.format()
        """

        if (args or kwargs) and isinstance(msg, str):
            if not kwargs:
                msg = msg.format(*args)
            else:
                msg = msg.format(*args, **kwargs)

        prefix = ''

        if self.timing:
            now = datetime.now()
            prefix += '{} '.format((now-self.laststamp).total_seconds())
            self.laststamp = now

        if self.printTag:
            prefix += '<{}> '.format(self.tag)

        if self.lineno:
            try:
                frame = sys._getframe(frame+1)
                prefix += '{}({}): '.format(os.path.basename(
                    frame.f_code.co_filename),frame.f_lineno)
            except Exception:
                frame = inspect.stack()[frame+1]
                prefix += '{}({}): '.format(os.path.basename(frame[1]),frame[2])

        self.__class__._printer[level]('{}{}\n'.format(prefix,msg))

        if not self.noUpdateUI and App.GuiUp:
            import FreeCADGui
            try:
                FreeCADGui.updateGui()
            except Exception:
                pass

    def _catch_logger_method(name: str, level: int, level_name: str):  # pylint: disable=no-self-argument
        """
        Create level catch logger.
        """
        docstring = f"""
            Catch any exception from a function and print as "{level_name}".

            * msg: message string. Unlike log printer, this argument must not
                    contain any string formatter.

            * func: a callable object

            * args: tuple of positional arguments to be passed to func.

            * kwargs: dictionary of keyword arguments to be passed to func.
            """

        def catch_fn(self, msg: str, func: Callable[..., object], *args, **kwargs) -> object | None:
            return self._catch(level, msg, func, args, kwargs)

        catch_fn.__doc__ = docstring
        catch_fn.__name__ = name
        return catch_fn

    def _catch(
            self,
            level: int,
            msg: str,
            func: Callable[..., object],
            args: tuple = (),
            kwargs: dict | None = None,
        ) -> object | None:
        """
        Internal function to log exception of any callable.

        * level: integer log level

        * msg: message string. Unlike _log(), this argument must not contain
                any string formatter.

        * func: a callable object

        * args: tuple of positional arguments to be passed to func.

        * kwargs: dictionary of keyword arguments to be passed to func.
        """
        try:
            if not kwargs:
                kwargs = {}
            return func(*args, **kwargs)
        except Exception:
            if self._isEnabledFor(level):
                self._log(level, f"{msg}\n{traceback.format_exc()}", frame=2)
        return None

    def report(
        self,
        msg: str,
        func: Callable[..., object],
        *args: object,
        **kwargs: object,
    ) -> object | None:
        """
        Catch any exception report it with a message box.

        * msg: message string. Unlike log printer, this argument must not
                contain any string formatter.

        * func: a callable object

        * args: tuple of positional arguments to be passed to func.

        * kwargs: dictionary of keyword arguments to be passed to func.
        """
        try:
            return func(*args, **kwargs)
        except Exception as e:
            self.error(f"{msg}\n{traceback.format_exc()}", frame=1)
            if App.GuiUp:
                import FreeCADGui, PySide
                PySide.QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    self.title,
                    str(e),
                )
        return None

    error = _logger_method("error", 0, "Error")
    warn = _logger_method("warn", 1, "Warning")
    msg = _logger_method("msg", 2, "Message")
    log = _logger_method("log", 3, "Log")
    trace = _logger_method("trace", 4, "Trace")
    info = msg
    debug = log

    catch = _catch_logger_method("catch", 0, "Error")
    catchWarn = _catch_logger_method("catchWarn", 1, "Warning")
    catchMsg = _catch_logger_method("catchMsg", 2, "Message")
    catchLog = _catch_logger_method("catchLog", 3, "Log")
    catchTrace = _catch_logger_method("catchTrace", 4, "Trace")
    catchInfo = catchMsg
    catchDebug = catchLog


App.Logger = FCADLogger


# ┌────────────────────────────────────────────────┐
# │ App definitions                                │
# └────────────────────────────────────────────────┘

# store the cmake variables
# This data comes from generated file src/App/CMakeScript.h and it is
# injected into globals in a previous stage.
App.__cmake__ = globals().get("cmake", [])

# store unit test names
App.__unit_test__ = []

App.addImportType("FreeCAD document (*.FCStd)", "FreeCAD")

# set to no gui, is overwritten by InitGui
App.GuiUp = 0

# Structured declarations for the Python-defined FreeCAD Units API.

@dataclasses.dataclass(frozen=True)
class QuantityConstant:
    """A named quantity constant installed on ``FreeCAD.Units``."""

    expression: str
    doc: str = ""


@dataclasses.dataclass(frozen=True)
class UnitConstant:
    """A named dimensional unit constant installed on ``FreeCAD.Units``."""

    signature: tuple[int, ...]
    doc: str = ""


QUANTITY_CONSTANTS: tuple[tuple[str, QuantityConstant], ...] = (
    ("NanoMetre", QuantityConstant("nm")),
    ("MicroMetre", QuantityConstant("um")),
    ("MilliMetre", QuantityConstant("mm")),
    ("CentiMetre", QuantityConstant("cm")),
    ("DeciMetre", QuantityConstant("dm")),
    ("Metre", QuantityConstant("m")),
    ("KiloMetre", QuantityConstant("km")),
    ("MilliLiter", QuantityConstant("ml")),
    ("Liter", QuantityConstant("l")),
    ("Hertz", QuantityConstant("Hz")),
    ("KiloHertz", QuantityConstant("kHz")),
    ("MegaHertz", QuantityConstant("MHz")),
    ("GigaHertz", QuantityConstant("GHz")),
    ("TeraHertz", QuantityConstant("THz")),
    ("MicroGram", QuantityConstant("ug")),
    ("MilliGram", QuantityConstant("mg")),
    ("Gram", QuantityConstant("g")),
    ("KiloGram", QuantityConstant("kg")),
    ("Ton", QuantityConstant("t")),
    ("Second", QuantityConstant("s")),
    ("Minute", QuantityConstant("min")),
    ("Hour", QuantityConstant("h")),
    ("Ampere", QuantityConstant("A")),
    ("NanoAmpere", QuantityConstant("nA")),
    ("MicroAmpere", QuantityConstant("uA")),
    ("MilliAmpere", QuantityConstant("mA")),
    ("KiloAmpere", QuantityConstant("kA")),
    ("MegaAmpere", QuantityConstant("MA")),
    ("Kelvin", QuantityConstant("K")),
    ("MilliKelvin", QuantityConstant("mK")),
    ("MicroKelvin", QuantityConstant("uK")),
    ("Mole", QuantityConstant("mol")),
    ("NanoMole", QuantityConstant("nmol")),
    ("MicroMole", QuantityConstant("umol")),
    ("MilliMole", QuantityConstant("mmol")),
    ("Candela", QuantityConstant("cd")),
    ("Inch", QuantityConstant("in")),
    ("Foot", QuantityConstant("ft")),
    ("Thou", QuantityConstant("thou")),
    ("Yard", QuantityConstant("yd")),
    ("Mile", QuantityConstant("mi")),
    ("SquareFoot", QuantityConstant("sqft")),
    ("CubicFoot", QuantityConstant("cft")),
    ("Pound", QuantityConstant("lb")),
    ("Ounce", QuantityConstant("oz")),
    ("Stone", QuantityConstant("st")),
    ("Hundredweights", QuantityConstant("cwt")),
    ("Newton", QuantityConstant("N")),
    ("MilliNewton", QuantityConstant("mN")),
    ("KiloNewton", QuantityConstant("kN")),
    ("MegaNewton", QuantityConstant("MN")),
    ("NewtonPerMeter", QuantityConstant("N/m")),
    ("MilliNewtonPerMeter", QuantityConstant("mN/m")),
    ("KiloNewtonPerMeter", QuantityConstant("kN/m")),
    ("MegaNewtonPerMeter", QuantityConstant("MN/m")),
    ("Pascal", QuantityConstant("Pa")),
    ("KiloPascal", QuantityConstant("kPa")),
    ("MegaPascal", QuantityConstant("MPa")),
    ("GigaPascal", QuantityConstant("GPa")),
    ("MilliBar", QuantityConstant("mbar")),
    ("Bar", QuantityConstant("bar")),
    ("PoundForce", QuantityConstant("lbf")),
    ("Torr", QuantityConstant("Torr")),
    ("mTorr", QuantityConstant("mTorr")),
    ("yTorr", QuantityConstant("uTorr")),
    ("PSI", QuantityConstant("psi")),
    ("KSI", QuantityConstant("ksi")),
    ("MPSI", QuantityConstant("Mpsi")),
    ("Watt", QuantityConstant("W")),
    ("NanoWatt", QuantityConstant("nW")),
    ("MicroWatt", QuantityConstant("uW")),
    ("MilliWatt", QuantityConstant("mW")),
    ("KiloWatt", QuantityConstant("kW")),
    ("VoltAmpere", QuantityConstant("VA")),
    ("Volt", QuantityConstant("V")),
    ("MilliVolt", QuantityConstant("mV")),
    ("KiloVolt", QuantityConstant("kV")),
    ("MegaSiemens", QuantityConstant("MS")),
    ("KiloSiemens", QuantityConstant("kS")),
    ("Siemens", QuantityConstant("S")),
    ("MilliSiemens", QuantityConstant("mS")),
    ("MicroSiemens", QuantityConstant("uS")),
    ("Ohm", QuantityConstant("Ohm")),
    ("KiloOhm", QuantityConstant("kOhm")),
    ("MegaOhm", QuantityConstant("MOhm")),
    ("Coulomb", QuantityConstant("C")),
    ("Tesla", QuantityConstant("T")),
    ("Gauss", QuantityConstant("G")),
    ("Weber", QuantityConstant("Wb")),
    # Oersted intentionally remains unavailable: exposing it reformats field
    # strengths such as 1 ampere per meter as Oersted.
    ("PicoFarad", QuantityConstant("pF")),
    ("NanoFarad", QuantityConstant("nF")),
    ("MicroFarad", QuantityConstant("uF")),
    ("MilliFarad", QuantityConstant("mF")),
    ("Farad", QuantityConstant("F")),
    ("NanoHenry", QuantityConstant("nH")),
    ("MicroHenry", QuantityConstant("uH")),
    ("MilliHenry", QuantityConstant("mH")),
    ("Henry", QuantityConstant("H")),
    ("Joule", QuantityConstant("J")),
    ("MilliJoule", QuantityConstant("mJ")),
    ("KiloJoule", QuantityConstant("kJ")),
    ("NewtonMeter", QuantityConstant("Nm")),
    ("VoltAmpereSecond", QuantityConstant("VAs")),
    ("WattSecond", QuantityConstant("Ws")),
    ("KiloWattHour", QuantityConstant("kWh")),
    ("ElectronVolt", QuantityConstant("eV")),
    ("KiloElectronVolt", QuantityConstant("keV")),
    ("MegaElectronVolt", QuantityConstant("MeV")),
    ("Calorie", QuantityConstant("cal")),
    ("KiloCalorie", QuantityConstant("kcal")),
    ("MPH", QuantityConstant("mi/h")),
    ("KMH", QuantityConstant("km/h")),
    ("Degree", QuantityConstant("deg")),
    ("Radian", QuantityConstant("rad")),
    ("Gon", QuantityConstant("gon")),
    ("AngularMinute", QuantityConstant("<angular-minute>")),
    ("AngularSecond", QuantityConstant("<angular-second>")),
)


UNIT_CONSTANTS: tuple[tuple[str, UnitConstant], ...] = (
    # SI base units.
    ("AmountOfSubstance", UnitConstant((0, 0, 0, 0, 0, 1,))),
    ("ElectricCurrent", UnitConstant((0, 0, 0, 1,))),
    ("Length", UnitConstant((1,))),
    ("LuminousIntensity", UnitConstant((0, 0, 0, 0, 0, 0, 1,))),
    ("Mass", UnitConstant((0, 1,))),
    ("Temperature", UnitConstant((0, 0, 0, 0, 1,))),
    ("TimeSpan", UnitConstant((0, 0, 1,))),
    # Derived and combined units.
    ("Acceleration", UnitConstant((1, 0, -2,))),
    ("Angle", UnitConstant((0, 0, 0, 0, 0, 0, 0, 1,))),
    ("AngleOfFriction", UnitConstant((0, 0, 0, 0, 0, 0, 0, 1,))),
    ("Area", UnitConstant((2,))),
    ("CompressiveStrength", UnitConstant((-1, 1, -2,))),
    ("CurrentDensity", UnitConstant((-2, 0, 0, 1,))),
    ("Density", UnitConstant((-3, 1,))),
    ("DissipationRate", UnitConstant((2, 0, -3,))),
    ("DynamicViscosity", UnitConstant((-1, 1, -1,))),
    ("Frequency", UnitConstant((0, 0, -1,))),
    ("MagneticFluxDensity", UnitConstant((0, 1, -2, -1,))),
    ("Magnetization", UnitConstant((-1, 0, 0, 1,))),
    ("ElectricalCapacitance", UnitConstant((-2, -1, 4, 2,))),
    ("ElectricalConductance", UnitConstant((-2, -1, 3, 2,))),
    ("ElectricalConductivity", UnitConstant((-3, -1, 3, 2,))),
    ("ElectricalInductance", UnitConstant((2, 1, -2, -2,))),
    ("ElectricalResistance", UnitConstant((2, 1, -3, -2,))),
    ("ElectricCharge", UnitConstant((0, 0, 1, 1,))),
    ("ElectricPotential", UnitConstant((2, 1, -3, -1,))),
    ("Force", UnitConstant((1, 1, -2,))),
    ("HeatFlux", UnitConstant((0, 1, -3, 0, 0,))),
    ("InverseArea", UnitConstant((-2,))),
    ("InverseLength", UnitConstant((-1,))),
    ("InverseVolume", UnitConstant((-3,))),
    ("KinematicViscosity", UnitConstant((2, 0, -1,))),
    ("Pressure", UnitConstant((-1, 1, -2,))),
    ("Power", UnitConstant((2, 1, -3,))),
    ("ShearModulus", UnitConstant((-1, 1, -2,))),
    ("SpecificEnergy", UnitConstant((2, 0, -2,))),
    ("SpecificHeat", UnitConstant((2, 0, -2, 0, -1,))),
    ("Stiffness", UnitConstant((0, 1, -2,))),
    ("Stress", UnitConstant((-1, 1, -2,))),
    ("ThermalConductivity", UnitConstant((1, 1, -3, 0, -1,))),
    ("ThermalExpansionCoefficient", UnitConstant((0, 0, 0, 0, -1,))),
    ("ThermalTransferCoefficient", UnitConstant((0, 1, -3, 0, -1,))),
    ("UltimateTensileStrength", UnitConstant((-1, 1, -2,))),
    ("Velocity", UnitConstant((1, 0, -1,))),
    ("VacuumPermittivity", UnitConstant((-3, -1, 4, 2,))),
    ("Volume", UnitConstant((3,))),
    ("VolumeFlowRate", UnitConstant((3, 0, -1,))),
    ("VolumetricThermalExpansionCoefficient", UnitConstant((0, 0, 0, 0, -1,))),
    ("Work", UnitConstant((2, 1, -2,))),
    ("YieldStrength", UnitConstant((-1, 1, -2,))),
    ("YoungsModulus", UnitConstant((-1, 1, -2,))),
)

# The values must match with that of the
# C++ enum class UnitSystem
class Scheme(IntEnum):
    """Unit display scheme selected by the FreeCAD preferences."""

    Internal = 0
    MKS = 1
    Imperial = 2
    ImperialDecimal = 3
    Centimeter = 4
    ImperialBuilding = 5
    MmMin = 6
    ImperialCivil = 7
    FEM = 8
    MeterDecimal = 9


class NumberFormat(IntEnum):
    """Number-format preference used when displaying quantities."""

    Default = 0
    Fixed = 1
    Scientific = 2


def install_units(app: types.ModuleType) -> None:
    """Install the declared quantities, units, and unit-format enums."""
    units = app.Units

    for name, spec in QUANTITY_CONSTANTS:
        if spec.expression == "<angular-minute>":
            value = units.Quantity().AngularMinute
        elif spec.expression == "<angular-second>":
            value = units.Quantity().AngularSecond
        else:
            value = units.Quantity(spec.expression)
        setattr(units, name, value)

    for name, spec in UNIT_CONSTANTS:
        setattr(units, name, units.Unit(*spec.signature))

    units.Scheme = Scheme
    units.NumberFormat = NumberFormat


install_units(App)

class ScaleType(IntEnum):
    """Scaling mode used by application-level geometry operations."""

    Other = -1
    NoScaling = 0
    NonUniformRight = 1
    NonUniformLeft = 2
    Uniform = 3

App.ScaleType = ScaleType

class PropertyType(IntEnum):
    """Flags describing the behavior of a FreeCAD property."""

    Prop_None = 0
    Prop_ReadOnly = 1
    Prop_Transient = 2
    Prop_Hidden = 4
    Prop_Output = 8
    Prop_NoRecompute = 16
    Prop_NoPersist = 32
    Prop_Input = 64

App.PropertyType = PropertyType

class ReturnType(IntEnum):
    """Return-shape selector used by FreeCAD Python callbacks."""

    PyObject = 0
    DocObject = 1
    DocAndPyObject = 2
    Placement = 3
    Matrix = 4
    LinkAndPlacement = 5
    LinkAndMatrix = 6

App.ReturnType = ReturnType


# ┌────────────────────────────────────────────────┐
# │ Init Framework                                 │
# └────────────────────────────────────────────────┘

class Transient:
    """
    Mark the symbol for removal from global scope on cleanup.
    """

    names = [
        "Path",
        "Callable",
        "QuantityConstant",
        "UnitConstant",
        "QUANTITY_CONSTANTS",
        "UNIT_CONSTANTS",
        "install_units",
    ]

    def __call__(self, target):
        Transient.names.append(target.__name__)
        return target

    @classmethod
    def cleanup(cls) -> None:
        # Remove imports
        # os: kept for backwards compat
        keep = set(("__builtins__", "FreeCAD", "App", "os", "sys", "traceback", "inspect"))
        names = [name for name, ref in globals().items() if isinstance(ref, types.ModuleType)]
        for name in names:
            if name not in keep:
                del globals()[name]

        # Remove transient symbols
        cls.names.extend(("transient", cls.__name__))
        for name in cls.names:
            del globals()[name]


transient = Transient()

@transient
@functools.cache
def resolve_path(path: Path) -> Path:
    return path.resolve()

@transient
def call_in_place(fn):
    """Call the function in place immediately after its definition."""
    fn()
    return fn

@transient
class utils:
    HLine = "-" * 80

    @staticmethod
    def str_to_paths(paths: str, delim: str = ";") -> list[Path]:
        """Convert a delimited string list of paths to a list of Path objects."""
        items = (item.strip() for item in paths.split(delim))
        # Filtering out empty paths: This may break backwards compat or not.
        # If something breaks, just remove the filter to allow empty paths
        non_empty_paths = filter(bool, items)
        return list(map(Path, non_empty_paths))

    @staticmethod
    def env_to_path(name: str) -> Path | None:
        if path := os.environ.get(name):
            return Path(path)
        return None

    @staticmethod
    def setup_tty_tab_completion():
        """
        Tries to setup readline-based tab-completion.

        Call this function only if you are in a tty-based REPL environment.
        """
        try:
            import readline
            import rlcompleter  # noqa: F401, import required
            readline.parse_and_bind("tab: complete")
        except ImportError:
            # Note: As there is no readline on Windows,
            #       we just ignore import errors here.
            pass


@transient
class PathPriority(IntEnum):
    Ignore = 0
    FallbackLast = 1
    FallbackFirst = 2
    OverrideLast = 3
    OverrideFirst = 4

@transient
@dataclasses.dataclass
class PathSet:
    """
    Collection of paths with priority support.

    Items can be inserted at specific priorities and composed in nested levels.

    Structure:
    [
        override:(OverrideFirst, *_, OverrideLast),
        *source,
        fallback:(FallbackFirst, *_, FallbackLast),
    ]
    """

    source: list["Path | PathSet"] = dataclasses.field(default_factory=list)
    override: collections.deque["Path | PathSet"] = dataclasses.field(default_factory=collections.deque)
    fallback: collections.deque["Path | PathSet"] = dataclasses.field(default_factory=collections.deque)

    def add(self, item: "Path | PathSet", priority: PathPriority = PathPriority.OverrideLast) -> None:
        """Add item into the corresponding priority slot."""
        if isinstance(item, Path):
            item = item.resolve()
            if not item.exists():
                return

        if priority == PathPriority.FallbackLast:
            self.fallback.append(item)
        elif priority == PathPriority.OverrideFirst:
            self.override.appendleft(item)
        elif priority == PathPriority.OverrideLast:
            self.override.append(item)
        elif priority == PathPriority.FallbackFirst:
            self.fallback.appendleft(item)
        elif priority == PathPriority.Ignore:
            pass
        else:
            msg = "Invalid path priority"
            raise ValueError(msg)

    def iter(self) -> coll_abc.Iterable[Path]:
        """
        Return iterable in priority order, higher first.
        """
        for section in (self.override, self.source, self.fallback):
            for path in section:
                if isinstance(path, PathSet):
                    yield from path.iter()
                else:
                    yield path

    def build(self) -> list[Path]:
        """
        Build and remove duplicates, keep priority order.
        """
        return list(dict.fromkeys(self.iter()))

@transient
class SearchPaths:
    """
    Manages search paths for binaries, libraries and modules.

    The most generic search path is PATH in environment.
    DLL search path is windows specific.
    sys.path is for module imports.
    """

    env_path: PathSet
    sys_path: PathSet
    dll_path: PathSet

    def __init__(self):
        self.env_path = PathSet([Path(p) for p in os.environ.get("PATH", "").split(os.pathsep)])
        self.sys_path = PathSet(sys.path)
        self.dll_path = PathSet()

    def add(
        self,
        item: Path | PathSet,
        *,
        env_path: PathPriority = PathPriority.OverrideLast,
        sys_path: PathPriority = PathPriority.OverrideFirst,
        dll_path: PathPriority = PathPriority.OverrideLast
    ) -> None:
        """
        Add item to required namespaces with the specified priority.

        Actual changes are buffered until commit().
        """
        self.env_path.add(item, env_path)
        self.sys_path.add(item, sys_path)
        self.dll_path.add(item, dll_path)

    def commit(self) -> None:
        """Apply changes to underlying namespaces and priorities."""
        os.environ["PATH"] = os.pathsep.join(str(path) for path in self.env_path.build())
        sys.path = [str(path) for path in self.sys_path.build()]

        if win32 := WindowsPlatform():
            win32.add_dll_search_paths(self.dll_path.build())

        # Reset
        self.__init__()


@transient
class Config:
    AdditionalModulePaths = utils.str_to_paths(App.ConfigGet("AdditionalModulePaths"))
    AdditionalMacroPaths = utils.str_to_paths(App.ConfigGet("AdditionalMacroPaths"))
    RunMode: str = App.ConfigGet('RunMode')
    DisabledAddons: set[str] = set(mod for mod in App.ConfigGet("DisabledAddons").split(";") if mod)


@transient
class WindowsPlatform:
    """
    Windows specific hooks.
    """

    initialized = False
    enabled = platform.system() == 'Windows' and hasattr(os, "add_dll_directory")

    def __init__(self) -> None:
        if not WindowsPlatform.enabled or WindowsPlatform.initialized:
            return

        if lib_pack := utils.env_to_path("FREECAD_LIBPACK_BIN"):
            os.add_dll_directory(str(lib_pack.resolve()))
        if win_dir := utils.env_to_path("WINDIR"):
            system32 = win_dir / "system32"
            os.add_dll_directory(str(system32.resolve()))

        WindowsPlatform.initialized = True

    def __bool__(self) -> bool:
        return self.enabled

    def add_dll_search_paths(self, paths: list[Path]) -> None:
        for path in paths:
            os.add_dll_directory(str(path.resolve()))


@transient
class DarwinPlatform:
    """
    MacOSX specific hooks.
    """

    enabled = platform.system() == "Darwin" and len(platform.mac_ver()[0]) > 0

    def __bool__(self) -> bool:
        return self.enabled

    def post(self) -> None:
        # add special path for MacOSX (bug #0000307): Where is this bug documented?
        sys.path.append(os.path.expanduser("~/Library/Application Support/FreeCAD/Mod"))


class ModState(IntEnum):
    Unsupported = -3
    Failed = -2
    Disabled = -1
    Discovered = 0
    Resolved = 1
    Loaded = 2


@transient
class Mod:
    """
    Base Mod.

    There are two types of Mods: Directory based (DirMod) or Module based (ExtMod).
    """

    ALL_ADDONS_DISABLED = "ALL_ADDONS_DISABLED"
    ADDON_DISABLED = "ADDON_DISABLED"
    PACKAGE_XML = "package.xml"

    state: ModState

    @property
    def kind(self) -> str:
        """Return the Mod type: 'Dir' or 'Ext'"""
        return "Ext"

    @property
    def init_mode(self) -> str:
        """Return the Mod init mode: 'exec' or 'import' or ''"""
        return "import"

    @property
    def metadata(self) -> App.Metadata | None:
        """Return Metadata from package.xml if any."""

    def check_disabled(self) -> bool:
        """
        Mods can be disabled by several methods:
        - command line argument: --disable-addon <name>
        - stop file: ALL_ADDONS_DISABLED
        - stop file: ADDON_DISABLED
        """

    def process_metadata(self, search_paths: SearchPaths) -> None:
        """
        Process package.xml if present to check version compatibility and to scan internal workbenches.
        """

    def run_init(self) -> None:
        """
        Run all required initialization scripts/modules: Init.py, init, __init__.py
        """

    def supports_freecad_version(self) -> bool:
        """
        Check if the Mod supports the current FreeCAD version.
        """
        if meta := self.metadata:
            return meta.supportsCurrentFreeCAD()
        return True

    def load(self, search_paths: SearchPaths) -> None:
        """
        Load the Mod.
        """
        try:
            self.process_metadata(search_paths)
        except Exception as ex:
            self.state = ModState.Failed
            Err(str(ex))
        else:
            if self.state == ModState.Resolved:
                self.run_init()
                if self.state == ModState.Resolved:
                    self.state = ModState.Loaded


@transient
class ExtMod(Mod):
    """
    Module based Mod (aka extension module).

    This kind of Mods are loaded using python module system, no direct filesystem or
    compile/execute hacks are used.

    extension modules must be defined in namespace freecad.*, i.e. freecad.MyAddon.
    """

    name: str  # full module name, i.e.: freecad.MyAddon

    def __init__(self, name: str):
        self.state = ModState.Resolved
        self.name = name

    @functools.cached_property
    def metadata(self) -> App.Metadata | None:
        with resources.as_file(resources.files(self.name)) as base:
            metadata = base / self.PACKAGE_XML
            if metadata.exists():
                return App.Metadata(str(metadata))
            metadata = base.parent.parent / self.PACKAGE_XML
            if metadata.exists():
                return App.Metadata(str(metadata))
        return None

    def check_disabled(self) -> bool:
        with resources.as_file(resources.files(self.name)) as base:
            return (base / self.ADDON_DISABLED).exists() or (base.parent.parent / self.ADDON_DISABLED).exists()

    def process_metadata(self, _search_paths: SearchPaths) -> None:
        meta = self.metadata
        if not meta:
            return

        if not self.supports_freecad_version():
            self.state = ModState.Unsupported
            Msg(f"NOTICE: {self.name} does not support this version of FreeCAD, so is being skipped")

    def _init_error(self, ex: Exception, error_msg: str) -> None:
        Err(f'During initialization the error "{ex!s}" occurred in {self.name}')
        Err(utils.HLine)
        Err(error_msg)
        Err(utils.HLine)
        Log(f'Init:      Initializing {self.name}... failed')
        Err(utils.HLine)
        Log(error_msg)
        Err(utils.HLine)

    def run_init(self) -> None:
        try:
            module = importlib.import_module(self.name) # Implicit run of __init__.py
        except Exception as ex:
            self._init_error(ex, traceback.format_exc())
            self.state = ModState.Failed
        else:
            self.run_secondary_init(module)

    def run_secondary_init(self, module: types.ModuleType) -> None:
        try:
            importlib.import_module(f"{module.__name__}.init")
        except ModuleNotFoundError:
            pass  # Ok, this module is optional
        except Exception as ex:
            self._init_error(ex, traceback.format_exc())
            self.state = ModState.Failed


@transient
class DirMod(Mod):
    """
    Directory based Mod. (aka Standard/Legacy).

    This kind of Mods are scanned from several directories in the system
    following certain priority. The name part of the path is used
    as the module name.

    Dir based modules can be overridden if several copies exists in different directories,
    resolution is based on directory priority.
    """

    INIT_PY = "Init.py"

    _path: collections.deque[Path]

    def __init__(self, path: Path) -> None:
        self.state = ModState.Discovered
        self._path = collections.deque()
        self._path.append(path)

    @property
    def kind(self) -> str:
        return "Dir"

    @property
    def init_mode(self) -> str:
        return "exec" if (self.path / self.INIT_PY).exists() else ''

    @property
    def name(self) -> str:
        return self.path.name

    @functools.cached_property
    def metadata(self) -> App.Metadata | None:
        metadata = self.path / self.PACKAGE_XML
        if metadata.exists():
            return App.Metadata(str(metadata))
        return None

    def process_metadata(self, search_paths: SearchPaths):
        meta = self.metadata
        if not meta:
            return

        if not self.supports_freecad_version():
            self.state = ModState.Unsupported
            Msg(f"NOTICE: {meta.Name} does not support this version of FreeCAD, so is being skipped")
            return

        content = meta.Content
        if "workbench" in content:
            workbenches = content["workbench"]
            for workbench in workbenches:
                if not workbench.supportsCurrentFreeCAD():
                    Msg(f"NOTICE: {meta.Name} content item {workbench.Name} does not support this version of FreeCAD, so is being skipped")
                    continue

                subdirectory = workbench.Name if not workbench.Subdirectory else workbench.Subdirectory
                subdirectory = re.split(r"[/\\]+", subdirectory)
                subdirectory = self.path / Path(*subdirectory)

                search_paths.add(
                    subdirectory,
                    env_path=PathPriority.OverrideLast,
                    sys_path=PathPriority.OverrideFirst,
                    dll_path=PathPriority.FallbackLast,
                )

    def override_with(self, path: Path) -> None:
        """
        Override current path with the one provided.
        """
        self._path.appendleft(path)

    @property
    def path(self) -> Path:
        """Current (highest priority) path."""
        return self._path[0]

    @property
    def alternative_paths(self) -> list[Path]:
        """Alternative paths in priority order"""
        return list(self._path)[1:]

    def check_disabled(self) -> bool:
        name = self.path.name

        if name in Config.DisabledAddons:
            Msg(f'NOTICE: Addon "{name}" disabled by presence of "--disable-addon {name}" argument')
            return True

        for flag in (self.ALL_ADDONS_DISABLED, self.ADDON_DISABLED):
            if (self.path / flag).exists():
                Msg(f'NOTICE: Addon "{self.path!s}" disabled by presence of {flag} stopfile')
                return True

        return False

    def resolve(self, search_paths: SearchPaths) -> None:
        """
        Add the current path to search paths to make it loadable.
        """
        if self.check_disabled():
            self.state = ModState.Disabled
            return

        search_paths.add(
            self.path,
            env_path=PathPriority.OverrideLast,
            sys_path=PathPriority.OverrideFirst,
        )

        self.state = ModState.Resolved

    def run_init(self) -> None:
        init_py = self.path / self.INIT_PY
        if not init_py.exists():
            self.state = ModState.Loaded
            Log(f"Init:      Initializing {self.path!s} ({self.INIT_PY} not found)... ignore")
            return

        try:
            source = init_py.read_text(encoding="utf-8")
            code = compile(source, init_py, 'exec')
            exec(code)
        except Exception as ex:
            Log(f"Init:      Initializing {self.path!s}... failed")
            Log(utils.HLine)
            Log(f"{traceback.format_exc()}")
            Log(utils.HLine)
            Err(f"During initialization the error \"{ex!s}\" occurred in {init_py!s}")
            Err("Please look into the log file for further information")
            self.state = ModState.Failed
        else:
            self.state = ModState.Loaded
            Log(f"Init:      Initializing {self.path!s}... done")


@transient
class ExtModScanner:
    """
    Scan extension Mods from the python import path.
    """

    mods: list[ExtMod]

    def __init__(self):
        self.mods = []

    def scan(self):
        import freecad
        modules = (m[1] for m in pkgutil.iter_modules(freecad.__path__, "freecad.") if m[2])
        for module_name in modules:
            mod = ExtMod(module_name)
            self.mods.append(mod)
            if module_name in Config.DisabledAddons:
                mod.state = ModState.Disabled
                Msg(f'NOTICE: Addon "{module_name}" disabled by presence of "--disable-addon {module_name}" argument')
                continue
            Log(f'Init:      Initializing {module_name}')

    def iter(self) -> coll_abc.Iterable[ExtMod]:
        return self.mods


@transient
class DirModScanner:
    """
    Sacan in the filesystem for Dir based Mods in the valid locations.
    """

    EXCLUDE: set[str] = set(["", "CVS", "__init__.py"]) # Why?
    mods: dict[str, DirMod]
    visited: set[str]

    def __init__(self) -> None:
        self.mods = {}
        self.visited = set()

    def iter(self) -> coll_abc.Iterable[DirMod]:
        """All discovered Mods."""
        return self.mods.values()

    def dirs(self) -> list[Path]:
        """Paths of all discovered Mods."""
        return [mod.path for mod in self.mods.values()]

    def scan_and_override(self, base: Path, *, flat: bool = False, warning: str | None = None) -> None:
        """
        Scan in base with higher priority.
        """
        if (key := str(resolve_path(base))) in self.visited:
            return

        self.visited.add(key)

        if not base.exists():
            if warning:
                Wrn(warning)
            return

        if warning:
            Wrn(warning)

        if flat:
            resolved = resolve_path(base)
            if any(resolve_path(mod.path) == resolved for mod in self.mods.values()):
                return
            self.mods[str(base)] = DirMod(base)
            return

        for mod_dir in filter(Path.is_dir, base.iterdir()):
            name = mod_dir.name.lower()
            if name in DirModScanner.EXCLUDE:
                continue

            if mod := self.mods.get(name):
                mod.override_with(mod_dir)
                continue

            self.mods[name] = DirMod(mod_dir)


# ┌────────────────────────────────────────────────┐
# │ Init Pipeline Definition                       │
# └────────────────────────────────────────────────┘

@transient
class InitPipeline:
    """
    Init sequence, setup search paths, scan and load Mods and run platform specific hooks.
    """

    std_home = Path(App.getHomePath()).resolve()
    user_home = Path(App.getUserAppDataDir()).resolve()
    try:
        std_lib = Path(App.getLibraryDir()).resolve()
    except OSError:
        # The library path is not strictly required, so if the OS itself raises an error when trying
        # to resolve it, just fall back to something reasonable. See #26864.
        std_lib = std_home / "lib"
        Log(f"Resolving library directory '{App.getLibraryDir()}' failed, using fallback '{std_lib}'")
    dir_mod_scanner = DirModScanner()
    ext_mod_scanner = ExtModScanner()
    search_paths = SearchPaths()

    def added_python_packages(self) -> PathSet:
        """
        Additional python packages installed by AddonManager/pip.
        """
        major, minor, _ = platform.python_version_tuple()
        packages = self.user_home / "AdditionalPythonPackages"
        vendor_path = packages / f"py{major}{minor}"
        paths = PathSet()
        paths.add(vendor_path)
        paths.add(packages)
        return paths

    def check_bundled_pivy(self) -> None:
        """
        Verify that bundled builds resolve the bundled Pivy package.
        """
        if App.ConfigGet("PIVY_SOURCE") != "bundled":
            return

        expected = (self.std_home / "Mod" / "pivy").resolve()

        def block_system_pivy(message: str) -> None:
            for name in list(sys.modules):
                if name == "pivy" or name.startswith("pivy."):
                    del sys.modules[name]
            sys.modules["pivy"] = None
            raise RuntimeError(message)

        def path_from_import_origin(origin: str) -> Path:
            path = Path(origin).resolve()
            return path.parent if path.name == "__init__.py" else path

        def find_pivy_path() -> Path:
            module = sys.modules.get("pivy")
            module_file = getattr(module, "__file__", None) if module else None
            if module_file:
                return path_from_import_origin(module_file)

            spec = importlib.util.find_spec("pivy")
            if spec is None:
                block_system_pivy(
                    f"Bundled Pivy is enabled, but pivy was not found. "
                    f"Expected bundled Pivy at {expected!s}."
                )
            if spec.submodule_search_locations:
                return Path(next(iter(spec.submodule_search_locations))).resolve()
            if spec.origin:
                return path_from_import_origin(spec.origin)
            block_system_pivy(
                f"Bundled Pivy is enabled, but pivy has no import location. "
                f"Expected bundled Pivy at {expected!s}."
            )

        resolved = find_pivy_path()
        if resolved != expected:
            block_system_pivy(
                f"Bundled Pivy is enabled, but Python resolves pivy from {resolved!s}. "
                f"Expected bundled Pivy at {expected!s}."
            )

        Log(f"Init:   Using bundled Pivy from {expected!s}")

    def scan(self) -> None:
        """
        Scan step, search for standard directories, libraries and Mods.
        """
        std_home = self.std_home
        user_home = self.user_home
        std_lib = self.std_lib
        std_mod = std_home / "Mod"
        std_ext = std_home / "Ext"
        std_bin = std_home / "bin"
        user_macro = Path(App.getUserMacroDir(True)).resolve()
        user_mod = user_home / "Mod"
        search_paths = self.search_paths

        legacy_user_mod = Path.home() / ".FreeCAD" / "Mod"
        if legacy_user_mod.exists():
            Wrn (f"User path has changed to {user_home!s}. Please move user modules and macros")

        # Libraries
        libraries = PathSet()
        libraries.add(std_home / "lib")
        libraries.add(std_home / "lib64")
        libraries.add(std_home / "lib-py3")
        libraries.add(std_lib)

        search_paths.add(
            libraries,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.OverrideLast,
        )

        # Tools
        search_paths.add(
            std_home / "Tools",
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.FallbackLast,
        )

        # Binaries
        search_paths.add(
            std_bin,
            env_path=PathPriority.OverrideFirst,
            sys_path=PathPriority.Ignore,
            dll_path=PathPriority.FallbackLast,
        )

        # Scan for Directory based Mods
        # Order is important because of overrides
        Log("Init:   Searching for modules...")
        mods = self.dir_mod_scanner
        mods.scan_and_override(std_mod)
        mods.scan_and_override(user_mod)
        mods.scan_and_override(user_macro / "Mod")
        additional_mods = Config.AdditionalModulePaths + Config.AdditionalMacroPaths
        for add in additional_mods:
            mods.scan_and_override(add, flat=True)

        # to have all the module-paths available in FreeCADGuiInit.py:
        App.__ModDirs__ = [str(d) for d in mods.dirs()]

        # this allows importing with:
        # from FreeCAD.Module import package
        import_path = PathSet([libraries])
        import_path.add(std_mod, PathPriority.OverrideFirst)
        import_path.add(user_mod, PathPriority.FallbackLast)
        App.__path__ = [str(path) for path in import_path.build()]

        # also add these directories to the sys.path to
        # not change the old behavior. once we have moved to
        # proper python modules this can eventually be removed.
        search_paths.add(
            std_mod,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.OverrideFirst,
        )
        search_paths.add(
            std_ext,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.OverrideLast,
        )

        # Additional installed packages (AddonManager/pip)
        search_paths.add(
            self.added_python_packages(),
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.FallbackLast,
        )

        # Resolve Dir Mods
        for mod in mods.iter():
            mod.resolve(search_paths)

    def load_mods(self) -> None:
        """
        Load Mods step, load both Dir based and Module based Mods.
        """
        module_cache = []

        # Update search paths to make Mods visible to import system.
        search_paths = self.search_paths
        search_paths.commit()
        self.check_bundled_pivy()

        # Dir Mods first
        for mod in self.dir_mod_scanner.iter():
            if mod.state == ModState.Resolved:
                mod.load(search_paths)
                module_cache.append(mod)

        # Update search paths: may have changed by dir loads
        search_paths.commit()

        # Finally, Module based Mod are loaded from python path
        self.ext_mod_scanner.scan()
        for mod in self.ext_mod_scanner.iter():
            if mod.state == ModState.Resolved:
                mod.load(search_paths)
                module_cache.append(mod)

        # Save to use in FreeCADGuiInit.py
        App.__ModCache__ = module_cache

    def register_macro_sources(self) -> None:
        """
        Add Macro sources to search paths.
        """
        std_macro = self.std_home / "Macro"
        user_macro_default = Path(App.getUserMacroDir(False)).resolve()
        user_macro = Path(App.getUserMacroDir(True)).resolve()

        # add MacroDir to path (RFE #0000504)
        self.search_paths.add(
            user_macro_default,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.FallbackLast,
        )
        self.search_paths.add(
            user_macro,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.FallbackLast,
        )
        self.search_paths.add(
            std_macro,
            env_path=PathPriority.Ignore,
            sys_path=PathPriority.FallbackLast,
        )
        self.search_paths.commit()
        App.__MacroDirs__ = list({str(user_macro), str(user_macro_default), str(std_macro)})

    def post(self) -> None:
        """
        Run final steps.
        """
        if macosx := DarwinPlatform():
            macosx.post()

    def setup_tty(self) -> None:
        # Note: just checking whether stdin is a TTY is not enough, as the GUI is set up only after this
        # script has run. And checking only the RunMode is not enough, as we are maybe not interactive.
        if Config.RunMode == 'Cmd' and hasattr(sys.stdin, 'isatty') and sys.stdin.isatty():
            utils.setup_tty_tab_completion()

    def report(self) -> None:
        std_mod = self.std_home / "Mod"
        Log(f"Using {std_mod!s} as module path!")

        Log("System path after init:")
        for path in os.environ["PATH"].split(os.pathsep):
            Log(f"   {path}")

        Log("FreeCADInit Mod summary:")
        output = []
        output.append(f"+-{'--':-<24}-+-{'----':-<10}-+-{'---':-<6}-+-{'-----':-<48}-")
        output.append(f"| {'Mod':<24} | {'State':<10} | {'Mode':<6} | {'Source':<48} ")
        output.append(output[0])

        for mod in self.dir_mod_scanner.iter():
            output.append(f"| {mod.name:<24.24} | {mod.state.name:<10.10} | {mod.init_mode:<6.6} | {mod.path!s}")
            for alt in mod.alternative_paths:
                output.append(f"| {' ':<24.24} | {' ':<10.10} | {' ':<6.6} | {alt!s}")

        for mod in self.ext_mod_scanner.iter():
            output.append(f"| {mod.name:<24.24} | {mod.state.name:<10.10} | {mod.init_mode:<6.6} | {mod.name}")

        for line in output:
            Log(line)
        Log(output[0])

    def run(self) -> None:
        """
        Pipeline entry point.
        """
        self.scan()
        self.load_mods()
        self.register_macro_sources()
        self.post()
        self.report()
        self.setup_tty()


# ┌────────────────────────────────────────────────┐
# │ Init Applications                              │
# └────────────────────────────────────────────────┘

@transient
@call_in_place
def init_applications() -> None:
    try:
        InitPipeline().run()
        Log('Init: App::FreeCADInit.py done')
    except Exception as ex:
        Err(f'Error in init_applications {ex!s}')
        Err(utils.HLine)
        Err(traceback.format_exc())
        Err(utils.HLine)


# ┌────────────────────────────────────────────────┐
# │ Cleanup for next scripts                       │
# └────────────────────────────────────────────────┘

# Reset logger to no extra newline for subsequent scripts (Backwards compat)
__logger.sep = ""

# Clean global namespace
transient.cleanup()
