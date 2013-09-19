#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre, Marijn van Aerle                                     *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import os, re, copy

__title__="FreeCAD IFC parser"
__author__ = "Yorik van Havre, Marijn van Aerle"
__url__ = "http://www.freecadweb.org"

'''
FreeCAD IFC parser, by Yorik van Havre, based on work by Marijn van Aerle

Usage:
        import ifcReader
        ifcdoc = ifcReader.IfcDocument("path/to/file.ifc")
        print ifcdoc.Entities
        myent = ifcdoc.Entities[20] # returns one entity
        myent = ifcdoc.getEnt(20) # alternative way
        polylines = ifcdoc.getEnt("IFCPOLYLINE") # returns a list
        print myent.attributes

The ifc document contains a list of entities, that can be retrieved
by iterating the list (indices corresponds to the entities ids)
or by using the getEnt() method. All entities have id, type
and attributes. Attributes can have values such as text or number,
or a link to another entity.

Important note:

1) For this reader to function, you need an IFC Schema Express file (.exp)
available here:
http://www.steptools.com/support/stdev_docs/express/ifc2x3/ifc2x3_tc1.exp
For licensing reasons we are not allowed to ship that file with FreeCAD.
Just place the .exp file together with this script.

2) IFC files can have ordered content (ordered list, no entity number missing)
or be much messier (entity numbers missing, etc). The performance of the reader
will be drastically different.
'''

IFCLINE_RE = re.compile("#(\d+)[ ]?=[ ]?(.*?)\((.*)\);[\\r]?$")
DEBUG = False

class IfcSchema:
    SIMPLETYPES = ["INTEGER", "REAL", "STRING", "NUMBER", "LOGICAL", "BOOLEAN"]
    NO_ATTR = ["WHERE", "INVERSE","WR2","WR3", "WR4", "WR5", "UNIQUE", "DERIVE"]

    def __init__(self, filename):
        self.filename = filename
        if not os.path.exists(filename):
            raise ImportError("no IFCSchema file found!")
        else:
            self.file = open(self.filename)
            self.data = self.file.read()
            self.types = self.readTypes()
            self.entities = self.readEntities()
            if DEBUG: print "Parsed from schema %s: %s entities and %s types" % (self.filename, len(self.entities), len(self.types))

    def readTypes(self):
        """
        Parse all the possible types from the schema, 
        returns a dictionary Name -> Type
        """
        types = {}
        for m in re.finditer("TYPE (.*) = (.*);", self.data):
            typename, typetype = m.groups() 
            if typetype in self.SIMPLETYPES:
                types[typename] = typetype
            else:
                types[typename] = "#" + typetype
                
        return types
        
    def readEntities(self):
        """
        Parse all the possible entities from the schema,
        returns a dictionary of the form:
        { name: { 
            "supertype": supertype, 
            "attributes": [{ key: value }, ..]
        }}  
        """
        entities = {}
        
        # Regexes must be greedy to prevent matching outer entity and end_entity strings
        # Regexes have re.DOTALL to match newlines
        for m in re.finditer("ENTITY (.*?)END_ENTITY;", self.data, re.DOTALL):
            entity = {}
            raw_entity_str = m.groups()[0]

            entity["name"] = re.search("(.*?)[;|\s]", raw_entity_str).groups()[0].upper()

            subtypeofmatch = re.search(".*SUBTYPE OF \((.*?)\);", raw_entity_str)
            entity["supertype"] = subtypeofmatch.groups()[0].upper() if subtypeofmatch else None

            # find the shortest string matched from the end of the entity type header to the
            # first occurence of a NO_ATTR string (when it occurs on a new line)
            inner_str = re.search(";(.*?)$", raw_entity_str, re.DOTALL).groups()[0]            

            attrs_str = min([inner_str.partition("\r\n "+a)[0] for a in self.NO_ATTR])
            attrs = []
            for am in re.finditer("(.*?) : (.*?);", attrs_str, re.DOTALL):
                name, attr_type = [s.replace("\r\n\t","") for s in am.groups()]
                attrs.append((name, attr_type))
            
            entity["attributes"] = attrs
            entities[entity["name"]] = entity
        

        return entities

    def getAttributes(self, name):
        """
        Get all attributes af an entity, including supertypes
        """
        ent = self.entities[name]

        attrs = []
        while ent != None:
            this_ent_attrs = copy.copy(ent["attributes"])
            this_ent_attrs.reverse()
            attrs.extend(this_ent_attrs)
            ent = self.entities.get(ent["supertype"], None)

        attrs.reverse()
        return attrs

