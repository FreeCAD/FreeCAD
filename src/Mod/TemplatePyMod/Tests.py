# FreeCAD TemplatePyMod module  
# (c) 2007 Juergen Riegel LGPL


import FreeCAD, unittest


#---------------------------------------------------------------------------
# define the test cases for this module
#---------------------------------------------------------------------------


class ParameterTestCase(unittest.TestCase):
    def setUp(self):
        self.TestPar = FreeCAD.ParamGet("System parameter:Test")
        
    def testGroup(self):
        #FreeCAD.PrintLog("Base::ParameterTestCase::testGroup\n")
        # check on Group creation
        Temp = self.TestPar.GetGroup("44")
        self.failUnless(self.TestPar.HasGroup("44"),"Test on created group failed")
        # check on Deletion
        self.TestPar.RemGroup("44")
        self.failUnless(not self.TestPar.HasGroup("44"),"Test on delete group failed")
        Temp =0

        #check on special conditions
    def testInt(self):
        #FreeCAD.PrintLog("Base::ParameterTestCase::testInt\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetInt("44",4711)
        self.failUnless(self.TestPar.GetInt("44") == 4711,"In and out error at Int")
        # check on Deletion
        self.TestPar.RemInt("44")
        self.failUnless(self.TestPar.GetInt("44",1) == 1,"Deletion error at Int")
        

    def testBool(self):
        #FreeCAD.PrintLog("Base::ParameterTestCase::testBool\n")
        # check on Int
        self.TestPar.SetBool("44",1)
        self.failUnless(self.TestPar.GetBool("44") == 1,"In and out error at Bool")
        # check on Deletion
        self.TestPar.RemBool("44")
        self.failUnless(self.TestPar.GetBool("44",0) == 0,"Deletion error at Bool")

    def testFloat(self):
        #FreeCAD.PrintLog("Base::ParameterTestCase::testFloat\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetFloat("44",4711.4711)
        self.failUnless(self.TestPar.GetFloat("44") == 4711.4711,"In and out error at Float")
        # check on Deletion
        self.TestPar.RemFloat("44")
        self.failUnless(self.TestPar.GetFloat("44",1.1) == 1.1,"Deletion error at Float")

    def testString(self):
        #FreeCAD.PrintLog("Base::ParameterTestCase::testFloat\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetString("44","abcdefgh")
        self.failUnless(self.TestPar.GetString("44") == "abcdefgh","In and out error at String")
        # check on Deletion
        self.TestPar.RemString("44")
        self.failUnless(self.TestPar.GetString("44","hallo") == "hallo","Deletion error at String")

    def testNesting(self):
        # Parameter testing
        #FreeCAD.PrintLog("Base::ParameterTestCase::testNesting\n")
        for i in range(50):
            self.TestPar.SetFloat(i,4711.4711)
            self.TestPar.SetInt(i,4711)
            self.TestPar.SetBool(i,1)
            Temp = self.TestPar.GetGroup(i)
            for l in range(50):
                Temp.SetFloat(l,4711.4711)
                Temp.SetInt(l,4711)
                Temp.SetBool(l,1)
        Temp = 0
        
    def testExportImport(self):
        # Parameter testing
        #FreeCAD.PrintLog("Base::ParameterTestCase::testNesting\n")
        self.TestPar.SetFloat("ExTest",4711.4711)
        self.TestPar.SetInt("ExTest",4711)
        self.TestPar.SetString("ExTest","4711")
        self.TestPar.SetBool("ExTest",1)
        Temp = self.TestPar.GetGroup("ExTest")
        Temp.SetFloat("ExTest",4711.4711)
        Temp.SetInt("ExTest",4711)
        Temp.SetString("ExTest","4711")
        Temp.SetBool("ExTest",1)
        self.TestPar.Export("ExportTest.FCExport")
        Temp = self.TestPar.GetGroup("ImportTest")
        Temp.Import("ExportTest.FCExport")
        self.failUnless(Temp.GetFloat("ExTest") == 4711.4711,"ExportImport error")
        Temp = 0
        
    def tearDown(self):
        #remove all
        TestPar = FreeCAD.ParamGet("System parameter:Test")
        TestPar.Clear()



    


        

