The Draft module
================

The Draft module offer several convenient functions to work with simple 2D and 3D objects.
These functions can be used in scripts and macros or from the python interpreter, once the Draft module has been imported.

Example::

  import FreeCAD
  from Draft import *
  myrect = makeRectangle(4,3)
  mydistance = FreeCAD.Vector(2,2,0)
  move(myrect,mydistance)

.. toctree::
   :maxdepth: 4

.. automodule:: Draft
   :members:
