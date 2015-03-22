# PlmXmlParser
#***************************************************************************
#*   (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2015                  *
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
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/


import xml.etree.ElementTree as ET

def ParseUserData(element):
    if element:
        res = {}
        for value in element.findall('{http://www.plmxml.org/Schemas/PLMXMLSchema}UserValue'):
            res[value.attrib['title']] = value.attrib['value']
        return res
    return None

def addPart(partElement):
    #print partElement.attrib
    pass

def addAssembly(asmElement):
    #print asmElement.attrib
    pass

def addReference(refElement):
    #print refElement.attrib
    pass


def main():

    tree = ET.parse('../../../../data/tests/Jt/Engine/2_Cylinder_Engine3.plmxml')
    root = tree.getroot()
    ProductDef = root.find('{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductDef')

    ParseUserData(ProductDef.find('{http://www.plmxml.org/Schemas/PLMXMLSchema}UserData'))

    InstanceGraph = ProductDef.find('{http://www.plmxml.org/Schemas/PLMXMLSchema}InstanceGraph')

    # get all the special elements we can read
    Instances = InstanceGraph.findall('{http://www.plmxml.org/Schemas/PLMXMLSchema}Instance')
    Parts = InstanceGraph.findall('{http://www.plmxml.org/Schemas/PLMXMLSchema}Part')
    ProductInstances = InstanceGraph.findall('{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductInstance')
    ProductRevisionViews = InstanceGraph.findall('{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductRevisionView')

    instanceTypesSet = set()
    for child in InstanceGraph:
        instanceTypesSet.add(child.tag)
    print "All types below the InstanceGraph:"
    for i in instanceTypesSet:
        print i
    print ""

    print len(Instances),'\t{http://www.plmxml.org/Schemas/PLMXMLSchema}Instance'
    print len(Parts),'\t{http://www.plmxml.org/Schemas/PLMXMLSchema}Part'
    print len(ProductInstances),'\t{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductInstance'
    print len(ProductRevisionViews),'\t{http://www.plmxml.org/Schemas/PLMXMLSchema}ProductRevisionView'

    # handle all instances
    for child in Instances:
        addReference(child)

    #handle the parts and assemblies
    for child in Parts:
        if 'type' in child.attrib:
            if child.attrib['type'] == 'solid' :
                addPart(child)
                continue
            if child.attrib['type'] == 'assembly' :
                addPart(child)
                continue
            print "Unknown Part type:",child
        else:
            print "not Type in Part", child


if __name__ == '__main__':
    main()
