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
import inspect
import os
import traceback
import typing
import re
import sys
from pathlib import Path
import importlib
import FreeCAD
import FreeCADGui

try:
    from FreeCADInitTools import (
        DIR_MOD_APP_COMPAT_GLOBAL_NAMES,
        dir_mod_compat_globals,
        dir_mod_package_name,
        exec_dir_mod_file,
    )
except ModuleNotFoundError:
    sys.path.append(FreeCAD.getLibraryDir())
    from FreeCADInitTools import (
        DIR_MOD_APP_COMPAT_GLOBAL_NAMES,
        dir_mod_compat_globals,
        dir_mod_package_name,
        exec_dir_mod_file,
    )

# shortcuts
Gui = FreeCADGui
App = FreeCAD

translate = FreeCAD.Qt.translate
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

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


_DIR_MOD_AUTO_RELOAD_EXCLUDED_DIRS = (
    "__pycache__",
    ".git",
    "Resources/translations",
    "build",
)


def _is_excluded_relative_path(relative: Path, excluded_dirs) -> bool:
    if not relative.parts:
        return False

    relative_text = relative.as_posix()
    for excluded in excluded_dirs:
        if "/" in excluded:
            if relative_text == excluded or relative_text.startswith(f"{excluded}/"):
                return True
        elif excluded in relative.parts:
            return True
    return False


def _iter_watch_paths_for_base(base_path: Path, watched_suffixes, excluded_dirs):
    base_path = Path(base_path).resolve()
    watched_suffixes = set(watched_suffixes)
    excluded_dirs = tuple(excluded_dirs)

    yield str(base_path)
    for directory, dirnames, filenames in os.walk(base_path):
        directory_path = Path(directory).resolve()
        relative = directory_path.relative_to(base_path)
        if _is_excluded_relative_path(relative, excluded_dirs):
            dirnames[:] = []
            continue

        yield str(directory_path)
        dirnames[:] = [
            dirname
            for dirname in dirnames
            if not _is_excluded_relative_path(relative / dirname, excluded_dirs)
        ]
        for filename in filenames:
            file_path = directory_path / filename
            if file_path.suffix in watched_suffixes:
                yield str(file_path)


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
                self.mod.remember_source_root(init_gui_py)
                exec_dir_mod_file(
                    self.mod.name,
                    init_gui_py,
                    self.mod.path,
                    dir_mod_gui_compat_globals(globals()),
                )
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


def _qt_core():
    try:
        from PySideWrapper import QtCore
    except ImportError:
        for candidate in ("PySide6.QtCore", "PySide2.QtCore", "PySide.QtCore"):
            try:
                return importlib.import_module(candidate)
            except ImportError:
                continue
        raise
    return QtCore


def _active_workbench_name() -> str | None:
    try:
        return Gui.activeWorkbench().name()
    except Exception:
        return None


def _reload_active_workbench() -> None:
    try:
        Gui.activeWorkbench().reloadActive()
    except Exception:
        pass


def _refresh_workbench_if_active(workbench_name: str) -> None:
    if _active_workbench_name() == workbench_name:
        _reload_active_workbench()


def _get_workbench_handler(name: str | None = None):
    if name:
        try:
            return Gui.getWorkbench(name)
        except Exception:
            for workbench_name, handler in Gui.listWorkbenches().items():
                if workbench_name == name or getattr(handler, "MenuText", None) == name:
                    return handler
            raise
    return Gui.activeWorkbench()


def _workbench_source_path(handler) -> Path:
    candidates = (
        getattr(handler, "Initialize", None),
        getattr(handler, "Activated", None),
        handler.__class__,
    )
    for candidate in candidates:
        if candidate is None:
            continue
        try:
            source = inspect.getsourcefile(candidate) or inspect.getfile(candidate)
        except (OSError, TypeError):
            continue
        if source:
            return Path(source).resolve()
    raise RuntimeError("Unable to resolve workbench source path")


