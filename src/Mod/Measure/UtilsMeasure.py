from abc import ABC, abstractmethod, abstractclassmethod
from typing import List, Tuple

class MeasureBasePython(ABC):

    @abstractclassmethod
    def isValidSelection(cls, selection):
        """Returns True if the given selection is valid for this measurment"""
        pass

    @abstractclassmethod
    def isPrioritySelection(cls, selection):
        """Returns True if creation of this measurment should be priorized over other measurements for the given selection"""
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


