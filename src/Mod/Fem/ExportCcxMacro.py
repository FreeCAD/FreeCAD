########### MACRO #############
import os
import FreeCADGui
import FreeCAD
import shutil
from PySide import QtGui

### START OF MACRO ###

# Mehcanical Analysis Select
sel = FreeCADGui.Selection.getSelection() # Selection
sel1=sel[0]

# Save folder select
dialog = QtGui.QFileDialog.getExistingDirectory()
destiny_folder = str(dialog)

# Proceed
if sel1.TypeId == 'Fem::FemAnalysisPython':
  try:
     dir1 = sel1.Document.TransientDir #Temporaly Directoy
     nam_fold = sel1.Uid[32:] # Analysis temporaly fold name
     # Analysis Final Directory
     direc = str(dir1 + '/FemAnl_' + nam_fold + '/')
     calculix_files = os.listdir(direc)
     for files in calculix_files:
          shutil.copy(direc + files,destiny_folder)
     FreeCAD.Console.PrintMessage('Mechanical Analysis files save in' + destiny_folder)
  except:
     FreeCAD.Console.PrintError('Sorry but none temporaly file exists')
else:
  FreeCAD.Console.PrintError('Error in Selection: Select a correct Mechanical Analysis')
