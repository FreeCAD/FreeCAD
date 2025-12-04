# SPDX-License-Identifier: MIT
#
# Copyright (c) 2019 Daniel Furtlehner (furti)
# Copyright (c) 2025 The FreeCAD Project
#
# This file is a derivative work of the sql_parser.py file from the
# FreeCAD-Reporting workbench (https://github.com/furti/FreeCAD-Reporting).
# As per the terms of the original MIT license, this derivative work is also
# licensed under the MIT license.

"""Contains the SQL parsing and execution engine for BIM/Arch objects."""

import FreeCAD
import re
from collections import deque

if FreeCAD.GuiUp:
    # In GUI mode, import PySide and create real translation functions.
    from PySide import QtCore

    def translate(context, text, comment=None):
        """Wraps the Qt translation function."""
        return QtCore.QCoreApplication.translate(context, text, comment)

    # QT_TRANSLATE_NOOP is used to mark strings for the translation tool but
    # does not perform the translation at definition time.
    QT_TRANSLATE_NOOP = QtCore.QT_TRANSLATE_NOOP
else:
    # In headless mode, create dummy (no-op) functions that simply
    # return the original text. This ensures the code runs without a GUI.
    def translate(context, text, comment=None):
        return text

    def QT_TRANSLATE_NOOP(context, text):
        return text


# Import exception types from the generated parser for type-safe handling.
from generated_sql_parser import UnexpectedEOF, UnexpectedToken, VisitError
import generated_sql_parser

from typing import List, Tuple, Any, Optional

__all__ = [
    "select",
    "count",
    "selectObjects",
    "selectObjectsFromPipeline",
    "getSqlKeywords",
    "getSqlApiDocumentation",
    "BimSqlSyntaxError",
    "SqlEngineError",
    "ReportStatement",
]

# --- Custom Exceptions for the SQL Engine ---


class SqlEngineError(Exception):
    """Base class for all custom exceptions in this module."""

    pass


class BimSqlSyntaxError(SqlEngineError):
    """Raised for any parsing or syntax error."""

    def __init__(self, message, is_incomplete=False):
        super().__init__(message)
        self.is_incomplete = is_incomplete


# --- Debug Helpers for the SQL Engine ---


def debug_transformer_method(func):
    """A decorator to add verbose logging to transformer methods for debugging."""

    def wrapper(self, items):
        print(f"\n>>> ENTERING: {func.__name__}")
        print(f"    RECEIVED: {repr(items)}")
        result = func(self, items)
        print(f"    RETURNING: {repr(result)}")
        print(f"<<< EXITING:  {func.__name__}")
        return result

    return wrapper


# --- Module-level Constants ---

SELECT_STAR_HEADER = "Object Label"
_CUSTOM_FRIENDLY_TOKEN_NAMES = {
    # This dictionary provides overrides for tokens where the name is not user-friendly.
    # Punctuation
    "RPAR": "')'",
    "LPAR": "'('",
    "COMMA": "','",
    "ASTERISK": "'*'",
    "DOT": "'.'",
    "SEMICOLON": "';'",
    # Arithmetic Operators
    "ADD": "'+'",
    "SUB": "'-'",
    "MUL": "'*'",
    "DIV": "'/'",
    # Comparison Operators (from the grammar)
    "EQUAL": "'='",
    "MORETHAN": "'>'",
    "LESSTHAN": "'<'",
    # Other non-keyword tokens
    "CNAME": "a property or function name",
    "STRING": "a quoted string like 'text'",
}


# --- Internal Helper Functions ---


def _get_property(obj, prop_name):
    """Gets a property from a FreeCAD object, including sub-properties."""
    # The property name implies sub-property access (e.g., 'Placement.Base.x')
    is_nested_property = lambda prop_name: "." in prop_name

    if not is_nested_property(prop_name):
        # Handle simple, direct properties first, which is the most common case.
        if hasattr(obj, prop_name):
            return getattr(obj, prop_name)
        return None
    else:
        # Handle nested properties (e.g., Placement.Base.x)
        current_obj = obj
        parts = prop_name.split(".")
        for part in parts:
            if hasattr(current_obj, part):
                current_obj = getattr(current_obj, part)
            else:
                return None
        return current_obj


def _generate_friendly_token_names(parser):
    """Dynamically builds the friendly token name map from the Lark parser instance."""
    friendly_names = _CUSTOM_FRIENDLY_TOKEN_NAMES.copy()
    for term in parser.terminals:
        # Add any keyword/terminal from the grammar that isn't already in our custom map.
        if term.name not in friendly_names:
            # By default, the friendly name is the keyword itself in single quotes.
            friendly_names[term.name] = f"'{term.name}'"
    return friendly_names


def _map_results_to_objects(headers, data_rows):
    """
    Maps the raw data rows from a query result back to FreeCAD DocumentObjects.

    It uses a 'Name' or 'Label' column in the results to perform the lookup.

    Parameters
    ----------
    headers : list of str
        The list of column headers from the query result.
    data_rows : list of list
        The list of data rows from the query result.

    Returns
    -------
    list of FreeCAD.DocumentObject
        A list of unique FreeCAD DocumentObject instances that correspond to the
        query results. Returns an empty list if no identifiable column is
        found or if no objects match.
    """
    if not data_rows:
        return []

    # Build a lookup map for fast access to objects by their unique Name.
    objects_by_name = {o.Name: o for o in FreeCAD.ActiveDocument.Objects}
    objects_by_label = {}  # Lazy-loaded if needed

    # Find the index of the column that contains the object identifier.
    # Prefer 'Name' as it is guaranteed to be unique. Fallback to 'Label'.
    if "Name" in headers:
        id_idx = headers.index("Name")
        lookup_dict = objects_by_name
    elif "Label" in headers:
        id_idx = headers.index("Label")
        objects_by_label = {o.Label: o for o in FreeCAD.ActiveDocument.Objects}
        lookup_dict = objects_by_label
    elif SELECT_STAR_HEADER in headers:  # Handle 'SELECT *' case
        id_idx = headers.index(SELECT_STAR_HEADER)
        objects_by_label = {o.Label: o for o in FreeCAD.ActiveDocument.Objects}
        lookup_dict = objects_by_label
    else:
        # If no identifiable column, we cannot map back to objects.
        # This can happen for queries like "SELECT 1 + 1".
        return []

    # Map the identifiers from the query results back to the actual objects.
    found_objects = []
    for row in data_rows:
        identifier = row[id_idx]
        obj = lookup_dict.get(identifier)
        if obj and obj not in found_objects:  # Avoid duplicates
            found_objects.append(obj)

    return found_objects


def _is_generic_group(obj):
    """
    Checks if an object is a generic group that should be excluded from
    architectural query results.
    """
    # A generic group is a group that is not an architecturally significant one
    # like a BuildingPart (which covers Floors, Buildings, etc.).
    return obj.isDerivedFrom("App::DocumentObjectGroup")


def _get_bim_type(obj):
    """
    Gets the most architecturally significant type for a FreeCAD object.

    This is a specialized utility for BIM reporting. It prioritizes explicit
    BIM properties (.IfcType) to correctly distinguish between different Arch
    objects that may share the same proxy (e.g., Doors and Windows).

    Parameters
    ----------
    obj : App::DocumentObject
        The object to inspect.

    Returns
    -------
    str
        The determined type string (e.g., 'Door', 'Building Storey', 'Wall').
    """
    if not obj:
        return None

    # 1. Prioritize the explicit IfcType for architectural objects.
    #    This correctly handles Door vs. Window and returns the raw value.
    if hasattr(obj, "IfcType"):
        if obj.IfcType and obj.IfcType != "Undefined":
            return obj.IfcType

    # 2. Check for legacy .Class property from old IFC imports.
    if hasattr(obj, "Class") and "Ifc" in str(obj.Class):
        return obj.Class

    # 3. Fallback to Proxy.Type for other scripted objects.
    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type

    # 4. Final fallback to the object's internal TypeId.
    if hasattr(obj, "TypeId"):
        return obj.TypeId

    return "Unknown"


def _is_bim_group(obj):
    """
    Checks if an object is a group-like container in a BIM context.

    Parameters
    ----------
    obj : App::DocumentObject
        The object to check.

    Returns
    -------
    bool
        True if the object is considered a BIM group.
    """
    bim_type = _get_bim_type(obj)
    # Note: 'Floor' and 'Building' are obsolete but kept for compatibility.
    return (
        obj.isDerivedFrom("App::DocumentObjectGroup") and bim_type != "LayerContainer"
    ) or bim_type in (
        "Project",
        "Site",
        "Building",
        "Building Storey",
        "Floor",
        "Building Element Part",
        "Space",
    )


def _get_direct_children(obj, discover_hosted_elements, include_components_from_additions):
    """
    Finds the immediate descendants of a single object.

    Encapsulates the different ways an object can be a "child" in FreeCAD's BIM context, checking
    for hierarchical containment (.Group), architectural hosting (.Hosts/.Host), and geometric
    composition (.Additions).

    Parameters
    ----------
    obj : App::DocumentObject
        The parent object to find the children of.

    discover_hosted_elements : bool
        If True, the function will perform checks to find objects that are architecturally hosted by
        `obj` (e.g., a Window in a Wall).

    include_components_from_additions : bool
        If True, the function will include objects found in the `obj.Additions` list, which are
        typically used for geometric composition.

    Returns
    -------
    list of App::DocumentObject
        A list of the direct child objects of `obj`.
    """
    children = []

    # 1. Hierarchical children from .Group (containment)
    if _is_bim_group(obj) and hasattr(obj, "Group") and obj.Group:
        children.extend(obj.Group)

    # 2. Architecturally-hosted elements
    if discover_hosted_elements:
        host_types = ["Wall", "Structure", "CurtainWall", "Precast", "Panel", "Roof"]
        if _get_bim_type(obj) in host_types:
            for item_in_inlist in obj.InList:
                element_to_check = item_in_inlist
                if hasattr(item_in_inlist, "getLinkedObject"):
                    linked = item_in_inlist.getLinkedObject()
                    if linked:
                        element_to_check = linked

                element_type = _get_bim_type(element_to_check)
                is_confirmed_hosted = False
                if element_type == "Window":
                    if hasattr(element_to_check, "Hosts") and obj in element_to_check.Hosts:
                        is_confirmed_hosted = True
                elif element_type == "Rebar":
                    if hasattr(element_to_check, "Host") and obj == element_to_check.Host:
                        is_confirmed_hosted = True

                if is_confirmed_hosted:
                    children.append(element_to_check)

    # 3. Geometric components from .Additions list
    if include_components_from_additions and hasattr(obj, "Additions") and obj.Additions:
        for addition_comp in obj.Additions:
            actual_addition = addition_comp
            if hasattr(addition_comp, "getLinkedObject"):
                linked_add = addition_comp.getLinkedObject()
                if linked_add:
                    actual_addition = linked_add
            children.append(actual_addition)

    return children


