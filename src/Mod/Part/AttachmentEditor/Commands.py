# ***************************************************************************
# *   Copyright (c) Victor Titov (DeepSOIC)                                 *
# *                                           (vv.titov@gmail.com) 2016     *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

from __future__ import absolute_import 

import FreeCAD as App

try:
    from PySide.QtCore import QT_TRANSLATE_NOOP
except ImportError:
    def QT_TRANSLATE_NOOP(ctx, msg):
        return msg

def editAttachment(feature = None, 
                   take_selection = False, 
                   create_transaction = True, 
                   callback_OK = None, 
                   callback_Cancel = None, 
                   callback_Apply = None):
    '''Opens attachment editing dialog.
    editAttachment(feature = None, 
                   take_selection = False, 
                   create_transaction = True, 
                   callback_OK = None, 
                   callback_Cancel = None, 
                   callback_Apply = None)
    feature: object to attach/modify. If None is supplied, the object is taken from 
    selection.
    take_selection: if True, current selection is filled into first references (but only 
    if object to be attached doesn't have any references already)
    create_transaction = if False, no undo transaction operations will be done by the 
    dialog (consequently, canceling the dialog will not reset the feature to original 
    state).
    callback_OK: function to be called upon OK. Invoked after writing values to feature, 
    committing transaction and closing the dialog.
    callback_Cancel: called after closing the dialog and aborting transaction.
    callback_Apply: invoked after writing values to feature.'''
    
    import AttachmentEditor.TaskAttachmentEditor as TaskAttachmentEditor
    global taskd # exposing to outside, for ease of debugging
    if feature is None:
        feature = Gui.Selection.getSelectionEx()[0].Object
    
    try:
        taskd = TaskAttachmentEditor.AttachmentEditorTaskPanel(feature, 
                                                               take_selection= take_selection, 
                                                               create_transaction= create_transaction,
                                                               callback_OK= callback_OK, 
                                                               callback_Cancel= callback_Cancel,
                                                               callback_Apply= callback_Apply)
        Gui.Control.showDialog(taskd)
    except TaskAttachmentEditor.CancelError:
        pass
    

class CommandEditAttachment:
    'Command to edit attachment'
    def GetResources(self):
        return {'Pixmap': 'Part_Attachment',
                'MenuText': QT_TRANSLATE_NOOP("AttachmentEditor","Attachment..."),
                'Accel': "",
                'ToolTip': QT_TRANSLATE_NOOP("AttachmentEditor","Edit attachment of selected object.")}
        
    def Activated(self):
        try:
            editAttachment()
        except Exception as err:
            from PySide import QtGui
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(str(err))
            mb.setWindowTitle("Error")
            mb.exec_()
        
    def IsActive(self):
        sel = Gui.Selection.getSelectionEx()
        if len(sel) == 1:
            if hasattr(sel[0].Object,"Placement"):
                return True
        return False

if App.GuiUp:
    global command_instance
    import FreeCADGui as Gui
    command_instance = CommandEditAttachment()
    Gui.addCommand('Part_EditAttachment', command_instance)
