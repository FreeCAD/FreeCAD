# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Classes for working with Addon metadata, as documented at
https://wiki.FreeCAD.org/Package_metadata"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum, auto
from typing import Tuple, Dict, List, Optional

try:
    # If this system provides a secure parser, use that:
    import defusedxml.ElementTree as ET
except ImportError:
    # Otherwise fall back to the Python standard parser
    import xml.etree.ElementTree as ET


@dataclass
class Contact:
    name: str
    email: str = ""


@dataclass
class License:
    name: str
    file: str = ""


class UrlType(IntEnum):
    bugtracker = 0
    discussion = auto()
    documentation = auto()
    readme = auto()
    repository = auto()
    website = auto()

    def __str__(self):
        return f"{self.name}"


@dataclass
class Url:
    location: str
    type: UrlType
    branch: str = ""


class Version:
    """Provide a more useful representation of Version information"""

    def __init__(self, from_string: str = None, from_list=None):
        """If from_string is a string, it is parsed to get the version. If from_list
        exists (and no string was provided), it is treated as a version list of
        [major:int, minor:int, patch:int, pre:str]"""
        self.version_as_list = [0, 0, 0, ""]
        if from_string is not None:
            self._init_from_string(from_string)
        elif from_list is not None:
            self._init_from_list(from_list)

    def _init_from_string(self, from_string: str):
        """Find the first digit in the given string, and send that substring off for
        parsing."""
        counter = 0
        for char in from_string:
            if char.isdigit():
                break
            counter += 1
        self._parse_string_to_tuple(from_string[counter:])

    def _init_from_list(self, from_list):
        for index, element in enumerate(from_list):
            if index < 3:
                self.version_as_list[index] = int(element)
            elif index == 3:
                self.version_as_list[index] = str(element)
            else:
                break

    def _parse_string_to_tuple(self, from_string: str):
        """We hand-parse only simple version strings, of the form 1.2.3suffix -- only
        the first digit is required."""
        splitter = from_string.split(".", 2)
        counter = 0
        for component in splitter:
            try:
                self.version_as_list[counter] = int(component)
                counter += 1
            except ValueError:
                if counter == 0:
                    raise ValueError(f"Invalid version string {from_string}")
                number, text = self._parse_final_entry(component)
                self.version_as_list[counter] = number
                self.version_as_list[3] = text

    @staticmethod
    def _parse_final_entry(final_string: str) -> Tuple[int, str]:
        """The last value is permitted to contain both a number and a word, and needs
        to be split"""
        digits = ""
        for c in final_string:
            if c.isdigit():
                digits += c
            else:
                break
        return int(digits), final_string[len(digits) :]

    def __repr__(self) -> str:
        v = self.version_as_list
        return f"{v[0]}.{v[1]}.{v[2]} {v[3]}"

    def __eq__(self, other) -> bool:
        return self.version_as_list == other.version_as_list

    def __ne__(self, other) -> bool:
        return not (self == other)

    def __lt__(self, other) -> bool:
        for a, b in zip(self.version_as_list, other.version_as_list):
            if a != b:
                return a < b
        return False

    def __gt__(self, other) -> bool:
        if self.version_as_list == other.version_as_list:
            return False
        return not (self < other)

    def __ge__(self, other) -> bool:
        return self > other or self == other

    def __le__(self, other) -> bool:
        return self < other or self == other


class DependencyType(IntEnum):
    automatic = 0
    internal = auto()
    addon = auto()
    python = auto()

    def __str__(self):
        return f"{self.name}"


@dataclass
class Dependency:
    package: str
    version_lt: str = ""
    version_lte: str = ""
    version_eq: str = ""
    version_gte: str = ""
    version_gt: str = ""
    condition: str = ""
    optional: bool = False
    dependency_type: DependencyType = DependencyType.automatic


@dataclass
class GenericMetadata:
    """Used to store unrecognized elements"""

    contents: str = ""
    attributes: Dict[str, str] = field(default_factory=dict)