def _module_paths(module) -> typing.Iterable[Path]:
    seen = set()

    if file := getattr(module, "__file__", None):
        raw_path = Path(file)
        for resolver in (Path.absolute, Path.resolve):
            try:
                candidate = resolver(raw_path)
            except OSError:
                continue
            if candidate in seen:
                continue
            seen.add(candidate)
            yield candidate

    for package_path in getattr(module, "__path__", []):
        raw_path = Path(package_path)
        for resolver in (Path.absolute, Path.resolve):
            try:
                candidate = resolver(raw_path)
            except OSError:
                continue
            if candidate in seen:
                continue
            seen.add(candidate)
            yield candidate


DIR_MOD_GUI_COMPAT_GLOBAL_NAMES = DIR_MOD_APP_COMPAT_GLOBAL_NAMES + (
    "Enum",
    "Gui",
    "FreeCADGui",
    "HintManager",
    "InputHint",
    "ModState",
    "ResolveMode",
    "SelectionStyle",
    "ToggleVisibilityMode",
    "Workbench",
    "dataclass",
    "translate",
    "typing",
)


def dir_mod_gui_compat_globals(source_globals: dict) -> dict:
    return dir_mod_compat_globals(source_globals, DIR_MOD_GUI_COMPAT_GLOBAL_NAMES)


def _purge_modules_by_path(base_path: Path) -> list[str]:
    removed = []
    for module_name, module in list(sys.modules.items()):
        if module_name in {"__main__", "FreeCADGuiInit"}:
            continue
        if module is None:
            continue

        for module_path in _module_paths(module):
            if module_path == base_path or module_path.is_relative_to(base_path):
                sys.modules.pop(module_name, None)
                removed.append(module_name)
                break

    removed.sort()
    return removed


def _purge_modules_by_prefix(prefix: str) -> list[str]:
    removed = []
    for module_name in list(sys.modules):
        if module_name == prefix or module_name.startswith(f"{prefix}."):
            sys.modules.pop(module_name, None)
            removed.append(module_name)
    removed.sort()
    return removed


def _purge_dir_mod_modules(base_path: Path, module_prefix: str | None = None) -> list[str]:
    # Dir mods still expose many legacy top-level imports (for example `BimSelect`),
    # so reloading must purge by filesystem path as well as the synthetic namespace.
    removed = set(_purge_modules_by_path(base_path))
    if module_prefix:
        removed.update(_purge_modules_by_prefix(module_prefix))
    return sorted(removed)


def _resolve_reload_target(name: str | None = None) -> dict[str, typing.Any]:
    handler = _get_workbench_handler(name)
    workbench_name = handler.__class__.__name__
    source_path = _workbench_source_path(handler)
    handler_module = handler.__class__.__module__

    for mod in App.__ModCache__:
        if mod.state != ModState.Loaded:
            continue

        if mod.kind == "Dir":
            base_path = mod.source_root
            module_prefix = dir_mod_package_name(mod.name)
            if handler_module == module_prefix or handler_module.startswith(f"{module_prefix}."):
                return {
                    "handler": handler,
                    "name": workbench_name,
                    "kind": "Dir",
                    "mod": mod,
                    "source_path": source_path,
                    "module_prefix": module_prefix,
                    "base_path": base_path,
                }
            if source_path == base_path or source_path.is_relative_to(base_path):
                return {
                    "handler": handler,
                    "name": workbench_name,
                    "kind": "Dir",
                    "mod": mod,
                    "source_path": source_path,
                    "base_path": base_path,
                }

        if mod.kind == "Ext":
            if handler_module == mod.name or handler_module.startswith(f"{mod.name}."):
                return {
                    "handler": handler,
                    "name": workbench_name,
                    "kind": "Ext",
                    "mod": mod,
                    "source_path": source_path,
                    "module_prefix": mod.name,
                    "base_path": source_path.parent,
                }

    raise RuntimeError(f"Workbench '{workbench_name}' is not backed by a reloadable Python module")


