import os
import tempfile
import unittest
import xml.etree.ElementTree as ElementTree
import zipfile

import FreeCAD

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewScaleRestoreTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDScaleRestore")
        FreeCAD.setActiveDocument("TDScaleRestore")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDScaleRestore")
        self.doc = FreeCAD.ActiveDocument
        self.page = createPageWithSVGTemplate(self.doc)
        self.page.Scale = 0.05

    def tearDown(self):
        for name in list(FreeCAD.listDocuments()):
            if name.startswith("TDScaleRestore"):
                FreeCAD.closeDocument(name)

    def rewrite_view_scale(self, file_path, view_name, scale):
        with zipfile.ZipFile(file_path, "r") as archive:
            entries = [(info, archive.read(info.filename)) for info in archive.infolist()]

        with zipfile.ZipFile(file_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for info, data in entries:
                if info.filename == "Document.xml":
                    root = ElementTree.fromstring(data)
                    for obj in root.findall(".//Object"):
                        if obj.get("name") != view_name:
                            continue
                        for prop in obj.findall("./Properties/Property"):
                            if prop.get("name") == "Scale":
                                prop.find("Float").set("value", f"{scale:.16f}")
                    data = ElementTree.tostring(root, encoding="utf-8", xml_declaration=True)
                archive.writestr(info, data)

    def assert_page_scale_survives_restore(self, type_id, view_name):
        view = self.doc.addObject(type_id, view_name)
        self.page.addView(view)
        view.ScaleType = "Page"
        self.assertAlmostEqual(view.Scale, self.page.Scale)

        temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".FCStd")
        temp_file.close()
        reopened = None
        try:
            self.doc.saveAs(temp_file.name)
            self.rewrite_view_scale(temp_file.name, view_name, 1.0)

            reopened = FreeCAD.openDocument(temp_file.name)
            restored_view = reopened.getObject(view_name)
            restored_page = reopened.getObject("Page")

            self.assertEqual(restored_view.ScaleType, "Page")
            self.assertAlmostEqual(restored_view.Scale, restored_page.Scale)
        finally:
            if reopened is not None:
                FreeCAD.closeDocument(reopened.Name)
            os.unlink(temp_file.name)

    def test_draft_view_page_scale_survives_restore_with_stale_saved_scale(self):
        self.assert_page_scale_survives_restore("TechDraw::DrawViewDraft", "DraftView")

    def test_arch_view_page_scale_survives_restore_with_stale_saved_scale(self):
        self.assert_page_scale_survives_restore("TechDraw::DrawViewArch", "BIMView")


if __name__ == "__main__":
    unittest.main()
