# ***************************************************************************
# *   Copyright (c) 2002,2003 Jürgen Riegel <juergen.riegel@web.de>         *
# *   Copyright (c) 2025 Frank Martínez <mnesarco at gmail dot com>         *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# FreeCAD init module - Gui
#
# Gathering all the information to start FreeCAD Gui.
# This is the forth of four init scripts:
# +------+------------------+-----------------------------+
# | This | Script           | Runs                        |
# +------+------------------+-----------------------------+
# |      | CMakeVariables   | always                      |
# |      | FreeCADInit      | always                      |
# |      | FreeCADTest      | only if test and not Gui    |
# | >>>> | FreeCADGuiInit   | only if Gui is up           |
# +------+------------------+-----------------------------+

from enum import IntEnum, Enum
from dataclasses import dataclass
import traceback
import typing
import re
from pathlib import Path
import importlib
import FreeCAD
import FreeCADGui

# shortcuts
Gui = FreeCADGui
App = FreeCAD

App.Console.PrintLog("Init: Running FreeCADGuiInit.py start script...\n")
App.Console.PrintLog("░░░▀█▀░█▀█░▀█▀░▀█▀░░░█▀▀░█░█░▀█▀░░\n")
App.Console.PrintLog("░░░░█░░█░█░░█░░░█░░░░█░█░█░█░░█░░░\n")
App.Console.PrintLog("░░░▀▀▀░▀░▀░▀▀▀░░▀░░░░▀▀▀░▀▀▀░▀▀▀░░\n")


# Declare symbols already defined in global by previous scripts to make linter happy.
if typing.TYPE_CHECKING:
    Log: typing.Callable = None
    Err: typing.Callable = None
    ModState: typing.Any = None


# The values must match with that of the C++ enum class ResolveMode
class ResolveMode(IntEnum):
    NoResolve = 0
    OldStyleElement = 1
    NewStyleElement = 2
    FollowLink = 3


Gui.Selection.ResolveMode = ResolveMode


# The values must match with that of the C++ enum class SelectionStyle
class SelectionStyle(IntEnum):
    NormalSelection = 0
    GreedySelection = 1


# The values must match with that of the Python enum class in ViewProvider.pyi
class ToggleVisibilityMode(Enum):
    CanToggleVisibility = "CanToggleVisibility"
    NoToggleVisibility = "NoToggleVisibility"


def _isCommandActive(name: str) -> bool:
    cmd = Gui.Command.get(name)
    return bool(cmd and cmd.isActive())


# this is to keep old code working
Gui.listCommands = Gui.Command.listAll
Gui.isCommandActive = _isCommandActive
Gui.Selection.SelectionStyle = SelectionStyle


# Important definitions
class Workbench:
    """The workbench base class."""

    MenuText = ""
    ToolTip = ""
    Icon = None

    __Workbench__: "Workbench"  # Injected by FreeCAD, see: Application::activateWorkbench

    def Initialize(self):
        """Initializes this workbench."""
        App.Console.PrintWarning(f"{self!s}: Workbench.Initialize() not implemented in subclass!")

    def ContextMenu(self, recipient):
        pass

    def appendToolbar(self, name, cmds):
        self.__Workbench__.appendToolbar(name, cmds)

    def removeToolbar(self, name):
        self.__Workbench__.removeToolbar(name)

    def listToolbars(self):
        return self.__Workbench__.listToolbars()

    def getToolbarItems(self):
        return self.__Workbench__.getToolbarItems()

    def appendCommandbar(self, name, cmds):
        self.__Workbench__.appendCommandbar(name, cmds)

    def removeCommandbar(self, name):
        self.__Workbench__.removeCommandbar(name)

    def listCommandbars(self):
        return self.__Workbench__.listCommandbars()

    def appendMenu(self, name, cmds):
        self.__Workbench__.appendMenu(name, cmds)

    def removeMenu(self, name):
        self.__Workbench__.removeMenu(name)

    def listMenus(self):
        return self.__Workbench__.listMenus()

    def appendContextMenu(self, name, cmds):
        self.__Workbench__.appendContextMenu(name, cmds)

    def removeContextMenu(self, name):
        self.__Workbench__.removeContextMenu(name)

    def reloadActive(self):
        self.__Workbench__.reloadActive()

    def name(self):
        return self.__Workbench__.name()

    def GetClassName(self):
        """Return the name of the associated C++ class."""
        # as default use this to simplify writing workbenches in Python
        return "Gui::PythonWorkbench"


class StandardWorkbench(Workbench):
    """
    A workbench defines the tool bars, command bars, menus,
    context menu and dockable windows of the main window.
    """

    def Initialize(self):
        """Initialize this workbench."""
        # load the module
        Log("Init: Loading FreeCAD GUI\n")

    def GetClassName(self):
        """Return the name of the associated C++ class."""
        return "Gui::StdWorkbench"


