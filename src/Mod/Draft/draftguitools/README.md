# General

2020 May

These files define the "GuiCommands", that is, classes called in a graphical
way through either buttons, menu entries, or context actions.
They don't define the graphical interfaces themselves, but set up
the interactions that allow running Python functions
when graphical data is provided, for example, when points or objects
are selected in the 3D view.

There is no need to check for the existence of the graphical interface (GUI)
when loading these classes as these classes only work with the GUI
so we must assume the interface is already available.
These command classes are normally loaded by `InitGui.py`
during the initialization of the workbench.

These tools were previously defined in the `DraftTools.py` module,
which was very large. Now `DraftTools.py` is an auxiliary module
which just loads the individual tool classes; therefore, importing
`DraftTools.py` in `InitGui.py` is sufficient to make all commands
of the Draft Workbench accessible to the system.
Then the toolbars can be defined with the command names.

The classes defined here internally use the public functions provided
by `Draft.py`, which are ultimately defined in the modules
under `draftutils/`, `draftfunctions/`, and `draftmake/`.

Those GUI commands that launch a "task panel" set up the respective widgets
in two different ways typically.
- "Old" commands set up the task panel from the `DraftGui.py` module.
- "New" commands call the respective class from a module in `drafttaskpanels/`,
which itself loads a `.ui` file (created with Qt Designer)
from `Resources/ui/`.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)

# To do

In the future each tool should have its own individual task panel file,
and its own `.ui` file.

This should be done by breaking `DraftGui.py`, creating many `.ui` files,
and making sure these GUI commands use them properly.
