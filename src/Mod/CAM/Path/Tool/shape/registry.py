# -*- coding: utf-8 -*-

import os
import pathlib
import zipfile
import shutil
from typing import Optional, Mapping, Type
import Path
import xml.etree.ElementTree as ET
from Path.Preferences import addToolPreferenceObserver
from .base import ToolBitShape


class ShapeRegistry:
    """
    Manages ToolBitShape files.
    """

    def __init__(self, shapes_dir: pathlib.Path):
        self.set_dir(shapes_dir)

    def is_empty(self):
        return not any(self.shape_dir.iterdir())

    def add_builtin_shapes(self):
        """Copies built-in shapes to the shape directory"""
        builtin = Path.Preferences.getBuiltinShapePath()
        for filepath in builtin.iterdir():
            dest = self.shape_dir / filepath.name
            if dest.exists():
                continue
            shutil.copy(filepath, dest)

    def ensure_initialized(self):
        if self.is_empty():
            Path.Log.info(
                f"ToolBitShape directory '{self.shape_dir}' empty; copying built-in shapes"
            )
            self.add_builtin_shapes()
            Path.Log.info(f"ToolBitShapes successfully copied to '{self.shape_dir}'")

    def set_dir(self, shapedir: pathlib.Path):
        self.shape_dir = shapedir
        self.shape_dir.mkdir(parents=True, exist_ok=True)

    @classmethod
    def get_shape_class_from_name(cls, name: str) -> Optional[Type[ToolBitShape]]:
        for cls in ToolBitShape.__subclasses__():
            if cls.name == name or name in cls.aliases:
                return cls
        return None

    def get_shape_filename_from_alias(self, alias: str) -> Optional[pathlib.Path]:
        for cls in ToolBitShape.__subclasses__():
            if cls.name == alias or alias in cls.aliases:
                return self.shape_dir / (cls.name.lower() + ".fcstd")
        return None

    def get_shape_name_from_filename(self, filename: str) -> str:
        # 1st try the most reliable method:
        # Temporarily open the file to read the name of the base object.
        # Since opening the file in FreeCAD is slow and we may need to
        # use this hundreds of time, we read from the XML directly instead.
        filepath = self.shape_dir / filename
        if filepath.exists():
            with zipfile.ZipFile(filepath, "r") as z:
                with z.open("Document.xml") as doc_xml:
                    tree = ET.parse(doc_xml)
                    root = tree.getroot()

                    # Extract name of the main Body from XML tree using xpath.
                    # The body should be a PartDesign::Body, and its label is
                    # stored in an Property element with a matching name.
                    label = None
                    xpath = './/Object[@name="Body"]//Property[@name="Label"]/String'
                    body_label_elem = root.find(xpath)
                    if body_label_elem is not None:
                        label = body_label_elem.get("value")

                    known = [c.name for c in ToolBitShape.__subclasses__()]
                    if label in known:
                        return label

        # 2nd:
        # If the shape name could not be determined (for example because the
        # file does not exist), try to infer it from the filename.
        fname = self.get_shape_class_from_name(os.path.splitext(filename)[0])
        if fname:
            return fname.name

        # 3rd: Default to Endmill if all of the methods failed
        return "Endmill"

    def get_shape_from_filename(
        self, filename: str, params: Optional[Mapping] = None
    ) -> ToolBitShape:
        """Retrieves a shape class by filename."""
        params = params or {}

        # For backward compatibility we fall back to getting the shape file by
        # alias if the file does not exist.
        filepath = self.shape_dir / filename
        if not filepath.exists():
            base = os.path.splitext(filename)[0]
            filepath = self.get_shape_filename_from_alias(base)
            Path.Log.warning(
                f"ToolBitShape '{filename}' not found in "
                f"{self.shape_dir}. Trying legacy '{filepath}'"
            )

        # If that also fails, try to find a built-in shape file.
        if not filepath or not filepath.exists():
            base = os.path.splitext(filename)[0]
            filepath = Path.Preferences.getBuiltinShapePath() / filename
            Path.Log.warning(
                f"ToolBitShape '{filename}' still not found in "
                f"{self.shape_dir}. Trying built-in '{filepath}'"
            )

        return ToolBitShape.from_file(filepath, **params)

    def get_shapes(self):
        """Returns a list of all shapes"""
        shapes = []
        for filepath in self.shape_dir.glob("*.fcstd"):
            try:
                shape = ToolBitShape.from_file(filepath)
                shapes.append(shape)
            except Exception as e:
                Path.Log.error(f"Failed to load ToolBitShape '{filepath}': {e}")
        return shapes


def on_tool_path_changed(group, key, value):
    Path.Log.track("signal received:", group, key, value)
    SHAPE_REGISTRY.set_dir(Path.Preferences.getShapePath())
    SHAPE_REGISTRY.ensure_initialized()


# Global instance of the ShapeRegistry
SHAPE_REGISTRY = ShapeRegistry(Path.Preferences.getShapePath())
addToolPreferenceObserver(on_tool_path_changed)