# TODO: Refactor architectural traversal logic.
# This function is a temporary, enhanced copy of the traversal algorithm
# found in ArchCommands.get_architectural_contents. It was duplicated here
# to avoid creating a circular dependency and to keep the BIM Report PR
# self-contained.
#
# A future refactoring task should:
# 1. Move this enhanced implementation into a new, low-level core utility
#    module (e.g., ArchCoreUtils.py).
# 2. Add a comprehensive unit test suite for this new core function.
# 3. Refactor this implementation and the original get_architectural_contents
#    to be simple wrappers around the new, centralized core function.
# This will remove the code duplication and improve the overall architecture.
def _traverse_architectural_hierarchy(
    initial_objects,
    max_depth=0,
    discover_hosted_elements=True,
    include_components_from_additions=False,
    include_groups_in_result=True,
    include_initial_objects_in_result=True,
):
    """
    Traverses the BIM hierarchy to find all descendants of a given set of objects.

    This function implements a Breadth-First Search (BFS) algorithm using a
    queue to safely and efficiently traverse the model. It is the core engine
    used by the CHILDREN and CHILDREN_RECURSIVE SQL functions.

    Parameters
    ----------
    initial_objects : list of App::DocumentObject
        The starting object(s) for the traversal.

    max_depth : int, optional
        The maximum number of architecturally significant levels to traverse.
        A value of 0 (default) means the traversal is unlimited. A value of 1
        will find direct children only. Generic organizational groups do not
        count towards the depth limit.

    discover_hosted_elements : bool, optional
        If True (default), the traversal will find objects that are
        architecturally hosted (e.g., Windows in Walls).

    include_components_from_additions : bool, optional
        If True, the traversal will include objects from `.Additions` lists.
        Defaults to False, as these are typically geometric components, not
        separate architectural elements.

    include_groups_in_result : bool, optional
        If True (default), generic organizational groups (App::DocumentObjectGroup)
        will be included in the final output. If False, they are traversed
        transparently but excluded from the results.

    include_initial_objects_in_result : bool, optional
        If True (default), the objects in `initial_objects` will themselves
        be included in the returned list.

    Returns
    -------
    list of App::DocumentObject
        A flat, unique list of all discovered descendant objects.
    """
    final_contents_list = []
    queue = deque()
    processed_or_queued_names = set()

    if not isinstance(initial_objects, list):
        initial_objects_list = [initial_objects]
    else:
        initial_objects_list = list(initial_objects)

    for obj in initial_objects_list:
        queue.append((obj, 0))
        processed_or_queued_names.add(obj.Name)

    while queue:
        obj, current_depth = queue.popleft()

        is_initial = obj in initial_objects_list
        if (is_initial and include_initial_objects_in_result) or not is_initial:
            if obj not in final_contents_list:
                final_contents_list.append(obj)

        if max_depth != 0 and current_depth >= max_depth:
            continue

        direct_children = _get_direct_children(
            obj, discover_hosted_elements, include_components_from_additions
        )

        for child in direct_children:
            if child.Name not in processed_or_queued_names:
                if _is_generic_group(child):
                    next_depth = current_depth
                else:
                    next_depth = current_depth + 1

                queue.append((child, next_depth))
                processed_or_queued_names.add(child.Name)

    if not include_groups_in_result:
        filtered_list = [obj for obj in final_contents_list if not _is_generic_group(obj)]
        return filtered_list

    return final_contents_list


def _execute_pipeline_for_objects(statements: List["ReportStatement"]) -> List:
    """
    Internal helper to run a pipeline and get the final list of objects.

    Unlike the main generator, this function consumes the entire pipeline and
    returns only the final list of resulting objects, for use in validation.
    """
    pipeline_input_objects = None

    for i, statement in enumerate(statements):
        if not statement.query_string or not statement.query_string.strip():
            pipeline_input_objects = [] if statement.is_pipelined else None
            continue

        source = pipeline_input_objects if statement.is_pipelined else None

        try:
            _, _, resulting_objects = _run_query(
                statement.query_string, mode="full_data", source_objects=source
            )
            pipeline_input_objects = resulting_objects
        except (SqlEngineError, BimSqlSyntaxError):
            # If any step fails, the final output is an empty list of objects.
            return []

    return pipeline_input_objects or []


# --- Logical Classes for the SQL Statement Object Model ---


class FunctionRegistry:
    """A simple class to manage the registration of SQL functions."""

    def __init__(self):
        self._functions = {}

    def register(self, name, function_class, category, signature, description, snippet=""):
        """Registers a class to handle a function with the given name."""
        self._functions[name.upper()] = {
            "class": function_class,
            "category": category,
            "signature": signature,
            "description": description,
            "snippet": snippet,
        }

    def get_class(self, name):
        """Retrieves the class registered for a given function name."""
        data = self._functions.get(name.upper())
        return data["class"] if data else None


# Create global, module-level registries that will be populated by decorators.
select_function_registry = FunctionRegistry()
from_function_registry = FunctionRegistry()


def register_select_function(name, category, signature, description, snippet=""):
    """
    A decorator that registers a class as a selectable SQL function.
    The decorated class must be a subclass of FunctionBase or similar.
    """

    def wrapper(cls):
        select_function_registry.register(name, cls, category, signature, description, snippet)
        return cls

    return wrapper


def register_from_function(name, category, signature, description, snippet=""):
    """
    A decorator that registers a class as a FROM clause SQL function.
    The decorated class must be a subclass of FromFunctionBase.
    """

    def wrapper(cls):
        from_function_registry.register(name, cls, category, signature, description, snippet)
        return cls

    return wrapper


class AggregateFunction:
    """Represents an aggregate function call like COUNT(*) or SUM(Height)."""

    def __init__(self, name, arg_extractors):
        self.function_name = name.lower()
        self.arg_extractors = arg_extractors

        if len(self.arg_extractors) != 1:
            raise ValueError(
                f"Aggregate function {self.function_name.upper()} requires exactly one argument."
            )

        self.argument = self.arg_extractors[0]

    def get_value(self, obj):
        # This method should never be called directly in a row-by-row context like a WHERE clause.
        # Aggregates are handled in a separate path (_execute_grouped_query or
        # the single-row path in _execute_non_grouped_query). Calling it here is a semantic error.
        raise SqlEngineError(
            f"Aggregate function '{self.function_name.upper()}' cannot be used in this context."
        )


@register_select_function(
    name="COUNT",
    category=QT_TRANSLATE_NOOP("ArchSql", "Aggregate"),
    signature="COUNT(* | property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Counts rows that match criteria."),
    snippet="SELECT COUNT(*) FROM document WHERE IfcType = 'Space'",
)
class CountFunction(AggregateFunction):
    """Implements the COUNT() aggregate function."""

    pass


@register_select_function(
    name="SUM",
    category=QT_TRANSLATE_NOOP("ArchSql", "Aggregate"),
    signature="SUM(property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Calculates the sum of a numerical property."),
    snippet="SELECT SUM(Area) FROM document WHERE IfcType = 'Space'",
)
class SumFunction(AggregateFunction):
    """Implements the SUM() aggregate function."""

    pass


@register_select_function(
    name="MIN",
    category=QT_TRANSLATE_NOOP("ArchSql", "Aggregate"),
    signature="MIN(property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Finds the minimum value of a property."),
    snippet="SELECT MIN(Length) FROM document WHERE IfcType = 'Wall'",
)
class MinFunction(AggregateFunction):
    """Implements the MIN() aggregate function."""

    pass


@register_select_function(
    name="MAX",
    category=QT_TRANSLATE_NOOP("ArchSql", "Aggregate"),
    signature="MAX(property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Finds the maximum value of a property."),
    snippet="SELECT MAX(Height) FROM document WHERE IfcType = 'Wall'",
)
class MaxFunction(AggregateFunction):
    """Implements the MAX() aggregate function."""

    pass


class FunctionBase:
    """A base class for non-aggregate functions like TYPE, CONCAT, etc."""

    def __init__(self, function_name, arg_extractors):
        self.function_name = function_name
        self.arg_extractors = arg_extractors
        # The 'base' is set by the transformer during parsing of a chain.
        self.base = None

    def get_value(self, obj):
        """
        Calculates the function's value. This is the entry point.
        It determines the object to operate on (from the chain, or the row object)
        and then calls the specific implementation.
        """
        if self.base:
            on_object = self.base.get_value(obj)
            if on_object is None:
                return None
        else:
            on_object = obj

        return self._execute_function(on_object, obj)

    def _execute_function(self, on_object, original_obj):
        """
        Child classes must implement this.
        - on_object: The object the function should run on (from the chain).
        - original_obj: The original row object, used to evaluate arguments.
        """
        raise NotImplementedError()


@register_select_function(
    name="TYPE",
    category=QT_TRANSLATE_NOOP("ArchSql", "Utility"),
    signature="TYPE(*)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Returns the object's BIM type (e.g., 'Wall')."),
    snippet="SELECT Label FROM document WHERE TYPE(*) = 'Wall'",
)
class TypeFunction(FunctionBase):
    """Implements the TYPE() function."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1 or self.arg_extractors[0] != "*":
            raise ValueError(f"Function {self.function_name} requires exactly one argument: '*'")

    def get_value(self, obj):
        # The argument for TYPE is the object itself, represented by '*'.
        return _get_bim_type(obj)


@register_select_function(
    name="LOWER",
    category=QT_TRANSLATE_NOOP("ArchSql", "String"),
    signature="LOWER(property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Converts text to lowercase."),
    snippet="SELECT Label FROM document WHERE LOWER(Label) = 'exterior wall'",
)
class LowerFunction(FunctionBase):
    """Implements the LOWER() string function."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1:
            raise ValueError(f"Function {self.function_name} requires exactly one argument.")

    def get_value(self, obj):
        value = self.arg_extractors[0].get_value(obj)
        return str(value).lower() if value is not None else None


