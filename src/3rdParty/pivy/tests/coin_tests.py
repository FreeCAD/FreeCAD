#!/usr/bin/env python

###
# Copyright (c) 2002-2007 Systems in Motion
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

###
# Pivy Coin unit test suite
#
# For detailed info on its usage and on how to write additional test cases
# read:
#   - http://pyunit.sourceforge.net/pyunit.html
#   - http://diveintopython.org/unit_testing/
#
# Invoke this script with '--help' for usage information.
#

from pivy.coin import *
import unittest
import math

class Autocasting(unittest.TestCase):
    def testFieldAutocast(self):
        """check autocasting for SoSFBool created through createInstance()"""
        t = SoType.fromName(SbName("SoSFBool"))
        self.assertTrue(isinstance(t.createInstance(), SoSFBool),
                        'not autocasted to an SoSFBool instance')

    def testNodeAutocast(self):
        """check autocasting for SoCone created through createInstance()"""
        t = SoType.fromName(SbName("SoCone"))
        self.assertTrue(isinstance(t.createInstance(), SoCone),
                        'not autocasted to an SoCone instance')

    def testNodeListAutocast(self):
        """check autocasting for elements in NodeList"""
        nl = SoNodeList()
        nl.append(SoCube())
        self.assertTrue(isinstance(nl.get(0), SoCube),
                        'not autocasted to an SoCube instance')

    def testTypeAutocast(self):
        """check if type objects return casted instances"""
        tCube = SoType.fromName(SbName("SoCube"))
        cube = tCube.createInstance()
        self.assertTrue(isinstance(cube, SoCube),
                        'SoType.createInstance not casted to SoCube')
        tSFBool = SoType.fromName(SbName("SoSFBool"))
        field = tSFBool.createInstance()
        self.assertTrue(isinstance(field, SoSFBool),
                        'SoType.createInstance not casted to SoSFBool')
        tPath = SoType.fromName(SbName("SoPath"))
        path = tPath.createInstance()
        self.assertTrue(isinstance(path, SoPath),
                        'SoType.createInstance not casted to SoPath') 

    def testFieldContainerAutocast(self):
        """check if a returned FieldContainer is casted"""
        m = SoMaterial()
        self.assertTrue(isinstance(m.diffuseColor.getContainer(), SoMaterial),
                        'SoField.getContainer is not casted correctly')
                        
