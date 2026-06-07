# SPDX-License-Identifier: LGPL-2.1-or-later

"""Parses Python binding interface files into a typed AST model."""

from dataclasses import dataclass
from enum import Enum
from functools import lru_cache
from pathlib import Path
import ast
import re
from typing import List
from model.typedModel import (
    GenerateModel,
    PythonExport,
    PythonModuleExport,
    Method,
    Attribute,
    Documentation,
    Author,
    Parameter,
    ParameterType,
    SequenceProtocol,
)

SIGNATURE_SEP = re.compile(r"\s+--\s+", re.DOTALL)
SELF_CLS_ARG = re.compile(r"\(\s*(self|cls)(\s*,\s*)?")
CTOR_SELF_CLS_ARG = re.compile(r"^__init__\(\$?(?:self|cls)(?:,\s*)?")
CTOR_NAME = re.compile(r"^__init__\(")


class ArgumentKind(Enum):
    PositionOnly = 0
    Arg = 1
    VarArg = 2
    KwOnly = 3
    KwArg = 4


@dataclass
class FuncArgument:
    name: str
    annotation: str
    kind: ArgumentKind


@dataclass(frozen=True)
class BindingClassInfo:
    class_name: str
    export_name: str
    python_name: str | None
    namespace: str
    include: str
    is_exported: bool
    path: Path
    father_include: str
    source_dependencies: tuple[Path, ...] = ()


class FunctionSignature:
    """
    Parse function arguments with correct classification and order.
    """

    args: list[FuncArgument]
    has_keywords: bool
    docstring: str
    annotated_text: str
    text: str

    const_flag: bool = False
    static_flag: bool = False
    class_flag: bool = False
    noargs_flag: bool = False
    is_overload: bool = False

    def __init__(self, func: ast.FunctionDef):
        self.args = []
        self.has_keywords = False
        self.is_overload = False
        self.docstring = ast.get_docstring(func) or ""

        args = func.args
        self.update_flags(func)

        self.args.extend(
            (
                FuncArgument(
                    arg.arg,
                    self.get_annotation_str(arg.annotation),
                    ArgumentKind.PositionOnly,
                )
                for arg in args.posonlyargs
            ),
        )

        self.args.extend(
            (
                FuncArgument(
                    arg.arg,
                    self.get_annotation_str(arg.annotation),
                    ArgumentKind.Arg,
                )
                for arg in args.args
            ),
        )

        # tricky part to determine if there are keyword arguments or not
        if args.args:
            if args.args[0].arg in ("self", "cls"):
                instance_args = len(args.args) > 1
            else:
                instance_args = True
        else:
            instance_args = False

        self.has_keywords = bool(instance_args or args.kwonlyargs or args.kwarg)

        if args.vararg:
            self.args.append(
                FuncArgument(
                    args.vararg.arg,
                    self.get_annotation_str(args.vararg.annotation),
                    ArgumentKind.VarArg,
                ),
            )

        self.args.extend(
            (
                FuncArgument(
                    arg.arg,
                    self.get_annotation_str(arg.annotation),
                    ArgumentKind.KwOnly,
                )
                for arg in args.kwonlyargs
            ),
        )

        if args.kwarg:
            self.args.append(
                FuncArgument(
                    args.kwarg.arg,
                    self.get_annotation_str(args.kwarg.annotation),
                    ArgumentKind.KwArg,
                ),
            )

        # Annotated signatures (Not supported by __text_signature__)
        returns = ast.unparse(func.returns) if func.returns else "object"
        parameters = ast.unparse(func.args)
        self.annotated_text = SELF_CLS_ARG.sub("(", f"{func.name}({parameters}) -> {returns}", 1)

        # Not Annotated signatures (supported by __text_signature__)
        all_args = [
            *args.posonlyargs,
            *args.args,
            args.vararg,
            *args.kwonlyargs,
            args.kwarg,
        ]
        for item in all_args:
            if item:
                item.annotation = None
        parameters = ast.unparse(args)
        self.text = SELF_CLS_ARG.sub(r"($\1\2", f"{func.name}({parameters})", 1)

    def get_annotation_str(self, node: ast.AST | None) -> str:
        if not node:
            return "object"
        return ast.unparse(node)

    def update_flags(self, func: ast.FunctionDef) -> None:
        self.typing_only_flag = False
        for deco in func.decorator_list:
            match deco:
                case ast.Name(id, _):
                    name = id
                case ast.Attribute(_, attr, _):
                    name = attr
                case _:
                    continue

            match name:
                case "constmethod":
                    self.const_flag = True
                case "classmethod":
                    self.class_flag = True
                case "no_args":
                    self.noargs_flag = True
                case "staticmethod":
                    self.static_flag = True
                case "overload":
                    self.is_overload = True
                case "typing_only":
                    self.typing_only_flag = True


