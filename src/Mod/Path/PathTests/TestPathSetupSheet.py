# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import PathScripts.PathSetupSheet as PathSetupSheet

from PathTests.PathTestUtils import PathTestBase

class TestPathSetupSheet(PathTestBase):
    class TestObject:
        def __init__(self, doc):
            self.Document = doc
            self.SetupSheet = None

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathSetupSheet")
        self.obj = self.TestObject(self.doc)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        '''Verify SetupSheet templateAttributes'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup()
        self.doc.recompute()

        attrs = ss.templateAttributes(True, True)

        self.assertEqual(attrs[PathSetupSheet.Default.HorizRapid], '0.00 mm/s')
        self.assertEqual(attrs[PathSetupSheet.Default.VertRapid], '0.00 mm/s')
        self.assertEqual(attrs[PathSetupSheet.Default.SafeHeight], '3.00 mm')
        self.assertEqual(attrs[PathSetupSheet.Default.ClearanceHeight], '5.00 mm')

    def test01(self):
        '''Verify SetupSheet template attributes roundtrip.'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup()
        self.doc.recompute()
        ss.DefaultVertRapid = '10 mm/s'
        ss.DefaultHorizRapid = '22 mm/s'
        ss.DefaultSafeHeight = '18 mm'
        ss.DefaultClearanceHeight = '23 mm'

        o2 = self.TestObject(self.doc)
        s2 = PathSetupSheet.SetupSheet(o2)
        s2.setup()
        self.doc.recompute()
        s2.setFromTemplate(ss.templateAttributes())
        self.doc.recompute()

        self.assertRoughly(self.obj.SetupSheet.DefaultVertRapid, o2.SetupSheet.DefaultVertRapid)
        self.assertRoughly(self.obj.SetupSheet.DefaultHorizRapid, o2.SetupSheet.DefaultHorizRapid)
        self.assertRoughly(self.obj.SetupSheet.DefaultSafeHeight, o2.SetupSheet.DefaultSafeHeight)
        self.assertRoughly(self.obj.SetupSheet.DefaultClearanceHeight, o2.SetupSheet.DefaultClearanceHeight)

    def test10(self):
        '''Verify template attributes encoding/decoding of floats.'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup('x')

        self.assertEqual(ss.expressionReference(), 'Spreadsheet')

        self.assertEqual(str(ss.encodeTemplateAttributes({'00': 13.00})), "{'00': 13.0}")
        self.assertEqual(str(ss.decodeTemplateAttributes({'00': 13.00})), "{'00': 13.0}")

    def test11(self):
        '''Verify template attributes encoding/decoding of strings.'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup('x')

        self.assertEqual(str(ss.encodeTemplateAttributes({'00': 'SetupSheet'})), "{'00': u'SetupSheet'}")
        self.assertEqual(str(ss.encodeTemplateAttributes({'00': 'Spreadsheet'})), "{'00': u'${SetupSheet}'}")
        self.assertEqual(str(ss.encodeTemplateAttributes({'00': 'Spreadsheet.y'})), "{'00': u'${SetupSheet}.y'}")
        self.assertEqual(str(ss.encodeTemplateAttributes({'00': '${SetupSheet}'})), "{'00': u'${SetupSheet}'}")

        self.assertEqual(str(ss.decodeTemplateAttributes({'00': 'SetupSheet'})), "{'00': u'SetupSheet'}")
        self.assertEqual(str(ss.decodeTemplateAttributes({'00': '${SetupSheet}'})), "{'00': u'Spreadsheet'}")
        self.assertEqual(str(ss.decodeTemplateAttributes({'00': '${SetupSheet}.y'})), "{'00': u'Spreadsheet.y'}")
        self.assertEqual(str(ss.decodeTemplateAttributes({'00': '${SetupSheet}.y - ${SetupSheet}.z'})), "{'00': u'Spreadsheet.y - Spreadsheet.z'}")

    def test12(self):
        '''Verify template attributes encoding/decoding of dictionaries.'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup('rrr')

        self.assertEqual(str(ss.encodeTemplateAttributes({'00': {'01': 'SetupSheet'}})), "{'00': {'01': u'SetupSheet'}}")
        self.assertEqual(str(ss.encodeTemplateAttributes({'00': {'01': 'Spreadsheet.y - Spreadsheet.z'}})), "{'00': {'01': u'${SetupSheet}.y - ${SetupSheet}.z'}}")

        self.assertEqual(str(ss.decodeTemplateAttributes({'00': {'01': 'SetupSheet'}})), "{'00': {'01': u'SetupSheet'}}")
        self.assertEqual(str(ss.decodeTemplateAttributes({'00': {'01': '${SetupSheet}.y - ${SetupSheet}.z'}})), "{'00': {'01': u'Spreadsheet.y - Spreadsheet.z'}}")

    def test13(self):
        '''Verify template attributes encoding/decoding of lists.'''
        ss = PathSetupSheet.SetupSheet(self.obj)
        ss.setup('hugo')

        attrs = {}
        attrs['00'] = 'x.Spreadsheet'
        attrs['01'] = [{'10': 'Spreadsheet', '11': 'Spreadsheet.y'}, {'20': 'Spreadsheet'}]
        attrs['02'] = [{'a': [{'b': 'Spreadsheet'}, {'c': 'Spreadsheet'}], 'b': [{'b': 'Spreadsheet'}]}]

        encoded = ss.encodeTemplateAttributes(attrs)
        self.assertEqual(encoded['00'], 'x.${SetupSheet}')
        self.assertEqual(len(encoded['01']), 2)
        self.assertEqual(encoded['01'][0]['10'], '${SetupSheet}')
        self.assertEqual(encoded['01'][0]['11'], '${SetupSheet}.y')
        self.assertEqual(str(encoded['01'][1]), "{'20': u'${SetupSheet}'}")
        self.assertEqual(len(encoded['02']), 1)
        self.assertEqual(len(encoded['02'][0]['a']), 2)
        self.assertEqual(str(encoded['02'][0]['a'][0]), "{'b': u'${SetupSheet}'}")
        self.assertEqual(str(encoded['02'][0]['a'][1]), "{'c': u'${SetupSheet}'}")
        self.assertEqual(len(encoded['02'][0]['b']), 1)
        self.assertEqual(str(encoded['02'][0]['b'][0]), "{'b': u'${SetupSheet}'}")

        decoded = ss.decodeTemplateAttributes(encoded)
        self.assertEqual(len(decoded), len(attrs))
        self.assertEqual(decoded['00'], attrs['00'])
        self.assertEqual(len(decoded['01']), len(attrs['01']))
        self.assertEqual(decoded['01'][0]['10'], attrs['01'][0]['10'])
        self.assertEqual(decoded['01'][0]['11'], attrs['01'][0]['11'])
        self.assertEqual(decoded['01'][1]['20'], attrs['01'][1]['20'])
        self.assertEqual(len(decoded['02']), len(attrs['02']))
        self.assertEqual(len(decoded['02'][0]['a']), len(attrs['02'][0]['a']))
        self.assertEqual(decoded['02'][0]['a'][0]['b'], attrs['02'][0]['a'][0]['b'])
        self.assertEqual(decoded['02'][0]['a'][1]['c'], attrs['02'][0]['a'][1]['c'])
        self.assertEqual(len(decoded['02'][0]['b']), len(attrs['02'][0]['b']))
        self.assertEqual(decoded['02'][0]['b'][0]['b'], attrs['02'][0]['b'][0]['b'])

        # just to be safe ...
        o2 = self.TestObject(self.doc)
        s2 = PathSetupSheet.SetupSheet(o2)
        s2.setup()
        self.doc.recompute()
        s2.setFromTemplate(ss.templateAttributes())
        o2.SetupSheet.Label = 'xxx'
        self.assertEqual(s2.expressionReference(), 'Spreadsheet001')
        dec = s2.decodeTemplateAttributes(encoded)
        # pick one
        self.assertEqual(dec['01'][0]['11'], 'Spreadsheet001.y')

