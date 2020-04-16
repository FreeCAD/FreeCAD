2020 February

These files provide auxiliary functions used by the Draft workbench.
Previously most of these were in `Draft.py`, `DraftTools.py`
and `DraftGui.py`.

In here we want modules with generic functions that can be used everywhere
in the workbench. We want these tools to depend only on standard modules
so that there are no circular dependencies, and so that they can be used
by all functions and graphical commands in this workbench
and possibly other workbenches.
- `utils`: basic functions
- `messages`: used to print messages
- `translate`: used to translate texts
- `init_tools`: used to initialize the workbench (toolbars and menus)
- `todo`: used to delay execution of certain graphical commands

Some auxiliary functions require that the graphical interface is loaded
as they deal with scripted objects' view providers or the 3D view.
- `gui_utils`: basic functions dealing with the graphical interface

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecadweb.org/viewtopic.php?f=23&t=38593&start=10#p341298)
