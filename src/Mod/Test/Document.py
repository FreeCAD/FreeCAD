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
import math


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

  def testAbortTransaction(self):
    self.Doc.openTransaction("Add")
    obj=self.Doc.addObject("App::FeatureTest","Label")
    self.Doc.abortTransaction()
    TempPath = tempfile.gettempdir()
    SaveName = TempPath + os.sep + "SaveRestoreTests.FCStd"
    self.Doc.saveAs(SaveName)

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
      self.fail("no exception thrown")
    self.Doc.RedoCount
    self.Doc.UndoNames
    self.Doc.RedoNames
    self.Doc.recompute()
    self.failUnless(L1.Integer == 4711)
    self.failUnless(L1.Float-47.11<0.001)
    self.failUnless(L1.Bool    == True)
    self.failUnless(L1.String  == "4711")
    #temporarily not checked because of strange behavior of boost::filesystem JR
    #self.failUnless(L1.Path  == "c:/temp")
    self.failUnless(float(L1.Angle)-3.0<0.001)
    self.failUnless(float(L1.Distance)-47.11<0.001)

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
    # in App::FeatureTest the current value is set to 4
    self.failUnless(L1.Enum  == "Four")
    L1.Enum = "Three"
    self.failUnless(L1.Enum  == "Three",     "Different value to 'Three'")
    L1.Enum = 2
    self.failUnless(L1.Enum  == "Two",     "Different value to 'Two'")
    try:
      L1.Enum = "SurelyNotInThere!"
    except:
      FreeCAD.Console.PrintLog("   exception thrown, OK\n")
    else:
      self.fail("no exception thrown")

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

  def testEnum(self):
    enumeration_choices = ["one", "two"]
    obj = self.Doc.addObject("App::FeaturePython","Label_2")
    obj.addProperty("App::PropertyEnumeration", "myEnumeration", "Enum", "mytest")
    with self.assertRaises(ValueError):
      obj.myEnumeration = enumeration_choices[0]

  def testMem(self):
    self.Doc.MemSize

  def testDuplicateLinks(self):
    obj = self.Doc.addObject("App::FeatureTest","obj")
    grp = self.Doc.addObject("App::DocumentObjectGroup","group")
    grp.Group = [obj,obj]
    self.Doc.removeObject(obj.Name)
    self.assertListEqual(grp.Group, [])

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

  def testExtensions(self):
    #we try to create a normal python object and add an extension to it
    obj = self.Doc.addObject("App::DocumentObject", "Extension_1")
    grp = self.Doc.addObject("App::DocumentObject", "Extension_2")
    #we should have all methods we need to handle extensions
    try:
      self.failUnless(not grp.hasExtension("App::GroupExtensionPython"))
      grp.addExtension("App::GroupExtensionPython", self)
      self.failUnless(grp.hasExtension("App::GroupExtension"))
      self.failUnless(grp.hasExtension("App::GroupExtensionPython"))
      grp.addObject(obj)
      self.failUnless(len(grp.Group) == 1)
      self.failUnless(grp.Group[0] == obj)
    except:
      self.failUnless(False)

    #test if the method override works
    class SpecialGroup():
        def allowObject(self, obj):
            return False;

    callback = SpecialGroup()
    grp2 = self.Doc.addObject("App::DocumentObject", "Extension_3")
    grp2.addExtension("App::GroupExtensionPython", callback)

    try:
      self.failUnless(grp2.hasExtension("App::GroupExtension"))
      grp2.addObject(obj)
      self.failUnless(len(grp2.Group) == 0)
    except:
      self.failUnless(True)

    self.Doc.removeObject(grp.Name)
    self.Doc.removeObject(grp2.Name)
    self.Doc.removeObject(obj.Name)
    del obj
    del grp
    del grp2

  def testExtensionBug0002785(self):

        class MyExtension():
            def __init__(self, obj):
                obj.addExtension("App::GroupExtensionPython", self)

        obj = self.Doc.addObject("App::DocumentObject", "myObj")
        MyExtension(obj)
        self.failUnless(obj.hasExtension("App::GroupExtension"))
        self.failUnless(obj.hasExtension("App::GroupExtensionPython"))
        self.Doc.removeObject(obj.Name)
        del obj

  def testExtensionGroup(self):
    obj = self.Doc.addObject("App::DocumentObject", "Obj")
    grp = self.Doc.addObject("App::FeaturePython", "Extension_2")
    grp.addExtension("App::GroupExtensionPython", None)
    grp.Group = [obj]
    self.assertTrue(obj in grp.Group)

  def testExtensionBugViewProvider(self):

    class Layer():
      def __init__(self, obj):
        obj.addExtension("App::GroupExtensionPython", self)

    class LayerViewProvider():
      def __init__(self, obj):
        obj.addExtension("Gui::ViewProviderGroupExtensionPython", self)
        obj.Proxy = self

    obj = self.Doc.addObject("App::FeaturePython","Layer")
    Layer(obj)
    self.failUnless(obj.hasExtension("App::GroupExtension"))

    if FreeCAD.GuiUp:
        LayerViewProvider(obj.ViewObject)
        self.failUnless(obj.ViewObject.hasExtension("Gui::ViewProviderGroupExtension"))
        self.failUnless(obj.ViewObject.hasExtension("Gui::ViewProviderGroupExtensionPython"))

    self.Doc.removeObject(obj.Name)
    del obj

  def testPropertyLink_Issue2902Part1(self):
    o1 = self.Doc.addObject("App::FeatureTest","test1")
    o2 = self.Doc.addObject("App::FeatureTest","test2")
    o3 = self.Doc.addObject("App::FeatureTest","test3")

    o1.Link=o2
    self.assertEqual(o1.Link, o2)
    o1.Link=o3
    self.assertEqual(o1.Link, o3)
    o2.Placement = FreeCAD.Placement()
    self.assertEqual(o1.Link, o3)

  def testNotification_Issue2902Part2(self):
    o = self.Doc.addObject("App::FeatureTest","test")

    plm = o.Placement
    o.Placement = FreeCAD.Placement()
    plm.Base.x = 5
    self.assertEqual(o.Placement.Base.x, 0)
    o.Placement.Base.x=5
    self.assertEqual(o.Placement.Base.x, 5)

  def testNotification_Issue2996(self):
    if not FreeCAD.GuiUp:
      return
    # works only if Gui is shown
    class ViewProvider:
      def __init__(self, vobj):
        vobj.Proxy=self

      def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

      def claimChildren(self):
        children = [self.Object.Link]
        return children

    obj=self.Doc.addObject("App::FeaturePython", "Sketch")
    obj.addProperty("App::PropertyLink","Link")
    ViewProvider(obj.ViewObject)

    ext=self.Doc.addObject("App::FeatureTest", "Extrude")
    ext.Link=obj

    sli=self.Doc.addObject("App::FeaturePython", "Slice")
    sli.addProperty("App::PropertyLink","Link").Link=ext
    ViewProvider(sli.ViewObject)

    com=self.Doc.addObject("App::FeaturePython", "CompoundFilter")
    com.addProperty("App::PropertyLink", "Link").Link=sli
    ViewProvider(com.ViewObject)

    ext.Label="test"

    self.assertEqual(ext.Link, obj)
    self.assertNotEqual(ext.Link, sli)

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("CreateTest")

