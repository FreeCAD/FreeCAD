#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 Ulrich Brammer <ulrich1a@users.sourceforge.net>    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# ***************************************************************************/


__title__ = "FreeCAD Spreadsheet Workbench - XLSX importer"
__author__ = "Ulrich Brammer <ulrich1a@users.sourceforge.net>"
__url__ = ["https://www.freecad.org"]

"""
This library imports an Excel-XLSX-file into FreeCAD.

Version 1.1, Nov. 2016:
Changed parser, adds rad-unit to trigonometric functions in order
to give the same result in FreeCAD.
Added factor to arcus-function in order to give the same result in FreeCAD
Added support for celltype "inlineStr"

Version 1.0:
It uses a minimal parser, in order to translate the IF-function into
the different FreeCAD version.
The other function-names are translated by search and replace.
Features:
- Imports tables defined inside Excel-document
- Set alias definitions
- Translate formulas known by FreeCAD. (see tokenDic as by version 1.1)
- set cross table references
- strings are imported
- references to cells with strings are working

known issues:
- units are not imported
- string support is minimal, the same as in FreeCAD
"""


import zipfile
import xml.dom.minidom
import FreeCAD as App

try:
    import FreeCADGui
except ValueError:
    gui = False
else:
    gui = True

if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


# The sepToken structure is used in the tokenizer functions isKey and
# getNextToken.
# sepToken defines a search tree for separator tokens with length of 1 to 3 characters
# it is also used as a list of separators between other tokens.
sepToken = {
    "(": None,
    "=": None,
    "<": "branchLower",
    ">": "branchHigher",
    ")": None,
    #  '"':None,
    #  ';':None,
    " ": None,
    ",": None,  # Separator on lists
    "!": None,  # Connector to cells on other Sheets
    "+": None,
    "-": None,
    "*": None,
    "/": None,
    "^": None,
}

branchLower = {">": None, "=": None}

branchHigher = {"=": None}


# Needed to get a reference from a string to a dict
treeDict = {"branchLower": branchLower, "branchHigher": branchHigher}

# The tokenDic is used in parseExpr.
# The tokenDic contains the following information:
# levelchange: -1: tree down, 0, +1: tree up
# replacement token
# function-state: needed to do something special in the parser
#     0 = normal, 1 = the pi-case, 2 = angle-function,
#     3 = IF-function, 4 = IF-truecase, 5 IF-falsecase


tokenDic = {
    "(": (1, "(", 0),
    "=": (0, "==", 0),
    "<>": (0, "!=", 0),
    ">=": (0, ">=", 0),
    "<=": (0, "<=", 0),
    "<": (0, "<", 0),
    ">": (0, ">", 0),
    ",": (0, ",", 0),
    ")": (-1, ")", 0),
    "!": (0, ".", 0),  # Connector to cells on other Sheets
    #  '"'  :( 2 ,'',  0),
    "+": (0, "+", 0),
    "-": (0, "-", 0),
    "*": (0, "*", 0),
    "/": (0, "/", 0),
    "^": (0, "^", 0),
    "IF": (0, "", 3),
    "ABS": (0, "abs", 0),
    "ACOS": (0, "pi/180deg*acos", 0),
    "ASIN": (0, "pi/180deg*asin", 0),
    "ATAN": (0, "pi/180deg*atan", 0),
    "ATAN2": (0, "pi/180deg*atan2", 0),
    "COS": (0, "cos", 2),
    "COSH": (0, "cosh", 2),
    "EXP": (0, "exp", 0),
    "LOG": (0, "log", 0),
    "LOG10": (0, "log10", 0),
    "MOD": (0, "mod", 0),
    "POWER": (0, "pow", 0),
    "SIN": (0, "sin", 2),
    "SINH": (0, "sinh", 2),
    "SQRT": (0, "sqrt", 0),
    "TAN": (0, "tan", 2),
    "TANH": (0, "tanh", 2),
    "AVERAGE": (0, "average", 0),
    "COUNT": (0, "count", 0),
    "MAX": (0, "max", 0),
    "MIN": (0, "min", 0),
    "STDEVA": (0, "stddev", 0),
    "SUM": (0, "sum", 0),
    "PI": (0, "pi", 1),
    "_xlfn.CEILING.MATH": (0, "ceil", 0),
    "_xlfn.FLOOR.MATH": (0, "floor", 0),
}


class exprNode(object):
    """This defines a tree class for expression parsing.
    A tree is built, to step down into the levels of the expression."""

    def __init__(self, parent, state, actIndex):
        self.state = state  # see comment: State used for Angle-functions and IF-function
        self.parent = parent  # Parent tree node
        self.lIndex = actIndex  # Index to the list of tokens
        self.result = ""


