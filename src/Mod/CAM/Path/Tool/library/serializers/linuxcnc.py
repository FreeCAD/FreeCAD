import io
from typing import Mapping, List, Optional, Type
import FreeCAD
from ...assets import Asset, AssetUri, AssetSerializer
from ..models.library import Library


class LinuxCNCSerializer(AssetSerializer):
    for_class: Type[Asset] = Library
    extensions: tuple[str] = (".tbl",)
    mime_type: str = "text/plain"

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "Camotics Tool Table")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        if not isinstance(asset, Library):
            raise TypeError("Asset must be a Library instance")

        output = io.BytesIO()
        for bit_no, bit in sorted(asset._bit_nos.items()):
            diameter = bit.get_diameter() or 2
            pocket = "P"  # TODO: is there a better way?
            # Format diameter to one decimal place and remove units
            diameter_value = diameter.Value if hasattr(diameter, 'Value') else diameter
            line = f"T{bit_no} {pocket} D{diameter_value:.1f} ;{bit.get_label()}\n"
            output.write(line.encode("ascii", "ignore"))

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
        raise NotImplementedError(
            "Deserialization is not supported for LinuxCNC .tbl files."
        )