# class must be defined in global scope to allow it to be reloaded on document open
class SaveRestoreSpecialGroup():
    def __init__(self, obj):
        obj.addExtension("App::GroupExtensionPython", self)
        obj.Proxy = self

    def allowObject(self, obj):
        return False;

# class must be defined in global scope to allow it to be reloaded on document open
class SaveRestoreSpecialGroupViewProvider():
    def __init__(self, obj):
        obj.addExtension("Gui::ViewProviderGroupExtensionPython", self)
        obj.Proxy = self

    def testFunction(self):
        pass

class DocumentSaveRestoreCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("SaveRestoreTests")
    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    L3 = self.Doc.addObject("App::FeatureTest","Label_3")
    self.TempPath = tempfile.gettempdir()
    FreeCAD.Console.PrintLog( '  Using temp path: ' + self.TempPath + '\n')

  def testSaveAndRestore(self):
    # saving and restoring
    SaveName = self.TempPath + os.sep + "SaveRestoreTests.FCStd"
    self.failUnless(self.Doc.Label_1.TypeTransient == 4711)
    self.Doc.Label_1.TypeTransient = 4712
    # setup Linking
    self.Doc.Label_1.Link = self.Doc.Label_2
    self.Doc.Label_2.Link = self.Doc.Label_3
    self.Doc.Label_1.LinkSub = (self.Doc.Label_2,["Sub1","Sub2"])
    self.Doc.Label_2.LinkSub = (self.Doc.Label_3,["Sub3","Sub4"])
    # save the document
    self.Doc.saveAs(SaveName)
    FreeCAD.closeDocument("SaveRestoreTests")
    self.Doc = FreeCAD.open(SaveName)
    self.failUnless(self.Doc.Label_1.Integer == 4711)
    self.failUnless(self.Doc.Label_2.Integer == 4711)
    # test Linkage
    self.failUnless(self.Doc.Label_1.Link == self.Doc.Label_2)
    self.failUnless(self.Doc.Label_2.Link == self.Doc.Label_3)
    self.failUnless(self.Doc.Label_1.LinkSub == (self.Doc.Label_2,["Sub1","Sub2"]))
    self.failUnless(self.Doc.Label_2.LinkSub == (self.Doc.Label_3,["Sub3","Sub4"]))
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

  def testExtensionSaveRestore(self):
    # saving and restoring
    SaveName = self.TempPath + os.sep + "SaveRestoreExtensions.FCStd"
    Doc = FreeCAD.newDocument("SaveRestoreExtensions")
    #we try to create a normal python object and add an extension to it
    obj  = Doc.addObject("App::DocumentObject", "Obj")
    grp1 = Doc.addObject("App::DocumentObject", "Extension_1")
    grp2 = Doc.addObject("App::FeaturePython", "Extension_2")

    grp1.addExtension("App::GroupExtensionPython", None)
    SaveRestoreSpecialGroup(grp2)
    if FreeCAD.GuiUp:
        SaveRestoreSpecialGroupViewProvider(grp2.ViewObject)
    grp2.Group = [obj]

    Doc.saveAs(SaveName)
    FreeCAD.closeDocument("SaveRestoreExtensions")
    Doc = FreeCAD.open(SaveName)

    self.failUnless(Doc.Extension_1.hasExtension("App::GroupExtension"))
    self.failUnless(Doc.Extension_2.hasExtension("App::GroupExtension"))
    self.failUnless(Doc.Extension_1.ExtensionProxy is None)
    self.failUnless(Doc.Extension_2.ExtensionProxy is not None)
    self.failUnless(Doc.Extension_2.Group[0] is Doc.Obj)
    self.failUnless(hasattr(Doc.Extension_2.Proxy, 'allowObject'))
    self.failUnless(hasattr(Doc.Extension_2.ExtensionProxy, 'allowObject'))

    if FreeCAD.GuiUp:
      self.failUnless(Doc.Extension_2.ViewObject.hasExtension("Gui::ViewProviderGroupExtensionPython"))
      self.failUnless(hasattr(Doc.Extension_2.ViewObject.Proxy, 'testFunction'))
      self.failUnless(hasattr(Doc.Extension_2.ViewObject.ExtensionProxy, 'testFunction'))

    FreeCAD.closeDocument("SaveRestoreExtensions")

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

  def testRecompute(self):

    # sequence to test recompute behaviour
    #       L1---\    L7
    #      /  \   \    |
    #    L2   L3   \  L8
    #   /  \ /  \  /
    #  L4   L5   L6

    L1 = self.Doc.addObject("App::FeatureTest","Label_1")
    L2 = self.Doc.addObject("App::FeatureTest","Label_2")
    L3 = self.Doc.addObject("App::FeatureTest","Label_3")
    L4 = self.Doc.addObject("App::FeatureTest","Label_4")
    L5 = self.Doc.addObject("App::FeatureTest","Label_5")
    L6 = self.Doc.addObject("App::FeatureTest","Label_6")
    L7 = self.Doc.addObject("App::FeatureTest","Label_7")
    L8 = self.Doc.addObject("App::FeatureTest","Label_8")
    L1.LinkList = [L2,L3,L6]
    L2.Link = L4
    L2.LinkList = [L5]
    L3.LinkList = [L5,L6]
    L7.Link = L8 #make second root

    self.failUnless(L7 in self.Doc.RootObjects)
    self.failUnless(L1 in self.Doc.RootObjects)

    self.failUnless(len(self.Doc.Objects) == len(self.Doc.TopologicalSortedObjects))

    seqDic = {}
    i = 0
    for obj in self.Doc.TopologicalSortedObjects:
        seqDic[obj] = i
        print(obj)
        i += 1

    self.failUnless(seqDic[L2] > seqDic[L1])
    self.failUnless(seqDic[L3] > seqDic[L1])
    self.failUnless(seqDic[L5] > seqDic[L2])
    self.failUnless(seqDic[L5] > seqDic[L3])
    self.failUnless(seqDic[L5] > seqDic[L1])


    self.failUnless((0, 0, 0, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    self.failUnless(self.Doc.recompute()==4)
    self.failUnless((1, 1, 1, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L5.touch()
    self.failUnless((1, 1, 1, 0, 0, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    self.failUnless(self.Doc.recompute()==4)
    self.failUnless((2, 2, 2, 0, 1, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L4.touch()
    self.failUnless(self.Doc.recompute()==3)
    self.failUnless((3, 3, 2, 1, 1, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L5.touch()
    self.failUnless(self.Doc.recompute()==4)
    self.failUnless((4, 4, 3, 1, 2, 0)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L6.touch()
    self.failUnless(self.Doc.recompute()==3)
    self.failUnless((5, 4, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L2.touch()
    self.failUnless(self.Doc.recompute()==2)
    self.failUnless((6, 5, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))
    L1.touch()
    self.failUnless(self.Doc.recompute()==1)
    self.failUnless((7, 5, 4, 1, 2, 1)==(L1.ExecCount,L2.ExecCount,L3.ExecCount,L4.ExecCount,L5.ExecCount,L6.ExecCount))

    self.Doc.removeObject(L1.Name)
    self.Doc.removeObject(L2.Name)
    self.Doc.removeObject(L3.Name)
    self.Doc.removeObject(L4.Name)
    self.Doc.removeObject(L5.Name)
    self.Doc.removeObject(L6.Name)
    self.Doc.removeObject(L7.Name)
    self.Doc.removeObject(L8.Name)

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

  def testUndoInList(self):

    self.Doc.UndoMode = 1

    self.Doc.openTransaction("Box")
    self.Box = self.Doc.addObject('App::FeatureTest')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Cylinder")
    self.Cylinder = self.Doc.addObject('App::FeatureTest')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Fuse")
    self.Fuse1 = self.Doc.addObject('App::FeatureTest', 'Fuse')
    self.Fuse1.LinkList = [self.Box, self.Cylinder]
    self.Doc.commitTransaction()

    self.Doc.undo()
    self.failUnless(len(self.Box.InList) == 0)
    self.failUnless(len(self.Cylinder.InList) == 0)

    self.Doc.redo()
    self.failUnless(len(self.Box.InList) == 1)
    self.failUnless(self.Box.InList[0] == self.Doc.Fuse)
    self.failUnless(len(self.Cylinder.InList) == 1)
    self.failUnless(self.Cylinder.InList[0] == self.Doc.Fuse)

  def testUndoIssue0003150Part1(self):

    self.Doc.UndoMode = 1

    self.Doc.openTransaction("Box")
    self.Box = self.Doc.addObject('App::FeatureTest')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Cylinder")
    self.Cylinder = self.Doc.addObject('App::FeatureTest')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Fuse")
    self.Fuse1 = self.Doc.addObject('App::FeatureTest')
    self.Fuse1.LinkList = [self.Box, self.Cylinder]
    self.Doc.commitTransaction()
    self.Doc.recompute()

    self.Doc.openTransaction("Sphere")
    self.Sphere = self.Doc.addObject('App::FeatureTest')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Fuse")
    self.Fuse2 = self.Doc.addObject('App::FeatureTest')
    self.Fuse2.LinkList = [self.Fuse1, self.Sphere]
    self.Doc.commitTransaction()
    self.Doc.recompute()

    self.Doc.openTransaction("Part")
    self.Part = self.Doc.addObject('App::Part')
    self.Doc.commitTransaction()

    self.Doc.openTransaction("Drag")
    self.Part.addObject(self.Fuse2)
    self.Doc.commitTransaction()

    #3 undos show the problem of failing recompute
    self.Doc.undo()
    self.Doc.undo()
    self.Doc.undo()
    self.failUnless(self.Doc.recompute() >= 0)

  def tearDown(self):
    # closing doc
    FreeCAD.closeDocument("UndoTest")

class DocumentGroupCases(unittest.TestCase):

  def setUp(self):
    self.Doc = FreeCAD.newDocument("GroupTests")

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

  def testGroupAndGeoFeatureGroup(self):

    # an object can only be in one group at once, that must be enforced
    obj1 = self.Doc.addObject("App::FeatureTest","obj1")
    grp1 = self.Doc.addObject("App::DocumentObjectGroup","Group1")
    grp2 = self.Doc.addObject("App::DocumentObjectGroup","Group2")
    grp1.addObject(obj1)
    self.failUnless(obj1.getParentGroup()==grp1)
    self.failUnless(obj1.getParentGeoFeatureGroup()==None)
    self.failUnless(grp1.hasObject(obj1))
    grp2.addObject(obj1)
    self.failUnless(grp1.hasObject(obj1)==False)
    self.failUnless(grp2.hasObject(obj1))

    # an object is allowed to be in a group and a geofeaturegroup
    prt1 = self.Doc.addObject("App::Part","Part1")
    prt2 = self.Doc.addObject("App::Part","Part2")

    prt1.addObject(grp2)
    self.failUnless(grp2.getParentGeoFeatureGroup()==prt1)
    self.failUnless(grp2.getParentGroup()==None)
    self.failUnless(grp2.hasObject(obj1))
    self.failUnless(prt1.hasObject(grp2))
    self.failUnless(prt1.hasObject(obj1))

    #it is not allowed to be in 2 geofeaturegroups
    prt2.addObject(grp2)
    self.failUnless(grp2.hasObject(obj1))
    self.failUnless(prt1.hasObject(grp2)==False)
    self.failUnless(prt1.hasObject(obj1)==False)
    self.failUnless(prt2.hasObject(grp2))
    self.failUnless(prt2.hasObject(obj1))
    try:
        grp = prt1.Group
        grp.append(obj1)
        prt1.Group = grp
    except:
        grp.remove(obj1)
        self.failUnless(prt1.Group == grp)
    else:
        self.fail("No exception thrown when object is in multiple Groups")

    #it is not allowed to be in 2 Groups
    prt2.addObject(grp1)
    grp = grp1.Group
    grp.append(obj1)
    try:
        grp1.Group = grp
    except:
        pass
    else:
        self.fail("No exception thrown when object is in multiple Groups")

    #cross linking between GeoFeatureGroups is not allowed
    self.Doc.recompute()
    box = self.Doc.addObject("App::FeatureTest","Box")
    cyl = self.Doc.addObject("App::FeatureTest","Cylinder")
    fus = self.Doc.addObject("App::FeatureTest","Fusion")
    fus.LinkList = [cyl, box]
    self.Doc.recompute()
    self.failUnless(fus.State[0] == 'Up-to-date')
    fus.LinkList = [] #remove all links as addObject would otherwise transfer all linked objects
    prt1.addObject(cyl)
    fus.LinkList = [cyl, box]
    self.Doc.recompute()
    #self.failUnless(fus.State[0] == 'Invalid')
    fus.LinkList = []
    prt1.addObject(box)
    fus.LinkList = [cyl, box]
    self.Doc.recompute()
    #self.failUnless(fus.State[0] == 'Invalid')
    fus.LinkList = []
    prt1.addObject(fus)
    fus.LinkList = [cyl, box]
    self.Doc.recompute()
    self.failUnless(fus.State[0] == 'Up-to-date')
    prt2.addObject(box) #this time addObject should move all dependencies to the new part
    self.Doc.recompute()
    self.failUnless(fus.State[0] == 'Up-to-date')

    #grouping must be resilient against cyclic links and not crash: #issue 0002567
    prt1.addObject(prt2)
    grp = prt2.Group
    grp.append(prt1)
    prt2.Group = grp
    self.Doc.recompute()
    prt2.Group = []
    try:
        prt2.Group = [prt2]
    except:
        pass
    else:
        self.fail("Exception is expected")

    self.Doc.recompute()

  def testIssue0003150Part2(self):
    self.box = self.Doc.addObject("App::FeatureTest")
    self.cyl = self.Doc.addObject("App::FeatureTest")
    self.sph = self.Doc.addObject("App::FeatureTest")

    self.fus1 = self.Doc.addObject("App::FeatureTest")
    self.fus2 = self.Doc.addObject("App::FeatureTest")

    self.fus1.LinkList = [self.box, self.cyl];
    self.fus2.LinkList = [self.sph, self.cyl];

    self.prt = self.Doc.addObject("App::Part")
    self.prt.addObject(self.fus1)
    self.failUnless(len(self.prt.Group)==5)
    self.failUnless(self.fus2.getParentGeoFeatureGroup() == self.prt)
    self.failUnless(self.prt.hasObject(self.sph))

    self.prt.removeObject(self.fus1)
    self.failUnless(len(self.prt.Group)==0)

  def tearDown(self):
    # closing doc
    FreeCAD.closeDocument("GroupTests")

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


class DocumentBacklinks(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument("BackLinks")

  def testIssue0003323(self):
    self.Doc.UndoMode=1
    self.Doc.openTransaction("Create object")
    obj1=self.Doc.addObject("App::FeatureTest","Test1")
    obj2=self.Doc.addObject("App::FeatureTest","Test2")
    obj2.Link=obj1
    self.Doc.commitTransaction()
    self.Doc.undo()
    self.Doc.openTransaction("Create object")

  def tearDown(self):
    # closing doc
    FreeCAD.closeDocument("BackLinks")


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

  def testRemoveProperty(self):
    prop = 'Something'
    self.Obj.addProperty('App::PropertyFloat', prop)
    self.Obj.Something = 0.01
    self.Doc.recompute()
    self.Doc.openTransaction('modify and remove property')
    self.Obj.Something = 0.00
    self.Obj.removeProperty(prop)
    self.Obj.recompute()
    self.Doc.abortTransaction()

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument("PropertyTests")


class DocumentExpressionCases(unittest.TestCase):
  def setUp(self):
    self.Doc = FreeCAD.newDocument()
    self.Obj1 = self.Doc.addObject("App::FeatureTest","Test")
    self.Obj2 = self.Doc.addObject("App::FeatureTest","Test")

  def assertAlmostEqual (self, v1, v2) :
    if (math.fabs(v2-v1) > 1E-12) :
      self.assertEqual(v1,v2)


  def testExpression(self):
    # set the object twice to test that the backlinks are removed when overwriting the expression
    self.Obj2.setExpression('Placement.Rotation.Angle', u'%s.Placement.Rotation.Angle' % self.Obj1.Name)
    self.Obj2.setExpression('Placement.Rotation.Angle', u'%s.Placement.Rotation.Angle' % self.Obj1.Name)
    self.Obj1.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(0,0,1),10))
    self.Doc.recompute()
    self.assertAlmostEqual(self.Obj1.Placement.Rotation.Angle, self.Obj2.Placement.Rotation.Angle)

    # clear the expression
    self.Obj2.setExpression('Placement.Rotation.Angle', None)
    self.assertAlmostEqual(self.Obj1.Placement.Rotation.Angle, self.Obj2.Placement.Rotation.Angle)
    self.Doc.recompute()
    self.assertAlmostEqual(self.Obj1.Placement.Rotation.Angle, self.Obj2.Placement.Rotation.Angle)
    # touch the objects to perform a recompute
    self.Obj1.Placement = self.Obj1.Placement
    self.Obj2.Placement = self.Obj2.Placement
    # must not raise a topological error
    self.assertEqual(self.Doc.recompute(), 2)

  def tearDown(self):
    #closing doc
    FreeCAD.closeDocument(self.Doc.Name)