@register_select_function(
    name="UPPER",
    category=QT_TRANSLATE_NOOP("ArchSql", "String"),
    signature="UPPER(property)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Converts text to uppercase."),
    snippet="SELECT Label FROM document WHERE UPPER(IfcType) = 'WALL'",
)
class UpperFunction(FunctionBase):
    """Implements the UPPER() string function."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1:
            raise ValueError(f"Function {self.function_name} requires exactly one argument.")

    def get_value(self, obj):
        value = self.arg_extractors[0].get_value(obj)
        return str(value).upper() if value is not None else None


@register_select_function(
    name="CONCAT",
    category=QT_TRANSLATE_NOOP("ArchSql", "String"),
    signature="CONCAT(value1, value2, ...)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Joins multiple strings and properties together."),
    snippet="SELECT CONCAT(Label, ': ', IfcType) FROM document",
)
class ConcatFunction(FunctionBase):
    """Implements the CONCAT() string function."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if not self.arg_extractors:
            raise ValueError(f"Function {self.function_name} requires at least one argument.")

    def get_value(self, obj):
        parts = [
            str(ex.get_value(obj)) if ex.get_value(obj) is not None else ""
            for ex in self.arg_extractors
        ]
        return "".join(parts)


@register_select_function(
    name="CONVERT",
    category=QT_TRANSLATE_NOOP("ArchSql", "Utility"),
    signature="CONVERT(quantity, 'unit')",
    description=QT_TRANSLATE_NOOP(
        "ArchSql", "Converts a Quantity to a different unit (e.g., CONVERT(Length, 'm'))."
    ),
    snippet="SELECT CONVERT(Length, 'm') AS LengthInMeters FROM document",
)
class ConvertFunction(FunctionBase):
    """Implements the CONVERT(Quantity, 'unit') function."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 2:
            raise ValueError(
                f"Function {self.function_name} requires exactly two arguments: a property and a unit string."
            )

    def get_value(self, obj):
        # Evaluate the arguments to get the input value and target unit string.
        input_value = self.arg_extractors[0].get_value(obj)
        unit_string = self.arg_extractors[1].get_value(obj)

        # The first argument must be a Quantity object to be convertible.
        if not isinstance(input_value, FreeCAD.Units.Quantity):
            raise SqlEngineError(
                f"CONVERT function requires a Quantity object as the first argument, but got {type(input_value).__name__}."
            )

        try:
            # Use the underlying API to perform the conversion.
            result_quantity = input_value.getValueAs(str(unit_string))
            return result_quantity.Value
        except Exception as e:
            # The API will raise an error for incompatible units (e.g., mm to kg).
            raise SqlEngineError(f"Unit conversion failed: {e}")


class FromFunctionBase:
    """Base class for all functions used in a FROM clause."""

    def __init__(self, args):
        # args will be the SelectStatement to be executed
        self.args = args

    def get_objects(self, source_objects=None):
        """Executes the subquery and returns the final list of objects."""
        raise NotImplementedError()

    def _get_parent_objects(self, source_objects=None):
        """
        Helper to execute the subquery and resolve the resulting rows back
        into a list of FreeCAD document objects.
        """
        if source_objects is not None:
            # If source_objects are provided by the pipeline, use them directly as the parents.
            return source_objects

        # Only execute the substatement if no source_objects are provided.
        headers, rows = self.args.execute(FreeCAD.ActiveDocument.Objects)

        if not rows:
            return []

        # Determine which column to use for mapping back to objects
        label_idx = headers.index("Label") if "Label" in headers else -1
        name_idx = headers.index("Name") if "Name" in headers else -1
        # Handle the special header name from a 'SELECT *' query
        if headers == [SELECT_STAR_HEADER]:
            label_idx = 0

        if label_idx == -1 and name_idx == -1:
            raise ValueError(
                "Subquery for FROM function must return an object identifier column: "
                "'Name' or 'Label' (or use SELECT *)."
            )

        # Build lookup maps once for efficient searching
        objects_by_name = {o.Name: o for o in FreeCAD.ActiveDocument.Objects}
        objects_by_label = {o.Label: o for o in FreeCAD.ActiveDocument.Objects}

        parent_objects = []
        for row in rows:
            parent = None
            # Prioritize matching by unique Name first
            if name_idx != -1:
                parent = objects_by_name.get(row[name_idx])
            # Fallback to user-facing Label if no match by Name
            if not parent and label_idx != -1:
                parent = objects_by_label.get(row[label_idx])

            if parent:
                parent_objects.append(parent)
        return parent_objects


@register_from_function(
    name="CHILDREN",
    category=QT_TRANSLATE_NOOP("ArchSql", "Hierarchical"),
    signature="CHILDREN(subquery)",
    description=QT_TRANSLATE_NOOP("ArchSql", "Selects direct child objects of a given parent set."),
    snippet="SELECT * FROM CHILDREN(SELECT * FROM document WHERE Label = 'My Floor')",
)
class ChildrenFromFunction(FromFunctionBase):
    """Implements the CHILDREN() function."""

    def get_objects(self, source_objects=None):
        recursive_handler = ChildrenRecursiveFromFunction(self.args)

        # Get the root objects to start from.
        subquery_statement = self.args[0]
        parent_objects = recursive_handler._get_parent_objects_from_subquery(
            subquery_statement, source_objects
        )
        if not parent_objects:
            return []

        # Call the core traversal function with a hard-coded max_depth of 1.
        return _traverse_architectural_hierarchy(
            initial_objects=parent_objects,
            max_depth=1,
            include_groups_in_result=False,
            include_initial_objects_in_result=False,  # Only return children
        )


@register_from_function(
    name="CHILDREN_RECURSIVE",
    category=QT_TRANSLATE_NOOP("ArchSql", "Hierarchical"),
    signature="CHILDREN_RECURSIVE(subquery, max_depth=15)",
    description=QT_TRANSLATE_NOOP(
        "ArchSql", "Selects all descendant objects of a given set, traversing the full hierarchy."
    ),
    snippet="SELECT * FROM CHILDREN_RECURSIVE(SELECT * FROM document WHERE Label = 'My Building')",
)
class ChildrenRecursiveFromFunction(FromFunctionBase):
    """Implements the CHILDREN_RECURSIVE() function."""

    def get_objects(self, source_objects=None):
        # The subquery is always the first argument.
        subquery_statement = self.args[0]
        max_depth = 15  # Default safe depth limit

        # The optional max_depth is the second argument. It will be a StaticExtractor.
        if len(self.args) > 1 and isinstance(self.args[1], StaticExtractor):
            # We get its raw value, which should be a float from the NUMBER terminal.
            max_depth = int(self.args[1].get_value(None))

        # Get the root objects to start from.
        # The subquery runs on the pipeline source if provided.
        parent_objects = self._get_parent_objects_from_subquery(subquery_statement, source_objects)
        if not parent_objects:
            return []

        # Call our fully-tested core traversal function with the correct parameters.
        return _traverse_architectural_hierarchy(
            initial_objects=parent_objects,
            max_depth=max_depth,
            include_groups_in_result=False,
            include_initial_objects_in_result=False,  # Critical: Only return children
        )

    def _get_parent_objects_from_subquery(self, substatement, source_objects=None):
        """Helper to execute a subquery statement and return its objects."""
        # This is a simplified version of the old _get_parent_objects method.
        # It executes the substatement and maps the results back to objects.
        if source_objects:
            # If a pipeline provides the source, the subquery runs on that source.
            headers, rows = substatement.execute(source_objects)
        else:
            headers, rows = substatement.execute(FreeCAD.ActiveDocument.Objects)

        return _map_results_to_objects(headers, rows)


@register_select_function(
    name="PARENT",
    category=QT_TRANSLATE_NOOP("ArchSql", "Hierarchical"),
    signature="PARENT(*)",
    description=QT_TRANSLATE_NOOP(
        "ArchSql", "Returns the immediate, architecturally significant parent of an object."
    ),
    snippet="SELECT Label, PARENT(*).Label AS Floor FROM document WHERE IfcType = 'Space'",
)
class ParentFunction(FunctionBase):
    """Implements the PARENT(*) function to find an object's container."""

    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1 or self.arg_extractors[0] != "*":
            raise ValueError(f"Function {self.function_name} requires exactly one argument: '*'")

    def _execute_function(self, on_object, original_obj):
        """
        Walks up the document tree from the given on_object to find the first
        architecturally significant parent, transparently skipping generic groups.
        """
        current_obj = on_object

        # Limit search depth to 20 levels to prevent infinite loops.
        for _ in range(20):
            # --- Step 1: Find the immediate parent of current_obj ---
            immediate_parent = None

            # Priority 1: Check for a host (for Windows, Doors, etc.)
            if hasattr(current_obj, "Hosts") and current_obj.Hosts:
                immediate_parent = current_obj.Hosts[0]

            # Priority 2: If no host, search InList for a true container.
            # A true container is an object that has the current object in its .Group list.
            elif hasattr(current_obj, "InList") and current_obj.InList:
                for obj_in_list in current_obj.InList:
                    if hasattr(obj_in_list, "Group") and current_obj in obj_in_list.Group:
                        immediate_parent = obj_in_list
                        break

            if not immediate_parent:
                return None  # No parent found, top of branch.

            # Check if the found parent is a generic group to be skipped.
            if _is_generic_group(immediate_parent):
                # The parent is a generic group. Skip it and continue the search
                # from this parent's level in the next loop.
                current_obj = immediate_parent
            else:
                # The parent is architecturally significant. This is our answer.
                return immediate_parent

        return None  # Search limit reached.


class GroupByClause:
    """Represents the GROUP BY clause of a SQL statement."""

    def __init__(self, columns):
        # columns is a list of ReferenceExtractor objects
        self.columns = columns


class OrderByClause:
    """Represents the ORDER BY clause of a SQL statement."""

    def __init__(self, column_references, direction="ASC"):
        # Store the string names of the columns to sort by, which can be properties or aliases.
        self.column_names = [ref.value for ref in column_references]
        self.direction = direction.upper()

    def __repr__(self):
        # Add a __repr__ for clearer debug logs.
        return f"<OrderByClause columns='{self.column_names}' dir='{self.direction}'>"


