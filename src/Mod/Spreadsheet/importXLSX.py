#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  importXLSX.py
#  
#  This library imports an Excel-XLSX-file into FreeCAD.
#  
#  Copyright (C) 2016  Ulrich Brammer <ulrich1a@users.sourceforge.net>
#  
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
#  MA  02110-1301  USA

__title__="FreeCAD Spreadsheet Workbench - XLSX importer"
__author__ =  "Ulrich Brammer <ulrich1a@users.sourceforge.net>"
__url__ = ["http://www.freecadweb.org"]

'''
This library imports an Excel-XLSX-file into FreeCAD.

It uses a minimal parser, in order to translate the IF-function into
the different FreeCAD version.
The other function-names are translated by search and replace.

Version 1.0:
- Imports tables defined inside Excel-document
- Set alias definitions
- Translate formulas known by FreeCAD. (see funcDic + IF)
- set cross table references
- strings are imported
- references to cells with strings are working

known issues:
- units are not imported
- string support is minimal, as it is so in FreeCAD
'''


import zipfile
import xml.dom.minidom
import FreeCAD as App

try: import FreeCADGui
except ValueError: gui = False
else: gui = True

if open.__module__ == '__builtin__':
    pythonopen = open


funcDic = {
  'ABS(':'abs(',
  'ACOS(':'acos(', 
  'ASIN(':'asin(',
  'ATAN(':'atan(', 
  'ATAN2(':'atan2(',
  'COS(':'cos(',
  'COSH(':'cosh(',
  'EXP(':'exp(',
  'LOG(':'log(',
  'LOG10(':'log10(', 
  'MOD(':'mod(', 
  'POWER(':'pow(', 
  'SIN(':'sin(', 
  'SINH(':'sinh(', 
  'SQRT(':'sqrt(',
  'TAN(':'tan(', 
  'TANH(':'tanh(',
  'AVERAGE(':'average(',
  'COUNT(':'count(', 
  'MAX(':'max(', 
  'MIN(':'min(', 
  'STDEVA(':'stddev(', 
  'SUM(':'sum(',
  'PI()':'pi'
  } 
  
# The treeToken structure is used in the tokenizer functions isKey and
# getNextToken.
# treeToken defines a search tree for tokens with length of 1 to 3 characters
treeToken = {
  '(':None,
  'I':'branchI',
  '=':None,
  '<':'branchLower',
  '>':'branchHigher',
  ')':None,
#  '"':None,
#  ';':None,
  ' ':None,
  ',':None,
  '!':None
  }
  
branchI = {'F':'branchF'}
branchF = {'(':None}

branchLower ={
  '>':None,
  '=':None
  }

branchHigher = {'=':None}  


# Needed to get a reference from a string to a dict 
treeDict = {
  'branchI':branchI,
  'branchF':branchF,
  'branchLower':branchLower,
  'branchHigher':branchHigher
  }

# The tokenDic is used in parseExpr.
# The tokenDic contains the following information:
# levelchange: -1: tree down, 0, +1: tree up
# replacement token
# special token list

tokenDic = {
  '('  :( 1, '(',  None),
  'IF(':( 1, '(',  None),
  '='  :( 0 ,'==', None),
  '<>' :( 0 ,'!=', None),
  '>=' :( 0 ,'>=', None),
  '<=' :( 0 ,'<=', None),
  '<'  :( 0 ,'<',  None),
  '>'  :( 0 ,'>',  None),
#  ';'  :( 0 ,';', ['?',':']),
  ','  :( 0 ,',', ['?',':']),
  ')'  :(-1 ,')',  None),
  '!'  :( 0 ,'.',  None)   #Connector to cells on other Sheets
#  '"'  :( 2 ,'',  None)
  }


class exprNode(object):
  ''' This defines a tree class for expression parsing'''   
  def __init__(self, parent, state):
    self.state = state #see comment: State machine for expression parsing
    self.parent = parent # Parent tree node
    self.result = ''


