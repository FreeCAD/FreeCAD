2020 February

These files define modules to create specific "scripted objects"
from the terminal, that is, without requiring the graphical interface.
They define both a creation command such as `make_arc`, and the corresponding
proxy class, such as `Arc`. The corresponding view provider class
should be defined in the modules in `draftviewproviders/`.

These modules should be split from the big `Draft.py` module.

At the moment the files in this directory aren't really used,
but are used as placeholders for when the migration of classes and functions
happens.

The creation functions should be used internally by the "GuiCommands"
defined in `DraftTools.py` or in the newer modules under `draftguitools/`
and `drafttaskpanels/`.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecadweb.org/viewtopic.php?f=23&t=38593&start=10#p341298)