class SelectStatement:
    def __init__(self, columns_info, from_clause, where_clause, group_by_clause, order_by_clause):
        self.columns_info = columns_info  # Stores (extractor_object, display_name) tuples
        self.from_clause = from_clause
        self.where_clause = where_clause
        self.group_by_clause = group_by_clause
        self.order_by_clause = order_by_clause

    def execute(self, all_objects):
        # 1. Phase 1: Get filtered and grouped object data.
        grouped_data = self._get_grouped_data(all_objects)
        # 2. Determine the column headers from the parsed statement
        headers = [display_name for _, display_name in self.columns_info]

        # 3. Phase 2: Process the SELECT columns to get the final data rows.
        results_data = self._process_select_columns(grouped_data, all_objects)

        # 4. Perform final sorting if an ORDER BY clause was provided.
        if self.order_by_clause:
            # Sorting logic: it finds the index of the ORDER BY column name/alias in the final
            # headers list and sorts the existing results_data based on that index.
            sort_column_indices = []
            for sort_column_name in self.order_by_clause.column_names:
                try:
                    # Find the 0-based index of the column to sort by from the
                    # final headers. This works for properties and aliases.
                    idx = headers.index(sort_column_name)
                    sort_column_indices.append(idx)
                except ValueError:
                    raise ValueError(
                        f"ORDER BY column '{sort_column_name}' is not in the SELECT list."
                    )

            is_descending = self.order_by_clause.direction == "DESC"

            # Define a sort key that can handle different data types.
            def sort_key(row):
                """
                Returns a tuple of sortable keys for a given row, one for each
                column specified in the ORDER BY clause.
                """
                keys = []
                for index in sort_column_indices:
                    value = row[index]
                    # Create a consistent, comparable key for each value.
                    if value is None:
                        keys.append((0, None))  # Nones sort first.
                    elif isinstance(value, (int, float, FreeCAD.Units.Quantity)):
                        num_val = (
                            value.Value if isinstance(value, FreeCAD.Units.Quantity) else value
                        )
                        keys.append((1, num_val))  # Numbers sort second.
                    else:
                        keys.append((2, str(value)))  # Everything else sorts as a string.
                return tuple(keys)

            results_data.sort(key=sort_key, reverse=is_descending)

        return headers, results_data

    def _get_extractor_signature(self, extractor):
        """Generates a unique, hashable signature for any extractor object."""
        if isinstance(extractor, ReferenceExtractor):
            return extractor.value
        elif isinstance(extractor, StaticExtractor):
            return f"'{extractor.get_value(None)}'"
        elif isinstance(extractor, FunctionBase):
            # Recursively build a signature for functions, e.g., "LOWER(Label)"
            arg_sigs = []
            for arg_ex in extractor.arg_extractors:
                if arg_ex == "*":
                    arg_sigs.append("*")
                else:
                    # This recursive call handles nested functions correctly
                    arg_sigs.append(self._get_extractor_signature(arg_ex))
            return f"{extractor.function_name.upper()}({', '.join(arg_sigs)})"
        # Return a non-string type for unsupported extractors to prevent accidental matches
        return None

    def validate(self):
        """
        Validates the select statement against SQL rules, such as those for GROUP BY.
        Raises ValueError on failure with a user-friendly message.
        """
        if self.group_by_clause:
            # Rule: Every column in the SELECT list must either be an aggregate function,
            # a static value, or be part of the GROUP BY clause.
            group_by_signatures = {
                self._get_extractor_signature(ex) for ex in self.group_by_clause.columns
            }

            for extractor, _ in self.columns_info:
                # This check is for columns that are inherently valid (aggregates, static values).
                # A regular function (FunctionBase) is NOT inherently valid, so it must be checked below.
                if isinstance(extractor, (AggregateFunction, StaticExtractor)):
                    continue

                if extractor == "*":
                    raise ValueError("Cannot use '*' in a SELECT statement with a GROUP BY clause.")

                # This is the main check. It generates a signature for the current SELECT column
                # (which could be a property OR a function) and ensures it exists in the GROUP BY clause.
                select_col_signature = self._get_extractor_signature(extractor)
                if select_col_signature not in group_by_signatures:
                    raise ValueError(
                        f"Column '{select_col_signature}' must appear in the GROUP BY clause "
                        "or be used in an aggregate function."
                    )
            return

        # Rule: If there is no GROUP BY, you cannot mix aggregate and non-aggregate columns.
        has_aggregate = any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info)
        # A non-aggregate is a ReferenceExtractor or a scalar function (FunctionBase).
        # StaticExtractors are always allowed.
        has_non_aggregate = any(
            isinstance(ex, (ReferenceExtractor, FunctionBase)) for ex, _ in self.columns_info
        )

        if has_aggregate and has_non_aggregate:
            raise ValueError(
                "Cannot mix aggregate functions (like COUNT) and other columns or functions (like Label or LOWER) "
                "without a GROUP BY clause."
            )

    def _get_grouping_key(self, obj, group_by_extractors):
        """Generates a tuple key for an object based on the GROUP BY columns."""
        key_parts = []
        for extractor in group_by_extractors:
            value = extractor.get_value(obj)
            # We must ensure the key part is hashable. Converting to string is a
            # safe fallback for unhashable types like lists, while preserving
            # the original value for common hashable types (str, int, None, etc.).
            if value is not None and not isinstance(value, (str, int, float, bool, tuple)):
                key_parts.append(str(value))
            else:
                key_parts.append(value)
        return tuple(key_parts)

    def _execute_grouped_query(self, objects):
        """Executes a query that contains a GROUP BY clause."""
        results_data = []
        groups = {}  # A dictionary to partition objects by their group key

        group_by_extractors = self.group_by_clause.columns

        # 1. Partition all filtered objects into groups
        for obj in objects:
            key = self._get_grouping_key(obj, group_by_extractors)
            if key not in groups:
                groups[key] = []
            groups[key].append(obj)

        # 2. Process each group to generate one summary row
        for key, object_list in groups.items():
            row = []
            for extractor, _ in self.columns_info:
                value = None
                if isinstance(extractor, AggregateFunction):
                    if extractor.function_name == "count":
                        # Distinguish between COUNT(*) and COUNT(property)
                        if extractor.argument == "*":
                            value = len(object_list)
                        else:
                            # Count only objects where the specified property is not None
                            prop_name = extractor.argument.value
                            count = sum(
                                1
                                for obj in object_list
                                if _get_property(obj, prop_name) is not None
                            )
                            value = count
                    else:
                        # For other aggregates, extract the relevant property from all objects in the group
                        arg_extractor = extractor.argument
                        values = []
                        for obj in object_list:
                            prop_val = arg_extractor.get_value(obj)
                            # Ensure we only aggregate numeric, non-null values
                            if prop_val is not None:
                                # Handle FreeCAD.Quantity by using its value
                                if isinstance(prop_val, FreeCAD.Units.Quantity):
                                    prop_val = prop_val.Value
                                if isinstance(prop_val, (int, float)):
                                    values.append(prop_val)

                        if not values:
                            value = None  # Return None if no valid numeric values were found
                        elif extractor.function_name == "sum":
                            value = sum(values)
                        elif extractor.function_name == "min":
                            value = min(values)
                        elif extractor.function_name == "max":
                            value = max(values)
                        else:
                            value = f"'{extractor.function_name}' NOT_IMPL"

                elif isinstance(extractor, FunctionBase):
                    # For non-aggregate functions, just calculate the value based on the first object.
                    # This is consistent with how non-grouped, non-aggregate columns are handled.
                    if object_list:
                        value = extractor.get_value(object_list[0])

                else:
                    # This must be a column from the GROUP BY clause. We find which part
                    # of the key corresponds to this column.
                    key_index = -1
                    if isinstance(extractor, ReferenceExtractor):
                        for i, gb_extractor in enumerate(group_by_extractors):
                            if gb_extractor.value == extractor.value:
                                key_index = i
                                break
                    if key_index != -1:
                        value = key[key_index]

                row.append(value)
            results_data.append(row)

        return results_data

    def _execute_non_grouped_query(self, objects):
        """Executes a simple query without a GROUP BY clause."""
        results_data = []

        # Check if this is a query with only aggregate or non-aggregate functions
        is_single_row_query = any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info)
        if is_single_row_query:
            # A query with functions but no GROUP BY always returns a single row.
            row = []
            for extractor, _ in self.columns_info:
                value = None

                if isinstance(extractor, StaticExtractor):
                    value = extractor.get_value(None)
                elif isinstance(extractor, AggregateFunction):
                    if extractor.function_name == "count":
                        if extractor.argument == "*":
                            value = len(objects)
                        else:
                            # Count only objects where the specified property is not None
                            prop_name = extractor.argument.value
                            count = sum(
                                1 for obj in objects if _get_property(obj, prop_name) is not None
                            )
                            value = count
                    else:
                        # For other aggregates, they must have a property to act on.
                        if isinstance(extractor.argument, ReferenceExtractor):
                            arg_extractor = extractor.argument
                            values = []
                            for obj in objects:
                                prop_val = arg_extractor.get_value(obj)
                                # Ensure we only aggregate numeric, non-null values
                                if prop_val is not None:
                                    if isinstance(prop_val, FreeCAD.Units.Quantity):
                                        prop_val = prop_val.Value
                                    if isinstance(prop_val, (int, float)):
                                        values.append(prop_val)

                            if not values:
                                value = None
                            elif extractor.function_name == "sum":
                                value = sum(values)
                            elif extractor.function_name == "min":
                                value = min(values)
                            elif extractor.function_name == "max":
                                value = max(values)
                            else:
                                value = f"'{extractor.function_name}' NOT_IMPL"
                elif isinstance(extractor, FunctionBase):
                    # For non-aggregate functions, calculate based on the first object if available.
                    if objects:
                        value = extractor.get_value(objects[0])
                else:
                    # This case (a ReferenceExtractor) is correctly blocked by the
                    # validate() method and should not be reached.
                    value = "INVALID_MIX"

                row.append(value)
            results_data.append(row)
        else:
            # This is a standard row-by-row query.
            for obj in objects:
                row = []
                for extractor, _ in self.columns_info:
                    if extractor == "*":
                        value = obj.Label if hasattr(obj, "Label") else getattr(obj, "Name", "")
                    else:
                        value = extractor.get_value(obj)

                    # Append the raw value; formatting is the writer's responsibility
                    row.append(value)
                results_data.append(row)

        return results_data

    def get_row_count(self, all_objects):
        """
        Calculates only the number of rows the query will produce, performing
        the minimal amount of work necessary. This is used by Arch.count()
        for a fast UI preview.
        """
        grouped_data = self._get_grouped_data(all_objects)
        return len(grouped_data)

    def _get_grouped_data(self, all_objects):
        """
        Performs Phase 1 of execution: FROM, WHERE, and GROUP BY.
        This is the fast part of the query that only deals with object lists.
        Returns a list of "groups", where each group is a list of objects.
        """
        filtered_objects = [
            o for o in all_objects if self.where_clause is None or self.where_clause.matches(o)
        ]

        if not self.group_by_clause:
            # If no GROUP BY, every object is its own group.
            # Return as a list of single-item lists to maintain a consistent data structure.
            return [[obj] for obj in filtered_objects]
        else:
            # If GROUP BY is present, partition the objects.
            groups = {}
            group_by_extractors = self.group_by_clause.columns
            for obj in filtered_objects:
                key = self._get_grouping_key(obj, group_by_extractors)
                if key not in groups:
                    groups[key] = []
                groups[key].append(obj)
            return list(groups.values())

    def _process_select_columns(self, grouped_data, all_objects_for_context):
        """
        Performs Phase 2 of execution: processes the SELECT columns.
        This is the slow part of the query that does data extraction,
        function calls, and aggregation.
        """
        results_data = []

        # Handle SELECT * as a special case for non-grouped queries
        if not self.group_by_clause and self.columns_info and self.columns_info[0][0] == "*":
            for group in grouped_data:
                obj = group[0]
                value = obj.Label if hasattr(obj, "Label") else getattr(obj, "Name", "")
                results_data.append([value])
            return results_data

        is_single_row_aggregate = (
            any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info)
            and not self.group_by_clause
        )
        if is_single_row_aggregate:
            # A query with aggregates but no GROUP BY always returns one summary row
            # based on all objects that passed the filter.

            all_filtered_objects = [obj for group in grouped_data for obj in group]
            row = self._calculate_row_values(all_filtered_objects)

            return [row]

        # Standard processing: one output row for each group.
        for group in grouped_data:
            row = self._calculate_row_values(group)
            results_data.append(row)

        return results_data

    def _calculate_row_values(self, object_list):
        """
        Helper that calculates all SELECT column values for a given list of objects
        (which can be a "group" or all filtered objects).
        """
        row = []
        for extractor, _ in self.columns_info:
            # Add a specific handler for the SELECT * case.
            if extractor == "*":
                if object_list:
                    obj = object_list[0]
                    value = obj.Label if hasattr(obj, "Label") else getattr(obj, "Name", "")
                    row.append(value)
                # '*' is the only column in this case, so we must stop here.
                continue

            value = None
            if isinstance(extractor, AggregateFunction):
                value = self._calculate_aggregate(extractor, object_list)
            elif isinstance(
                extractor, (StaticExtractor, FunctionBase, ReferenceExtractor, ArithmeticOperation)
            ):
                # For non-aggregate extractors, the value is based on the first object in the list.
                if object_list:
                    value = extractor.get_value(object_list[0])
            else:  # Should not be reached with proper validation
                value = "INVALID_EXTRACTOR"
            row.append(value)
        return row

    def _calculate_aggregate(self, extractor, object_list):
        """Helper to compute the value for a single aggregate function."""
        if extractor.function_name == "count":
            if extractor.argument == "*":
                return len(object_list)
            else:
                prop_name = extractor.argument.value
                return sum(1 for obj in object_list if _get_property(obj, prop_name) is not None)

        # For other aggregates, extract numeric values
        arg_extractor = extractor.argument
        values = []
        for obj in object_list:
            prop_val = arg_extractor.get_value(obj)
            if prop_val is not None:
                if isinstance(prop_val, FreeCAD.Units.Quantity):
                    prop_val = prop_val.Value
                if isinstance(prop_val, (int, float)):
                    values.append(prop_val)

        if not values:
            return None
        elif extractor.function_name == "sum":
            return sum(values)
        elif extractor.function_name == "min":
            return min(values)
        elif extractor.function_name == "max":
            return max(values)

        return f"'{extractor.function_name}' NOT_IMPL"


