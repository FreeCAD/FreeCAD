# SPDX-License-Identifier: LGPL-2.1-or-later

"""Concise user feedback for expected Forms modeling failures."""

import FreeCAD as App
import Part

from .brep import ConversionError


# These failures describe rejected user input or geometry. Programming errors
# deliberately remain outside this tuple so their tracebacks are not hidden.
MODELING_ERRORS = (ConversionError, Part.OCCError, ValueError)


def report_modeling_error(context, error, status_widget=None):
    """Report an expected modeling rejection without raising through Qt."""
    detail = str(error) or error.__class__.__name__
    message = f"{context}: {detail}" if context else detail
    App.Console.PrintWarning(message + "\n")
    if status_widget is not None:
        status_widget.setText(detail)
    if App.GuiUp:
        import FreeCADGui as Gui

        Gui.getMainWindow().statusBar().showMessage(message, 5000)
    return False