class FieldSetValue(unittest.TestCase):
    """checks various setValue(s) calls for fields"""
    def testSFBool(self):
        """check setValue for SoSFBool"""
        t = SoSFBool()
        s = SoSFBool()
        t.setValue(True)
        s.setValue(t)
        self.assertTrue(True == t.getValue() == s.getValue(), 
                        'setValue on SoSFBool failed')

    def testSFColor(self):
        """check setValue for SoSFColor"""
        t = SoSFColor()
        s = SoSFColor()
        t.setValue(0,1,1)
        self.assertTrue(t.getValue() == SbColor(0,1,1), 
                        'setValue 3 floats on SoSFColor failed')
        t.setValue([0,1,0])
        self.assertTrue(t.getValue() == SbColor(0,1,0), 
                        'setValue float sequence on SoSFColor failed')
        t.setValue(SbColor(1,0,0))
        self.assertTrue(t.getValue() == SbColor(1,0,0), 
                        'setValue SbColor on SoSFColor failed')
        t.setValue(SbVec3f(1,0,1))
        self.assertTrue(t.getValue() == SbColor(1,0,1), 
                        'setValue SbVec3f on SoSFColor failed') 
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(),
                        'setValue othe SoSFcolor on SoSFColor failed')
        
    def testSFEngine(self):
        """check setValue for SoSFEngine"""
        t = SoSFEngine()
        s = SoSFEngine()
        c = SoCounter()
        t.setValue(c)
        self.assertTrue(t.getValue() == c, 
                        'setValue engine on SoSFEngine failed')
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(), 
                        'setValue other SoSFEngine on SoSFEngine failed')

    def testSFString(self):
        """check setValue for SoSFString"""
        t = SoSFString()
        s = SoSFString()
        c = SbString('bla')
        t.setValue(c)
        self.assertTrue(t.getValue() == c, 
                        'setValue SbString on SoSFString failed')
        t.setValue('hello')
        self.assertTrue(t.getValue() == 'hello', 
                        'setValue string on SoSFString failed')        
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(), 
                        'setValue other SoSFString on SoSFString failed')
        
    def testSFInt32(self):
        """check setValue for SoSFInt32"""
        t = SoSFInt32()
        s = SoSFInt32()
        t.setValue(10)
        s.setValue(t)
        self.assertTrue(10 == t.getValue() == s.getValue(), 
                        'setValue on SoSFInt32 failed')

    def testSFFloat(self):
        """check setValue for SoSFFloat"""
        t = SoSFFloat()
        s = SoSFFloat()
        t.setValue(10.5)
        s.setValue(t)
        self.assertTrue(10.5 == t.getValue() == s.getValue(), 
                        'setValue on SoSFFloat failed')

    def testSFShort(self):
        """check setValue for SoSFShort"""
        t = SoSFShort()
        s = SoSFShort()
        t.setValue(10)
        s.setValue(t)
        self.assertTrue(10 == t.getValue() == s.getValue(), 
                        'setValue on SoSFShort failed')

    def testSFMatrix(self):
        """check setValue for SoSFMatrix"""
        t = SoSFMatrix()
        s = SoSFMatrix()
        # m = SbMatrix([[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0]])
        m2 = SbMatrix([[1,1,1,1],[1,1,1,1],[1,1,1,1],[1,1,1,1]])
        t.setValue(m2)
        s.setValue(t)
        self.assertTrue(m2 == t.getValue() == s.getValue(), 
                        'setValue on SoSFMatrix failed')                       

    def testSFName(self):
        """check setValue for SoSFName"""
        t = SoSFName()
        s = SoSFName()
        c = SbName('bla')
        t.setValue(c)
        self.assertTrue(t.getValue() == c, 
                        'setValue SbName on SoSFName failed')
        t.setValue('hello')
        self.assertTrue(t.getValue() == 'hello', 
                        'setValue string on SoSFName failed')        
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(), 
                        'setValue other SoSFName on SoSFName failed')

    def testSFNode(self):
        """check setValue for SoSFNode"""
        t = SoSFNode()
        s = SoSFNode()
        c = SoCone()
        t.setValue(c)
        s.setValue(t)
        self.assertTrue(c == t.getValue() == s.getValue(), 
                        'setValue on SoSFNode failed')
        self.assertTrue(isinstance(t.getValue(), SoCone), 
                        'autocast on SoSFNode.getValue failed')
        
    def testSFRotation(self):
        """check setValue for SoSFRotation"""
        t = SoSFRotation()
        s = SoSFRotation()
        m = SbRotation(1,0,0,0)
        t.setValue(m)
        s.setValue(t)
        self.assertTrue(m == t.getValue() == s.getValue(), 
                        'setValue on SoSFRotation failed') 
        t.setValue(0,1,0,0)
        self.assertTrue(SbRotation(0,1,0,0) == t.getValue(),
                        'setValue on SoSFRotation from 4 values failed')
        t.setValue([0,0,1,0])
        self.assertTrue(SbRotation(0,0,1,0) == t.getValue(),
                        'setValue on SoSFRotation from 4 values failed')

    def testSoRotation(self):
        r = SoRotation()
        rot = r.rotation.getValue()
        rot *= SbRotation(SbVec3f(0,0,1), M_PI/90.0)
        r.rotation.setValue(rot)

    def testSFVec3f(self):
        """check setValue for SoSFVec3f"""
        t = SoSFVec3f()
        s = SoSFVec3f()
        t.setValue(0,1,1)
        self.assertTrue(t.getValue() == SbVec3f(0,1,1), 
                        'setValue 3 floats on SoSFVec3f failed')
        t.setValue([0,1,0])
        self.assertTrue(t.getValue() == SbVec3f(0,1,0), 
                        'setValue float sequence on SoSFVec3f failed')
        t.setValue(SbVec3f(1,0,0))
        self.assertTrue(t.getValue() == SbVec3f(1,0,0), 
                        'setValue SbVec3f on SoSFVec3f failed')
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(),
                        'setValue other SoSFVec3f on SoSFVec3f failed')


    def testSFVec2f(self):
        """check setValue for SoSFVec2f"""
        t = SoSFVec2f()
        s = SoSFVec2f()
        t.setValue(0,1)
        self.assertTrue(t.getValue() == SbVec2f(0,1), 
                        'setValue 2 floats on SoSFVec2f failed')
        t.setValue([1,0])
        self.assertTrue(t.getValue() == SbVec2f(1,0), 
                        'setValue float sequence on SoSFVec2f failed')
        t.setValue(SbVec2f(1,1))
        self.assertTrue(t.getValue() == SbVec2f(1,1), 
                        'setValue SbVec2f on SoSFVec2f failed')
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(),
                        'setValue other SoSFVec2f on SoSFVec2f failed')

    def testSFVec4f(self):
        """check setValue for SoSFVec4f"""
        t = SoSFVec4f()
        s = SoSFVec4f()
        t.setValue(0,1,1,0)
        self.assertTrue(t.getValue() == SbVec4f(0,1,1,0), 
                        'setValue 4 floats on SoSFVec4f failed')
        t.setValue([0,1,0,1])
        self.assertTrue(t.getValue() == SbVec4f(0,1,0,1), 
                        'setValue float sequence on SoSFVec4f failed')
        t.setValue(SbVec4f(1,0,0,1))
        self.assertTrue(t.getValue() == SbVec4f(1,0,0,1), 
                        'setValue SbVec4f on SoSFVec4f failed')
        s.setValue(t)
        self.assertTrue(t.getValue() == s.getValue(),
                        'setValue other SoSFVec4f on SoSFVec4f failed')

    def testSFImage(self):
        """check setValue for SoSFImage"""
        t = SoSFImage()
        s = SoSFImage()        
        t.setValue(SbVec2s(2,2), 1, "abcd")
        s.setValue(t)
        self.assertTrue(("abcd", SbVec2s(2,2), 1) == t.getValue() == s.getValue(), 
                        'setValue on SoSFImage failed')

    def testSFImage3(self):
        """check setValue for SoSFImage3"""
        t = SoSFImage3()
        s = SoSFImage3()        
        t.setValue(SbVec3s(2,2,2), 1, "abcdefgh")
        s.setValue(t)
        self.assertTrue(("abcdefgh", SbVec3s(2,2,2), 1) == t.getValue() == s.getValue(), 
                        'setValue on SoSFImage3 failed')

    def testSFPath(self):
        """check setValue for SoSFPath"""
        t = SoSFPath()
        s = SoSFPath()
        c = SoPath()
        c.ref()
        t.setValue(c)
        s.setValue(t)
        self.assertTrue(c == t.getValue() == s.getValue(), 
                        'setValue on SoSFPath failed')
        self.assertTrue(isinstance(t.getValue(), SoPath), 
                        'autocast on SoSFPath.getValue failed')

    def testSFPlane(self):
        """check setValue for SoSFPlane"""
        t = SoSFPlane()
        s = SoSFPlane()
        c = SbPlane()
        t.setValue(c)
        s.setValue(t)
        self.assertTrue(c == t.getValue() == s.getValue(), 
                        'setValue on SoSFPlane failed')

    def testSFUInt32(self):
        """check setValue for SoSFUInt32"""
        t = SoSFUInt32()
        s = SoSFUInt32()
        t.setValue(10)
        s.setValue(t)
        self.assertTrue(10 == t.getValue() == s.getValue(), 
                        'setValue on SoSFUInt32 failed')

    def testSFUShort(self):
        """check setValue for SoSFUShort"""
        t = SoSFUShort()
        s = SoSFUShort()
        t.setValue(10)
        s.setValue(t)
        self.assertTrue(10 == t.getValue() == s.getValue(), 
                        'setValue on SoSFUShort failed')

    def testSFTime(self):
        """check setValue for SoSFTime"""
        t = SoSFTime()
        s = SoSFTime()
        t.setValue(150.5)
        s.setValue(t)
        self.assertTrue(150.5 == t.getValue() == s.getValue(), 
                        'setValue on SoSFTime failed')

    def testMFBool(self):
        """check setValue(s) for SoMFBool"""
        t = SoMFBool()
        s = SoMFBool()
        t.setValues([0,1])
        self.assertTrue(t.getValues() == [0,1],
                        'setValues with sequence on SoMFBool failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,1,0,1],
                        'setValues with start and sequence on SoMFBool failed')
        t.setValues(0,1,[1,0])
        self.assertTrue(t.getValues() == [1,1,0,1],
                        'setValues with start, length and sequence on SoMFBool failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFBool on SoMFBool failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with bool on SoMFBool failed')

    def testMFColor(self):
        """check setValue(s) for SoMFColor"""
        t = SoMFColor()
        s = SoMFColor()
        t.setValues([[0,0,1]])
        t.setValues(1,[[0,0,1]])
        t.setValues(2,1,[[0,0,1],[1,0,1]])
        self.assertTrue(t.getValues() == [SbColor(0,0,1),SbColor(0,0,1),SbColor(0,0,1)],
                        'setValues with sequence of float triples on SoMFColor failed')                    
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with another SoMFColor failed' )
        t.setValues([SbColor(0,0,1)])
        t.setValues(1,[SbColor(0,0,1)])
        t.setValues(2,1,[SbColor(0,0,1), SbColor(1,0,1)])
        self.assertTrue(t.getValues() == [SbColor(0,0,1),SbColor(0,0,1),SbColor(0,0,1)],
                        'setValues with sequence of SbColors on SoMFColor failed')                    
        t.setValues([])
        self.assertTrue(t.getValues() == [SbColor(0,0,1),SbColor(0,0,1),SbColor(0,0,1)],
                        'setValue with empty sequence of SbColors failed' )

    def testMFEngine(self):
        """check setValue(s) for SoMFEngine"""
        t = SoMFEngine()
        s = SoMFEngine()
        c = SoCounter()
        c.ref()
        c2 = SoCounter()
        c2.ref()
        t.setValue(c)
        self.assertTrue(t.getValues() == [c], 
                        'setValue engine on SoMFEngine failed')
        t.setValues([c,c2])
        self.assertTrue(t.getValues() == [c,c2], 
                        'setValues on SoMFEngine failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFEngine on SoMFEngine failed')
        t.setValues(2,1,[c,c2])
        self.assertTrue(t.getValues() == [None,None,c],
                        'setValues with start, length and sequence on SoMFEngine failed')

    def testMFEnum(self):
        """check setValue(s) for SoMFEnum"""
        t = SoMFEnum()
        s = SoMFEnum()
        t.setValues([0,2])
        self.assertTrue(t.getValues() == [0,2],
                        'setValues with sequence on SoMFEnum failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,2,0,1],
                        'setValues with start and sequence on SoMFEnum failed')
        t.setValues(0,1,[-1,0])
        self.assertTrue(t.getValues() == [-1,2,0,1],
                        'setValues with start, length and sequence on SoMFEnum failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFEnum on SoMFEnum failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with single int on SoMFEnum failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [0],
                        'setValues with empty list on SoMFEnum failed')

    def testMFString(self):
        """check setValue(s) for SoMFString"""
        t = SoMFString()
        s = SoMFString()
        t.setValues(['1','2'])
        t.setValues(1,['3'])
        t.setValues(2,1,['4','5'])
        self.assertTrue(t.getValues() == ['1','3','4'],
                        'setValues with sequence of strings on SoMFString failed')                    
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with another SoMFString failed' )
        t.setValues([SbString('1')])
        t.setValues(1,[SbString('2')])
        t.setValues(2,1,[SbString('3'), SbString('4')])
        self.assertTrue(t.getValues() == ['1','2','3'],
                        'setValues with sequence of SbStrings on SoMFString failed')                    


    def testMFInt32(self):
        """check setValue(s) for SoMFInt32"""
        t = SoMFInt32()
        s = SoMFInt32()
        t.setValues([0,2])
        self.assertTrue(t.getValues() == [0,2],
                        'setValues with sequence on SoMFInt32 failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,2,0,1],
                        'setValues with start and sequence on SoMFInt32 failed')
        t.setValues(0,1,[-1,0])
        self.assertTrue(t.getValues() == [-1,2,0,1],
                        'setValues with start, length and sequence on SoMFInt32 failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFInt32 on SoMFInt32 failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with single int on SoMFInt32 failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [0],
                        'setValues with empty list on SoMFInt32 failed')

    def testMFFloat(self):
        """check setValue(s) for SoMFFloat"""
        t = SoMFFloat()
        s = SoMFFloat()
        t.setValues([0.5,2])
        self.assertTrue(t.getValues() == [0.5,2],
                        'setValues with sequence on SoMFFloat failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0.5,2,0,1],
                        'setValues with start and sequence on SoMFFloat failed')
        t.setValues(0,1,[1.5,0])
        self.assertTrue(t.getValues() == [1.5,2,0,1],
                        'setValues with start, length and sequence on SoMFFloat failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFFloat on SoMFFloat failed')
        t.setValue(-0.5)
        self.assertTrue(t.getValues() == [-0.5],
                        'setValue with single int on SoMFFloat failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [-0.5],
                        'setValues with empty list on SoMFFloat failed')     

    def testMFShort(self):
        """check setValue(s) for SoMFShort"""
        t = SoMFShort()
        s = SoMFShort()
        t.setValues([0,2])
        self.assertTrue(t.getValues() == [0,2],
                        'setValues with sequence on SoMFShort failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,2,0,1],
                        'setValues with start and sequence on SoMFShort failed')
        t.setValues(0,1,[-1,0])
        self.assertTrue(t.getValues() == [-1,2,0,1],
                        'setValues with start, length and sequence on SoMFShort failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFShort on SoMFShort failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with single int on SoMFShort failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [0],
                        'setValues with empty list on SoMFShort failed')
        
    def testMFMatrix(self):
        """check setValue(s) for SoMFMatrix"""
        t = SoMFMatrix()
        s = SoMFMatrix()
        m = SbMatrix([[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0]])
        m2 = SbMatrix([[1,1,1,1],[1,1,1,1],[1,1,1,1],[1,1,1,1]])
        t.setValues([m])
        self.assertTrue(t.getValues() == [m],
                        'setValues with sequence on SoMFMatrix failed')
        t.setValues(2,[m2,m2])
        self.assertTrue(t.getValues()[2:4] == [m2,m2],
                        'setValues with start and sequence on SoMFMatrix failed')
        t.setValues(1,1,[m2,m])
        self.assertTrue(t.getValues() == [m,m2,m2,m2],
                        'setValues with start, length and sequence on SoMFMatrix failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFMatrix on SoMFMatrix failed')
        t.setValue(m2)
        self.assertTrue(t.getValues() == [m2],
                        'setValue with single int on SoMFMatrix failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [m2],
                        'setValues with empty list on SoMFMatrix failed')                     

    def testMFName(self):
        """check setValue(s) for SoMFName"""
        t = SoMFName()
        s = SoMFName()
        t.setValues(['1','2'])
        t.setValues(1,['3'])
        t.setValues(2,1,['4','5'])
        self.assertTrue(t.getValues() == ['1','3','4'],
                        'setValues with sequence of Names on SoMFName failed')                    
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with another SoMFName failed' )
        t.setValues([SbName('1')])
        t.setValues(1,[SbName('2')])
        t.setValues(2,1,[SbName('3'), SbName('4')])
        self.assertTrue(t.getValues() == ['1','2','3'],
                        'setValues with sequence of SbNames on SoMFName failed')                    

    def testMFNode(self):
        """check setValue(s) for SoMFNode"""
        t = SoMFNode()
        s = SoMFNode()
        c = SoMaterial()
        c.ref()
        c2 = SoTransform()
        c2.ref()
        t.setValue(c)
        self.assertTrue(t.getValues() == [c], 
                        'setValue Node on SoMFNode failed')
        t.setValues([c,c2])
        self.assertTrue(t.getValues() == [c,c2], 
                        'setValues on SoMFNode failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFNode on SoMFNode failed')
        t.setValues(2,1,[c,c2])
        self.assertTrue(t.getValues() == [None,None,c],
                        'setValues with start, length and sequence on SoMFNode failed')         

    def testMFPath(self):
        """check setValue(s) for SoMFPath"""
        t = SoMFPath()
        s = SoMFPath()
        p = SoPath()
        p.ref()
        p2 = SoPath()
        p2.ref()
        t.setValue(p)
        self.assertTrue(t.getValues() == [p], 
                        'setValue Path on SoMFPath failed')
        t.setValues([p,p2])
        self.assertTrue(t.getValues() == [p,p2], 
                        'setValues on SoMFPath failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFPath on SoMFPath failed')
        t.setValues(2,1,[p,p2])
        self.assertTrue(t.getValues() == [None,None,p],
                        'setValues with start, length and sequence on SoMFPath failed')         

    def testMFPlane(self):
        """check setValue(s) for SoMFPlane"""
        t = SoMFPlane()
        s = SoMFPlane()
        m = SbPlane(SbVec3f(1,0,0),0)
        m2 = SbPlane(SbVec3f(0,1,0),1)
        t.setValues([m])
        self.assertEqual(t.getValues(), [m])
        t.setValues(2,[m2,m2])
        self.assertEqual(t.getValues()[2:4],[m2,m2])
        t.setValues(1,1,[m2,m])
        self.assertEqual(t.getValues(), [m,m2,m2,m2])
        t.setValue(s)
        self.assertEqual(t.getValues(), s.getValues())
        t.setValue(m2)
        self.assertEqual(t.getValues(), [m2])
        t.setValues([])
        self.assertEqual(t.getValues(), [m2]) # appears to be the correct behaviour as in Coin :/

    def testMFRotation(self):
        """check setValue(s) for SoMFRotation"""
        t = SoMFRotation()
        s = SoMFRotation()
        m = SbRotation(1,0,0,0)
        m2 = SbRotation(0,0,0,1)
        t.setValues([m])
        self.assertEqual(t.getValues(), [m])
        t.setValues(2,[m2,m2])
        self.assertEqual(t.getValues()[2:4],[m2,m2])
        t.setValues(1,1,[m2,m])
        self.assertEqual(t.getValues(), [m,m2,m2,m2])
        t.setValue(s)
        self.assertEqual(t.getValues(), s.getValues())
        t.setValue(m2)
        self.assertEqual(t.getValues(), [m2])
        t.setValue(1,0,0,0)
        self.assertEqual(t.getValues(), [m])
        t.setValue([0,0,0,1])
        self.assertEqual(t.getValues(), [m2])
        t.setValues([])
        self.assertEqual(t.getValues(), [m2]) # appears to be the correct behaviour as in Coin :/

    def testMFTime(self):
        """check setValue(s) for SoMFTime"""
        t = SoMFTime()
        s = SoMFTime()
        t.setValues([SbTime(0.5),SbTime(2)])
        self.assertTrue(t.getValues() == [SbTime(0.5),SbTime(2)],
                        'setValues with sequence on SoMFTime failed')
        t.setValues(2,[SbTime(0),SbTime(1)])
        self.assertTrue(t.getValues() == [SbTime(0.5),SbTime(2),SbTime(0),SbTime(1)],
                        'setValues with start and sequence on SoMFTime failed')
        t.setValues(0,1,[SbTime(1.5),SbTime(0)])
        self.assertTrue(t.getValues() == [SbTime(1.5),SbTime(2),SbTime(0),SbTime(1)],
                        'setValues with start, length and sequence on SoMFTime failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFTime on SoMFTime failed')
        t.setValue(SbTime(-0.5))
        self.assertTrue(t.getValues() == [SbTime(-0.5)],
                        'setValue with single int on SoMFTime failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [SbTime(-0.5)],
                        'setValues with empty list on SoMFTime failed') 

    def testMFUInt32(self):
        """check setValue(s) for SoMFUInt32"""
        t = SoMFUInt32()
        s = SoMFUInt32()
        t.setValues([0,2])
        self.assertTrue(t.getValues() == [0,2],
                        'setValues with sequence on SoMFUInt32 failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,2,0,1],
                        'setValues with start and sequence on SoMFUInt32 failed')
        t.setValues(0,1,[1,0])
        self.assertTrue(t.getValues() == [1,2,0,1],
                        'setValues with start, length and sequence on SoMFUInt32 failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFUInt32 on SoMFUInt32 failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with single int on SoMFUInt32 failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [0],
                        'setValues with empty list on SoMFUInt32 failed')

    def testMFUShort(self):
        """check setValue(s) for SoMFUShort"""
        t = SoMFUShort()
        s = SoMFUShort()
        t.setValues([0,2])
        self.assertTrue(t.getValues() == [0,2],
                        'setValues with sequence on SoMFUShort failed')
        t.setValues(2,[0,1])
        self.assertTrue(t.getValues() == [0,2,0,1],
                        'setValues with start and sequence on SoMFUShort failed')
        t.setValues(0,1,[1,0])
        self.assertTrue(t.getValues() == [1,2,0,1],
                        'setValues with start, length and sequence on SoMFUShort failed')
        t.setValue(s)
        self.assertTrue(t.getValues() == s.getValues(),
                        'setValue with other SoMFUShort on SoMFUShort failed')
        t.setValue(0)
        self.assertTrue(t.getValues() == [0],
                        'setValue with single int on SoMFUShort failed')
        t.setValues([])
        self.assertTrue(t.getValues() == [0],
                        'setValues with empty list on SoMFUShort failed')

    def testMFVec2f(self):
        """check setValue(s) for SoMFVec2f"""
        t = SoMFVec2f()
        s = SoMFVec2f()
        m = SbVec2f(1,0)
        m2 = SbVec2f(0,0)
        t.setValues([m])
        self.assertEqual(t.getValues(), [m])
        t.setValues(2,[m2,m2])
        self.assertEqual(t.getValues()[2:4],[m2,m2])
        t.setValues(1,1,[m2,m])
        self.assertEqual(t.getValues(), [m,m2,m2,m2])
        t.setValue(s)
        self.assertEqual(t.getValues(), s.getValues())
        t.setValue(m2)
        self.assertEqual(t.getValues(), [m2])
        t.setValues([])
        self.assertEqual(t.getValues(), [m2]) 
        t.setValues([[0,0],[1,0]])
        self.assertEqual(t.getValues(), [m2,m])
        t.setValue([0,0])
        self.assertEqual(t.getValues(), [m2])

    def testMFVec3f(self):
        """check setValue(s) for SoMFVec3f"""
        t = SoMFVec3f()
        s = SoMFVec3f()
        m = SbVec3f(1,0,0)
        m2 = SbVec3f(0,0,1)
        t.setValues([m])
        self.assertEqual(t.getValues(), [m])
        t.setValues(2,[m2,m2])
        self.assertEqual(t.getValues()[2:4],[m2,m2])
        t.setValues(1,1,[m2,m])
        self.assertEqual(t.getValues(), [m,m2,m2,m2])
        t.setValue(s)
        self.assertEqual(t.getValues(), s.getValues())
        t.setValue(m2)
        self.assertEqual(t.getValues(), [m2])
        t.setValues([])
        self.assertEqual(t.getValues(), [m2]) 
        t.setValues([[0,0,1],[1,0,0]])
        self.assertEqual(t.getValues(), [m2,m])
        t.setValue([0,0,1])
        self.assertEqual(t.getValues(), [m2])       

    def testMFVec4f(self):
        """check setValue(s) for SoMFVec4f"""
        t = SoMFVec4f()
        s = SoMFVec4f()
        m = SbVec4f(1,0,0,0)
        m2 = SbVec4f(0,0,1,0)
        t.setValues([m])
        self.assertEqual(t.getValues(), [m])
        t.setValues(2,[m2,m2])
        self.assertEqual(t.getValues()[2:4],[m2,m2])
        t.setValues(1,1,[m2,m])
        self.assertEqual(t.getValues(), [m,m2,m2,m2])
        t.setValue(s)
        self.assertEqual(t.getValues(), s.getValues())
        t.setValue(m2)
        self.assertEqual(t.getValues(), [m2])
        t.setValues([])
        self.assertEqual(t.getValues(), [m2]) 
        t.setValues([[0,0,1,0],[1,0,0,0]])
        self.assertEqual(t.getValues(), [m2,m])
        t.setValue([0,0,1,0])
        self.assertEqual(t.getValues(), [m2])        


