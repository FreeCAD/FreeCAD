"""Manual rules for PyCXX type contexts that cannot be derived mechanically.

These rules cover two distinct cases:
- internal helper contexts that should not become public stubs
- exported contexts whose public target names are not discoverable yet

Prefer improving automatic mapping over adding new public-target rules. Keep
manual rules for true special cases such as console/debugger stream helpers.

This file is the explicit exception list for the pipeline. If a generated
public name comes from a hard-coded rule rather than source-derived discovery,
the mapping should be visible here.
"""

from __future__ import annotations

from dataclasses import dataclass

from .model import PublicTypeTarget


@dataclass(frozen=True)
class TypeContextRule:
    """Manual classification for one exact or prefix-matched type context."""

    source: str
    context_name: str | None = None
    context_prefix: str | None = None
    internal_reason: str | None = None
    public_targets: tuple[PublicTypeTarget, ...] = ()


TYPE_CONTEXT_RULES = (
    TypeContextRule(
        source="src/Base/Interpreter.cpp",
        context_name="PythonStdOutput",
        internal_reason="startup sys.stdout sink; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonConsolePy.cpp",
        context_name="PythonStdout",
        internal_reason="Python console sys.stdout replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonConsolePy.cpp",
        context_name="PythonStderr",
        internal_reason="Python console sys.stderr replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonConsolePy.cpp",
        context_name="OutputStdout",
        internal_reason="report view sys.stdout replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonConsolePy.cpp",
        context_name="OutputStderr",
        internal_reason="report view sys.stderr replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonConsolePy.cpp",
        context_name="PythonStdin",
        internal_reason="Python console sys.stdin replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonDebugger.cpp",
        context_name="PythonDebugStdout",
        internal_reason="debugger sys.stdout replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonDebugger.cpp",
        context_name="PythonDebugStderr",
        internal_reason="debugger sys.stderr replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Gui/PythonDebugger.cpp",
        context_name="PythonDebugExcept",
        internal_reason="debugger excepthook replacement; not exported from a FreeCAD module",
    ),
    TypeContextRule(
        source="src/Base/ParameterPy.cpp",
        context_name="ParameterGrp",
        public_targets=(PublicTypeTarget("FreeCAD", "_ParameterGrp"),),
    ),
    TypeContextRule(
        source="src/Gui/MainWindowPy.cpp",
        context_name="MainWindowPy",
        public_targets=(PublicTypeTarget("FreeCADGui", "_MainWindow"),),
    ),
    TypeContextRule(
        source="src/Gui/MDIViewPy.cpp",
        context_name="MDIViewPy",
        public_targets=(PublicTypeTarget("FreeCADGui", "_MDIView"),),
    ),
    TypeContextRule(
        source="src/Gui/SplitView3DInventor.cpp",
        context_name="AbstractSplitViewPy",
        public_targets=(PublicTypeTarget("FreeCADGui", "_AbstractSplitView"),),
    ),
    TypeContextRule(
        source="src/Gui/TaskView/TaskDialogPython.cpp",
        context_name="Control",
        public_targets=(PublicTypeTarget("FreeCADGui", "_Control", "Control"),),
    ),
    TypeContextRule(
        source="src/Gui/TaskView/TaskDialogPython.cpp",
        context_name="TaskDialog",
        public_targets=(PublicTypeTarget("FreeCADGui", "_TaskDialog"),),
    ),
    TypeContextRule(
        source="src/Gui/View3DPy.cpp",
        context_name="View3DInventorPy",
        public_targets=(PublicTypeTarget("FreeCADGui", "_View3DInventor"),),
    ),
    TypeContextRule(
        source="src/Gui/View3DViewerPy.cpp",
        context_name="View3DInventorViewerPy",
        public_targets=(PublicTypeTarget("FreeCADGui", "_View3DInventorViewer"),),
    ),
    TypeContextRule(
        source="src/Gui/WidgetFactory.cpp",
        context_name="PyResource",
        public_targets=(PublicTypeTarget("FreeCADGui", "_PyResource"),),
    ),
    TypeContextRule(
        source="src/Mod/Sandbox/App/DocumentProtectorPy.cpp",
        context_name="DocumentProtectorPy",
        public_targets=(PublicTypeTarget("Sandbox", "_DocumentProtector"),),
    ),
    TypeContextRule(
        source="src/Mod/Sandbox/App/DocumentProtectorPy.cpp",
        context_name="DocumentObjectProtectorPy",
        public_targets=(PublicTypeTarget("Sandbox", "_DocumentObjectProtector"),),
    ),
    TypeContextRule(
        source="src/Mod/Spreadsheet/Gui/SpreadsheetView.cpp",
        context_name="SheetViewPy",
        public_targets=(PublicTypeTarget("SpreadsheetGui", "_SheetView"),),
    ),
    TypeContextRule(
        source="src/Mod/TechDraw/Gui/MDIViewPage.cpp",
        context_name="MDIViewPagePy",
        public_targets=(PublicTypeTarget("TechDrawGui", "_MDIViewPage"),),
    ),
    TypeContextRule(
        source="src/Mod/Test/Gui/UnitTestPy.cpp",
        context_name="TestGui.UnitTest",
        public_targets=(PublicTypeTarget("QtUnitGui", "_UnitTest"),),
    ),
)


def _validate_rule(rule: TypeContextRule) -> None:
    has_exact = rule.context_name is not None
    has_prefix = rule.context_prefix is not None
    if has_exact == has_prefix:
        raise ValueError(f"invalid type context rule for {rule.source}: need exactly one matcher")
    if rule.internal_reason is not None and rule.public_targets:
        raise ValueError(
            f"invalid type context rule for {rule.source}: internal and public targets are mixed"
        )
    if rule.internal_reason is None and not rule.public_targets:
        raise ValueError(f"invalid type context rule for {rule.source}: no effect")


def _matches(rule: TypeContextRule, source: str, context_name: str) -> bool:
    if rule.source != source:
        return False
    if rule.context_name is not None:
        return rule.context_name == context_name
    if rule.context_prefix is not None:
        return context_name.startswith(rule.context_prefix)
    return False


def _resolved_targets(rule: TypeContextRule, context_name: str) -> list[PublicTypeTarget]:
    targets: list[PublicTypeTarget] = []
    for target in rule.public_targets:
        class_symbol = target.class_symbol.replace("{context}", context_name)
        variable_symbol = (
            None
            if target.variable_symbol is None
            else target.variable_symbol.replace("{context}", context_name)
        )
        base_symbols = tuple(
            base_symbol.replace("{context}", context_name) for base_symbol in target.base_symbols
        )
        targets.append(
            PublicTypeTarget(
                module_name=target.module_name,
                class_symbol=class_symbol,
                variable_symbol=variable_symbol,
                base_symbols=base_symbols,
            )
        )
    return targets


def matching_type_context_rules(source: str, context_name: str) -> list[TypeContextRule]:
    return [rule for rule in TYPE_CONTEXT_RULES if _matches(rule, source, context_name)]


def type_context_public_targets(source: str, context_name: str) -> list[PublicTypeTarget]:
    targets: list[PublicTypeTarget] = []
    for rule in matching_type_context_rules(source, context_name):
        targets.extend(_resolved_targets(rule, context_name))
    return targets


def type_context_internal_reason(source: str, context_name: str) -> str | None:
    reasons = [
        rule.internal_reason
        for rule in matching_type_context_rules(source, context_name)
        if rule.internal_reason is not None
    ]
    if not reasons:
        return None
    return "; ".join(dict.fromkeys(reasons))


for _rule in TYPE_CONTEXT_RULES:
    _validate_rule(_rule)
