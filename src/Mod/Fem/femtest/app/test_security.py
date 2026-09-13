# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

__title__ = "FEM App security unit tests"

import io
import os
import shutil
import tempfile
import unittest
import zipfile

import FreeCAD

from .support_utils import fcc_print

# A document holding a single Fem::FemPostPipeline whose Data property points at an embedded
# file called "payload.zip". Restoring Fem::PropertyPostDataObject selects its zip branch from
# that name, which is what feeds the inner archive to the extraction loop. The FEM module is
# loaded on demand by the type system when it meets the object type, so no workbench needs to
# be active for this to run.
MALICIOUS_DOCUMENT_XML = b"""<?xml version='1.0' encoding='utf-8'?>
<Document SchemaVersion="4" ProgramVersion="1.1.3" FileVersion="1">
    <Properties Count="1" TransientCount="0">
        <Property name="Label" type="App::PropertyString">
            <String value="zipslip"/>
        </Property>
    </Properties>
    <Objects Count="1">
        <Object type="Fem::FemPostPipeline" name="Pipeline" id="1" />
    </Objects>
    <ObjectData Count="1">
        <Object name="Pipeline">
            <Properties Count="1" TransientCount="0">
                <Property name="Data" type="Fem::PropertyPostDataObject">
                    <Data file="payload.zip"/>
                </Property>
            </Properties>
        </Object>
    </ObjectData>
</Document>"""

PAYLOAD = b"this file should never have been written\n"


class TestPostDataObjectSecurity(unittest.TestCase):
    """Regression tests for GHSA-9vjf-h8f4-c229.

    Fem::PropertyPostDataObject stores multi block datasets as a zip inside the FCStd. The
    names of the entries in that inner zip come straight out of an untrusted file, so joining
    them to the extraction directory unchecked let a crafted document write anywhere the user
    can write.
    """

    fcc_print("import TestPostDataObjectSecurity")

    def setUp(self):
        if "BUILD_FEM_VTK" not in FreeCAD.__cmake__:
            self.skipTest("FEM was built without VTK, PropertyPostDataObject is not compiled")

        self.working_dir = tempfile.mkdtemp(prefix="fem_post_security_")
        self.document = None
        self.escape_targets = []

    def tearDown(self):
        if self.document is not None:
            FreeCAD.closeDocument(self.document.Name)
            self.document = None
        for target in self.escape_targets:
            if os.path.exists(target):
                os.remove(target)
        shutil.rmtree(self.working_dir, ignore_errors=True)

    def set_escape_target(self, name):
        """Return the path a successful escape would land on in the tests below. Nothing is created
        here, this is just the path we've crafted the test to land on if the escape is successful.
        """
        target = os.path.join(FreeCAD.getTempPath(), name)
        self.assertFalse(os.path.exists(target), "stale file from an earlier run: " + str(target))
        self.escape_targets.append(target)
        return target

    def write_malicious_document(self, entry_names):
        """Build an FCStd whose embedded post-processing archive uses the given entry names."""
        inner = io.BytesIO()
        with zipfile.ZipFile(inner, "w", zipfile.ZIP_DEFLATED) as archive:
            # The reader preloads the first entry, so a real file always has a leading dummy
            archive.writestr("dummy", b"")
            for name in entry_names:
                # ZipInfo rewrites os.sep into "/" when it is built from a name, which would
                # turn the backslash cases below into forward slash ones on Windows. Assigning
                # the name afterwards stores the bytes an attacker would actually put there.
                entry = zipfile.ZipInfo("entry")
                entry.filename = name
                entry.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(entry, PAYLOAD)

        path = os.path.join(self.working_dir, "malicious.FCStd")
        with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as outer:
            outer.writestr("Document.xml", MALICIOUS_DOCUMENT_XML)
            outer.writestr("payload.zip", inner.getvalue())
        return path

    def open_malicious_document(self, entry_names):
        path = self.write_malicious_document(entry_names)
        self.document = FreeCAD.openDocument(path)

        # Guard against a vacuous pass: if the hand written Document.xml ever stops matching
        # the schema, the object would silently not be restored and nothing would be extracted
        # regardless of whether the entry names are checked.
        self.assertEqual(len(self.document.Objects), 1)
        self.assertEqual(self.document.Objects[0].TypeId, "Fem::FemPostPipeline")

    def test_forward_slash_traversal_is_rejected(self):
        target = self.set_escape_target("zipslip_forward_escape.txt")
        self.open_malicious_document(["/../zipslip_forward_escape.txt"])
        self.assertFalse(
            os.path.exists(target),
            "a zip entry name escaped the extraction directory: {}".format(target),
        )

    def test_backslash_traversal_is_rejected(self):
        target = self.set_escape_target("zipslip_backslash_escape.txt")
        self.open_malicious_document(["/..\\zipslip_backslash_escape.txt"])
        self.assertFalse(
            os.path.exists(target),
            "a backslash zip entry name escaped the extraction directory: {}".format(target),
        )

    def test_deep_traversal_is_rejected(self):
        target = self.set_escape_target("zipslip_deep_escape.txt")
        deep = "/" + "../" * 30 + os.path.relpath(target, os.path.splitdrive(target)[0] + os.sep)
        self.open_malicious_document([deep.replace(os.sep, "/")])
        self.assertFalse(
            os.path.exists(target),
            "a deep zip entry name escaped the extraction directory: {}".format(target),
        )

    def test_mixed_entries_document_still_opens(self):
        target = self.set_escape_target("zipslip_mixed_escape.txt")
        self.open_malicious_document(["/../zipslip_mixed_escape.txt", "/datafile.vtm"])
        self.assertFalse(os.path.exists(target))

    def test_legitimate_dataset_survives_a_round_trip(self):
        """The names FreeCAD itself writes start with a separator and must keep working. Make sure
        that the separator-prefixed names that get generated aren't rejected by the sanitizer: if
        they were, all old files would break.
        """
        if "BUILD_FEM_VTK_PYTHON" not in FreeCAD.__cmake__:
            self.skipTest("FEM was built without the VTK Python bindings")
        try:
            from vtkmodules.vtkCommonCore import vtkPoints
            from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet, vtkUnstructuredGrid
        except ImportError:
            self.skipTest("the VTK Python modules are not importable")

        points = vtkPoints()
        points.InsertNextPoint(0.0, 0.0, 0.0)
        points.InsertNextPoint(1.0, 0.0, 0.0)
        points.InsertNextPoint(0.0, 1.0, 0.0)
        grid = vtkUnstructuredGrid()
        grid.SetPoints(points)
        dataset = vtkMultiBlockDataSet()
        dataset.SetNumberOfBlocks(1)
        dataset.SetBlock(0, grid)

        path = os.path.join(self.working_dir, "roundtrip.FCStd")
        self.document = FreeCAD.newDocument("post_roundtrip")
        pipeline = self.document.addObject("Fem::FemPostPipeline", "Pipeline")
        pipeline.Data = dataset
        self.document.saveAs(path)
        # Drop the reference before reopening so tearDown never sees a closed document
        name = self.document.Name
        self.document = None
        FreeCAD.closeDocument(name)

        self.document = FreeCAD.openDocument(path)
        restored = self.document.getObject("Pipeline").Data
        self.assertEqual(restored.GetNumberOfBlocks(), 1)
        self.assertEqual(restored.GetBlock(0).GetNumberOfPoints(), 3)
