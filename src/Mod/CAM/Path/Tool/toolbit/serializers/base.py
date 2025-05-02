# -*- coding: utf-8 -*-
from abc import ABC
from typing import Any, Tuple, Optional
from ..models.base import ToolBit

class ToolBitSerializer(ABC):
    NAME: str
    EXTENSIONS: Tuple[str]

    def __init__(self, *args: Any, **kwargs: Any):
        pass

    def serialize_toolbit(self, bit: ToolBit) -> bytes:
        raise NotImplementedError

    def deserialize_toolbit(self, data: bytes) -> Optional[ToolBit]:
        raise NotImplementedError