class FromClause:
    def __init__(self, reference):
        self.reference = reference

    def get_objects(self, source_objects=None):
        """
        Delegates the object retrieval to the contained logical object.
        This works for both ReferenceExtractor and FromFunctionBase children.
        """
        return self.reference.get_objects(source_objects=source_objects)


class WhereClause:
    def __init__(self, expression):
        self.expression = expression

    def matches(self, obj):
        return self.expression.evaluate(obj)


class BooleanExpression:
    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right

    def evaluate(self, obj):
        if self.op is None:
            return self.left.evaluate(obj)
        elif self.op == "and":
            return self.left.evaluate(obj) and self.right.evaluate(obj)
        elif self.op == "or":
            return self.left.evaluate(obj) or self.right.evaluate(obj)
        else:
            # An unknown operator is an invalid state and should raise an error.
            raise SqlEngineError(f"Unknown boolean operator: '{self.op}'")


class BooleanComparison:
    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right
        # Validation: Aggregate functions are not allowed in WHERE clauses.
        if isinstance(self.left, AggregateFunction) or isinstance(self.right, AggregateFunction):
            raise SqlEngineError(
                "Aggregate functions (like COUNT, SUM) cannot be used in a WHERE clause."
            )

    def evaluate(self, obj):
        # The 'get_value' method is polymorphic and works for ReferenceExtractor,
        # StaticExtractor, and all FunctionBase derivatives.
        left_val = self.left.get_value(obj)
        right_val = self.right.get_value(obj)
        if self.op == "is":
            return left_val is right_val
        if self.op == "is_not":
            return left_val is not right_val
        # Strict SQL-like NULL semantics: any comparison (except IS / IS NOT)
        # with a None (NULL) operand evaluates to False. Use IS / IS NOT for
        # explicit NULL checks.
        if left_val is None or right_val is None:
            return False

        # Normalize Quantities to their raw numerical values first.
        # After this step, we are dealing with basic Python types.
        if isinstance(left_val, FreeCAD.Units.Quantity):
            left_val = left_val.Value
        if isinstance(right_val, FreeCAD.Units.Quantity):
            right_val = right_val.Value

        # Prioritize numeric comparison if both operands are numbers.
        if isinstance(left_val, (int, float)) and isinstance(right_val, (int, float)):
            ops = {
                "=": lambda a, b: a == b,
                "!=": lambda a, b: a != b,
                ">": lambda a, b: a > b,
                "<": lambda a, b: a < b,
                ">=": lambda a, b: a >= b,
                "<=": lambda a, b: a <= b,
            }
            if self.op in ops:
                return ops[self.op](left_val, right_val)

        # Fallback to string-based comparison for all other cases (including 'like').
        try:
            str_left = str(left_val)
            str_right = str(right_val)
        except Exception:
            # This is a defensive catch. If an object's __str__ method is buggy and raises
            # an error, we treat the comparison as False rather than crashing the whole query.
            return False

        def like_to_regex(pattern):
            s = str(pattern).replace("%", ".*").replace("_", ".")
            return s

        ops = {
            "=": lambda a, b: a == b,
            "!=": lambda a, b: a != b,
            "like": lambda a, b: re.search(like_to_regex(b), a, re.IGNORECASE) is not None,
        }

        # Note: Operators like '>' are intentionally not in this dictionary.
        # If the code reaches here with a '>' operator and non-numeric types,
        # it will correctly return False, as a string-based '>' is not supported.
        return ops[self.op](str_left, str_right) if self.op in ops else False


class InComparison:
    """Represents a SQL 'IN (values...)' comparison."""

    def __init__(self, reference_extractor, literal_extractors):
        self.reference_extractor = reference_extractor
        # Eagerly extract the static string values for efficient lookup
        self.values_set = {ex.get_value(None) for ex in literal_extractors}

    def evaluate(self, obj):
        property_value = self.reference_extractor.get_value(obj)
        # The check is a simple Python 'in' against the pre-calculated set
        return property_value in self.values_set


class ArithmeticOperation:
    """Represents a recursive arithmetic operation (e.g., a + (b * c))."""

    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right

    def _normalize_value(self, value):
        """Converts Quantities to floats for calculation, propagating None."""
        # This is the first point of defense.
        if value is None:
            return None

        if isinstance(value, FreeCAD.Units.Quantity):
            return value.Value
        elif isinstance(value, (int, float)):
            return value
        else:
            # A non-numeric, non-None value is still an error.
            type_name = type(value).__name__
            raise SqlEngineError(
                f"Cannot perform arithmetic on a non-numeric value of type '{type_name}'."
            )

    def get_value(self, obj):
        """Recursively evaluates the calculation tree, propagating None."""
        left_val = self._normalize_value(self.left.get_value(obj))
        right_val = self._normalize_value(self.right.get_value(obj))

        # This is the second point of defense. If either operand resolved to None,
        # the entire arithmetic expression resolves to None (SQL NULL).
        if left_val is None or right_val is None:
            return None

        if self.op == "+":
            return left_val + right_val
        if self.op == "-":
            return left_val - right_val
        if self.op == "*":
            return left_val * right_val
        if self.op == "/":
            return left_val / right_val if right_val != 0 else float("inf")

        raise SqlEngineError(f"Unknown arithmetic operator: '{self.op}'")


class ReferenceExtractor:
    """
    Represents a request to extract a value from an object, handling nesting.

    This class is the core of property access in the SQL engine. It can represent a simple property
    access (e.g., `Label`), a nested property access using a dot-notation string (e.g.,
    `Shape.Volume`), or a chained access on the result of another function (e.g.,
    `PARENT(*).Label`).

    The chained access is achieved by making the class recursive. The `base` attribute can hold
    another extractor object (like `ParentFunction` or another `ReferenceExtractor`), which is
    evaluated first to get an intermediate object from which the final `value` is extracted.
    """

    def __init__(self, value, base=None):
        """
        Initializes the ReferenceExtractor.

        Parameters
        ----------
        value : str
            The name of the property to extract (e.g., 'Label', 'Shape.Volume').
        base : object, optional
            Another logical extractor object that, when evaluated, provides the base object for this
            extraction. If None, the property is extracted from the main object of the current row.
            Defaults to None.
        """
        self.value = value
        self.base = base

    def get_value(self, obj):
        """
        Extracts and returns the final property value from a given object.

        If `self.base` is set, this method first recursively calls `get_value` on the base to
        resolve the intermediate object (e.g., executing `PARENT(*)` to get the parent). It then
        extracts the property specified by `self.value` from that intermediate object.

        If `self.base` is None, it directly extracts the property from the provided row object
        `obj`.

        Parameters
        ----------
        obj : FreeCAD.DocumentObject
            The document object for the current row being processed.

        Returns
        -------
        any
            The value of the requested property, or None if any part of the access chain is invalid
            or returns None.
        """
        if self.base:
            base_object = self.base.get_value(obj)
            # If the base evaluates to None (e.g., PARENT(*) on a top-level object),
            # we cannot get a property from it. Return None to prevent errors.
            if base_object is None:
                return None
            return _get_property(base_object, self.value)
        else:
            # Original behavior for a base reference from the current row's object.
            return _get_property(obj, self.value)

    def get_objects(self, source_objects=None):
        """
        Provides the interface for the FromClause to get the initial set of objects to query.

        This method is only intended to be used for the special case of `FROM document`, where it
        returns all objects in the active document. In all other contexts, it returns an empty list.

        Returns
        -------
        list of FreeCAD.DocumentObject
            A list of all objects in the active document if `self.value` is 'document', otherwise an
            empty list.
        """
        if source_objects is not None:
            # If source_objects are provided, they override 'FROM document'.
            return source_objects
        if self.value == "document" and not self.base:
            found_objects = FreeCAD.ActiveDocument.Objects
            return found_objects
        return []


