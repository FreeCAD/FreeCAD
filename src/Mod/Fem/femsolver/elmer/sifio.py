# ***************************************************************************
# *   Copyright (c) 2016 Markus Hovorka <m.hovorka@live.de>                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM solver Elmer sifio"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import collections
import six


SIMULATION = "Simulation"
CONSTANTS = "Constants"
BODY = "Body"
MATERIAL = "Material"
BODY_FORCE = "Body Force"
EQUATION = "Equation"
SOLVER = "Solver"
BOUNDARY_CONDITION = "Boundary Condition"
INITIAL_CONDITION = "Initial Condition"
COMPONENT = "Component"


_VALID_SECTIONS = (
    SIMULATION,
    CONSTANTS,
    BODY,
    MATERIAL,
    BODY_FORCE,
    EQUATION,
    SOLVER,
    BOUNDARY_CONDITION,
    INITIAL_CONDITION,
    COMPONENT,
)


_NUMBERED_SECTIONS = (
    BODY,
    MATERIAL,
    BODY_FORCE,
    EQUATION,
    SOLVER,
    BOUNDARY_CONDITION,
    INITIAL_CONDITION,
    COMPONENT,
)


_SECTION_DELIM = "End"
_WHITESPACE = " "
_INDENT = " " * 2
_NEWLINE = "\n"


_TYPE_REAL = "Real"
_TYPE_INTEGER = "Integer"
_TYPE_LOGICAL = "Logical"
_TYPE_STRING = "String"
_TYPE_FILE = "File"


WARN = "Warn"
IGNORE = "Ignore"
ABORT = "Abort"
SILENT = "Silent"


def createSection(name):
    section = Section(name)
    if not isValid(section):
        raise ValueError("Invalid section name: %s" % name)
    return section


def writeSections(sections, stream):
    ids = _IdManager()
    _Writer(ids, sections, stream).write()


def isNumbered(section):
    return section.name in _NUMBERED_SECTIONS


def isValid(section):
    return section.name in _VALID_SECTIONS


class Builder(object):

    _ACTIVE_SOLVERS = "Active Solvers"

    def __init__(self):
        self._customSections = []
        self._simulation = createSection(SIMULATION)
        self._constants = createSection(CONSTANTS)
        self._bodies = collections.OrderedDict()
        self._boundaries = collections.OrderedDict()

    def getBoundaryNames(self):
        return self._boundaries.keys()

    def getBodyNames(self):
        return self._bodies.keys()

    def simulation(self, key, attr):
        self._simulation[key] = attr

    def constant(self, key, attr):
        self._constants[key] = attr

    def initial(self, body, key, attr):
        section = self._getFromBody(body, INITIAL_CONDITION)
        section[key] = attr

    def material(self, body, key, attr):
        section = self._getFromBody(body, MATERIAL)
        section[key] = attr

    def equation(self, body, key, attr):
        section = self._getFromBody(body, EQUATION)
        section[key] = attr

    def bodyForce(self, body, key, attr):
        section = self._getFromBody(body, BODY_FORCE)
        section[key] = attr

    def addSolver(self, body, solverSection):
        section = self._getFromBody(body, EQUATION)
        if self._ACTIVE_SOLVERS not in section:
            section[self._ACTIVE_SOLVERS] = []
        section[self._ACTIVE_SOLVERS].append(solverSection)

    def boundary(self, boundary, key, attr):
        if boundary not in self._boundaries:
            self._boundaries[boundary] = createSection(BOUNDARY_CONDITION)
        self._boundaries[boundary][key] = attr

    def addSection(self, section):
        if section not in self._customSections:
            self._customSections.append(section)

    def _getFromBody(self, body, key):
        if body not in self._bodies:
            self._bodies[body] = createSection(BODY)
        if key not in self._bodies[body]:
            self._bodies[body][key] = createSection(key)
        return self._bodies[body][key]

    def __iter__(self):
        allSections = list(self._customSections)
        allSections.append(self._simulation)
        allSections.append(self._constants)
        for name, section in self._bodies.items():
            section["Name"] = name
            allSections.append(section)
            if MATERIAL in section and section[MATERIAL] not in allSections:
                allSections.append(section[MATERIAL])
            if EQUATION in section and section[EQUATION] not in allSections:
                eqSection = section[EQUATION]
                allSections.append(eqSection)
                if self._ACTIVE_SOLVERS in eqSection:
                    for solverSection in eqSection[self._ACTIVE_SOLVERS]:
                        if solverSection not in allSections:
                            allSections.append(solverSection)
            if BODY_FORCE in section and section[BODY_FORCE] not in allSections:
                allSections.append(section[BODY_FORCE])
            if INITIAL_CONDITION in section and section[INITIAL_CONDITION] not in allSections:
                allSections.append(section[INITIAL_CONDITION])
        for name, section in self._boundaries.items():
            section["Name"] = name
            allSections.append(section)
        return iter(allSections)


