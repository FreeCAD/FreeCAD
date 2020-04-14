"""Provide translate functions for the Draft Workbench.

This module contains auxiliary functions to translate strings
using the QtCore module.
"""
## @package translate
# \ingroup DRAFT
# \brief Provide translate functions for the Draft Workbench.

# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from PySide import QtCore
from PySide import QtGui
import six

Qtranslate = QtGui.QApplication.translate

# This property only exists in Qt4, which is normally paired
# with Python 2.
# But if Python 2 is used with Qt5 (rare),
# this assignment will fail.
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
except AttributeError:
    _encoding = None


def translate(context, text, utf8_decode=False):
    r"""Translate the text using the Qt translate function.

    It wraps around `QtGui.QApplication.translate`,
    which is the same as `QtCore.QCoreApplication.translate`.

    Parameters
    ----------
    context: str
        In C++ it is typically a class name.
        But it can also be any other string to categorize the translation,
        for example, the name of a workbench, tool, or function
        that is being translated. Usually it will be the name
        of the workbench.

    text: str
        Text that will be translated. It could be a single word,
        a full sentence, paragraph, or multiple paragraphs with new lines.
        Usually the last endline character '\\n'
        that finishes the string doesn't need to be included
        for translation.

    utf8_decode: bool
        It defaults to `False`.
        This must be set to `True` to indicate that the `text`
        is an `'utf8'` encoded string, so it should be returned as such.
        This option is ignored when using Python 3
        as with Python 3 all strings are `'utf8'` by default.

    Returns
    -------
    str
        A unicode string returned by `QtGui.QApplication.translate`.

        If `utf8_decode` is `True`, the resulting string will be encoded
        in `'utf8'`, and a `bytes` object will be returned.
        ::
            Qtranslate = QtGui.QApplication.translate
            return Qtranslate(context, text, None).encode("utf8")

    Unicode strings
    ---------------
    Whether it is Qt4 or Qt5, the `translate` function
    always returns a unicode string.
    The difference is how it handles the input.

    Reference: https://pyside.github.io/docs/pyside/PySide/QtCore/

    In Qt4 the translate function has a 4th parameter to define the encoding
    of the input string.

    >>> QtCore.QCoreApplication.translate(context, text, None, UnicodeUT8)
    >>> QtGui.QApplication.translate(context, text, None, UnicodeUT8)

    Reference: https://doc.qt.io/qtforpython/PySide2/QtCore

    In Qt5 the strings are always assumed unicode, so the 4th parameter
    is for a different use, and it is not used.

    >>> QtCore.QCoreApplication.translate(context, text, None)
    >>> QtGui.QApplication.translate(context, text, None)
    """
    # Python 3 and Qt5
    # The text is a utf8 string, and since it is Qt5
    # the translate function doesn't use the 4th parameter
    if six.PY3:
        return Qtranslate(context, text, None)
    # Python 2
    elif QtCore.qVersion() > "4":
        # Python 2 and Qt5
        if utf8_decode:
            # The text is a utf8 string, and since it is Qt5
            # the translate function doesn't use the 4th parameter
            return Qtranslate(context, text, None)
        else:
            # The text is not a unicode string, and since it is Qt5
            # the translate function doesn't use the 4th parameter.
            # Therefore the output string needs to be encoded manually
            # as utf8 bytes before returning.
            return Qtranslate(context, text, None).encode("utf8")
    else:
        # Python 2 and Qt4
        if utf8_decode:
            # The text is a utf8 string, and since it is Qt4
            # the translate function uses the 4th parameter
            # to handle the input encoding.
            return Qtranslate(context, text, None, _encoding)
        else:
            # The text is not a unicode string, and since it is Qt4
            # the translate function uses the 4th parameter
            # to handle the encoding.
            # In this case, the `encoding` is `None`, therefore
            # the output string needs to be encoded manually
            # as utf8 bytes before returning.
            return Qtranslate(context, text, None, _encoding).encode("utf8")


# Original code no longer used. It is listed here for reference
# to show how the different pairings Py2/Qt4, Py3/Qt5, Py2/Qt5, Py3/Qt4
# were handled in the past.
# If there is a problem with the code above, this code can be made active
# again, and the code above can be commented out.
#
# =============================================================================
# try:
#     _encoding = QtGui.QApplication.UnicodeUTF8 if six.PY2 else None
#     def translate(context, text, utf8_decode=True):
#         """convenience function for Qt translator
#             context: str
#                 context is typically a class name (e.g., "MyDialog")
#             text: str
#                 text which gets translated
#             utf8_decode: bool [False]
#                 if set to true utf8 encoded unicode will be returned.
#                 This option does not have influence
#                 on python3 as for python3 we are returning utf-8 encoded
#                 unicode by default!
#         """
#         if six.PY3:
#             return Qtranslate(context, text, None)
#         elif utf8_decode:
#             return Qtranslate(context, text, None, _encoding)
#         else:
#             return Qtranslate(context, text, None, _encoding).encode("utf8")
#
# except AttributeError:
#     def translate(context, text, utf8_decode=False):
#         """convenience function for Qt translator
#             context: str
#                 context is typically a class name (e.g., "MyDialog")
#             text: str
#                 text which gets translated
#             utf8_decode: bool [False]
#                 if set to true utf8 encoded unicode will be returned.
#                 This option does not have influence
#                 on python3 as for python3 we are returning utf-8 encoded
#                 unicode by default!
#         """
#         if six.PY3:
#             return Qtranslate(context, text, None)
#         elif QtCore.qVersion() > "4":
#             if utf8_decode:
#                 return Qtranslate(context, text, None)
#             else:
#                 return Qtranslate(context, text, None).encode("utf8")
#         else:
#             if utf8_decode:
#                 return Qtranslate(context, text, None, _encoding)
#             else:
#                 return Qtranslate(context, text, None,
#                                   _encoding).encode("utf8")
# =============================================================================

# The same Qt translate function is provided here
QT_TRANSLATE_NOOP = QtCore.QT_TRANSLATE_NOOP


def _tr(text):
    """Translate with the context set to Draft. Our own function.

    It uses our own `translate` defined function which internally still
    uses `QtCore.QCoreApplication.translate`.

    This is normally used inside a function that prints text.

    >>> print(tr("Some text that will be translated"))

    Parameters
    ----------
    text : str
        Any text string.

    Returns
    -------
    str
        Returns the translated string at runtime.
    """
    return translate("Draft", text)


def _qtr(text):
    """Translate with the context set to Draft. QtCore function.

    It uses `QtCore.QT_TRANSLATE_NOOP` function to perform translation.

    This is normally used inside a function that prints text.

    >>> print(qtr("Some text that will be translated"))

    Parameters
    ----------
    text : str
        Any text string.

    Returns
    -------
    str
        Returns the translated string at runtime.
    """
    return QT_TRANSLATE_NOOP("Draft", text)