class StaticExtractor:
    def __init__(self, value):
        self.value = value

    def get_value(self, obj):
        return self.value


# --- Lark Transformer ---


class SqlTransformerMixin:
    """
    A mixin class containing all our custom transformation logic for SQL rules.
    It has no __init__ to avoid conflicts in a multiple inheritance scenario.
    """

    def start(self, i):
        return i[0]

    def statement(self, children):
        # The 'columns' rule produces a list of (extractor, display_name) tuples
        columns_info = next((c for c in children if c.__class__ == list), None)
        from_c = next((c for c in children if isinstance(c, FromClause)), None)
        where_c = next((c for c in children if isinstance(c, WhereClause)), None)
        group_by_c = next((c for c in children if isinstance(c, GroupByClause)), None)
        order_by_c = next((c for c in children if isinstance(c, OrderByClause)), None)

        return SelectStatement(columns_info, from_c, where_c, group_by_c, order_by_c)

    def from_source(self, items):
        # This method handles the 'from_source' rule.
        # items[0] will either be a CNAME token (for 'document') or a
        # transformed FromFunctionBase object.
        item = items[0]
        if isinstance(item, generated_sql_parser.Token) and item.type == "CNAME":
            # If it's the CNAME 'document', create the base ReferenceExtractor for it.
            return ReferenceExtractor(str(item))
        else:
            # Otherwise, it's already a transformed function object, so just return it.
            return item

    def from_clause(self, i):
        return FromClause(i[1])

    def where_clause(self, i):
        return WhereClause(i[1])

    def from_function(self, items):
        function_name_token = items[0]
        # The arguments are a list that can contain the subquery statement
        # and an optional StaticExtractor for max_depth.
        args = items[1:]
        function_name = str(function_name_token).upper()
        function_class = self.from_function_registry.get_class(function_name)
        if not function_class:
            raise ValueError(f"Unknown FROM function: {function_name}")
        return function_class(args)

    def group_by_clause(self, items):
        # Allow both property references and function calls as grouping keys.
        references = [
            item for item in items if isinstance(item, (ReferenceExtractor, FunctionBase))
        ]
        return GroupByClause(references)

    def order_by_clause(self, items):
        # items contains: ORDER, BY, reference, (",", reference)*, optional direction

        # The ORDER BY clause only operates on the names of the final columns, which are always
        # parsed as simple identifiers.
        column_references = []
        for item in items:
            if isinstance(item, ReferenceExtractor):
                column_references.append(item)
            # This is the new, stricter validation.
            elif isinstance(item, (FunctionBase, ArithmeticOperation)):
                raise ValueError(
                    "ORDER BY expressions are not supported directly. Please include the expression "
                    "as a column in the SELECT list with an alias, and ORDER BY the alias."
                )

        direction = "ASC"
        # The optional direction token will be the last item if it exists.
        last_item = items[-1]
        if isinstance(last_item, generated_sql_parser.Token) and last_item.type in ("ASC", "DESC"):
            direction = last_item.value.upper()

        return OrderByClause(column_references, direction)

    def columns(self, items):
        # `items` is a list of results from `column` rules, which are (extractor, display_name) tuples
        return items

    def as_clause(self, items):
        # The alias will be the second item. It can either be a transformed
        # StaticExtractor (from a quoted string) or a raw CNAME token.
        alias_part = items[1]

        if isinstance(alias_part, StaticExtractor):
            # Case 1: The alias was a quoted string like "Floor Name".
            # The 'literal' rule transformed it into a StaticExtractor.
            return alias_part.get_value(None)
        else:
            # Case 2: The alias was an unquoted name like FloorName.
            # The grammar passed the raw CNAME Token directly.
            return str(alias_part)

    def column(self, items):
        # Each item in `items` is either '*' (for SELECT *) or an extractor object.
        # We need to return a (extractor, display_name) tuple.
        extractor = items[0]
        alias = items[1] if len(items) > 1 else None

        # Determine the default display name first
        default_name = "Unknown Column"
        if extractor == "*":
            default_name = SELECT_STAR_HEADER
        elif isinstance(extractor, ReferenceExtractor):
            default_name = extractor.value
        elif isinstance(extractor, StaticExtractor):
            default_name = str(extractor.get_value(None))
        elif isinstance(extractor, AggregateFunction):
            # Correctly handle the argument for default name generation.
            arg = extractor.argument
            arg_display = "?"  # fallback
            if arg == "*":
                arg_display = "*"
            elif hasattr(arg, "value"):  # It's a ReferenceExtractor
                arg_display = arg.value
            default_name = f"{extractor.function_name.upper()}({arg_display})"
        elif isinstance(extractor, FunctionBase):
            # Create a nice representation for multi-arg functions
            arg_strings = []
            for arg_ex in extractor.arg_extractors:
                if arg_ex == "*":
                    arg_strings.append("*")
                elif isinstance(arg_ex, ReferenceExtractor):
                    arg_strings.append(arg_ex.value)
                elif isinstance(arg_ex, StaticExtractor):
                    arg_strings.append(f"'{arg_ex.get_value(None)}'")
                else:
                    arg_strings.append("?")  # Fallback
            default_name = f"{extractor.function_name.upper()}({', '.join(arg_strings)})"

        # Use the alias if provided, otherwise fall back to the default name.
        final_name = alias if alias is not None else default_name
        return (extractor, final_name)

    def boolean_expression_recursive(self, items):
        return BooleanExpression(items[0], items[1].value.lower(), items[2])

    def boolean_expression(self, i):
        return BooleanExpression(i[0], None, None)

    def boolean_or(self, i):
        return i[0]

    def boolean_and(self, i):
        return i[0]

    def boolean_term(self, i):
        return i[0]

    def boolean_comparison(self, items):
        return BooleanComparison(items[0], items[1], items[2])

    def primary(self, items):
        # This transformer handles the 'primary' grammar rule.
        # It transforms a CNAME token into a base ReferenceExtractor.
        # All other items (functions, literals, numbers) are already transformed
        # by their own methods, so we just pass them up.
        item = items[0]
        if isinstance(item, generated_sql_parser.Token) and item.type == "CNAME":
            return ReferenceExtractor(str(item))
        return item

    def factor(self, items):
        # This transformer handles the 'factor' rule for chained property access.
        # It receives a list of the transformed children.
        # The first item is the base (the result of the 'primary' rule).
        # The subsequent items are the CNAME tokens for each property access.

        # Start with the base of the chain.
        base_extractor = items[0]

        # Iteratively wrap the base with a new ReferenceExtractor for each
        # property in the chain.
        for prop_token in items[1:]:
            prop_name = str(prop_token)
            base_extractor = ReferenceExtractor(prop_name, base=base_extractor)

        return base_extractor

    def in_expression(self, items):
        # Unpack the items: the factor to check, and then all literal extractors.
        factor_to_check = items[0]
        literal_extractors = [item for item in items[1:] if isinstance(item, StaticExtractor)]
        return InComparison(factor_to_check, literal_extractors)

    def comparison_operator(self, i):
        return i[0]

    def eq_op(self, _):
        return "="

    def neq_op(self, _):
        return "!="

    def like_op(self, _):
        return "like"

    def is_op(self, _):
        return "is"

    def is_not_op(self, _):
        return "is_not"

    def gt_op(self, _):
        return ">"

    def lt_op(self, _):
        return "<"

    def gte_op(self, _):
        return ">="

    def lte_op(self, _):
        return "<="

    def operand(self, items):
        # This method is now "dumb" and simply passes up the already-transformed object.
        # The transformation of terminals happens in their own dedicated methods below.
        return items[0]

    def literal(self, items):
        return StaticExtractor(items[0].value[1:-1])

    def NUMBER(self, token):
        # This method is automatically called by Lark for any NUMBER terminal.
        return StaticExtractor(float(token.value))

    def NULL(self, token):
        # This method is automatically called by Lark for any NULL terminal.
        return StaticExtractor(None)

    def ASTERISK(self, token):
        # This method is automatically called by Lark for any ASTERISK terminal.
        # Return the string '*' to be used as a special identifier.
        return "*"

    def function_args(self, items):
        # This method just collects all arguments into a single list.
        return items

    def term(self, items):
        """
        Builds a left-associative tree for multiplication/division.
        This is a critical change to fix the data flow for the factor rule.
        """
        tree = items[0]
        for i in range(1, len(items), 2):
            op_token = items[i]
            right = items[i + 1]
            tree = ArithmeticOperation(tree, op_token.value, right)
        return tree

    def expr(self, items):
        """Builds a left-associative tree for addition/subtraction."""
        tree = items[0]
        for i in range(1, len(items), 2):
            op_token = items[i]
            right = items[i + 1]
            tree = ArithmeticOperation(tree, op_token.value, right)
        return tree

    def member_access(self, items):
        """
        This transformer handles the 'member_access' rule for chained property access.
        It can handle both simple properties (CNAME) and function calls after a dot.
        """
        # Start with the base of the chain (the result of the 'primary' rule).
        base_extractor = items[0]

        # The rest of the items are a mix of CNAME tokens and transformed Function objects.
        for member in items[1:]:
            if isinstance(member, generated_sql_parser.Token) and member.type == "CNAME":
                # Case 1: A simple property access like '.Label'
                # Wrap the current chain in a new ReferenceExtractor.
                base_extractor = ReferenceExtractor(str(member), base=base_extractor)
            else:
                # Case 2: A function call like '.PARENT(*)'
                # The 'member' is already a transformed object (e.g., ParentFunction).
                # We set its base to the current chain, making it the new end of the chain.
                member.base = base_extractor
                base_extractor = member

        return base_extractor

    def function(self, items):
        function_name_token = items[0]
        function_name = str(function_name_token).upper()
        # Arguments are optional (e.g. for a future function).
        args = items[1] if len(items) > 1 else []

        # Special handling for aggregates, which all use the AggregateFunction logic
        # but are instantiated via their specific subclasses.
        aggregate_map = {
            "COUNT": CountFunction,
            "SUM": SumFunction,
            "MIN": MinFunction,
            "MAX": MaxFunction,
        }
        if function_name in aggregate_map:
            # Instantiate the correct subclass (CountFunction, etc.) but pass the
            # function name (e.g., 'count') to the AggregateFunction constructor.
            return aggregate_map[function_name](function_name.lower(), args)

        # Look up the function in the injected SELECT function registry
        function_class = self.select_function_registry.get_class(function_name)

        if function_class:
            return function_class(function_name, args)

        # If the function is not in our registry, it's a validation error.
        raise ValueError(f"Unknown SELECT function: {function_name}")


