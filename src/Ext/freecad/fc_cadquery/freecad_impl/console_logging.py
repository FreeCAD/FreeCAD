import sys
import logging

import FreeCAD

# Logging Handler
#   Retain a reference to the logging handler so it may be removed on requeset.
#   Also to prevent 2 handlers being added
_logging_handler = None

# FreeCAD Logging Handler
class FreeCADConsoleHandler(logging.Handler):
    """logging.Handler class to output to FreeCAD's console"""

    def __init__(self, *args, **kwargs):
        super(FreeCADConsoleHandler, self).__init__(*args, **kwargs)

        # Test for expected print functions
        # (just check they exist, if they don't an exception will be raised)
        FreeCAD.Console.PrintMessage
        FreeCAD.Console.PrintWarning
        FreeCAD.Console.PrintError

    def emit(self, record):
        log_text = self.format(record) + "\n"
        if record.levelno >= logging.ERROR:
            FreeCAD.Console.PrintError(log_text)
        elif record.levelno >= logging.WARNING:
            FreeCAD.Console.PrintWarning(log_text)
        else:
            FreeCAD.Console.PrintMessage(log_text)


def enable(level=None, format="%(message)s"):
    """
    Enable python builtin logging, and output it somewhere you can see.
     - FreeCAD Console, or
     - STDOUT (if output to console fails, for whatever reason)

    Any script can log to FreeCAD console with:

        >>> import cadquery
        >>> cadquery.freecad_impl.console_logging.enable()
        >>> import logging
        >>> log = logging.getLogger(__name__)
        >>> log.debug("detailed info, not normally displayed")
        >>> log.info("some information")
        some information
        >>> log.warning("some warning text")  # orange text
        some warning text
        >>> log.error("an error message")  # red text
        an error message

    logging only needs to be enabled once, somewhere in your codebase.
    debug logging level can be set with:

        >>> import cadquery
        >>> import logging
        >>> cadquery.freecad_impl.console_logging.enable(logging.DEBUG)
        >>> log = logging.getLogger(__name__)
        >>> log.debug("debug logs will now be displayed")
        debug logs will now be displayed

    :param level: logging level to display, one of logging.(DEBUG|INFO|WARNING|ERROR)
    :param format: logging format to display (search for "python logging format" for details)
    :return: the logging Handler instance in effect
    """
    global _logging_handler

    # Set overall logging level (done even if handler has already been assigned)
    root_logger = logging.getLogger()
    if level is not None:
        root_logger.setLevel(level)
    elif _logging_handler is None:
        # level is not specified, and ho handler has been added yet.
        # assumption: user is enabling logging for the first time with no parameters.
        # let's make it simple for them and default the level to logging.INFO
        # (logging default level is logging.WARNING)
        root_logger.setLevel(logging.INFO)

    if _logging_handler is None:
        # Determine which Handler class to use
        try:
            _logging_handler = FreeCADConsoleHandler()
        except Exception as e:
            raise
            # Fall back to STDOUT output (better than nothing)
            _logging_handler = logging.StreamHandler(sys.stdout)

        # Configure and assign handler to root logger
        _logging_handler.setLevel(logging.DEBUG)
        root_logger.addHandler(_logging_handler)

    # Set formatting (can be used to re-define logging format)
    formatter = logging.Formatter(format)
    _logging_handler.setFormatter(formatter)

    return _logging_handler


def disable():
    """
    Disables logging to FreeCAD console (or STDOUT).
    Note, logging may be enabled by another imported module, so this isn't a
    guarantee; this function undoes logging_enable(), nothing more.
    """
    global _logging_handler
    if _logging_handler:
        root_logger = logging.getLogger()
        root_logger.handlers.remove(_logging_handler)
        _logging_handler = None