class Function:
    name: str
    signatures: list[FunctionSignature]

    def __init__(self, func: ast.FunctionDef) -> None:
        self.name = func.name
        self.signatures = [FunctionSignature(func)]

    def update(self, func: ast.FunctionDef) -> None:
        self.signatures.append(FunctionSignature(func))

    @property
    def public_signatures(self) -> list[FunctionSignature]:
        return [sig for sig in self.signatures if not sig.typing_only_flag]

    @property
    def docstring(self) -> str:
        return "\n\n".join((f.docstring for f in self.public_signatures if f.docstring))

    @property
    def has_keywords(self) -> bool:
        signatures = self.public_signatures
        overloads = len(signatures) > 1
        if overloads:
            return any(sig.has_keywords for sig in signatures if sig.is_overload)
        return signatures[0].has_keywords if signatures else False

    @property
    def signature(self) -> FunctionSignature | None:
        """First non overload signature"""
        for sig in self.public_signatures:
            if not sig.is_overload:
                return sig
        return None

    @property
    def doc_signatures(self) -> list[FunctionSignature]:
        signatures = self.public_signatures
        if len(signatures) == 1:
            return [signatures[0]]

        implemented = [sig for sig in signatures if not sig.is_overload]
        return implemented or [sig for sig in signatures if sig.is_overload]

    @property
    def annotated_signatures(self) -> list[FunctionSignature]:
        overloads = [sig for sig in self.public_signatures if sig.is_overload]
        return overloads or self.doc_signatures

    @property
    def static_flag(self) -> bool:
        return any(sig.static_flag for sig in self.public_signatures)

    @property
    def const_flag(self) -> bool:
        return any(sig.const_flag for sig in self.public_signatures)

    @property
    def class_flag(self) -> bool:
        return any(sig.class_flag for sig in self.public_signatures)

    @property
    def noargs_flag(self) -> bool:
        return any(sig.noargs_flag for sig in self.public_signatures)

    @property
    def typing_only_flag(self) -> bool:
        return bool(self.signatures) and not self.public_signatures

    def add_signature_docs(self, doc: Documentation) -> None:
        _compose_signature_docs(
            doc,
            [sig.text for sig in self.doc_signatures],
            [sig.annotated_text for sig in self.annotated_signatures],
        )


def _decorator_name(node: ast.AST) -> str | None:
    match node:
        case ast.Name(id=name):
            return name
        case ast.Attribute(attr=attr):
            return attr
        case ast.Call(func=func):
            return _decorator_name(func)
    return None


def _compose_signature_docs(doc: Documentation, docstring: list[str], signature: list[str]) -> None:
    if not docstring:
        return

    user_doc = doc.UserDocu or ""
    marker = SIGNATURE_SEP.search(user_doc)
    if marker:
        user_doc = user_doc[marker.end() :].strip()

    docstring.append("--\n")  # mark __text_signature__
    docstring.extend(signature)  # Include real annotated signature in user docstring
    if user_doc:
        docstring.append(f"\n{user_doc}")  # Rest of the docstring
    doc.UserDocu = "\n".join(docstring)


def _format_constructor_signature(signature: str, class_name: str) -> str:
    signature = CTOR_SELF_CLS_ARG.sub(f"{class_name}(", signature)
    return CTOR_NAME.sub(f"{class_name}(", signature)


def _constructor_user_doc(func: Function, class_name: str) -> str:
    doc = _parse_docstring_for_documentation(func.docstring)
    _compose_signature_docs(
        doc,
        [_format_constructor_signature(sig.text, class_name) for sig in func.doc_signatures],
        [
            _format_constructor_signature(sig.annotated_text, class_name)
            for sig in func.annotated_signatures
        ],
    )
    return (doc.UserDocu or "").strip()


def _append_user_doc(doc: Documentation, extra_user_doc: str) -> None:
    extra_user_doc = extra_user_doc.strip()
    if not extra_user_doc:
        return

    if doc.UserDocu:
        doc.UserDocu = f"{doc.UserDocu.rstrip()}\n\n{extra_user_doc}"
    else:
        doc.UserDocu = extra_user_doc


def _get_module_docstring(tree: ast.Module) -> str:
    for node in tree.body:
        match node:
            case ast.Expr(value=ast.Constant(value=str() as docstring)):
                return docstring
            case ast.Import() | ast.ImportFrom():
                continue
            case _:
                break
    return ""


def _extract_decorator_kwargs(decorator: ast.expr) -> dict:
    """
    Extract keyword arguments from a decorator call like `@export(Father="...", Name="...")`.
    Returns them in a dict.
    """
    if not isinstance(decorator, ast.Call):
        return {}
    result = {}
    for kw in decorator.keywords:
        match kw.value:
            case ast.Constant(value=val):
                result[kw.arg] = val
            case _:
                pass
    return result


def _parse_docstring_for_documentation(docstring: str) -> Documentation:
    """
    Given a docstring, parse out DeveloperDocu, UserDocu, Author, Licence, etc.
    This is a simple heuristic-based parser. Adjust as needed for your format.
    """
    dev_docu = None
    user_docu = None
    author_name = None
    author_email = None
    author_licence = None

    if not docstring:
        return Documentation()

    import textwrap

    # Remove common indentation
    dedented_docstring = textwrap.dedent(docstring).strip()
    lines = dedented_docstring.split("\n")
    user_docu_lines = []

    for raw_line in lines:
        stripped_line = raw_line.strip()
        if stripped_line.startswith("DeveloperDocu:"):
            dev_docu = stripped_line.split("DeveloperDocu:", 1)[1].strip()
        elif stripped_line.startswith("UserDocu:"):
            user_docu = stripped_line.split("UserDocu:", 1)[1].strip()
        elif stripped_line.startswith("Author:"):
            # e.g. "Author: John Doe (john@example.com)"
            author_part = stripped_line.split("Author:", 1)[1].strip()
            match = re.search(r"(.*?)\s*\((.*?)\)", author_part)
            if match:
                author_name = match.group(1).strip()
                author_email = match.group(2).strip()
            else:
                author_name = author_part
        elif stripped_line.startswith("Licence:"):
            author_licence = stripped_line.split("Licence:", 1)[1].strip()
        else:
            user_docu_lines.append(raw_line)

    if user_docu is None:
        user_docu = "\n".join(user_docu_lines)

    author_obj = None
    if author_name or author_email or author_licence:
        author_obj = Author(
            content=docstring,
            Name=author_name or "",
            EMail=author_email or "",
            Licence=author_licence or "LGPL",
        )

    return Documentation(
        Author=author_obj,
        DeveloperDocu=dev_docu,
        UserDocu=user_docu,
    )


