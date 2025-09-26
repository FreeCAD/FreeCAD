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
from Draft import get_type

if FreeCAD.GuiUp:
#    from PySide.QtCore import QT_TRANSLATE_NOOP
    from draftutils.translate import translate
else:
    def translate(ctxt, txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

# Import exception types from the generated parser for type-safe handling.
from generated_sql_parser import UnexpectedEOF, UnexpectedToken, VisitError
import generated_sql_parser

from typing import List, Tuple, Any, Optional
__all__ = [
    'select',
    'count',
    'getSqlKeywords',
    'getSqlApiDocumentation',
    'BimSqlSyntaxError',
    'SqlEngineError',
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

SELECT_STAR_HEADER = 'Object Label'
_CUSTOM_FRIENDLY_TOKEN_NAMES = {
    # This dictionary provides overrides for tokens where the name is not user-friendly.
    # Punctuation
     'RPAR': "')'",
     'LPAR': "'('",
     'COMMA': "','",
     'ASTERISK': "'*'",
    # Other non-keyword tokens
     'CNAME': "a property or function name",
 }


# --- Internal Helper Functions ---

def _get_property(obj, prop_name):
    """Gets a property from a FreeCAD object, including sub-properties."""
    # The property name implies sub-property access (e.g., 'Placement.Base.x')
    is_nested_property = lambda prop_name: '.' in prop_name

    if not is_nested_property(prop_name):
        # Handle simple, direct properties first, which is the most common case.
        if hasattr(obj, prop_name):
            return getattr(obj, prop_name)
        return None
    else:
        # Handle nested properties (e.g., Placement.Base.x)
        current_obj = obj
        parts = prop_name.split('.')
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


# --- Logical Classes for the SQL Statement Object Model ---

class FunctionRegistry:
    """A simple class to manage the registration of SQL functions."""
    def __init__(self):
        self._functions = {}

    def register(self, name, function_class, category, signature, description, snippet=""):
        """Registers a class to handle a function with the given name."""
        self._functions[name.upper()] = {
            'class': function_class,
            'category': category,
            'signature': signature,
            'description': description,
            'snippet': snippet,
        }

    def get_class(self, name):
        """Retrieves the class registered for a given function name."""
        data = self._functions.get(name.upper())
        return data['class'] if data else None


class AggregateFunction:
    """Represents an aggregate function call like COUNT(*) or SUM(Height)."""
    def __init__(self, name, arg_extractors):
        self.function_name = name.lower()
        self.arg_extractors = arg_extractors

        if len(self.arg_extractors) != 1:
            raise ValueError(f"Aggregate function {self.function_name.upper()} requires exactly one argument.")

        self.argument = self.arg_extractors[0]

    def get_value(self, obj):
        # This method should never be called directly in a row-by-row context like a WHERE clause.
        # Aggregates are handled in a separate path (_execute_grouped_query or
        # the single-row path in _execute_non_grouped_query). Calling it here is a semantic error.
        raise SqlEngineError(f"Aggregate function '{self.function_name.upper()}' cannot be used in this context.")


class FunctionBase:
    """A base class for non-aggregate functions like TYPE, CONCAT, etc."""
    def __init__(self, function_name, arg_extractors):
        self.function_name = function_name
        self.arg_extractors = arg_extractors
        # Arity (argument count) check is performed by child classes.
    def get_value(self, obj):
        """Calculates the function's value for a single object row."""
        raise NotImplementedError()


class TypeFunction(FunctionBase):
    """Implements the TYPE() function."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1 or self.arg_extractors[0] != '*':
            raise ValueError(f"Function {self.function_name} requires exactly one argument: '*'")

    def get_value(self, obj):
        # The argument for TYPE is the object itself, represented by '*'.
        import Draft
        return Draft.get_type(obj)


class LowerFunction(FunctionBase):
    """Implements the LOWER() string function."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1:
            raise ValueError(f"Function {self.function_name} requires exactly one argument.")

    def get_value(self, obj):
        value = self.arg_extractors[0].get_value(obj)
        return str(value).lower() if value is not None else None


class UpperFunction(FunctionBase):
    """Implements the UPPER() string function."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1:
            raise ValueError(f"Function {self.function_name} requires exactly one argument.")

    def get_value(self, obj):
        value = self.arg_extractors[0].get_value(obj)
        return str(value).upper() if value is not None else None


class ConcatFunction(FunctionBase):
    """Implements the CONCAT() string function."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if not self.arg_extractors:
            raise ValueError(f"Function {self.function_name} requires at least one argument.")

    def get_value(self, obj):
        parts = [str(ex.get_value(obj)) if ex.get_value(obj) is not None else '' for ex in self.arg_extractors]
        return "".join(parts)


class ConvertFunction(FunctionBase):
    """Implements the CONVERT(Quantity, 'unit') function."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 2:
            raise ValueError(f"Function {self.function_name} requires exactly two arguments: a property and a unit string.")

    def get_value(self, obj):
        # Evaluate the arguments to get the input value and target unit string.
        input_value = self.arg_extractors[0].get_value(obj)
        unit_string = self.arg_extractors[1].get_value(obj)

        # The first argument must be a Quantity object to be convertible.
        if not isinstance(input_value, FreeCAD.Units.Quantity):
            raise SqlEngineError(f"CONVERT function requires a Quantity object as the first argument, but got {type(input_value).__name__}.")

        try:
            # Use the underlying API to perform the conversion.
            result_quantity = input_value.getValueAs(str(unit_string))
            return result_quantity.Value
        except Exception as e:
            # The API will raise an error for incompatible units (e.g., mm to kg).
            raise SqlEngineError(f"Unit conversion failed: {e}")


class FromFunctionBase:
    """Base class for all functions used in a FROM clause."""
    def __init__(self, substatement):
        self.substatement = substatement

    def get_objects(self):
        """Executes the subquery and returns the final list of objects."""
        raise NotImplementedError()

    def _get_parent_objects(self):
        """
        Helper to execute the subquery and resolve the resulting rows back
        into a list of FreeCAD document objects.
        """
        headers, rows = self.substatement.execute()
        if not rows:
            return []

        # Determine which column to use for mapping back to objects
        label_idx = headers.index('Label') if 'Label' in headers else -1
        name_idx = headers.index('Name') if 'Name' in headers else -1
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


class ChildrenFromFunction(FromFunctionBase):
    """Implements the CHILDREN() function."""

    def _collect_contained_children(self, parent_obj, results_set):
        """
        Simple recursive helper to find children in .Group,
        traversing through any nested groups.
        """
        # Use safe getattr to avoid errors on objects without a .Group
        group_list = getattr(parent_obj, 'Group', None)
        if not group_list:
            return

        for member in group_list:
            # If member is a group, recurse. Otherwise, add it to results.
            if getattr(member, 'isDerivedFrom', lambda x: False)("App::DocumentObjectGroup"):
                self._collect_contained_children(member, results_set)
            else:
                results_set.add(member)

    def get_objects(self):
        # Execute the subquery to find parent objects.
        parent_objects = self._get_parent_objects()
        if not parent_objects:
            return []

        parent_set = set(parent_objects)
        results_set = set()

        # 1. Find HOSTED children in a single pass over the document.
        for obj in FreeCAD.ActiveDocument.Objects:
            hosts = getattr(obj, 'Hosts', None)
            if hosts and any(h in parent_set for h in hosts):
                results_set.add(obj)

        # 2. Find CONTAINED children by traversing .Group on each parent.
        for parent in parent_set:
            self._collect_contained_children(parent, results_set)

        return list(results_set)


class ParentFunction(FunctionBase):
    """Implements the PARENT(*) function to find an object's container."""
    def __init__(self, function_name, arg_extractors):
        super().__init__(function_name, arg_extractors)
        if len(self.arg_extractors) != 1 or self.arg_extractors[0] != '*':
            raise ValueError(f"Function {self.function_name} requires exactly one argument: '*'")

    def get_value(self, obj):
        """
        Walks up the hierarchy from the object to find the first
        architecturally significant parent (e.g., a Floor, not a generic Group).
        """
        current_obj = obj
        # Limit search depth to prevent infinite loops in malformed documents
        for _ in range(20):
            if not hasattr(current_obj, 'InList') or not current_obj.InList:
                return None # Reached the top of this branch

            # For simplicity, we consider the first parent in the InList.
            parent = current_obj.InList[0]
            parent_type = get_type(parent)

            # A "significant" parent is an architectural container (Floor, Building)
            # or a host object (Wall, Structure). A generic "Group" is not.
            significant_types = {'BuildingPart', 'Wall', 'Structure'}
            is_significant = parent_type in significant_types

            if is_significant:
                return parent # Found the significant parent

            # If the parent is not significant (e.g., it's a generic group),
            # continue walking up the tree from that parent.
            current_obj = parent

        return None # Search limit exceeded


class GroupByClause:
    """Represents the GROUP BY clause of a SQL statement."""
    def __init__(self, columns):
        # columns is a list of ReferenceExtractor objects
        self.columns = columns


class OrderByClause:
    """Represents the ORDER BY clause of a SQL statement."""
    def __init__(self, column_references, direction='ASC'):
        self.column_names = [ref.value for ref in column_references]
        self.direction = direction.upper()

    def __repr__(self):
        # Add a __repr__ for clearer debug logs.
        return f"<OrderByClause columns='{self.column_names}' dir='{self.direction}'>"


class SelectStatement:
    def __init__(self, columns_info, from_clause, where_clause, group_by_clause, order_by_clause):
        self.columns_info = columns_info # Stores (extractor_object, display_name) tuples
        self.from_clause = from_clause
        self.where_clause = where_clause
        self.group_by_clause = group_by_clause
        self.order_by_clause = order_by_clause

    def execute(self):
        # 1. Phase 1: Get filtered and grouped object data.
        grouped_data = self._get_grouped_data()
        # 2. Determine the column headers from the parsed statement
        headers = [display_name for _, display_name in self.columns_info]

        # 3. Phase 2: Process the SELECT columns to get the final data rows.
        results_data = self._process_select_columns(grouped_data)

        # 4. Perform final sorting if an ORDER BY clause was provided.
        if self.order_by_clause:
            sort_keys = []
            for original_name in self.order_by_clause.column_names:
                final_name = original_name
                # Check if the ORDER BY column was aliased in the SELECT list.
                for extractor, aliased_name in self.columns_info:
                    if isinstance(extractor, ReferenceExtractor) and extractor.value == original_name:
                        final_name = aliased_name
                        break

                try:
                    # Find the column index to sort by from the final headers.
                    sort_column_index = headers.index(final_name)
                    sort_keys.append(sort_column_index)
                except ValueError:
                    raise ValueError(
                        f"ORDER BY column '{original_name}' is not in the SELECT list."
                    )

            is_descending = self.order_by_clause.direction == 'DESC'

            # Define a sort key that can handle different data types.
            def sort_key(row):
                """
                Returns a tuple of sortable keys for a given row, one for each
                column specified in the ORDER BY clause.
                """
                keys = []
                for index in sort_keys:
                    value = row[index]
                    # Create a consistent, comparable key for each value.
                    if value is None:
                        keys.append((0, None))  # Nones sort first.
                    elif isinstance(value, (int, float, FreeCAD.Units.Quantity)):
                        num_val = value.Value if isinstance(value, FreeCAD.Units.Quantity) else value
                        keys.append((1, num_val)) # Numbers sort second.
                    else:
                        keys.append((2, str(value))) # Everything else sorts as a string.
                return tuple(keys)

            results_data.sort(key=sort_key, reverse=is_descending)

        return headers, results_data

    def validate(self):
        """
        Validates the select statement against SQL rules, such as those for GROUP BY.
        Raises ValueError on failure with a user-friendly message.
        """
        if self.group_by_clause:
            # Rule: Every column in the SELECT list must either be an aggregate function,
            # a static value, or be part of the GROUP BY clause.
            group_by_col_names = {ex.value for ex in self.group_by_clause.columns}

            for extractor, _ in self.columns_info:
                if isinstance(extractor, (AggregateFunction, StaticExtractor, FunctionBase)):
                    continue  # Valid cases

                if extractor == '*':
                    raise ValueError("Cannot use '*' in a SELECT statement with a GROUP BY clause.")

                if isinstance(extractor, ReferenceExtractor):
                    if extractor.value not in group_by_col_names:
                        raise ValueError(
                            f"Column '{extractor.value}' must appear in the GROUP BY clause "
                            "or be used in an aggregate function."
                        )
            return

        # Rule: If there is no GROUP BY, you cannot mix aggregate and non-aggregate columns.
        has_aggregate = any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info)
        # A non-aggregate is a ReferenceExtractor or a scalar function (FunctionBase).
        # StaticExtractors are always allowed.
        has_non_aggregate = any(isinstance(ex, (ReferenceExtractor, FunctionBase))
                                          for ex, _ in self.columns_info)

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
                    if extractor.function_name == 'count':
                        # Distinguish between COUNT(*) and COUNT(property)
                        if extractor.argument == '*':
                            value = len(object_list)
                        else:
                            # Count only objects where the specified property is not None
                            prop_name = extractor.argument.value
                            count = sum(1 for obj in object_list if _get_property(obj, prop_name) is not None)
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
                            value = None # Return None if no valid numeric values were found
                        elif extractor.function_name == 'sum':
                            value = sum(values)
                        elif extractor.function_name == 'min':
                            value = min(values)
                        elif extractor.function_name == 'max':
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
                    if extractor.function_name == 'count':
                        if extractor.argument == '*':
                            value = len(objects)
                        else:
                            # Count only objects where the specified property is not None
                            prop_name = extractor.argument.value
                            count = sum(1 for obj in objects if _get_property(obj, prop_name) is not None)
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
                            elif extractor.function_name == 'sum':
                                value = sum(values)
                            elif extractor.function_name == 'min':
                                value = min(values)
                            elif extractor.function_name == 'max':
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
                    if extractor == '*':
                        value = obj.Label if hasattr(obj, 'Label') else getattr(obj, 'Name', '')
                    else:
                        value = extractor.get_value(obj)

                    # Append the raw value; formatting is the writer's responsibility
                    row.append(value)
                results_data.append(row)

        return results_data

    def get_row_count(self):
        """
        Calculates only the number of rows the query will produce, performing
        the minimal amount of work necessary. This is used by Arch.count()
        for a fast UI preview.
        """
        grouped_data = self._get_grouped_data()
        return len(grouped_data)

    def _get_grouped_data(self):
        """
        Performs Phase 1 of execution: FROM, WHERE, and GROUP BY.
        This is the fast part of the query that only deals with object lists.
        Returns a list of "groups", where each group is a list of objects.
        """
        all_objects = self.from_clause.get_objects()
        filtered_objects = [o for o in all_objects if self.where_clause is None or self.where_clause.matches(o)]

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

    def _process_select_columns(self, grouped_data):
        """
        Performs Phase 2 of execution: processes the SELECT columns.
        This is the slow part of the query that does data extraction,
        function calls, and aggregation.
        """
        results_data = []

        # Handle SELECT * as a special case for non-grouped queries
        if not self.group_by_clause and self.columns_info and self.columns_info[0][0] == '*':
            for group in grouped_data:
                obj = group[0]
                value = obj.Label if hasattr(obj, 'Label') else getattr(obj, 'Name', '')
                results_data.append([value])
            return results_data

        is_single_row_aggregate = any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info) and not self.group_by_clause
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
            value = None
            if isinstance(extractor, AggregateFunction):
                value = self._calculate_aggregate(extractor, object_list)
            elif isinstance(extractor, (StaticExtractor, FunctionBase, ReferenceExtractor, ArithmeticOperation)):
                # For non-aggregate extractors, the value is based on the first object in the list.
                if object_list:
                    value = extractor.get_value(object_list[0])
            else: # Should not be reached with proper validation
                value = "INVALID_EXTRACTOR"
            row.append(value)
        return row

    def _calculate_aggregate(self, extractor, object_list):
        """Helper to compute the value for a single aggregate function."""
        if extractor.function_name == 'count':
            if extractor.argument == '*':
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
        elif extractor.function_name == 'sum':
            return sum(values)
        elif extractor.function_name == 'min':
            return min(values)
        elif extractor.function_name == 'max':
            return max(values)

        return f"'{extractor.function_name}' NOT_IMPL"


