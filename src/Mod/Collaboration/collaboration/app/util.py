# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import os
import logging

import FreeCAD


class __CollaborationEnv:
    def __init__(self):
        # self.debug_level = os.environ.get("COLLABORATION_DEBUG_LEVEL", "info")
        self.debug_level = os.environ.get("COLLABORATION_DEBUG_LEVEL", "debug")

    def get_debug_level(self):
        if self.debug_level == "info":
            return logging.INFO
        elif self.debug_level == "debug":
            return logging.DEBUG
        elif self.debug_level == "error":
            return logging.ERROR
        elif self.debug_level == "warning":
            return logging.WARNING
        else:
            return logging.INFO


env = __CollaborationEnv()


class FreeCADHandler(logging.Handler):
    def __init__(self):
        logging.Handler.__init__(self)

    def emit(self, record):
        msg = self.format(record) + "\n"
        c = FreeCAD.Console
        if record.levelno >= logging.ERROR:
            c.PrintError(msg)
        elif record.levelno >= logging.WARNING:
            c.PrintWarning(msg)
        else:
            c.PrintMessage(msg)


def get_logger(name):
    logger = logging.getLogger(name)
    logger.setLevel(env.get_debug_level())
    handler = FreeCADHandler()
    if env.get_debug_level() >= logging.INFO:
        formatter = logging.Formatter("%(levelname)s: %(message)s")
    else:
        formatter = logging.Formatter("%(levelname)s: %(name)s:%(lineno)d %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger
