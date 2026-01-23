# SPDX-License-Identifier: LGPL-2.1-or-later

"""Parses Python binding interface files into a typed AST model."""

from dataclasses import dataclass
from enum import Enum
import ast
import re
from typing import List
from model.typedModel import (
    GenerateModel,
    PythonExport,
    Methode,
    Attribute,
    Documentation,
    Author,
    Parameter,
    ParameterType,
    SequenceProtocol,
)

SIGNATURE_SEP = re.compile(r"\s+--\s+", re.DOTALL)
SELF_CLS_ARG = re.compile(r"\(\s*(self|cls)(\s*,\s*)?")


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
        all_args = [*args.posonlyargs, *args.args, args.vararg, *args.kwonlyargs, args.kwarg]
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


class Function:
    name: str
    signatures: list[FunctionSignature]

    def __init__(self, func: ast.FunctionDef) -> None:
        self.name = func.name
        self.signatures = [FunctionSignature(func)]

    def update(self, func: ast.FunctionDef) -> None:
        self.signatures.append(FunctionSignature(func))

    @property
    def docstring(self) -> str:
        return "\n".join((f.docstring for f in self.signatures))

    @property
    def has_keywords(self) -> bool:
        overloads = len(self.signatures) > 1
        if overloads:
            return any(sig.has_keywords for sig in self.signatures if sig.is_overload)
        return self.signatures[0].has_keywords

    @property
    def signature(self) -> FunctionSignature | None:
        """First non overload signature"""
        for sig in self.signatures:
            if not sig.is_overload:
                return sig
        return None

    @property
    def static_flag(self) -> bool:
        return any(sig.static_flag for sig in self.signatures)

    @property
    def const_flag(self) -> bool:
        return any(sig.const_flag for sig in self.signatures)

    @property
    def class_flag(self) -> bool:
        return any(sig.class_flag for sig in self.signatures)

    @property
    def noargs_flag(self) -> bool:
        return any(sig.noargs_flag for sig in self.signatures)

    def add_signature_docs(self, doc: Documentation) -> None:
        if len(self.signatures) == 1:
            docstring = [self.signatures[0].text]
            signature = [self.signatures[0].annotated_text]
        else:
            docstring = [sig.text for sig in self.signatures if not sig.is_overload]
            signature = [sig.annotated_text for sig in self.signatures if sig.is_overload]

        if not docstring:
            return

        user_doc = doc.UserDocu or ""
        marker = SIGNATURE_SEP.search(user_doc)
        if marker:
            user_doc = user_doc[marker.end() :].strip()

        docstring.append("--\n")  # mark __text_signature__
        docstring.extend(signature)  # Include real annotated signature in user docstring
        docstring.append(f"\n{user_doc}")  # Rest of the docstring
        doc.UserDocu = "\n".join(docstring)


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
        case _ if py_type.startswith(("pycxxvector")):
            return ParameterType.VECTOR
        case _:
            return ParameterType.OBJECT


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
            # Evaluate the type annotation and detect Final for read-only attributes
            if isinstance(stmt.annotation, ast.Name):
                # e.g. `str`
                type_name = stmt.annotation.id
                readonly = False
            elif isinstance(stmt.annotation, ast.Subscript):
                # Check if this is a Final type hint, e.g. Final[int] or typing.Final[int]
                is_final = (
                    isinstance(stmt.annotation.value, ast.Name)
                    and stmt.annotation.value.id == "Final"
                ) or (
                    isinstance(stmt.annotation.value, ast.Attribute)
                    and stmt.annotation.value.attr == "Final"
                )
                if is_final:
                    readonly = True
                    # Extract the inner type from the Final[...] annotation
                    type_name = _get_type_str(stmt.annotation.slice)
                else:
                    type_name = _get_type_str(stmt.annotation)
                    readonly = False
            else:
                type_name = "object"
                readonly = False

            param_type = _python_type_to_parameter_type(type_name)

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