class FromClause:
    def __init__(self, reference):
        self.reference = reference
    def get_objects(self):
        """
        Delegates the object retrieval to the contained logical object.
        This works for both ReferenceExtractor and FromFunctionBase children.
        """
        return self.reference.get_objects()


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
        elif self.op == 'and':
            return self.left.evaluate(obj) and self.right.evaluate(obj)
        elif self.op == 'or':
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
            raise SqlEngineError("Aggregate functions (like COUNT, SUM) cannot be used in a WHERE clause.")

    def evaluate(self, obj):
        # The 'get_value' method is polymorphic and works for ReferenceExtractor,
        # StaticExtractor, and all FunctionBase derivatives.
        left_val = self.left.get_value(obj)
        right_val = self.right.get_value(obj)
        if self.op == 'is': return left_val is right_val
        if self.op == 'is_not': return left_val is not right_val
        # Strict SQL-like NULL semantics: any comparison (except IS / IS NOT)
        # with a None (NULL) operand evaluates to False. Use IS / IS NOT for
        # explicit NULL checks.
        if left_val is None or right_val is None:
            return False
        if isinstance(left_val, FreeCAD.Units.Quantity): left_val = left_val.Value
        if isinstance(right_val, FreeCAD.Units.Quantity): right_val = right_val.Value

        def like_to_regex(pattern):
            s = str(pattern).replace('%', '.*').replace('_', '.')
            return s

        ops = {
            '=': lambda a,b: a == b, '!=': lambda a,b: a != b,
            '>': lambda a,b: a > b, '<': lambda a,b: a < b,
            '>=': lambda a,b: a >= b, '<=': lambda a,b: a <= b,
            'like': lambda a,b: re.search(like_to_regex(b), str(a), re.IGNORECASE) is not None
        }

        try:
            if self.op in ['=', '!=', 'like', '>', '<', '>=', '<=']: # Explicitly list ops that need str conversion
                left_val, right_val = str(left_val), str(right_val)
        except Exception:
            # This is a defensive catch. If an object's __str__ method is buggy and raises
            # an error, we treat the comparison as False rather than crashing the whole query.
            return False

        return ops[self.op](left_val, right_val) if self.op in ops else False


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
        """Converts Quantities to floats for calculation."""
        if isinstance(value, FreeCAD.Units.Quantity):
            return value.Value
        elif isinstance(value, (int, float)):
            return value
        else:
            # If a non-numeric type is used, we must raise a clear error.
            # This prevents silent failures and confusing TypeErrors later on.
            type_name = type(value).__name__
            raise SqlEngineError(f"Cannot perform arithmetic on a non-numeric value of type '{type_name}'.")

    def get_value(self, obj):
        """Recursively evaluates the calculation tree."""
        left_val = self._normalize_value(self.left.get_value(obj))
        right_val = self._normalize_value(self.right.get_value(obj))

        if self.op == '+':
            return left_val + right_val
        if self.op == '-':
            return left_val - right_val
        if self.op == '*':
            return left_val * right_val
        if self.op == '/':
            # Basic division-by-zero protection.
            return left_val / right_val if right_val != 0 else float('inf')


