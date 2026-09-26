# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import unittest


class PythonFeatureTest(unittest.TestCase):
    """Checks that the Python variants of the TechDraw features expose the
    concrete feature type together with the FeaturePython proxy type."""

    # TechDraw::<typeId> -> base class expected in the object's Python MRO
    PYTHON_FEATURES = {
        "TechDraw::DrawPagePython": "DrawPage",
        "TechDraw::DrawViewPython": "DrawView",
        "TechDraw::DrawViewAnnotationPython": "DrawViewAnnotation",
        "TechDraw::DrawViewPartPython": "DrawViewPart",
        "TechDraw::DrawViewSectionPython": "DrawViewPart",
        "TechDraw::DrawComplexSectionPython": "DrawViewPart",
        "TechDraw::DrawViewDetailPython": "DrawViewPart",
        "TechDraw::DrawViewMultiPython": "DrawViewPart",
        "TechDraw::DrawTemplatePython": "DrawTemplate",
        "TechDraw::DrawParametricTemplatePython": "DrawParametricTemplate",
        "TechDraw::DrawSVGTemplatePython": "DrawSVGTemplate",
        "TechDraw::DrawViewSymbolPython": "DrawViewSymbol",
        "TechDraw::DrawViewDraftPython": "DrawViewSymbol",
        "TechDraw::DrawViewSpreadsheetPython": "DrawViewSymbol",
        "TechDraw::DrawLeaderLinePython": "DrawLeaderLine",
        "TechDraw::DrawRichAnnoPython": "DrawRichAnno",
        "TechDraw::DrawTilePython": "DrawTile",
        "TechDraw::DrawTileWeldPython": "DrawTileWeld",
        "TechDraw::DrawWeldSymbolPython": "DrawWeldSymbol",
        "TechDraw::DrawBrokenViewPython": "DrawBrokenView",
        "TechDraw::DrawViewClipPython": "DrawViewClip",
        "TechDraw::DrawViewImagePython": "DrawView",
        "TechDraw::DrawHatchPython": "DrawHatch",
        "TechDraw::DrawGeomHatchPython": "DrawGeomHatch",
    }

    def setUp(self):
        FreeCAD.newDocument("TDPython")
        FreeCAD.setActiveDocument("TDPython")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDPython")

    def tearDown(self):
        FreeCAD.closeDocument("TDPython")

    @staticmethod
    def _mroNames(obj):
        return [cls.__name__ for cls in type(obj).__mro__]

    def testConcretePythonType(self):
        """Python feature objects are instances of their concrete feature type"""
        for typeId, baseName in self.PYTHON_FEATURES.items():
            with self.subTest(typeId=typeId):
                name = typeId.replace("TechDraw::", "")
                obj = FreeCAD.ActiveDocument.addObject(typeId, name)
                mro = self._mroNames(obj)
                # FeaturePythonPyT wrapper
                self.assertIn("FeaturePython", mro)
                self.assertTrue(hasattr(obj, "__fc_template__"))
                # concrete Python binding of the feature
                self.assertIn(baseName, mro)

    def testDynamicMethods(self):
        """FeaturePython objects keep dynamically attached methods"""

        def dummy(self):
            return 42

        for typeId in self.PYTHON_FEATURES:
            with self.subTest(typeId=typeId):
                name = typeId.replace("TechDraw::", "")
                obj = FreeCAD.ActiveDocument.addObject(typeId, name)
                obj.dummy = dummy
                self.assertEqual(obj.dummy(), 42)


if __name__ == "__main__":
    unittest.main()
