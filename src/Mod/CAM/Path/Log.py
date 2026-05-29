# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
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

    RESET = -1
    ERROR = 0
    WARNING = 1
    NOTICE = 2
    INFO = 3
    DEBUG = 4

    _names = {
        ERROR: "ERROR",
        WARNING: "WARNING",
        NOTICE: "NOTICE",
        INFO: "INFO",
        DEBUG: "DEBUG",
    }

    @classmethod
    def toString(cls, level):
        return cls._names.get(level, "UNKNOWN")


def logToConsole(yes):
    """(boolean) - if set to True (default behaviour) log messages are printed to the console. Otherwise they are printed to stdout."""
    global _useConsole
    _useConsole = yes


def thisModule():
    """returns the module id of the caller please use logger.getModule instead if possible"""
    return _caller()[0]


def _caller():
    """internal function to determine the calling module."""
    filename, line, func, text = traceback.extract_stack(limit=3)[0]
    return os.path.splitext(os.path.basename(filename))[0], line, func


_defaultLogLevel = Level.NOTICE
_useConsole = True
_trackAll = False
_moduleLoggers = {}


class ModuleLogger:
    _tracked = False
    _logLevel = None

    def __init__(self, module, initialLogLevel=None):
        self._logLevel = initialLogLevel
        self._module = module

    def getModule(self):
        return self._module

    def _log(self, level, msg, *args):
        """internal function to do the logging"""

        if not self.willLogAt(level):
            return None

        message = ("{}.{}: " + str(msg)).format(self.getModule(), Level.toString(level), *args)

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

    def isTrackingEnabled(self):
        return self._tracked or _trackAll

    def setTrackingEnabled(self, enabled):
        """(enabled)"""
        self._tracked = enabled

    def enableTracking(self):
        self.setTrackingEnabled(True)

    def disableTracking(self):
        self.setTrackingEnabled(False)

    def getLevel(self):
        if self._logLevel is None:
            return _defaultLogLevel
        else:
            return self._logLevel

    def setLevel(self, level):
        if level == Level.RESET:
            self._logLevel = None
        else:
            self._logLevel = level

    def willLogAt(self, level):
        """(level)"""
        return self.getLevel() >= level

    def debug(self, message, *args):
        """(message, *args)"""
        if not self.willLogAt(Level.DEBUG):
            return None

        module, line, func = _caller()
        return self._log(Level.DEBUG, "({}) - " + str(message), line, *args)

    def info(self, message, *args):
        """(message, *args)"""
        return self._log(Level.INFO, message, *args)

    def notice(self, message, *args):
        """(message, *args)"""
        return self._log(Level.NOTICE, message, *args)

    def warning(self, message, *args):
        """(message, *args)"""
        return self._log(Level.WARNING, message, *args)

    def error(self, message, *args):
        """(message, *args)"""
        return self._log(Level.ERROR, message, *args)

    def track(self, *args, fmt=None):
        """(....) - call with arguments of current function you want logged if tracking is enabled."""

        if self.isTrackingEnabled():
            module, line, func = _caller()

            if fmt is None:
                formattedArgs = ", ".join([str(arg) for arg in args])
            else:
                formattedArgs = fmt.format(*args)

            message = f"{module}({line}).{func}({formattedArgs})"
            if _useConsole:
                FreeCAD.Console.PrintMessage(message + "\n")
            else:
                print(message)
            return message
        return None


def getModuleLoggerWithLevelOrDebug(level, debug, module=None):
    """(level, debug, module=None)"""

    if module is None:
        module = _caller()[0]

    withLevel = Level.DEBUG if debug else level
    return getModuleLogger(module, withLevel=withLevel, enableTracking=debug)


def getModuleLogger(module=None, withLevel=None, enableTracking=None):
    """(module=None, withLevel=None, enableTracking=None)"""

    if module is None:
        module = _caller()[0]

    logger = _moduleLoggers.get(module, None)
    if logger is None:
        logger = ModuleLogger(module, initialLogLevel=withLevel)
        _moduleLoggers[module] = logger
    elif withLevel is not None:
        logger.setLevel(withLevel)

    if enableTracking is not None:
        logger.setTrackingEnabled(enableTracking)

    return logger


def trackAllModules(boolean):
    """(boolean) - if True all modules will be tracked, otherwise tracking is up to the module setting."""
    global _trackAll
    _trackAll = boolean


def untrackAllModules():
    """In addition to stop tracking all modules it also clears the tracking flag for all individual modules."""
    global _trackAll
    global _moduleLoggers

    _trackAll = False

    for module in _moduleLoggers.values():
        module.disableTracking()


# deprecated methods:


def setLevel(level, module=None):
    """(level, module = None)

    deprecated - use logger.setLevel instead

    if no module is specified the default log level is set.
    Otherwise the module specific log level is changed (use RESET to clear)."""
    global _defaultLogLevel
    global _moduleLoggers
    if module is None:
        if level == Level.RESET:
            _defaultLogLevel = Level.NOTICE

            for module in _moduleLoggers.values():
                module.setLevel(Level.RESET)
        else:
            _defaultLogLevel = level
    else:
        getModuleLogger(module).setLevel(level)


def getLevel(module=None):
    """(module = None) - return the global (None) or module specific log level.

    deprecated - use logger.getLevel instead
    """
    if module is None:
        return _defaultLogLevel
    else:
        return getModuleLogger(module).getLevel()


def trackModule(module=None):
    """(module = None) - start tracking given module, current module if not set.

    deprecated - use logger.enableTracking instead
    """

    if module is None:
        module, _, _ = _caller()

    getModuleLogger(module).enableTracking()


def untrackModule(module=None):
    """(module = None) - stop tracking given module, current module if not set.

    deprecated - use logger.disableTracking instead
    """
    global _moduleLoggers

    if module is None:
        module, _, _ = _caller()

    logger = _moduleLoggers.get(module, None)

    if logger is not None:
        logger.disableTracking()


def debug(msg):
    """(msg)

    deprecated - use logger.debug(format, *args) instead
    """
    module, line, func = _caller()
    return getModuleLogger(module)._log(Level.DEBUG, "({}) - {}", line, msg)


def info(msg):
    """(msg)

    deprecated - use logger.info(format, *args) instead
    """
    return getModuleLogger(_caller()[0]).info("{}", msg)


def notice(msg):
    """(msg)

    deprecated - use logger.notice(format, *args) instead
    """
    return getModuleLogger(_caller()[0]).notice("{}", msg)


def warning(msg):
    """(msg)

    deprecated - use logger.warning(format, *args) instead
    """
    return getModuleLogger(_caller()[0]).warning("{}", msg)


def error(msg):
    """(msg)

    deprecated - use logger.error(format, *args) instead
    """
    return getModuleLogger(_caller()[0]).error("{}", msg)


def track(*args):
    """(*args)

    deprecated - use logger.track(format, *args) instead
    """
    module, line, func = _caller()

    if getModuleLogger(module).isTrackingEnabled():

        formattedArgs = ", ".join([str(arg) for arg in args])

        message = f"{module}({line}).{func}({formattedArgs})"
        if _useConsole:
            FreeCAD.Console.PrintMessage(message + "\n")
        else:
            print(message)
        return message
    return None
