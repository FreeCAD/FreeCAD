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

    def test_export_dxf_no_dangling_mlinestyle(self):
        """Exported DXF must not reference an mlinestyle it does not define.

        Regression test for issue #31836. The C++ DXF exporter used to copy a
        ``$CMLSTYLE STANDARD`` variable into the HEADER section without ever
        emitting the matching ACAD_MLINESTYLE dictionary in the OBJECTS
        section. FreeCAD exports no MLINE entities, so that header reference is
        dangling. It makes GNU LibreDWG's dxf2dwg abort while resolving the
        dictionary, which breaks DWG export for every drawing.

        The header content comes from the plate files header14.rub and
        header12.rub, so both the R14 and R12 outputs are checked.
        """
        operation = "importDXF.export"
        _msg("  Test '{}' (issue #31836)".format(operation))

        Draft.make_wire(
            [
                App.Vector(0, 0, 0),
                App.Vector(100, 0, 0),
                App.Vector(100, 60, 0),
                App.Vector(0, 60, 0),
            ],
            closed=True,
        )
        self.doc.recompute()

        hGrp = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        was_legacy = hGrp.GetBool("dxfUseLegacyExporter", False)
        tmp_dir = tempfile.mkdtemp()
        try:
            # The dangling reference lives in the C++ exporter's header plate,
            # so make sure the legacy Python exporter is not used here.
            hGrp.SetBool("dxfUseLegacyExporter", False)

            # nospline=False writes the R14 header (header14.rub),
            # nospline=True writes the R12 header (header12.rub).
            for nospline, tag in ((False, "r14"), (True, "r12")):
                out_file = os.path.join(tmp_dir, "issue31836_{}.dxf".format(tag))
                importDXF.export(self.doc.Objects, out_file, nospline=nospline)
                self.assertTrue(
                    os.path.exists(out_file), "'{}' produced no {} file".format(operation, tag)
                )
                with open(out_file, encoding="utf-8", errors="replace") as fp:
                    content = fp.read()
                if "$CMLSTYLE" in content:
                    self.assertIn(
                        "MLINESTYLE",
                        content,
                        "{} DXF sets $CMLSTYLE but defines no ACAD_MLINESTYLE "
                        "(dangling reference, issue #31836)".format(tag),
                    )
        finally:
            hGrp.SetBool("dxfUseLegacyExporter", was_legacy)
            shutil.rmtree(tmp_dir, ignore_errors=True)


## @}