class SbStringMethods(unittest.TestCase):
    """tests various string stuff"""
    def testCharConstructor(self):
        """check SbString(char *)"""
        s = SbString('hello')
        self.assertEqual(s.getString(), 'hello')

    def testCharLimits(self):
        """check SbString(char *, int begin, int end)"""
        s = SbString('hello', 1, 3)
        self.assertEqual(s.getString(), 'ell')

    def testOtherString(self):
        """check SbString(SbString)"""
        s = SbString('hello')
        t = SbString(s)
        self.assertEqual(s.getString(), t.getString())

    def testDigits(self):
        """check SbString(int)"""
        s = SbString(100)
        self.assertEqual(s.getString(), '100' )

class SbNameMethods(unittest.TestCase):
    """tests various Name stuff"""
    def testCharConstructor(self):
        """check SbName(char *)"""
        s = SbName('hello')
        self.assertEqual(s.getString(), 'hello')

    def testOtherName(self):
        """check SbName(SbName)"""
        s = SbName('hello')
        t = SbName(s)
        self.assertEqual(s.getString(), t.getString())

    def testOtherString(self):
        """check SbName(SbString)"""
        s = SbString('hello')
        t = SbName(s)
        self.assertEqual(s.getString(), t.getString())

    def testFromName(self):
        """check passing strings instead of SbName to SoType.fromName()"""
        t = SoType.fromName("SoCone")

