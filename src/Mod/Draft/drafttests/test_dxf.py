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
import tempfile
import unittest

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

    @unittest.skipUnless(App.GuiUp, "requires Draft Text view provider")
    def test_cpp_import_text_uses_dxf_height_not_draft_preference(self):
        """Verify C++ DXF text import ignores the active Draft text size preference."""

        dxf_content = (
            "0\nSECTION\n2\nENTITIES\n"
            "0\nTEXT\n8\n0\n10\n0\n20\n0\n30\n0\n40\n1.0\n1\nSample\n"
            "0\nENDSEC\n0\nEOF\n"
        )

        hGrp = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

        wasShowDialog = hGrp.GetBool("dxfShowDialog", True)
        wasUseLegacyImporter = hGrp.GetBool("dxfUseLegacyImporter", False)
        wasImportTexts = hGrp.GetBool("dxftext", False)
        wasUseLayers = hGrp.GetBool("dxfUseDraftVisGroups", True)
        wasImportMode = hGrp.GetInt("DxfImportMode", 2)
        wasTextHeight = hGrp.GetFloat("textheight", 1.0)

        doc = None
        with tempfile.NamedTemporaryFile("w", suffix=".dxf", delete=False) as dxf_file:
            dxf_file.write(dxf_content)
            dxf_path = dxf_file.name

        try:
            hGrp.SetBool("dxfShowDialog", False)
            hGrp.SetBool("dxfUseLegacyImporter", False)
            hGrp.SetBool("dxftext", True)
            hGrp.SetBool("dxfUseDraftVisGroups", True)
            hGrp.SetInt("DxfImportMode", 2)
            hGrp.SetFloat("textheight", 10.0)

            doc = importDXF.open(dxf_path)
            text_objects = [
                obj
                for obj in doc.Objects
                if hasattr(obj, "Proxy") and obj.Proxy.__class__.__name__ == "Text"
            ]

            self.assertEqual(len(text_objects), 1)
            self.assertAlmostEqual(text_objects[0].ViewObject.FontSize, importDXF.TEXTSCALING)
        finally:
            hGrp.SetBool("dxfShowDialog", wasShowDialog)
            hGrp.SetBool("dxfUseLegacyImporter", wasUseLegacyImporter)
            hGrp.SetBool("dxftext", wasImportTexts)
            hGrp.SetBool("dxfUseDraftVisGroups", wasUseLayers)
            hGrp.SetInt("DxfImportMode", wasImportMode)
            hGrp.SetFloat("textheight", wasTextHeight)
            os.unlink(dxf_path)
            if doc:
                App.closeDocument(doc.Name)

    def test_export_dxf(self):
        """Create some figures and export them to a DXF file."""
        operation = "importDXF.export"
        _msg("  Test '{}'".format(operation))

        file = "Mod/Draft/drafttest/out_test.dxf"
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        obj = aux.fake_function(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))


## @}