class FormulaTranslator(object):
    """This class translates a cell-formula from Excel to FreeCAD."""

    def __init__(self):
        self.tokenList = ["="]

    def translateForm(self, actExpr):
        self.getNextToken(actExpr)
        # print("tokenList: ", self.tokenList)
        self.resultTree = exprNode(None, 0, 1)
        self.resultTree.result = self.tokenList[0]
        self.parseExpr(self.resultTree)
        # print('parseResult: ', self.resultTree.result)
        return self.resultTree.result

    def getNextToken(self, theExpr):
        """This is the recursive tokenizer for an excel formula.
        It appends all identified tokens to self.tokenList."""
        # print('next Token theExpr: ', theExpr)
        # print('actTList: ', self.tokenList)
        tokenComplete = False
        keyToken = False
        if len(theExpr) > 0:
            theTok = theExpr[0]
            theExpr = theExpr[1:]
            if theTok in sepToken:
                keyToken = True
                branch = sepToken[theTok]
                while branch:
                    # print(branch, ' theExpr[0]: ',theExpr[0])
                    if theExpr[0] in treeDict[branch]:
                        branch = treeDict[branch][theExpr[0]]
                        theTok = theTok + theExpr[0]
                        theExpr = theExpr[1:]
                    else:
                        branch = None
                tokenComplete = True
                self.tokenList.append(theTok)
                self.getNextToken(theExpr)
            else:
                if len(theExpr) > 0:
                    while not tokenComplete:
                        if not self.isKey(theExpr):
                            theTok = theTok + theExpr[0]
                            theExpr = theExpr[1:]
                            if len(theExpr) == 0:
                                tokenComplete = True
                        else:
                            tokenComplete = True
                self.tokenList.append(theTok)
                self.getNextToken(theExpr)

    def isKey(self, theExpr):
        # print('look up: ', theExpr)
        keyToken = False
        lenExpr = len(theExpr)
        if theExpr[0] in sepToken:
            branch = sepToken[theExpr[0]]

            if branch is None:
                keyToken = True
            else:
                # print('There is a branch. look up: ', theExpr[1])
                if (lenExpr > 1) and (theExpr[1] in treeDict[branch]):
                    branch = treeDict[branch][theExpr[0]]
                    if branch is None:
                        keyToken = True
                    else:
                        if (lenExpr > 2) and (theExpr[2] in treeDict[branch]):
                            keyToken = True
                else:
                    keyToken = True
        return keyToken

    def parseExpr(self, treeNode):
        token = self.tokenList[treeNode.lIndex]
        treeNode.lIndex += 1
        if token in tokenDic:
            lChange, newToken, funcState = tokenDic[token]
        else:
            lChange = 0
            newToken = token
            funcState = 0
        # print('treeNode.state: ', treeNode.state, ' my.index: ', treeNode.lIndex-1, ' ', token, ' fState: ', funcState)

        if token == ",":
            if treeNode.state == 4:
                newToken = ":"
                treeNode.state = 6
            if treeNode.state == 3:
                newToken = "?"
                treeNode.state = 4

        if funcState == 3:
            funcState = 0
            newNode = exprNode(treeNode, 3, treeNode.lIndex)
            self.parseIF(newNode)
        else:
            treeNode.result = treeNode.result + newToken

        if funcState == 2:
            funcState = 0
            newNode = exprNode(treeNode, 2, treeNode.lIndex)
            self.parseAngle(newNode)
            treeNode.result = treeNode.result + ")"
        elif funcState == 1:
            treeNode.lIndex += 2  # do skip the 2 parentheses of the PI()

        if lChange == -1:
            # print 'state: ', treeNode.state, 'parent.result: ', treeNode.parent.result, ' mine: ', treeNode.result
            treeNode.parent.result = treeNode.parent.result + treeNode.result
            treeNode.parent.lIndex = treeNode.lIndex
            # print('Go one level up, state: ', treeNode.state)
            if treeNode.state < 2:
                # print(' Look up more token above')
                if treeNode.lIndex < len(self.tokenList):
                    self.parseExpr(treeNode.parent)

        elif lChange == 1:
            # print('Go one level down')
            newNode = exprNode(treeNode, 1, treeNode.lIndex)
            self.parseExpr(newNode)
            treeNode.lIndex = newNode.lIndex
        else:
            if treeNode.lIndex < len(self.tokenList):
                # print('parse to the end')
                self.parseExpr(treeNode)

    def parseIF(self, treeNode):
        # print('IF state: ', treeNode.state)
        treeNode.result = treeNode.result + "("
        treeNode.lIndex += 1
        self.parseExpr(treeNode)
        # print('IF result: ', treeNode.result)
        return

    def parseAngle(self, treeNode):
        # print('Angle state: ', treeNode.state)
        treeNode.result = treeNode.result + "(1rad*("
        treeNode.lIndex += 1
        self.parseExpr(treeNode)
        # print('angle result: ', treeNode.result)


