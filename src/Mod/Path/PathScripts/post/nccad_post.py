# *********************************************************************************************
# *   Copyright (c) 2019/2020 Rene 'Renne' Bartsch, B.Sc. Informatics <rene@bartschnet.de>    *
# *                                                                                           *
# *   This file is part of the FreeCAD CAx development system.                                *
# *                                                                                           *
# *   This program is free software; you can redistribute it and/or modify                    *
# *   it under the terms of the GNU Lesser General Public License (LGPL)                      *
# *   as published by the Free Software Foundation; either version 2 of                       *
# *   the License, or (at your option) any later version.                                     *
# *   for detail see the LICENCE text file.                                                   *
# *                                                                                           *
# *   FreeCAD is distributed in the hope that it will be useful,                              *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of                          *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                           *
# *   GNU Lesser General Public License for more details.                                     *
# *                                                                                           *
# *   You should have received a copy of the GNU Library General Public                       *
# *   License along with FreeCAD; if not, write to the Free Software                          *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307                    *
# *   USA                                                                                     *
# *                                                                                           *
# ********************************************************************************************/
import FreeCAD
import PathScripts
from PathScripts import PostUtils
import datetime


TOOLTIP='''
This is a postprocessor file for the Path workbench. It is used to take
a pseudo-gcode fragment outputted by a Path object and output real GCode
suitable for the Max Computer GmbH nccad9 Computer Numeric Control.

Supported features:

- 3-axis milling
- manual tool change with tool number as comment
- spindle speed as comment

!!! gCode files must use the suffix .knc !!!'''


MACHINE_NAME = '''Max Computer GmbH nccad9 MCS/KOSY'''


# gCode for changing tools
# M01 <String> ; Displays <String> and waits for user interaction
TOOL_CHANGE = '''G77      ; Move to release position
M10 O6.0 ; Stop spindle
M01 Insert tool TOOL
G76      ; Move to reference point to ensure correct coordinates after tool change
M10 O6.1 ; Start spindel'''


# gCode finishing the program
POSTAMBLE = '''G77      ; Move to release position
M10 O6.0 ; Stop spindle'''


# gCode header with information about CAD-software, post-processor and date/time
HEADER = ''';Exported by FreeCAD
;Post Processor: {}
;CAM file: {}
;Output Time: {}
'''.format(__name__, FreeCAD.ActiveDocument.FileName, str(datetime.datetime.now()))


def export(objectslist, filename, argstring):

  gcode = HEADER

  for obj in objectslist:

    for command in obj.Path.Commands:

      # Manipulate tool change commands
      if 'M6' == command.Name:
        gcode += TOOL_CHANGE.replace('TOOL', str(int(command.Parameters['T'])))

      # Convert spindle speed (rpm) command to comment
      elif 'M3' == command.Name:
        gcode += 'M01 Set spindle speed to ' + str(int(command.Parameters['S'])) + ' rounds per minute'

      # Add other commands
      else:
        gcode += command.Name

        # Loop through command parameters
        for parameter, value in command.Parameters.items():

          # Multiply F parameter value by 10 (FreeCAD = mm/s, nccad = 1/10 mm/s)
          if 'F' == parameter:
            value *= 10

          # Add command parameters and values and round float as nccad9 does not support exponents
          gcode += ' ' + parameter + str(round(value, 5))

      gcode += '\n'

  gcode += POSTAMBLE + '\n'

  # Open editor window
  if FreeCAD.GuiUp:
    dia = PostUtils.GCodeEditorDialog()
    dia.editor.setText(gcode)
    result = dia.exec_()
    if result:
      gcode = dia.editor.toPlainText()

  # Save to file
  if filename != '-':
    gfile = open(filename, "w")
    gfile.write(gcode)
    gfile.close()

  return filename