class ReferenceExtractor:
    def __init__(self, value): self.value = value
    def get_value(self, obj): return _get_property(obj, self.value)
    def get_objects(self):
        """Provides a consistent interface for the FromClause."""
        if self.value == 'document':
            return FreeCAD.ActiveDocument.Objects
        return []


class StaticExtractor:
    def __init__(self, value): self.value = value
    def get_value(self, obj): return self.value


# --- Lark Transformer ---

class SqlTransformerMixin:
    """
    A mixin class containing all our custom transformation logic for SQL rules.
    It has no __init__ to avoid conflicts in a multiple inheritance scenario.
    """
    def start(self, i): return i[0]
    def statement(self, children):
        # The 'columns' rule produces a list of (extractor, display_name) tuples
        columns_info = next((c for c in children if c.__class__ == list), None)
        from_c = next((c for c in children if isinstance(c, FromClause)), None)
        where_c = next((c for c in children if isinstance(c, WhereClause)), None)
        group_by_c = next((c for c in children if isinstance(c, GroupByClause)), None)
        order_by_c = next((c for c in children if isinstance(c, OrderByClause)), None)

        return SelectStatement(columns_info, from_c, where_c, group_by_c, order_by_c)

    def from_clause(self, i):
        return FromClause(i[1])

    def from_source(self, items):
        return items[0]

    def where_clause(self, i): return WhereClause(i[1])

    def from_function(self, items):
        function_name_token, substatement = items[0], items[1]
        function_name = str(function_name_token).upper()
        function_class = self.from_function_registry.get_class(function_name)
        if not function_class:
            raise ValueError(f"Unknown FROM function: {function_name}")
        return function_class(substatement)

    def group_by_clause(self, items):
        references = [item for item in items if isinstance(item, ReferenceExtractor)]
        return GroupByClause(references)

    def order_by_clause(self, items):
        # items contains: ORDER, BY, reference, (",", reference)*, optional direction
        column_references = [item for item in items if isinstance(item, ReferenceExtractor)]
        direction = 'ASC'
        # The optional direction token will be the last item if it exists.
        last_item = items[-1]
        if isinstance(last_item, generated_sql_parser.Token) and last_item.type in ('ASC', 'DESC'):
            direction = last_item.value.upper()

        return OrderByClause(column_references, direction)

    def columns(self, items):
        # `items` is a list of results from `column` rules, which are (extractor, display_name) tuples
        return items

    def as_clause(self, items):
        # items[0]=AS token, items[1]=StaticExtractor from the 'literal' rule.
        # We must extract the raw string value from the extractor object.
        return items[1].get_value(None)

    def column(self, items):
        # Each item in `items` is either '*' (for SELECT *) or an extractor object.
        # We need to return a (extractor, display_name) tuple.
        extractor = items[0]
        alias = items[1] if len(items) > 1 else None

        # Determine the default display name first
        default_name = 'Unknown Column'
        if extractor == '*':
            default_name = SELECT_STAR_HEADER
        elif isinstance(extractor, ReferenceExtractor):
            default_name = extractor.value
        elif isinstance(extractor, StaticExtractor):
            default_name = str(extractor.get_value(None))
        elif isinstance(extractor, AggregateFunction):
            # Correctly handle the argument for default name generation.
            arg = extractor.argument
            arg_display = "?" # fallback
            if arg == '*':
                arg_display = '*'
            elif hasattr(arg, 'value'): # It's a ReferenceExtractor
                arg_display = arg.value
            default_name = f"{extractor.function_name.upper()}({arg_display})"
        elif isinstance(extractor, FunctionBase):
            # Create a nice representation for multi-arg functions
            arg_strings = []
            for arg_ex in extractor.arg_extractors:
                if arg_ex == '*':
                    arg_strings.append('*')
                elif isinstance(arg_ex, ReferenceExtractor):
                    arg_strings.append(arg_ex.value)
                elif isinstance(arg_ex, StaticExtractor):
                    arg_strings.append(f"'{arg_ex.get_value(None)}'")
                else:
                    arg_strings.append('?') # Fallback
            default_name = f"{extractor.function_name.upper()}({', '.join(arg_strings)})"

        # Use the alias if provided, otherwise fall back to the default name.
        final_name = alias if alias is not None else default_name
        return (extractor, final_name)

    def boolean_expression_recursive(self, items):
        return BooleanExpression(items[0], items[1].value.lower(), items[2])

    def boolean_expression(self, i): return BooleanExpression(i[0], None, None)
    def boolean_or(self, i): return i[0]
    def boolean_and(self, i): return i[0]
    def boolean_term(self, i): return i[0]

    def boolean_comparison(self, items):
        return BooleanComparison(items[0], items[1], items[2])

    def in_expression(self, items):
        reference_extractor = items[0]
        literal_extractors = [item for item in items[1:] if isinstance(item, StaticExtractor)]
        return InComparison(reference_extractor, literal_extractors)

    def comparison_operator(self, i): return i[0]
    def eq_op(self, _): return "="
    def neq_op(self, _): return "!="
    def like_op(self, _): return "like"
    def is_op(self, _): return "is"
    def is_not_op(self, _): return "is_not"
    def gt_op(self, _): return ">"
    def lt_op(self, _): return "<"
    def gte_op(self, _): return ">="
    def lte_op(self, _): return "<="

    def operand(self, items):
        # This method is now "dumb" and simply passes up the already-transformed object.
        # The transformation of terminals happens in their own dedicated methods below.
        return items[0]

    def reference(self, items): return ReferenceExtractor(str(items[0]))
    def literal(self, items): return StaticExtractor(items[0].value[1:-1])
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

    def _build_left_associative_tree(self, items):
        """
        A generic helper to build a left-associative calculation tree from a
        flat list of [operand, operator, operand, operator, ...].
        This is used by both the 'expr' and 'term' grammar rules.
        """
        tree = items[0]
        for i in range(1, len(items), 2):
            op_token = items[i]
            right = items[i+1]
            tree = ArithmeticOperation(tree, op_token.value, right)
        return tree

    # Both 'expr' and 'term' rules use the same tree-building logic.
    expr = _build_left_associative_tree
    term = _build_left_associative_tree

    def factor(self, items):
        # Handles parentheses by simply returning the transformed inner expression.
        return items[0]

    def function(self, items):
        function_name_token = items[0]
        function_name = str(function_name_token).upper()
        # Arguments are optional (e.g. for a future function).
        args = items[1] if len(items) > 1 else []

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
    This is the single source of truth for engine setup.
    """
    # 1. Create Registries
    select_function_registry = FunctionRegistry()
    from_function_registry = FunctionRegistry()

    # 2. Register all built-in function classes

    # SELECT Functions
    select_function_registry.register('COUNT', AggregateFunction, category="Aggregate", signature="COUNT(* | property)",
                                      description="Counts rows that match criteria.", snippet="SELECT COUNT(*) FROM document WHERE IfcType = 'Space'")
    select_function_registry.register('SUM', AggregateFunction, category="Aggregate", signature="SUM(property)",
                                      description="Calculates the sum of a numerical property.", snippet="SELECT SUM(Area) FROM document WHERE IfcType = 'Space'")
    select_function_registry.register('MIN', AggregateFunction, category="Aggregate", signature="MIN(property)",
                                      description="Finds the minimum value of a property.", snippet="SELECT MIN(Length) FROM document WHERE IfcType = 'Wall'")
    select_function_registry.register('MAX', AggregateFunction, category="Aggregate", signature="MAX(property)",
                                      description="Finds the maximum value of a property.", snippet="SELECT MAX(Height) FROM document WHERE IfcType = 'Wall'")
    select_function_registry.register('LOWER', LowerFunction, category="String", signature="LOWER(property)",
                                      description="Converts text to lowercase.", snippet="SELECT Label FROM document WHERE LOWER(Label) = 'exterior wall'")
    select_function_registry.register('UPPER', UpperFunction, category="String", signature="UPPER(property)",
                                      description="Converts text to uppercase.", snippet="SELECT Label FROM document WHERE UPPER(IfcType) = 'WALL'")
    select_function_registry.register('CONCAT', ConcatFunction, category="String", signature="CONCAT(value1, value2, ...)",
                                      description="Joins multiple strings and properties together.", snippet="SELECT CONCAT(Label, ': ', IfcType) FROM document")
    select_function_registry.register('TYPE', TypeFunction, category="Utility", signature="TYPE(*)",
                                      description="Returns the object's BIM type (e.g., 'Wall').", snippet="SELECT Label FROM document WHERE TYPE(*) = 'Wall'")
    select_function_registry.register('CONVERT', ConvertFunction, category="Utility", signature="CONVERT(quantity, 'unit')",
                                      description="Converts a Quantity to a different unit (e.g., CONVERT(Length, 'm')).", snippet="SELECT CONVERT(Length, 'm') AS LengthInMeters FROM document")
    # FROM Functions
    from_function_registry.register('CHILDREN', ChildrenFromFunction, category="Hierarchical", signature="CHILDREN(subquery)",
                                      description="Selects child objects of a given parent set.", snippet="SELECT * FROM CHILDREN(SELECT * FROM document WHERE Label = 'My Floor')")
    select_function_registry.register('PARENT', ParentFunction, category="Hierarchical", signature="PARENT(*)",
                                      description="Returns the immediate, architecturally significant parent of an object.", snippet="SELECT Label, PARENT(*).Label AS Floor FROM document WHERE IfcType = 'Space'")

    # 3. Define and instantiate the transformer
    class FinalTransformer(generated_sql_parser.Transformer, SqlTransformerMixin):
        def __init__(self, select_functions, from_functions):
            self.select_function_registry = select_functions
            self.from_function_registry = from_functions

    transformer = FinalTransformer(
        select_functions=select_function_registry,
        from_functions=from_function_registry
    )

    # 4. Instantiate the parser
    parser = generated_sql_parser.Lark_StandAlone()

    # 5. Generate friendly token names from the initialized parser
    friendly_token_names = _generate_friendly_token_names(parser)

    return parser, transformer, friendly_token_names


# --- Module-level Globals (initialized by the engine) ---
try:
    _parser, _transformer, _FRIENDLY_TOKEN_NAMES = _initialize_engine()
except Exception as e:
    _parser, _transformer, _FRIENDLY_TOKEN_NAMES = None, None, {}
    FreeCAD.Console.PrintError(f"BIM SQL engine failed to initialize: {e}\n")


# --- Internal API Functions ---

def _run_query(query_string: str, mode: str):
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
            raise SqlEngineError("BIM SQL engine is not initialized. Check console for errors on startup.")
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
            is_incomplete = e.token.type == '$END'
            expected_str = ', '.join([_FRIENDLY_TOKEN_NAMES.get(t, f"'{t}'") for t in e.expected])
            message = (f"Syntax Error: Unexpected '{e.token.value}' at line {e.line}, column {e.column}. "
                       f"Expected {expected_str}.")
            raise BimSqlSyntaxError(message, is_incomplete=is_incomplete) from e
        except UnexpectedEOF as e:
            raise BimSqlSyntaxError("Query is incomplete.", is_incomplete=True) from e

    statement = _parse_and_transform(query_string)

    if mode == 'count_only':
        # Phase 1: Perform the fast filtering and grouping to get the
        # correct final row count.
        grouped_data = statement._get_grouped_data()
        row_count = len(grouped_data)

        # If there are no results, the query is valid and simply returns 0 rows.
        if row_count == 0:
            return 0

        # Phase 2 Validation: Perform a "sample execution" on the first group
        # to validate the SELECT clause and catch any execution-time errors.
        # We only care if it runs without error; the result is discarded.
        first_group_sample = grouped_data[0]
        _ = statement._process_select_columns([first_group_sample])

        # If the sample execution succeeds, the query is fully valid.
        return row_count
    else: # 'full_data'
        return statement.execute()


# --- Public API Functions ---

def count(query_string: str) -> Tuple[int, Optional[str]]:
    """
    Executes a query and returns only the count of resulting rows.

    This is a "safe" public API function intended for UI components like
    live validation feedback. It catches all exceptions and returns a
    consistent tuple, making it safe to call with incomplete user input.

    Parameters
    ----------
    query_string : str
        The raw SQL query string.

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
        count_result = _run_query(query_string, mode='count_only')
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
    # The 'select' function always performs a full data query.
    headers, results_data = _run_query(query_string, mode='full_data')
    return headers, results_data