def _get_type_str(node):
    """Recursively convert an AST node for a type annotation to its string representation."""
    match node:
        case ast.Name(id=name):
            # Handle qualified names (e.g., typing.List)
            return name
        case ast.Attribute(value=val, attr=attr):
            # For annotations like List[str] (or Final[List[str]]), build the string recursively.
            return f"{_get_type_str(val)}.{attr}"
        case ast.Subscript(value=val, slice=slice_node):
            value_str = _get_type_str(val)
            slice_str = _get_type_str(slice_node)
            return f"{value_str}[{slice_str}]"
        case ast.Tuple(elts=elts):
            # For multiple types (e.g., Tuple[int, str])
            return ", ".join(_get_type_str(elt) for elt in elts)
        case _:
            # Fallback for unsupported node types
            return "object"


def _subscript_items(node: ast.AST) -> list[ast.AST]:
    if isinstance(node, ast.Tuple):
        return list(node.elts)
    return [node]


def _is_type_name(node: ast.AST, *names: str) -> bool:
    type_name = _get_type_str(node).lower()
    return type_name in {name.lower() for name in names}


def _cxx_type_from_annotation(node: ast.AST) -> str | None:
    if not isinstance(node, ast.Subscript) or not _is_type_name(
        node.value, "Annotated", "typing.Annotated"
    ):
        return None

    for metadata in _subscript_items(node.slice)[1:]:
        if not isinstance(metadata, ast.Call) or not _is_type_name(metadata.func, "cxx_type"):
            continue
        if metadata.args and isinstance(metadata.args[0], ast.Constant):
            value = metadata.args[0].value
            if isinstance(value, str):
                return value
    return None


def _cxx_type_to_parameter_type(cxx_type: str) -> ParameterType:
    if cxx_type == "Vector":
        return ParameterType.VECTOR
    return ParameterType.OBJECT


def _annotation_to_parameter_type(node: ast.AST) -> ParameterType:
    if cxx_type := _cxx_type_from_annotation(node):
        return _cxx_type_to_parameter_type(cxx_type)

    if isinstance(node, ast.Subscript) and _is_type_name(
        node.value, "Annotated", "typing.Annotated"
    ):
        items = _subscript_items(node.slice)
        if items:
            return _annotation_to_parameter_type(items[0])

    return _python_type_to_parameter_type(_get_type_str(node))


def _python_type_to_parameter_type(py_type: str) -> ParameterType:
    """
    Map a Python type annotation (as a string) to the ParameterType enum if possible.
    Fallback to OBJECT if unrecognized.
    """
    py_type = py_type.lower()
    match py_type:
        case _ if py_type in ("int", "builtins.int"):
            return ParameterType.LONG
        case _ if py_type in ("float", "builtins.float"):
            return ParameterType.FLOAT
        case _ if py_type in ("str", "builtins.str"):
            return ParameterType.STRING
        case _ if py_type in ("bool", "builtins.bool"):
            return ParameterType.BOOLEAN
        case _ if py_type.startswith(("list", "typing.list")):
            return ParameterType.LIST
        case _ if py_type.startswith(("dict", "typing.dict")):
            return ParameterType.DICT
        case _ if py_type.startswith(("callable", "typing.callable")):
            return ParameterType.CALLABLE
        case _ if py_type.startswith(("sequence", "typing.sequence")):
            return ParameterType.SEQUENCE
        case _ if py_type.startswith(("tuple", "typing.tuple")):
            return ParameterType.TUPLE
        case _ if py_type.startswith("pycxxvector"):
            return ParameterType.VECTOR
        case _:
            return ParameterType.OBJECT


_PARAMETER_TYPE_HEADER_INCLUDES = {
    ParameterType.VECTOR: "Base/GeometryPyCXX.h",
}


def _attribute_header_includes(attributes: list[Attribute]) -> list[str]:
    includes = []
    for attr in attributes:
        include = _PARAMETER_TYPE_HEADER_INCLUDES.get(attr.Parameter.Type)
        if include and include not in includes:
            includes.append(include)
    return includes


def _filter_header_includes(includes: list[str], *excluded_includes: str) -> list[str]:
    excluded = set(excluded_includes)
    return [include for include in includes if include not in excluded]


def _parse_class_attributes(class_node: ast.ClassDef, source_code: str) -> List[Attribute]:
    """
    Parse top-level attributes (e.g. `TypeId: str = ""`) from the class AST node.
    We'll create an `Attribute` for each. For the `Documentation` of each attribute,
    we might store minimal or none if there's no docstring.
    """
    attributes = []
    default_doc = Documentation(DeveloperDocu="", UserDocu="", Author=None)

    for idx, stmt in enumerate(class_node.body):
        if isinstance(stmt, ast.AnnAssign):
            # e.g.: `TypeId: Final[str] = ""`
            name = stmt.target.id if isinstance(stmt.target, ast.Name) else "unknown"
            # Evaluate the type annotation and detect Final for read-only attributes.
            annotation = stmt.annotation
            readonly = False
            if isinstance(annotation, ast.Subscript):
                is_final = (
                    isinstance(annotation.value, ast.Name) and annotation.value.id == "Final"
                ) or (
                    isinstance(annotation.value, ast.Attribute) and annotation.value.attr == "Final"
                )
                if is_final:
                    readonly = True
                    annotation = annotation.slice

            param_type = _annotation_to_parameter_type(annotation)

            # Look for a docstring immediately following the attribute definition.
            attr_doc = default_doc
            if idx + 1 < len(class_node.body):
                next_stmt = class_node.body[idx + 1]
                if (
                    isinstance(next_stmt, ast.Expr)
                    and isinstance(next_stmt.value, ast.Constant)
                    and isinstance(next_stmt.value.value, str)
                ):
                    docstring = next_stmt.value.value

                    # Parse the docstring to build a Documentation object.
                    attr_doc = _parse_docstring_for_documentation(docstring)

            param = Parameter(Name=name, Type=param_type)
            attr = Attribute(
                Documentation=attr_doc,
                Parameter=param,
                Name=name,
                ReadOnly=readonly,
            )
            attributes.append(attr)

    return attributes