# --- Engine Initialization ---


def _initialize_engine():
    """
    Creates and configures all components of the SQL engine.
    Function registration is now handled automatically via decorators on each
    function's class definition.
    """

    # 1. Define and instantiate the transformer.
    class FinalTransformer(generated_sql_parser.Transformer, SqlTransformerMixin):
        def __init__(self):
            # The transformer still needs access to the registries to look up
            # standard function classes.
            self.select_function_registry = select_function_registry
            self.from_function_registry = from_function_registry

    transformer = FinalTransformer()

    # 2. Instantiate the parser
    parser = generated_sql_parser.Lark_StandAlone()

    # 3. Generate friendly token names from the initialized parser
    friendly_token_names = _generate_friendly_token_names(parser)

    return parser, transformer, friendly_token_names


# --- Module-level Globals (initialized by the engine) ---
try:
    _parser, _transformer, _FRIENDLY_TOKEN_NAMES = _initialize_engine()
except Exception as e:
    _parser, _transformer, _FRIENDLY_TOKEN_NAMES = None, None, {}
    FreeCAD.Console.PrintError(f"BIM SQL engine failed to initialize: {e}\n")


# --- Internal API Functions ---


def _run_query(query_string: str, mode: str, source_objects: Optional[List] = None):
    """
    The single, internal entry point for the SQL engine.

    This function encapsulates the entire query process: parsing, transformation,
    validation, and execution. It uses a 'mode' parameter to decide whether
    to perform a full data execution or a lightweight, performant count. It is
    a "silent" function that raises a specific exception on any failure, but
    performs no logging itself.

    Parameters
    ----------
    query_string : str
        The raw SQL query string to be processed.
    mode : str
        The execution mode, either 'full_data' or 'count_only'.
    source_objects : list of FreeCAD.DocumentObject, optional
        If provided, the query will run on this list of objects instead of the
        entire document. Defaults to None.

    Returns
    -------
    int or tuple
        If mode is 'count_only', returns an integer representing the row count.
        If mode is 'full_data', returns a tuple `(headers, data_rows)`.

    Raises
    ------
    SqlEngineError
        For general engine errors, such as initialization failures or
        validation errors (e.g., mixing aggregates without GROUP BY).
    BimSqlSyntaxError
        For any syntax, parsing, or transformation error, with a flag to
        indicate if the query was simply incomplete.
    """

    def _parse_and_transform(query_string_internal: str) -> "SelectStatement":
        """Parses and transforms the string into a logical statement object."""
        if not _parser or not _transformer:
            raise SqlEngineError(
                "BIM SQL engine is not initialized. Check console for errors on startup."
            )
        try:
            tree = _parser.parse(query_string_internal)
            statement_obj = _transformer.transform(tree)
            statement_obj.validate()
            return statement_obj
        except ValueError as e:
            raise SqlEngineError(str(e))
        except VisitError as e:
            message = f"Transformer Error: Failed to process rule '{e.rule}'. Original error: {e.orig_exc}"
            raise BimSqlSyntaxError(message) from e
        except UnexpectedToken as e:
            # Heuristic for a better typing experience: If the unexpected token's
            # text is a prefix of any of the keywords the parser was expecting,
            # we can assume the user is still typing that keyword. In this case,
            # we treat the error as "Incomplete" instead of a harsh "Syntax Error".
            token_text = e.token.value.upper()
            # The `e.expected` list from Lark contains the names of the expected terminals.
            is_prefix_of_expected = any(
                expected_keyword.startswith(token_text)
                for expected_keyword in e.expected
                if expected_keyword.isupper()
            )

            if is_prefix_of_expected:
                raise BimSqlSyntaxError("Query is incomplete.", is_incomplete=True) from e

            # If it's not an incomplete keyword, proceed with a full syntax error.
            is_incomplete = e.token.type == "$END"
            # Filter out internal Lark tokens before creating the message
            friendly_expected = [
                _FRIENDLY_TOKEN_NAMES.get(t, f"'{t}'") for t in e.expected if not t.startswith("__")
            ]
            expected_str = ", ".join(friendly_expected)
            message = (
                f"Syntax Error: Unexpected '{e.token.value}' at line {e.line}, column {e.column}. "
                f"Expected {expected_str}."
            )
            raise BimSqlSyntaxError(message, is_incomplete=is_incomplete) from e
        except UnexpectedEOF as e:
            raise BimSqlSyntaxError("Query is incomplete.", is_incomplete=True) from e

    statement = _parse_and_transform(query_string)

    all_objects = statement.from_clause.get_objects(source_objects=source_objects)

    if mode == "count_only":
        # Phase 1: Perform the fast filtering and grouping to get the
        # correct final row count.
        grouped_data = statement._get_grouped_data(all_objects)
        row_count = len(grouped_data)

        # If there are no results, the query is valid and simply returns 0 rows.
        if row_count == 0:
            return 0, [], []

        # Phase 2 Validation: Perform a "sample execution" on the first group
        # to validate the SELECT clause and catch any execution-time errors.
        # We only care if it runs without error; the result is discarded.
        # For aggregates without GROUP BY, the "group" is all filtered objects.
        is_single_row_aggregate = (
            any(isinstance(ex, AggregateFunction) for ex, _ in statement.columns_info)
            and not statement.group_by_clause
        )
        if is_single_row_aggregate:
            sample_group = [obj for group in grouped_data for obj in group]
        else:
            sample_group = grouped_data[0]

        # Get headers from the parsed statement object.
        headers = [display_name for _, display_name in statement.columns_info]
        # Get data from the process method, which returns a single list of rows.
        data = statement._process_select_columns([sample_group], all_objects)

        # If the sample execution succeeds, the query is fully valid.
        # The resulting_objects are not needed for the count validation itself,
        # but are returned for API consistency.
        resulting_objects = _map_results_to_objects(headers, data)
        return row_count, headers, resulting_objects
    else:  # 'full_data'
        headers, results_data = statement.execute(all_objects)
        resulting_objects = _map_results_to_objects(headers, results_data)
        return headers, results_data, resulting_objects


# --- Public API Objects ---


class ReportStatement:
    """A data model for a single statement within a BIM Report.

    This class encapsulates all the information required to execute and present
    a single query. It holds the SQL string itself, options for how its
    results should be formatted in the final spreadsheet, and the crucial flag
    that controls whether it participates in a pipeline.

    Instances of this class are created and managed by the ReportTaskPanel UI
    and are passed as a list to the `execute_pipeline` engine function. The
    class includes methods for serialization to and from a dictionary format,
    allowing it to be persisted within a FreeCAD document object.

    Parameters
    ----------
    description : str, optional
        A user-defined description for the statement. This is shown in the UI
        and can optionally be used as a section header in the spreadsheet.
    query_string : str, optional
        The SQL query to be executed for this statement.
    use_description_as_header : bool, optional
        If True, the `description` will be written as a merged header row
        before the statement's results in the spreadsheet. Defaults to False.
    include_column_names : bool, optional
        If True, the column names from the SQL query (e.g., 'Label', 'Area')
        will be included as a header row for this statement's results.
        Defaults to True.
    add_empty_row_after : bool, optional
        If True, an empty row will be inserted after this statement's results
        to provide visual spacing in the report. Defaults to False.
    print_results_in_bold : bool, optional
        If True, the data cells for this statement's results will be formatted
        with a bold font for emphasis. Defaults to False.
    is_pipelined : bool, optional
        If True, this statement will use the resulting objects from the
        previous statement as its data source instead of the entire document.
        This is the flag that enables pipeline functionality. Defaults to False.

    Attributes
    ----------
    _validation_status : str
        A transient (not saved) string indicating the validation state for the
        UI (e.g., "OK", "ERROR").
    _validation_message : str
        A transient (not saved) user-facing message corresponding to the
        validation status.
    _validation_count : int
        A transient (not saved) count of objects found during validation.
    """

    def __init__(
        self,
        description="",
        query_string="",
        use_description_as_header=False,
        include_column_names=True,
        add_empty_row_after=False,
        print_results_in_bold=False,
        is_pipelined=False,
    ):
        self.description = description
        self.query_string = query_string
        self.use_description_as_header = use_description_as_header
        self.include_column_names = include_column_names
        self.add_empty_row_after = add_empty_row_after
        self.print_results_in_bold = print_results_in_bold
        self.is_pipelined = is_pipelined

        # Internal validation state (transient, not serialized)
        self._validation_status = "Ready"  # e.g., "OK", "0_RESULTS", "ERROR", "INCOMPLETE"
        self._validation_message = translate(
            "Arch", "Ready"
        )  # e.g., "Found 5 objects.", "Syntax Error: ..."
        self._validation_count = 0

    def dumps(self):
        """Returns the internal state for serialization."""
        return {
            "description": self.description,
            "query_string": self.query_string,
            "use_description_as_header": self.use_description_as_header,
            "include_column_names": self.include_column_names,
            "add_empty_row_after": self.add_empty_row_after,
            "print_results_in_bold": self.print_results_in_bold,
            "is_pipelined": self.is_pipelined,
        }

    def loads(self, state):
        """Restores the internal state from serialized data."""
        self.description = state.get("description", "")
        self.query_string = state.get("query_string", "")
        self.use_description_as_header = state.get("use_description_as_header", False)
        self.include_column_names = state.get("include_column_names", True)
        self.add_empty_row_after = state.get("add_empty_row_after", False)
        self.print_results_in_bold = state.get("print_results_in_bold", False)
        self.is_pipelined = state.get("is_pipelined", False)

        # Validation state is transient and re-calculated on UI load/edit
        self._validation_status = "Ready"
        self._validation_message = translate("Arch", "Ready")
        self._validation_count = 0

    def validate_and_update_status(self):
        """Runs validation for this statement's query and updates its internal status."""
        if not self.query_string.strip():
            self._validation_status = "OK"  # Empty query is valid, no error
            self._validation_message = translate("Arch", "Ready")
            self._validation_count = 0
            return

        # Avoid shadowing the module-level `count` function by using a
        # different local name for the numeric result.
        count_result, error = count(self.query_string)

        if error == "INCOMPLETE":
            self._validation_status = "INCOMPLETE"
            self._validation_message = translate("Arch", "Typing...")
            self._validation_count = -1
        elif error:
            self._validation_status = "ERROR"
            self._validation_message = error
            self._validation_count = -1
        elif count_result == 0:
            self._validation_status = "0_RESULTS"
            self._validation_message = translate("Arch", "Query is valid, but found 0 objects.")
            self._validation_count = 0
        else:
            self._validation_status = "OK"
            self._validation_message = (
                f"{translate('Arch', 'Found')} {count_result} {translate('Arch', 'objects')}."
            )
            self._validation_count = count_result


