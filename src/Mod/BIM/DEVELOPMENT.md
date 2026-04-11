# BIM Development Notes

## Python Workbench Reload

The BIM workbench supports reloading its Python-side code without restarting FreeCAD.

Command ids:
- `Std_ReloadActivePythonWorkbench`
- `Std_ToggleActivePythonWorkbenchAutoReload`

Python API:

```python
import FreeCADGui as Gui

Gui.reloadPythonWorkbench("BIMWorkbench")
Gui.startPythonWorkbenchAutoReload("BIMWorkbench")
Gui.stopPythonWorkbenchAutoReload("BIMWorkbench")
```

Reload support includes:
- rerunning `InitGui.py` and reimporting BIM Python modules
- resetting and rebuilding the Python workbench handler
- disposing workbench- and session-owned GUI state such as observers, manipulators, task watchers, and similar runtime-managed objects

These are generic active-workbench commands backed by `Gui.reloadPythonWorkbench(...)`
and the auto-reload APIs.

Out of scope:
- document proxies or view providers created from older Python classes; recreate affected objects after reload
- open task or dialog sessions; reopen them after reload
- C++ changes; rebuild and restart FreeCAD
