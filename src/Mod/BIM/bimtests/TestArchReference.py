# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
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

import os
import tempfile
import types
import zipfile

import Arch
import ArchReference
from bimtests import TestArchBase


class TestArchReference(TestArchBase.TestArchBase):

    def test_makeReference(self):
        """Test the makeReference function."""
        operation = "Testing makeReference..."
        self.printTestMessage(operation)

        obj = Arch.makeReference()
        self.assertIsNotNone(obj, "makeReference failed to create an object")
        self.assertEqual(obj.Label, "External Reference", "Incorrect default label for Reference")

    def test_whole_object_skips_part_design_body_children(self):
        """Test that whole-object references keep the Body and skip its child shapes."""
        operation = "Testing PartDesign Body child filtering..."
        self.printTestMessage(operation)

        filename = self._makeReferenceFile("""<?xml version="1.0" encoding="utf-8"?>
<Document>
  <Objects Count="3">
    <Object type="PartDesign::Body" name="Body" id="1" />
    <Object type="Sketcher::SketchObject" name="Sketch" id="2" />
    <Object type="PartDesign::Pad" name="Pad" id="3" />
  </Objects>
  <ObjectData Count="3">
    <Object name="Body">
      <Properties Count="4">
        <Property name="Group" type="App::PropertyLinkList">
          <LinkList count="2">
            <Link value="Sketch"/>
            <Link value="Pad"/>
          </LinkList>
        </Property>
        <Property name="Label" type="App::PropertyString">
          <String value="Body"/>
        </Property>
        <Property name="Shape" type="Part::PropertyPartShape">
          <Part file="Body.Shape.brp"/>
        </Property>
      </Properties>
    </Object>
    <Object name="Sketch">
      <Properties Count="2">
        <Property name="Label" type="App::PropertyString">
          <String value="Sketch"/>
        </Property>
        <Property name="Shape" type="Part::PropertyPartShape">
          <Part file="Sketch.Shape.brp"/>
        </Property>
      </Properties>
    </Object>
    <Object name="Pad">
      <Properties Count="2">
        <Property name="Label" type="App::PropertyString">
          <String value="Pad"/>
        </Property>
        <Property name="Shape" type="Part::PropertyPartShape">
          <Part file="Pad.Shape.brp"/>
        </Property>
      </Properties>
    </Object>
  </ObjectData>
</Document>
""")

        parts = self._readReferenceParts(filename)

        self.assertFalse(parts["Body"][3], "Body shape must be imported")
        self.assertTrue(parts["Sketch"][3], "Body child sketch must be skipped")
        self.assertTrue(parts["Pad"][3], "Body child feature must be skipped")

    def test_whole_object_keeps_building_part_children(self):
        """Test that BuildingPart still skips itself, not its children."""
        operation = "Testing BuildingPart child filtering..."
        self.printTestMessage(operation)

        filename = self._makeReferenceFile("""<?xml version="1.0" encoding="utf-8"?>
<Document>
  <Objects Count="2">
    <Object type="App::GeometryPython" name="BuildingPart" id="1" />
    <Object type="Part::Feature" name="Wall" id="2" />
  </Objects>
  <ObjectData Count="2">
    <Object name="BuildingPart">
      <Properties Count="4">
        <Property name="Group" type="App::PropertyLinkList">
          <LinkList count="1">
            <Link value="Wall"/>
          </LinkList>
        </Property>
        <Property name="Label" type="App::PropertyString">
          <String value="BuildingPart"/>
        </Property>
        <Property name="Proxy" type="App::PropertyPythonObject">
          <Python value="" encoded="yes" module="ArchBuildingPart" class="BuildingPart"/>
        </Property>
        <Property name="Shape" type="Part::PropertyPartShape">
          <Part file="BuildingPart.Shape.brp"/>
        </Property>
      </Properties>
    </Object>
    <Object name="Wall">
      <Properties Count="2">
        <Property name="Label" type="App::PropertyString">
          <String value="Wall"/>
        </Property>
        <Property name="Shape" type="Part::PropertyPartShape">
          <Part file="Wall.Shape.brp"/>
        </Property>
      </Properties>
    </Object>
  </ObjectData>
</Document>
""")

        parts = self._readReferenceParts(filename)

        self.assertTrue(parts["BuildingPart"][3], "BuildingPart shape must be skipped")
        self.assertFalse(parts["Wall"][3], "BuildingPart child shape must be imported")

    def _makeReferenceFile(self, document_xml):
        fd, filename = tempfile.mkstemp(suffix=".FCStd")
        os.close(fd)
        self.addCleanup(lambda: os.path.exists(filename) and os.remove(filename))
        with zipfile.ZipFile(filename, "w") as zdoc:
            zdoc.writestr("Document.xml", document_xml)
        return filename

    def _readReferenceParts(self, filename):
        reference = ArchReference.ArchReference.__new__(ArchReference.ArchReference)
        obj = types.SimpleNamespace(Document=types.SimpleNamespace(FileName=filename))
        return reference.getPartsListFCSTD(obj, filename)
