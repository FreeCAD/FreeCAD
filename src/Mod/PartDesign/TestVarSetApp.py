import unittest
import tempfile
import os

import FreeCAD as App

NAME_DOC_BASE = "TestVarSet"
NAME_DOC_VARIANT = "TestVarSetVariant"


def basicSetUp(obj):
    obj.Doc = App.newDocument(NAME_DOC_BASE)

    # create a part with a body
    obj.Part = obj.Doc.addObject("App::Part", "Part")
    obj.Box = obj.Doc.addObject("Part::Box", "Box")
    obj.Part.addObject(obj.Box)

    # create a variable set Params within the part
    obj.Params = obj.Doc.addObject("App::VarSet", "Params")
    obj.Params.addProperty("App::PropertyFloat", "Length", "Params")
    obj.Params.addProperty("App::PropertyFloat", "Width", "Params")
    obj.Params.addProperty("App::PropertyFloat", "Height", "Params")
    obj.Params.Length = 300
    obj.Params.Width = 200
    obj.Params.Height = 100
    obj.Part.addObject(obj.Params)

    obj.Box.setExpression('Length', '<<Params>>.Length')
    obj.Box.setExpression('Width', '<<Params>>.Width')
    obj.Box.setExpression('Height', '<<Params>>.Height')

    obj.Doc.recompute()


def createArgVarSet(doc, varSet):
    """Create a VarSet to be used as an argument in a variant

    @param doc the document in which the VarSet has to be created
    @param varSet the varSet that acts as equivalent one

    @return a copy of varSet (that is naturally equivalent) to be used as
    argument to a variant.
    """
    argVarSet = doc.copyObject(varSet)
    argVarSet.Length = 350
    argVarSet.Width = 250
    argVarSet.Height = 150

    return argVarSet


class TestVarSet(unittest.TestCase):
    def setUp(self):
        basicSetUp(self)

    # Tests whether changing parameters of a non-exposed varset affect the geometry
    def testParamsNotExposed(self):
        self.Params.Length = 400
        self.Doc.recompute()
        self.assertEqual(self.Params.Length, self.Box.Length)

    def getExpressionMap(self, Obj):
        return {k: v for (k, v) in Obj.ExpressionEngine}

    # Tests whether the expressions have changed on expose
    def testExpressionChangeExposed(self):
        self.Params.Exposed = True
        self.Doc.recompute()

        expressionMap = self.getExpressionMap(self.Box)

        self.assertEqual(expressionMap["Length"], "hiddenref(<<Part>>.Params.Length)")
        self.assertEqual(expressionMap["Width"], "hiddenref(<<Part>>.Params.Width)")
        self.assertEqual(expressionMap["Height"], "hiddenref(<<Part>>.Params.Height)")

    # Tests whether the expressions have changed back on not exposing
    def testExpressionChangeNonExposed(self):
        self.Params.Exposed = True
        self.Doc.recompute()

        self.Params.Exposed = False
        self.Doc.recompute()

        expressionMap = self.getExpressionMap(self.Box)

        self.assertEqual(expressionMap["Length"], "<<Params>>.Length")
        self.assertEqual(expressionMap["Width"], "<<Params>>.Width")
        self.assertEqual(expressionMap["Height"], "<<Params>>.Height")

    # Tests whether changing parameters of an exposed varset affect the geometry
    def testParamsExposed(self):
        self.Params.Exposed = True
        self.Doc.recompute()

        self.Params.Length = 400
        self.Doc.recompute()
        self.assertEqual(self.Params.Length, self.Box.Length)
        self.assertEqual(self.Part.Params, self.Params)

    # Tests whether copying a non-exposed variable set succeeds
    def testVarSetCopyNonExposed(self):
        raisedException = False

        try:
            varSet = self.Doc.copyObject(self.Params)
        except Exception:
            raisedException = True

        self.assertFalse(raisedException)
        self.assertNotEqual(varSet.Name, self.Params.Name)

    # Tests whether copying an exposed variable set succeeds
    def testVarSetCopyExposed(self):
        raisedException = False

        try:
            self.Params.Exposed = True
            varSet = self.Doc.copyObject(self.Params)
        except Exception:
            raisedException = True

        self.assertFalse(raisedException)
        self.assertNotEqual(varSet.Name, self.Params.Name)
        # the copied VarSet should not be exposed
        self.assertFalse(varSet.Exposed)

    def testExposedReplaced(self):
        argVarSet = createArgVarSet(self.Doc, self.Params)

        self.Params.Exposed = True

        self.Doc.recompute()

        self.Part.Params = argVarSet

        self.Doc.recompute()

        self.assertEqual(argVarSet.Length, self.Box.Length)
        self.assertEqual(argVarSet.Width, self.Box.Width)
        self.assertEqual(argVarSet.Height, self.Box.Height)

    def testExposedReplacedNoEffectInternal(self):
        argVarSet = createArgVarSet(self.Doc, self.Params)

        self.Params.Exposed = True

        self.Doc.recompute()

        self.Part.Params = argVarSet

        self.Doc.recompute()

        # this should have no effect on the length of the box
        self.Params.Length = 400

        self.assertEqual(argVarSet.Length, self.Box.Length)

    def tearDown(self):
        App.closeDocument(NAME_DOC_BASE)


