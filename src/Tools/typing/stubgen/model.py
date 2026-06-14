"""Shared definitions for the stub generation pipeline.

This module is the schema layer for the tooling. It centralizes:
- immutable configuration such as default input directories
- regexes used to recognize C++ binding patterns
- enums and type aliases shared across parsing and generation
- dataclasses that represent discovered bindings and emitted stub fragments

It intentionally contains only inert definitions. Parsing behavior, manual rule
interpretation, and output generation live elsewhere so this file stays safe to
read first when orienting within the pipeline.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
import re
from typing import Literal, TypeAlias

SOURCE_EXTENSIONS = {".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
SKIPPED_SOURCE_PREFIXES = (
    ("src", "3rdParty"),
    ("src", "Tools", "typing", "generated"),
    ("src", "Tools", "typing", "inputs"),
    ("src", "Tools", "bindings", "templates"),
)
DEFAULT_SOURCE_DIR = Path("src")
DEFAULT_STUBS_OUT_DIR = Path("src/Tools/typing/generated")
DEFAULT_OVERLAY_DIR = Path("src/Tools/typing/inputs/overlays")
MODULE_STUB_PYI_SUFFIX = ".module.pyi"

# Match CMake bindings registrations like:
#   generate_from_py(BaseClass)
GENERATE_FROM_PY_CALL_RE = re.compile(r"\bgenerate_from_py_?\s*\(\s*(?P<base>[^\s)]+)\s*\)")

# Match PyCXX method registrations like:
#   add_varargs_method("name", &Type::method, "doc")
ADD_METHOD_RE = re.compile(r"\b(?P<kind>add_(?:varargs|keyword|noargs)_method)\s*\(")

# Match behavior naming such as:
#   behaviors().name("Vector")
BEHAVIOR_NAME_RE = re.compile(r"\bbehaviors\s*\(\s*\)\s*\.\s*name\s*\(\s*\"([^\"]+)\"\s*\)")

CPP_IDENTIFIER = r"[A-Za-z_]\w*"
CPP_QUALIFIED_NAME = rf"(?:{CPP_IDENTIFIER}\s*::\s*)*{CPP_IDENTIFIER}"
CPP_TABLE_REFERENCE = rf"(?:{CPP_IDENTIFIER}::)*{CPP_IDENTIFIER}"

# Match PyCXX extension module declarations like:
#   Py::ExtensionModule<BaseModule>("FreeCAD")
EXTENSION_MODULE_RE = re.compile(
    rf"\bPy::ExtensionModule\s*<\s*(?P<cpp_name>{CPP_QUALIFIED_NAME})\s*>\s*"
    r"\(\s*\"(?P<python_name>[^\"]+)\"\s*\)"
)

# Match PyMethodDef tables like:
#   static PyMethodDef MyModule_methods[] = {
PYMETHODDEF_RE = re.compile(
    r"(?:static\s+)?(?:const\s+)?(?:struct\s+)?PyMethodDef\s+"
    rf"(?P<table>{CPP_TABLE_REFERENCE})\s*\[\s*\]\s*=\s*\{{"
)

# Match PyModuleDef definitions like:
#   static PyModuleDef moduledef = {
PYMODULEDEF_RE = re.compile(
    rf"(?:static\s+)?(?:struct\s+)?PyModuleDef\s+(?P<definition>{CPP_IDENTIFIER})\s*=\s*\{{"
)

# Match module creation assignments like:
#   PyObject* mod = PyModule_Create(&moduledef)
PYMODULE_CREATE_RE = re.compile(
    rf"(?:PyObject\s*\*\s*)?(?P<variable>{CPP_IDENTIFIER})\s*=\s*"
    rf"PyModule_Create\s*\(\s*&(?P<definition>{CPP_IDENTIFIER})\s*\)"
)

# Match module imports like:
#   PyObject* part = PyImport_ImportModule("Part")
PYMODULE_IMPORT_RE = re.compile(
    rf"(?:PyObject\s*\*\s*)?(?P<variable>{CPP_IDENTIFIER})\s*=\s*"
    r"PyImport_ImportModule\s*\(\s*\"(?P<module>[^\"]+)\"\s*\)"
)

# Match nested module wiring like:
#   PyModule_AddObject(parent, "Geom2d", child)
PYMODULE_ADD_OBJECT_RE = re.compile(
    rf"PyModule_AddObject\s*\(\s*(?P<parent>{CPP_IDENTIFIER})\s*,\s*"
    rf"\"(?P<name>[^\"]+)\"\s*,\s*(?P<child>{CPP_IDENTIFIER})\s*\)"
)

# Match table attachment like:
#   PyModule_AddFunctions(module, SomeType::Methods)
PYMODULE_ADD_FUNCTIONS_RE = re.compile(
    rf"PyModule_AddFunctions\s*\(\s*(?P<module>{CPP_IDENTIFIER})\s*,\s*"
    rf"(?P<table>{CPP_TABLE_REFERENCE})\s*\)"
)

# Match local table aliases like:
#   auto methods = SomeType::Methods;
PYMETHOD_ALIAS_RE = re.compile(
    rf"\b(?P<alias>{CPP_IDENTIFIER})\s*=\s*(?P<table>{CPP_TABLE_REFERENCE})\s*;"
)

# Match C/C++ string literals, including wide or UTF-prefixed forms like:
#   "doc", u8"name", L"label"
STRING_LITERAL_RE = re.compile(r'(?:u8|u|U|L)?("(?:\\.|[^"\\])*")')

# Match init helpers like:
#   PyObject* mod = Base::initModule()
INIT_MODULE_RE = re.compile(
    rf"(?:PyObject\s*\*\s*)?(?P<variable>{CPP_IDENTIFIER})\s*=\s*"
    rf"(?P<namespace>{CPP_IDENTIFIER})::initModule\s*\(\s*\)"
)

# Match Py::Object wrappers around module variables like:
#   Py::Object wrapper(module)
PY_OBJECT_WRAPPER_RE = re.compile(
    rf"Py::Object\s+(?P<variable>{CPP_IDENTIFIER})\s*\(\s*(?P<source>{CPP_IDENTIFIER})\s*\)"
)

# Match module creation without import like:
#   PyObject* gui = PyImport_AddModule("FreeCADGui")
PYIMPORT_ADD_MODULE_RE = re.compile(
    rf"(?:PyObject\s*\*\s*)?(?P<variable>{CPP_IDENTIFIER})\s*=\s*"
    r"PyImport_AddModule\s*\(\s*\"(?P<module>[^\"]+)\"\s*\)"
)

# Match module attribute fetches like:
#   PyObject* sub(gui.getAttr("Selection").ptr())
GETATTR_MODULE_RE = re.compile(
    rf"PyObject\s*\*\s*(?P<variable>{CPP_IDENTIFIER})\s*"
    rf"\(\s*(?P<owner>{CPP_IDENTIFIER})\.getAttr\s*\(\s*\"(?P<name>[^\"]+)\"\s*\)\.ptr\s*\(\s*\)\s*\)"
)

# Match class type objects referenced as:
#   Base::VectorPy::Type
CPP_TYPE_NAME_RE = re.compile(r"((?:[A-Za-z_]\w*\s*::\s*)*[A-Za-z_]\w*)\s*::\s*Type\b")
HELPER_PYI_FILES = {
    "src/Base/Metadata.pyi",
    "src/Base/PyObjectBase.pyi",
}
PUBLIC_STUB_DECORATORS = {
    "classmethod",
    "overload",
    "staticmethod",
}

BindingFamily: TypeAlias = Literal["pycxx_add_method", "pymethoddef"]
ContextKind: TypeAlias = Literal["pycxx_module", "pymethoddef_table", "python_type", "unknown"]
MethodKind: TypeAlias = Literal["keyword", "noargs", "varargs"]
ContextEntry: TypeAlias = tuple[int, ContextKind, str]
ImportTarget: TypeAlias = tuple[str, str]


class ScannerState(Enum):
    CODE = auto()
    LINE_COMMENT = auto()
    BLOCK_COMMENT = auto()
    STRING = auto()
    CHAR = auto()


@dataclass(frozen=True)
class BindingMethod:
    family: BindingFamily
    source: str
    line: int
    table: str | None
    context_kind: ContextKind
    context_name: str
    inferred_module: str | None
    method_kind: MethodKind
    python_name: str
    cxx_callable: str
    flags: str
    doc: str
    generated_source: bool


@dataclass(frozen=True)
class ModuleDef:
    source: str
    name: str
    table: str | None


@dataclass(frozen=True)
class BindingClass:
    source: str
    line: int
    class_name: str
    export_name: str
    python_name: str | None
    public_names: list[str]
    base_class: str | None
    explicit_export: bool


@dataclass(frozen=True)
class PublicTypeTarget:
    module_name: str
    class_symbol: str
    variable_symbol: str | None = None
    base_symbols: tuple[str, ...] = ()


@dataclass(frozen=True)
class PublicTypeGroup:
    class_symbol: str
    variable_symbol: str | None
    base_symbols: tuple[str, ...]
    methods: list[BindingMethod]


@dataclass(frozen=True)
class StubSignature:
    parameters: str
    returns: str
    class_symbol: str | None = None
    doc: str | None = None


@dataclass(frozen=True)
class ImportBinding:
    module: str
    name: str | None = None


@dataclass(frozen=True)
class PublicClassStub:
    source: str
    import_lines: tuple[str, ...] = ()


StubSignatureKey: TypeAlias = tuple[str, str, str]
StubSignatureGroup: TypeAlias = tuple[StubSignature, ...]
StubSignatureOverrides: TypeAlias = dict[StubSignatureKey, StubSignatureGroup]
