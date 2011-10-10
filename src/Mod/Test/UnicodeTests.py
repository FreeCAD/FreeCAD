# -*- coding: utf-8 -*-
#   (c) Juergen Riegel (juergen.riegel@web.de) 2007 LGPL   

# Open and edit only in UTF-8 !!!!!!

import FreeCAD, os, unittest, tempfile


#---------------------------------------------------------------------------
# define the functions to test the FreeCAD Document code
#---------------------------------------------------------------------------


class UnicodeBasicCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("CreateTest")

  def testUnicodeLabel(self):
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    L1.Label = u"हिन्दी"
    self.failUnless(L1.Label == u"हिन्दी")

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("CreateTest")

class DocumentSaveRestoreCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("SaveRestoreTests")
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    L1.Label = u"हिन्दी"
    self.TempPath = tempfile.gettempdir()
    FreeCAD.Console.PrintLog( '  Using temp path: ' + self.TempPath + '\n')
    
  def testSaveAndRestore(self):
    # saving and restoring
    SaveName = self.TempPath + os.sep + "UnicodeTest.FCStd"
    self.Doc.FileName = SaveName
    self.Doc.save()
    self.Doc.FileName = ""
    self.Doc = FreeCAD.open(SaveName)
    self.failUnless(self.Doc.Label_1.Label == u"हिन्दी")
    FreeCAD.closeDocument("UnicodeTest")
    
  
  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("SaveRestoreTests")

      