def _resolve_workbench_runtime_name(name: str | None = None) -> str | None:
    if name is None:
        try:
            handler = _get_workbench_handler()
        except Exception:
            return None
        return handler.__class__.__name__ if handler else None

    try:
        return _get_workbench_handler(name).__class__.__name__
    except Exception:
        return name


class WorkbenchRuntime:
    def __init__(self, name: str, generation: int):
        self.name = name
        self.generation = generation
        self._cleanup: list[typing.Callable[[], None]] = []
        self._owned: dict[str, tuple[typing.Any, typing.Callable[[], None] | None]] = {}
        self.data: dict[str, typing.Any] = {}
        self._disposed = False

    def _run_cleanup(self, cleanup: typing.Callable[[], None]) -> None:
        try:
            cleanup()
        except Exception:
            Err(f"Workbench runtime cleanup failed for {self.name}\n")
            Log(traceback.format_exc())

    def _ensure_active(self) -> None:
        if self._disposed:
            raise RuntimeError(f"Workbench runtime '{self.name}' is disposed")

    def onDispose(self, callback: typing.Callable[[], None]):
        self._ensure_active()
        self._cleanup.append(callback)
        return callback

    def remember(self, key: str, value):
        self._ensure_active()
        self.data[key] = value
        return value

    def own(self, key: str, value, cleanup: typing.Callable[[], None] | None = None):
        self._ensure_active()
        self.release(key)
        self._owned[key] = (value, cleanup)
        return value

    def getOwned(self, key: str, default=None):
        entry = self._owned.get(key)
        if entry is None:
            return default
        return entry[0]

    def owns(self, key: str) -> bool:
        return key in self._owned

    def release(self, key: str) -> bool:
        self._ensure_active()
        entry = self._owned.pop(key, None)
        if entry is None:
            return False

        _value, cleanup = entry
        if cleanup is not None:
            self._run_cleanup(cleanup)
        return True

    def addDocumentObserver(self, observer):
        self._ensure_active()
        Gui.addDocumentObserver(observer)

        def cleanup():
            try:
                Gui.removeDocumentObserver(observer)
            except Exception:
                pass

        self.onDispose(cleanup)
        return observer

    def addSelectionObserver(
        self,
        observer,
        *args,
        key: str | None = None,
        **kwargs,
    ):
        self._ensure_active()
        Gui.Selection.addObserver(observer, *args, **kwargs)

        def cleanup():
            try:
                Gui.Selection.removeObserver(observer)
            except Exception:
                pass

        if key is not None:
            self.own(key, observer, cleanup)
        else:
            self.onDispose(cleanup)
        return observer

    def addWorkbenchManipulator(
        self,
        manipulator,
        refresh: bool = False,
        key: str | None = None,
    ):
        self._ensure_active()
        Gui.addWorkbenchManipulator(manipulator)
        if refresh:
            _refresh_workbench_if_active(self.name)

        def cleanup():
            try:
                Gui.removeWorkbenchManipulator(manipulator)
            except Exception:
                pass
            if refresh:
                _refresh_workbench_if_active(self.name)

        if key is not None:
            self.own(key, manipulator, cleanup)
        else:
            self.onDispose(cleanup)
        return manipulator

    def setTaskWatcher(self, watchers):
        self._ensure_active()
        Gui.Control.clearTaskWatcher()
        Gui.Control.addTaskWatcher(watchers)

        def cleanup():
            try:
                Gui.Control.clearTaskWatcher()
            except Exception:
                pass

        self.onDispose(cleanup)
        return watchers

    def addTimer(self, timer, *, stop: bool = True, delete: bool = False):
        self._ensure_active()

        def cleanup():
            if stop and hasattr(timer, "stop"):
                try:
                    timer.stop()
                except Exception:
                    pass
            if delete and hasattr(timer, "deleteLater"):
                try:
                    timer.deleteLater()
                except Exception:
                    pass

        self.onDispose(cleanup)
        return timer

    def overrideActionShortcut(self, key: str, action, shortcut) -> None:
        self._ensure_active()
        previous_shortcut = action.shortcut()
        action.setShortcut(shortcut)

        def cleanup():
            try:
                action.setShortcut(previous_shortcut)
            except Exception:
                pass

        self.own(key, previous_shortcut, cleanup)

    def dispose(self) -> None:
        if self._disposed:
            return

        self._disposed = True
        while self._owned:
            _key, (_value, cleanup) = self._owned.popitem()
            if cleanup is not None:
                self._run_cleanup(cleanup)

        while self._cleanup:
            cleanup = self._cleanup.pop()
            self._run_cleanup(cleanup)

        self.data.clear()