class SbRotationMethods(unittest.TestCase):
    "tests various Rotation stuff"""
    def testAxisAngleCons(self):
        """check SbRotation(SbVector, float)"""
        r = SbRotation(SbVec3f(1,0,0), 3.14159264)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,-4.3711388286737929e-008))

    def testFloat4Cons(self):
        """check SbRotation(float[4])"""
        r = SbRotation((1.0,0.0,0.0,0.0))
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    def testFloatsCons(self):
        """check SbRotation(float,float,float,float)"""
        r = SbRotation(1.0,0.0,0.0,0.0)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    def testMatrixCons(self):
        """check SbRotation(SbMatrix)"""
        m = SbMatrix()
        m.setRotate(SbRotation(1.0,0.0,0.0,0.0))
        r = SbRotation(m)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    # DISABLED until correctness of recently introduced precision
    # divergence in Coin is properly investigated. 20100426 tamer.
    # def testVecFTCons(self):
    #     """check SbRotation(SbVec3f, SbVec3f)"""
    #     r = SbRotation(SbVec3f(1,0,0), SbVec3f(0,1,0))
    #     r = r*r
    #     self.assertEqual(r.getValue(), (0.0,0.0,1.0,0.0))

    def testAxisAngleSet(self):
        """check SbRotation.setValue(SbVector, float)"""
        r = SbRotation()
        r.setValue(SbVec3f(1,0,0), 3.14159264)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,-4.3711388286737929e-008))

    def testFloat4Set(self):
        """check SbRotation.setValue(float[4])"""
        r = SbRotation()
        r.setValue(1.0,0.0,0.0,0.0)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    def testFloatsSet(self):
        """check SbRotation.setValue(float,float,float,float)"""
        r = SbRotation()
        r.setValue(1.0,0.0,0.0,0.0)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    def testMatrixSet(self):
        """check SbRotation.setValue(SbMatrix)"""
        m = SbMatrix()
        m.setRotate(SbRotation(1.0,0.0,0.0,0.0))
        r = SbRotation()
        r.setValue(m)
        self.assertEqual(r.getValue(), (1.0,0.0,0.0,0.0))

    # DISABLED until correctness of recently introduced precision
    # divergence in Coin is properly investigated. 20100426 tamer.
    # def testVecFTSet(self):
    #     """check SbRotation.setValue(SbVec3f, SbVec3f)"""
    #     r = SbRotation()
    #     r.setValue(SbVec3f(1,0,0), SbVec3f(0,1,0))
    #     r = r*r
    #     self.assertEqual(r.getValue(), (0.0,0.0,1.0,0.0))

