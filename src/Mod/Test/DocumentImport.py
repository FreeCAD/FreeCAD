# ***************************************************************************
# *   Copyright (c) 2003 Juergen Riegel <juergen.riegel@web.de>             *
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
# ***************************************************************************/

import FreeCAD, unittest

# ---------------------------------------------------------------------------
# define the functions to test the FreeCAD Document import code
# ---------------------------------------------------------------------------


class DocumentImportCases(unittest.TestCase):
    def testDXFImportCPPIssue20195(self):
        import importDXF
        from draftutils import params

        # Set options, doing our best to restore them:
        wasShowDialog = params.get_param("dxfShowDialog")
        wasUseLayers = params.get_param("dxfUseDraftVisGroups")
        wasUseLegacyImporter = params.get_param("dxfUseLegacyImporter")
        wasCreatePart = params.get_param("dxfCreatePart")
        wasCreateDraft = params.get_param("dxfCreateDraft")
        wasCreateSketch = params.get_param("dxfCreateSketch")

        try:
            # disable Preferences dialog in gui mode (avoids popup prompt to user)
            params.set_param("dxfShowDialog", False)
            # Preserve the DXF layers (makes the checking of document contents easier)
            params.set_param("dxfUseDraftVisGroups", True)
            # Use the new C++ importer -- that's where the bug was
            params.set_param("dxfUseLegacyImporter", False)
            # create simple part shapes (3 params)
            # This is required to display the bug because creation of Draft objects clears out the
            # pending exception this test is looking for, whereas creation of the simple shape object
            # actually throws on the pending exception so the entity is absent from the document.
            params.set_param("dxfCreatePart", True)
            params.set_param("dxfCreateDraft", False)
            params.set_param("dxfCreateSketch", False)
            importDXF.insert(
                FreeCAD.getHomePath() + "Mod/Test/TestData/DXFSample.dxf", "ImportedDocName"
            )
        finally:
            params.set_param("dxfShowDialog", wasShowDialog)
            params.set_param("dxfUseDraftVisGroups", wasUseLayers)
            params.set_param("dxfUseLegacyImporter", wasUseLegacyImporter)
            params.set_param("dxfCreatePart", wasCreatePart)
            params.set_param("dxfCreateDraft", wasCreateDraft)
            params.set_param("dxfCreateSketch", wasCreateSketch)
        doc = FreeCAD.getDocument("ImportedDocName")
        # This doc should have 3 objects: The Layers container, the DXF layer called 0, and one Line
        self.assertEqual(len(doc.Objects), 3)
        FreeCAD.closeDocument("ImportedDocName")