def _runtime_tables():
    runtimes = getattr(Gui, "_pythonWorkbenchRuntimes", None)
    if runtimes is None:
        runtimes = {}
        Gui._pythonWorkbenchRuntimes = runtimes

    generations = getattr(Gui, "_pythonWorkbenchRuntimeGenerations", None)
    if generations is None:
        generations = {}
        Gui._pythonWorkbenchRuntimeGenerations = generations

    return runtimes, generations


def workbenchRuntime(name: str | None = None) -> WorkbenchRuntime:
    runtime_name = _resolve_workbench_runtime_name(name)
    if not runtime_name:
        raise RuntimeError("No active Python workbench runtime")
    runtimes, generations = _runtime_tables()
    runtime = runtimes.get(runtime_name)
    if runtime is None or runtime._disposed:
        generation = generations.get(runtime_name, 0) + 1
        generations[runtime_name] = generation
        runtime = WorkbenchRuntime(runtime_name, generation)
        runtimes[runtime_name] = runtime
    return runtime


def findWorkbenchRuntime(name: str | None = None) -> WorkbenchRuntime | None:
    runtime_name = _resolve_workbench_runtime_name(name)
    if not runtime_name:
        return None

    runtimes, _generations = _runtime_tables()
    runtime = runtimes.get(runtime_name)
    if runtime is None or runtime._disposed:
        return None
    return runtime


class SessionRuntime(WorkbenchRuntime):
    def __init__(self, workbench_name: str, session_name: str, generation: int):
        super().__init__(f"{workbench_name}:{session_name}", generation)
        self.workbench_name = workbench_name
        self.session_name = session_name


def _resolve_session_runtime_target(
    name: str,
    workbench_name: str | None = None,
) -> tuple[str, str, str]:
    if not name:
        raise RuntimeError("Session runtime name is required")

    if workbench_name is None and ":" in name:
        explicit_workbench, explicit_name = name.split(":", 1)
        if explicit_workbench and explicit_name:
            workbench_name = explicit_workbench
            name = explicit_name

    runtime_workbench_name = _resolve_workbench_runtime_name(workbench_name)
    if not runtime_workbench_name:
        raise RuntimeError("Session runtime parent workbench is required")

    return runtime_workbench_name, name, f"{runtime_workbench_name}:{name}"


def _session_runtime_table() -> dict[str, SessionRuntime]:
    runtimes = getattr(Gui, "_pythonSessionRuntimes", None)
    if runtimes is None:
        runtimes = {}
        Gui._pythonSessionRuntimes = runtimes
    return runtimes


def sessionRuntime(name: str, workbench_name: str | None = None) -> SessionRuntime:
    runtime_workbench_name, session_name, qualified_name = _resolve_session_runtime_target(
        name,
        workbench_name,
    )
    runtimes = _session_runtime_table()
    runtime = runtimes.get(qualified_name)
    if runtime is None or runtime._disposed:
        runtime = SessionRuntime(runtime_workbench_name, session_name, generation=0)
        runtimes[qualified_name] = runtime
    return runtime