def getSqlKeywords() -> List[str]:
    """
    Returns a list of all keywords and function names for syntax highlighters.

    This function provides the single source of truth for SQL syntax in the
    BIM workbench. It dynamically introspects the initialized Lark parser and
    the function registries.

    Returns
    -------
    List[str]
        A sorted, unique list of all keywords and function names (e.g.,
        ['SELECT', 'FROM', 'COUNT', 'CHILDREN', ...]). Returns an empty list
        if the engine failed to initialize.
    """
    # The parser and transformer are initialized at the module level.
    # We just check if the initialization was successful.
    if not _parser or not _transformer:
        return []

    keywords = []
    # This blocklist contains all uppercase terminals from the grammar that are NOT
    # actual keywords a user would type. This provides a robust filter.
    NON_KEYWORD_TERMINALS = {
        'WS', 'CNAME', 'STRING', 'NUMBER', 'LPAR', 'RPAR', 'COMMA', 'ASTERISK'
    }

    # 1. Get all keywords from the parser's terminals.
    # A terminal is a keyword if its name is uppercase and not in our blocklist.
    for term in _parser.terminals:
        # Filter out internal, anonymous tokens generated by Lark.
        is_internal = term.name.startswith('__') # Filter out internal __ANON tokens
        if term.name.isupper() and term.name not in NON_KEYWORD_TERMINALS and not is_internal:
            keywords.append(term.name)

    # 2. Get all registered function names (e.g., COUNT, CONCAT, CHILDREN) by accessing them through
    #    the initialized transformer.
    keywords.extend(list(_transformer.select_function_registry._functions.keys()))
    keywords.extend(list(_transformer.from_function_registry._functions.keys()))

    return sorted(list(set(keywords))) # Return a sorted, unique list.