class SoBaseListMethods(unittest.TestCase):
    """tests various methods of SoBaseList"""
    def testGet(self):
        """check SoBaseList.get downcast type return"""
        l = SoBaseList()
        c = SoCone()
        l.append(c)
        self.assertTrue(isinstance(l.get(0), SoCone))

    def testItem(self):
        """check SoBaseList[] downcast type return"""
        l = SoBaseList()
        c = SoCone()
        l.append(c)
        self.assertTrue(isinstance(l[0], SoCone))

    def testSetItem(self):
        """check SoBaseList [0] = value support"""
        l = SoBaseList()
        f = SoCone()
        l.append(None) # note, that the set method underlying access does not expand!
        l[0] = f
        self.assertTrue(l[0].this == f.this)

    def testIteration(self):
        """check iteration support by all lists derived from SbPList"""
        l = SoBaseList()
        c = SoCone()
        l.append(c)
        for x in l:
            self.assertTrue(x.this == c.this)

class SoNodeListMethods(unittest.TestCase):
    """tests various methods of SoNodeList"""
    def testGet(self):
        """check SoNodeList.get() downcast type return"""
        l = SoNodeList()
        c = SoCone()
        l.append(c)
        self.assertTrue(isinstance(l.get(0), SoCone))

    def testItem(self):
        """check SoNodeList[] downcast type return"""
        l = SoNodeList()
        c = SoCone()
        l.append(c)
        self.assertTrue(isinstance(l[0], SoCone))

    def testSetItem(self):
        """check SoNodeList [0] = value support"""
        l = SoNodeList()
        c = SoCone()
        l.append(None) # note, that the set method underlying access does not expand!
        l[0] = c
        self.assertTrue(l[0].this == c.this)

