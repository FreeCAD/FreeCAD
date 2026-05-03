# pyright: strict

"""Extract a neutral C++ API model from Doxygen XML output."""

from __future__ import annotations

from pathlib import Path
from xml.etree import ElementTree as ET

from .cpp_api_model import (
    CppApiClass,
    CppApiEnum,
    CppApiEnumValue,
    CppApiFunction,
    CppApiModel,
    CppApiNamespace,
    CppSourceLocation,
)

IGNORED_NAMESPACE_NAMES = {"std"}
ALLOWED_TOP_NAMESPACES = {"App", "Attacher", "Base", "Data", "Gui", "Part"}
ALLOWED_SOURCE_PREFIXES = (
    "src/App/",
    "src/Base/",
    "src/Gui/",
    "src/Mod/Part/App/",
)
MEMBER_SECTION_KINDS = {"func", "public-func", "public-static-func"}
ENUM_SECTION_KINDS = {"enum", "public-type"}


def compact_text(text: str) -> str:
    return " ".join(text.split())


def description_text(element: ET.Element | None) -> str | None:
    if element is None:
        return None
    paragraphs: list[str] = []
    for para in element.findall(".//para"):
        text = compact_text("".join(para.itertext()))
        if text:
            paragraphs.append(text)
    if paragraphs:
        return "\n\n".join(paragraphs)
    text = compact_text("".join(element.itertext()))
    return text or None


def source_location(root: Path, location: ET.Element | None) -> CppSourceLocation | None:
    if location is None:
        return None
    file_path = location.get("file")
    if not file_path:
        return None
    line = location.get("line")
    resolved = Path(file_path)
    relative_path = resolved.relative_to(root) if resolved.is_absolute() else resolved
    return CppSourceLocation(
        path=relative_path.as_posix(),
        line=int(line) if line and line.isdigit() else None,
    )


def top_namespace(qualified_name: str) -> str:
    return qualified_name.split("::", 1)[0]


def allowed_top_namespace(qualified_name: str) -> bool:
    return top_namespace(qualified_name) in ALLOWED_TOP_NAMESPACES


def project_source_location(location: CppSourceLocation | None) -> bool:
    if location is None:
        return False
    return any(location.path.startswith(prefix) for prefix in ALLOWED_SOURCE_PREFIXES)


def class_display_name(qualified_name: str) -> str:
    parts = qualified_name.split("::")
    if len(parts) <= 2:
        return parts[-1]
    return "::".join(parts[1:])


def function_declaration(member: ET.Element) -> str:
    member_type = compact_text("".join((member.findtext("type") or "").splitlines()))
    definition = compact_text(member.findtext("definition") or member.findtext("name") or "")
    args = compact_text(member.findtext("argsstring") or "")
    trailing = compact_text(member.findtext("exceptions") or "")
    if member_type and not definition.startswith(member_type):
        signature = f"{member_type} {definition}{args}"
    else:
        signature = f"{definition}{args}"
    if trailing:
        signature = f"{signature} {trailing}"
    return compact_text(signature)


def extract_function(member: ET.Element, root: Path) -> CppApiFunction:
    return CppApiFunction(
        name=member.findtext("name") or "",
        declaration=function_declaration(member),
        doc=description_text(member.find("briefdescription"))
        or description_text(member.find("detaileddescription")),
        location=source_location(root, member.find("location")),
    )


def enum_declaration(member: ET.Element) -> str:
    strong = member.get("strong") == "yes"
    prefix = "enum class" if strong else "enum"
    name = member.findtext("name") or ""
    return f"{prefix} {name}".strip()


def extract_enum(member: ET.Element, root: Path) -> CppApiEnum:
    values: list[CppApiEnumValue] = []
    for value in member.findall("enumvalue"):
        initializer = compact_text(value.findtext("initializer") or "")
        values.append(
            CppApiEnumValue(
                name=value.findtext("name") or "",
                initializer=initializer or None,
                doc=description_text(value.find("briefdescription"))
                or description_text(value.find("detaileddescription")),
            )
        )
    return CppApiEnum(
        name=member.findtext("name") or "",
        declaration=enum_declaration(member),
        doc=description_text(member.find("briefdescription"))
        or description_text(member.find("detaileddescription")),
        values=tuple(values),
        location=source_location(root, member.find("location")),
    )