class TestVarSetBinder(unittest.TestCase):
    def setUp(self):
        basicSetUp(self)
        self.Params.Exposed = True

        # save the base document as we will link to it
        tempDir = tempfile.gettempdir()
        tempPathDoc = os.path.join(tempDir, f"{NAME_DOC_BASE}.FCStd")
        self.Doc.saveAs(tempPathDoc)

        # the same is necessary for the binder document
        self.DocBinder = App.newDocument(NAME_DOC_VARIANT)
        tempPathDocBinder = os.path.join(tempDir, f"{NAME_DOC_VARIANT}.FCStd")
        self.DocBinder.saveAs(tempPathDocBinder)

        self.Binder = self.DocBinder.addObject("PartDesign::SubShapeBinder", "Binder")
        self.Binder.Support = self.Part
        self.DocBinder.recompute()

    # Tests whether changing parameters of a non-exposed varset affect the geometry
    def testBinderLinked(self):
        self.assertFalse(hasattr(self.Binder, "Params"))
        self.assertEqual(self.Params.Length, self.Binder.Shape.BoundBox.XLength)
        self.assertEqual(self.Params.Width, self.Binder.Shape.BoundBox.YLength)
        self.assertEqual(self.Params.Height, self.Binder.Shape.BoundBox.ZLength)

    def testBinderCopyOnChange(self):
        self.Binder.BindCopyOnChange = "Enabled"
        self.DocBinder.recompute()

        self.assertTrue(hasattr(self.Binder, "Params"))
        self.assertEqual(self.Binder.Params, self.Params)

        argVarSet = createArgVarSet(self.DocBinder, self.Params)

        self.DocBinder.recompute()

        self.Binder.Params = argVarSet

        self.DocBinder.recompute()

        self.assertEqual(argVarSet.Length, self.Binder.Shape.BoundBox.XLength)
        self.assertEqual(argVarSet.Width, self.Binder.Shape.BoundBox.YLength)
        self.assertEqual(argVarSet.Height, self.Binder.Shape.BoundBox.ZLength)

    def testBinderCopyOnChangeDependencies(self):
        self.Binder.BindCopyOnChange = "Enabled"
        self.DocBinder.recompute()

        argVarSet = createArgVarSet(self.DocBinder, self.Params)

        self.DocBinder.recompute()

        self.Binder.Params = argVarSet

        self.DocBinder.recompute()

        self.assertEqual(argVarSet.Length, self.Binder.Shape.BoundBox.XLength)
        self.assertEqual(argVarSet.Width, self.Binder.Shape.BoundBox.YLength)
        self.assertEqual(argVarSet.Height, self.Binder.Shape.BoundBox.ZLength)

        argVarSet.Length += 100

        self.DocBinder.recompute()

        # this means that even though we make use of hidden references,
        # dependencies are still tracked.
        self.assertEqual(argVarSet.Length, self.Binder.Shape.BoundBox.XLength)

    def tearDown(self):
        App.closeDocument(NAME_DOC_BASE)
        App.closeDocument(NAME_DOC_VARIANT)