def _collect_function_defs(nodes) -> list[ast.FunctionDef]:
    funcs = []
    for node in nodes:
        if isinstance(node, ast.FunctionDef):
            funcs.append(node)
        elif isinstance(node, ast.If):
            funcs.extend(_collect_function_defs(node.body))
            funcs.extend(_collect_function_defs(node.orelse))
    return funcs


def _collect_functions(class_node: ast.ClassDef) -> dict[str, Function]:
    functions: dict[str, Function] = {}
    for func_node in _collect_function_defs(class_node.body):
        if func := functions.get(func_node.name):
            func.update(func_node)
        else:
            functions[func_node.name] = Function(func_node)
    return functions


def _parse_methods(
    functions: dict[str, Function],
    *,
    skip_bound_argument: bool,
    allow_bound_decorators: bool,
) -> List[Method]:
    """
    Parse methods from collected class functions, extracting:
      - Method name
      - Parameters (from the function signature / annotations)
      - Docstring
    """
    methods = []

    for func in functions.values():
        if func.name == "__init__":
            continue
        if func.typing_only_flag:
            continue
        if not allow_bound_decorators and (func.static_flag or func.class_flag or func.const_flag):
            raise ValueError(
                f"Module-level function '{func.name}' cannot use bound-method decorators"
            )
        doc_obj = _parse_docstring_for_documentation(func.docstring)
        func.add_signature_docs(doc_obj)
        method_params = []

        signature = func.signature
        if signature is None:
            continue

        # Process positional parameters (skipping self/cls)
        for arg_i, arg in enumerate(signature.args):
            param_name = arg.name
            if skip_bound_argument and arg_i == 0 and param_name in ("self", "cls"):
                continue
            param_type = _python_type_to_parameter_type(arg.annotation)
            method_params.append(Parameter(Name=param_name, Type=param_type))

        method = Method(
            Name=func.name,
            Documentation=doc_obj,
            Parameter=method_params,
            Const=func.const_flag if allow_bound_decorators else False,
            Static=func.static_flag if allow_bound_decorators else False,
            Class=func.class_flag if allow_bound_decorators else False,
            Keyword=func.has_keywords,
            NoArgs=func.noargs_flag,
        )

        methods.append(method)

    return methods


def _get_module_from_path(path: str) -> str:
    """
    Returns the name of the FreeCAD module from the path.
    Examples:
        .../src/Base/Persistence.py    -> "Base"
        .../src/Mod/CAM/Path/__init__.py -> "CAM"
    """
    # 1. Split the path by the OS separator.
    import os

    parts = path.split(os.sep)

    # 2. Attempt to find "src" in the path components.
    try:
        idx_src = len(parts) - 1 - list(reversed(parts)).index("src")
    except ValueError:
        # If "src" is not found, we cannot determine the module name.
        return None

    # 3. Check if there is a path component immediately after "src".
    #    If there isn't, we have nothing to return.
    if idx_src + 1 >= len(parts):
        return None

    next_part = parts[idx_src + 1]

    # 4. If the next component is "Mod", then the module name is the
    #    component AFTER "Mod" (e.g. "CAM" in "Mod/CAM").
    if next_part == "Mod":
        if idx_src + 2 < len(parts):
            return parts[idx_src + 2]
        else:
            # "Mod" is the last component
            return None
    else:
        # 5. Otherwise, if it's not "Mod", we treat that next component
        #    itself as the module name (e.g. "Base").
        return next_part


def _extract_module_name(import_path: str, default_module: str) -> str:
    """
    Given an import_path like "Base.Foo", return "Base".
    If import_path has no dot (e.g., "Foo"), return default_module.

    Examples:
        extract_module_name("Base.Foo", default_module="Fallback")  -> "Base"
        extract_module_name("Foo", default_module="Fallback")       -> "Fallback"
    """
    if "." in import_path:
        # Take everything before the first dot
        return import_path.split(".", 1)[0]
    else:
        # No dot, return the fallback module name
        return default_module


def _get_module_path(module_name: str) -> str:
    if module_name in ["Base", "App", "Gui"]:
        return module_name
    return "Mod/" + module_name


def _parse_imports(tree) -> dict:
    """
    Parses the given source_code for import statements and constructs
    a mapping from imported name -> module path.

    For example, code like:

        from Metadata import export, forward_declarations, constmethod
        from PyObjectBase import PyObjectBase
        from Base.Foo import Foo
        from typing import List, Final

    yields a mapping of:
        {
            "export": "Metadata",
            "forward_declarations": "Metadata",
            "constmethod": "Metadata",
            "PyObjectBase": "PyObjectBase",
            "Foo": "Base.Foo",
            "List": "typing",
            "Final": "typing"
        }
    """
    name_to_module_map = {}

    for node in tree.body:
        match node:
            # Handle 'import X' or 'import X as Y'
            case ast.Import(names=names):
                # e.g. import foo, import foo as bar
                for alias in names:
                    imported_name = alias.asname if alias.asname else alias.name
                    name_to_module_map[imported_name] = alias.name
            # Handle 'from X import Y, Z as W'
            case ast.ImportFrom(module=module, names=names):
                module_name = module if module is not None else ""
                for alias in names:
                    imported_name = alias.asname if alias.asname else alias.name
                    name_to_module_map[imported_name] = module_name
            case _:
                pass

    return name_to_module_map