class SoFieldListMethods(unittest.TestCase):
    """tests various methods of SoFieldList"""
    def testGet(self):
        """check SoFieldList.get() downcast type return"""
        l = SoFieldList()
        f = SoSFBool()
        l.append(f)
        self.assertTrue(isinstance(l.get(0), SoSFBool))

    def testItem(self):
        """check SoFieldList[] downcast type return"""
        l = SoFieldList()
        f = SoSFBool()
        l.append(f)
        self.assertTrue(isinstance(l[0], SoSFBool))

    def testSetItem(self):
        """check SoFieldList [0] = value support"""
        l = SoFieldList()
        f = SoSFBool()
        l[0] = f
        self.assertTrue(l[0] == f)

class FieldAssignment(unittest.TestCase):
    """tests field assignment for all kinds of types"""
    def testAssign(self):
        """check field assignment for all kinds of types"""
        n = SoCone()
        n.bottomRadius = 10
        self.assertEqual(n.bottomRadius.getValue(), 10 )
        n = SoInfo()
        n.string = "hello"
        self.assertEqual(n.string.getValue(), "hello" )

class SoFieldMethods(unittest.TestCase):
    """test SoField methods"""
    def testGet(self):
        """check SoField.get() ASCII field's value return"""
        s = SoSFVec3f()
        s.setValue(1,2,3)
        self.assertTrue(s.get() == "1 2 3",
                        'SoField.get() failed')
                        
    def testLen(self):
        """check len() for SoMFields"""
        s = SoMFInt32()
        self.assertTrue(len(s) == 0 == s.getNum(),
                        'len(s) on empty field failed')
        s.setValues([1,2,3])
        self.assertTrue(len(s) == 3 == s.getNum(),
                        'len(s) on non-empty field failed')

    def testGet1(self):
        """check get1() for SoMFields"""
        s = SoMFInt32()
        s.setValues([1,2,3])
        self.assertTrue(s.get1(1) == '2', 'get1(1) failed')
        
