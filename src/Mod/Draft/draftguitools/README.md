2020 February

These files define the "GuiCommands", that is, classes called in a graphical
way through either buttons, menu entries, or context actions.
They don't define the graphical interfaces themselves, they just setup
tools that connect with FreeCAD's C++ code.

These tools should be split from the big `DraftTools.py` module.
The classes defined here internally use the GUI-less functions
defined in `Draft.py`, or in the newer modules under `draftobjects/`.

These tools are loaded by `InitGui.py`, and thus require the graphical
interface to exist.

Those commands that require a "task panel" call the respective module
and class in `drafttaskpanels/`. The task panel interfaces themselves
are defined inside the `Resources/ui/` files created with QtCreator.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecadweb.org/viewtopic.php?f=23&t=38593&start=10#p341298)
