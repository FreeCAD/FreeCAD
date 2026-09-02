# SPD-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Alan Grover <awgrover@gmail.com>

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Error classes for CAM.
If it is about something CAM (gcode, Postable, PP,...) you should throw one of these.
"""


class CAMError(Exception):
    """
    Base class for distinquishing
    Suppply context information
    """

    def __init__(
        self,
        message,
        job=None,
        operation=None,
        item=None,
        command=None,
        line=None,
        pp=None,
        extra=None,
    ):
        self.job = job
        self.operation = operation
        self.item = item  # usually only have .item if not .operation
        self.command = command
        self.line = line  # line number usually
        self.pp = pp  # post-processor
        self.extra = extra  # final value appended (str'ified)
        super().__init__(message)

    def __str__(self):
        context = []
        if self.job:
            context.append(f"Job <{self.job.Label}>")
        if self.operation:
            context.append(f"Op <{self.operation.Label}>")
        if self.item:
            context.append(f"Postable {item.item_type}:{item.label}")
        if self.command:
            context.append(f"<{self.command.toGCode()}>")
        if self.line:
            context.append(f"line {self.line}")
        if self.pp:
            context.append(f"{self.pp}")

        context_string = ", in " + ", in ".join(context) if context else ""
        if self.extra:
            context_string += str(self.extra)
        return f"{super().__str__()}{context_string}"


class CAMValueError(CAMError):
    """
    Is the value of something a problem?
    """


class CAMAttributeError(CAMError):
    """
    Is the attribute inappropriate, missing, etc?
    """


class CAMNotImplementedError(CAMError):
    """
    Something is not implemented
    """