def findSessionRuntime(name: str, workbench_name: str | None = None) -> SessionRuntime | None:
    if not name:
        return None

    try:
        _runtime_workbench_name, _session_name, qualified_name = _resolve_session_runtime_target(
            name,
            workbench_name,
        )
    except Exception:
        return None

    runtime = _session_runtime_table().get(qualified_name)
    if runtime is None or runtime._disposed:
        return None
    return runtime


def disposeSessionRuntime(name: str, workbench_name: str | None = None) -> bool:
    if not name:
        return False

    try:
        _runtime_workbench_name, _session_name, qualified_name = _resolve_session_runtime_target(
            name,
            workbench_name,
        )
    except Exception:
        return False

    runtime = _session_runtime_table().pop(qualified_name, None)
    if runtime is None:
        return False

    runtime.dispose()
    return True


def _disposeSessionRuntimes(workbench_name: str | None = None) -> int:
    runtimes = _session_runtime_table()
    if workbench_name:
        runtime_workbench_name = _resolve_workbench_runtime_name(workbench_name) or workbench_name
        names = [
            name
            for name, runtime in list(runtimes.items())
            if runtime.workbench_name == runtime_workbench_name
        ]
    else:
        names = list(runtimes)

    disposed = 0
    for name in names:
        if disposeSessionRuntime(name):
            disposed += 1
    return disposed


def disposeWorkbenchRuntime(name: str | None = None) -> bool:
    runtime_name = _resolve_workbench_runtime_name(name)
    if not runtime_name:
        return False

    runtimes, _generations = _runtime_tables()
    runtime = runtimes.pop(runtime_name, None)
    if not runtime:
        return False

    runtime.dispose()
    return True


def reloadPythonWorkbench(name: str | None = None) -> list[str]:
    """
    Reload a Python workbench by removing its handler, clearing imported modules, rerunning the
    workbench bootstrap, and restoring the previous active workbench when needed.
    """

    target = _resolve_reload_target(name)
    workbench_name = target["name"]
    was_active = _active_workbench_name() == workbench_name

    if hasattr(Gui, "resetWorkbench"):
        if not Gui.resetWorkbench(workbench_name):
            raise RuntimeError(f"Resetting workbench '{workbench_name}' failed")
    else:
        if was_active:
            Gui.activateWorkbench("NoneWorkbench")
        Gui.removeWorkbench(workbench_name)

    _disposeSessionRuntimes(_resolve_workbench_runtime_name(workbench_name) or workbench_name)
    disposeWorkbenchRuntime(workbench_name)

    if target["kind"] == "Dir":
        removed = _purge_dir_mod_modules(target["base_path"], target.get("module_prefix"))
        loader = DirModGui(target["mod"])
        sub_workbench = (
            None
            if target["source_path"].parent == target["mod"].path.resolve()
            else target["source_path"].parent
        )
        ok = loader.run_init_gui(sub_workbench)
    else:
        removed = _purge_modules_by_prefix(target["module_prefix"])
        loader = ExtModGui(target["mod"])
        ok = loader.run_init_gui()

    if not ok:
        raise RuntimeError(f"Reloading workbench '{workbench_name}' failed")

    if was_active:
        Gui.activateWorkbench(workbench_name)

    return removed


def canReloadPythonWorkbench(name: str | None = None) -> bool:
    try:
        _resolve_reload_target(name)
    except Exception:
        return False
    return True


def _canonical_auto_reload_name(name: str | None = None) -> str | None:
    if name is None:
        return _active_workbench_name()

    try:
        return _resolve_reload_target(name)["name"]
    except Exception:
        return _resolve_workbench_runtime_name(name) or name


