2020 May

These files provide auxiliary functions used by the Draft workbench.

Previously most of these functions were defined in `Draft.py`,
`DraftTools.py`, and `DraftGui.py`. However, due to being defined in these
big modules, it was impossible to use them individually without importing
the entire modules. So a decision was made to split the functions
into smaller modules.

In here we want modules with generic functions that can be used everywhere
in the workbench. We want these tools to depend only on standard Python
modules and basic FreeCAD methods so that there are no circular dependencies,
and so that they can be used by all functions and graphical commands
in this workbench, and others if possible.
- `utils`: basic functions
- `messages`: used to print messages
- `translate`: used to translate texts
- `init_tools`: used to initialize the workbench (toolbars and menus)
- `todo`: used to delay execution of certain graphical commands

Some auxiliary functions require that the graphical interface is loaded
as they deal with scripted objects' view providers or the 3D view.
- `gui_utils`: basic functions dealing with the graphical interface
- `init_draft_statusbar`: functions to initialize the status bar

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)
