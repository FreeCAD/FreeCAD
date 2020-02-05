2020 February

These files provide the logic behind the task panel of the GuiCommands
defined in `draftguitools/`.

The task panel graphical interface is properly defined in
the `Resources/ui/` files, which are made with QtCreator.

There are many commands which aren't defined in `draftguitools/`.
These are defined in the big `DraftGui.py` module, which needs to be split
into individual GuiCommands, and each should have its own dedicated
`.ui` file.

