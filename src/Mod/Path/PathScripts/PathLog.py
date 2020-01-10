# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import os
import traceback

class Level:
    """Enumeration of log levels, used for setLevel and getLevel."""
    # pylint: disable=no-init
    RESET   = -1
    ERROR   = 0
    WARNING = 1
    NOTICE  = 2
    INFO    = 3
    DEBUG   = 4

    _names = { ERROR: 'ERROR', WARNING: 'WARNING', NOTICE: 'NOTICE', INFO: 'INFO', DEBUG: 'DEBUG' }

    @classmethod
    def toString(cls, level):
        return cls._names.get(level, 'UNKNOWN')

_defaultLogLevel = Level.NOTICE
_moduleLogLevel  = { }
_useConsole = True
_trackModule = { }
_trackAll = False

def logToConsole(yes):
    """(boolean) - if set to True (default behaviour) log messages are printed to the console. Otherwise they are printed to stdout."""
    global _useConsole # pylint: disable=global-statement
    _useConsole = yes

def setLevel(level, module = None):
    """(level, module = None)
       if no module is specified the default log level is set.
       Otherwise the module specific log level is changed (use RESET to clear)."""
    global _defaultLogLevel # pylint: disable=global-statement
    global _moduleLogLevel # pylint: disable=global-statement
    if module:
        if level == Level.RESET:
            if _moduleLogLevel.get(module, -1) != -1:
                del _moduleLogLevel[module]
        else:
            _moduleLogLevel[module] = level
    else:
        if level == Level.RESET:
            _defaultLogLevel = Level.NOTICE
            _moduleLogLevel = { }
        else:
            _defaultLogLevel = level

def getLevel(module = None):
    """(module = None) - return the global (None) or module specific log level."""
    if module:
        return _moduleLogLevel.get(module, _defaultLogLevel)
    return _defaultLogLevel

def thisModule():
    """returns the module id of the caller, can be used for setLevel, getLevel and trackModule."""
    return _caller()[0]

def _caller():
    """internal function to determine the calling module."""
    filename, line, func, text = traceback.extract_stack(limit=3)[0] # pylint: disable=unused-variable
    return os.path.splitext(os.path.basename(filename))[0], line, func

def _log(level, module_line_func, msg):
    """internal function to do the logging"""
    module, line, func = module_line_func # pylint: disable=unused-variable
    if getLevel(module) >= level:
        message = "%s.%s: %s" % (module, Level.toString(level), msg)
        if _useConsole:
            message += "\n"
            if level == Level.NOTICE:
                FreeCAD.Console.PrintLog(message)
            elif level == Level.WARNING:
                FreeCAD.Console.PrintWarning(message)
            elif level == Level.ERROR:
                FreeCAD.Console.PrintError(message)
            else:
                FreeCAD.Console.PrintMessage(message)
        else:
            print(message)
        return message
    return None

def debug(msg):
    """(message)"""
    return _log(Level.DEBUG, _caller(), msg)
def info(msg):
    """(message)"""
    return _log(Level.INFO, _caller(), msg)
def notice(msg):
    """(message)"""
    return _log(Level.NOTICE, _caller(), msg)
def warning(msg):
    """(message)"""
    return _log(Level.WARNING, _caller(), msg)
def error(msg):
    """(message)"""
    return _log(Level.ERROR, _caller(), msg)

def trackAllModules(boolean):
    """(boolean) - if True all modules will be tracked, otherwise tracking is up to the module setting."""
    global _trackAll # pylint: disable=global-statement
    _trackAll = boolean

def untrackAllModules():
    """In addition to stop tracking all modules it also clears the tracking flag for all individual modules."""
    global _trackAll # pylint: disable=global-statement
    global _trackModule # pylint: disable=global-statement
    _trackAll = False
    _trackModule = { }

def trackModule(module = None):
    """(module = None) - start tracking given module, current module if not set."""
    global _trackModule # pylint: disable=global-statement
    if module:
        _trackModule[module] = True
    else:
        mod, line, func = _caller() # pylint: disable=unused-variable
        _trackModule[mod] = True

def untrackModule(module = None):
    """(module = None) - stop tracking given module, current module if not set."""
    global _trackModule # pylint: disable=global-statement
    if module and _trackModule.get(module, None):
        del _trackModule[module]
    elif not module:
        mod, line, func = _caller() # pylint: disable=unused-variable
        if _trackModule.get(mod, None):
            del _trackModule[mod]

def track(*args):
    """(....) - call with arguments of current function you want logged if tracking is enabled."""
    module, line, func = _caller()
    if _trackAll or _trackModule.get(module, None):
        message = "%s(%d).%s(%s)" % (module, line, func, ', '.join([str(arg) for arg in args]))
        if _useConsole:
            FreeCAD.Console.PrintMessage(message + "\n")
        else:
            print(message)
        return message
    return None

