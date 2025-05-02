# -*- coding: utf-8 -*-

import pathlib
import shutil
from typing import Optional, Mapping, Type
import Path
from Path.Preferences import addToolPreferenceObserver
from .models.base import ToolBit
from .serializers import FCTBSerializer


class ToolBitRegistry:
    """
    Manages ToolBits files.
    """

    def __init__(self, bit_dir: pathlib.Path):
        self.set_dir(bit_dir)

    def is_empty(self):
        return not any(self.bit_dir.iterdir())

    def add_builtin_bits(self):
        """Copies built-in bits to the Bit directory"""
        builtin = Path.Preferences.getBuiltinToolBitPath()
        for filepath in builtin.iterdir():
            dest = self.bit_dir / filepath.name
            if dest.exists():
                continue
            shutil.copy(filepath, dest)

    def ensure_initialized(self):
        if self.is_empty():
            Path.Log.info(
                f"ToolBit directory '{self.bit_dir}' empty; copying built-in bits"
            )
            self.add_builtin_bits()
            Path.Log.info(f"ToolBits successfully copied to '{self.bit_dir}'")

    def set_dir(self, bit_dir: pathlib.Path):
        self.bit_dir = bit_dir
        self.bit_dir.mkdir(parents=True, exist_ok=True)

    def get_bit_from_filename(
        self, filename: str, params: Optional[Mapping] = None
    ) -> ToolBit:
        """Retrieves a ToolBit by filename."""
        params = params or {}
        filepath = self.bit_dir / filename
        if not filepath.exists():
            Path.Log.error(f"ToolBit file '{filepath}' does not exist.")
            return None

        serializer = FCTBSerializer()
        data = filepath.read_bytes()
        return serializer.deserialize_toolbit(data)

    @classmethod
    def get_bit_class_from_shape_name(cls, shape_name: str) -> Optional[Type[ToolBit]]:
        """
        Returns the ToolBit subclass for the given shape name.
        """
        for subclass in ToolBit.__subclasses__():
            shape_cls = subclass.SHAPE_CLASS
            if shape_cls.name == shape_name or shape_name in shape_cls.aliases:
                return subclass
        return None


def on_tool_path_changed(group, key, value):
    Path.Log.track("signal received:", group, key, value)
    TOOLBIT_REGISTRY.set_dir(Path.Preferences.getToolBitPath())
    TOOLBIT_REGISTRY.ensure_initialized()


# Global instance of the ToolBitRegistry
TOOLBIT_REGISTRY = ToolBitRegistry(Path.Preferences.getToolBitPath())
addToolPreferenceObserver(on_tool_path_changed)
