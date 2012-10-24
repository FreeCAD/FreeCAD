#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2004                       *   
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
#*   Juergen Riegel 2004                                                   *
#***************************************************************************/

import FreeCAD, os, unittest, tempfile

class ConsoleTestCase(unittest.TestCase):
    def setUp(self):
        self.count = 0

    def testPrint(self):
        FreeCAD.Console.PrintMessage("   Printing message\n")
        FreeCAD.Console.PrintError("   Printing error\n")
        FreeCAD.Console.PrintWarning("   Printing warning\n")
        FreeCAD.Console.PrintLog("   Printing Log\n")

    def testSynchronPrintFromThread(self):
        import thread, time
        def adder():
            lock.acquire()
            self.count=self.count+1
            # call of Console method is thread-safe
            FreeCAD.Console.PrintMessage("Call from Python thread: count="+str(self.count)+"\n")
            lock.release()

        lock=thread.allocate_lock()
        for i in range(10):
            thread.start_new(adder,())

        time.sleep(3)
        self.failUnless(self.count==10,"Synchronization of threads failed")
        FreeCAD.Console.PrintMessage(str(self.count)+"\n")

    def testAsynchronPrintFromThread(self):
        import thread, time
        def adder():
            self.count=self.count+1
            # call of Console method is thread-safe
            FreeCAD.Console.PrintMessage("Call from Python thread (not synchronized): count="+str(self.count)+"\n")

        lock=thread.allocate_lock()
        for i in range(10):
            thread.start_new(adder,())

        time.sleep(3)
        FreeCAD.Console.PrintMessage(str(self.count)+"\n")

#    def testStatus(self):
#        SLog = FreeCAD.GetStatus("Console","Log")
#        SErr = FreeCAD.GetStatus("Console","Err")
#        SWrn = FreeCAD.GetStatus("Console","Wrn")
#        SMsg = FreeCAD.GetStatus("Console","Msg")
#        FreeCAD.SetStatus("Console","Log",1)
#        FreeCAD.SetStatus("Console","Err",1)
#        FreeCAD.SetStatus("Console","Wrn",1)
#        FreeCAD.SetStatus("Console","Msg",1)
#        self.failUnless(FreeCAD.GetStatus("Console","Msg")==1,"Set and read status failed (Console,Msg)")
#        self.failUnless(FreeCAD.GetStatus("Console","Err")==1,"Set and read status failed (Console,Err)")
#        self.failUnless(FreeCAD.GetStatus("Console","Wrn")==1,"Set and read status failed (Console,Wrn)")
#        self.failUnless(FreeCAD.GetStatus("Console","Log")==1,"Set and read status failed (Console,Log)")
#        FreeCAD.SetStatus("Console","Log",0)
#        FreeCAD.SetStatus("Console","Err",0)
#        FreeCAD.SetStatus("Console","Wrn",0)
#        FreeCAD.SetStatus("Console","Msg",0)
#        self.failUnless(FreeCAD.GetStatus("Console","Msg")==0,"Set and read status failed (Console,Msg)")
#        self.failUnless(FreeCAD.GetStatus("Console","Err")==0,"Set and read status failed (Console,Err)")
#        self.failUnless(FreeCAD.GetStatus("Console","Wrn")==0,"Set and read status failed (Console,Wrn)")
#        self.failUnless(FreeCAD.GetStatus("Console","Log")==0,"Set and read status failed (Console,Log)")
#        FreeCAD.SetStatus("Console","Log",SLog)
#        FreeCAD.SetStatus("Console","Err",SErr)
#        FreeCAD.SetStatus("Console","Wrn",SWrn)
#        FreeCAD.SetStatus("Console","Msg",SMsg)

    def tearDown(self):
        pass

class ParameterTestCase(unittest.TestCase):
    def setUp(self):
        self.TestPar = FreeCAD.ParamGet("System parameter:Test")
        
    def testGroup(self):
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testGroup\n")
        # check on Group creation
        Temp = self.TestPar.GetGroup("44")
        self.failUnless(self.TestPar.HasGroup("44"),"Test on created group failed")
        # check on Deletion
        self.TestPar.RemGroup("44")
        self.failUnless(not self.TestPar.HasGroup("44"),"Test on delete group failed")
        Temp =0

        #check on special conditions
    def testInt(self):
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testInt\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetInt("44",4711)
        self.failUnless(self.TestPar.GetInt("44") == 4711,"In and out error at Int")
        # check on Deletion
        self.TestPar.RemInt("44")
        self.failUnless(self.TestPar.GetInt("44",1) == 1,"Deletion error at Int")
        

    def testBool(self):
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testBool\n")
        # check on Int
        self.TestPar.SetBool("44",1)
        self.failUnless(self.TestPar.GetBool("44") == 1,"In and out error at Bool")
        # check on Deletion
        self.TestPar.RemBool("44")
        self.failUnless(self.TestPar.GetBool("44",0) == 0,"Deletion error at Bool")

    def testFloat(self):
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testFloat\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetFloat("44",4711.4711)
        self.failUnless(self.TestPar.GetFloat("44") == 4711.4711,"In and out error at Float")
        # check on Deletion
        self.TestPar.RemFloat("44")
        self.failUnless(self.TestPar.GetFloat("44",1.1) == 1.1,"Deletion error at Float")

    def testString(self):
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testFloat\n")
        #Temp = FreeCAD.ParamGet("System parameter:Test/44")
        # check on Int
        self.TestPar.SetString("44","abcdefgh")
        self.failUnless(self.TestPar.GetString("44") == "abcdefgh","In and out error at String")
        # check on Deletion
        self.TestPar.RemString("44")
        self.failUnless(self.TestPar.GetString("44","hallo") == "hallo","Deletion error at String")

    def testMatrix(self):
        m=FreeCAD.Matrix(4,2,1,0,1,1,1,0,0,0,1,0,0,0,0,1)
        u=m.multiply(m.inverse())
        self.failUnless(u==FreeCAD.Matrix(),"Invalid inverse of matrix")

    def testNesting(self):
        # Parameter testing
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testNesting\n")
        for i in range(50):
            self.TestPar.SetFloat(`i`,4711.4711)
            self.TestPar.SetInt(`i`,4711)
            self.TestPar.SetBool(`i`,1)
            Temp = self.TestPar.GetGroup(`i`)
            for l in range(50):
                Temp.SetFloat(`l`,4711.4711)
                Temp.SetInt(`l`,4711)
                Temp.SetBool(`l`,1)
        Temp = 0
        
    def testExportImport(self):
        # Parameter testing
        #FreeCAD.Console.PrintLog("Base::ParameterTestCase::testNesting\n")
        self.TestPar.SetFloat("ExTest",4711.4711)
        self.TestPar.SetInt("ExTest",4711)
        self.TestPar.SetString("ExTest","4711")
        self.TestPar.SetBool("ExTest",1)
        Temp = self.TestPar.GetGroup("ExTest")
        Temp.SetFloat("ExTest",4711.4711)
        Temp.SetInt("ExTest",4711)
        Temp.SetString("ExTest","4711")
        Temp.SetBool("ExTest",1)
        TempPath = tempfile.gettempdir() + os.sep + "ExportTest.FCExport"

        self.TestPar.Export(TempPath)
        Temp = self.TestPar.GetGroup("ImportTest")
        Temp.Import(TempPath)
        self.failUnless(Temp.GetFloat("ExTest") == 4711.4711,"ExportImport error")
        Temp = 0
        
    def tearDown(self):
        #remove all
        TestPar = FreeCAD.ParamGet("System parameter:Test")
        TestPar.Clear()
