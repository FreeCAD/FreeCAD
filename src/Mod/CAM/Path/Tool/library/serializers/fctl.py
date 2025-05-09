import json
from typing import Optional
from ..models.library import Library
from .base import LibrarySerializer


class FCTBLibrarySerializer(LibrarySerializer):
    NAME = "FreeCAD Tool Bit Library"
    EXTENSIONS = (".fctl",)

    def serialize_library(self, library: Library) -> bytes:
        attrs = library.to_dict()
        return json.dumps(attrs, sort_keys=True, indent=2).encode("utf-8")

    def deserialize_library(self, data: bytes) -> Optional[Library]:
        attrs = json.loads(data.decode("utf-8"))
        return Library.from_dict(attrs)
