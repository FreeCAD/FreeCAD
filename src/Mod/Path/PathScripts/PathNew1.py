# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 -> Developer Info Here <-                          *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# Visit a FreeCAD long-time contributor, sliptonic, page at
# https://github.com/sliptonic/FreeCAD/wiki/Developing-for-Path
# for additional information regarding developing for the Path Workbench.

from __future__ import print_function  # Must be at the beginning of the file per Python protocol

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp

from PySide import QtCore

__title__ = "Path New 1 Operation"
__author__ = "username (Iamthe Author)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of New 1 operation."
__contributors__ = ""  # additional contributors may put handles, usernames, or names here
__created__ = "2019"
__scriptVersion__ = "1a"  # Update script version with each iteration of the file
__lastModified__ = "2019-08-06 17:37 CST"  # Update with date and time of last modification

LOGLEVEL = False  # Set to 'True' if you wish to enable PathLog.debug() comments inserted throughout code

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


# Change the 'ObjectNew1' class name to that of your new operation created here
class ObjectNew1(PathOp.ObjectOp):
    '''Proxy object for New 1 operation.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def opFeatures(self, obj):
        '''opFeatures(obj)
        Return all standard features necessary for this operation
        A list of standard features available is found in the PathOp.py module'''
        # Edit PathSelection.py module to change feture geometry accepted for PathOp.FeatureBaseGeometry 
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseGeometry 

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create facing specific properties
        Operation-specific properties are generally REQUIRED to be declared in this initOperation() method, with a few exceptions.
        Declare operation-specific properties required for basic functionality of the operation, independent of GUI availability.
        You do NOT need to declare properties necessary for the operation if they are declared in standard features returned in the opFeatures() method.
        The PathNew1Gui.py module will contain code to connect properties declared here with the GUI editor window and GUI system in general.

        Some templates for property declarations.  More property types are available - see PathSetupSheetOpPrototype.py module.
            obj.addProperty("App::PropertyEnumeration", "Selection_List", "Section_Name_In_Properties_View_Data_Tab", QtCore.QT_TRANSLATE_NOOP("App::Property", "Description of property."))
            obj.addProperty("App::PropertyFloat", "Float_Value", "Section_Name_In_Properties_View_Data_Tab", QtCore.QT_TRANSLATE_NOOP("App::Property", "Description of property."))
            obj.addProperty("App::PropertyPercent", "Integer_Percent", "Section_Name_In_Properties_View_Data_Tab", QtCore.QT_TRANSLATE_NOOP("App::Property", "Description of property."))
            obj.addProperty("App::PropertyDistance", "Float_Value_With_Units", "Section_Name_In_Properties_View_Data_Tab", QtCore.QT_TRANSLATE_NOOP("App::Property", "Description of property."))
            obj.addProperty("App::PropertyBool", "Checkbox", "Section_Name_In_Properties_View_Data_Tab", QtCore.QT_TRANSLATE_NOOP("App::Property", "Description of property."))
        '''
        obj.addProperty("App::PropertyEnumeration", "Combo1", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Combo box with choices: c1A, c1B, c1C, c1D."))
        obj.addProperty("App::PropertyEnumeration", "Combo2", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Combo box with choices: c2A, c2B, c2C, c2D."))
        obj.addProperty("App::PropertyFloat", "Spin1", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Float value spin box - NO UNITS"))
        obj.addProperty("App::PropertyFloat", "Spin2", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Float value spin box - NO UNITS"))
        obj.addProperty("App::PropertyDistance", "Line1", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Float value text entry that is unit aware."))
        obj.addProperty("App::PropertyDistance", "Line2", "Section 1", QtCore.QT_TRANSLATE_NOOP("App::Property", "Float value text entry that is unit aware."))

        obj.addProperty("App::PropertyBool", "Checkbox1", "Section 2", QtCore.QT_TRANSLATE_NOOP("App::Property", "Simple checkbox"))
        obj.addProperty("App::PropertyBool", "Checkbox2", "Section 2", QtCore.QT_TRANSLATE_NOOP("App::Property", "Simple checkbox"))

        # Define list of selections for properties of type: App::PropertyEnumeration
        obj.Combo1 = ['c1A', 'c1B', 'c1C', 'c1D']
        obj.Combo2 = ['c2A', 'c2B', 'c2C', 'c2D']

        # Default values for properties are set in a separate method

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job)
        Initialize default values for properties. The Job object is readily available.'''

        obj.Combo1 = 'c1A'
        obj.Combo2 = 'c2B'
        obj.Spin1 = -36.28
        obj.Spin2 = 105.2
        obj.Line1.Value = -1.0  # .Value attribute must be used because of property type: PropertyDistance  (units aware). Value here must be the FreeCAD standard unit (metric)
        obj.Line2 = '2.54 cm'  # If you do not use the .Value attribute for a direct FreeCAD standard quantity, place value and units in string.
        obj.Checkbox1 = False
        obj.Checkbox1 = True

    def setEditorProperties(self, obj):
        '''setEditorProperties(obj)
        This method is dedicated to setting editor properties: 2=hide, 1=show without editing, 0=show with full editing of values'''
        # Used to limit editing, hide, or reveal with editing the properties declared in initOperation() method
        if obj.Combo1 == 'c1D':
            obj.setEditorMode('Combo2', 2)  # 2 = hidden in Properties View Data tab
            obj.setEditorMode('Checkbox1', 0)  # 0 = visible and editable in Properties View Data tab
        else:
            obj.setEditorMode('Combo2', 0)  # 2 = hidden in Properties View Data tab
            obj.setEditorMode('Checkbox1', 2)  # 0 = visible and editable in Properties View Data tab

        if obj.Checkbox1 is True:
            obj.setEditorMode('Line2', 2)  # 2 = hidden in Properties View Data tab
        else:
            obj.setEditorMode('Line2', 1)  # 1 = visible WITHOUT editing in Properties View Data tab

    def onChanged(self, obj, prop):
        '''onChanged(obj, prop)
        This method is called each time the operation is changed or edited.'''
        if prop == "Combo1" or prop == "Checkbox1":
            self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        '''opOnDocumentRestored(obj)
        This method is called when the document is re-opened or restored.'''
        self.setEditorProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... main exectable code for New 1 operation'''
        PathLog.track()
        # Locate parent job object and return reference to that object
        Job = PathUtils.findParentJob(obj)
        # Access the SetupSheet of the Job object, saving a value to a local variable
        SafeHeightOffset = Job.SetupSheet.SafeHeightOffset.Value

        # Sample function call for adding debug comments when "LOGLEVEL = True"
        PathLog.debug("SafeHeightOffset: {}".format(SafeHeightOffset))

        # This new operation will likely generate g-code. Collect the commands and append them to self.commandlist
        New1_operation_result = []
        New1_operation_result.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        New1_operation_result.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        # Gcode commands created in this operation should be added to self.commandlist
        self.commandlist.extend(New1_operation_result)

        # return value is not required ??


def SetupProperties():
    '''SetupProperties()
    All properties declared newly in this operation/module - excluding those included
        in standard features in opFeatures() - must be added to the setup list here
        for proper GUI functionality'''
    setup = []
    setup.append("Combo1")
    setup.append("Combo2")
    setup.append("Spin1")
    setup.append("Spin2")
    setup.append("Line1")
    setup.append("Line2")
    setup.append("Checkbox1")
    setup.append("Checkbox2")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    # Change the 'ObjectNew1' class name here to that of your new operation class created here
    # It should match the class definition in this module, above
    obj.Proxy = ObjectNew1(obj, name)
    return obj
