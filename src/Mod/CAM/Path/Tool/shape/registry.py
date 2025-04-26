# -*- coding: utf-8 -*-

import os
import pathlib
import zipfile
from typing import Optional, Mapping
import Path
import xml.etree.ElementTree as ET
from . import util
from .base import ToolBitShape


class ShapeRegistry:
    """
    Manages ToolBitShape files.
    """

    def __init__(self, shapes_dir: pathlib.Path):
        self.set_dir(shapes_dir)

    def set_dir(self, shapes_dir: pathlib.Path):
        self.shape_dir = shapes_dir
        self.shape_dir.mkdir(parents=True, exist_ok=True)

    def get_shape_name_from_filename(self, filename: str) -> str:
        # 1st try the most reliable method:
        # Temporarily open the file to read the name of the base object.
        # Since opening the file in FreeCAD is slow and we may need to
        # use this hundreds of time, we read from the XML directly instead.
        filepath = self.shape_dir / filename
        if filepath.exists():
            with zipfile.ZipFile(filepath, 'r') as z:
                with z.open('Document.xml') as doc_xml:
                    tree = ET.parse(doc_xml)
                    root = tree.getroot()

                    # Extract name of the main Body from XML tree using xpath.
                    # The body should be a PartDesign::Body, and its label is
                    # stored in an Property element with a matching name.
                    label = None
                    xpath = './/Object[@name="Body"]//Property[@name="Label"]/String'
                    body_label_elem = root.find(xpath)
                    if body_label_elem is not None:
                        label = body_label_elem.get('value')

                    known = [c.name for c in ToolBitShape.__subclasses__()]
                    if label in known:
                        return label

        # 2nd:
        # If the shape name could not be determined (for example because the
        # file does not exist), try to infer it from the filename.
        fname = util.get_shape_class_from_name(os.path.splitext(filename)[0])
        if fname:
            return fname.name

        # 3rd: Default to Endmill if all of the methods failed
        return "Endmill"

    def get_shape_from_filename(self, filename: str, params: Optional[Mapping]=None) -> ToolBitShape:
        """Retrieves a shape class by filename."""
        params = params or {}

        # For backward compatibility we fall back to getting the shape file by
        # alias if the file does not exist.
        filepath = self.shape_dir / filename
        if not filepath.exists():
            fname = util.get_shape_filename_from_alias(os.path.splitext(filename)[0])
            if not fname:
                raise Exception(f"ToolBitShape file '{filename}' not found!\n")
            Path.Log.warning(f"ToolBitShape '{filename}' not found. Trying legacy '{fname}'\n")
            filepath = self.shape_dir / fname

        return ToolBitShape.from_file(filepath, **params)


# Global instance of the ShapeRegistry
SHAPE_REGISTRY = ShapeRegistry(util.get_builtin_shape_dir())