def _get_native_class_name(klass: str) -> str:
    return klass


def _get_native_python_class_name(klass: str) -> str:
    if klass == "PyObjectBase":
        return klass
    return klass + "Py"


def _source_root_from_path(path: str | Path) -> Path | None:
    file_path = Path(path).resolve()
    parts = file_path.parts
    try:
        src_index = len(parts) - 1 - list(reversed(parts)).index("src")
    except ValueError:
        return None
    return Path(*parts[: src_index + 1])


def _header_path_from_pyi(path: Path, source_root: Path, header_stem: str) -> str:
    return (path.resolve().parent.relative_to(source_root) / f"{header_stem}.h").as_posix()


def _include_path_from_pyi(path: str | Path, module_name: str, native_class_name: str) -> str:
    source_root = _source_root_from_path(path)
    if source_root is not None:
        return _header_path_from_pyi(Path(path), source_root, native_class_name)
    return _get_module_path(module_name) + "/" + native_class_name + ".h"


def _source_header_exists(source_root: Path, include: str) -> bool:
    return bool(include) and (source_root / include).is_file()


def _is_exported_class(class_node: ast.ClassDef) -> bool:
    return any(_decorator_name(decorator) == "export" for decorator in class_node.decorator_list)


def _export_kwargs_from_class(class_node: ast.ClassDef) -> dict:
    for decorator in class_node.decorator_list:
        if _decorator_name(decorator) == "export":
            return _extract_decorator_kwargs(decorator)
    return {}


def _inherited_include_parent_info(
    class_node: ast.ClassDef,
    path: Path,
    source_root: Path,
    imports_mapping: dict[str, str] | None,
    default_include: str,
    visited: frozenset[tuple[str, str]],
) -> BindingClassInfo | None:
    if not imports_mapping or not class_node.bases:
        return None
    if _source_header_exists(source_root, default_include):
        return None

    base_class_name = _extract_base_class_name(class_node.bases[0])
    parent_info = _infer_binding_parent_info(
        path,
        imports_mapping.get(base_class_name, ""),
        base_class_name,
        visited=visited,
    )
    if not parent_info or not parent_info.is_exported:
        return None
    if not _source_header_exists(source_root, parent_info.include):
        return None
    return parent_info


def _inherited_include_from_parent(
    class_node: ast.ClassDef,
    path: Path,
    source_root: Path,
    imports_mapping: dict[str, str] | None,
    default_include: str,
    visited: frozenset[tuple[str, str]],
) -> str:
    parent_info = _inherited_include_parent_info(
        class_node,
        path,
        source_root,
        imports_mapping,
        default_include,
        visited,
    )
    if not parent_info:
        return ""
    return parent_info.include


def _binding_info_from_class(
    class_node: ast.ClassDef,
    path: Path,
    source_root: Path,
    imports_mapping: dict[str, str] | None = None,
    visited: frozenset[tuple[str, str]] | None = None,
    infer_parent_include: bool = True,
) -> BindingClassInfo:
    if visited is None:
        visited = frozenset()
    visited = visited | {(path.resolve().as_posix(), class_node.name)}

    export_kwargs = _export_kwargs_from_class(class_node)
    export_name = export_kwargs.get("Name", "") or _get_native_python_class_name(class_node.name)
    python_name = export_kwargs.get("PythonName", None)
    if not isinstance(python_name, str) or not python_name:
        python_name = None
    module_name = _get_module_from_path(path.as_posix()) or ""
    default_include = _include_path_from_pyi(path, module_name, class_node.name)
    include = export_kwargs.get("Include", "")
    include_parent_info = None
    if not include and infer_parent_include:
        include_parent_info = _inherited_include_parent_info(
            class_node,
            path,
            source_root,
            imports_mapping,
            default_include,
            visited,
        )
        if include_parent_info:
            include = include_parent_info.include
    source_dependencies = ()
    if include_parent_info:
        source_dependencies = (
            include_parent_info.path,
            *include_parent_info.source_dependencies,
        )
    return BindingClassInfo(
        class_name=class_node.name,
        export_name=export_name,
        python_name=python_name,
        namespace=export_kwargs.get("Namespace", "") or module_name or "",
        include=include or default_include,
        is_exported=_is_exported_class(class_node),
        path=path,
        father_include=_header_path_from_pyi(path, source_root, export_name),
        source_dependencies=source_dependencies,
    )


def _binding_info_from_file(
    path: Path,
    source_root: Path,
    class_name: str,
    visited: frozenset[tuple[str, str]] | None = None,
) -> BindingClassInfo | None:
    if not path.exists():
        return None
    key = (path.resolve().as_posix(), class_name)
    if visited and key in visited:
        return None

    try:
        tree = ast.parse(path.read_text(encoding="utf-8"))
    except (OSError, SyntaxError):
        return None
    imports_mapping = _parse_imports(tree)

    for node in tree.body:
        if isinstance(node, ast.ClassDef) and node.name == class_name:
            return _binding_info_from_class(
                node,
                path,
                source_root,
                imports_mapping=imports_mapping,
                visited=visited,
            )

    return None


def _add_unique_path(paths: list[Path], path: Path) -> None:
    if path not in paths:
        paths.append(path)


def _add_source_dependency(dependencies: list[str], path: Path) -> None:
    dependency = path.resolve().as_posix()
    if dependency not in dependencies:
        dependencies.append(dependency)