class _PythonWorkbenchAutoReloader:
    WATCHED_SUFFIXES = {
        ".py",
        ".pyi",
        ".qrc",
        ".qss",
        ".svg",
        ".ui",
        ".xml",
    }
    EXCLUDED_DIRS = _DIR_MOD_AUTO_RELOAD_EXCLUDED_DIRS

    def __init__(self, name: str, debounce_ms: int = 500):
        self.name = name
        self.debounce_ms = debounce_ms
        self.QtCore = _qt_core()
        self.watcher = self.QtCore.QFileSystemWatcher()
        self.timer = self.QtCore.QTimer()
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.reload)
        self.watcher.fileChanged.connect(self.schedule_reload)
        self.watcher.directoryChanged.connect(self.schedule_reload)
        self.rearm()

    def _iter_watch_paths(self) -> typing.Iterable[str]:
        target = _resolve_reload_target(self.name)
        yield from _iter_watch_paths_for_base(
            target["base_path"],
            self.WATCHED_SUFFIXES,
            self.EXCLUDED_DIRS,
        )

    def rearm(self):
        current = self.watcher.files() + self.watcher.directories()
        if current:
            self.watcher.removePaths(current)

        paths = []
        for path in sorted(set(self._iter_watch_paths())):
            if Path(path).exists():
                paths.append(path)

        if paths:
            self.watcher.addPaths(paths)

    def schedule_reload(self, _path):
        self.timer.start(self.debounce_ms)

    def reload(self):
        try:
            removed = reloadPythonWorkbench(self.name)
        except Exception as ex:
            Err(f"Auto reload failed for {self.name}: {ex!s}\n")
            Log(traceback.format_exc())
        else:
            Log(f"Auto reloaded {self.name} " f"({len(removed)} Python modules cleared)\n")
        finally:
            self.rearm()

    def stop(self):
        self.timer.stop()
        current = self.watcher.files() + self.watcher.directories()
        if current:
            self.watcher.removePaths(current)


def startPythonWorkbenchAutoReload(name: str | None = None, debounce_ms: int = 500) -> None:
    target = _resolve_reload_target(name)
    reloaders = getattr(Gui, "_pythonWorkbenchReloaders", {})
    existing = reloaders.get(target["name"])
    if existing:
        existing.stop()

    reloader = _PythonWorkbenchAutoReloader(target["name"], debounce_ms=debounce_ms)
    reloaders[target["name"]] = reloader
    Gui._pythonWorkbenchReloaders = reloaders

    Log(f"Started Python auto reload for {target['name']}\n")


def stopPythonWorkbenchAutoReload(name: str | None = None) -> bool:
    reloaders = getattr(Gui, "_pythonWorkbenchReloaders", {})
    target_name = _canonical_auto_reload_name(name)
    if not target_name:
        return False

    reloader = reloaders.pop(target_name, None)
    if not reloader:
        return False

    reloader.stop()
    Gui._pythonWorkbenchReloaders = reloaders
    Log(f"Stopped Python auto reload for {target_name}\n")
    return True


def isPythonWorkbenchAutoReloadActive(name: str | None = None) -> bool:
    reloaders = getattr(Gui, "_pythonWorkbenchReloaders", {})
    target_name = _canonical_auto_reload_name(name)
    if not target_name:
        return False
    return target_name in reloaders


def _report_python_workbench_reload_limitations(workbench_name: str) -> None:
    App.Console.PrintMessage(
        f"{workbench_name} reload note: existing Python document proxies/view providers may still "
        "need recreation, and C++ changes still require a rebuild/restart.\n"
    )


class Std_ReloadActivePythonWorkbench:
    def GetResources(self):
        return {
            "Pixmap": "",
            "MenuText": QT_TRANSLATE_NOOP(
                "Std_ReloadActivePythonWorkbench",
                "Reload Active Python Workbench",
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Std_ReloadActivePythonWorkbench",
                "Reload the active reloadable Python workbench without restarting FreeCAD."
                " Existing Python document proxies or C++ changes are not hot-reloaded.",
            ),
        }

    def IsActive(self):
        return canReloadPythonWorkbench()

    def Activated(self):
        target = _resolve_reload_target()
        workbench_name = target["name"]
        removed = reloadPythonWorkbench(workbench_name)
        App.Console.PrintMessage(
            f"Reloaded {workbench_name} ({len(removed)} Python modules cleared)\n"
        )
        _report_python_workbench_reload_limitations(workbench_name)