class NoneWorkbench(Workbench):
    """An empty workbench."""

    MenuText = "<none>"
    ToolTip = "The default empty workbench"

    def Initialize(self):
        """Initialize this workbench."""
        # load the module
        Log("Init: Loading FreeCAD GUI\n")

    def GetClassName(self):
        """Return the name of the associated C++ class."""
        return "Gui::NoneWorkbench"


@dataclass
class InputHint:
    """
    Represents a single input hint (shortcut suggestion).

    The message is a Qt formatting string with placeholders like %1, %2, ...
    The placeholders are replaced with input representations - be it keys, mouse buttons etc.
    Each placeholder corresponds to one input sequence. Sequence can either be:
     - one input from Gui.UserInput enum
     - tuple of mentioned enum values representing the input sequence

    >>> InputHint("%1 change mode", Gui.UserInput.KeyM)
    will result in a hint displaying `[M] change mode`

    >>> InputHint("%1 new line", (Gui.UserInput.KeyControl, Gui.UserInput.KeyEnter))
    will result in a hint displaying `[ctrl][enter] new line`

    >>> InputHint("%1/%2 increase/decrease ...", Gui.UserInput.KeyU, Gui.UserInput.KeyJ)
    will result in a hint displaying `[U]/[J] increase / decrease ...`
    """

    InputSequence = Gui.UserInput | tuple[Gui.UserInput, ...]

    message: str
    sequences: list[InputSequence]

    def __init__(self, message: str, *sequences: InputSequence):
        self.message = message
        self.sequences = list(sequences)


class HintManager:
    """
    A convenience class for managing input hints (shortcut suggestions) displayed to the user.
    It is here mostly to provide well-defined and easy to reach API from python without developers needing
    to call low-level functions on the main window directly.
    """

    def show(self, *hints: InputHint):
        """
        Displays the specified input hints to the user.

        :param hints: List of hints to show.
        """
        Gui.getMainWindow().showHint(*hints)

    def hide(self):
        """
        Hides all currently displayed input hints.
        """
        Gui.getMainWindow().hideHint()


Gui.InputHint = InputHint
Gui.HintManager = HintManager()


class ModGui:
    """
    Mod Gui Loader.
    """

    mod: typing.Any

    def run_init_gui(self, sub_workbench: Path | None = None) -> bool:
        return False

    def process_metadata(self) -> bool:
        return False

    def load(self) -> None:
        """
        Load the Mod Gui.
        """
        try:
            if self.mod.state == ModState.Loaded and not self.process_metadata():
                self.run_init_gui()
        except Exception as ex:
            self.mod.state = ModState.Failed
            Err(str(ex))


class DirModGui(ModGui):
    """
    Dir Mod Gui Loader.
    """

    INIT_GUI_PY = "InitGui.py"

    def __init__(self, mod):
        self.mod = mod

    def run_init_gui(self, sub_workbench: Path | None = None) -> bool:
        target = sub_workbench or self.mod.path
        init_gui_py = target / self.INIT_GUI_PY
        if init_gui_py.exists():
            try:
                source = init_gui_py.read_text(encoding="utf-8")
                code = compile(source, init_gui_py, "exec")
                exec(code)
            except Exception as ex:
                sep = "-" * 100 + "\n"
                Log(f"Init:      Initializing {target!s}... failed\n")
                Log(sep)
                Log(traceback.format_exc())
                Log(sep)
                Err(f'During initialization the error "{ex!s}" occurred in {init_gui_py!s}\n')
                Err("Look into the log file for further information\n")
            else:
                Log(f"Init:      Initializing {target!s}... done\n")
                return True
        else:
            Log(f"Init:      Initializing {target!s} (InitGui.py not found)... ignore\n")
        return False

    def process_metadata(self) -> bool:
        meta = self.mod.metadata
        if not meta:
            return False

        content = meta.Content
        processed = False
        if "workbench" in content:
            FreeCAD.Gui.addIconPath(str(self.mod.path))
            workbenches = content["workbench"]
            for workbench_metadata in workbenches:
                if not workbench_metadata.supportsCurrentFreeCAD():
                    continue

                subdirectory = workbench_metadata.Subdirectory or workbench_metadata.Name
                subdirectory = self.mod.path / Path(*re.split(r"[/\\]+", subdirectory))
                if not subdirectory.exists():
                    continue

                if self.run_init_gui(subdirectory):
                    processed = True
                    # Try to generate a new icon from the metadata-specified information
                    classname = workbench_metadata.Classname
                    if classname:
                        try:
                            wb_handle = FreeCAD.Gui.getWorkbench(classname)
                        except Exception:
                            Log(
                                f"Failed to get handle to {classname} -- no icon "
                                "can be generated, check classname in package.xml\n"
                            )
                        else:
                            GeneratePackageIcon(
                                str(subdirectory),
                                workbench_metadata,
                                wb_handle,
                            )
        return processed


