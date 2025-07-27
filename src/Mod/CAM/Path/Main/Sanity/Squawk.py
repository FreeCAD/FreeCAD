import os
from datetime import datetime
from enum import Enum, auto

import FreeCAD


class SquawkType(Enum):
    """Enumeration for squawk types."""

    NOTE = "NOTE"
    WARNING = "WARNING"
    CAUTION = "CAUTION"
    TIP = "TIP"

    def __str__(self):
        """Return the string representation of the squawk type."""
        return self.value


class Squawk:
    """
    Represents a notification, warning, or other message in the CAM sanity check.
    """

    def __init__(self, operator, note, date=None, squawk_type=SquawkType.NOTE):
        """
        Create a new Squawk object.

        Args:
            operator (str): The entity generating the squawk
            note (str): The message content
            date (datetime, optional): Timestamp of the squawk, defaults to current time
            squawk_type (str): Type of squawk (NOTE, WARNING, CAUTION, TIP)
            icon (str, optional): A string representing the squawk icon.
        """
        self.operator = operator
        self.note = note
        self.date = date or datetime.now()
        self._validate_and_set_type(squawk_type)
        self._set_icon()

    def _validate_and_set_type(self, squawk_type):
        """Validate and set the squawk type."""
        if isinstance(squawk_type, SquawkType):
            self.squawk_type = squawk_type
        else:
            self.squawk_type = SquawkType.NOTE

    def _set_icon(self):
        """Set the appropriate icon string based on squawk type."""
        icon_map = {
            SquawkType.TIP: "Sanity_Bulb",
            SquawkType.NOTE: "Sanity_Note",
            SquawkType.WARNING: "Sanity_Warning",
            SquawkType.CAUTION: "Sanity_Caution",
        }

        icon_name = icon_map.get(self.squawk_type, "Sanity_Note")
        self.icon = f"{FreeCAD.getHomePath()}Mod/CAM/Path/Main/Sanity/{icon_name}.svg"

    def to_dict(self):
        """
        Convert the Squawk to a dictionary format compatible with the existing code.

        Returns:
            dict: Dictionary representation of the squawk
        """
        return {
            "date": str(self.date),
            "operator": self.operator,
            "note": self.note,
            "squawkType": self.squawk_type,
            "icon": self.icon,
        }


# For backward compatibility
def create_squawk(operator, note, date=None, squawk_type=SquawkType.NOTE):
    """Legacy function for creating a squawk dictionary directly."""
    squawk = Squawk(operator, note, date, squawk_type)
    return squawk.to_dict()
