2020 February

These files define the view provider classes of the "scripted objects"
defined by the workbench. These scripted objects are originally
defined in the big `Draft.py` file.

Each scripted object has a creation command like `make_arc`, 
a proxy class like `Arc`, and a view provider like `ViewProviderArc`.
The view providers define the code that indicates how they are displayed
in the tree view and in the 3D view, and visual properties
such as line thickness, line color, face color, and transparency.
These properties are only available when the graphical interface exists,
otherwise they are ignored.

Each scripted object in `draftobjects/` should import its corresponding
view provider from this directory as long the graphical interface
is available.

These modules should be split from the big `Draft.py` module.

At the moment the files in this directory aren't really used,
but are used as placeholders for when the migration of classes happens.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecadweb.org/viewtopic.php?f=23&t=38593&start=10#p341298)