# --- Public API Functions ---


def selectObjectsFromPipeline(statements: list) -> list:
    """
    Executes a multi-statement pipeline and returns a final list of FreeCAD objects.

    This is a high-level convenience function for scripting complex, multi-step
    selections that are too difficult or cumbersome for a single SQL query.

    Parameters
    ----------
    statements : list of ArchReport.ReportStatement
        A configured list of statements defining the pipeline.

    Returns
    -------
    list of FreeCAD.DocumentObject
        A list of the final FreeCAD.DocumentObject instances that result from
        the pipeline.
    """
    # 1. The pipeline orchestrator is a generator. We consume it to get the
    #    list of all output blocks.
    output_blocks = list(execute_pipeline(statements))

    if not output_blocks:
        return []

    # 2. For scripting, we are only interested in the final result.
    #    The final result is the last item yielded by the generator.
    _, final_headers, final_data = output_blocks[-1]

    # 3. Use the existing helper to map the final raw data back to objects.
    return _map_results_to_objects(final_headers, final_data)


def execute_pipeline(statements: List["ReportStatement"]):
    """
    Executes a list of statements, handling chained data flow as a generator.

    This function orchestrates a multi-step query. It yields the results of
    each standalone statement or the final statement of a contiguous pipeline,
    allowing the caller to handle presentation.

    Parameters
    ----------
    statements : list of ArchReport.ReportStatement
        A configured list of statements to execute.

    Yields
    ------
    tuple
        A tuple `(statement, headers, data_rows)` for each result block that
        should be outputted.
    """
    pipeline_input_objects = None

    for i, statement in enumerate(statements):
        # Skip empty queries (user may have a placeholder statement)
        if not statement.query_string or not statement.query_string.strip():
            # If this empty statement breaks a chain, reset the pipeline
            if not statement.is_pipelined:
                pipeline_input_objects = None
            continue

        # Determine the data source for the current query
        source = pipeline_input_objects if statement.is_pipelined else None

        try:
            headers, data, resulting_objects = _run_query(
                statement.query_string, mode="full_data", source_objects=source
            )
        except (SqlEngineError, BimSqlSyntaxError) as e:
            # If a step fails, yield an error block and reset the pipeline
            yield (statement, ["Error"], [[str(e)]])
            pipeline_input_objects = None
            continue

        # The output of this query becomes the input for the next one.
        pipeline_input_objects = resulting_objects

        # If this statement is NOT pipelined, it breaks any previous chain.
        # Its own output becomes the new potential start for a subsequent chain.
        if not statement.is_pipelined:
            pass  # The pipeline_input_objects is already correctly set for the next step.

        # Determine if this statement's results should be part of the final output.
        is_last_statement = i == len(statements) - 1

        # A statement's results are yielded UNLESS it is an intermediate step.
        # An intermediate step is any statement that is immediately followed by a pipelined statement.
        is_intermediate_step = not is_last_statement and statements[i + 1].is_pipelined

        if not is_intermediate_step:
            yield (statement, headers, data)


def count(query_string: str, source_objects: Optional[List] = None) -> Tuple[int, Optional[str]]:
    """
    Executes a query and returns only the count of resulting rows.

    This is a "safe" public API function intended for UI components like
    live validation feedback. It catches all exceptions and returns a
    consistent tuple, making it safe to call with incomplete user input.

    Parameters
    ----------
    query_string : str
        The raw SQL query string.
    source_objects : list of FreeCAD.DocumentObject, optional
        If provided, the query count will run on this list of objects instead
        of the entire document. Defaults to None.

    Returns
    -------
    Tuple[int, Optional[str]]
        A tuple `(count, error_string)`.
        On success, `count` is the number of rows and `error_string` is `None`.
        On failure, `count` is `-1` and `error_string` contains a user-friendly
        description of the error (e.g., "INCOMPLETE", "Syntax Error").
    """
    if (query_string.count("'") % 2 != 0) or (query_string.count('"') % 2 != 0):
        return -1, "INCOMPLETE"

    try:
        count_result, _, _ = _run_query(
            query_string, mode="count_only", source_objects=source_objects
        )
        return count_result, None
    except BimSqlSyntaxError as e:
        if e.is_incomplete:
            return -1, "INCOMPLETE"
        else:
            # Pass the specific, detailed error message up to the caller.
            return -1, str(e)
    except SqlEngineError as e:
        return -1, str(e)


def select(query_string: str) -> Tuple[List[str], List[List[Any]]]:
    """
    Executes a query and returns the full results.

    This function implements a "Catch, Log, and Re-Raise" pattern. It is
    safe in that it logs a detailed error to the console, but it is also
    "unsafe" in that it re-raises the exception to signal the failure to
    the calling function, which is responsible for handling it.

    Returns
    -------
    Tuple[List[str], List[List[Any]]]
        A tuple `(headers, data_rows)`.

    Raises
    ------
    SqlEngineError
        Re-raises any SqlEngineError or BimSqlSyntaxError without logging it.
        The caller is responsible for logging and handling.
    """
    # This is the "unsafe" API. It performs no error handling and lets all
    # exceptions propagate up to the caller, who is responsible for logging
    # or handling them as needed.
    headers, results_data, _ = _run_query(query_string, mode="full_data")
    return headers, results_data


def selectObjects(query_string: str) -> List["FreeCAD.DocumentObject"]:
    """
    Selects objects from the active document using a SQL-like query.

    This provides a declarative way to select BIM objects
    based on their properties. This is a convenience function for scripting.

    Parameters
    ----------
    query_string : str
        The SQL query to execute. For example:
        'SELECT * FROM document WHERE IfcType = "Wall" AND Label LIKE "%exterior%"'

    Returns
    -------
    list of App::DocumentObject
        A list of the FreeCAD document objects that match the query.
        Returns an empty list if the query is invalid or finds no objects.
    """
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("Arch.selectObjects() requires an active document.\n")
        return []

    try:
        # Execute the query using the internal 'select' function.
        headers, data_rows = select(query_string)

        return _map_results_to_objects(headers, data_rows)

    except (SqlEngineError, BimSqlSyntaxError) as e:
        # If the query fails, log the error and return an empty list.
        FreeCAD.Console.PrintError(f"Arch.selectObjects query failed: {e}\n")
        return []


def getSqlKeywords(kind="all") -> List[str]:
    """
    Returns a list of all keywords and function names for syntax highlighters.

    This function provides the single source of truth for SQL syntax in the
    BIM workbench. It dynamically introspects the initialized Lark parser and
    the function registries. It can return different subsets of keywords
    based on the `kind` parameter.

    Parameters
    ----------
    kind : str, optional
        Specifies the type of keyword list to return.
        - 'all': (Default) Returns all single keywords from the grammar.
        - 'no_space': Returns a list of keywords that should not have a
          trailing space in autocompletion (e.g., functions, modifiers).

    Returns
    -------
    List[str]
        A sorted, unique list of keywords and function names (e.g.,
        ['SELECT', 'FROM', 'COUNT', 'CHILDREN', ...]). Returns an empty list
        if the engine failed to initialize.
    """
    # The parser and transformer are initialized at the module level.
    # We just check if the initialization was successful.
    if not _parser or not _transformer:
        return []

    if kind == "no_space":
        no_space_keywords = _get_sql_function_names()
        no_space_keywords.update({"ASC", "DESC"})
        return sorted(list(no_space_keywords))

    # Default behavior for kind='all'
    keywords = []
    # This blocklist contains all uppercase terminals from the grammar that are NOT
    # actual keywords a user would type.
    NON_KEYWORD_TERMINALS = {"WS", "CNAME", "STRING", "NUMBER", "LPAR", "RPAR", "COMMA", "ASTERISK"}

    # 1. Get all keywords from the parser's terminals.
    # A terminal is a keyword if its name is uppercase and not in our blocklist.
    for term in _parser.terminals:
        # Filter out internal, anonymous tokens generated by Lark.
        is_internal = term.name.startswith("__")  # Filter out internal __ANON tokens
        if term.name.isupper() and term.name not in NON_KEYWORD_TERMINALS and not is_internal:
            keywords.append(term.name)

    # 2. Get all registered function names.
    keywords.extend(list(_get_sql_function_names()))

    return sorted(list(set(keywords)))  # Return a sorted, unique list.


def _get_sql_function_names() -> set:
    """(Internal) Returns a set of all registered SQL function names."""
    if not _transformer:
        return set()
    select_funcs = set(_transformer.select_function_registry._functions.keys())
    from_funcs = set(_transformer.from_function_registry._functions.keys())
    return select_funcs.union(from_funcs)


def getSqlApiDocumentation() -> dict:
    """
    Generates a structured dictionary describing the supported SQL dialect.

    This function introspects the live engine configuration and performs
    just-in-time translation of descriptive strings to ensure they appear
    in the user's current language.

    Returns
    -------
    dict
        A dictionary with keys 'clauses' and 'functions'. 'functions' is a
        dict of lists, categorized by function type.
    """
    api_data = {"clauses": [], "functions": {}}

    # Combine all function registries into one for easier iteration.
    all_registries = {
        **_transformer.select_function_registry._functions,
        **_transformer.from_function_registry._functions,
    }

    # Group functions by their registered category, translating as we go.
    for name, data in all_registries.items():
        # The category and description strings were marked for translation
        # with a context of "ArchSql" when they were registered.
        translated_category = translate("ArchSql", data["category"])

        if translated_category not in api_data["functions"]:
            api_data["functions"][translated_category] = []

        api_data["functions"][translated_category].append(
            {
                "name": name,
                "signature": data["signature"],
                "description": translate("ArchSql", data["description"]),
                "snippet": data["snippet"],
            }
        )

    # To get a clean list of "Clauses" for the cheatsheet, we use an explicit
    # whitelist of keywords that represent major SQL clauses.
    CLAUSE_KEYWORDS = {
        "SELECT",
        "FROM",
        "WHERE",
        "GROUP",
        "BY",
        "ORDER",
        "AS",
        "AND",
        "OR",
        "IS",
        "NOT",
        "IN",
        "LIKE",
    }
    all_terminals = {term.name for term in _parser.terminals}
    api_data["clauses"] = sorted([k for k in CLAUSE_KEYWORDS if k in all_terminals])

    return api_data
