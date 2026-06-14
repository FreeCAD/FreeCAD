2020 May

These files define the proxy classes for "scripted objects"
defined within the workbench. The corresponding viewprovider classes
should be defined in the modules in `draftviewproviders/`.

Each scripted object has a creation function like `make_rectangle`,
a proxy class like `Rectangle`, and a viewprovider class
like `ViewProviderRectangle`.
The proxy classes define the code that manipulates the internal properties
of the objects, determining how the internal shape is calculated.
These properties are "real" information because they affect the actual
geometrical shape of the object.
Each make function in `draftmake/` should import its corresponding
proxy class from this package in order to build the new scripted object.

These classes were previously defined in the `Draft.py` module,
which was very large. Now `Draft.py` is an auxiliary module
which just loads the individual classes in order to provide backwards
compatibility for older files.
Other workbenches can import these classes for their own use,
including creating derived classes.
```py
import Draft

new_obj = App.ActiveDocument.addObject("Part::Part2DObjectPython", "New")
Draft.Rectangle(new_obj)


# Subclass
class NewObject(Draft.Rectangle):
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
