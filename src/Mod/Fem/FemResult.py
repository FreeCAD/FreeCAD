#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 - Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk> *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__ = "DocumentOject Class to hold Fem result"
__author__ = "Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

import FreeCAD
import Fem

def makeFemResult(result_obj_name):
    obj= FreeCAD.ActiveDocument.addObject('Fem::FemResultObjectPython', result_obj_name)
    # detect domain later, CFD or Mechanical later
    _MechanicalResult(obj)
    if FreeCAD.GuiUp:
        from _ViewProviderFemResult import _ViewProviderFemResult
        _ViewProviderFemResult(obj.ViewObject)
    return obj

class _MechanicalResult(object):
    def __init__(self, obj):
        self.Type = "MechanicalResult"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        # `Time, Stats` should have been defined in base cpp class
        obj.addProperty("App::PropertyVectorList", "DisplacementVectors", "Fem",
                                "List of displacement vectors", True)  # does not show up in propertyEditor of combiView
        obj.addProperty("App::PropertyVectorList", "StressVectors", "Fem",
                                "List of stress vectors", True)  # does not show up in propertyEditor of combiView
        obj.addProperty("App::PropertyVectorList", "StrainVectors", "Fem",
                                "List of strain vectors", True)  # does not show up in propertyEditor of combiView
        obj.addProperty("App::PropertyFloatList", "DisplacementLengths", "Fem",
                                "List of displacement lengths", True)  # readonly in propertyEditor of combiView
        obj.addProperty("App::PropertyFloatList", "StressValues", "Fem",
                                "", True)
        obj.addProperty("App::PropertyFloatList", "PrincipalMax", "Fem",
                                "", True)
        obj.addProperty("App::PropertyFloatList", "PrincipalMed", "Fem",
                                "", True)
        obj.addProperty("App::PropertyFloatList", "PrincipalMin", "Fem",
                                "", True)
        obj.addProperty("App::PropertyFloatList", "MaxShear", "Fem",
                                "List of Maximum Shear stress values", True)
        obj.addProperty("App::PropertyFloatList", "UserDefined", "Fem",
                                "User Defined Results", True)
        # temperature field is needed in the thermal stress analysis
        obj.addProperty("App::PropertyFloatList", "Temperature", "Fem",
                                    "Temperature field", True)
        # for frequency analysis
        obj.addProperty("App::PropertyInteger", "Eigenmode", "Fem",
                                "", True)
        obj.addProperty("App::PropertyFloat", "EigenmodeFrequency", "Fem",
                                "User Defined Results", True)

    ############ standard FeutureT methods ##########
    def execute(self, obj):
        """"this method is executed on object creation and whenever the document is recomputed"
        update Part or Mesh should NOT lead to recompution of the analysis automatically, time consuming
        """
        return

    def onChanged(self, obj, prop):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
