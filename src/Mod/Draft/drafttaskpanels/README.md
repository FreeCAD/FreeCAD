# General

2020 May

These files provide the logic behind the "task panels" of the "GuiCommands"
defined in `draftguitools/`.

These files should not have code to create the task panel widgets manually.
These interfaces should be properly defined in `.ui` files
made with Qt Designer and placed inside the `Resources/ui/` directory.

There are many GUI commands which are "old style" and thus don't have
an individual task panel. These commands use the task panel defined
in the big `DraftGui.py` module. This module defines many widgets
for many tools, and selectively chooses the widgets to show and to hide
depending on the command that is activated.

A big file that controls many widgets at the same time is difficult
to handle and to maintain because changing the behavior of one widget
may affect the operation of various GUI commands.
This must be changed so that in the future each tool has its own
individual task panel file, and its own `.ui` file.
Individual files are more maintainable because changes can be done
to a single tool without affecting the rest.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)

# To do

In the future each tool should have its own individual task panel file,
and its own `.ui` file.

This should be done by breaking `DraftGui.py`, creating many `.ui` files,
creating many task panel modules, and making sure these modules
are used correctly by the GUI commands.