class Sif(object):

    _CHECK_KEYWORDS = "Check Keywords"
    _HEADER = "Header"
    _MESHDB_ATTR = "Mesh DB"
    _INCLUDE_ATTR = "Include Path"
    _RESULT_ATTR = "Results Directory"

    def __init__(self, sections=[], meshLocation="."):
        self.sections = sections
        self.meshPath = meshLocation
        self.checkKeywords = WARN
        self.incPath = ""
        self.resPath = ""

    def write(self, stream):
        self._writeCheckKeywords(stream)
        stream.write(_NEWLINE * 2)
        self._writeHeader(stream)
        stream.write(_NEWLINE * 2)
        writeSections(self.sections, stream)

    def _writeCheckKeywords(self, stream):
        stream.write(self._CHECK_KEYWORDS)
        stream.write(_WHITESPACE)
        stream.write(self.checkKeywords)

    def _writeHeader(self, stream):
        stream.write(self._HEADER)
        stream.write(_NEWLINE)
        self._writeAttr(self._MESHDB_ATTR, self.meshPath, stream)
        stream.write(_NEWLINE)
        if self.incPath:
            self._writeAttr(self._INCLUDE_ATTR, self.incPath, stream)
            stream.write(_NEWLINE)
        if self.resPath:
            self._writeAttr(self._RESULT_ATTR, self.resPath, stream)
            stream.write(_NEWLINE)
        stream.write(_SECTION_DELIM)

    def _writeAttr(self, name, value, stream):
        stream.write(_INDENT)
        stream.write(name)
        stream.write(_WHITESPACE)
        stream.write('"%s"' % value)


class Section(object):

    def __init__(self, name):
        self.name = name
        self.priority = 0
        self._attrs = dict()

    def __setitem__(self, key, value):
        self._attrs[key] = value

    def __getitem__(self, key):
        return self._attrs[key]

    def __delitem__(self, key):
        del self._attrs[key]

    def __iter__(self):
        return self._attrs.items()

    def keys(self):
        return self._attrs.keys()

    def __contains__(self, item):
        return item in self._attrs

    def __str__(self):
        return str(self._attrs)

    def __repr__(self):
        return self.name


class FileAttr(str):
    pass


