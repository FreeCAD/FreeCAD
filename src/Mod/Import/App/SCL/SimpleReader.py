# Copyright (c) 2014, Juergen Riegel (FreeCAD@juergen-riegel.net)
# All rights reserved.

# This file is part of the StepClassLibrary (SCL).
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#   Neither the name of the <ORGANIZATION> nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Simple Part21 STEP reader

Reads a given STEP file. Maps the entities and instantiate the
corresponding classes.
In addition it writes out a graphviz file with the entity graph.
"""

import Part21, sys


__title__ = "Simple Part21 STEP reader"
__author__ = "Juergen Riegel"
__version__ = "0.1 (Jan 2014)"


class SimpleParser:
    """read the file

    Part21.Part21Parser Loads all instances definition of a Part21 file into memory.
    Two dicts are created:
    Part21.Part21Parser._instance_definition : stores attributes, key is the instance integer id
    Part21.Part21Parser._number_of_ancestors : stores the number of ancestors of entity id. This enables
    to define the order of instances creation.
    """

    def __init__(self, filename):
        import time
        import sys

        self._p21loader = Part21.Part21Parser(filename)
        # self._p21loader._number_of_ancestors = {} # not needed, save memory
        self.schemaModule = None
        self.schemaClasses = None
        self.instanceMape = {}
        # for i in self._p21loader._instances_definition.keys():
        #    print i,self._p21loader._instances_definition[i][0],self._p21loader._instances_definition[i][1]

    def _writeGraphVizEdge(self, num, attrList, file):
        for i in attrList:
            if isinstance(i, list):
                self._writeGraphVizEdge(num, i, file)
            elif isinstance(i, str):
                if not i == "" and i[0] == "#":
                    key = int(i[1:])
                    file.write("  " + repr(num) + " -> " + repr(key) + "\n")

    def writeGraphViz(self, fileName):
        print("Writing GraphViz file %s..." % fileName)
        gvFile = open(fileName, "w")

        gvFile.write(
            'digraph G {\n  node [fontname=Verdana,fontsize=12]\n  node [style=filled]\n  node [fillcolor="#EEEEEE"]\n  node [color="#EEEEEE"]\n  edge [color="#31CEF0"]\n'
        )
        for i in list(self._p21loader._instances_definition):
            entityStr = "#" + repr(i)
            nameStr = self._p21loader._instances_definition[i][0].lower()
            sttrStr = (
                repr(self._p21loader._instances_definition[i][1])
                .replace('"', "")
                .replace("'", "")
                .replace(" ", "")
            )
            if len(sttrStr) > 40:
                sttrStr = sttrStr[:39] + "...."
            gvFile.write(
                "  " + repr(i) + ' [label="' + entityStr + "\n" + nameStr + "\n" + sttrStr + '"]\n'
            )
            self._writeGraphVizEdge(i, self._p21loader._instances_definition[i][1], gvFile)
        gvFile.write("}\n")

    def instantiate(self):
        """Instantiate the python class from the entities"""
        import inspect

        # load the needed schema module
        if self._p21loader.get_schema_name() == "config_control_design":
            import config_control_design

            self.schemaModule = config_control_design
        if self._p21loader.get_schema_name() == "automotive_design":
            import automotive_design

            self.schemaModule = automotive_design

        if self.schemaModule:
            self.schemaClasses = dict(inspect.getmembers(self.schemaModule))

        for i in list(self._p21loader._instances_definition):
            # print i
            if i not in self.instanceMape:
                self._create_entity_instance(i)

    def _create_entity_instance(self, instance_id):
        if instance_id in self._p21loader._instances_definition:
            instance_definition = self._p21loader._instances_definition[instance_id]
            # print "Instance definition to process",instance_definition
            # first find class name
            class_name = instance_definition[0].lower()
            # print "Class name:%s"%class_name

            if not class_name == "":
                classDef = self.schemaClasses[class_name]
                # then attributes
                # print object_.__doc__
            instance_attributes = instance_definition[1]
            self._transformAttributes(instance_attributes)
            print("Attribute list after transform: ", instance_attributes)

            self.instanceMape[instance_id] = str(
                "dummy#:" + str(instance_id)
            )  # dummy instance to test
        else:
            print("############################# lost entity: ", instance_id)
            self.instanceMape[instance_id] = int(41)  # dummy
        # print "instance_attributes:",instance_attributes
        # a = object_(*instance_attributes)

    def _transformAttributes(self, attrList):
        n = 0
        for i in attrList:
            if isinstance(i, list):
                self._transformAttributes(i)
            elif isinstance(i, str):
                if i == "":
                    print("empty string")
                elif i[0] == "#":
                    key = int(i[1:])
                    # print 'Item: ',int(i[1:])
                    if key in self.instanceMape:
                        attrList[n] = self.instanceMape[key]
                    else:
                        self._create_entity_instance(key)
                        if key not in self.instanceMape:
                            raise NameError("Needed instance not instantiated: ", key)
                        else:
                            attrList[n] = self.instanceMape[key]
                elif i[0] == "$":
                    # print 'Dollar'
                    pass
                elif i[0] == "'":
                    print("Dopelstring: ", i[1:-1])
                else:
                    print("String: ", i)
            else:
                raise NameError("Unknown attribute type")
            n = n + 1


if __name__ == "__main__":
    sys.path.append("..")  # path where config_control_design.py is found
    parser = SimpleReader("Aufspannung.stp")  # simple test file
    # parser.instantiate()
    parser.writeGraphViz("TestGrap.gv")
    # dot.exe -Tsvg -o Test.svg e:\fem-dev\src\Mod\Import\App\SCL\TestGrap-geo.gv
