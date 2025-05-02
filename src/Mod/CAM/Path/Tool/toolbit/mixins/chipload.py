import FreeCAD
from PySide.QtCore import QT_TRANSLATE_NOOP

class ChiploadMixin:
    """
    This is a interface class to indicate that the ToolBit can chip, i.e.
    it has a Chipload property.
    It is used to determine if the tool bit can be used for chip removal.
    """

    def __init__(self, obj, *args, **kwargs):
        obj.addProperty(
            "App::PropertyLength",
            "Chipload",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property", "Chipload per tooth"
            ),
        )
        obj.Chipload = FreeCAD.Units.Quantity("0.0 mm")

    def get_chipload(self) -> FreeCAD.Units.Quantity:
        return self.obj.Chipload
    
    def set_chipload(self, value: FreeCAD.Units.Quantity):
        if not isinstance(value, FreeCAD.Units.Quantity):
            raise ValueError("Chipload must be a FreeCAD Units.Quantity")
        self.obj.Chipload = value
