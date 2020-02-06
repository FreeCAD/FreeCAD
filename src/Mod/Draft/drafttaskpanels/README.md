2020 February

These files provide the logic behind the task panels of the "GuiCommands"
defined in `draftguitools/`.

These files should not have code to create the task panel windows
and widgets manually. These interfaces should be properly defined
in the `.ui` files made with QtCreator and placed inside
the `Resources/ui/` directory.

There are many commands which aren't defined in `draftguitools/`,
and which therefore don't have an individual task panel.
These commands are defined in the big `DraftTools.py` file,
and their task panels are manually written in the large `DraftGui.py` module.
Therefore, these commands should be split into individual files,
and each should have its own `.ui` file.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecadweb.org/viewtopic.php?f=23&t=38593&start=10#p341298)
