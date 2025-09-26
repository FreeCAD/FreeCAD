from __future__ import annotations
from dataclasses import dataclass, field
from enum import Enum
from typing import List, Optional


#
# Enums
#


class ParameterType(str, Enum):
    BOOLEAN = "Boolean"
    INT = "Int"
    LONG = "Long"
    STRING = "String"
    OBJECT = "Object"
    FLOAT = "Float"
    COMPLEX = "Complex"
    CHAR = "Char"
    TUPLE = "Tuple"
    LIST = "List"
    DICT = "Dict"
    MODULE = "Module"
    CALLABLE = "Callable"
    SEQUENCE = "Sequence"
    VECTOR = "Vector"

    def __str__(self):
        return self.value


#
# Supporting Classes
#


@dataclass
class Author:
    """Represents the <Author> element inside <Documentation>."""

    # The text content of <Author> is effectively a string;
    # we capture it in `content` to hold the text node if needed.
    content: Optional[str] = None

    # Attributes
    Name: str = "FreeCAD Project"
    EMail: str = "example@freecad.org"
    Licence: str = "LGPL"


@dataclass
class Documentation:
    """
    Corresponds to the <Documentation> element.
    Can contain an <Author>, <DeveloperDocu>, and <UserDocu>.
    """

    Author: Optional[Author] = None
    DeveloperDocu: Optional[str] = None
    UserDocu: Optional[str] = None


@dataclass
class Property:
    """
    Corresponds to <Property> in the schema.
    It has required attributes Name, Type and optional StartValue,
    plus optional child <Documentation>.
    """

    # Child
    Documentation: Optional[Documentation] = None

    # Attributes
    Name: str = ""
    Type: str = ""
    StartValue: Optional[str] = None


@dataclass
class ViewProvider:
    """
    Corresponds to <ViewProvider>, which can contain 0..∞ <Property> children.
    """

    Property: List[Property] = field(default_factory=list)


@dataclass
class Parameter:
    """
    Corresponds to <Parameter> in the schema.
    It has a required 'Name' (str) and a required 'Type' (enumeration).
    """

    Name: str
    Type: ParameterType


#
# Elements under <PythonExport>
#


@dataclass
class Methode:
    """
    Corresponds to <Methode> inside <PythonExport>.
    Contains an optional <Documentation> and 0..∞ <Parameter>.
    """

    Documentation: Optional[Documentation] = None
    Parameter: List[Parameter] = field(default_factory=list)

    # Attributes
    Name: str = ""
    Const: Optional[bool] = None
    Keyword: bool = False
    NoArgs: bool = False
    Class: bool = False
    Static: bool = False


@dataclass
class Attribute:
    """
    Corresponds to <Attribute> inside <PythonExport>.
    It has a required <Documentation>, a required <Parameter>,
    and attributes Name, ReadOnly.
    """

    Documentation: Documentation
    Parameter: Parameter

    # Attributes
    Name: str
    ReadOnly: bool


@dataclass
class SequenceProtocol:
    """
    Corresponds to the <Sequence> element inside <PythonExport>.
    All attributes are required booleans.
    """

    sq_length: bool = False
    sq_concat: bool = False
    sq_repeat: bool = False
    sq_item: bool = False
    mp_subscript: bool = False
    sq_ass_item: bool = False
    mp_ass_subscript: bool = False
    sq_contains: bool = False
    sq_inplace_concat: bool = False
    sq_inplace_repeat: bool = False


@dataclass
class PythonExport:
    """
    Corresponds to <PythonExport> inside <GenerateModel>.
    It contains:
      - optional <Documentation>
      - 0..∞ <Methode>
      - 0..∞ <Attribute>
      - optional <Sequence>
      - optional <CustomAttributes>
      - one <ClassDeclarations> (type=string)
      - one <ForwardDeclarations> (type=string)
    Plus many attributes with required/optional flags.
    """

    Documentation: Optional[Documentation] = None
    Methode: List[Methode] = field(default_factory=list)
    Attribute: List[Attribute] = field(default_factory=list)
    Sequence: Optional[SequenceProtocol] = None
    CustomAttributes: Optional[str] = ""  # To match the original XML model
    ClassDeclarations: str = ""
    ForwardDeclarations: str = ""
    NoArgs: bool = False

    # Attributes
    ModuleName: str = ""
    Name: str = ""
    PythonName: Optional[str] = None
    Include: str = ""
    Father: str = ""
    Twin: str = ""
    Namespace: str = ""
    FatherInclude: str = ""
    FatherNamespace: str = ""
    Constructor: bool = False
    NumberProtocol: bool = False
    RichCompare: bool = False
    TwinPointer: str = ""
    Delete: bool = False
    Reference: Optional[bool] = None
    Initialization: bool = False
    DisableNotify: bool = False
    DescriptorGetter: bool = False
    DescriptorSetter: bool = False
    IsExplicitlyExported: bool = False


