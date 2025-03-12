# /***************************************************************************
#  *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
#  *                                                                         *
#  *   This file is part of FreeCAD.                                         *
#  *                                                                         *
#  *   FreeCAD is free software: you can redistribute it and/or modify it    *
#  *   under the terms of the GNU Lesser General Public License as           *
#  *   published by the Free Software Foundation, either version 2.1 of the  *
#  *   License, or (at your option) any later version.                       *
#  *                                                                         *
#  *   FreeCAD is distributed in the hope that it will be useful, but        *
#  *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#  *   Lesser General Public License for more details.                       *
#  *                                                                         *
#  *   You should have received a copy of the GNU Lesser General Public      *
#  *   License along with FreeCAD. If not, see                               *
#  *   <https://www.gnu.org/licenses/>.                                      *
#  *                                                                         *
#  **************************************************************************/

from abc import ABC, abstractmethod, abstractclassmethod
from typing import List, Tuple


class MeasureBasePython(ABC):

    @abstractclassmethod
    def isValidSelection(cls, selection):
        """Returns True if the given selection is valid for this measurement"""
        pass

    @abstractclassmethod
    def isPrioritySelection(cls, selection):
        """Returns True if creation of this measurement should be prioritized over other measurements for the given selection"""
        pass

    @abstractclassmethod
    def getInputProps(cls) -> Tuple[str]:
        """Returns all properties that the measurement's result depends on"""
        return ()

    @abstractmethod
    def getSubject(self, obj) -> Tuple:
        """Returns all objects that are measured, this is used to autohide the measurement if the relevant elements are not visible"""
        return []

    @abstractmethod
    def parseSelection(self, obj, selection):
        """Sets the measurements properties from the given selection"""
        pass
