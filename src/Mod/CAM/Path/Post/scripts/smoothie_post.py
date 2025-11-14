# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import os
import socket
import sys
from typing import Any, Dict, Optional

from Path.Post.Processor import PostProcessor

import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

#
# Define some types that are used throughout this file.
#
Values = Dict[str, Any]


class Smoothie(PostProcessor):
    """
    The SmoothieBoard post processor class.

    This postprocessor outputs G-code suitable for SmoothieBoard controllers.
    It supports direct network upload to the SmoothieBoard via TCP/IP.
    """

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Refactored SmoothieBoard post processor"),
        tooltipargs=["ip-addr", "verbose"],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Refactored SmoothieBoard post processor initialized.")
        self.ip_addr: Optional[str] = None
        self.verbose: bool = False

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        #
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        # The order of parameters.
        # SmoothieBoard doesn't want K properties on XY plane (like LinuxCNC).
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
        ]
        #
        # Used in the argparser code as the "name" of the postprocessor program.
        #
        values["MACHINE_NAME"] = "SmoothieBoard"
        #
        # Any commands in this value will be output as the last commands
        # in the G-code file.
        #
        values[
            "POSTAMBLE"
        ] = """M05
G17 G90
M2"""
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values["PREAMBLE"] = """G17 G90"""

    def init_arguments(self, values, argument_defaults, arguments_visible):
        """Initialize command-line arguments, including SmoothieBoard-specific options."""
        parser = super().init_arguments(values, argument_defaults, arguments_visible)

        # Add SmoothieBoard-specific argument group
        smoothie_group = parser.add_argument_group("SmoothieBoard-specific arguments")

        smoothie_group.add_argument(
            "--ip-addr", help="IP address for direct upload to SmoothieBoard (e.g., 192.168.1.100)"
        )

        smoothie_group.add_argument(
            "--verbose",
            action="store_true",
            help="Enable verbose output for network transfer debugging",
        )

        return parser

    def process_arguments(self):
        """Process arguments and update values, including SmoothieBoard-specific settings."""
        flag, args = super().process_arguments()

        if flag and args:
            # Update SmoothieBoard-specific values from parsed arguments
            if hasattr(args, "ip_addr") and args.ip_addr:
                self.ip_addr = args.ip_addr
                Path.Log.info(f"SmoothieBoard IP address set to: {self.ip_addr}")

            if hasattr(args, "verbose"):
                self.verbose = args.verbose
                if self.verbose:
                    Path.Log.info("Verbose mode enabled")

        return flag, args

    def export(self):
        """Override export to handle network upload to SmoothieBoard."""
        # First, do the standard export processing
        gcode_sections = super().export()

        if gcode_sections is None:
            return None

        # If IP address is specified, send to SmoothieBoard instead of writing to file
        if self.ip_addr:
            # Combine all G-code sections
            gcode = ""
            for section_name, section_gcode in gcode_sections:
                if section_gcode:
                    gcode += section_gcode

            # Get the output filename from the job
            filename = self._job.PostProcessorOutputFile
            if not filename or filename == "-":
                filename = "output.nc"

            self._send_to_smoothie(self.ip_addr, gcode, filename)

            # Return the gcode for display/editor
            return gcode_sections

        # Normal file-based export
        return gcode_sections

    def _send_to_smoothie(self, ip: str, gcode: str, fname: str) -> None:
        """
        Send G-code directly to SmoothieBoard via network.

        Args:
            ip: IP address of the SmoothieBoard
            gcode: G-code string to send
            fname: Filename to use on the SmoothieBoard SD card
        """
        fname = os.path.basename(fname)
        FreeCAD.Console.PrintMessage(f"Sending to SmoothieBoard: {fname}\n")

        gcode = gcode.rstrip()
        filesize = len(gcode)

        try:
            # Make connection to SmoothieBoard SFTP server (port 115)
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(4.0)
            s.connect((ip, 115))
            tn = s.makefile(mode="rw")

            # Read startup prompt
            ln = tn.readline()
            if not ln.startswith("+"):
                FreeCAD.Console.PrintError(f"Failed to connect with SFTP: {ln}\n")
                return

            if self.verbose:
                print("RSP: " + ln.strip())

            # Issue initial store command
            tn.write(f"STOR OLD /sd/{fname}\n")
            tn.flush()

            ln = tn.readline()
            if not ln.startswith("+"):
                FreeCAD.Console.PrintError(f"Failed to create file: {ln}\n")
                return

            if self.verbose:
                print("RSP: " + ln.strip())

            # Send size of file
            tn.write(f"SIZE {filesize}\n")
            tn.flush()

            ln = tn.readline()
            if not ln.startswith("+"):
                FreeCAD.Console.PrintError(f"Failed: {ln}\n")
                return

            if self.verbose:
                print("RSP: " + ln.strip())

            # Now send file
            cnt = 0
            for line in gcode.splitlines(True):
                tn.write(line)
                if self.verbose:
                    cnt += len(line)
                    print("SND: " + line.strip())
                    print(f"{cnt}/{filesize}\r", end="")

            tn.flush()

            ln = tn.readline()
            if not ln.startswith("+"):
                FreeCAD.Console.PrintError(f"Failed to save file: {ln}\n")
                return

            if self.verbose:
                print("RSP: " + ln.strip())

            # Exit
            tn.write("DONE\n")
            tn.flush()
            tn.close()

            FreeCAD.Console.PrintMessage("Upload complete\n")

        except socket.timeout:
            FreeCAD.Console.PrintError(f"Connection timeout while connecting to {ip}:115\n")
        except ConnectionRefusedError:
            FreeCAD.Console.PrintError(
                f"Connection refused by {ip}:115. Is the SmoothieBoard running?\n"
            )
        except Exception as e:
            FreeCAD.Console.PrintError(f"Error sending to SmoothieBoard: {str(e)}\n")

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a SmoothieBoard controller.

        This postprocessor supports direct network upload to SmoothieBoard
        via the --ip-addr argument.
        """
        return tooltip