class IfcFile:
    """
    Parses an ifc file given by filename, entities can be retrieved by name and id
    The whole file is stored in a dictionary (in memory)
    """
    
    entsById = {}
    entsByName = {}

    def __init__(self, filename,schema):
        self.filename = filename
        self.schema = IfcSchema(schema)
        self.file = open(self.filename)
        self.entById, self.entsByName, self.header = self.read()
        self.file.close()
        if DEBUG: print "Parsed from file %s: %s entities" % (self.filename, len(self.entById))
    
    def getEntityById(self, id):
        return self.entById.get(id, None)
    
    def getEntitiesByName(self, name):
        return self.entsByName.get(name, None)

    def read(self):
        """
        Returns 2 dictionaries, entById and entsByName
        """
        entById = {}
        entsByName = {}
        header = 'HEADER '
        readheader = False
        for line in self.file:
            e = self.parseLine(line)
            if e:
                entById[int(e["id"])] = e
                ids = e.get(e["name"],[])
                ids.append(e["id"])
                entsByName[e["name"]] = list(set(ids))
            elif 'HEADER' in line:
                readheader = True
            elif readheader:
                if 'ENDSEC' in line:
                    readheader = False
                else:
                    header += line
                    
        return [entById, entsByName, header]

    def parseLine(self, line):
        """
        Parse a line 
        """ 
        m = IFCLINE_RE.search(line)  # id,name,attrs
        if m:
            id, name, attrs = m.groups()
            id = id.strip()
            name = name.strip()
            attrs = attrs.strip()
        else:
            return False
        
        return {"id": id, "name": name, "attributes": self.parseAttributes(name, attrs)}

    def parseAttributes(self, ent_name, attrs_str):
        """
        Parse the attributes of a line
        """
        parts = []
        lastpos = 0
        
        while lastpos < len(attrs_str):
            newpos = self.nextString(attrs_str, lastpos)
            parts.extend(self.parseAttribute(attrs_str[lastpos:newpos-1]))
            lastpos = newpos
        
        schema_attributes = self.schema.getAttributes(ent_name)

        assert len(schema_attributes) == len(parts), \
            "Expected %s attributes, got %s (entity: %s" % \
            (len(schema_attributes), len(parts), ent_name)
        
        attribute_names = [a[0] for a in schema_attributes]
        
        return dict(zip(attribute_names, parts))

    def parseAttribute(self, attr_str):
        """
        Map a single attribute to a python type (recursively)
        """
        parts = []
        lastpos = 0
        while lastpos < len(attr_str):
            newpos = self.nextString(attr_str, lastpos)
            s = attr_str[lastpos:newpos-1]
            if (s[0] == "(" and s[-1] == ")"): # list, recurse
                parts.append(self.parseAttribute(s[1:-1]))
            else:
                try:
                    parts.append(float(s)) # number, any kind
                except ValueError:
                    if s[0] == "'" and s[-1] == "'": # string
                        parts.append(s[1:-1])
                    elif s == "$":
                        parts.append(None)
                    else:
                        parts.append(s) # ref, enum or other

            lastpos = newpos
        
        return parts


    def nextString(self, s, start):
        """
        Parse the data part of a line
        """
        parens = 0
        quotes = 0

        for pos in range(start,len(s)):
            c = s[pos]
            if c == "," and parens == 0 and quotes == 0:
                return pos+1
            elif c == "(" and quotes == 0:
                parens += 1
            elif c == ")" and quotes == 0:
                parens -= 1
            elif c == "\'" and quotes == 0:
                quotes = 1
            elif c =="\'" and quotes == 1:
                quotes = 0
            
        return len(s)+1                  

class IfcEntity:
    "a container for an IFC entity"
    def __init__(self,ent,doc=None):
        self.data = ent
        self.id = int(ent['id'])
        self.type = ent['name'].upper().strip(",[]()")
        self.attributes = ent['attributes']
        self.doc = doc

    def __repr__(self):
        return str(self.id) + ' : ' + self.type + ' ' + str(self.attributes)

    def getProperty(self,propName):
        "finds the value of the given property or quantity in this object, if exists"
        propsets = self.doc.find('IFCRELDEFINESBYPROPERTIES','RelatedObjects',self)
        if not propsets: return None
        propset = []
        for p in propsets:
            if hasattr(p.RelatingPropertyDefinition,"HasProperties"):
                propset.extend(p.RelatingPropertyDefinition.HasProperties)
            elif hasattr(p.RelatingPropertyDefinition,"Quantities"):
                propset.extend(p.RelatingPropertyDefinition.Quantities)
        for prop in propset:
            if prop.Name == propName:
                print "found valid",prop
                if hasattr(prop,"LengthValue"):
                    return prop.LengthValue
                elif hasattr(prop,"AreaValue"):
                    return prop.AreaValue
                elif hasattr(prop,"VolumeValue"):
                    return prop.VolumeValue
                elif hasattr(prop,"NominalValue"):
                    return prop.NominalValue
        return None

    def getAttribute(self,attr):
        "returns the value of the given attribute, if exists"
        if hasattr(self,attr):
            return self.__dict__[attr]
        return None
            
