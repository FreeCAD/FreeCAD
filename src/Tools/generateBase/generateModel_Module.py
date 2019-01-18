#!/usr/bin/env python

#
# Generated Mon Aug 21 13:00:21 2017 by generateDS.py.
# Update it with: python generateDS.py -o generateModel_Module.py generateMetaModel_Module.xsd
#
# WARNING! All changes made in this file will be lost!
#

from __future__ import print_function # this allows py2 to print(str1,str2) correctly

import sys
import getopt
from xml.dom import minidom
from xml.dom import Node

#
# If you have installed IPython you can uncomment and use the following.
# IPython is available from http://ipython.scipy.org/.
#

## from IPython.Shell import IPShellEmbed
## args = ''
## ipshell = IPShellEmbed(args,
##     banner = 'Dropping into IPython',
##     exit_msg = 'Leaving Interpreter, back to program.')

# Then use the following line where and when you want to drop into the
# IPython shell:
#    ipshell('<some message> -- Entering ipshell.\nHit Ctrl-D to exit')

#
# Support/utility functions.
#

def showIndent(outfile, level):
    for idx in range(level):
        outfile.write('    ')

def quote_xml(inStr):
    s1 = inStr
    s1 = s1.replace('&', '&amp;')
    s1 = s1.replace('<', '&lt;')
    s1 = s1.replace('"', '&quot;')
    return s1

def quote_python(inStr):
    s1 = inStr
    if s1.find("'") == -1:
        if s1.find('\n') == -1:
            return "'%s'" % s1
        else:
            return "'''%s'''" % s1
    else:
        if s1.find('"') != -1:
            s1 = s1.replace('"', '\\"')
        if s1.find('\n') == -1:
            return '"%s"' % s1
        else:
            return '"""%s"""' % s1


class MixedContainer:
    # Constants for category:
    CategoryNone = 0
    CategoryText = 1
    CategorySimple = 2
    CategoryComplex = 3
    # Constants for content_type:
    TypeNone = 0
    TypeText = 1
    TypeString = 2
    TypeInteger = 3
    TypeFloat = 4
    TypeDecimal = 5
    TypeDouble = 6
    TypeBoolean = 7
    def __init__(self, category, content_type, name, value):
        self.category = category
        self.content_type = content_type
        self.name = name
        self.value = value
    def getCategory(self):
        return self.category
    def getContenttype(self, content_type):
        return self.content_type
    def getValue(self):
        return self.value
    def getName(self):
        return self.name
    def export(self, outfile, level, name):
        if self.category == MixedContainer.CategoryText:
            outfile.write(self.value)
        elif self.category == MixedContainer.CategorySimple:
            self.exportSimple(outfile, level, name)
        else:    # category == MixedContainer.CategoryComplex
            self.value.export(outfile, level, name)
    def exportSimple(self, outfile, level, name):
        if self.content_type == MixedContainer.TypeString:
            outfile.write('<%s>%s</%s>' % (self.name, self.value, self.name))
        elif self.content_type == MixedContainer.TypeInteger or \
                self.content_type == MixedContainer.TypeBoolean:
            outfile.write('<%s>%d</%s>' % (self.name, self.value, self.name))
        elif self.content_type == MixedContainer.TypeFloat or \
                self.content_type == MixedContainer.TypeDecimal:
            outfile.write('<%s>%f</%s>' % (self.name, self.value, self.name))
        elif self.content_type == MixedContainer.TypeDouble:
            outfile.write('<%s>%g</%s>' % (self.name, self.value, self.name))
    def exportLiteral(self, outfile, level, name):
        if self.category == MixedContainer.CategoryText:
            showIndent(outfile, level)
            outfile.write('MixedContainer(%d, %d, "%s", "%s"),\n' % \
                (self.category, self.content_type, self.name, self.value))
        elif self.category == MixedContainer.CategorySimple:
            showIndent(outfile, level)
            outfile.write('MixedContainer(%d, %d, "%s", "%s"),\n' % \
                (self.category, self.content_type, self.name, self.value))
        else:    # category == MixedContainer.CategoryComplex
            showIndent(outfile, level)
            outfile.write('MixedContainer(%d, %d, "%s",\n' % \
                (self.category, self.content_type, self.name,))
            self.value.exportLiteral(outfile, level + 1)
            showIndent(outfile, level)
            outfile.write(')\n')


#
# Data representation classes.
#