def section_functions(section: ET.Element, root: Path) -> list[CppApiFunction]:
    functions: list[CppApiFunction] = []
    for member in section.findall("memberdef"):
        if member.get("kind") != "function":
            continue
        functions.append(extract_function(member, root))
    return functions


def section_enums(section: ET.Element, root: Path) -> list[CppApiEnum]:
    enums: list[CppApiEnum] = []
    for member in section.findall("memberdef"):
        if member.get("kind") != "enum":
            continue
        enums.append(extract_enum(member, root))
    return enums


def extract_namespace(root: Path, compound: ET.Element) -> CppApiNamespace | None:
    qualified_name = compound.findtext("compoundname") or ""
    if not qualified_name or qualified_name in IGNORED_NAMESPACE_NAMES:
        return None
    location = source_location(root, compound.find("location"))
    if not allowed_top_namespace(qualified_name) or not project_source_location(location):
        return None
    functions: list[CppApiFunction] = []
    enums: list[CppApiEnum] = []
    for section in compound.findall("sectiondef"):
        kind = section.get("kind", "")
        if kind in MEMBER_SECTION_KINDS:
            functions.extend(section_functions(section, root))
        if kind in ENUM_SECTION_KINDS:
            enums.extend(section_enums(section, root))
    return CppApiNamespace(
        qualified_name=qualified_name,
        name=qualified_name.rsplit("::", 1)[-1],
        doc=description_text(compound.find("briefdescription"))
        or description_text(compound.find("detaileddescription")),
        functions=tuple(functions),
        enums=tuple(enums),
        location=location,
    )


def extract_class(root: Path, compound: ET.Element) -> CppApiClass | None:
    qualified_name = compound.findtext("compoundname") or ""
    if "::" not in qualified_name or not allowed_top_namespace(qualified_name):
        return None
    location = source_location(root, compound.find("location"))
    if not project_source_location(location):
        return None
    methods: list[CppApiFunction] = []
    enums: list[CppApiEnum] = []
    for section in compound.findall("sectiondef"):
        kind = section.get("kind", "")
        if kind in MEMBER_SECTION_KINDS:
            methods.extend(section_functions(section, root))
        if kind in ENUM_SECTION_KINDS:
            enums.extend(section_enums(section, root))
    bases = tuple(
        compact_text("".join(base.itertext()))
        for base in compound.findall("basecompoundref")
        if compact_text("".join(base.itertext()))
    )
    return CppApiClass(
        qualified_name=qualified_name,
        name=qualified_name.rsplit("::", 1)[-1],
        display_name=class_display_name(qualified_name),
        top_namespace=top_namespace(qualified_name),
        kind=compound.get("kind", "class"),
        doc=description_text(compound.find("briefdescription"))
        or description_text(compound.find("detaileddescription")),
        bases=bases,
        methods=tuple(methods),
        enums=tuple(enums),
        location=location,
    )


def compound_from_refid(xml_dir: Path, refid: str) -> ET.Element:
    compound_path = xml_dir / f"{refid}.xml"
    tree = ET.parse(compound_path)
    compound = tree.getroot().find("compounddef")
    if compound is None:
        raise ValueError(f"{compound_path}: missing compounddef")
    return compound


def extract_cpp_api_model(root: Path, xml_dir: Path) -> CppApiModel:
    """Build a neutral C++ API model from Doxygen XML output."""

    index_tree = ET.parse(xml_dir / "index.xml")
    namespaces: list[CppApiNamespace] = []
    classes: list[CppApiClass] = []

    for compound_ref in index_tree.getroot().findall("compound"):
        kind = compound_ref.get("kind", "")
        refid = compound_ref.get("refid")
        if not refid:
            continue
        if kind not in {"namespace", "class", "struct"}:
            continue
        compound = compound_from_refid(xml_dir, refid)
        qualified_name = compound.findtext("compoundname") or ""
        if not qualified_name:
            continue
        if top_namespace(qualified_name) in IGNORED_NAMESPACE_NAMES:
            continue
        if kind == "namespace":
            namespace = extract_namespace(root, compound)
            if namespace is not None:
                namespaces.append(namespace)
            continue
        klass = extract_class(root, compound)
        if klass is not None:
            classes.append(klass)

    namespaces.sort(key=lambda namespace: namespace.qualified_name)
    classes.sort(key=lambda klass: klass.qualified_name)
    return CppApiModel(namespaces=tuple(namespaces), classes=tuple(classes))
