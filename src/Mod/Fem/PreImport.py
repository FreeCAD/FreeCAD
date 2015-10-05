import os
#
import FreeCAD
from PySide import QtCore
from PySide.QtCore import Qt
import Fem

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtGui
    from PySide.QtGui import QApplication