def _module_path_variants(import_path: str, class_name: str) -> list[tuple[str, ...]]:
    module_parts = tuple(part for part in import_path.strip(".").split(".") if part)
    if not module_parts:
        return [(class_name,)]

    variants = [
        module_parts,
        (*module_parts, class_name),
    ]

    last = module_parts[-1]
    if last.endswith("Py"):
        variants.append((*module_parts[:-1], last[: -len("Py")]))
    if last != class_name:
        variants.append((*module_parts[:-1], class_name))

    result = []
    for variant in variants:
        if variant and variant not in result:
            result.append(variant)
    return result


def _candidate_parent_pyi_paths(
    path: str | Path,
    imported_from_module: str,
    class_name: str,
    source_root: Path,
) -> list[Path]:
    current_path = Path(path).resolve()
    candidates = []

    import_path = imported_from_module.strip(".")
    if import_path and "." not in import_path:
        _add_unique_path(candidates, current_path.parent / f"{import_path}.pyi")
        if import_path.endswith("Py"):
            _add_unique_path(candidates, current_path.parent / f"{import_path[:-2]}.pyi")

    for variant in _module_path_variants(import_path, class_name):
        first = variant[0]
        if first == "Mod" and len(variant) > 2:
            _add_unique_path(candidates, source_root.joinpath(*variant).with_suffix(".pyi"))
            continue

        if first in {"Base", "App", "Gui"}:
            _add_unique_path(candidates, source_root.joinpath(*variant).with_suffix(".pyi"))
            continue

        if len(variant) > 1:
            module_name = first
            rest = variant[1:]
            _add_unique_path(
                candidates,
                source_root.joinpath("Mod", module_name, *rest).with_suffix(".pyi"),
            )
            if rest[0] not in {"App", "Gui"}:
                _add_unique_path(
                    candidates,
                    source_root.joinpath("Mod", module_name, "App", *rest).with_suffix(".pyi"),
                )
                _add_unique_path(
                    candidates,
                    source_root.joinpath("Mod", module_name, "Gui", *rest).with_suffix(".pyi"),
                )

    return candidates


def _module_name_candidates(info: BindingClassInfo, source_root: Path) -> set[str]:
    rel_path = info.path.relative_to(source_root)
    stem = rel_path.stem
    dirs = rel_path.parts[:-1]
    candidates = {stem}

    if dirs:
        candidates.add(".".join((*dirs, stem)))
        candidates.add(".".join((*dirs, info.export_name)))

    if dirs and dirs[0] == "Mod" and len(dirs) >= 2:
        module_name = dirs[1]
        rest = dirs[2:]
        candidates.add(".".join((module_name, *rest, stem)))
        candidates.add(".".join((module_name, *rest, info.export_name)))

        collapsed_rest = tuple(part for part in rest if part not in {"App", "Gui"})
        candidates.add(".".join((module_name, *collapsed_rest, stem)))
        candidates.add(".".join((module_name, *collapsed_rest, info.export_name)))

    if info.python_name:
        candidates.add(info.python_name)
        python_name_parts = tuple(info.python_name.split("."))
        candidates.add(".".join((*python_name_parts[:-1], info.export_name)))

    return {candidate for candidate in candidates if candidate}


@lru_cache(maxsize=None)
def _binding_class_index(source_root: str) -> tuple[dict, dict, dict]:
    root = Path(source_root)
    by_module_and_class = {}
    by_directory_module_and_class = {}
    by_class = {}

    for pyi_path in root.rglob("*.pyi"):
        try:
            tree = ast.parse(pyi_path.read_text(encoding="utf-8"))
        except (OSError, SyntaxError):
            continue
        imports_mapping = _parse_imports(tree)

        for node in tree.body:
            if not isinstance(node, ast.ClassDef):
                continue

            info = _binding_info_from_class(
                node,
                pyi_path,
                root,
                imports_mapping=imports_mapping,
                infer_parent_include=False,
            )
            by_class.setdefault(info.class_name, []).append(info)

            directory_key = pyi_path.parent.relative_to(root).as_posix()
            by_directory_module_and_class.setdefault(
                (directory_key, pyi_path.stem, info.class_name), []
            ).append(info)
            by_directory_module_and_class.setdefault(
                (directory_key, info.export_name, info.class_name), []
            ).append(info)

            for module_name in _module_name_candidates(info, root):
                by_module_and_class.setdefault((module_name, info.class_name), []).append(info)

    return by_module_and_class, by_directory_module_and_class, by_class


def _single_info(items: list[BindingClassInfo] | None) -> BindingClassInfo | None:
    if items and len(items) == 1:
        return items[0]
    return None


def _binding_info_from_index(
    path: str | Path,
    imported_from_module: str,
    class_name: str,
    source_root: Path,
    visited: frozenset[tuple[str, str]] | None = None,
) -> BindingClassInfo | None:
    by_module_and_class, by_directory_module_and_class, by_class = _binding_class_index(
        source_root.as_posix()
    )

    current_path = Path(path).resolve()
    current_dir_key = current_path.parent.relative_to(source_root).as_posix()
    import_path = imported_from_module.strip(".")

    if import_path and "." not in import_path:
        info = _single_info(
            by_directory_module_and_class.get((current_dir_key, import_path, class_name))
        )
        if info:
            return _binding_info_from_file(info.path, source_root, class_name, visited) or info

    info = _single_info(by_module_and_class.get((import_path, class_name)))
    if info:
        return _binding_info_from_file(info.path, source_root, class_name, visited) or info

    if import_path.endswith("Py"):
        info = _single_info(by_module_and_class.get((import_path[: -len("Py")], class_name)))
        if info:
            return _binding_info_from_file(info.path, source_root, class_name, visited) or info

    info = _single_info(by_class.get(class_name))
    if info:
        return _binding_info_from_file(info.path, source_root, class_name, visited) or info
    return None


