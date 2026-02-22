.. SPDX-License-Identifier: LGPL-2.1-or-later
.. SPDX-FileNotice: Part of the FreeCAD project.

The FreeCAD Document Object
===========================

.. toctree::
   :maxdepth: 4

.. automodule:: DocumentObject

   .. autoclass:: DocumentObject
      :members:

      .. method:: __setstate__(value)

         allows to save custom attributes of this object as strings, so they can be saved when saving the FreeCAD document

      .. method:: __getstate__()

         reads values previously saved with __setstate__()
