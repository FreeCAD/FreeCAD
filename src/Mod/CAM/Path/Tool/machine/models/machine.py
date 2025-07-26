# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import uuid
from abc import ABC
from enum import Enum
from typing import Dict, Optional, List
import FreeCAD
import Path
from ...assets import Asset
from .component import MachineComponent
from .axis import Axis
from .spindle import Spindle


translate = FreeCAD.Qt.translate


class MachineFeature(Enum):
    TURNING_2D = "TURNING_2D"
    MILLING_3D = "MILLING_3D"
    RIGID_TAPPING = "RIGID_TAPPING"


class Machine(MachineComponent, Asset, ABC):
    """Represents a machine (e.g. a 3 axis CNC or a lathe)."""

    asset_type = "machine"
    _property_object_name = "Attributes"
    _property_object_group = "Machine"

    def __init__(
        self,
        name: str,
        label: Optional[str] = None,
        post_processor: str = "generic",
        post_processor_args: str = "",
        icon: Optional[str] = None,
        feature_flags: Optional[List[MachineFeature]] = None,
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Machine object.

        Args:
            name: Machine name.
            label: Machine label.
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            icon: Icon name.
            feature_flags: List of machine feature flags.
            id: Unique identifier (optional).
        """
        super().__init__(name=name, label=label, icon=icon)
        Asset.__init__(self)
        self._id = id or str(uuid.uuid1())
        self._feature_flags = feature_flags or []

        # Set property values
        self.post_processor = post_processor  # property setter below checks validity
        self._post_processor_args = post_processor_args

    def get_id(self):
        return self._id

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        raise NotImplementedError

    @property
    def feature_flags(self) -> List[MachineFeature]:
        """Returns the machine's feature flags."""
        return self._feature_flags

    @feature_flags.setter
    def feature_flags(self, value: List[MachineFeature]):
        self._feature_flags = value

    @property
    def summary(self) -> str:
        """
        Returns a summary of the machine parameters for display in the UI.

        Returns:
            Summary string.
        """
        n_axes = len(self.find_children_by_type(Axis))
        n_spindles = len(self.find_children_by_type(Spindle))
        return FreeCAD.Qt.translate(
            "CAM",
            f"{n_axes-n_spindles}-axis {self.get_type()} with " f"{n_spindles} spindles",
        )

    @property
    def spindles(self) -> List[Spindle]:
        return self.find_children_by_type(Spindle)

    @property
    def post_processor(self) -> str:
        """Name of the FreeCAD post processor."""
        return self._post_processor

    @post_processor.setter
    def post_processor(self, value: str):
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        if value not in available_post_processors:
            raise AttributeError(f"Post processor '{value}' does not exist.")
        self._post_processor = value

    @property
    def post_processor_args(self) -> str:
        """Arguments for the post processor."""
        return self._post_processor_args

    @post_processor_args.setter
    def post_processor_args(self, value: str):
        self._post_processor_args = value

    def validate(self) -> None:
        """Validates machine parameters."""
        super().validate()
        if not self.label:
            raise AttributeError(translate("CAM", "Machine name is required"))

    def _dump_self(self, indent_str="") -> str:
        output = f"{indent_str}{self.get_type()} {self.label}:\n"
        output += f"{indent_str}  Feature Flags: {', '.join([f.value for f in self._feature_flags]) or 'None'}\n"
        output += f"{indent_str}  Post Processor: {self._post_processor or 'None'}\n"
        output += f"{indent_str}  Post Processor Args: {self._post_processor_args or 'None'}\n"
        output += super()._dump_self(indent_str)
        return output

    def to_dict(self) -> Dict:
        data = super().to_dict()
        data.update(
            {
                "id": self.get_id(),
                "post_processor": self.post_processor,
                "post_processor_args": self.post_processor_args,
                "feature_flags": [f.value for f in self.feature_flags],
            }
        )
        return data

    @classmethod
    def _from_dict_self(cls, data: Dict) -> "Machine":
        instance = cls(name=data["name"], label=data.get("label"), id=data["id"])
        feature_flags = [MachineFeature(f) for f in data.get("feature_flags", [])]
        instance.post_processor = data.get("post_processor", instance.post_processor)
        instance.post_processor_args = (
            data.get("post_processor_args") or instance.post_processor_args
        )
        instance.icon = data.get("icon", instance.icon)
        instance.feature_flags = feature_flags
        return instance
