# SPDX-License-Identifier: LGPL-2.1-or-later

from PyObjectBase import PyObjectBase

class Precision(PyObjectBase):
    """
    This is the Precision class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @staticmethod
    def angular() -> float:
        """
        Returns the recommended precision value when checking the equality of two angles (given in radians)
        """
        ...

    @staticmethod
    def confusion() -> float:
        """
        Returns the recommended precision value when checking coincidence of two points in real space
        """
        ...

    @staticmethod
    def squareConfusion() -> float:
        """
        Returns square of confusion
        """
        ...

    @staticmethod
    def intersection() -> float:
        """
        Returns the precision value in real space, frequently used by intersection algorithms
        """
        ...

    @staticmethod
    def approximation() -> float:
        """
        Returns the precision value in real space, frequently used by approximation algorithms
        """
        ...

    @staticmethod
    def parametric() -> float:
        """
        Convert a real space precision to a parametric space precision
        """
        ...

    @staticmethod
    def isInfinite() -> bool:
        """
        Returns True if R may be considered as an infinite number
        """
        ...

    @staticmethod
    def isPositiveInfinite() -> bool:
        """
        Returns True if R may  be considered as a positive infinite number
        """
        ...

    @staticmethod
    def isNegativeInfinite() -> bool:
        """
        Returns True if R may  be considered as a negative infinite number
        """
        ...

    @staticmethod
    def infinite() -> float:
        """
        Returns a  big number that  can  be  considered as infinite
        """
        ...