class SbTimeMethods(unittest.TestCase):
    """test SbTime methods"""
    def testConstructors(self):
        """check SbTime constructors"""
        t1 = SbTime()
        t2 = SbTime(12.005)
        t3 = SbTime(12, 5000)
        self.assertEqual( t2, t3 )
        
    def testSetValue(self):
        """check SbTime setValue methods"""
        t1 = SbTime(12.005)
        t2 = SbTime()
        t2.setValue(12.005)
        self.assertEqual(t1, t2)
        t2.setValue(12, 5000)
        self.assertEqual(t1, t2)

class OperatorTests(unittest.TestCase):
    """checks various operator overloaded methods"""
    def testEqNone(self):
        """check __eq__ operator None comparison"""
        self.assertTrue(not (SoSeparator() == None))
        
    def testNqNone(self):
        """check __nq__ operator None comparison"""
        self.assertTrue((SoSeparator() != None))
        
class InitTests(unittest.TestCase):
    """checks if all node packages are initialized at startup"""
    def testInitNodeKits(self):
        """check if SoNodeKit::init was called"""
        self.assertTrue(SoShapeKit() is not None)

    def testInitInteraction(self):
        """check if interaction nodes (draggers etc) are initialized at startup"""
        self.assertTrue(SoAntiSquish() is not None)

class NodeKitTests(unittest.TestCase):
    """checks API of SoBaseKit and related stuff"""
    def testRecursiveParts(self):
        """check if . returns a part not a nodefield"""
        s = SoShapeKit()
        self.assertTrue(isinstance(s.appearance.material, SoMaterial))

class SoGroupMethods(unittest.TestCase):
    """checks methods and operators for SoGroup"""
    def testSequence(self):
        """check SoGroup sequence operator"""
        g = SoGroup()
        c = SoCone()
        g.addChild(c)
        self.assertEqual(len(g), 1)
        for x in g:
            self.assertEqual(x, c)

    def testContains(self):
        """check SoGroup containment operator in"""
        g = SoGroup()
        c = SoCone()
        c2 = SoCone()
        g.addChild(c)
        self.assertTrue(c in g)
        self.assertTrue(c2 not in g)

    def testAccess(self):
        """check SoGroup array access operator []"""
        g = SoGroup()
        c = SoCone()
        c2 = SoCone()
        g.addChild(c)
        g.addChild(c2)
        self.assertEqual(g[0], c)
        self.assertEqual(g[1], c2)
        try:
            g[2]
        except IndexError as e:
            test_sucessfully = True

    def testIAdd(self):
        g = SoGroup()
        c = SoCone()
        d = SoCube()
        g += [c, [g]]

