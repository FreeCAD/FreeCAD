2020 May

These modules contain the basic functions to create custom "scripted objects"
defined within the workbench.

Each scripted object has a "make function" like `make_rectangle`,
a proxy class like `Rectangle`, and a viewprovider class
like `ViewProviderRectangle`.
Each make function should import the two corresponding classes
in order to create a new object with the correct data
and visual properties for that object.
These classes should be defined in the modules in `draftobjects/`
and `draftviewproviders/`.

The make functions can be used in both graphical and non-graphical
modes (terminal only); in the latter case the viewprovider is not used.
The functions are also used internally by the graphical "GuiCommands"
(buttons, menu actions) defined in the modules in `draftguitools/`
and `drafttaskpanels/`.

These make functions were previously defined in the `Draft.py` module,
which was very large. Now `Draft.py` just loads the individual modules
in order to provide these functions under the `Draft` namespace.

```py
import Draft

new_obj1 = Draft.make_rectangle(...)
new_obj2 = Draft.make_circle(...)
new_obj3 = Draft.make_line(...)
```

The functions in the `Draft` namespace are considered to be the public
application programming interface (API) of the workbench, and should be
usable in scripts, macros, and other workbenches.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)
