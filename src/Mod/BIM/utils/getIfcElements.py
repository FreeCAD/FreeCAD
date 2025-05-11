# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""This script converts the computer iterpretable listing ifcXML XSD into a list
of non-abstract children of IfcProduct"""

import xml.sax, json, copy


class IfcElementHandler(xml.sax.ContentHandler):
    def __init__(self):
        super().__init__()
        self.elements = {}
        self.current_element_name = None
        self.enums = {}
        self.current_enum_name = None
        self.attribute_stack = []

    def startElement(self, name, attrs):
        if name == "xs:element" and "substitutionGroup" in attrs:
            self.elements[attrs["name"]] = {
                "is_abstract": True if "abstract" in attrs else False,
                "parent": attrs["substitutionGroup"][len("ifc:") :],
                "attributes": [],
            }
            self.current_element_name = attrs["name"]
        elif name == "xs:simpleType" and "name" in attrs and "Enum" in attrs["name"]:
            self.current_enum_name = attrs["name"]
            self.enums[self.current_enum_name] = []
        elif name == "xs:enumeration" and self.current_enum_name:
            self.enums[self.current_enum_name].append(attrs["value"].upper())
        elif (
            name == "xs:attribute"
            and self.current_element_name
            and "name" in attrs
            and "type" in attrs
        ):
            self.elements[self.current_element_name]["attributes"].append(
                {
                    "name": attrs["name"],
                    "type": attrs["type"].replace("ifc:", ""),
                }
            )

    def endDocument(self):
        elements = {}

        for name, data in self.elements.items():
            for index, attribute in enumerate(data["attributes"]):
                data["attributes"][index] = self.resolve_enums(attribute)

        for name, data in self.elements.items():
            if data["is_abstract"]:
                continue
            if self.is_an_ifcproduct(data):
                self.attribute_stack = []
                self.get_parent_attributes(data)
                elements[name] = copy.deepcopy(data)
                elements[name]["attributes"] = copy.deepcopy(self.attribute_stack)

        self.elements = elements

    def resolve_enums(self, attribute):
        if attribute["type"] in self.enums:
            attribute["is_enum"] = True
            attribute["enum_values"] = self.enums[attribute["type"]]
            return attribute
        attribute["is_enum"] = False
        attribute["enum_values"] = []
        return attribute

    def get_parent_attributes(self, data):
        self.attribute_stack.extend(data["attributes"])
        if (
            data["parent"] != "IfcProduct"
        ):  # For now, we treat attributes above IfcProduct in a special way
            self.get_parent_attributes(self.elements[data["parent"]])

    def is_an_ifcproduct(self, data):
        if data["parent"] == "IfcProduct":
            return True
        else:
            for name, parent_data in self.elements.items():
                if name == data["parent"]:
                    return self.is_an_ifcproduct(parent_data)
        return False


xsd_path = "IFC4_ADD2.xsd"
handler = IfcElementHandler()
parser = xml.sax.make_parser()
parser.setContentHandler(handler)
parser.parse(xsd_path)
print(json.dumps(handler.elements, indent=4))
