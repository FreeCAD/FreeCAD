# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, no_args
from Base.PyObjectBase import PyObjectBase


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
    def addMeasureType(id: str, label: str, measureType: object, /) -> None:
        """
        Add a new measure type.

        id : str
            Unique identifier of the measure type.
        label : str
            Name of the module.
        measureType : Measure.MeasureBasePython
            The actual measure type.
        """

    @staticmethod
    @no_args
    def getMeasureTypes() -> list[tuple[str, str, object]]:
        """
        Returns a list of all registered measure types.
        """
