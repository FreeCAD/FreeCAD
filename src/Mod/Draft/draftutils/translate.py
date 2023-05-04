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
"""Provides utility functions that wrap around the Qt translate functions.

This module contains auxiliary functions to translate strings
using the QtCore module.
"""
## @package translate
# \ingroup draftutils
# \brief Provides utility functions that wrap around the Qt translate function.

## \addtogroup draftutils
# @{
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

Qtranslate = QtCore.QCoreApplication.translate

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
except AttributeError:
    _encoding = None


def translate(context, text, comment=None):
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
        Usually the last endline character '\n'
        that finishes the string doesn't need to be included
        for translation.

    Returns
    -------
    str
        A unicode string returned by `QtGui.QApplication.translate`.

    Unicode strings
    ---------------
    Reference: https://doc.qt.io/qtforpython/PySide2/QtCore

    In Qt5 the strings are always assumed unicode

    >>> QtCore.QCoreApplication.translate(context, text, None)
    >>> QtGui.QApplication.translate(context, text, None)
    """
    return Qtranslate(context, text, comment)


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


## @}