class IfcDocument:
    "an object representing an IFC document"
    def __init__(self,filename,schema="IFC2X3_TC1.exp",debug=False):
        DEBUG = debug
        f = IfcFile(filename,schema)
        self.filename = filename
        self.data = f.entById
        self.Entities = {0:f.header}
        for k,e in self.data.iteritems():
            eid = int(e['id'])
            self.Entities[eid] = IfcEntity(e,self)
        if DEBUG: print len(self.Entities),"entities created. Creating attributes..."
        for k,ent in self.Entities.iteritems():
            if DEBUG: print "attributing entity ",ent
            if hasattr(ent,"attributes"):
                for k,v in ent.attributes.iteritems():
                    if DEBUG: print "parsing attribute: ",k," value ",v
                    if isinstance(v,str):
                        val = self.__clean__(v)
                    elif isinstance(v,list):
                        val = []
                        for item in v:
                            if isinstance(item,str):
                                val.append(self.__clean__(item))
                            else:
                                val.append(item)
                    else:
                        val = v
                    setattr(ent,k.strip(),val)
        if DEBUG: print "Document successfully created"

    def __clean__(self,value):
        "turns an attribute value into something usable"
        try:
            val = value.strip(" ()'")
            if val[:3].upper() == "IFC":
                if "IFCTEXT" in val.upper():
                    l = val.split("'")
                    if len(l) == 3: val = l[1]
                elif "IFCBOOLEAN" in value.upper():
                    l = val.split(".")
                    if len(l) == 3: val = l[1]
                    if val.upper() == "F": val = False
                    elif val.upper() == "T": val = True
                elif "IFCREAL" in val.upper():
                    l = val.split("(")
                    if len(l) == 2: val = float(l[1].strip(")"))
            else:
                if '#' in val:
                    if "," in val:
                        val = val.split(",")
                        l = []
                        for subval in val:
                            if '#' in subval:
                                s = subval.strip(" #")
                                if DEBUG: print "referencing ",s," : ",self.getEnt(int(s))
                                l.append(self.getEnt(int(s)))
                        val = l
                    else:
                        val = val.strip()
                        val = val.replace("#","")
                        if DEBUG: print "referencing ",val," : ",self.getEnt(int(val))
                        val =  self.getEnt(int(val))
                        if not val:
                            val = value
        except:
            if DEBUG: print "error parsing attribute",value
            val = value
        return val
        
    def __repr__(self):
        return "IFC Document: " + self.filename + ', ' + str(len(self.Entities)) + " entities "

    def getEnt(self,ref):
        "gets an entity by id number, or a list of entities by type"
        if isinstance(ref,int):
            if ref in self.Entities:
                return self.Entities[ref]
        elif isinstance(ref,str):
            l = []
            ref = ref.upper()
            for k,ob in self.Entities.iteritems():
                if hasattr(ob,"type"):
                    if ob.type == ref:
                        l.append(ob)
            return l
        return None

    def search(self,pat):
        "searches entities types for partial match"
        l = []
        pat = pat.upper()
        for k,ob in self.Entities.iteritems():
            if hasattr(ob,"type"):
                if pat in ob.type:
                    if not ob.type in l:
                        l.append(ob.type)
        return l

    def find(self,pat1,pat2=None,pat3=None):
        '''finds objects in the current IFC document.
        arguments can be of the following form:
        - (pattern): returns object types matching the given pattern (same as search)
        - (type,property,value): finds, in all objects of type "type", those whose
          property "property" has the given value
        '''
        if pat3:
            bobs = self.getEnt(pat1)
            obs = []
            for bob in bobs:
                if hasattr(bob,pat2):
                    if bob.getAttribute(pat2) == pat3:
                        obs.append(bob)
            return obs
        elif pat1:
            ll = self.search(pat1)
            obs = []
            for l in ll:
                obs.extend(self.getEnt(l))
            return obs
        return None
                        
if __name__ == "__main__":
    print __doc__

