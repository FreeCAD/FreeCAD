print("Loaded file TODO.py")
import os
import FreeCAD as App
from PySide import QtGui
from PySide import QtCore

from SafeViewer import SafeViewer

"""
TODO for this project:
OK find a way to use the FreeCAD 3D viewer without segfaults or disappearing widgets
OK fix sync problem when moving too fast
OK split the list of tools vs. document objects
OK save to disk the list of tools
OK always display including when switching workbenches
OK slightly larger popup widget to avoid scrollbar for the extra info for document objects
OK turn this into a standalone mod
OK Optimize so that it's not so slow
OK speed up startup to show the box instantly and do the slow loading on first click.
OK One small bug: when the 3D view is initialized, it causes a loss of focus on the drop-down. We restore it, but the currently-selected index is left unchanged, so the down or up arrow has to be pressed twice.
* split into several files, try to keep the absolute minimum of code possible in the main file to speed up startup
OK segfault when reloading
* Disable the spacebar shortcut (can't type space in the search field…)
* Possibly disable the home and end, and use ctrl+home and ctrl+end instead?
"""