@dataclass
class Metadata:
    """A pure-python implementation of the Addon Manager's Metadata handling class"""

    name: str = ""
    version: Version = None
    date: str = ""
    description: str = ""
    type: str = ""
    maintainer: List[Contact] = field(default_factory=list)
    license: List[License] = field(default_factory=list)
    url: List[Url] = field(default_factory=list)
    author: List[Contact] = field(default_factory=list)
    depend: List[Dependency] = field(default_factory=list)
    conflict: List[Dependency] = field(default_factory=list)
    replace: List[Dependency] = field(default_factory=list)
    tag: List[str] = field(default_factory=list)
    icon: str = ""
    classname: str = ""
    subdirectory: str = ""
    file: List[str] = field(default_factory=list)
    freecadmin: Version = None
    freecadmax: Version = None
    pythonmin: Version = None
    content: Dict[str, List[Metadata]] = field(default_factory=dict)  # Recursive def.


def get_first_supported_freecad_version(metadata: Metadata) -> Optional[Version]:
    """Look through all content items of this metadata element and determine what the
    first version of freecad that ANY of the items support is. For example, if it
    contains several workbenches, some of which require v0.20, and some 0.21, then
    0.20 is returned. Returns None if frecadmin is unset by any part of this object."""

    current_earliest = metadata.freecadmin if metadata.freecadmin is not None else None
    for content_class in metadata.content.values():
        for content_item in content_class:
            content_first = get_first_supported_freecad_version(content_item)
            if content_first is not None:
                if current_earliest is None:
                    current_earliest = content_first
                else:
                    current_earliest = min(current_earliest, content_first)

    return current_earliest


class MetadataReader:
    """Read metadata XML data and construct a Metadata object"""

    @staticmethod
    def from_file(filename: str) -> Metadata:
        """A convenience function for loading the Metadata from a file"""
        with open(filename, "rb") as f:
            data = f.read()
            return MetadataReader.from_bytes(data)

    @staticmethod
    def from_bytes(data: bytes) -> Metadata:
        """Read XML data from bytes and use it to construct Metadata"""
        element_tree = ET.fromstring(data)
        return MetadataReader._process_element_tree(element_tree)

    @staticmethod
    def _process_element_tree(root: ET.Element) -> Metadata:
        """Parse an element tree and convert it into a Metadata object"""
        namespace = MetadataReader._determine_namespace(root)
        return MetadataReader._create_node(namespace, root)

    @staticmethod
    def _determine_namespace(root: ET.Element) -> str:
        accepted_namespaces = ["{https://wiki.freecad.org/Package_Metadata}", ""]
        for ns in accepted_namespaces:
            if root.tag == f"{ns}package":
                return ns
        raise RuntimeError("No 'package' element found in metadata file")

    @staticmethod
    def _parse_child_element(namespace: str, child: ET.Element, metadata: Metadata):
        """Figure out what sort of metadata child represents, and add it to the
        metadata object."""

        tag = child.tag[len(namespace) :]
        if tag in ["name", "date", "description", "type", "icon", "classname", "subdirectory"]:
            # Text-only elements
            metadata.__dict__[tag] = child.text
        elif tag in ["version", "freecadmin", "freecadmax", "pythonmin"]:
            try:
                metadata.__dict__[tag] = Version(from_string=child.text)
            except ValueError:
                print(
                    f"Invalid version specified for tag {tag} in Addon {metadata.name}: {child.text}"
                )
                metadata.__dict__[tag] = Version(from_list=[0, 0, 0])
        elif tag in ["tag", "file"]:
            # Lists of strings
            if child.text:
                metadata.__dict__[tag].append(child.text)
        elif tag in ["maintainer", "author"]:
            # Lists of contacts
            metadata.__dict__[tag].append(MetadataReader._parse_contact(child))
        elif tag == "license":
            # List of licenses
            metadata.license.append(MetadataReader._parse_license(child))
        elif tag == "url":
            # List of urls
            metadata.url.append(MetadataReader._parse_url(child))
        elif tag in ["depend", "conflict", "replace"]:
            # Lists of dependencies
            metadata.__dict__[tag].append(MetadataReader._parse_dependency(child))
        elif tag == "content":
            MetadataReader._parse_content(namespace, metadata, child)

    @staticmethod
    def _parse_contact(child: ET.Element) -> Contact:
        email = child.attrib["email"] if "email" in child.attrib else ""
        return Contact(name=child.text, email=email)

    @staticmethod
    def _parse_license(child: ET.Element) -> License:
        file = child.attrib["file"] if "file" in child.attrib else ""
        return License(name=child.text, file=file)

    @staticmethod
    def _parse_url(child: ET.Element) -> Url:
        url_type = UrlType.website
        branch = ""
        if "type" in child.attrib and child.attrib["type"] in UrlType.__dict__:
            url_type = UrlType[child.attrib["type"]]
            if url_type == UrlType.repository:
                branch = child.attrib["branch"] if "branch" in child.attrib else ""
        return Url(location=child.text, type=url_type, branch=branch)

    @staticmethod
    def _parse_dependency(child: ET.Element) -> Dependency:
        v_lt = child.attrib["version_lt"] if "version_lt" in child.attrib else ""
        v_lte = child.attrib["version_lte"] if "version_lte" in child.attrib else ""
        v_eq = child.attrib["version_eq"] if "version_eq" in child.attrib else ""
        v_gte = child.attrib["version_gte"] if "version_gte" in child.attrib else ""
        v_gt = child.attrib["version_gt"] if "version_gt" in child.attrib else ""
        condition = child.attrib["condition"] if "condition" in child.attrib else ""
        optional = "optional" in child.attrib and child.attrib["optional"].lower() == "true"
        dependency_type = DependencyType.automatic
        if "type" in child.attrib and child.attrib["type"] in DependencyType.__dict__:
            dependency_type = DependencyType[child.attrib["type"]]
        return Dependency(
            child.text,
            version_lt=v_lt,
            version_lte=v_lte,
            version_eq=v_eq,
            version_gte=v_gte,
            version_gt=v_gt,
            condition=condition,
            optional=optional,
            dependency_type=dependency_type,
        )

    @staticmethod
    def _parse_content(namespace: str, metadata: Metadata, root: ET.Element):
        """Given a content node, loop over its children, and if they are a recognized
        element type, recurse into each one to parse it."""
        known_content_types = ["workbench", "macro", "preferencepack"]
        for child in root:
            content_type = child.tag[len(namespace) :]
            if content_type in known_content_types:
                if content_type not in metadata.content:
                    metadata.content[content_type] = []
                metadata.content[content_type].append(MetadataReader._create_node(namespace, child))

    @staticmethod
    def _create_node(namespace, child) -> Metadata:
        new_content_item = Metadata()
        for content_child in child:
            MetadataReader._parse_child_element(namespace, content_child, new_content_item)
        return new_content_item


