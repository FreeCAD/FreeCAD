# Copyright (c) 2011, Thomas Paviot (tpaviot@gmail.com)
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

import re
from . import Utils
import time


INSTANCE_DEFINITION_RE = re.compile("#(\d+)[^\S\n]?=[^\S\n]?(.*?)\((.*)\)[^\S\n]?;[\\r]?$")


def map_string_to_num(stri):
    """Take a string, check whether it is an integer, a float or not"""
    if ("." in stri) or ("E" in stri):  # it's definitely a float
        return REAL(stri)
    else:
        return INTEGER(stri)


class Model:
    """
    A model contains a list of instances
    """

    def __init__(self, name):
        self._name = name
        # a dict of instances
        # each time an instance is added to the model, count is incremented
        self._instances = {}
        self._number_of_instances = 0

    def add_instance(self, instance):
        """
        Adds an instance to the model
        """
        self._number_of_instances += 1
        self._instances[self._number_of_instances - 1] = instance

    def print_instances(self):
        """
        Dump instances to stdout
        """
        for idx in range(self._number_of_instances):
            "=========="
            print("Instance #%i" % (idx + 1))
            print(self._instances[idx])


class Part21EntityInstance:
    """
    A class to represent a Part21 instance as defined in one Part21 file
    A Part21EntityInstance is defined by the following arguments:
    entity_name: a string
    entity_attributes: a list of strings to represent an attribute.
    For instance, the following expression:
    #4 = PRODUCT_DEFINITION_SHAPE('$','$',#5);
    will result in :
    entity : <class 'config_control_design.product_definition_shape'>
    entity_instance_attributes: ['$','$','#5']
    """

    def __init__(self, entity_name, attributes):
        self._entity
        self._attributes_definition = attributes
        print(self._entity_name)
        print(self._attributes_definition)


class Part21Parser:
    """
    Loads all instances definition of a Part21 file into memory.
    Two dicts are created:
    self._instance_definition : stores attributes, key is the instance integer id
    self._number_of_ancestors : stores the number of ancestors of entity id. This enables
    to define the order of instances creation.
    """

    def __init__(self, filename):
        self._filename = filename
        # the schema
        self._schema_name = ""
        # the dict self._instances contain instance definition
        self._instances_definition = {}
        # this dict contains lists of 0 ancestors, 1 ancestor, etc.
        # initializes this dict
        # self._number_of_ancestors = {} # this kind of sorting don't work on non-trivial files
        # for i in range(2000):
        #    self._number_of_ancestors[i]=[]
        self.parse_file()
        # reduce number_of_ancestors dict
        # for item in self._number_of_ancestors.keys():
        #    if len(self._number_of_ancestors[item])==0:
        #        del self._number_of_ancestors[item]

    def get_schema_name(self):
        print(schema_name)
        return self._schema_name

    def get_number_of_instances(self):
        return len(self._instances_definition)

    def parse_file(self):
        init_time = time.time()
        print("Parsing file %s..." % self._filename)
        fp = open(self._filename)
        while True:
            line = fp.readline()
            if not line:
                break
            # there may be a multiline definition. In this case, we read lines until we found
            # a ;
            while line.find(";") == -1:  # it's a multiline
                line = line.replace("\n", "").replace("\r", "") + fp.readline()
            # parse line
            match_instance_definition = INSTANCE_DEFINITION_RE.search(line)  # id,name,attrs
            if match_instance_definition:
                instance_id, entity_name, entity_attrs = match_instance_definition.groups()
                instance_int_id = int(instance_id)
                # find number of ancestors
                # number_of_ancestors = entity_attrs.count('#')
                # fill number of ancestors dict
                # self._number_of_ancestors[number_of_ancestors].append(instance_int_id) # this kind of sorting don't work on non-trivial files
                # parse attributes string
                entity_attrs_list, str_len = Utils.process_nested_parent_str(entity_attrs)
                # then finally append this instance to the disct instance
                self._instances_definition[instance_int_id] = (entity_name, entity_attrs_list)
            else:  # does not match with entity instance definition, parse the header
                if line.startswith("FILE_SCHEMA"):
                    # identify the schema name
                    self._schema_name = line.split("'")[1].split("'")[0].split(" ")[0].lower()
        fp.close()
        print("done in %fs." % (time.time() - init_time))
        print("schema: - %s entities %i" % (self._schema_name, len(self._instances_definition)))


class EntityInstancesFactory(object):
    """
    This class creates entity instances from the str definition
    For instance, the definition:
    20: ('CARTESIAN_POINT', ["''", '(5.,125.,20.)'])
    will result in:
    p = ARRAY(1,3,REAL)
    p.[1] = REAL(5)
    p.[2] = REAL(125)
    p.[3] = REAL(20)
    new_instance = cartesian_point(STRING(''),p)
    """

    def __init__(self, schema_name, instance_definition):
        # First try to import the schema module
        pass


class Part21Population(object):
    def __init__(self, part21_loader):
        """Take a part21_loader a tries to create entities"""
        self._part21_loader = part21_loader
        self._aggregate_scope = []
        self._aggr_scope = False
        self.create_entity_instances()

    def create_entity_instances(self):
        """Starts entity instances creation"""
        for number_of_ancestor in list(self._part21_loader._number_of_ancestors):
            for entity_definition_id in self._part21_loader._number_of_ancestors[
                number_of_ancestor
            ]:
                self.create_entity_instance(entity_definition_id)

    def create_entity_instance(self, instance_id):
        instance_definition = self._part21_loader._instances_definition[instance_id]
        print("Instance definition to process", instance_definition)
        # first find class name
        class_name = instance_definition[0].lower()
        print("Class name:%s" % class_name)
        object_ = globals()[class_name]
        # then attributes
        # print object_.__doc__
        instance_attributes = instance_definition[1]
        print("instance_attributes:", instance_attributes)
        a = object_(*instance_attributes)


if __name__ == "__main__":
    import time
    import sys
    from config_control_design import *

    p21loader = Part21Parser("gasket1.p21")
    print("Creating instances")
    p21population = Part21Population(p21loader)
