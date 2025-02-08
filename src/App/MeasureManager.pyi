from Base.Metadata import export, no_args
from Base.PyObjectBase import PyObjectBase
from typing import List, Tuple, TypeAlias


MeasureType: TypeAlias = object

@export(
    Constructor=False,
    Delete=True,
    )
class MeasureManager(PyObjectBase):
    """
    MeasureManager class.

    The MeasureManager handles measure types and geometry handler across FreeCAD.

    Author: David Friedli (david@friedli-be.ch)
    Licence: LGPL
    DeveloperDocu: MeasureManager
    """

    @staticmethod
    def addMeasureType(id: str, label: str, measureType: MeasureType) -> None:
        """
        addMeasureType(id, label, measureType) -> None

        Add a new measure type.

        id : str
            Unique identifier of the measure type.
        label : str
            Name of the module.
        measureType : Measure.MeasureBasePython
            The actual measure type.
        """
        ...

    @staticmethod
    @no_args
    def getMeasureTypes() -> List[Tuple[str, str, MeasureType]]:
        """
        getMeasureTypes() -> List[(id, label, pythonMeasureType)]

        Returns a list of all registered measure types.
        """
        ...
