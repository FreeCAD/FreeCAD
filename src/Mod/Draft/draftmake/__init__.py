# ***************************************************************************
# *   (c) 2020 Carlo Pavan <carlopav@gmail.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Modules that contain functions to create custom scripted objects.

These functions represent the basic application programming interface (API)
of the Draft Workbench to create custom objects.

These functions first create a simple, extensible object defined in C++,
for example, `Part::FeaturePython` or `Part::Part2DObjectPython`,
and then assign a custom proxy class to define its data properties,
and a custom viewprovider class to define its visual properties.

The proxy class is imported from `draftobjects` and the corresponding
viewprovider from `draftviewproviders`.

::

    import FreeCAD as App
    import draftobjects.my_obj as my_obj
    import draftviewproviders.view_my_obj as view_my_obj

    def make_function(input_data):
        doc = App.ActiveDocument
        new_obj = doc.addObject("Part::Part2DObjectPython",
                                "New_obj")
        my_obj.Obj(new_obj)

        if App.GuiUp:
            view_my_obj.ViewProviderObj(new_obj.ViewObject)

        return new_obj

Assigning the viewprovider class is only possible if the graphical interface
is available. In a terminal-only session the viewproviders are not accessible
because the `ViewObject` attribute does not exist.

If an object is created and saved in a console-only session,
the object will not have the custom viewprovider when the file is opened
in the GUI version of the program; it will only have a basic viewprovider
associated to the base C++ object.

If needed, the custom viewprovider can be assigned after opening
the GUI version of the program; then the object will have access
to additional visual properties.

::

    import Draft
    Draft.ViewProviderDraft(new_obj.ViewObject)

Most Draft objects are based on two base `Part` objects
that are able to handle a `Part::TopoShape`.

- `Part::Part2DObjectPython` if they should handle only 2D shapes, and
- `Part::FeaturePython` if they should handle any type of shape (2D or 3D).

Generic objects that don't need to handle shapes but which
can have other properties like `App::PropertyPlacement`,
or which can interact with the 3D view through Coin,
can be based on the simple `App::FeaturePython` object.
"""
## \defgroup draftmake draftmake
# \ingroup DRAFT
# \brief Modules with functions to create the custom scripted objects.
