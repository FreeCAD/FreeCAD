import unittest

import FreeCAD as App


class ColorTransparencyTest(unittest.TestCase):

    def setUp(self):
        self._doc = App.newDocument()
        self._pg = App.ParamGet('User parameter:BaseApp/Preferences/View')
        self._backup_default_transparency = self._pg.GetInt('DefaultShapeTransparency')
        self._backup_default_shapecolor = self._pg.GetUnsigned('DefaultShapeColor')


    def tearDown(self):
        App.closeDocument(self._doc.Name)
        self._pg.SetInt('DefaultShapeTransparency', self._backup_default_transparency)
        self._pg.SetUnsigned('DefaultShapeColor', self._backup_default_shapecolor)


    def test_default_shape_transparency(self):
        """
        related: https://github.com/FreeCAD/FreeCAD/pull/11866
        related: https://github.com/FreeCAD/FreeCAD/pull/11586
        """
        transparency = 70
        self._pg.SetInt('DefaultShapeTransparency', transparency)
        obj = self._doc.addObject('Part::Box')
        assert obj.ViewObject.Transparency == transparency
        obj.ViewObject.ShapeAppearance[0].DiffuseColor = (0.5, 0.0, 0.0)

        self.assertEqual(obj.ViewObject.Transparency, transparency,
            'transparency was unexpectedly changed to {} when changing the color.'.format(
            obj.ViewObject.Transparency))


    def test_default_shape_color(self):
        """
        related: https://github.com/FreeCAD/FreeCAD/pull/11866
        """

        """
        This test isn't currently valid as it draws from the hard coded default material.

        The preference editor doesn't allow for setting transparencies. The default value
        of 0 corresponds to a fully transparent color, which is not desirable. It changes
        the transparency when loading to 1.0
        """

        self._pg.SetUnsigned('DefaultShapeColor', 0xff000000)  # red
        obj = self._doc.addObject('Part::Box')

        self.assertEqual(obj.ViewObject.ShapeAppearance[0].DiffuseColor, (1.0, 0.0, 0.0, 1.0),
            'default shape color was not set correctly')
        self.assertEqual(obj.ViewObject.ShapeMaterial.DiffuseColor, (1.0, 0.0, 0.0, 1.0),
            'default material color was not set correctly')


    def test_app_plane_transparency(self):
        """
        related: https://github.com/FreeCAD/FreeCAD/pull/12064
        """
        self._pg.SetInt('DefaultShapeTransparency', 70)
        obj = self._doc.addObject('App::Origin')
        t = self._doc.findObjects('App::Plane')[0].ViewObject.Transparency

        self.assertEqual(t, 0,
            'transparency of App::Plane object is {} instead of 0'.format(t))

