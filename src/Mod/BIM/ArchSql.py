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

# Import exception types from the generated parser for robust, type-safe handling.
try:
    from generated_sql_parser import UnexpectedEOF, UnexpectedToken
except ImportError:
    # Provide a dummy class if the parser hasn't been generated yet.
    class UnexpectedEOF(Exception): pass
    class UnexpectedToken(Exception): pass

# Global variables to cache the parser and transformer for performance
_parser = None
_transformer = None

def get_property(obj, prop_name):
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

# Query Object Model
class GroupByClause:
    """Represents the GROUP BY clause of a SQL statement."""
    def __init__(self, columns):
        # columns is a list of ReferenceExtractor objects
        self.columns = columns

class AggregateFunction:
    """Represents an aggregate function call like COUNT(*) or SUM(Height)."""
    def __init__(self, function_name, argument):
        self.function_name = function_name.lower()
        # argument can be the string "*" or another extractor (e.g., ReferenceExtractor)
        self.argument = argument

    def get_value(self, obj):
        # This method is a placeholder. The actual calculation will happen
        # during the grouped execution phase, not on a single object.
        return None

class SelectStatement:
    def __init__(self, columns_info, from_clause, where_clause, group_by_clause):
        self.columns_info = columns_info # Stores (extractor_object, display_name) tuples
        self.from_clause = from_clause
        self.where_clause = where_clause
        self.group_by_clause = group_by_clause

    def execute(self):
        # 1. Get and filter the initial object list from the document
        all_objects = self.from_clause.get_objects()
        filtered_objects = [o for o in all_objects if self.where_clause is None or self.where_clause.matches(o)]

        # 2. Determine the column headers from the parsed statement
        headers = [display_name for _, display_name in self.columns_info]

        # 3. Decide which execution path to take based on the query structure
        if self.group_by_clause:
            results_data = self._execute_grouped_query(filtered_objects)
        else:
            results_data = self._execute_non_grouped_query(filtered_objects)

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
                if isinstance(extractor, (AggregateFunction, StaticExtractor)):
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
        # A non-aggregate is a ReferenceExtractor. StaticExtractors are allowed.
        has_non_aggregate_reference = any(isinstance(ex, ReferenceExtractor) for ex, _ in self.columns_info)

        if has_aggregate and has_non_aggregate_reference:
            raise ValueError(
                "Cannot mix aggregate functions and object properties (e.g., 'Label') "
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
                            count = sum(1 for obj in object_list if get_property(obj, prop_name) is not None)
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

                else:
                    # This must be a column from the GROUP BY clause. We find which part
                    # of the key corresponds to this column.
                    key_index = -1
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

        # Check if this is a query with only aggregate functions (e.g., SELECT COUNT(*))
        is_aggregate_query = any(isinstance(ex, AggregateFunction) for ex, _ in self.columns_info)

        if is_aggregate_query:
            # An aggregate query without a GROUP BY always returns a single row.
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
                            count = sum(1 for obj in objects if get_property(obj, prop_name) is not None)
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

class FromClause:
    def __init__(self, reference):
        self.reference = reference
    def get_objects(self):
        if self.reference.value == 'document':
            return FreeCAD.ActiveDocument.Objects
        return []

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
        if self.op is None: return self.left.evaluate(obj)
        if self.op == 'and': return self.left.evaluate(obj) and self.right.evaluate(obj)
        if self.op == 'or': return self.left.evaluate(obj) or self.right.evaluate(obj)

class BooleanComparison:
    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right
    def evaluate(self, obj):
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
        except: return False

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

class ReferenceExtractor:
    def __init__(self, value): self.value = value
    def get_value(self, obj): return get_property(obj, self.value)

class StaticExtractor:
    def __init__(self, value): self.value = value
    def get_value(self, obj): return self.value

# Lark Transformer (with no runtime dependency on the the lark library)
class SqlTreeTransformer:
    def start(self, i): return i[0]
    def statement(self, children):
        # The 'columns' rule produces a list of (extractor, display_name) tuples
        columns_info = next((c for c in children if c.__class__ == list), None)
        from_c = next((c for c in children if isinstance(c, FromClause)), None)
        where_c = next((c for c in children if isinstance(c, WhereClause)), None)
        group_by_c = next((c for c in children if isinstance(c, GroupByClause)), None)

        return SelectStatement(columns_info, from_c, where_c, group_by_c)

    def from_clause(self, i): return FromClause(i[1])
    def where_clause(self, i): return WhereClause(i[1])

    def group_by_clause(self, items):
        references = [item for item in items if isinstance(item, ReferenceExtractor)]
        return GroupByClause(references)

    # --- Columns Transformer ---
    def columns(self, items):
        # `items` is a list of results from `column` rules, which are (extractor, display_name) tuples
        return items

    def column(self, items):
        # Each item in `items` is either '*' (for SELECT *) or an extractor object.
        # We need to return a (extractor, display_name) tuple.
        extractor = items[0]
        if extractor == '*':
            return ('*', 'Object Label') # Default label for SELECT *
        elif isinstance(extractor, ReferenceExtractor):
            return (extractor, extractor.value) # Use property name as display name
        elif isinstance(extractor, StaticExtractor):
            # For static values, use their string representation as display name
            return (extractor, str(extractor.get_value(None)))
        elif isinstance(extractor, AggregateFunction):
            arg_display = "*" if extractor.argument == "*" else extractor.argument.value
            display_name = f"{extractor.function_name.upper()}({arg_display})"
            return (extractor, display_name)
        else: # Fallback, should not happen with current grammar
            return (extractor, "Unknown Column")
    # --- END Columns Transformer ---

    def asterisk(self, _): return "*"

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

    def operand(self, i): return i[0]
    def reference(self, items): return ReferenceExtractor(str(items[0]))
    def literal(self, items): return StaticExtractor(items[0].value[1:-1])
    def null(self, _): return StaticExtractor(None)

    def function(self, items):
        function_name, argument = str(items[0]), items[1]
        return AggregateFunction(function_name, argument)


def _get_query_object(query_string):
    """
    Internal function to parse and transform a query string.
    """
    global _parser, _transformer
    if _parser is None:
        try:
            import generated_sql_parser

            class FinalTransformer(generated_sql_parser.Transformer, SqlTreeTransformer):
                pass

            _parser = generated_sql_parser.Lark_StandAlone()
            _transformer = FinalTransformer()
        except ImportError:
            return None, "Parser not generated. Please rebuild FreeCAD."
        except Exception as e:
            return None, f"Initialization Error: {e}"

    try:
        tree = _parser.parse(query_string)
        statement = _transformer.transform(tree)
        statement.validate()
        return statement, None
    except ValueError as e:
        # Catch validation errors from the validate() method.
        return None, str(e)
    except Exception as e:
        return None, e

def run_query_for_count(query_string):
    """Public interface for the Task Panel to get a result count."""

    if (query_string.count("'") % 2 != 0) or (query_string.count('"') % 2 != 0):
        return -1, "INCOMPLETE"

    statement, error = _get_query_object(query_string)
    if error:
        is_incomplete = isinstance(error, UnexpectedEOF) or \
                        (isinstance(error, UnexpectedToken) and error.token.type == '$END')

        if is_incomplete:
            return -1, "INCOMPLETE"

        return -1, f"Syntax Error: Invalid token near '{error.token}'" if isinstance(error, UnexpectedToken) else "Syntax Error"
    try:
        # The execute method returns (headers, data_rows). We only need count.
        headers, results_data = statement.execute()
        return len(results_data), None
    except Exception as e:
        return -1, f"Execution Error: {e}"

def run_query_for_objects(query_string):
    """Public interface for the Report object to get the resulting objects (headers, data_rows)."""
    statement, error = _get_query_object(query_string)
    if error:
        if isinstance(error, str):
            FreeCAD.Console.PrintError(f"BIM Report Validation Error: {error}\n")
        else:
            FreeCAD.Console.PrintError(f"BIM Report Execution Error: A {type(error).__name__} occurred.\n")
        return [], [] # Return empty headers and data on error.

    headers, results_data = statement.execute()
    return headers, results_data