class SbBaseClasses(unittest.TestCase):
    """checks methods and operators of and between various Sb* classes"""
    def testConstructors(self):
        v1 = SbVec3f()
        v2 = SbVec3f(1,2,3)
        v3 = SbVec3f([1,2,3])
        v4 = SbVec3f(v2)

        v1 = SbVec4f()
        v2 = SbVec4f(1,2,3,4)
        v3 = SbVec4f([1,2,3,4])
        v4 = SbVec4f(v2)

        v1 = SbVec3d()
        v2 = SbVec3d(1,2,3)
        v3 = SbVec3d([1,2,3])
        v4 = SbVec3d(v2)

        v1 = SbVec4d()
        v2 = SbVec4d(1,2,3,4)
        v3 = SbVec4d([1,2,3,4])
        v4 = SbVec4d(v2)

        m0 = SbMatrix()
        m1 = SbMatrix([[0,1,0,0], [0,0,1,0], [1,0,0,0], [0,0,0,1]])
        m2 = SbMatrix(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
        # the following is not part of Coin API, should it be supported ?
        #m3 = SbMatrix(m1)

        m0 = SbDPMatrix()
        m1 = SbDPMatrix([[0,1,0,0], [0,0,1,0], [1,0,0,0], [0,0,0,1]])
        m2 = SbDPMatrix(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
        # the following treats the SbMatrix as a sequence!
        #m3 = SbDPMatrix(SbMatrix())

    def testVecMatrixOperators(self):
        """tests operators between vec and matrix classes"""
        v = SbVec3f(1,0,0)
        m = SbMatrix([[0,1,0,0], [0,0,1,0], [1,0,0,0], [0,0,0,1]])
        self.assertEqual(m * v, SbVec3f(0,0,1))
        self.assertEqual(v * m, SbVec3f(0,1,0))
        v4 = SbVec4f(1,0,0,1)
        self.assertEqual(v4 * m, SbVec4f(0,1,0,1))

    def testVectorAsArrays(self):
        """tests various container operators of vectors"""

        d = {}
        d[2] = [1,2]
        d[3] = [1, 2, 3]
        d[4] = [1.0, 2.0, 3.0, 4.0]
        
        vecs = [SbVec2s(d[2]), SbVec2f(d[2]), SbVec2d(d[2]),
                SbVec3s(d[3]), SbVec3f(d[3]), SbVec3d(d[3]),
                SbVec4f(d[4]), SbVec4d(d[4])]

        for i,v in zip([2,2,2,3,3,3,4,4], vecs):
            self.assertEqual(i, len(v))
            self.assertEqual([x for x in v], d[i] )
            for j in range(i):
                self.assertEqual(d[i][j], v[j])

    def testVecdDBMatrixOperators(self):
        """tests operators between vec and matrix classes, double precision"""
        v = SbVec3d(1,0,0)
        m = SbDPMatrix([[0,1,0,0], [0,0,1,0], [1,0,0,0], [0,0,0,1]])
        self.assertEqual(m * v, SbVec3d(0,0,1))
        self.assertEqual(v * m, SbVec3d(0,1,0))
        # fails due to a bug in Coin3D, hopefully will be fixed soon
        v4 = SbVec4d(1,0,0,1)
        self.assertEqual(v4 * m, SbVec4d(0,1,0,1), 'bug in Coin3D, left in to check')

class SoPathTests(unittest.TestCase):
    """checks various methods for SoPath"""

    def setUp(self):
        self.nodes = [ SoSeparator() ]
        for i in range(3):
            child = SoGroup()
            self.nodes[-1].addChild(child)
            self.nodes.append(child)

    def tearDown(self):
        del self.nodes

    def testSoPathIterator(self):
        """check iterator in SoPath"""
        path = SoPath()
        for n in self.nodes:
            path.append(n)
        index = 0
        for n in path:
            self.assertTrue(n == self.nodes[index])
            index += 1
        for i in path.index():
            self.assertEqual(i, 0)

class SoInputTests(unittest.TestCase):
    """check method SoInput.setBuffer"""

    def setUp(self):
        self.text = "Separator {}"
        self.input = SoInput()

    def testSetBuffer(self):
        self.input.setBuffer(self.text)


class SbVecTests(unittest.TestCase):
    def setUp(self):
        self.sbvec3f = coin.SbVec3f(1, 1, 1)

    def multDivTest(self):
        self.assertEqual(self.sbvec3f, self.sbvec3f * 2. / 2.)
        # TODO: also support ints

class SbPLaneTests(unittest.TestCase):

    def setUp(self):
        self.a = coin.SbVec3f([0, 0, 0])
        self.b = coin.SbVec3f([0, 0, 1])
        self.c = coin.SbVec3f([0, 1, 0])
        self.line = coin.SbLine(a, b)
        self.plane1 = coin.SbPlane(b, a)
        self.plane2 = coin.SbPlane(c, a)

    def intersection(self):
        point = self.plane1.intersect(self.line)
        line = self.plane1.intersect(self.plane2)

if __name__ == "__main__":
    unittest.main(verbosity=4)