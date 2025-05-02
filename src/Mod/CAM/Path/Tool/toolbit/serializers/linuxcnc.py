from typing import Optional
from ..models.base import ToolBit
from ..mixins import RotaryToolBitMixin
from .base import ToolBitSerializer


class LinuxCNCToolBitSerializer(ToolBitSerializer):
    NAME = "LinuxCNC"
    EXTENSIONS = ()  # LinuxCNC does not have tool files; tools are rows in tool tables
    ID_POOL = 0

    def serialize_toolbit(self, bit: ToolBit) -> bytes:
        self.ID_POOL += 1

        pocket = ""  # TODO: bit.get_pocket() or ''
        if isinstance(bit, RotaryToolBitMixin):
            # TODO: preferred export units should be passed to serializer constructor,
            # and whoever creates an instance of the serializer would probably retrieve
            # the machine units from a Machine object before creating the serializer.
            diameter = bit.get_diameter().getValueAs("mm")
        else:
            diameter = None

        line = f"T{self.ID_POOL} P{pocket} D{diameter} ;{bit.get_label()}"
        return line.encode("ascii","ignore")

    def deserialize_toolbit(self, data: bytes) -> Optional[ToolBit]:
        raise NotImplementedError("LinuxCNC toolbit deserialization is not yet supported")