def _infer_binding_parent_info(
    path: str | Path,
    imported_from_module: str,
    class_name: str,
    visited: frozenset[tuple[str, str]] | None = None,
    use_index: bool = True,
) -> BindingClassInfo | None:
    source_root = _source_root_from_path(path)
    if source_root is None:
        return None

    for candidate in _candidate_parent_pyi_paths(
        path, imported_from_module, class_name, source_root
    ):
        info = _binding_info_from_file(candidate, source_root, class_name, visited)
        if info:
            return info

    if not use_index:
        return None
    return _binding_info_from_index(path, imported_from_module, class_name, source_root, visited)


def _extract_base_class_name(base: ast.expr) -> str:
    """
    Extract the base class name from an AST node using ast.unparse.
    For generic bases (e.g. GenericParent[T]), it removes the generic part.
    For qualified names (e.g. some_module.ParentClass), it returns only the last part.
    """
    base_str = ast.unparse(base)
    # Remove generic parameters if present.
    if "[" in base_str:
        base_str = base_str.split("[", 1)[0]
    # For qualified names, take only the class name.
    if "." in base_str:
        base_str = base_str.split(".")[-1]
    return base_str


def _parse_class(
    class_node,
    source_code: str,
    path: str,
    imports_mapping: dict,
    source_dependencies: list[str] | None = None,
) -> PythonExport:
    base_class_name = None
    for base in class_node.bases:
        base_class_name = _extract_base_class_name(base)
        break  # Only consider the first base class.

    assert base_class_name is not None

    is_exported = False
    export_decorator_kwargs = {}
    forward_declarations_text = ""
    class_declarations_text = ""
    sequence_protocol_kwargs = None

    for decorator in class_node.decorator_list:
        match decorator:
            case ast.Name(id="export"):
                export_decorator_kwargs = {}
                is_exported = True
            case ast.Call(func=ast.Name(id="export"), keywords=_, args=_):
                export_decorator_kwargs = _extract_decorator_kwargs(decorator)
                is_exported = True
            case ast.Call(func=ast.Name(id="forward_declarations"), args=args):
                if args:
                    match args[0]:
                        case ast.Constant(value=val):
                            forward_declarations_text = val
            case ast.Call(func=ast.Name(id="class_declarations"), args=args):
                if args:
                    match args[0]:
                        case ast.Constant(value=val):
                            class_declarations_text = val
            case ast.Call(func=ast.Name(id="sequence_protocol"), keywords=_, args=_):
                sequence_protocol_kwargs = _extract_decorator_kwargs(decorator)
            case _:
                pass

    if "Constructor" in export_decorator_kwargs:
        raise ValueError(
            f"@export(Constructor=...) is no longer supported for class "
            f"'{class_node.name}'; define __init__ in the pyi class instead."
        )

    # Parse imports to compute module metadata
    module_name = _get_module_from_path(path)

    imported_from_module = imports_mapping.get(base_class_name, "")
    parent_module_name = _extract_module_name(imported_from_module, module_name)

    functions = _collect_functions(class_node)
    class_docstring = ast.get_docstring(class_node) or ""
    doc_obj = _parse_docstring_for_documentation(class_docstring)
    if constructor := functions.get("__init__"):
        if constructor.signature is None and constructor.docstring.strip():
            _append_user_doc(doc_obj, _constructor_user_doc(constructor, class_node.name))
    has_constructor = "__init__" in functions
    class_attributes = _parse_class_attributes(class_node, source_code)
    class_methods = _parse_methods(
        functions,
        skip_bound_argument=True,
        allow_bound_decorators=True,
    )

    native_class_name = _get_native_class_name(class_node.name)
    native_python_class_name = _get_native_python_class_name(class_node.name)
    include = _include_path_from_pyi(path, module_name, native_class_name)
    source_root = _source_root_from_path(path)
    include_parent_info = None
    if source_root is not None:
        include_parent_info = _inherited_include_parent_info(
            class_node,
            Path(path),
            source_root,
            imports_mapping,
            include,
            frozenset({(Path(path).resolve().as_posix(), class_node.name)}),
        )
        if include_parent_info:
            include = include_parent_info.include
    twin = export_decorator_kwargs.get("Twin", "") or native_class_name
    twin_pointer = export_decorator_kwargs.get("TwinPointer", "") or twin

    father_info = _infer_binding_parent_info(path, imported_from_module, base_class_name)
    if father_info:
        if source_dependencies is not None and (
            not export_decorator_kwargs.get("Father")
            or not export_decorator_kwargs.get("FatherInclude")
            or not export_decorator_kwargs.get("FatherNamespace")
            or not export_decorator_kwargs.get("Include")
        ):
            _add_source_dependency(source_dependencies, father_info.path)
        father_native_python_class_name = father_info.export_name
        father_include = father_info.father_include
        father_namespace = father_info.namespace
    else:
        father_native_python_class_name = _get_native_python_class_name(base_class_name)
        father_include = (
            _get_module_path(parent_module_name) + "/" + father_native_python_class_name + ".h"
        )
        father_namespace = parent_module_name
    if (
        source_dependencies is not None
        and not export_decorator_kwargs.get("Include")
        and include_parent_info
    ):
        _add_source_dependency(source_dependencies, include_parent_info.path)
        for dependency in include_parent_info.source_dependencies:
            _add_source_dependency(source_dependencies, dependency)

    py_export = PythonExport(
        Documentation=doc_obj,
        ModuleName=module_name,
        Name=export_decorator_kwargs.get("Name", "") or native_python_class_name,
        PythonName=export_decorator_kwargs.get("PythonName", "") or None,
        Include=export_decorator_kwargs.get("Include", "") or include,
        Father=export_decorator_kwargs.get("Father", "") or father_native_python_class_name,
        Twin=twin,
        TwinPointer=twin_pointer,
        Namespace=export_decorator_kwargs.get("Namespace", "") or module_name,
        FatherInclude=export_decorator_kwargs.get("FatherInclude", "") or father_include,
        FatherNamespace=export_decorator_kwargs.get("FatherNamespace", "") or father_namespace,
        Constructor=has_constructor,
        NumberProtocol=export_decorator_kwargs.get("NumberProtocol", False),
        RichCompare=export_decorator_kwargs.get("RichCompare", False),
        Delete=export_decorator_kwargs.get("Delete", False),
        Reference=export_decorator_kwargs.get("Reference", None),
        Initialization=export_decorator_kwargs.get("Initialization", False),
        DisableNotify=export_decorator_kwargs.get("DisableNotify", False),
        DescriptorGetter=export_decorator_kwargs.get("DescriptorGetter", False),
        DescriptorSetter=export_decorator_kwargs.get("DescriptorSetter", False),
        HeaderIncludes=_filter_header_includes(
            _attribute_header_includes(class_attributes),
            export_decorator_kwargs.get("FatherInclude", "") or father_include,
            export_decorator_kwargs.get("Include", "") or include,
        ),
        ForwardDeclarations=forward_declarations_text,
        ClassDeclarations=class_declarations_text,
        IsExplicitlyExported=is_exported,
    )

    # Attach sequence protocol metadata if provided.
    if sequence_protocol_kwargs is not None:
        seq_protocol = SequenceProtocol(**sequence_protocol_kwargs)
        py_export.Sequence = seq_protocol

    py_export.Attribute.extend(class_attributes)
    py_export.Methode.extend(class_methods)

    return py_export


