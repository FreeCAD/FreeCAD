2020 February

These files define the GuiCommands, that is, actions called in a graphical
way, either buttons, menu entries, or context commands.

These tools should be split from the big `DraftTools.py` module.

These tools are initialized by `InitGui.py`, and require the graphical
interface to exist.

Those commands that require a "task panel" call the respective module
and class in `drafttaskpanels/`.