def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
        return "".join(rc)


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
            cellType = "n"  # FIXME: some cells don't have t and s attributes

        # print("reference: ", ref, ' Cell type: ', cellType)

        if cellType == "inlineStr":
            iStringList = cell.getElementsByTagName("is")
            # print('iString: ', iStringList)
            for stringEle in iStringList:
                tElement = stringEle.getElementsByTagName("t")[0]
                theString = getText(tElement.childNodes)

                # print('theString: ', theString)
                actCellSheet.set(ref, theString)

        formulaRef = cell.getElementsByTagName("f")
        if len(formulaRef) == 1:
            theFormula = getText(formulaRef[0].childNodes)
            # print("theFormula: ", theFormula)
            fTrans = FormulaTranslator()
            actCellSheet.set(ref, fTrans.translateForm(theFormula))

        else:
            valueRef = cell.getElementsByTagName("v")
            # print('valueRef: ', valueRef)
            if len(valueRef) == 1:
                valueRef = cell.getElementsByTagName("v")[0]
                if valueRef:
                    theValue = getText(valueRef.childNodes)
                    # print("theValue: ", theValue)
                    if cellType == "n":
                        actCellSheet.set(ref, theValue)
                    if cellType == "s":
                        actCellSheet.set(ref, (sList[int(theValue)]))


def handleWorkBook(theBook, sheetDict, Doc):
    theSheets = theBook.getElementsByTagName("sheet")
    # print("theSheets: ", theSheets)
    for sheet in theSheets:
        sheetAtts = sheet.attributes
        nameRef = sheetAtts.getNamedItem("name")
        sheetName = getText(nameRef.childNodes)
        # print("table name: ", sheetName)
        idRef = sheetAtts.getNamedItem("sheetId")
        sheetFile = "sheet" + getText(idRef.childNodes) + ".xml"
        # print("sheetFile: ", sheetFile)
        # add FreeCAD-spreadsheet
        sheetDict[sheetName] = (Doc.addObject("Spreadsheet::Sheet", sheetName), sheetFile)

    theAliases = theBook.getElementsByTagName("definedName")
    for theAlias in theAliases:
        aliAtts = theAlias.attributes
        nameRef = aliAtts.getNamedItem("name")
        aliasName = getText(nameRef.childNodes)
        # print("aliasName: ", aliasName)

        aliasRef = getText(theAlias.childNodes)  # aliasRef can be None
        if aliasRef and "$" in aliasRef:
            refList = aliasRef.split("!$")
            adressList = refList[1].split("$")
            # print("aliasRef: ", aliasRef)
            # print('Sheet Name: ', refList[0])
            # print('Adress: ', adressList[0] + adressList[1])
            actSheet, sheetFile = sheetDict[refList[0]]
            actSheet.setAlias(adressList[0] + adressList[1], aliasName)


def handleStrings(theStr, sList):
    print("process Strings: ")
    stringElements = theStr.getElementsByTagName("t")
    for sElem in stringElements:
        print("string: ", getText(sElem.childNodes))
        sList.append(getText(sElem.childNodes))


def open(nameXLSX):

    if len(nameXLSX) > 0:
        z = zipfile.ZipFile(nameXLSX)

        theDoc = App.newDocument()

        sheetDict = dict()
        stringList = []

        theBookFile = z.open("xl/workbook.xml")
        theBook = xml.dom.minidom.parse(theBookFile)
        handleWorkBook(theBook, sheetDict, theDoc)
        theBook.unlink()

        if "xl/sharedStrings.xml" in z.namelist():
            theStringFile = z.open("xl/sharedStrings.xml")
            theStrings = xml.dom.minidom.parse(theStringFile)
            handleStrings(theStrings, stringList)
            theStrings.unlink()

        for sheetSpec in sheetDict:
            # print("sheetSpec: ", sheetSpec)
            theSheet, sheetFile = sheetDict[sheetSpec]
            f = z.open("xl/worksheets/" + sheetFile)
            myDom = xml.dom.minidom.parse(f)

            handleWorkSheet(myDom, theSheet, stringList)
            myDom.unlink()

        z.close()
        # This is needed more than once, otherwise some references are not calculated!
        theDoc.recompute()
        theDoc.recompute()
        theDoc.recompute()
        return theDoc


def insert(nameXLSX, docname):
    try:
        theDoc = App.getDocument(docname)
    except NameError:
        theDoc = App.newDocument(docname)
    App.ActiveDocument = theDoc

    sheetDict = dict()
    stringList = []

    z = zipfile.ZipFile(nameXLSX)
    theBookFile = z.open("xl/workbook.xml")
    theBook = xml.dom.minidom.parse(theBookFile)
    handleWorkBook(theBook, sheetDict, theDoc)
    theBook.unlink()

    if "xl/sharedStrings.xml" in z.namelist():
        theStringFile = z.open("xl/sharedStrings.xml")
        theStrings = xml.dom.minidom.parse(theStringFile)
        handleStrings(theStrings, stringList)
        theStrings.unlink()

    for sheetSpec in sheetDict:
        # print("sheetSpec: ", sheetSpec)
        theSheet, sheetFile = sheetDict[sheetSpec]
        f = z.open("xl/worksheets/" + sheetFile)
        myDom = xml.dom.minidom.parse(f)

        handleWorkSheet(myDom, theSheet, stringList)
        myDom.unlink()

    z.close()
    # This is needed more than once, otherwise some references are not calculated!
    theDoc.recompute()
    theDoc.recompute()
    theDoc.recompute()