class Std_ToggleActivePythonWorkbenchAutoReload:
    def GetResources(self):
        return {
            "Pixmap": "",
            "MenuText": QT_TRANSLATE_NOOP(
                "Std_ToggleActivePythonWorkbenchAutoReload",
                "Toggle Active Python Workbench Auto Reload",
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Std_ToggleActivePythonWorkbenchAutoReload",
                "Start or stop file-watch auto reload for the active reloadable Python workbench."
                " Existing Python proxies/view providers are not refreshed automatically.",
            ),
        }

    def IsActive(self):
        return canReloadPythonWorkbench() and hasattr(Gui, "stopPythonWorkbenchAutoReload")

    def Activated(self):
        target = _resolve_reload_target()
        workbench_name = target["name"]
        if isPythonWorkbenchAutoReloadActive(workbench_name):
            stopPythonWorkbenchAutoReload(workbench_name)
            App.Console.PrintMessage(f"Stopped Python auto reload for {workbench_name}\n")
        else:
            startPythonWorkbenchAutoReload(workbench_name)
            App.Console.PrintMessage(f"Started Python auto reload for {workbench_name}\n")
            _report_python_workbench_reload_limitations(workbench_name)


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
        Log(f"Init:      Packaged workbench {workbench_metadata.Name} specified icon\
            in class {workbench_metadata.Classname}")
        Log(" ... replacing with icon from package.xml data.\n")
    wb_handle.__dict__["Icon"] = str(absolute_filename.resolve())


# signal that the gui is up
App.GuiUp = 1
App.Gui = FreeCADGui
FreeCADGui.Workbench = Workbench
Gui.workbenchRuntime = workbenchRuntime
Gui.findWorkbenchRuntime = findWorkbenchRuntime
Gui.disposeWorkbenchRuntime = disposeWorkbenchRuntime
Gui.sessionRuntime = sessionRuntime
Gui.findSessionRuntime = findSessionRuntime
Gui.disposeSessionRuntime = disposeSessionRuntime
Gui.canReloadPythonWorkbench = canReloadPythonWorkbench
Gui.reloadPythonWorkbench = reloadPythonWorkbench
Gui.startPythonWorkbenchAutoReload = startPythonWorkbenchAutoReload
Gui.stopPythonWorkbenchAutoReload = stopPythonWorkbenchAutoReload
Gui.isPythonWorkbenchAutoReloadActive = isPythonWorkbenchAutoReloadActive
Gui.addCommand("Std_ReloadActivePythonWorkbench", Std_ReloadActivePythonWorkbench())
Gui.addCommand(
    "Std_ToggleActivePythonWorkbenchAutoReload",
    Std_ToggleActivePythonWorkbenchAutoReload(),
)

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
FreeCAD.addImportType("Python (*.py *.FCMacro *.fcmacro *.FCScript *.fcscript)", "FreeCADGui")
FreeCAD.addExportType("Inventor V2.1 (*.iv)", "FreeCADGui")
FreeCAD.addExportType("VRML V2.0 (*.wrl *.vrml *.wrz *.wrl.gz)", "FreeCADGui")
FreeCAD.addExportType("X3D Extensible 3D (*.x3d *.x3dz)", "FreeCADGui")
FreeCAD.addExportType("WebGL/X3D (*.xhtml)", "FreeCADGui")
FreeCAD.addTranslatableExportType(
    translate("FileFormat", "Portable Document Format"), ["pdf"], "FreeCADGui"
)
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
    del ModGui
    del typing, re