class _Writer(object):

    def __init__(self, idManager, sections, stream):
        self._idMgr = idManager
        self._sections = sections
        self._stream = stream

    def write(self):
        sortedSections = sorted(
            self._sections, key=lambda s: s.priority, reverse=True)
        for s in sortedSections:
            self._writeSection(s)
            self._stream.write(_NEWLINE)

    def _writeSection(self, s):
        self._writeSectionHeader(s)
        self._writeSectionBody(s)
        self._writeSectionFooter(s)
        self._stream.write(_NEWLINE)

    def _writeSectionHeader(self, s):
        self._stream.write(s.name)
        self._stream.write(_WHITESPACE)
        if isNumbered(s):
            self._stream.write(str(self._idMgr.getId(s)))

    def _writeSectionFooter(self, s):
        self._stream.write(_NEWLINE)
        self._stream.write(_SECTION_DELIM)

    def _writeSectionBody(self, s):
        for key in sorted(s.keys()):  # def keys() from class sifio.Section is called
            self._writeAttribute(key, s[key])

    def _writeAttribute(self, key, data):
        if isinstance(data, Section):
            self._stream.write(_NEWLINE)
            self._writeScalarAttr(key, data)
        elif isinstance(data, FileAttr):
            self._stream.write(_NEWLINE)
            self._writeFileAttr(key, data)
        elif self._isCollection(data):
            if len(data) == 1:
                scalarData = self._getOnlyElement(data)
                self._stream.write(_NEWLINE)
                self._writeScalarAttr(key, scalarData)
            elif len(data) > 1:
                self._stream.write(_NEWLINE)
                self._writeArrAttr(key, data)
        else:
            self._stream.write(_NEWLINE)
            self._writeScalarAttr(key, data)

    def _getOnlyElement(self, collection):
        it = iter(collection)
        return it.next()

    def _isCollection(self, data):
        return (
            not isinstance(data, six.string_types)
            and isinstance(data, collections.Iterable)
        )

    def _checkScalar(self, dataType):
        if issubclass(dataType, int):
            return self._genIntAttr
        if issubclass(dataType, float):
            return self._genFloatAttr
        if issubclass(dataType, bool):
            return self._genBoolAttr
        if issubclass(dataType, str):
            return self._genStrAttr
        return None

    def _writeScalarAttr(self, key, data):
        attrType = self._getAttrTypeScalar(data)
        if attrType is None:
            raise ValueError("Unsupported data type: %s" % type(data))
        self._stream.write(_INDENT)
        self._stream.write(key)
        self._stream.write(_WHITESPACE)
        self._stream.write("=")
        self._stream.write(_WHITESPACE)
        self._stream.write(attrType)
        self._stream.write(_WHITESPACE)
        self._stream.write(self._preprocess(data, type(data)))

    def _writeArrAttr(self, key, data):
        attrType = self._getAttrTypeArr(data)
        self._stream.write(_INDENT)
        self._stream.write(key)
        self._stream.write("(%d)" % len(data))
        self._stream.write(_WHITESPACE)
        self._stream.write("=")
        self._stream.write(_WHITESPACE)
        self._stream.write(attrType)
        for val in data:
            self._stream.write(_WHITESPACE)
            self._stream.write(self._preprocess(val, type(val)))

    def _writeFileAttr(self, key, data):
        self._stream.write(_INDENT)
        self._stream.write(key)
        self._stream.write(_WHITESPACE)
        self._stream.write("=")
        self._stream.write(_WHITESPACE)
        self._stream.write(_TYPE_FILE)
        for val in data.split("/"):
            if val:
                self._stream.write(_WHITESPACE)
                self._stream.write('"%s"' % val)

    def _getSifDataType(self, dataType):
        if issubclass(dataType, Section):
            return _TYPE_INTEGER
        if issubclass(dataType, bool):
            return _TYPE_LOGICAL
        if issubclass(dataType, int):
            return _TYPE_INTEGER
        if issubclass(dataType, float):
            return _TYPE_REAL
        if issubclass(dataType, six.string_types):    # use six to be sure to be Python 2.7 and 3.x compatible
            return _TYPE_STRING
        raise ValueError("Unsupported data type: %s" % dataType)

    def _preprocess(self, data, dataType):
        if issubclass(dataType, Section):
            return str(self._idMgr.getId(data))
        if issubclass(dataType, six.string_types):    # use six to be sure to be Python 2.7 and 3.x compatible
            return '"%s"' % data
        return str(data)

    def _getAttrTypeScalar(self, data):
        return self._getSifDataType(type(data))

    def _getAttrTypeArr(self, data):
        if not data:
            raise ValueError("Collections must not be empty.")
        it = iter(data)
        dataType = type(next(it))
        for entry in it:
            if not isinstance(entry, dataType):
                raise ValueError("Collection must be homogenueous")
        return self._getSifDataType(dataType)


class _IdManager(object):

    def __init__(self, firstId=1):
        self._pool = dict()
        self._ids = dict()
        self.firstId = firstId

    def setId(self, section):
        if section.name not in self._pool:
            self._pool[section.name] = self.firstId
        self._ids[section] = self._pool[section.name]
        self._pool[section.name] += 1

    def getId(self, section):
        if section not in self._ids:
            self.setId(section)
        return self._ids[section]

##  @}
