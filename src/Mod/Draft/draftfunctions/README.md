2020 May

These modules provide supporting functions for dealing
with the custom "scripted objects" defined within the workbench.

The functions are meant to be used in the creation step of the objects,
by the "make functions" in `draftmake/`, but also by the graphical
"Gui Commands" modules in `draftguitools/` and `drafttaskpanels/`.

These functions should deal with the internal shapes of the objects,
or other special properties. They should not be very generic;
if they are very generic then they are more appropriate to be included
in the modules in `draftutils/`.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)