class MetadataWriter:
    """Utility class for serializing a Metadata object into the package.xml standard
    XML file."""

    @staticmethod
    def write(metadata: Metadata, path: str):
        """Write the metadata to a file located at path. Overwrites the file if it
        exists. Raises OSError if writing fails."""
        tree = MetadataWriter._create_tree_from_metadata(metadata)
        tree.write(path)

    @staticmethod
    def _create_tree_from_metadata(metadata: Metadata) -> ET.ElementTree:
        """Create the XML ElementTree representation of the given Metadata object."""
        tree = ET.ElementTree()
        root = tree.getroot()
        root.attrib["xmlns"] = "https://wiki.freecad.org/Package_Metadata"
        for key, value in metadata.__dict__.items():
            if isinstance(value, str):
                node = ET.SubElement(root, key)
                node.text = value
            else:
                MetadataWriter._create_list_node(metadata, key, root)
        return tree

    @staticmethod
    def _create_list_node(metadata: Metadata, key: str, root: ET.Element):
        for item in metadata.__dict__[key]:
            node = ET.SubElement(root, key)
            if key in ["maintainer", "author"]:
                if item.email:
                    node.attrib["email"] = item.email
                node.text = item.name
            elif key == "license":
                if item.file:
                    node.attrib["file"] = item.file
                node.text = item.name
            elif key == "url":
                if item.branch:
                    node.attrib["branch"] = item.branch
                node.attrib["type"] = str(item.type)
                node.text = item.location
            elif key in ["depend", "conflict", "replace"]:
                for dep_key, dep_value in item.__dict__.items():
                    if isinstance(dep_value, str) and dep_value:
                        node.attrib[dep_key] = dep_value
                    elif isinstance(dep_value, bool):
                        node.attrib[dep_key] = "True" if dep_value else "False"
                    elif isinstance(dep_value, DependencyType):
                        node.attrib[dep_key] = str(dep_value)
