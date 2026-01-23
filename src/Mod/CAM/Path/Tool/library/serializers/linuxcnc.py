# SPDX-License-Identifier: LGPL-2.1-or-later

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
import io
from typing import Mapping, List, Optional, Type
import FreeCAD
import Path
from ...assets import Asset, AssetUri, AssetSerializer
from ...toolbit import ToolBit
from ...toolbit.mixins import RotaryToolBitMixin
from ..models.library import Library


class LinuxCNCSerializer(AssetSerializer):
    for_class: Type[Asset] = Library
    extensions: tuple[str] = (".tbl",)
    mime_type: str = "text/plain"
    can_import = False

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "LinuxCNC Tool Table")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        if not isinstance(asset, Library):
            raise TypeError("Asset must be a Library instance")

        output = io.BytesIO()
        for bit_no, bit in sorted(asset._bit_nos.items()):
            # Connor: assert isinstance(bit, ToolBit)
            # if not isinstance(bit, RotaryToolBitMixin):
            #     Path.Log.warning(
            #         f"Skipping too {bit.label} (bit.id) because it is not a rotary tool"
            #     )
            #     continue
            # Commenting this out. Why did we skip because it is not a rotary tool?
            diameter = bit.get_diameter().getUserPreferred()[0]
            pocket = "P0"  # TODO: is there a better way?
            # TODO: Strip units by splitting at the first space if diameter is a string
            # This is where we need a machine definition so we can export these out correctly
            # for a metric or imperial machine
            # Using user preferred for now
            if hasattr(diameter, "Value"):
                diameter_value = str(diameter.Value).replace(",", ".")
            elif isinstance(diameter, str):
                diameter_value = diameter.split(" ")[0].replace(",", ".")
            else:
                diameter_value = str(diameter).replace(",", ".")
            line = (
                f"T{bit_no} {pocket} X0 Y0 Z0 A0 B0 C0 U0 V0 W0 "
                f"D{diameter_value} I0 J0 Q0 ;{bit.label}\n"
            )
            output.write(line.encode("utf-8"))

        return output.getvalue()

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Library:
        # LinuxCNC .tbl files do not contain enough information to fully
        # reconstruct a Library and its ToolBits.
        # Therefore, deserialization is not supported.
        raise NotImplementedError("Deserialization is not supported for LinuxCNC .tbl files.")