def getSqlApiDocumentation() -> dict:
    """
    Generates a structured dictionary describing the supported SQL dialect.

    This function is fully dynamic and introspects the live engine
    configuration. All user-facing strings are pre-translated.

    Returns
    -------
    dict
        A dictionary with keys 'clauses' and 'functions'. 'functions' is a
        dict of lists, categorized by function type.
    """
    api_data = {
        'clauses': [],
        'functions': {}
    }

    # Combine all function registries into one for easier iteration.
    all_registries = {
        **_transformer.select_function_registry._functions,
        **_transformer.from_function_registry._functions
    }

    # Group functions by their registered category.
    for name, data in all_registries.items():
        category = data['category']
        if category not in api_data['functions']:
            api_data['functions'][category] = []
        api_data['functions'][category].append({
            'name': name,
            'signature': data['signature'],
            'description': data['description'],
            'snippet': data['snippet'],
        })

    # To get a clean list of "Clauses" for the cheatsheet, we use an explicit
    # whitelist of keywords that represent major SQL clauses. This is more
    # robust and clearer than trying to filter out all non-clause tokens.
    CLAUSE_KEYWORDS = {
        'SELECT', 'FROM', 'WHERE', 'GROUP', 'BY', 'ORDER', 'AS', 'AND', 'OR', 'IS', 'NOT', 'IN', 'LIKE'
    }
    # We still get all terminals from the parser to ensure our list is valid.
    all_terminals = {term.name for term in _parser.terminals}
    api_data['clauses'] = sorted([k for k in CLAUSE_KEYWORDS if k in all_terminals])

    return api_data