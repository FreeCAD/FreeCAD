# PlmXmlParser
# ***************************************************************************
# *   Copyright (c) 2015 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/


import xml.etree.ElementTree as ET

FreeCAD_On = False
FreeCAD_Doc = None
FreeCAD_ObjList = []


def ParseUserData(element):
    res = {}
    for i in element.findall("{http://www.plmxml.org/Schemas/PLMXMLSchema}UserData"):
        for value in i.findall("{http://www.plmxml.org/Schemas/PLMXMLSchema}UserValue"):
            res[value.attrib["title"]] = value.attrib["value"]
    return res


def addPart(partElement):
    global FreeCAD_On, FreeCAD_Doc, FreeCAD_ObjList
    print("=== Part ======================================================")
    name = partElement.attrib["name"]
    id = partElement.attrib["id"]
    userData = ParseUserData(partElement)

    bound = partElement.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}Bound")
    print(bound.attrib["values"])

    representation = partElement.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}Representation")
    format = representation.attrib["format"]
    location = representation.attrib["location"]

    print(id, name, userData, format, location)
    if FreeCAD_On:
        import FreeCAD, Assembly

        print("Create Reference")
        partObject = FreeCAD_Doc.addObject("App::Part", id)
        FreeCAD_ObjList.append(partObject)
        partObject.Label = name
        partObject.Meta = userData


def addAssembly(asmElement):
    global FreeCAD_On, FreeCAD_Doc, FreeCAD_ObjList
    print("=== Assembly ======================================================")
    userData = ParseUserData(asmElement)
    name = asmElement.attrib["name"]
    id = asmElement.attrib["id"]
    instanceRefs = asmElement.attrib["instanceRefs"]
    userData["instanceRefs"] = instanceRefs

    print(id, name, instanceRefs, userData)
    if FreeCAD_On:
        import FreeCAD, Assembly

        print("Create Reference")
        admObject = FreeCAD_Doc.addObject("Assembly::Product", id)
        FreeCAD_ObjList.append(admObject)
        admObject.Label = name
        admObject.Meta = userData


def addReference(refElement):
    global FreeCAD_On, FreeCAD_Doc, FreeCAD_ObjList
    print("=== Reference ======================================================")
    userData = ParseUserData(refElement)
    partRef = refElement.attrib["partRef"][1:]
    userData["partRef"] = partRef
    id = refElement.attrib["id"]
    name = refElement.attrib["name"]
    transform = refElement.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}Transform")
    mtrx = [float(i) for i in transform.text.split(" ")]
    print(mtrx)
    print(id, name, partRef)

    if FreeCAD_On:
        import FreeCAD, Assembly

        print("Create Reference")
        refObject = FreeCAD_Doc.addObject("Assembly::ProductRef", id)
        FreeCAD_ObjList.append(refObject)
        refObject.Label = name
        refObject.Meta = userData


def resolveRefs():
    global FreeCAD_On, FreeCAD_Doc, FreeCAD_ObjList
    print("=== Resolve References ======================================================")
    if FreeCAD_On:
        for i in FreeCAD_ObjList:
            if i.TypeId == "Assembly::Product":
                objectList = []
                for l in i.Meta["instanceRefs"].split(" "):
                    objectList.append(FreeCAD_Doc.getObject(l))
                i.Items = objectList
            if i.TypeId == "Assembly::ProductRef":
                i.Item = FreeCAD_Doc.getObject(i.Meta["partRef"])


def open(fileName):
    """called when freecad opens an PlmXml file"""
    global FreeCAD_On, FreeCAD_Doc
    import FreeCAD, os

    docname = os.path.splitext(os.path.basename(fileName))[0]
    doc = FreeCAD.newDocument(docname)
    message = 'Started with opening of "' + fileName + '" file\n'
    FreeCAD.Console.PrintMessage(message)
    FreeCAD_Doc = doc
    FreeCAD_On = True
    parse(fileName)
    resolveRefs()


def insert(filename, docname):
    """called when freecad imports an PlmXml file"""
    global FreeCAD_On, FreeCAD_Doc
    import FreeCAD

    FreeCAD.setActiveDocument(docname)
    doc = FreeCAD.getDocument(docname)
    FreeCAD.Console.PrintMessage('Started import of "' + filename + '" file')
    FreeCAD_Doc = doc
    FreeCAD_On = True
    parse(fileName)
    resolveRefs()


def main():
    parse("../../../../data/tests/Jt/Engine/2_Cylinder_Engine3.plmxml")


def parse(fileName):

    tree = ET.parse(fileName)
    root = tree.getroot()
    ProductDef = root.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductDef")

    res = ParseUserData(ProductDef.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}UserData"))

    InstanceGraph = ProductDef.find("{http://www.plmxml.org/Schemas/PLMXMLSchema}InstanceGraph")

    # get all the special elements we can read
    Instances = InstanceGraph.findall("{http://www.plmxml.org/Schemas/PLMXMLSchema}Instance")
    Parts = InstanceGraph.findall("{http://www.plmxml.org/Schemas/PLMXMLSchema}Part")
    ProductInstances = InstanceGraph.findall(
        "{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductInstance"
    )
    ProductRevisionViews = InstanceGraph.findall(
        "{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductRevisionView"
    )

    instanceTypesSet = set()
    for child in InstanceGraph:
        instanceTypesSet.add(child.tag)
    print("All types below the InstanceGraph:")
    for i in instanceTypesSet:
        print(i)
    print("")

    print(len(Instances), "\t{http://www.plmxml.org/Schemas/PLMXMLSchema}Instance")
    print(len(Parts), "\t{http://www.plmxml.org/Schemas/PLMXMLSchema}Part")
    print(len(ProductInstances), "\t{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductInstance")
    print(
        len(ProductRevisionViews),
        "\t{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductRevisionView",
    )

    # handle all instances
    for child in Instances:
        addReference(child)

    # handle the parts and assemblies
    for child in Parts:
        if "type" in child.attrib:
            if child.attrib["type"] == "solid":
                addPart(child)
                continue
            if child.attrib["type"] == "assembly":
                addAssembly(child)
                continue
            print("Unknown Part type:", child)
        else:
            print("not Type in Part", child)


if __name__ == "__main__":
    main()
