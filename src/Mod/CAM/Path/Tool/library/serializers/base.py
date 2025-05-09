# -*- coding: utf-8 -*-
from abc import ABC
from typing import Any, Tuple, Optional
from ..models.library import Library

class LibrarySerializer(ABC):
    NAME: str
    EXTENSIONS: Tuple[str]

    def __init__(self, *args: Any, **kwargs: Any):
        pass

    def serialize_library(self, library: Library) -> bytes:
        raise NotImplementedError

    def deserialize_library(self, data: bytes) -> Optional[Library]:
        raise NotImplementedError