#
# Module-Related Classes
#


@dataclass
class Dependencies:
    """
    Corresponds to the <Dependencies> element inside <Module>.
    It contains 0..∞ local <Module> elements which are not typed in the XSD.
    We'll treat these as strings or possibly minimal structures.
    """

    Module: List[str] = field(default_factory=list)


@dataclass
class Feature:
    """
    Corresponds to <Feature> in <Module>'s <Content>.
    Has optional <Documentation>, 0..∞ <Property>, optional <ViewProvider>,
    and a required attribute 'Name'.
    """

    Documentation: Optional[Documentation] = None
    Property: List[Property] = field(default_factory=list)
    ViewProvider: Optional[ViewProvider] = None

    # Attributes
    Name: str = ""


@dataclass
class DocObject:
    """
    Corresponds to <DocObject> in <Module>'s <Content>.
    Has optional <Documentation>, 0..∞ <Property>, and a required 'Name' attribute.
    """

    Documentation: Optional[Documentation] = None
    Property: List[Property] = field(default_factory=list)

    # Attributes
    Name: str = ""


@dataclass
class ModuleContent:
    """
    Corresponds to the <Content> element in <Module>.
    Contains:
      - 0..∞ <Property>
      - 0..∞ <Feature>
      - 0..∞ <DocObject>
      - 0..∞ <GuiCommand>
      - 0..∞ <PreferencesPage>
    """

    Property: List[Property] = field(default_factory=list)
    Feature: List[Feature] = field(default_factory=list)
    DocObject: List[DocObject] = field(default_factory=list)
    GuiCommand: List[str] = field(default_factory=list)
    PreferencesPage: List[str] = field(default_factory=list)


@dataclass
class Module:
    """
    Corresponds to the top-level <Module> element.
    Has optional <Documentation>, optional <Dependencies>,
    a required <Content>, and a required attribute Name.
    """

    Documentation: Optional[Documentation] = None
    Dependencies: Optional[Dependencies] = None
    Content: ModuleContent = field(default_factory=ModuleContent)

    # Attributes
    Name: str = ""


#
# Root Element
#


@dataclass
class GenerateModel:
    """
    Corresponds to the root element <GenerateModel>.
    Contains 0..∞ <Module> and 0..∞ <PythonExport>.
    """

    Module: List[Module] = field(default_factory=list)
    PythonExport: List[PythonExport] = field(default_factory=list)

    def dump(self):
        # Print or process the resulting GenerateModel object
        print("Parsed GenerateModel object:")

        if self.PythonExport:
            py_exp = self.PythonExport[0]
            print("PythonExport Name:", py_exp.Name)
            if py_exp.Documentation and py_exp.Documentation.Author:
                print("Author Name:", py_exp.Documentation.Author.Name)
                print("Author Email:", py_exp.Documentation.Author.EMail)
                print("Author Licence:", py_exp.Documentation.Author.Licence)
            print("DeveloperDocu:", py_exp.Documentation.DeveloperDocu)
            print("UserDocu:", py_exp.Documentation.UserDocu)

            print("Class Attributes:")
            for attr in py_exp.Attribute:
                print(f"  - {attr.Name} (type={attr.Parameter.Type}, readOnly={attr.ReadOnly})")

            print("Methods:")
            for meth in py_exp.Methode:
                print(f"  - {meth.Name}")
                # Each method might have parameters
                for param in meth.Parameter:
                    print(f"    * param: {param.Name}, type={param.Type}")