class FormulaTranslator(object):
  ''' This class tranlates a cell-formula from Excel to FreeCAD.'''
  def __init__(self):
    self.theTList = ['=']

  def translateForm(self, actExpr):
    self.getNextToken(actExpr)
    #print "tokenList: ", self.theTList
    self.resultTree = exprNode(None, 5)
    self.resultTree.result = self.resultTree.result + self.theTList[0]
    self.parseExpr(self.theTList, 1, self.resultTree)
    #print 'parseResult: ', self.resultTree.result
    return self.replaceFunc(self.resultTree.result)


  def replaceFunc(self, cellFormula):
    for funcKey in funcDic:
      if funcKey in cellFormula:
        cellFormula = cellFormula.replace(funcKey, funcDic[funcKey])
    return cellFormula
  
  
  def getNextToken(self, theExpr):
    #print 'next Token theExpr: ', theExpr
    #print 'actTList: ', self.theTList
    tokenComplete = False
    keyToken = False
    if len(theExpr)>0:
      theTok = theExpr[0]
      theExpr = theExpr[1:]
      if theTok in treeToken:
        keyToken = True
        branch = treeToken[theTok]
        while branch:
          #print branch, ' theExpr[0]: ',theExpr[0] 
          if theExpr[0] in treeDict[branch]:
            branch = treeDict[branch][theExpr[0]]
            theTok = theTok + theExpr[0]
            theExpr = theExpr[1:]
          else:
            branch= None
        tokenComplete = True
        self.theTList.append(theTok)
        self.getNextToken(theExpr)
      else:
        if len(theExpr)>0:
          while (not tokenComplete):
            if not self.isKey(theExpr):
              theTok = theTok + theExpr[0]
              theExpr = theExpr[1:]
              if len(theExpr) == 0:
                tokenComplete = True
            else:
              tokenComplete = True
        self.theTList.append(theTok)
        self.getNextToken(theExpr)
          
    
  def isKey(self, theExpr):
    #print 'look up: ', theExpr
    keyToken = False
    lenExpr = len(theExpr)
    if theExpr[0] in treeToken:
      branch = treeToken[theExpr[0]]
      
      if branch == None:
        keyToken = True
      else:
        #print 'There is a branch. look up: ', theExpr[1]
        if (lenExpr > 1) and (theExpr[1] in treeDict[branch]):
          branch = treeDict[branch][theExpr[0]]
          if branch == None:
            keyToken = True
          else:
            if (lenExpr > 2) and (theExpr[2] in treeDict[branch]):
              keyToken = True
        else:
          keyToken = True
    return keyToken
          
  
  # State machine for expression parsing
  # 0 in ifsubexpression
  # 1 in conditional
  # 2 in truecase
  # 3 in falsecase
  # 4 in subexpression
  # 5 toplevel '='
  
  def parseExpr(self, tokenList, index, theTree):
    token = tokenList[index]
    #print 'state: ', theTree.state, ' ', token
    nextIdx = index + 1
    if token in tokenDic:
      lChange, newToken, specialList = tokenDic[token]
    else:
      lChange = 0
      newToken = token
      specialList = None
    
    if lChange == 1:
      theTree.result = theTree.result + newToken
      if token == '(':
        state = 4
      else:
        state = 1
      newNode = exprNode(theTree, state)
      self.parseExpr(tokenList, nextIdx, newNode)
    else:
      if lChange == 0:
        if theTree.state > 2:
          theTree.result = theTree.result + newToken
        else:
          if (theTree.state == 1):
            if specialList:
              theTree.result = theTree.result + specialList[0]
              theTree.state = 2
            else:
              theTree.result = theTree.result + newToken
          else:
            if (theTree.state == 2):
              if specialList:
                theTree.result = theTree.result + specialList[1]
                theTree.state = 3
              else:
                theTree.result = theTree.result + newToken
          
        if nextIdx < len(tokenList):
          self.parseExpr(tokenList, nextIdx, theTree)
      else:
        theTree.parent.result = theTree.parent.result + theTree.result + newToken
        if nextIdx < len(tokenList):
          self.parseExpr(tokenList, nextIdx, theTree.parent)
      
  # End of Formula Translator

def getText(nodelist):
  rc = []
  for node in nodelist:
    if node.nodeType == node.TEXT_NODE:
      rc.append(node.data)
  return ''.join(rc)


def handleWorkSheet(theDom, actSheet, strList):
  rows = theDom.getElementsByTagName("row")
  for row in rows:
    handleCells(row.getElementsByTagName("c"), actSheet, strList)
  

def handleCells(cellList, actCellSheet, sList):
  for cell in cellList:
    cellAtts = cell.attributes
    refRef = cellAtts.getNamedItem("r")
    ref = getText(refRef.childNodes)

    refType = cellAtts.getNamedItem("t")
    if refType:
      cellType = getText(refType.childNodes)
    else:
      cellType = 'n'   # fix me some cells dont have t and s attributes

    #print "reference: ", ref, ' Cell type: ', cellType

    formulaRef = cell.getElementsByTagName("f")
    if len(formulaRef)==1:
      theFormula = getText(formulaRef[0].childNodes)
      #print "theFormula: ", theFormula
      fTrans = FormulaTranslator()
      actCellSheet.set(ref, fTrans.translateForm(theFormula))

    else:
      valueRef = cell.getElementsByTagName("v")[0]
      if valueRef:
        theValue = getText(valueRef.childNodes)
        #print "theValue: ", theValue
        if cellType == 'n':
          actCellSheet.set(ref, theValue)
        if cellType == 's':
          actCellSheet.set(ref, (sList[int(theValue)]).encode('utf8'))


