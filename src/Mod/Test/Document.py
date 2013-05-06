#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2003                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2003                                                   *
#***************************************************************************/

import FreeCAD, os, unittest, tempfile


#---------------------------------------------------------------------------
# define the functions to test the FreeCAD Document code
#---------------------------------------------------------------------------


class DocumentBasicCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("CreateTest")

  def testCreateDestroy(self):
    #FIXME: Causes somehow a ref count error but it's _not_ FreeCAD.getDocument()!!!
    #If we remove the whole method no error appears.
    self.failUnless(FreeCAD.getDocument("CreateTest")!= None,"Creating Document failed")

  def testAddition(self):
    # Cannot write a real test case for that but when debugging the
    # C-code there shouldn't be a memory leak (see rev. 1814)
    self.Doc.openTransaction("Add")
    L1 = self.Doc.addObject("App::FeatureTest","Label")
    self.Doc.commitTransaction()
    self.Doc.undo()

  def testAddRemoveUndo(self):
    # Bug #0000525
    self.Doc.openTransaction("Add")
    obj=self.Doc.addObject("App::FeatureTest","Label")
    self.Doc.commitTransaction()
    self.Doc.removeObject(obj.Name)
    self.Doc.undo()
    self.Doc.undo()

  def testRemoval(self):
    # Cannot write a real test case for that but when debugging the
    # C-code there shouldn't be a memory leak (see rev. 1814)
    self.Doc.openTransaction("Add")
    L1 = self.Doc.addObject("App::FeatureTest","Label")
    self.Doc.commitTransaction()
    self.Doc.openTransaction("Rem")
    L1 = self.Doc.removeObject("Label")
    self.Doc.commitTransaction()

  def testObjects(self):
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    #call members to check for errors in ref counting
    self.Doc.ActiveObject
    self.Doc.Objects
    self.Doc.UndoMode
    self.Doc.UndoRedoMemSize
    self.Doc.UndoCount
    # test read only mechanismus
    try:
      self.Doc.UndoCount = 3
    except:
      FreeCAD.Console.PrintLog("   exception thrown, OK\n")
    else:
      self.fail("no exeption thrown")
    self.Doc.RedoCount
    self.Doc.UndoNames
    self.Doc.RedoNames
    self.Doc.recompute()
    self.failUnless(L1.Integer == 4711)
    self.failUnless(L1.Float-47.11<0.001)
    self.failUnless(L1.Bool    == True)
    self.failUnless(L1.String  == "4711")
    #temporarily not checked because of strange behavior of boost::fielesystem JR
    #self.failUnless(L1.Path  == "c:/temp")
    self.failUnless(L1.Angle-3.0<0.001)
    self.failUnless(L1.Distance-47.11<0.001)

    # test basic property stuff
    self.failUnless(not L1.getDocumentationOfProperty("Source1") == "")
    self.failUnless(L1.getGroupOfProperty("Source1") == "Feature Test")
    self.failUnless(L1.getTypeOfProperty("Source1") == [])


    # test the constraint types ( both are constraint to percent range)
    self.failUnless(L1.ConstraintInt == 5)
    self.failUnless(L1.ConstraintFloat-5.0<0.001)
    L1.ConstraintInt = 500
    L1.ConstraintFloat = 500.0
    self.failUnless(L1.ConstraintInt == 100)
    self.failUnless(L1.ConstraintFloat - 100.0 < 0.001)
    L1.ConstraintInt = -500
    L1.ConstraintFloat = -500.0
    self.failUnless(L1.ConstraintInt == 0)
    self.failUnless(L1.ConstraintFloat - 0.0 < 0.001)

    # test enum property
    self.failUnless(L1.Enum  == "Four")
    L1.Enum = "One"
    L1.Enum = 2
    self.failUnless(L1.Enum  == "Two",     "Different value to 'Two'")
    try:
      L1.Enum = "SurlyNotInThere!"
    except:
      FreeCAD.Console.PrintLog("   exception thrown, OK\n")
    else:
      self.fail("no exeption thrown")

    #self.failUnless(L1.IntegerList  == [4711]   )
    #f = L1.FloatList
    #self.failUnless(f -47.11<0.001    )
    #self.failUnless(L1.Matrix  == [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0,13.0,14.0,15.0,16.0] )
    #self.failUnless(L1.Vector  == [1.0,2.0,3.0])

    self.failUnless(L1.Label== "Label_1","Invalid object name")
    L1.Label="Label_2"
    self.Doc.recompute()
    self.failUnless(L1.Label== "Label_2","Invalid object name")
    self.Doc.removeObject("Label_1")

  def testMem(self):
    self.Doc.MemSize

  def testAddRemove(self):
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    # must delete object
    self.Doc.removeObject(L1.Name)
    try:
      L1.Name
    except:
      self.failUnless(True)
    else:
      self.failUnless(False)
    del L1

    # What do we expect here?
    self.Doc.openTransaction("AddRemove")
    L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    self.Doc.removeObject(L2.Name)
    self.Doc.commitTransaction()
    self.Doc.undo()
    try:
      L2.Name
    except:
      self.failUnless(True)
    else:
      self.failUnless(False)
    del L2

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("CreateTest")

class DocumentSaveRestoreCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("SaveRestoreTests")
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    self.TempPath = tempfile.gettempdir()
    FreeCAD.Console.PrintLog( '  Using temp path: ' + self.TempPath + '\n')

  def testSaveAndRestore(self):
    # saving and restoring
    SaveName = self.TempPath + os.sep + "SaveRestoreTests.FCStd"
    self.failUnless(self.Doc.Label_1.TypeTransient == 4711)
    self.Doc.Label_1.TypeTransient = 4712
    # setup Linking
    self.Doc.Label_1.Link = self.Doc.Label_2
    self.Doc.Label_2.Link = self.Doc.Label_1
    self.Doc.Label_1.LinkSub = (self.Doc.Label_2,["Sub1","Sub2"])
    self.Doc.Label_2.LinkSub = (self.Doc.Label_1,["Sub3","Sub4"])
    # save the document
    self.Doc.saveAs(SaveName)
    FreeCAD.closeDocument("SaveRestoreTests")
    self.Doc = FreeCAD.open(SaveName)
    self.failUnless(self.Doc.Label_1.Integer == 4711)
    self.failUnless(self.Doc.Label_2.Integer == 4711)
    # test Linkage
    self.failUnless(self.Doc.Label_1.Link == self.Doc.Label_2)
    self.failUnless(self.Doc.Label_2.Link == self.Doc.Label_1)
    self.failUnless(self.Doc.Label_1.LinkSub == (self.Doc.Label_2,["Sub1","Sub2"]))
    self.failUnless(self.Doc.Label_2.LinkSub == (self.Doc.Label_1,["Sub3","Sub4"]))
    # do  NOT save transient properties
    self.failUnless(self.Doc.Label_1.TypeTransient == 4711)
    self.failUnless(self.Doc == FreeCAD.getDocument(self.Doc.Name))

  def testRestore(self):
    Doc = FreeCAD.newDocument("RestoreTests")
    Doc.addObject("App::FeatureTest","Label_1")
    # saving and restoring
    FileName = self.TempPath + os.sep + "Test2.FCStd"
    Doc.saveAs(FileName)
    # restore must first clear the current content
    Doc.restore()
    self.failUnless(len(Doc.Objects) == 1)
    FreeCAD.closeDocument("RestoreTests")

  def testActiveDocument(self):
    # open 2nd doc
    Second = FreeCAD.newDocument("Active")
    FreeCAD.closeDocument("Active")
    try:
        # There might be no active document anymore
        # This also checks for dangling pointers
        Active = FreeCAD.activeDocument()
        # Second is still a valid object
        self.failUnless(Second != Active)
    except:
        # Okay, no document open
        self.failUnless(True)

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("SaveRestoreTests")

class DocumentRecomputeCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("RecomputeTests")
    self.L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    self.L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    self.L3 = self.Doc.addObject("App::FeatureTest","Label_3")

  def testDescent(self):
    # testing the up and downstream stuff
    FreeCAD.Console.PrintLog("def testDescent(self):Testcase not implemented\n")
    self.L1.Link = self.L2
    self.L2.Link = self.L3


  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("RecomputeTests")

class UndoRedoCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("UndoTest")
    self.Doc.UndoMode = 0
    self.Doc.addObject("App::FeatureTest","Base")
    self.Doc.addObject("App::FeatureTest","Del")
    self.Doc.getObject("Del").Integer  = 2

  def testUndoProperties(self):
    # switch on the Undo
    self.Doc.UndoMode = 1

    # first transaction
    self.Doc.openTransaction("Transaction1")
    self.Doc.addObject("App::FeatureTest","test1")
    self.Doc.getObject("test1").Integer  = 1
    self.Doc.getObject("test1").String   = "test1"
    self.Doc.getObject("test1").Float    = 1.0
    self.Doc.getObject("test1").Bool  = 1

    #self.Doc.getObject("test1").IntegerList  = 1
    #self.Doc.getObject("test1").FloatList  = 1.0

    #self.Doc.getObject("test1").Matrix  = (1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0)
    #self.Doc.getObject("test1").Vector  = (1.0,1.0,1.0)

    # second transaction
    self.Doc.openTransaction("Transaction2")
    self.Doc.getObject("test1").Integer  = 2
    self.Doc.getObject("test1").String   = "test2"
    self.Doc.getObject("test1").Float    = 2.0
    self.Doc.getObject("test1").Bool  = 0

    # switch on the Undo OFF
    self.Doc.UndoMode = 0

  def testUndoClear(self):
    # switch on the Undo
    self.Doc.UndoMode = 1
    self.assertEqual(self.Doc.UndoNames,[])
    self.assertEqual(self.Doc.UndoCount,0)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    self.Doc.openTransaction("Transaction1")
    # becomes the active object
    self.Doc.addObject("App::FeatureTest","test1")
    self.Doc.commitTransaction()
    # removes the active object
    self.Doc.undo()
    self.assertEqual(self.Doc.ActiveObject,None)
    # deletes the active object
    self.Doc.clearUndos()
    self.assertEqual(self.Doc.ActiveObject,None)

  def testUndo(self):
    # switch on the Undo
    self.Doc.UndoMode = 1
    self.assertEqual(self.Doc.UndoNames,[])
    self.assertEqual(self.Doc.UndoCount,0)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # first transaction
    self.Doc.openTransaction("Transaction1")
    self.Doc.addObject("App::FeatureTest","test1")
    self.Doc.getObject("test1").Integer  = 1
    self.Doc.getObject("Del").Integer  = 1
    self.Doc.removeObject("Del")
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # second transaction
    self.Doc.openTransaction("Transaction2")
    self.assertEqual(self.Doc.UndoNames,['Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.Doc.getObject("test1").Integer  = 2
    self.assertEqual(self.Doc.UndoNames,['Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # abort second transaction
    self.Doc.abortTransaction()
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.assertEqual(self.Doc.getObject("test1").Integer, 1)

    # again second transaction
    self.Doc.openTransaction("Transaction2")
    self.Doc.getObject("test1").Integer  = 2
    self.assertEqual(self.Doc.UndoNames,['Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # third transaction
    self.Doc.openTransaction("Transaction3")
    self.assertEqual(self.Doc.UndoNames,['Transaction3','Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,3)
    self.Doc.getObject("test1").Integer  = 3
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # fourth transaction
    self.Doc.openTransaction("Transaction4")
    self.assertEqual(self.Doc.UndoNames,['Transaction4','Transaction3','Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,4)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.Doc.getObject("test1").Integer  = 4

    # undo the fourth transaction
    self.Doc.undo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 3)
    self.assertEqual(self.Doc.UndoNames,['Transaction3','Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,3)
    self.assertEqual(self.Doc.RedoNames,['Transaction4'])
    self.assertEqual(self.Doc.RedoCount,1)

    # undo the third transaction
    self.Doc.undo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 2)
    self.assertEqual(self.Doc.UndoNames,['Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,['Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,2)

    # undo the second transaction
    self.Doc.undo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 1)
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,['Transaction2','Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,3)

    # undo the first transaction
    self.Doc.undo()
    self.failUnless(self.Doc.getObject("test1") == None)
    self.failUnless(self.Doc.getObject("Del").Integer == 2)
    self.assertEqual(self.Doc.UndoNames,[])
    self.assertEqual(self.Doc.UndoCount,0)
    self.assertEqual(self.Doc.RedoNames,['Transaction1','Transaction2','Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,4)

    # redo the first transaction
    self.Doc.redo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 1)
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,['Transaction2','Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,3)

    # redo the second transaction
    self.Doc.redo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 2)
    self.assertEqual(self.Doc.UndoNames,['Transaction2','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,['Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,2)

    # undo the second transaction
    self.Doc.undo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 1)
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,['Transaction2','Transaction3','Transaction4'])
    self.assertEqual(self.Doc.RedoCount,3)

    # new transaction eight
    self.Doc.openTransaction("Transaction8")
    self.Doc.getObject("test1").Integer  = 8
    self.assertEqual(self.Doc.UndoNames,['Transaction8','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.Doc.abortTransaction()
    self.assertEqual(self.Doc.UndoNames,['Transaction1'])
    self.assertEqual(self.Doc.UndoCount,1)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # again new transaction eight
    self.Doc.openTransaction("Transaction8")
    self.Doc.getObject("test1").Integer  = 8
    self.assertEqual(self.Doc.UndoNames,['Transaction8','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

    # again new transaction nine
    self.Doc.openTransaction("Transaction9")
    self.Doc.getObject("test1").Integer  = 9
    self.assertEqual(self.Doc.UndoNames,['Transaction9','Transaction8','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,3)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.Doc.commitTransaction()
    self.assertEqual(self.Doc.UndoNames,['Transaction9','Transaction8','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,3)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)
    self.assertEqual(self.Doc.getObject("test1").Integer, 9)

    # undo the ninth transaction
    self.Doc.undo()
    self.assertEqual(self.Doc.getObject("test1").Integer, 8)
    self.assertEqual(self.Doc.UndoNames,['Transaction8','Transaction1'])
    self.assertEqual(self.Doc.UndoCount,2)
    self.assertEqual(self.Doc.RedoNames,['Transaction9'])
    self.assertEqual(self.Doc.RedoCount,1)

    # switch on the Undo OFF
    self.Doc.UndoMode = 0
    self.assertEqual(self.Doc.UndoNames,[])
    self.assertEqual(self.Doc.UndoCount,0)
    self.assertEqual(self.Doc.RedoNames,[])
    self.assertEqual(self.Doc.RedoCount,0)

  def testGroup(self):
    # Add an object to the group
    L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    G1 = self.Doc.addObject("App::DocumentObjectGroup","Group")
    G1.addObject(L2)
    self.failUnless(G1.hasObject(L2))

    # Adding the group to itself must fail
    try:
      G1.addObject(G1)
    except:
      FreeCAD.Console.PrintLog("Cannot add group to itself, OK\n")
    else:
      self.fail("Adding the group to itself must not be possible")

    self.Doc.UndoMode = 1

    # Remove object from group
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Label_2")
    self.Doc.commitTransaction()
    self.failUnless(G1.getObject("Label_2") == None)
    self.Doc.undo()
    self.failUnless(G1.getObject("Label_2") != None)

    # Remove first group and then the object
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Group")
    self.Doc.removeObject("Label_2")
    self.Doc.commitTransaction()
    self.Doc.undo()
    self.failUnless(G1.getObject("Label_2") != None)

    # Remove first object and then the group in two transactions
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Label_2")
    self.Doc.commitTransaction()
    self.failUnless(G1.getObject("Label_2") == None)
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Group")
    self.Doc.commitTransaction()
    self.Doc.undo()
    self.Doc.undo()
    self.failUnless(G1.getObject("Label_2") != None)

    # Remove first object and then the group in one transaction
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Label_2")
    self.failUnless(G1.getObject("Label_2") == None)
    self.Doc.removeObject("Group")
    self.Doc.commitTransaction()
    self.Doc.undo()
    # FIXME: See bug #1820554
    self.failUnless(G1.getObject("Label_2") != None)

    # Add a second object to the group
    L3 = self.Doc.addObject("App::FeatureTest","Label_3")
    G1.addObject(L3)
    self.Doc.openTransaction("Remove")
    self.Doc.removeObject("Label_2")
    self.failUnless(G1.getObject("Label_2") == None)
    self.Doc.removeObject("Label_3")
    self.failUnless(G1.getObject("Label_3") == None)
    self.Doc.removeObject("Group")
    self.Doc.commitTransaction()
    self.Doc.undo()
    self.failUnless(G1.getObject("Label_3") != None)
    self.failUnless(G1.getObject("Label_2") != None)

    self.Doc.UndoMode = 0

    # Cleanup
    self.Doc.removeObject("Group")
    self.Doc.removeObject("Label_2")
    self.Doc.removeObject("Label_3")


  def tearDown(self):
    # closing doc
    FreeCAD.closeDocument("UndoTest")


class DocumentPlatformCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("PlatformTests")
    self.Doc.addObject("App::FeatureTest", "Test")
    self.TempPath = tempfile.gettempdir()
    self.DocName = self.TempPath + os.sep + "PlatformTests.FCStd"

  def testFloatList(self):
    self.Doc.Test.FloatList = [-0.05, 2.5, 5.2]

    # saving and restoring
    self.Doc.saveAs(self.DocName)
    FreeCAD.closeDocument("PlatformTests")
    self.Doc = FreeCAD.open(self.DocName)

    self.failUnless(abs(self.Doc.Test.FloatList[0] + .05) < 0.01)
    self.failUnless(abs(self.Doc.Test.FloatList[1] - 2.5) < 0.01)
    self.failUnless(abs(self.Doc.Test.FloatList[2] - 5.2) < 0.01)

  def testColorList(self):
    self.Doc.Test.ColourList = [(1.0,0.5,0.0),(0.0,0.5,1.0)]

    # saving and restoring
    self.Doc.saveAs(self.DocName)
    FreeCAD.closeDocument("PlatformTests")
    self.Doc = FreeCAD.open(self.DocName)

    self.failUnless(abs(self.Doc.Test.ColourList[0][0] - 1.0) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[0][1] - 0.5) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[0][2] - 0.0) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[0][3] - 0.0) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[1][0] - 0.0) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[1][1] - 0.5) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[1][2] - 1.0) < 0.01)
    self.failUnless(abs(self.Doc.Test.ColourList[1][3] - 0.0) < 0.01)

  def testVectorList(self):
    self.Doc.Test.VectorList = [(-0.05, 2.5, 5.2),(-0.05, 2.5, 5.2)]

    # saving and restoring
    self.Doc.saveAs(self.DocName)
    FreeCAD.closeDocument("PlatformTests")
    self.Doc = FreeCAD.open(self.DocName)

    self.failUnless(len(self.Doc.Test.VectorList) == 2)

  def testPoints(self):
    try:
      self.Doc.addObject("Points::Feature", "Points")

      # saving and restoring
      self.Doc.saveAs(self.DocName)
      FreeCAD.closeDocument("PlatformTests")
      self.Doc = FreeCAD.open(self.DocName)

      self.failUnless(self.Doc.Points.Points.count() == 0)
    except:
      pass

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("PlatformTests")
class DocumentFileIncludeCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("FileIncludeTests")
    # testing with undo
    self.Doc.UndoMode = 1


  def testApplyFiles(self):
    self.Doc.openTransaction("Transaction0")
    self.L1 = self.Doc.addObject("App::DocumentObjectFileIncluded","FileObject1")
    self.failUnless(self.L1.File =="")
    self.Filename = self.L1.File

    self.Doc.openTransaction("Transaction1")
    self.TempPath = tempfile.gettempdir()
    # creating a file in the Transient directory of the document
    file = open(self.Doc.getTempFileName("test"),"w")
    file.write("test No1")
    file.close()
    # applying the file
    self.L1.File = (file.name,"Test.txt")
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    # read again
    file = open(self.L1.File,"r")
    self.failUnless(file.read()=="test No1")
    file.close()
    file = open(self.TempPath+"/testNest.txt","w")
    file.write("test No2")
    file.close()
    # applying the file
    self.Doc.openTransaction("Transaction2")
    self.L1.File = file.name
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    # read again
    file = open(self.L1.File,"r")
    self.failUnless(file.read()=="test No2")
    file.close()
    self.Doc.undo()
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    # read again
    file = open(self.L1.File,"r")
    self.failUnless(file.read()=="test No1")
    file.close()
    self.Doc.undo()
    # read again
    self.failUnless(self.L1.File == "")
    self.Doc.redo()
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    # read again
    file = open(self.L1.File,"r")
    self.failUnless(file.read()=="test No1")
    file.close()
    self.Doc.redo()
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    # read again
    file = open(self.L1.File,"r")
    self.failUnless(file.read()=="test No2")
    file.close()
    # Save restore test
    FileName = self.TempPath+"/FileIncludeTests.fcstd"
    self.Doc.saveAs(FileName)
    FreeCAD.closeDocument("FileIncludeTests")
    self.Doc = FreeCAD.open(self.TempPath+"/FileIncludeTests.fcstd")
    # check if the file is still there
    self.L1 = self.Doc.getObject("FileObject1")
    file = open(self.L1.File,"r")
    res = file.read()
    FreeCAD.Console.PrintLog( res +"\n")
    self.failUnless(res=="test No2")
    self.failUnless(self.L1.File.split("/")[-1] == "Test.txt")
    file.close()

    # test for bug #94 (File overlap in PropertyFileIncluded)
    L2 = self.Doc.addObject("App::DocumentObjectFileIncluded","FileObject2")
    L3 = self.Doc.addObject("App::DocumentObjectFileIncluded","FileObject3")

    # creating two files in the Transient directory of the document
    file1 = open(self.Doc.getTempFileName("test"),"w")
    file1.write("test No1")
    file1.close()
    file2 = open(self.Doc.getTempFileName("test"),"w")
    file2.write("test No2")
    file2.close()

    # applying the file with the same base name
    L2.File = (file1.name,"Test.txt")
    L3.File = (file2.name,"Test.txt")

    file = open(L2.File,"r")
    self.failUnless(file.read()=="test No1")
    file.close()
    file = open(L3.File,"r")
    self.failUnless(file.read()=="test No2")
    file.close()

    # create a second document, copy a file and close the document
    # the test is about to put the file to the correct transient dir
    doc2 = FreeCAD.newDocument("Doc2")
    L4 = doc2.addObject("App::DocumentObjectFileIncluded","FileObject")
    L5 = doc2.addObject("App::DocumentObjectFileIncluded","FileObject")
    L6 = doc2.addObject("App::DocumentObjectFileIncluded","FileObject")
    L4.File = (L3.File,"Test.txt")
    L5.File = L3.File
    L6.File = L3.File
    FreeCAD.closeDocument("FileIncludeTests")
    self.Doc = FreeCAD.open(self.TempPath+"/FileIncludeTests.fcstd")
    self.failUnless(os.path.exists(L4.File))
    self.failUnless(os.path.exists(L5.File))
    self.failUnless(os.path.exists(L6.File))
    self.failUnless(L5.File != L6.File)
    # copy file from L5 which is in the same directory
    L7 = doc2.addObject("App::DocumentObjectFileIncluded","FileObject3")
    L7.File = (L5.File,"Copy.txt")
    self.failUnless(os.path.exists(L5.File))
    FreeCAD.closeDocument("Doc2")


  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("FileIncludeTests")


class DocumentPropertyCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("PropertyTests")
    self.Obj = self.Doc.addObject("App::FeaturePython","Test")

  def testDescent(self):
    # testing the up and downstream stuff
    props=self.Obj.supportedProperties()
    for i in props:
        self.Obj.addProperty(i,i)
    tempPath = tempfile.gettempdir()
    tempFile = tempPath + os.sep + "PropertyTests.FCStd"
    self.Doc.saveAs(tempFile)
    FreeCAD.closeDocument("PropertyTests")
    self.Doc = FreeCAD.open(tempFile)

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("PropertyTests")