class ExtModGui(ModGui):
    """
    Ext Mod Gui Loader.
    """

    def __init__(self, mod):
        self.mod = mod

    def run_init_gui(self, _sub_workbench: Path | None = None) -> bool:
        Log(f"Init: Initializing {self.mod.name}\n")
        try:
            try:
                importlib.import_module(f"{self.mod.name}.init_gui")
            except ModuleNotFoundError:
                Log(f"Init: No init_gui module found in {self.mod.name}, skipping\n")
            else:
                Log(f"Init: Initializing {self.mod.name}... done\n")
                return True
        except ImportError as ex:
            Err(f'During initialization the error "{ex!s}" occurred\n')
        except Exception as ex:
            sep = "-" * 80 + "\n"
            Err(f'During initialization the error "{ex!s}" occurred in {self.mod.name}\n')
            Err(sep)
            Err(traceback.format_exc())
            Err(sep)
            Log(f"Init:      Initializing {self.mod.name}... failed\n")
            Log(sep)
            Log(traceback.format_exc())
            Log(sep)
        return False


def InitApplications():

    # Patch freecad module with gui alias of FreeCADGui
    import freecad

    freecad.gui = FreeCADGui

    Log("Init:   Searching modules\n")

    def mod_gui_init(kind: str, mod_type: type, output: list[str]) -> None:
        for mod in App.__ModCache__:
            if mod.kind == kind:
                if mod.state == ModState.Loaded:
                    gui = mod_type(mod)
                    gui.load()
                if mod.init_mode:
                    row = (
                        f"| {mod.name:<24.24} | {mod.state.name:<10.10} | {mod.init_mode:<6.6} |\n"
                    )
                    output.append(row)

    output = []
    output.append(f"+-{'--':-<24}-+-{'--------':-<10}-+-{'---':-<6}-+\n")
    output.append(f"| {'Mod':<24} | {'Gui State':<10} | {'Mode':<6} |\n")
    output.append(output[0])

    mod_gui_init("Dir", DirModGui, output)
    Log("All modules with GUIs using InitGui.py are now initialized\n")

    mod_gui_init("Ext", ExtModGui, output)
    Log("All modules with GUIs initialized using pkgutil are now initialized\n")

    Log("FreeCADGuiInit Mod summary:\n")
    for line in output:
        Log(line)
    Log(output[0])


def GeneratePackageIcon(
    subdirectory: str, workbench_metadata: FreeCAD.Metadata, wb_handle: Workbench
) -> None:
    relative_filename = workbench_metadata.Icon
    if not relative_filename:
        # Although a required element, this content item does not have an icon. Just bail out
        return
    absolute_filename = Path(subdirectory) / Path(relative_filename)
    if hasattr(wb_handle, "Icon") and wb_handle.Icon:
        Log(
            f"Init:      Packaged workbench {workbench_metadata.Name} specified icon\
            in class {workbench_metadata.Classname}"
        )
        Log(" ... replacing with icon from package.xml data.\n")
    wb_handle.__dict__["Icon"] = str(absolute_filename.resolve())


# signal that the gui is up
App.GuiUp = 1
App.Gui = FreeCADGui
FreeCADGui.Workbench = Workbench

Gui.addWorkbench(NoneWorkbench())

# init modules
InitApplications()

# set standard workbench (needed as fallback)
Gui.activateWorkbench("NoneWorkbench")

# Register .py, .FCScript and .FCMacro
FreeCAD.addImportType("Inventor V2.1 (*.iv *.IV)", "FreeCADGui")
FreeCAD.addImportType(
    "VRML V2.0 (*.wrl *.WRL *.vrml *.VRML *.wrz *.WRZ *.wrl.gz *.WRL.GZ)", "FreeCADGui"
)
FreeCAD.addImportType("Python (*.py *.FCMacro *.FCScript *.fcmacro *.fcscript)", "FreeCADGui")
FreeCAD.addExportType("Inventor V2.1 (*.iv)", "FreeCADGui")
FreeCAD.addExportType("VRML V2.0 (*.wrl *.vrml *.wrz *.wrl.gz)", "FreeCADGui")
FreeCAD.addExportType("X3D Extensible 3D (*.x3d *.x3dz)", "FreeCADGui")
FreeCAD.addExportType("WebGL/X3D (*.xhtml)", "FreeCADGui")
FreeCAD.addExportType("Portable Document Format (*.pdf)", "FreeCADGui")
# FreeCAD.addExportType("IDTF (for 3D PDF) (*.idtf)","FreeCADGui")
# FreeCAD.addExportType("3D View (*.svg)","FreeCADGui")

Log("Init: Running FreeCADGuiInit.py start script... done\n")

# ┌────────────────────────────────────────────────┐
# │ Cleanup                                        │
# └────────────────────────────────────────────────┘

if not typing.TYPE_CHECKING:
    del InitApplications
    del NoneWorkbench
    del StandardWorkbench
    del App.__ModCache__, ModGui, DirModGui, ExtModGui
    del typing, re, Path, importlib