def handleWorkBook(theBook, sheetDict, Doc):
  theSheets = theBook.getElementsByTagName("sheet")
  #print "theSheets: ", theSheets
  for sheet in theSheets:
    sheetAtts = sheet.attributes
    nameRef = sheetAtts.getNamedItem("name")
    sheetName = getText(nameRef.childNodes)
    #print "table name: ", sheetName
    idRef = sheetAtts.getNamedItem("sheetId")
    sheetFile = "sheet" + getText(idRef.childNodes) + '.xml'
    #print "sheetFile: ", sheetFile
    # add FreeCAD-spreadsheet
    sheetDict[sheetName] = (Doc.addObject('Spreadsheet::Sheet', sheetName), sheetFile)
    
  theAliases = theBook.getElementsByTagName("definedName")
  for theAlias in theAliases:
    aliAtts = theAlias.attributes
    nameRef = aliAtts.getNamedItem("name")
    aliasName = getText(nameRef.childNodes)
    #print "aliasName: ", aliasName

    aliasRef = getText(theAlias.childNodes)
    if '$' in aliasRef:
      refList = aliasRef.split('!$')
      adressList = refList[1].split('$')
      #print "aliasRef: ", aliasRef
      #print 'Sheet Name: ', refList[0]
      #print 'Adress: ', adressList[0] + adressList[1]
      actSheet, sheetFile = sheetDict[refList[0]]
      actSheet.setAlias(adressList[0]+adressList[1], aliasName.encode('utf8'))

def handleStrings(theStr, sList):
  print 'process Strings: '
  stringElements = theStr.getElementsByTagName('t')
  for sElem in stringElements:
    print 'string: ', getText(sElem.childNodes)
    sList.append(getText(sElem.childNodes))

def open(nameXLSX):

  if len(nameXLSX) > 0:
    z=zipfile.ZipFile(nameXLSX)
    
    theDoc = App.newDocument()
    
    sheetDict = dict()
    stringList = []
    
    theBookFile=z.open('xl/workbook.xml')
    theBook = xml.dom.minidom.parse(theBookFile)
    handleWorkBook(theBook, sheetDict, theDoc)
    theBook.unlink()
    
    if 'xl/sharedStrings.xml' in z.namelist():
      theStringFile=z.open('xl/sharedStrings.xml')
      theStrings = xml.dom.minidom.parse(theStringFile)
      handleStrings(theStrings, stringList)
      theStrings.unlink()
    
    for sheetSpec in sheetDict:
      #print "sheetSpec: ", sheetSpec
      theSheet, sheetFile = sheetDict[sheetSpec]
      f=z.open('xl/worksheets/' + sheetFile)
      myDom = xml.dom.minidom.parse(f)
      
      handleWorkSheet(myDom, theSheet, stringList)
      myDom.unlink()
    
    z.close()
    # This is needed more than once, otherwise some references are not calculated!
    theDoc.recompute()
    theDoc.recompute()
    theDoc.recompute()
    return theDoc
    
def insert(nameXLSX,docname):
  try:
          theDoc=App.getDocument(docname)
  except NameError:
          theDoc=App.newDocument(docname)
  App.ActiveDocument = theDoc

  sheetDict = dict()
  stringList = []

  z=zipfile.ZipFile(nameXLSX)
  theBookFile=z.open('xl/workbook.xml')
  theBook = xml.dom.minidom.parse(theBookFile)
  handleWorkBook(theBook, sheetDict, theDoc)
  theBook.unlink()
  
  if 'xl/sharedStrings.xml' in z.namelist():
    theStringFile=z.open('xl/sharedStrings.xml')
    theStrings = xml.dom.minidom.parse(theStringFile)
    handleStrings(theStrings, stringList)
    theStrings.unlink()
  
  for sheetSpec in sheetDict:
    #print "sheetSpec: ", sheetSpec
    theSheet, sheetFile = sheetDict[sheetSpec]
    f=z.open('xl/worksheets/' + sheetFile)
    myDom = xml.dom.minidom.parse(f)
    
    handleWorkSheet(myDom, theSheet, stringList)
    myDom.unlink()
  
  z.close()
  # This is needed more than once, otherwise some references are not calculated!
  theDoc.recompute()
  theDoc.recompute()
  theDoc.recompute()


