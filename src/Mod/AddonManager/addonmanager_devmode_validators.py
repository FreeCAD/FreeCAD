# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Validators used for various line edits """

import keyword

from PySide.QtGui import (
    QValidator,
)

# QRegularExpressionValidator was only added at the very end of the PySide
# development cycle, so make sure to support the older QRegExp version as well.
try:
    from PySide.QtGui import (
        QRegularExpressionValidator,
    )
    from PySide.QtCore import QRegularExpression

    RegexWrapper = QRegularExpression
    RegexValidatorWrapper = QRegularExpressionValidator
except ImportError:
    QRegularExpressionValidator = None
    QRegularExpression = None
    from PySide.QtGui import (
        QRegExpValidator,
    )
    from PySide.QtCore import QRegExp

    RegexWrapper = QRegExp
    RegexValidatorWrapper = QRegExpValidator


# pylint: disable=too-few-public-methods


class NameValidator(QValidator):
    """Simple validator to exclude characters that are not valid in filenames."""

    invalid = '/\\?%*:|"<>'

    def validate(self, value: str, _: int):
        """Check the value against the validator"""
        for char in value:
            if char in NameValidator.invalid:
                return QValidator.Invalid
        return QValidator.Acceptable

    def fixup(self, value: str) -> str:
        """Remove invalid characters from value"""
        result = ""
        for char in value:
            if char not in NameValidator.invalid:
                result += char
        return result


class PythonIdentifierValidator(QValidator):
    """Validates whether input is a valid Python identifier."""

    def validate(self, value: str, _: int):
        """The function that does the validation."""
        if not value:
            return QValidator.Intermediate

        if not value.isidentifier():
            return QValidator.Invalid  # Includes an illegal character of some sort

        if keyword.iskeyword(value):
            return QValidator.Intermediate  # They can keep typing and it might become valid

        return QValidator.Acceptable


class SemVerValidator(RegexValidatorWrapper):
    """Implements the officially-recommended regex validator for Semantic version numbers."""

    # https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
    semver_re = RegexWrapper(
        r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
        + r"(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)"
        + r"(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
        + r"(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
    )

    def __init__(self):
        super().__init__()
        if hasattr(self, "setRegularExpression"):
            self.setRegularExpression(SemVerValidator.semver_re)
        else:
            self.setRegExp(SemVerValidator.semver_re)

    @classmethod
    def check(cls, value: str) -> bool:
        """Returns true if value validates, and false if not"""
        return cls.semver_re.match(value).hasMatch()


class CalVerValidator(RegexValidatorWrapper):
    """Implements a basic regular expression validator that makes sure an entry corresponds
    to a CalVer version numbering standard."""

    calver_re = RegexWrapper(
        r"^(?P<major>[1-9]\d{3})\.(?P<minor>[0-9]{1,2})\.(?P<patch>0|[0-9]{0,2})"
        + r"(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)"
        + r"(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
        + r"(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
    )

    def __init__(self):
        super().__init__()
        if hasattr(self, "setRegularExpression"):
            self.setRegularExpression(CalVerValidator.calver_re)
        else:
            self.setRegExp(CalVerValidator.calver_re)

    @classmethod
    def check(cls, value: str) -> bool:
        """Returns true if value validates, and false if not"""
        return cls.calver_re.match(value).hasMatch()


class VersionValidator(QValidator):
    """Implements the officially-recommended regex validator for Semantic version numbers, and a
    decent approximation of the same thing for CalVer-style version numbers."""

    def __init__(self):
        super().__init__()
        self.semver = SemVerValidator()
        self.calver = CalVerValidator()

    def validate(self, value: str, position: int):
        """Called for validation, returns a tuple of the validation state, the value, and the
        position."""
        semver_result = self.semver.validate(value, position)
        calver_result = self.calver.validate(value, position)

        if semver_result[0] == QValidator.Acceptable:
            return semver_result
        if calver_result[0] == QValidator.Acceptable:
            return calver_result
        if semver_result[0] == QValidator.Intermediate:
            return semver_result
        if calver_result[0] == QValidator.Intermediate:
            return calver_result
        return QValidator.Invalid, value, position
