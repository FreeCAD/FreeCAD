2020 May

These files define the viewprovider classes for "scripted objects"
defined within the workbench. The corresponding proxy object classes
should be defined in the modules in `draftobjects/`.

Each scripted object has a creation function like `make_rectangle`,
a proxy class like `Rectangle`, and a viewprovider class
like `ViewProviderRectangle`.
The view providers define how they are displayed in the tree view
and in the 3D view, that is, visual properties such as line thickness,
line color, face color, and transparency.
These properties are only available when the graphical interface is loaded;
in a console only session, these properties cannot be read nor set.
These properties aren't very "real" because they don't affect the actual
geometrical shape of the object.
Each make function in `draftmake/` should import its corresponding
viewprovider from this package in order to set the visual properties
of the new scripted object, as long the graphical interface
is available.

These classes were previously defined in the `Draft.py` module,
which was very large. Now `Draft.py` is an auxiliary module
which just loads the individual classes in order to provide backwards
compatibility for older files.
Other workbenches can import these classes for their own use,
including creating derived classes.
```py
import Draft

new_obj = App.ActiveDocument.addObject("Part::Part2DObjectPython", "New")

if App.GuiUp:
    Draft.ViewProviderRectangle(new_obj.ViewObject)


# Subclass
class NewViewProvider(Draft.ViewProviderRectangle):
   ...
```

As the scripted objects are rebuilt every time a document is loaded,
this means that, in general, these modules cannot be renamed
without risking breaking previously saved files. They can be renamed
only if the old class can be migrated to point to a new class,
for example, by creating a reference to the new class named the same
as the older class.

For more information see the thread:
[[Discussion] Splitting Draft tools into their own modules](https://forum.freecad.org/viewtopic.php?f=23&t=38593&start=10#p341298)