def _parse_methods(class_node: ast.ClassDef) -> List[Methode]:
    """
    Parse methods from the class AST node, extracting:
      - Method name
      - Parameters (from the function signature / annotations)
      - Docstring
    """
    methods = []

    def collect_function_defs(nodes) -> list[ast.FunctionDef]:
        funcs = []
        for node in nodes:
            if isinstance(node, ast.FunctionDef):
                funcs.append(node)
            elif isinstance(node, ast.If):
                funcs.extend(collect_function_defs(node.body))
                funcs.extend(collect_function_defs(node.orelse))
        return funcs

    # Collect including overloads
    functions: dict[str, Function] = {}
    for func_node in collect_function_defs(class_node.body):
        if func := functions.get(func_node.name):
            func.update(func_node)
        else:
            functions[func_node.name] = Function(func_node)

    for func in functions.values():
        doc_obj = _parse_docstring_for_documentation(func.docstring)
        func.add_signature_docs(doc_obj)
        method_params = []

        signature = func.signature
        if signature is None:
            continue

        # Process positional parameters (skipping self/cls)
        for arg_i, arg in enumerate(signature.args):
            param_name = arg.name
            if arg_i == 0 and param_name in ("self", "cls"):
                continue
            param_type = _python_type_to_parameter_type(arg.annotation)
            method_params.append(Parameter(Name=param_name, Type=param_type))

        method = Methode(
            Name=func.name,
            Documentation=doc_obj,
            Parameter=method_params,
            Const=func.const_flag,
            Static=func.static_flag,
            Class=func.class_flag,
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


def _parse_class(class_node, source_code: str, path: str, imports_mapping: dict) -> PythonExport:
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

    # Parse imports to compute module metadata
    module_name = _get_module_from_path(path)

    imported_from_module = imports_mapping[base_class_name]
    parent_module_name = _extract_module_name(imported_from_module, module_name)

    class_docstring = ast.get_docstring(class_node) or ""
    doc_obj = _parse_docstring_for_documentation(class_docstring)
    class_attributes = _parse_class_attributes(class_node, source_code)
    class_methods = _parse_methods(class_node)

    native_class_name = _get_native_class_name(class_node.name)
    native_python_class_name = _get_native_python_class_name(class_node.name)
    include = _get_module_path(module_name) + "/" + native_class_name + ".h"

    father_native_python_class_name = _get_native_python_class_name(base_class_name)
    father_include = (
        _get_module_path(parent_module_name) + "/" + father_native_python_class_name + ".h"
    )

    py_export = PythonExport(
        Documentation=doc_obj,
        ModuleName=module_name,
        Name=export_decorator_kwargs.get("Name", "") or native_python_class_name,
        PythonName=export_decorator_kwargs.get("PythonName", "") or None,
        Include=export_decorator_kwargs.get("Include", "") or include,
        Father=export_decorator_kwargs.get("Father", "") or father_native_python_class_name,
        Twin=export_decorator_kwargs.get("Twin", "") or native_class_name,
        TwinPointer=export_decorator_kwargs.get("TwinPointer", "") or native_class_name,
        Namespace=export_decorator_kwargs.get("Namespace", "") or module_name,
        FatherInclude=export_decorator_kwargs.get("FatherInclude", "") or father_include,
        FatherNamespace=export_decorator_kwargs.get("FatherNamespace", "") or parent_module_name,
        Constructor=export_decorator_kwargs.get("Constructor", False),
        NumberProtocol=export_decorator_kwargs.get("NumberProtocol", False),
        RichCompare=export_decorator_kwargs.get("RichCompare", False),
        Delete=export_decorator_kwargs.get("Delete", False),
        Reference=export_decorator_kwargs.get("Reference", None),
        Initialization=export_decorator_kwargs.get("Initialization", False),
        DisableNotify=export_decorator_kwargs.get("DisableNotify", False),
        DescriptorGetter=export_decorator_kwargs.get("DescriptorGetter", False),
        DescriptorSetter=export_decorator_kwargs.get("DescriptorSetter", False),
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
    non_explicit_exports = []

    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            py_export = _parse_class(node, source_code, path, imports_mapping)
            if py_export.IsExplicitlyExported:
                explicit_exports.append(py_export)
            else:
                non_explicit_exports.append(py_export)

    model = GenerateModel()
    if explicit_exports:
        # Use only explicitly exported classes.
        model.PythonExport.extend(explicit_exports)
    else:
        # No explicit exports; allow only one non-exported class.
        if len(non_explicit_exports) == 1:
            model.PythonExport.append(non_explicit_exports[0])
        elif len(non_explicit_exports) > 1:
            raise Exception(
                "Multiple non explicitly-exported classes were found, please use @export."
            )
        else:
            raise Exception("No classes found for export.")

    return model


def parse(path):
    model = parse_python_code(path)
    return model


def main():
    import sys

    args = sys.argv[1:]
    model = parse(args[0])
    model.dump()


if __name__ == "__main__":
    main()