class GenerateModel:
    subclass = None
    def __init__(self, Module=None, PythonExport=None):
        if Module is None:
            self.Module = []
        else:
            self.Module = Module
        if PythonExport is None:
            self.PythonExport = []
        else:
            self.PythonExport = PythonExport
    def factory(*args_, **kwargs_):
        if GenerateModel.subclass:
            return GenerateModel.subclass(*args_, **kwargs_)
        else:
            return GenerateModel(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getModule(self): return self.Module
    def setModule(self, Module): self.Module = Module
    def addModule(self, value): self.Module.append(value)
    def insertModule(self, index, value): self.Module[index] = value
    def getPythonexport(self): return self.PythonExport
    def setPythonexport(self, PythonExport): self.PythonExport = PythonExport
    def addPythonexport(self, value): self.PythonExport.append(value)
    def insertPythonexport(self, index, value): self.PythonExport[index] = value
    def export(self, outfile, level, name_='GenerateModel'):
        showIndent(outfile, level)
        outfile.write('<%s>\n' % name_)
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='GenerateModel'):
        pass
    def exportChildren(self, outfile, level, name_='GenerateModel'):
        for Module_ in self.getModule():
            Module_.export(outfile, level)
        for PythonExport_ in self.getPythonexport():
            PythonExport_.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='GenerateModel'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        pass
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Module=[\n')
        level += 1
        for Module in self.Module:
            showIndent(outfile, level)
            outfile.write('Module(\n')
            Module.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('PythonExport=[\n')
        level += 1
        for PythonExport in self.PythonExport:
            showIndent(outfile, level)
            outfile.write('PythonExport(\n')
            PythonExport.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        pass
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Module':
            obj_ = Module.factory()
            obj_.build(child_)
            self.Module.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'PythonExport':
            obj_ = PythonExport.factory()
            obj_.build(child_)
            self.PythonExport.append(obj_)
# end class GenerateModel


class PythonExport:
    subclass = None
    def __init__(self, FatherNamespace='', DisableNotify=0, RichCompare=0, Name='', Reference=0, FatherInclude='', Namespace='', Initialization=0, Father='', PythonName='', Twin='', Constructor=0, TwinPointer='', Include='', NumberProtocol=0, Delete=0, Documentation=None, Methode=None, Attribute=None, Sequence=None, CustomAttributes='', ClassDeclarations=''):
        self.FatherNamespace = FatherNamespace
        self.DisableNotify = DisableNotify
        self.RichCompare = RichCompare
        self.Name = Name
        self.Reference = Reference
        self.FatherInclude = FatherInclude
        self.Namespace = Namespace
        self.Initialization = Initialization
        self.Father = Father
        self.PythonName = PythonName
        self.Twin = Twin
        self.Constructor = Constructor
        self.TwinPointer = TwinPointer
        self.Include = Include
        self.NumberProtocol = NumberProtocol
        self.Delete = Delete
        self.Documentation = Documentation
        if Methode is None:
            self.Methode = []
        else:
            self.Methode = Methode
        if Attribute is None:
            self.Attribute = []
        else:
            self.Attribute = Attribute
        self.Sequence = Sequence
        self.CustomAttributes = CustomAttributes
        self.ClassDeclarations = ClassDeclarations
    def factory(*args_, **kwargs_):
        if PythonExport.subclass:
            return PythonExport.subclass(*args_, **kwargs_)
        else:
            return PythonExport(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getMethode(self): return self.Methode
    def setMethode(self, Methode): self.Methode = Methode
    def addMethode(self, value): self.Methode.append(value)
    def insertMethode(self, index, value): self.Methode[index] = value
    def getAttribute(self): return self.Attribute
    def setAttribute(self, Attribute): self.Attribute = Attribute
    def addAttribute(self, value): self.Attribute.append(value)
    def insertAttribute(self, index, value): self.Attribute[index] = value
    def getSequence(self): return self.Sequence
    def setSequence(self, Sequence): self.Sequence = Sequence
    def getCustomattributes(self): return self.CustomAttributes
    def setCustomattributes(self, CustomAttributes): self.CustomAttributes = CustomAttributes
    def getClassdeclarations(self): return self.ClassDeclarations
    def setClassdeclarations(self, ClassDeclarations): self.ClassDeclarations = ClassDeclarations
    def getFathernamespace(self): return self.FatherNamespace
    def setFathernamespace(self, FatherNamespace): self.FatherNamespace = FatherNamespace
    def getDisablenotify(self): return self.DisableNotify
    def setDisablenotify(self, DisableNotify): self.DisableNotify = DisableNotify
    def getRichcompare(self): return self.RichCompare
    def setRichcompare(self, RichCompare): self.RichCompare = RichCompare
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def getReference(self): return self.Reference
    def setReference(self, Reference): self.Reference = Reference
    def getFatherinclude(self): return self.FatherInclude
    def setFatherinclude(self, FatherInclude): self.FatherInclude = FatherInclude
    def getNamespace(self): return self.Namespace
    def setNamespace(self, Namespace): self.Namespace = Namespace
    def getInitialization(self): return self.Initialization
    def setInitialization(self, Initialization): self.Initialization = Initialization
    def getFather(self): return self.Father
    def setFather(self, Father): self.Father = Father
    def getPythonname(self): return self.PythonName
    def setPythonname(self, PythonName): self.PythonName = PythonName
    def getTwin(self): return self.Twin
    def setTwin(self, Twin): self.Twin = Twin
    def getConstructor(self): return self.Constructor
    def setConstructor(self, Constructor): self.Constructor = Constructor
    def getTwinpointer(self): return self.TwinPointer
    def setTwinpointer(self, TwinPointer): self.TwinPointer = TwinPointer
    def getInclude(self): return self.Include
    def setInclude(self, Include): self.Include = Include
    def getNumberprotocol(self): return self.NumberProtocol
    def setNumberprotocol(self, NumberProtocol): self.NumberProtocol = NumberProtocol
    def getDelete(self): return self.Delete
    def setDelete(self, Delete): self.Delete = Delete
    def export(self, outfile, level, name_='PythonExport'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='PythonExport')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='PythonExport'):
        outfile.write(' FatherNamespace="%s"' % (self.getFathernamespace(), ))
        if self.getDisablenotify() is not None:
            outfile.write(' DisableNotify="%s"' % (self.getDisablenotify(), ))
        if self.getRichcompare() is not None:
            outfile.write(' RichCompare="%s"' % (self.getRichcompare(), ))
        outfile.write(' Name="%s"' % (self.getName(), ))
        if self.getReference() is not None:
            outfile.write(' Reference="%s"' % (self.getReference(), ))
        outfile.write(' FatherInclude="%s"' % (self.getFatherinclude(), ))
        outfile.write(' Namespace="%s"' % (self.getNamespace(), ))
        if self.getInitialization() is not None:
            outfile.write(' Initialization="%s"' % (self.getInitialization(), ))
        outfile.write(' Father="%s"' % (self.getFather(), ))
        if self.getPythonname() is not None:
            outfile.write(' PythonName="%s"' % (self.getPythonname(), ))
        outfile.write(' Twin="%s"' % (self.getTwin(), ))
        if self.getConstructor() is not None:
            outfile.write(' Constructor="%s"' % (self.getConstructor(), ))
        outfile.write(' TwinPointer="%s"' % (self.getTwinpointer(), ))
        outfile.write(' Include="%s"' % (self.getInclude(), ))
        if self.getNumberprotocol() is not None:
            outfile.write(' NumberProtocol="%s"' % (self.getNumberprotocol(), ))
        if self.getDelete() is not None:
            outfile.write(' Delete="%s"' % (self.getDelete(), ))
    def exportChildren(self, outfile, level, name_='PythonExport'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        for Methode_ in self.getMethode():
            Methode_.export(outfile, level)
        for Attribute_ in self.getAttribute():
            Attribute_.export(outfile, level)
        if self.Sequence:
            self.Sequence.export(outfile, level)
        showIndent(outfile, level)
        outfile.write('<CustomAttributes>%s</CustomAttributes>\n' % quote_xml(self.getCustomattributes()))
        showIndent(outfile, level)
        outfile.write('<ClassDeclarations>%s</ClassDeclarations>\n' % quote_xml(self.getClassdeclarations()))
    def exportLiteral(self, outfile, level, name_='PythonExport'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('FatherNamespace = "%s",\n' % (self.getFathernamespace(),))
        showIndent(outfile, level)
        outfile.write('DisableNotify = "%s",\n' % (self.getDisablenotify(),))
        showIndent(outfile, level)
        outfile.write('RichCompare = "%s",\n' % (self.getRichcompare(),))
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
        showIndent(outfile, level)
        outfile.write('Reference = "%s",\n' % (self.getReference(),))
        showIndent(outfile, level)
        outfile.write('FatherInclude = "%s",\n' % (self.getFatherinclude(),))
        showIndent(outfile, level)
        outfile.write('Namespace = "%s",\n' % (self.getNamespace(),))
        showIndent(outfile, level)
        outfile.write('Initialization = "%s",\n' % (self.getInitialization(),))
        showIndent(outfile, level)
        outfile.write('Father = "%s",\n' % (self.getFather(),))
        showIndent(outfile, level)
        outfile.write('PythonName = "%s",\n' % (self.getPythonname(),))
        showIndent(outfile, level)
        outfile.write('Twin = "%s",\n' % (self.getTwin(),))
        showIndent(outfile, level)
        outfile.write('Constructor = "%s",\n' % (self.getConstructor(),))
        showIndent(outfile, level)
        outfile.write('TwinPointer = "%s",\n' % (self.getTwinpointer(),))
        showIndent(outfile, level)
        outfile.write('Include = "%s",\n' % (self.getInclude(),))
        showIndent(outfile, level)
        outfile.write('NumberProtocol = "%s",\n' % (self.getNumberprotocol(),))
        showIndent(outfile, level)
        outfile.write('Delete = "%s",\n' % (self.getDelete(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('Methode=[\n')
        level += 1
        for Methode in self.Methode:
            showIndent(outfile, level)
            outfile.write('Methode(\n')
            Methode.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('Attribute=[\n')
        level += 1
        for Attribute in self.Attribute:
            showIndent(outfile, level)
            outfile.write('Attribute(\n')
            Attribute.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        if self.Sequence:
            showIndent(outfile, level)
            outfile.write('Sequence=Sequence(\n')
            self.Sequence.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('CustomAttributes=%s,\n' % quote_python(self.getCustomattributes()))
        showIndent(outfile, level)
        outfile.write('ClassDeclarations=%s,\n' % quote_python(self.getClassdeclarations()))
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('FatherNamespace'):
            self.FatherNamespace = attrs.get('FatherNamespace').value
        if attrs.get('DisableNotify'):
            if attrs.get('DisableNotify').value in ('true', '1'):
                self.DisableNotify = 1
            elif attrs.get('DisableNotify').value in ('false', '0'):
                self.DisableNotify = 0
            else:
                raise ValueError('Bad boolean attribute (DisableNotify)')
        if attrs.get('RichCompare'):
            if attrs.get('RichCompare').value in ('true', '1'):
                self.RichCompare = 1
            elif attrs.get('RichCompare').value in ('false', '0'):
                self.RichCompare = 0
            else:
                raise ValueError('Bad boolean attribute (RichCompare)')
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
        if attrs.get('Reference'):
            if attrs.get('Reference').value in ('true', '1'):
                self.Reference = 1
            elif attrs.get('Reference').value in ('false', '0'):
                self.Reference = 0
            else:
                raise ValueError('Bad boolean attribute (Reference)')
        if attrs.get('FatherInclude'):
            self.FatherInclude = attrs.get('FatherInclude').value
        if attrs.get('Namespace'):
            self.Namespace = attrs.get('Namespace').value
        if attrs.get('Initialization'):
            if attrs.get('Initialization').value in ('true', '1'):
                self.Initialization = 1
            elif attrs.get('Initialization').value in ('false', '0'):
                self.Initialization = 0
            else:
                raise ValueError('Bad boolean attribute (Initialization)')
        if attrs.get('Father'):
            self.Father = attrs.get('Father').value
        if attrs.get('PythonName'):
            self.PythonName = attrs.get('PythonName').value
        if attrs.get('Twin'):
            self.Twin = attrs.get('Twin').value
        if attrs.get('Constructor'):
            if attrs.get('Constructor').value in ('true', '1'):
                self.Constructor = 1
            elif attrs.get('Constructor').value in ('false', '0'):
                self.Constructor = 0
            else:
                raise ValueError('Bad boolean attribute (Constructor)')
        if attrs.get('TwinPointer'):
            self.TwinPointer = attrs.get('TwinPointer').value
        if attrs.get('Include'):
            self.Include = attrs.get('Include').value
        if attrs.get('NumberProtocol'):
            if attrs.get('NumberProtocol').value in ('true', '1'):
                self.NumberProtocol = 1
            elif attrs.get('NumberProtocol').value in ('false', '0'):
                self.NumberProtocol = 0
            else:
                raise ValueError('Bad boolean attribute (NumberProtocol)')
        if attrs.get('Delete'):
            if attrs.get('Delete').value in ('true', '1'):
                self.Delete = 1
            elif attrs.get('Delete').value in ('false', '0'):
                self.Delete = 0
            else:
                raise ValueError('Bad boolean attribute (Delete)')
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Methode':
            obj_ = Methode.factory()
            obj_.build(child_)
            self.Methode.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Attribute':
            obj_ = Attribute.factory()
            obj_.build(child_)
            self.Attribute.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Sequence':
            obj_ = Sequence.factory()
            obj_.build(child_)
            self.setSequence(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'CustomAttributes':
            CustomAttributes_ = ''
            for text__content_ in child_.childNodes:
                CustomAttributes_ += text__content_.nodeValue
            self.CustomAttributes = CustomAttributes_
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'ClassDeclarations':
            ClassDeclarations_ = ''
            for text__content_ in child_.childNodes:
                ClassDeclarations_ += text__content_.nodeValue
            self.ClassDeclarations = ClassDeclarations_
# end class PythonExport


class Methode:
    subclass = None
    def __init__(self, Const=0, Name='', Keyword=0, Documentation=None, Parameter=None):
        self.Const = Const
        self.Name = Name
        self.Keyword = Keyword
        self.Documentation = Documentation
        if Parameter is None:
            self.Parameter = []
        else:
            self.Parameter = Parameter
    def factory(*args_, **kwargs_):
        if Methode.subclass:
            return Methode.subclass(*args_, **kwargs_)
        else:
            return Methode(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getParameter(self): return self.Parameter
    def setParameter(self, Parameter): self.Parameter = Parameter
    def addParameter(self, value): self.Parameter.append(value)
    def insertParameter(self, index, value): self.Parameter[index] = value
    def getConst(self): return self.Const
    def setConst(self, Const): self.Const = Const
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def getKeyword(self): return self.Keyword
    def setKeyword(self, Keyword): self.Keyword = Keyword
    def export(self, outfile, level, name_='Methode'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Methode')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Methode'):
        if self.getConst() is not None:
            outfile.write(' Const="%s"' % (self.getConst(), ))
        outfile.write(' Name="%s"' % (self.getName(), ))
        if self.getKeyword() is not None:
            outfile.write(' Keyword="%s"' % (self.getKeyword(), ))
    def exportChildren(self, outfile, level, name_='Methode'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        for Parameter_ in self.getParameter():
            Parameter_.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Methode'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Const = "%s",\n' % (self.getConst(),))
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
        showIndent(outfile, level)
        outfile.write('Keyword = "%s",\n' % (self.getKeyword(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('Parameter=[\n')
        level += 1
        for Parameter in self.Parameter:
            showIndent(outfile, level)
            outfile.write('Parameter(\n')
            Parameter.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Const'):
            if attrs.get('Const').value in ('true', '1'):
                self.Const = 1
            elif attrs.get('Const').value in ('false', '0'):
                self.Const = 0
            else:
                raise ValueError('Bad boolean attribute (Const)')
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
        if attrs.get('Keyword'):
            if attrs.get('Keyword').value in ('true', '1'):
                self.Keyword = 1
            elif attrs.get('Keyword').value in ('false', '0'):
                self.Keyword = 0
            else:
                raise ValueError('Bad boolean attribute (Keyword)')
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Parameter':
            obj_ = Parameter.factory()
            obj_.build(child_)
            self.Parameter.append(obj_)
# end class Methode


class Attribute:
    subclass = None
    def __init__(self, ReadOnly=0, Name='', Documentation=None, Parameter=None):
        self.ReadOnly = ReadOnly
        self.Name = Name
        self.Documentation = Documentation
        self.Parameter = Parameter
    def factory(*args_, **kwargs_):
        if Attribute.subclass:
            return Attribute.subclass(*args_, **kwargs_)
        else:
            return Attribute(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getParameter(self): return self.Parameter
    def setParameter(self, Parameter): self.Parameter = Parameter
    def getReadonly(self): return self.ReadOnly
    def setReadonly(self, ReadOnly): self.ReadOnly = ReadOnly
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def export(self, outfile, level, name_='Attribute'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Attribute')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Attribute'):
        outfile.write(' ReadOnly="%s"' % (self.getReadonly(), ))
        outfile.write(' Name="%s"' % (self.getName(), ))
    def exportChildren(self, outfile, level, name_='Attribute'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        if self.Parameter:
            self.Parameter.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Attribute'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('ReadOnly = "%s",\n' % (self.getReadonly(),))
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        if self.Parameter:
            showIndent(outfile, level)
            outfile.write('Parameter=Parameter(\n')
            self.Parameter.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('ReadOnly'):
            if attrs.get('ReadOnly').value in ('true', '1'):
                self.ReadOnly = 1
            elif attrs.get('ReadOnly').value in ('false', '0'):
                self.ReadOnly = 0
            else:
                raise ValueError('Bad boolean attribute (ReadOnly)')
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Parameter':
            obj_ = Parameter.factory()
            obj_.build(child_)
            self.setParameter(obj_)
# end class Attribute


class Sequence:
    subclass = None
    def __init__(self, sq_ass_item=0, sq_item=0, sq_concat=0, sq_inplace_repeat=0, mp_ass_subscript=0, mp_subscript=0, sq_contains=0, sq_repeat=0, sq_length=0, sq_inplace_concat=0, valueOf_=''):
        self.sq_ass_item = sq_ass_item
        self.sq_item = sq_item
        self.sq_concat = sq_concat
        self.sq_inplace_repeat = sq_inplace_repeat
        self.mp_ass_subscript = mp_ass_subscript
        self.mp_subscript = mp_subscript
        self.sq_contains = sq_contains
        self.sq_repeat = sq_repeat
        self.sq_length = sq_length
        self.sq_inplace_concat = sq_inplace_concat
        self.valueOf_ = valueOf_
    def factory(*args_, **kwargs_):
        if Sequence.subclass:
            return Sequence.subclass(*args_, **kwargs_)
        else:
            return Sequence(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getSq_ass_item(self): return self.sq_ass_item
    def setSq_ass_item(self, sq_ass_item): self.sq_ass_item = sq_ass_item
    def getSq_item(self): return self.sq_item
    def setSq_item(self, sq_item): self.sq_item = sq_item
    def getSq_concat(self): return self.sq_concat
    def setSq_concat(self, sq_concat): self.sq_concat = sq_concat
    def getSq_inplace_repeat(self): return self.sq_inplace_repeat
    def setSq_inplace_repeat(self, sq_inplace_repeat): self.sq_inplace_repeat = sq_inplace_repeat
    def getMp_ass_subscript(self): return self.mp_ass_subscript
    def setMp_ass_subscript(self, mp_ass_subscript): self.mp_ass_subscript = mp_ass_subscript
    def getMp_subscript(self): return self.mp_subscript
    def setMp_subscript(self, mp_subscript): self.mp_subscript = mp_subscript
    def getSq_contains(self): return self.sq_contains
    def setSq_contains(self, sq_contains): self.sq_contains = sq_contains
    def getSq_repeat(self): return self.sq_repeat
    def setSq_repeat(self, sq_repeat): self.sq_repeat = sq_repeat
    def getSq_length(self): return self.sq_length
    def setSq_length(self, sq_length): self.sq_length = sq_length
    def getSq_inplace_concat(self): return self.sq_inplace_concat
    def setSq_inplace_concat(self, sq_inplace_concat): self.sq_inplace_concat = sq_inplace_concat
    def getValueOf_(self): return self.valueOf_
    def setValueOf_(self, valueOf_): self.valueOf_ = valueOf_
    def export(self, outfile, level, name_='Sequence'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Sequence')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Sequence'):
        outfile.write(' sq_ass_item="%s"' % (self.getSq_ass_item(), ))
        outfile.write(' sq_item="%s"' % (self.getSq_item(), ))
        outfile.write(' sq_concat="%s"' % (self.getSq_concat(), ))
        outfile.write(' sq_inplace_repeat="%s"' % (self.getSq_inplace_repeat(), ))
        outfile.write(' mp_ass_subscript="%s"' % (self.getMp_ass_subscript(), ))
        outfile.write(' mp_subscript="%s"' % (self.getMp_subscript(), ))
        outfile.write(' sq_contains="%s"' % (self.getSq_contains(), ))
        outfile.write(' sq_repeat="%s"' % (self.getSq_repeat(), ))
        outfile.write(' sq_length="%s"' % (self.getSq_length(), ))
        outfile.write(' sq_inplace_concat="%s"' % (self.getSq_inplace_concat(), ))
    def exportChildren(self, outfile, level, name_='Sequence'):
        outfile.write(self.valueOf_)
    def exportLiteral(self, outfile, level, name_='Sequence'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('sq_ass_item = "%s",\n' % (self.getSq_ass_item(),))
        showIndent(outfile, level)
        outfile.write('sq_item = "%s",\n' % (self.getSq_item(),))
        showIndent(outfile, level)
        outfile.write('sq_concat = "%s",\n' % (self.getSq_concat(),))
        showIndent(outfile, level)
        outfile.write('sq_inplace_repeat = "%s",\n' % (self.getSq_inplace_repeat(),))
        showIndent(outfile, level)
        outfile.write('mp_ass_subscript = "%s",\n' % (self.getMp_ass_subscript(),))
        showIndent(outfile, level)
        outfile.write('mp_subscript = "%s",\n' % (self.getMp_subscript(),))
        showIndent(outfile, level)
        outfile.write('sq_contains = "%s",\n' % (self.getSq_contains(),))
        showIndent(outfile, level)
        outfile.write('sq_repeat = "%s",\n' % (self.getSq_repeat(),))
        showIndent(outfile, level)
        outfile.write('sq_length = "%s",\n' % (self.getSq_length(),))
        showIndent(outfile, level)
        outfile.write('sq_inplace_concat = "%s",\n' % (self.getSq_inplace_concat(),))
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('valueOf_ = "%s",\n' % (self.valueOf_,))
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('sq_ass_item'):
            if attrs.get('sq_ass_item').value in ('true', '1'):
                self.sq_ass_item = 1
            elif attrs.get('sq_ass_item').value in ('false', '0'):
                self.sq_ass_item = 0
            else:
                raise ValueError('Bad boolean attribute (sq_ass_item)')
        if attrs.get('sq_item'):
            if attrs.get('sq_item').value in ('true', '1'):
                self.sq_item = 1
            elif attrs.get('sq_item').value in ('false', '0'):
                self.sq_item = 0
            else:
                raise ValueError('Bad boolean attribute (sq_item)')
        if attrs.get('sq_concat'):
            if attrs.get('sq_concat').value in ('true', '1'):
                self.sq_concat = 1
            elif attrs.get('sq_concat').value in ('false', '0'):
                self.sq_concat = 0
            else:
                raise ValueError('Bad boolean attribute (sq_concat)')
        if attrs.get('sq_inplace_repeat'):
            if attrs.get('sq_inplace_repeat').value in ('true', '1'):
                self.sq_inplace_repeat = 1
            elif attrs.get('sq_inplace_repeat').value in ('false', '0'):
                self.sq_inplace_repeat = 0
            else:
                raise ValueError('Bad boolean attribute (sq_inplace_repeat)')
        if attrs.get('mp_ass_subscript'):
            if attrs.get('mp_ass_subscript').value in ('true', '1'):
                self.mp_ass_subscript = 1
            elif attrs.get('mp_ass_subscript').value in ('false', '0'):
                self.mp_ass_subscript = 0
            else:
                raise ValueError('Bad boolean attribute (mp_ass_subscript)')
        if attrs.get('mp_subscript'):
            if attrs.get('mp_subscript').value in ('true', '1'):
                self.mp_subscript = 1
            elif attrs.get('mp_subscript').value in ('false', '0'):
                self.mp_subscript = 0
            else:
                raise ValueError('Bad boolean attribute (mp_subscript)')
        if attrs.get('sq_contains'):
            if attrs.get('sq_contains').value in ('true', '1'):
                self.sq_contains = 1
            elif attrs.get('sq_contains').value in ('false', '0'):
                self.sq_contains = 0
            else:
                raise ValueError('Bad boolean attribute (sq_contains)')
        if attrs.get('sq_repeat'):
            if attrs.get('sq_repeat').value in ('true', '1'):
                self.sq_repeat = 1
            elif attrs.get('sq_repeat').value in ('false', '0'):
                self.sq_repeat = 0
            else:
                raise ValueError('Bad boolean attribute (sq_repeat)')
        if attrs.get('sq_length'):
            if attrs.get('sq_length').value in ('true', '1'):
                self.sq_length = 1
            elif attrs.get('sq_length').value in ('false', '0'):
                self.sq_length = 0
            else:
                raise ValueError('Bad boolean attribute (sq_length)')
        if attrs.get('sq_inplace_concat'):
            if attrs.get('sq_inplace_concat').value in ('true', '1'):
                self.sq_inplace_concat = 1
            elif attrs.get('sq_inplace_concat').value in ('false', '0'):
                self.sq_inplace_concat = 0
            else:
                raise ValueError('Bad boolean attribute (sq_inplace_concat)')
    def buildChildren(self, child_, nodeName_):
        self.valueOf_ = ''
        for child in child_.childNodes:
            if child.nodeType == Node.TEXT_NODE:
                self.valueOf_ += child.nodeValue
# end class Sequence


class Module:
    subclass = None
    def __init__(self, Name='', Documentation=None, Dependencies=None, Content=None):
        self.Name = Name
        self.Documentation = Documentation
        self.Dependencies = Dependencies
        self.Content = Content
    def factory(*args_, **kwargs_):
        if Module.subclass:
            return Module.subclass(*args_, **kwargs_)
        else:
            return Module(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getDependencies(self): return self.Dependencies
    def setDependencies(self, Dependencies): self.Dependencies = Dependencies
    def getContent(self): return self.Content
    def setContent(self, Content): self.Content = Content
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def export(self, outfile, level, name_='Module'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Module')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Module'):
        outfile.write(' Name="%s"' % (self.getName(), ))
    def exportChildren(self, outfile, level, name_='Module'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        if self.Dependencies:
            self.Dependencies.export(outfile, level)
        if self.Content:
            self.Content.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Module'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        if self.Dependencies:
            showIndent(outfile, level)
            outfile.write('Dependencies=Dependencies(\n')
            self.Dependencies.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        if self.Content:
            showIndent(outfile, level)
            outfile.write('Content=Content(\n')
            self.Content.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Dependencies':
            obj_ = Dependencies.factory()
            obj_.build(child_)
            self.setDependencies(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Content':
            obj_ = Content.factory()
            obj_.build(child_)
            self.setContent(obj_)
# end class Module


class Dependencies:
    subclass = None
    def __init__(self, Module=None):
        if Module is None:
            self.Module = []
        else:
            self.Module = Module
    def factory(*args_, **kwargs_):
        if Dependencies.subclass:
            return Dependencies.subclass(*args_, **kwargs_)
        else:
            return Dependencies(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getModule(self): return self.Module
    def setModule(self, Module): self.Module = Module
    def addModule(self, value): self.Module.append(value)
    def insertModule(self, index, value): self.Module[index] = value
    def export(self, outfile, level, name_='Dependencies'):
        showIndent(outfile, level)
        outfile.write('<%s>\n' % name_)
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Dependencies'):
        pass
    def exportChildren(self, outfile, level, name_='Dependencies'):
        for Module_ in self.getModule():
            Module_.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Dependencies'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        pass
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Module=[\n')
        level += 1
        for Module in self.Module:
            showIndent(outfile, level)
            outfile.write('Module(\n')
            Module.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        pass
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Module':
            obj_ = Module.factory()
            obj_.build(child_)
            self.Module.append(obj_)
# end class Dependencies


class Content:
    subclass = None
    def __init__(self, Property=None, Feature=None, DocObject=None, GuiCommand=None, PreferencesPage=None):
        if Property is None:
            self.Property = []
        else:
            self.Property = Property
        if Feature is None:
            self.Feature = []
        else:
            self.Feature = Feature
        if DocObject is None:
            self.DocObject = []
        else:
            self.DocObject = DocObject
        if GuiCommand is None:
            self.GuiCommand = []
        else:
            self.GuiCommand = GuiCommand
        if PreferencesPage is None:
            self.PreferencesPage = []
        else:
            self.PreferencesPage = PreferencesPage
    def factory(*args_, **kwargs_):
        if Content.subclass:
            return Content.subclass(*args_, **kwargs_)
        else:
            return Content(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getProperty(self): return self.Property
    def setProperty(self, Property): self.Property = Property
    def addProperty(self, value): self.Property.append(value)
    def insertProperty(self, index, value): self.Property[index] = value
    def getFeature(self): return self.Feature
    def setFeature(self, Feature): self.Feature = Feature
    def addFeature(self, value): self.Feature.append(value)
    def insertFeature(self, index, value): self.Feature[index] = value
    def getDocobject(self): return self.DocObject
    def setDocobject(self, DocObject): self.DocObject = DocObject
    def addDocobject(self, value): self.DocObject.append(value)
    def insertDocobject(self, index, value): self.DocObject[index] = value
    def getGuicommand(self): return self.GuiCommand
    def setGuicommand(self, GuiCommand): self.GuiCommand = GuiCommand
    def addGuicommand(self, value): self.GuiCommand.append(value)
    def insertGuicommand(self, index, value): self.GuiCommand[index] = value
    def getPreferencespage(self): return self.PreferencesPage
    def setPreferencespage(self, PreferencesPage): self.PreferencesPage = PreferencesPage
    def addPreferencespage(self, value): self.PreferencesPage.append(value)
    def insertPreferencespage(self, index, value): self.PreferencesPage[index] = value
    def export(self, outfile, level, name_='Content'):
        showIndent(outfile, level)
        outfile.write('<%s>\n' % name_)
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Content'):
        pass
    def exportChildren(self, outfile, level, name_='Content'):
        for Property_ in self.getProperty():
            Property_.export(outfile, level)
        for Feature_ in self.getFeature():
            Feature_.export(outfile, level)
        for DocObject_ in self.getDocobject():
            DocObject_.export(outfile, level)
        for GuiCommand_ in self.getGuicommand():
            showIndent(outfile, level)
            outfile.write('<GuiCommand>%s</GuiCommand>\n' % quote_xml(GuiCommand_))
        for PreferencesPage_ in self.getPreferencespage():
            showIndent(outfile, level)
            outfile.write('<PreferencesPage>%s</PreferencesPage>\n' % quote_xml(PreferencesPage_))
    def exportLiteral(self, outfile, level, name_='Content'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        pass
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Property=[\n')
        level += 1
        for Property in self.Property:
            showIndent(outfile, level)
            outfile.write('Property(\n')
            Property.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('Feature=[\n')
        level += 1
        for Feature in self.Feature:
            showIndent(outfile, level)
            outfile.write('Feature(\n')
            Feature.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('DocObject=[\n')
        level += 1
        for DocObject in self.DocObject:
            showIndent(outfile, level)
            outfile.write('DocObject(\n')
            DocObject.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('GuiCommand=[\n')
        level += 1
        for GuiCommand in self.GuiCommand:
            showIndent(outfile, level)
            outfile.write('%s,\n' % quote_python(GuiCommand))
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        showIndent(outfile, level)
        outfile.write('PreferencesPage=[\n')
        level += 1
        for PreferencesPage in self.PreferencesPage:
            showIndent(outfile, level)
            outfile.write('%s,\n' % quote_python(PreferencesPage))
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        pass
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Property':
            obj_ = Property.factory()
            obj_.build(child_)
            self.Property.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Feature':
            obj_ = Feature.factory()
            obj_.build(child_)
            self.Feature.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'DocObject':
            obj_ = DocObject.factory()
            obj_.build(child_)
            self.DocObject.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'GuiCommand':
            GuiCommand_ = ''
            for text__content_ in child_.childNodes:
                GuiCommand_ += text__content_.nodeValue
            self.GuiCommand.append(GuiCommand_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'PreferencesPage':
            PreferencesPage_ = ''
            for text__content_ in child_.childNodes:
                PreferencesPage_ += text__content_.nodeValue
            self.PreferencesPage.append(PreferencesPage_)
# end class Content


class Feature:
    subclass = None
    def __init__(self, Name='', Documentation=None, Property=None, ViewProvider=None):
        self.Name = Name
        self.Documentation = Documentation
        if Property is None:
            self.Property = []
        else:
            self.Property = Property
        self.ViewProvider = ViewProvider
    def factory(*args_, **kwargs_):
        if Feature.subclass:
            return Feature.subclass(*args_, **kwargs_)
        else:
            return Feature(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getProperty(self): return self.Property
    def setProperty(self, Property): self.Property = Property
    def addProperty(self, value): self.Property.append(value)
    def insertProperty(self, index, value): self.Property[index] = value
    def getViewprovider(self): return self.ViewProvider
    def setViewprovider(self, ViewProvider): self.ViewProvider = ViewProvider
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def export(self, outfile, level, name_='Feature'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Feature')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Feature'):
        outfile.write(' Name="%s"' % (self.getName(), ))
    def exportChildren(self, outfile, level, name_='Feature'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        for Property_ in self.getProperty():
            Property_.export(outfile, level)
        if self.ViewProvider:
            self.ViewProvider.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Feature'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('Property=[\n')
        level += 1
        for Property in self.Property:
            showIndent(outfile, level)
            outfile.write('Property(\n')
            Property.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
        if self.ViewProvider:
            showIndent(outfile, level)
            outfile.write('ViewProvider=ViewProvider(\n')
            self.ViewProvider.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Property':
            obj_ = Property.factory()
            obj_.build(child_)
            self.Property.append(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'ViewProvider':
            obj_ = ViewProvider.factory()
            obj_.build(child_)
            self.setViewprovider(obj_)
# end class Feature


class DocObject:
    subclass = None
    def __init__(self, Name='', Documentation=None, Property=None):
        self.Name = Name
        self.Documentation = Documentation
        if Property is None:
            self.Property = []
        else:
            self.Property = Property
    def factory(*args_, **kwargs_):
        if DocObject.subclass:
            return DocObject.subclass(*args_, **kwargs_)
        else:
            return DocObject(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getProperty(self): return self.Property
    def setProperty(self, Property): self.Property = Property
    def addProperty(self, value): self.Property.append(value)
    def insertProperty(self, index, value): self.Property[index] = value
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def export(self, outfile, level, name_='DocObject'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='DocObject')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='DocObject'):
        outfile.write(' Name="%s"' % (self.getName(), ))
    def exportChildren(self, outfile, level, name_='DocObject'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
        for Property_ in self.getProperty():
            Property_.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='DocObject'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('Property=[\n')
        level += 1
        for Property in self.Property:
            showIndent(outfile, level)
            outfile.write('Property(\n')
            Property.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Property':
            obj_ = Property.factory()
            obj_.build(child_)
            self.Property.append(obj_)
# end class DocObject


class Property:
    subclass = None
    def __init__(self, Type='', Name='', StartValue='', Documentation=None):
        self.Type = Type
        self.Name = Name
        self.StartValue = StartValue
        self.Documentation = Documentation
    def factory(*args_, **kwargs_):
        if Property.subclass:
            return Property.subclass(*args_, **kwargs_)
        else:
            return Property(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getDocumentation(self): return self.Documentation
    def setDocumentation(self, Documentation): self.Documentation = Documentation
    def getType(self): return self.Type
    def setType(self, Type): self.Type = Type
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def getStartvalue(self): return self.StartValue
    def setStartvalue(self, StartValue): self.StartValue = StartValue
    def export(self, outfile, level, name_='Property'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Property')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Property'):
        outfile.write(' Type="%s"' % (self.getType(), ))
        outfile.write(' Name="%s"' % (self.getName(), ))
        if self.getStartvalue() is not None:
            outfile.write(' StartValue="%s"' % (self.getStartvalue(), ))
    def exportChildren(self, outfile, level, name_='Property'):
        if self.Documentation:
            self.Documentation.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='Property'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Type = "%s",\n' % (self.getType(),))
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
        showIndent(outfile, level)
        outfile.write('StartValue = "%s",\n' % (self.getStartvalue(),))
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Documentation:
            showIndent(outfile, level)
            outfile.write('Documentation=Documentation(\n')
            self.Documentation.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Type'):
            self.Type = attrs.get('Type').value
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
        if attrs.get('StartValue'):
            self.StartValue = attrs.get('StartValue').value
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Documentation':
            obj_ = Documentation.factory()
            obj_.build(child_)
            self.setDocumentation(obj_)
# end class Property


class Documentation:
    subclass = None
    def __init__(self, Author=None, DeveloperDocu='', UserDocu=''):
        self.Author = Author
        self.DeveloperDocu = DeveloperDocu
        self.UserDocu = UserDocu
    def factory(*args_, **kwargs_):
        if Documentation.subclass:
            return Documentation.subclass(*args_, **kwargs_)
        else:
            return Documentation(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getAuthor(self): return self.Author
    def setAuthor(self, Author): self.Author = Author
    def getDeveloperdocu(self): return self.DeveloperDocu
    def setDeveloperdocu(self, DeveloperDocu): self.DeveloperDocu = DeveloperDocu
    def getUserdocu(self): return self.UserDocu
    def setUserdocu(self, UserDocu): self.UserDocu = UserDocu
    def export(self, outfile, level, name_='Documentation'):
        showIndent(outfile, level)
        outfile.write('<%s>\n' % name_)
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Documentation'):
        pass
    def exportChildren(self, outfile, level, name_='Documentation'):
        if self.Author:
            self.Author.export(outfile, level)
        showIndent(outfile, level)
        outfile.write('<DeveloperDocu>%s</DeveloperDocu>\n' % quote_xml(self.getDeveloperdocu()))
        showIndent(outfile, level)
        outfile.write('<UserDocu>%s</UserDocu>\n' % quote_xml(self.getUserdocu()))
    def exportLiteral(self, outfile, level, name_='Documentation'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        pass
    def exportLiteralChildren(self, outfile, level, name_):
        if self.Author:
            showIndent(outfile, level)
            outfile.write('Author=Author(\n')
            self.Author.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        showIndent(outfile, level)
        outfile.write('DeveloperDocu=%s,\n' % quote_python(self.getDeveloperdocu()))
        showIndent(outfile, level)
        outfile.write('UserDocu=%s,\n' % quote_python(self.getUserdocu()))
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        pass
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Author':
            obj_ = Author.factory()
            obj_.build(child_)
            self.setAuthor(obj_)
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'DeveloperDocu':
            DeveloperDocu_ = ''
            for text__content_ in child_.childNodes:
                DeveloperDocu_ += text__content_.nodeValue
            self.DeveloperDocu = DeveloperDocu_
        elif child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'UserDocu':
            UserDocu_ = ''
            for text__content_ in child_.childNodes:
                UserDocu_ += text__content_.nodeValue
            self.UserDocu = UserDocu_
# end class Documentation


class Author:
    subclass = None
    def __init__(self, Name='', Licence='', EMail='', valueOf_=''):
        self.Name = Name
        self.Licence = Licence
        self.EMail = EMail
        self.valueOf_ = valueOf_
    def factory(*args_, **kwargs_):
        if Author.subclass:
            return Author.subclass(*args_, **kwargs_)
        else:
            return Author(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def getLicence(self): return self.Licence
    def setLicence(self, Licence): self.Licence = Licence
    def getEmail(self): return self.EMail
    def setEmail(self, EMail): self.EMail = EMail
    def getValueOf_(self): return self.valueOf_
    def setValueOf_(self, valueOf_): self.valueOf_ = valueOf_
    def export(self, outfile, level, name_='Author'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Author')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Author'):
        outfile.write(' Name="%s"' % (self.getName(), ))
        if self.getLicence() is not None:
            outfile.write(' Licence="%s"' % (self.getLicence(), ))
        outfile.write(' EMail="%s"' % (self.getEmail(), ))
    def exportChildren(self, outfile, level, name_='Author'):
        outfile.write(self.valueOf_)
    def exportLiteral(self, outfile, level, name_='Author'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
        showIndent(outfile, level)
        outfile.write('Licence = "%s",\n' % (self.getLicence(),))
        showIndent(outfile, level)
        outfile.write('EMail = "%s",\n' % (self.getEmail(),))
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('valueOf_ = "%s",\n' % (self.valueOf_,))
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
        if attrs.get('Licence'):
            self.Licence = attrs.get('Licence').value
        if attrs.get('EMail'):
            self.EMail = attrs.get('EMail').value
    def buildChildren(self, child_, nodeName_):
        self.valueOf_ = ''
        for child in child_.childNodes:
            if child.nodeType == Node.TEXT_NODE:
                self.valueOf_ += child.nodeValue
# end class Author


class ViewProvider:
    subclass = None
    def __init__(self, Property=None):
        if Property is None:
            self.Property = []
        else:
            self.Property = Property
    def factory(*args_, **kwargs_):
        if ViewProvider.subclass:
            return ViewProvider.subclass(*args_, **kwargs_)
        else:
            return ViewProvider(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getProperty(self): return self.Property
    def setProperty(self, Property): self.Property = Property
    def addProperty(self, value): self.Property.append(value)
    def insertProperty(self, index, value): self.Property[index] = value
    def export(self, outfile, level, name_='ViewProvider'):
        showIndent(outfile, level)
        outfile.write('<%s>\n' % name_)
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='ViewProvider'):
        pass
    def exportChildren(self, outfile, level, name_='ViewProvider'):
        for Property_ in self.getProperty():
            Property_.export(outfile, level)
    def exportLiteral(self, outfile, level, name_='ViewProvider'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        pass
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Property=[\n')
        level += 1
        for Property in self.Property:
            showIndent(outfile, level)
            outfile.write('Property(\n')
            Property.exportLiteral(outfile, level)
            showIndent(outfile, level)
            outfile.write('),\n')
        level -= 1
        showIndent(outfile, level)
        outfile.write('],\n')
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        pass
    def buildChildren(self, child_, nodeName_):
        if child_.nodeType == Node.ELEMENT_NODE and \
            nodeName_ == 'Property':
            obj_ = Property.factory()
            obj_.build(child_)
            self.Property.append(obj_)
# end class ViewProvider


class Parameter:
    subclass = None
    def __init__(self, Type='', Name='', valueOf_=''):
        self.Type = Type
        self.Name = Name
        self.valueOf_ = valueOf_
    def factory(*args_, **kwargs_):
        if Parameter.subclass:
            return Parameter.subclass(*args_, **kwargs_)
        else:
            return Parameter(*args_, **kwargs_)
    factory = staticmethod(factory)
    def getType(self): return self.Type
    def setType(self, Type): self.Type = Type
    def getName(self): return self.Name
    def setName(self, Name): self.Name = Name
    def getValueOf_(self): return self.valueOf_
    def setValueOf_(self, valueOf_): self.valueOf_ = valueOf_
    def export(self, outfile, level, name_='Parameter'):
        showIndent(outfile, level)
        outfile.write('<%s' % (name_, ))
        self.exportAttributes(outfile, level, name_='Parameter')
        outfile.write('>\n')
        self.exportChildren(outfile, level + 1, name_)
        showIndent(outfile, level)
        outfile.write('</%s>\n' % name_)
    def exportAttributes(self, outfile, level, name_='Parameter'):
        outfile.write(' Type="%s"' % (self.getType(), ))
        outfile.write(' Name="%s"' % (self.getName(), ))
    def exportChildren(self, outfile, level, name_='Parameter'):
        outfile.write(self.valueOf_)
    def exportLiteral(self, outfile, level, name_='Parameter'):
        level += 1
        self.exportLiteralAttributes(outfile, level, name_)
        self.exportLiteralChildren(outfile, level, name_)
    def exportLiteralAttributes(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('Type = "%s",\n' % (self.getType(),))
        showIndent(outfile, level)
        outfile.write('Name = "%s",\n' % (self.getName(),))
    def exportLiteralChildren(self, outfile, level, name_):
        showIndent(outfile, level)
        outfile.write('valueOf_ = "%s",\n' % (self.valueOf_,))
    def build(self, node_):
        attrs = node_.attributes
        self.buildAttributes(attrs)
        for child_ in node_.childNodes:
            nodeName_ = child_.nodeName.split(':')[-1]
            self.buildChildren(child_, nodeName_)
    def buildAttributes(self, attrs):
        if attrs.get('Type'):
            self.Type = attrs.get('Type').value
        if attrs.get('Name'):
            self.Name = attrs.get('Name').value
    def buildChildren(self, child_, nodeName_):
        self.valueOf_ = ''
        for child in child_.childNodes:
            if child.nodeType == Node.TEXT_NODE:
                self.valueOf_ += child.nodeValue
# end class Parameter


from xml.sax import handler, make_parser

class SaxStackElement:
    def __init__(self, name='', obj=None):
        self.name = name
        self.obj = obj
        self.content = ''

#
# SAX handler
#
class SaxGeneratemodelHandler(handler.ContentHandler):
    def __init__(self):
        self.stack = []
        self.root = None

    def getRoot(self):
        return self.root

    def setDocumentLocator(self, locator):
        self.locator = locator
    
    def showError(self, msg):
        print('*** (showError):', msg)
        sys.exit(-1)

    def startElement(self, name, attrs):
        done = 0
        if name == 'GenerateModel':
            obj = GenerateModel.factory()
            stackObj = SaxStackElement('GenerateModel', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Module':
            obj = Module.factory()
            stackObj = SaxStackElement('Module', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'PythonExport':
            obj = PythonExport.factory()
            val = attrs.get('FatherNamespace', None)
            if val is not None:
                obj.setFathernamespace(val)
            val = attrs.get('DisableNotify', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setDisablenotify(1)
                elif val in ('false', '0'):
                    obj.setDisablenotify(0)
                else:
                    self.reportError('"DisableNotify" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('RichCompare', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setRichcompare(1)
                elif val in ('false', '0'):
                    obj.setRichcompare(0)
                else:
                    self.reportError('"RichCompare" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            val = attrs.get('Reference', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setReference(1)
                elif val in ('false', '0'):
                    obj.setReference(0)
                else:
                    self.reportError('"Reference" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('FatherInclude', None)
            if val is not None:
                obj.setFatherinclude(val)
            val = attrs.get('Namespace', None)
            if val is not None:
                obj.setNamespace(val)
            val = attrs.get('Initialization', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setInitialization(1)
                elif val in ('false', '0'):
                    obj.setInitialization(0)
                else:
                    self.reportError('"Initialization" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('Father', None)
            if val is not None:
                obj.setFather(val)
            val = attrs.get('PythonName', None)
            if val is not None:
                obj.setPythonname(val)
            val = attrs.get('Twin', None)
            if val is not None:
                obj.setTwin(val)
            val = attrs.get('Constructor', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setConstructor(1)
                elif val in ('false', '0'):
                    obj.setConstructor(0)
                else:
                    self.reportError('"Constructor" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('TwinPointer', None)
            if val is not None:
                obj.setTwinpointer(val)
            val = attrs.get('Include', None)
            if val is not None:
                obj.setInclude(val)
            val = attrs.get('NumberProtocol', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setNumberprotocol(1)
                elif val in ('false', '0'):
                    obj.setNumberprotocol(0)
                else:
                    self.reportError('"NumberProtocol" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('Delete', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setDelete(1)
                elif val in ('false', '0'):
                    obj.setDelete(0)
                else:
                    self.reportError('"Delete" attribute must be boolean ("true", "1", "false", "0")')
            stackObj = SaxStackElement('PythonExport', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Documentation':
            obj = Documentation.factory()
            stackObj = SaxStackElement('Documentation', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Methode':
            obj = Methode.factory()
            val = attrs.get('Const', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setConst(1)
                elif val in ('false', '0'):
                    obj.setConst(0)
                else:
                    self.reportError('"Const" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            val = attrs.get('Keyword', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setKeyword(1)
                elif val in ('false', '0'):
                    obj.setKeyword(0)
                else:
                    self.reportError('"Keyword" attribute must be boolean ("true", "1", "false", "0")')
            stackObj = SaxStackElement('Methode', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Parameter':
            obj = Parameter.factory()
            val = attrs.get('Type', None)
            if val is not None:
                obj.setType(val)
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            stackObj = SaxStackElement('Parameter', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Attribute':
            obj = Attribute.factory()
            val = attrs.get('ReadOnly', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setReadonly(1)
                elif val in ('false', '0'):
                    obj.setReadonly(0)
                else:
                    self.reportError('"ReadOnly" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            stackObj = SaxStackElement('Attribute', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Sequence':
            obj = Sequence.factory()
            val = attrs.get('sq_ass_item', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_ass_item(1)
                elif val in ('false', '0'):
                    obj.setSq_ass_item(0)
                else:
                    self.reportError('"sq_ass_item" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_item', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_item(1)
                elif val in ('false', '0'):
                    obj.setSq_item(0)
                else:
                    self.reportError('"sq_item" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_concat', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_concat(1)
                elif val in ('false', '0'):
                    obj.setSq_concat(0)
                else:
                    self.reportError('"sq_concat" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_inplace_repeat', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_inplace_repeat(1)
                elif val in ('false', '0'):
                    obj.setSq_inplace_repeat(0)
                else:
                    self.reportError('"sq_inplace_repeat" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('mp_ass_subscript', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setMp_ass_subscript(1)
                elif val in ('false', '0'):
                    obj.setMp_ass_subscript(0)
                else:
                    self.reportError('"mp_ass_subscript" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('mp_subscript', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setMp_subscript(1)
                elif val in ('false', '0'):
                    obj.setMp_subscript(0)
                else:
                    self.reportError('"mp_subscript" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_contains', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_contains(1)
                elif val in ('false', '0'):
                    obj.setSq_contains(0)
                else:
                    self.reportError('"sq_contains" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_repeat', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_repeat(1)
                elif val in ('false', '0'):
                    obj.setSq_repeat(0)
                else:
                    self.reportError('"sq_repeat" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_length', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_length(1)
                elif val in ('false', '0'):
                    obj.setSq_length(0)
                else:
                    self.reportError('"sq_length" attribute must be boolean ("true", "1", "false", "0")')
            val = attrs.get('sq_inplace_concat', None)
            if val is not None:
                if val in ('true', '1'):
                    obj.setSq_inplace_concat(1)
                elif val in ('false', '0'):
                    obj.setSq_inplace_concat(0)
                else:
                    self.reportError('"sq_inplace_concat" attribute must be boolean ("true", "1", "false", "0")')
            stackObj = SaxStackElement('Sequence', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'CustomAttributes':
            stackObj = SaxStackElement('CustomAttributes', None)
            self.stack.append(stackObj)
            done = 1
        elif name == 'ClassDeclarations':
            stackObj = SaxStackElement('ClassDeclarations', None)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Dependencies':
            obj = Dependencies.factory()
            stackObj = SaxStackElement('Dependencies', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Content':
            obj = Content.factory()
            stackObj = SaxStackElement('Content', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Property':
            obj = Property.factory()
            stackObj = SaxStackElement('Property', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Feature':
            obj = Feature.factory()
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            stackObj = SaxStackElement('Feature', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'ViewProvider':
            obj = ViewProvider.factory()
            stackObj = SaxStackElement('ViewProvider', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'DocObject':
            obj = DocObject.factory()
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            stackObj = SaxStackElement('DocObject', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'GuiCommand':
            stackObj = SaxStackElement('GuiCommand', None)
            self.stack.append(stackObj)
            done = 1
        elif name == 'PreferencesPage':
            stackObj = SaxStackElement('PreferencesPage', None)
            self.stack.append(stackObj)
            done = 1
        elif name == 'Author':
            obj = Author.factory()
            val = attrs.get('Name', None)
            if val is not None:
                obj.setName(val)
            val = attrs.get('Licence', None)
            if val is not None:
                obj.setLicence(val)
            val = attrs.get('EMail', None)
            if val is not None:
                obj.setEmail(val)
            stackObj = SaxStackElement('Author', obj)
            self.stack.append(stackObj)
            done = 1
        elif name == 'DeveloperDocu':
            stackObj = SaxStackElement('DeveloperDocu', None)
            self.stack.append(stackObj)
            done = 1
        elif name == 'UserDocu':
            stackObj = SaxStackElement('UserDocu', None)
            self.stack.append(stackObj)
            done = 1
        if not done:
            self.reportError('"%s" element not allowed here.' % name)

    def endElement(self, name):
        done = 0
        if name == 'GenerateModel':
            if len(self.stack) == 1:
                self.root = self.stack[-1].obj
                self.stack.pop()
                done = 1
        elif name == 'Module':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addModule(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'PythonExport':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addPythonexport(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Documentation':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setDocumentation(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Methode':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addMethode(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Parameter':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addParameter(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Attribute':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addAttribute(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Sequence':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setSequence(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'CustomAttributes':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.setCustomattributes(content)
                self.stack.pop()
                done = 1
        elif name == 'ClassDeclarations':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.setClassdeclarations(content)
                self.stack.pop()
                done = 1
        elif name == 'Dependencies':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setDependencies(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Content':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setContent(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Property':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addProperty(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'Feature':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addFeature(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'ViewProvider':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setViewprovider(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'DocObject':
            if len(self.stack) >= 2:
                self.stack[-2].obj.addDocobject(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'GuiCommand':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.addGuicommand(content)
                self.stack.pop()
                done = 1
        elif name == 'PreferencesPage':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.addPreferencespage(content)
                self.stack.pop()
                done = 1
        elif name == 'Author':
            if len(self.stack) >= 2:
                self.stack[-2].obj.setAuthor(self.stack[-1].obj)
                self.stack.pop()
                done = 1
        elif name == 'DeveloperDocu':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.setDeveloperdocu(content)
                self.stack.pop()
                done = 1
        elif name == 'UserDocu':
            if len(self.stack) >= 2:
                content = self.stack[-1].content
                self.stack[-2].obj.setUserdocu(content)
                self.stack.pop()
                done = 1
        if not done:
            self.reportError('"%s" element not allowed here.' % name)

    def characters(self, chrs, start, end):
        if len(self.stack) > 0:
            self.stack[-1].content += chrs[start:end]

    def reportError(self, mesg):
        locator = self.locator
        sys.stderr.write('Doc: %s  Line: %d  Column: %d\n' % \
            (locator.getSystemId(), locator.getLineNumber(), 
            locator.getColumnNumber() + 1))
        sys.stderr.write(mesg)
        sys.stderr.write('\n')
        sys.exit(-1)
        #raise RuntimeError

USAGE_TEXT = """
Usage: python <Parser>.py [ -s ] <in_xml_file>
Options:
    -s        Use the SAX parser, not the minidom parser.
"""

def usage():
    print(USAGE_TEXT)
    sys.exit(-1)


#
# SAX handler used to determine the top level element.
#
class SaxSelectorHandler(handler.ContentHandler):
    def __init__(self):
        self.topElementName = None
    def getTopElementName(self):
        return self.topElementName
    def startElement(self, name, attrs):
        self.topElementName = name
        raise StopIteration


def parseSelect(inFileName):
    infile = open(inFileName, 'r')
    topElementName = None
    parser = make_parser()
    documentHandler = SaxSelectorHandler()
    parser.setContentHandler(documentHandler)
    try:
        try:
            parser.parse(infile)
        except StopIteration:
            topElementName = documentHandler.getTopElementName()
        if topElementName is None:
            raise RuntimeError('no top level element')
        topElementName = topElementName.replace('-', '_').replace(':', '_')
        if topElementName not in globals():
            raise RuntimeError('no class for top element: %s' % topElementName)
        topElement = globals()[topElementName]
        infile.seek(0)
        doc = minidom.parse(infile)
    finally:
        infile.close()
    rootNode = doc.childNodes[0]
    rootObj = topElement.factory()
    rootObj.build(rootNode)
    # Enable Python to collect the space used by the DOM.
    doc = None
    sys.stdout.write('<?xml version="1.0" ?>\n')
    rootObj.export(sys.stdout, 0)
    return rootObj


def saxParse(inFileName):
    parser = make_parser()
    documentHandler = SaxGeneratemodelHandler()
    parser.setDocumentHandler(documentHandler)
    parser.parse('file:%s' % inFileName)
    root = documentHandler.getRoot()
    sys.stdout.write('<?xml version="1.0" ?>\n')
    root.export(sys.stdout, 0)
    return root


def saxParseString(inString):
    parser = make_parser()
    documentHandler = SaxGeneratemodelHandler()
    parser.setDocumentHandler(documentHandler)
    parser.feed(inString)
    parser.close()
    rootObj = documentHandler.getRoot()
    #sys.stdout.write('<?xml version="1.0" ?>\n')
    #rootObj.export(sys.stdout, 0)
    return rootObj


def parse(inFileName):
    doc = minidom.parse(inFileName)
    rootNode = doc.documentElement
    rootObj = GenerateModel.factory()
    rootObj.build(rootNode)
    # Enable Python to collect the space used by the DOM.
    doc = None
    sys.stdout.write('<?xml version="1.0" ?>\n')
    rootObj.export(sys.stdout, 0, name_="GenerateModel")
    return rootObj


def parseString(inString):
    doc = minidom.parseString(inString)
    rootNode = doc.documentElement
    rootObj = GenerateModel.factory()
    rootObj.build(rootNode)
    # Enable Python to collect the space used by the DOM.
    doc = None
    sys.stdout.write('<?xml version="1.0" ?>\n')
    rootObj.export(sys.stdout, 0, name_="GenerateModel")
    return rootObj


def parseLiteral(inFileName):
    doc = minidom.parse(inFileName)
    rootNode = doc.documentElement
    rootObj = GenerateModel.factory()
    rootObj.build(rootNode)
    # Enable Python to collect the space used by the DOM.
    doc = None
    sys.stdout.write('from generateModel_Module import *\n\n')
    sys.stdout.write('rootObj = GenerateModel(\n')
    rootObj.exportLiteral(sys.stdout, 0, name_="GenerateModel")
    sys.stdout.write(')\n')
    return rootObj


def main():
    args = sys.argv[1:]
    if len(args) == 2 and args[0] == '-s':
        saxParse(args[1])
    elif len(args) == 1:
        parse(args[0])
    else:
        usage()


if __name__ == '__main__':
    main()
    #import pdb
    #pdb.run('main()')

