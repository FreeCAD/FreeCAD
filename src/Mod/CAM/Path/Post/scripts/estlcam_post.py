# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Fae Corrigan <faecorrigandesign@gmail.com>         *
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

from typing import Any, Dict
from Path.Post.Processor import PostProcessor
import Path
import FreeCAD


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = Dict[str, Any]

POST_TYPE = 'machine'


class Estlcam(PostProcessor):
    coolant_End_Command = ''
    @classmethod
    def get_common_property_schema(cls):
        '''Return common properties with Estlcam defaults (uses base defaults).'''
        '''Override common properties with Estlcam-specific defaults.'''
        common_props = super().get_common_property_schema()

        for prop in common_props:
            if prop['name'] == 'file_extension':
                prop['default'] = 'cnc'
            elif prop['name'] == 'supports_tool_radius_compensation':
                prop['default'] = False
            elif prop['name'] == 'preamble':
                prop['default'] = 'G90 G94\nG17'
            elif prop['name'] == 'postamble':
                prop['default'] = 'M05\nG17 G90\nG28\nM30\n'
            elif prop['name'] == 'pre_tool_change':
                # Stop the spindle before every tool change.
                prop['default'] = 'M05'
            elif prop['name'] == 'output_tool_length_offset':
                # Makera has no G43 support.
                prop['default'] = False
            elif prop['name'] == 'translate_drill_cycles':
                # Makera can't run canned cycles; expand them to G0/G1/G4 moves.
                prop['default'] = True
            elif prop['name'] == 'drill_cycles_to_translate':
                prop['default'] = 'G81\nG82\nG83'
            elif prop['name'] == 'parameter_order':
                # Makera doesn't want K on the XY plane (arcs need work),
                # and has no H/D (tool length offset) or C axis support.
                prop['default'] = 'XYZABIJFSTQRL'
            elif prop['name'] == 'axis_precision':
                prop['default'] = 5
            elif prop['name'] == 'feed_precision':
                prop['default'] = 5
            elif prop['name'] == 'spindle_decimals':
                prop['default'] = 0

        return common_props

    @classmethod
    def get_property_schema(cls):
         return [
            {
                'name': 'TOOL_CHANGE_USE_ALTCMD',
                'type': 'bool',
                'runtime': False,
                'label': translate('CAM', 'Use Alternative Tool Change'),
                'default': False,
                'help': translate(
                    'CAM', 'Use alternative tool change command.'
                ),
            },
            {
                'name': 'min_feed_rate',
                'type': 'float',
                'label': translate('CAM', 'Minimum Feed Rate'),
                'default': 5.0,
                'min': 0.0,
                'max': 100000.0,
                'decimals': 2,
                'help': translate(
                    'CAM',
                    'Feed rates (in the current output units/min) below this value '
                    'abort posting - this usually indicates a missing feed rate.',
                ),
            },
            {
                'name': 'min_spindle_speed',
                'type': 'float',
                'label': translate('CAM', 'Minimum Spindle Speed'),
                'default': 5.0,
                'min': 0.0,
                'max': 20000.0,
                'decimals': 0,
                'help': translate(
                    'CAM',
                    'Spindle speeds (in RPM) below this value abort posting - this '
                    'usually indicates a missing spindle speed.',
                ),
            },
            ]

    def __init__(self, job):
        super().__init__(
            job=job,
            tooltip=translate('CAM', 'Estlcam post processor'),
            tooltipargs=[],
            units='Metric',
        )
        Path.Log.debug('Estlcam post processor initialized')

    def init_values(self, values: Values) -> None:
        '''Initialize values that are used throughout the postprocessor.'''
        #
        super().init_values(values)

        values['POSTPROCESSOR_FILE_NAME'] = __name__
        values['MACHINE_NAME'] = 'Estlcam'
        
        
        if self._machine and hasattr(self._machine, 'postprocessor_properties'):
            props = self._machine.postprocessor_properties
            values['TOOL_CHANGE_USE_ALTCMD'] = props.get('TOOL_CHANGE_USE_ALTCMD', False)
            values['MIN_FEED_RATE'] = props.get('min_feed_rate', 5.0)
            values['MIN_SPINDLE_SPEED'] = props.get('min_spindle_speed', 5.0)
        else:
            values['TOOL_CHANGE_USE_ALTCMD'] = False
            values['MIN_FEED_RATE'] = 5.0
            values['MIN_SPINDLE_SPEED'] = 5.0
            
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values['PREAMBLE'] = ''''''
        #
        # Any commands in this value will be output as the last commands
        # in the G-code file.
        #
        values['POSTAMBLE'] = '''M5'''
    
    def convert_command_to_gcode(self, command: Path.Command):
        props = self._machine.postprocessor_properties
        
        if command.Name == 'G21':
            return ''
        if command.Name in ('M7' , 'M07' ): 
            self.coolant_End_Command = 'M11'
            return 'M10'
        if command.Name in ('M8' , 'M08' ): 
            self.coolant_End_Command = 'M9'
            return 'M8'
        
        if command.Name in ('M9' , 'M09'): #expand M9 for flood, M11 for mist
            return self.coolant_End_Command
        if command.Name in ('M6','M06'):
            if self._machine.postprocessor_properties.get('TOOL_CHANGE_USE_ALTCMD', False):
                gcode_return = super().convert_command_to_gcode(command)
                return gcode_return.replace('M6', 'M0')
        return super().convert_command_to_gcode(command)
        
    @property
    def tooltip(self):

        tooltip = '''
        Generate G-code from a Path that is compatible with the Estlcam CNC controller.
        Have a look at https://www.estlcam.de/steuerung_cnc_programme_en.php
        '''
        return tooltip

    @property
    def tooltipArgs(self):
        argtooltip = super().tooltipArgs

        # One could add additional arguments here.
        # argtooltip += '''
        # --arg1: This is the first argument
        # --arg2: This is the second argument

        # '''
        return argtooltip

    @property
    def units(self):
        return self._units