def _extract_module_kwargs(tree: ast.Module) -> tuple[dict, bool]:
    for node in tree.body:
        if not isinstance(node, ast.Expr) or not isinstance(node.value, ast.Call):
            continue

        name = _decorator_name(node.value.func)
        if name != "module":
            continue

        return _extract_decorator_kwargs(node.value), True

    return {}, False


def _module_export_name_from_path(path: str) -> str:
    import os

    filename = os.path.basename(path)
    suffix = ".module.pyi"
    if filename.endswith(suffix):
        return filename[: -len(suffix)]
    return os.path.splitext(filename)[0]


def parse_module_python_code(path: str) -> GenerateModel:
    with open(path, "r", encoding="utf-8") as file:
        source_code = file.read()

    tree = ast.parse(source_code)

    class_defs = [node for node in tree.body if isinstance(node, ast.ClassDef)]
    if class_defs:
        raise ValueError(".module.pyi files cannot contain exported classes")

    functions = _collect_functions(tree)
    if not functions:
        raise ValueError("No module-level functions found for export.")

    methods = _parse_methods(
        functions,
        skip_bound_argument=False,
        allow_bound_decorators=False,
    )
    metadata, explicit_export = _extract_module_kwargs(tree)

    module_name = _get_module_from_path(path)
    export_name = metadata.get("Name", "") or _module_export_name_from_path(path)
    namespace = metadata.get("Namespace", "") or export_name
    doc_obj = _parse_docstring_for_documentation(_get_module_docstring(tree))

    model = GenerateModel()
    model.PythonModule.append(
        PythonModuleExport(
            Documentation=doc_obj,
            Method=methods,
            ModuleName=module_name or "",
            Name=export_name,
            Namespace=namespace,
            IsExplicitlyExported=explicit_export,
        )
    )
    return model


def parse_python_code(path: str) -> GenerateModel:
    """
    Parse the given Python source code and build a GenerateModel containing
    PythonExport entries. If any class is explicitly exported using @export,
    only those classes are used. If no classes have the @export decorator,
    then a single non-exported class is assumed to be the export. If there
    are multiple non-exported classes, an exception is raised.
    """
    with open(path, "r") as file:
        source_code = file.read()

    tree = ast.parse(source_code)
    imports_mapping = _parse_imports(tree)

    explicit_exports = []
    explicit_source_dependencies = []
    non_explicit_exports = []

    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            class_source_dependencies = []
            py_export = _parse_class(
                node, source_code, path, imports_mapping, class_source_dependencies
            )
            if py_export.IsExplicitlyExported:
                explicit_exports.append(py_export)
                for dependency in class_source_dependencies:
                    if dependency not in explicit_source_dependencies:
                        explicit_source_dependencies.append(dependency)
            else:
                non_explicit_exports.append((py_export, class_source_dependencies))

    model = GenerateModel()
    if explicit_exports:
        # Use only explicitly exported classes.
        model.PythonExport.extend(explicit_exports)
        model.SourceDependencies.extend(explicit_source_dependencies)
    else:
        # No explicit exports; allow only one non-exported class.
        if len(non_explicit_exports) == 1:
            py_export, source_dependencies = non_explicit_exports[0]
            model.PythonExport.append(py_export)
            model.SourceDependencies.extend(source_dependencies)
        elif len(non_explicit_exports) > 1:
            raise Exception(
                "Multiple non explicitly-exported classes were found, please use @export."
            )
        else:
            raise Exception("No classes found for export.")

    return model


def parse(path):
    if path.endswith(".module.pyi"):
        return parse_module_python_code(path)
    model = parse_python_code(path)
    return model


def main():
    import sys

    args = sys.argv[1:]
    model = parse(args[0])
    model.dump()


if __name__ == "__main__":
    main()
