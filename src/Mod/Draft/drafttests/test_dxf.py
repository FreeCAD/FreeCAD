# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Workbench, DXF import and export tests."""

## @package test_dxf
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, DXF import and export tests.

## \addtogroup drafttests
# @{

import os
import shutil
import tempfile
from collections import Counter

import FreeCAD as App
import Draft
from drafttests import auxiliary as aux
from drafttests import test_base
from draftutils.messages import _msg
import importDXF


class DraftDXF(test_base.DraftTestCaseDoc):
    """Test reading and writing of DXF files with Draft."""

    def test_read_dxf_Issue24314(self):
        """Verify that reading a DXF file does not leave pending Python error states"""

        file = "Mod/Draft/drafttests/Issue24314.dxf"
        in_file = os.path.join(App.getHomePath(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        hGrp = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

        # Set options, doing our best to restore them:
        wasShowDialog = hGrp.GetBool("dxfShowDialog", True)
        wasUseLegacyImporter = hGrp.GetBool("dxfUseLegacyImporter", False)
        wasUseLayers = hGrp.GetBool("dxfUseDraftVisGroups", True)
        wasImportMode = hGrp.GetInt("DxfImportMode", 2)
        wasCreateSketch = hGrp.GetBool("dxfCreateSketch", False)
        wasImportAnonymousBlocks = hGrp.GetBool("dxfstarblocks", False)

        doc = None
        try:
            # disable Preferences dialog in gui mode (avoids popup prompt to user)
            hGrp.SetBool("dxfShowDialog", False)
            # Use the new C++ importer -- that's where the bug was
            hGrp.SetBool("dxfUseLegacyImporter", False)
            # Preserve the DXF layers (makes the checking of document contents easier)
            hGrp.SetBool("dxfUseDraftVisGroups", True)
            # create simple part shapes (2 params)
            # This is required to display the bug because creation of Draft objects clears out the
            # pending exception this test is looking for, whereas creation of the simple shape object
            # actually throws on the pending exception so the entity is absent from the document.
            hGrp.SetInt("DxfImportMode", 2)
            hGrp.SetBool("dxfCreateSketch", False)
            hGrp.SetBool("dxfstarblocks", False)
            doc = importDXF.open(in_file)
            # This doc should have 3 objects: The Layers container, the DXF layer called 0, and one Line
            self.assertEqual(len(doc.Objects), 3)
        finally:
            hGrp.SetBool("dxfShowDialog", wasShowDialog)
            hGrp.SetBool("dxfUseLegacyImporter", wasUseLegacyImporter)
            hGrp.SetBool("dxfUseDraftVisGroups", wasUseLayers)
            hGrp.SetInt("DxfImportMode", wasImportMode)
            hGrp.SetBool("dxfCreateSketch", wasCreateSketch)
            hGrp.SetBool("dxfstarblocks", wasImportAnonymousBlocks)
            if doc:
                App.closeDocument(doc.Name)

    def make_export_objects(self):
        """Create objects that export without a GUI, plus one the exporter skips."""
        line = Draft.make_line(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        circle = Draft.make_circle(5)
        rectangle = Draft.make_rectangle(4, 3)
        angular_dimension = Draft.make_angular_dimension(
            App.Vector(0, 0, 0), [0, 90], App.Vector(3, 3, 0)
        )
        self.doc.recompute()
        return [line, circle, rectangle, angular_dimension]

    def check_dxf_structure(self, path):
        """Check the properties of a DXF file that AutoCAD-family readers depend on."""
        with open(path, "rb") as f:
            self.assertTrue(f.read().endswith(b"EOF\n"), "no newline after EOF")

        pairs = aux.dxf_group_pairs(path)

        table_names = []
        i = 0
        while i < len(pairs):
            if pairs[i] == ("0", "TABLE"):
                name = pairs[i + 1][1]
                declared = None
                records = []
                i += 2
                while pairs[i] != ("0", "ENDTAB"):
                    code, value = pairs[i]
                    if code == "70" and declared is None:
                        declared = int(value)
                    elif pairs[i] == ("0", name):
                        records.append(None)
                    elif code == "2" and records and records[-1] is None:
                        records[-1] = value
                    i += 1
                table_names.append(name)
                self.assertEqual(
                    declared,
                    len(records),
                    f"{name} table declares {declared} entries but has {len(records)}",
                )
                self.assertEqual(
                    len(records), len(set(records)), f"{name} table has duplicate names: {records}"
                )
            i += 1
        self.assertEqual(table_names.count("DIMSTYLE"), 1, "expected exactly one DIMSTYLE table")

        blocks = [
            pairs[k + 1][1]
            for k, pair in enumerate(pairs[:-1])
            if pair == ("100", "AcDbBlockBegin")
        ]
        self.assertEqual(len(blocks), len(set(blocks)), f"duplicate BLOCK names: {blocks}")

        handles = []
        in_header = False
        for code, value in pairs:
            if (code, value) == ("2", "HEADER"):
                in_header = True
            elif (code, value) == ("0", "ENDSEC"):
                in_header = False
            elif not in_header and code in ("5", "105") and value.strip():
                handles.append(value.strip())
        duplicate_handles = [handle for handle, n in Counter(handles).items() if n > 1]
        self.assertFalse(duplicate_handles, f"duplicate handles: {duplicate_handles}")

    def test_export_dxf(self):
        """Export figures to a DXF file and check that the file is structurally valid."""
        out_dir = tempfile.mkdtemp()
        try:
            path = os.path.join(out_dir, "out_test.dxf")
            aux.export_dxf(self.make_export_objects(), path)
            self.assertTrue(os.path.exists(path), "no DXF file was written")
            self.check_dxf_structure(path)
        finally:
            shutil.rmtree(out_dir, ignore_errors=True)

    def test_export_dxf_stats(self):
        """The exporter reports what it wrote and what it skipped."""
        import Import

        objects = self.make_export_objects()
        out_dir = tempfile.mkdtemp()
        try:
            importDXF.readPreferences()
            stats = Import.exportDxf(
                obj=objects,
                name=os.path.join(out_dir, "out_test.dxf"),
                version=14,
                helpers=importDXF,
            )
        finally:
            shutil.rmtree(out_dir, ignore_errors=True)

        self.assertEqual(stats["totalObjectsProcessed"], len(objects))
        self.assertEqual(stats["layerCount"], 1)
        self.assertEqual(stats["entityCounts"]["CIRCLE"], 1)
        self.assertEqual(stats["entityCounts"]["LINE"], 5)
        self.assertEqual(list(stats["skippedObjects"]), ["AngularDimension"])
        self.assertEqual(len(stats["skippedObjects"]["AngularDimension"]), 1)

    def test_export_dxf_text_encoding(self):
        """Text is written as Windows-1252 bytes, with escapes for other characters."""
        text = Draft.make_text(["É à ß € … 日本"], App.Vector(0, 0, 0))
        self.doc.recompute()
        out_dir = tempfile.mkdtemp()
        try:
            path = os.path.join(out_dir, "out_test.dxf")
            aux.export_dxf([text], path)
            pairs = aux.dxf_group_pairs(path)
        finally:
            shutil.rmtree(out_dir, ignore_errors=True)

        index = pairs.index(("0", "TEXT"))
        value = next(value for code, value in pairs[index:] if code == "1")
        self.assertEqual(value.encode("latin-1"), b"\xc9 \xe0 \xdf \x80 \x85 \\U+65E5\\U+672C")


## @}
