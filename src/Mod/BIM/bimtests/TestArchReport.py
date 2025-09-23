# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

"""Unit tests for the ArchReport and ArchSql modules."""

import os
from unittest.mock import patch
import FreeCAD
import Arch
import ArchReport
import ArchSql
from bimtests import TestArchBase


class TestArchReport(TestArchBase.TestArchBase):

    def setUp(self):
        super().setUp()
        self.doc = self.document

        self.wall_ext = Arch.makeWall(length=1000, name="Exterior Wall")
        self.wall_ext.IfcType = "Wall"
        self.wall_ext.Height = FreeCAD.Units.Quantity(3000, "mm") # Store as Quantity for robust comparison

        self.wall_int = Arch.makeWall(length=500, name="Interior partition wall")
        self.wall_int.IfcType = "Wall"
        self.wall_int.Height = FreeCAD.Units.Quantity(2500, "mm") # Store as Quantity

        self.column = Arch.makeStructure(length=300, width=330, height=2000, name="Main Column")
        self.column.IfcType = "Column"

        self.beam = Arch.makeStructure(length=2000, width=200, height=400, name="Main Beam")
        self.beam.IfcType = "Beam"

        self.window = Arch.makeWindow(name="Living Room Window")
        self.window.IfcType = "Window"

        self.part_box = self.doc.addObject("Part::Box", "Generic Box") # This object has no IfcType property

        # Define a clean list of *only* the objects created by the test setUp
        self.test_objects_in_doc = [
            self.wall_ext, self.wall_int, self.column, self.beam, self.window, self.part_box
        ]
        self.test_object_labels = sorted([o.Label for o in self.test_objects_in_doc])

        # We create the spreadsheet here, but it's part of the test setup, not a queryable object
        self.spreadsheet = self.doc.addObject("Spreadsheet::Sheet", "ReportTarget")
        self.doc.recompute()


    def _run_query_for_objects(self, query_string):
        """
        Helper method to encapsulate the report creation and execution workflow.
        Returns the (headers, data_rows) tuple found by the query.
        This helper ensures the returned data format is consistent for tests.

        Note: For SELECT *, it returns (['Object Label'], list_of_labels) for easier assertion.
        For specific columns, it returns (headers, list_of_lists_of_values).
        """
        report = Arch.makeReport()
        report.Target = self.spreadsheet

        # Ensure the proxy live statements reflect persisted data
        if hasattr(report, 'Proxy'):
            try:
                report.Proxy.hydrate_live_statements(report)
            except Exception:
                # Be permissive in tests; hydration should normally succeed
                pass

        # Ensure the first statement exists and holds the query string. We
        # must operate on the proxy's live_statements (runtime objects). If
        # none exist, create a persisted dict and hydrate again.
        if not hasattr(report, 'Statements') or not report.Statements:
            report.Statements = [ArchReport.ReportStatement(description="Statement 1", query_string=query_string).dumps()]
            if hasattr(report, 'Proxy'):
                report.Proxy.hydrate_live_statements(report)
        else:
            if hasattr(report, 'Proxy') and getattr(report.Proxy, 'live_statements', None):
                report.Proxy.live_statements[0].query_string = query_string
            else:
                # Fallback: replace persisted dict with one containing the query
                report.Statements = [ArchReport.ReportStatement(description="Statement 1", query_string=query_string).dumps()]

        # Persist the change through the proxy if available
        if hasattr(report, 'Proxy'):
            try:
                report.Proxy.commit_statements()
            except Exception:
                # Tests should continue even if commit is not possible in this env
                pass

        self.doc.recompute()

        headers, results_data_from_sql = Arch.select(query_string)

        self.assertIsInstance(headers, list, f"Headers should be a list for: {query_string}")
        self.assertIsInstance(results_data_from_sql, list, f"Results data should be a list for: {query_string}")

        # For aggregate queries (e.g., containing COUNT, GROUP BY), the results are summaries,
        # not direct object properties. The filtering logic below is incorrect for them.
        # We can detect an aggregate query if the headers contain typical aggregate function names.
        is_aggregate_query  = any(agg in h for h in headers for agg in ['COUNT', 'SUM', 'MIN', 'MAX'])
        if is_aggregate_query:
            return headers, results_data_from_sql

        # If SELECT *, results_data_from_sql is list of lists, e.g., [['Exterior Wall'], ...].
        # Extract flat list of labels.
        if headers == ['Object Label']:
            extracted_labels = [row[0] for row in results_data_from_sql]
            # Filter against our defined test objects only for the assert.
            filtered_labels = [label for label in extracted_labels if label in self.test_object_labels]
            return headers, filtered_labels # Return (headers, list_of_labels) for SELECT *

        # For specific column selections, results_data_from_sql is list of lists of values.
        # Filter these rows based on whether their first element (label) is one of our test objects.
        # This assumes the first column is always the object's label for non-aggregate queries.
        filtered_results_for_specific_columns = []
        for row in results_data_from_sql:
            if len(row) > 0 and row[0] in self.test_object_labels:
                filtered_results_for_specific_columns.append(row)

        return headers, filtered_results_for_specific_columns

    def _find_preset_file(self, filename):
        """
        Finds a test resource file by searching in the build tree first,
        then falling back to the final install directory. This makes the
        test robust for both development and post-install testing.
        """
        # Path for running tests from a standard build directory (most common)
        # This navigates from .../bimtests/ up to the BIM module root
        # Note: A self-correction was made here from a previous version.
        # The path is relative to the *test file's* location.
        build_path = os.path.join(os.path.dirname(__file__), "..", "Presets", "ArchReport", filename)
        if os.path.exists(build_path):
            return build_path

        # Fallback to the final installed resource directory
        install_path = os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "ArchReport", filename)
        if os.path.exists(install_path):
            return install_path

        # If neither is found, the test will fail below.
        return None

    # Category 1: Basic Object Creation and Validation
    def test_makeReport_default(self):
        report = Arch.makeReport()
        self.assertIsNotNone(report, "makeReport failed to create an object.")
        self.assertEqual(report.Label, "Report", "Default report label is incorrect.")

    def test_report_properties(self):
        report = Arch.makeReport()
        self.assertTrue(hasattr(report, "Statements"), "Report object is missing 'Statements' property.")
        self.assertTrue(hasattr(report, "Target"), "Report object is missing 'Target' property.")

    # Category 2: Core SELECT Functionality
    def test_select_all_from_document(self):
        """Test a 'SELECT * FROM document' query."""
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document')

        self.assertEqual(headers, ['Object Label'])
        self.assertCountEqual(results_labels, self.test_object_labels, "Should find all queryable objects.")

    def test_select_specific_columns_from_document(self):
        """Test a 'SELECT Label, IfcType, Height FROM document' query."""
        query_string = 'SELECT Label, IfcType, Height FROM document WHERE IfcType = "Wall"'
        headers, results_data = self._run_query_for_objects(query_string)

        self.assertEqual(headers, ['Label', 'IfcType', 'Height'])
        self.assertEqual(len(results_data), 2)

        expected_rows = [
            ["Exterior Wall", "Wall", self.wall_ext.Height],
            ["Interior partition wall", "Wall", self.wall_int.Height],
        ]
        self.assertCountEqual(results_data, expected_rows, "Specific column data mismatch.")

    # Category 3: WHERE Clause Filtering
    def test_where_equals_string(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType = "Wall"')
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.wall_ext.Label, self.wall_int.Label])

    def test_where_not_equals_string(self):
        """Test a WHERE clause with a not-equals check."""
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType != "Wall"')
        # Strict SQL semantics: comparisons with NULL are treated as UNKNOWN
        # and therefore excluded. Use IS NULL / IS NOT NULL to test for nulls.
        expected_labels = [self.column.Label, self.beam.Label, self.window.Label]
        self.assertEqual(len(results_labels), 3)
        self.assertCountEqual(results_labels, expected_labels)

    def test_where_is_null(self):
        """Test a WHERE clause with an IS NULL check."""
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NULL')
        # --- FIX: Corrected expected labels for IS NULL ---
        # This now expects only self.part_box as it's the only one in self.test_objects_in_doc with IfcType=None.
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.part_box.Label)

    def test_where_is_not_null(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NOT NULL')
        self.assertEqual(len(results_labels), 5)
        self.assertNotIn(self.part_box.Label, results_labels)

    def test_where_like_case_insensitive(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "exterior wall"')
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_like_wildcard_middle(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "%wall%"')
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.wall_ext.Label, self.wall_int.Label])

    def test_null_equality_is_excluded(self):
        """Strict SQL: comparisons with NULL should be excluded; use IS NULL."""
        headers, results = self._run_query_for_objects('SELECT * FROM document WHERE IfcType = NULL')
        # '=' with NULL should not match (UNKNOWN -> excluded)
        self.assertEqual(len(results), 0)

    def test_null_inequality_excludes_nulls(self):
        """Strict SQL: IfcType != 'Wall' should exclude rows where IfcType is NULL."""
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType != "Wall"')
        expected_labels = [self.column.Label, self.beam.Label, self.window.Label]
        self.assertCountEqual(results_labels, expected_labels)

    def test_is_null_and_is_not_null_behaviour(self):
        headers, isnull_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NULL')
        self.assertIn(self.part_box.Label, isnull_labels)

        headers, isnotnull_labels = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NOT NULL')
        self.assertNotIn(self.part_box.Label, isnotnull_labels)

    def test_where_like_wildcard_end(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "Exterior%"')
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_boolean_and(self):
        query = 'SELECT * FROM document WHERE IfcType = "Wall" AND Label LIKE "%Exterior%"'
        headers, results_labels = self._run_query_for_objects(query)
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_boolean_or(self):
        query = 'SELECT * FROM document WHERE IfcType = "Window" OR IfcType = "Column"'
        headers, results_labels = self._run_query_for_objects(query)
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.window.Label, self.column.Label])

    # Category 4: Edge Cases and Error Handling
    def test_query_no_results(self):
        headers, results_labels = self._run_query_for_objects('SELECT * FROM document WHERE Label = "NonExistentObject"')
        self.assertEqual(len(results_labels), 0)

    @patch('FreeCAD.Console.PrintError')
    def test_query_invalid_syntax(self, mock_print_error):
        # The low-level function now raises an exception on failure.
        with self.assertRaises(Arch.BimSqlSyntaxError) as cm:
            ArchSql._get_query_object('SELECT FROM document WHERE')
        self.assertFalse(cm.exception.is_incomplete, "A syntax error should not be marked as incomplete.")

        # The high-level function for the UI catches it and returns a simple string.
        count, error_str = Arch.count('SELECT FROM document WHERE')
        self.assertEqual(count, -1)
        self.assertIsInstance(error_str, str)
        self.assertIn('Syntax Error', error_str)

    def test_incomplete_queries_are_handled_gracefully(self):
        incomplete_queries = [
            "SELECT",
            "SELECT *",
            "SELECT * FROM",
            "SELECT * FROM document WHERE",
            "SELECT * FROM document WHERE Label =",
            "SELECT * FROM document WHERE Label LIKE",
            "SELECT * FROM document WHERE Label like \"%wa", # Test case for incomplete string literal
        ]

        for query in incomplete_queries:
            with self.subTest(query=query):
                count, error = Arch.count(query)
                self.assertEqual(error, "INCOMPLETE", f"Query '{query}' should be marked as INCOMPLETE.")

    def test_invalid_partial_tokens_are_errors(self):
        invalid_queries = {
            "Partial keyword": "SELEC",
            "Mistyped keyword": "SELECT * FRM document",
        }

        for name, query in invalid_queries.items():
            with self.subTest(name=name, query=query):
                count, error = Arch.count(query)
                self.assertNotEqual(error, "INCOMPLETE", f"Query '{query}' should be a syntax error, not incomplete.")
                self.assertIsNotNone(error, f"Query '{query}' should have returned an error.")

    def test_report_no_target(self):
        try:
            report = Arch.makeReport()
            # Creation initializes a target spreadsheet; verify it's set
            self.assertIsNotNone(report.Target, "Report Target should be set on creation.")
            # Set the first statement's query string
            # Prefer operating on the proxy runtime objects when available
            if hasattr(report, 'Proxy'):
                # Ensure live statements are hydrated from persisted storage
                report.Proxy.hydrate_live_statements(report)

                if not getattr(report.Proxy, 'live_statements', None):
                    # No live statements: create a persisted starter and hydrate again
                    report.Statements = [ArchReport.ReportStatement(description="Statement 1", query_string='SELECT * FROM document').dumps()]
                    report.Proxy.hydrate_live_statements(report)
                else:
                    report.Proxy.live_statements[0].query_string = 'SELECT * FROM document'
                    report.Proxy.commit_statements()
            else:
                # Fallback for environments without a proxy: persist a dict
                if not hasattr(report, 'Statements') or not report.Statements:
                    report.Statements = [ArchReport.ReportStatement(description="Statement 1", query_string='SELECT * FROM document').dumps()]
                else:
                        # Persist a fresh statement dict in the fallback path
                        report.Statements = [ArchReport.ReportStatement(description="Statement 1", query_string='SELECT * FROM document').dumps()]
            self.doc.recompute()
        except Exception as e:
            self.fail(f"Recomputing a report with no Target raised an unexpected exception: {e}")

        # UX: when the report runs without a pre-set Target, it should create
        # a spreadsheet, set the sheet.ReportName, and persist the Target link
        # so subsequent runs are deterministic.
        self.assertIsNotNone(report.Target, "Report Target should be set after running with no pre-existing Target.")
        self.assertEqual(getattr(report.Target, 'ReportName', None), report.Name)

    def test_group_by_ifctype_with_count(self):
        """Test GROUP BY with COUNT(*) to summarize objects by type."""
        # Add a WHERE clause to exclude test scaffolding objects from the count.
        query = "SELECT IfcType, COUNT(*) FROM document " \
                "WHERE TypeId != 'App::FeaturePython' AND TypeId != 'Spreadsheet::Sheet' " \
                "GROUP BY IfcType"
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ['IfcType', 'COUNT(*)'])

        # Convert results to a dict for easy, order-independent comparison.
        # Handle the case where IfcType is None, which becomes a key in the dict.
        results_dict = {row[0] if row[0] != 'None' else None: int(row[1]) for row in results_data}

        expected_counts = {
            "Wall": 2,
            "Column": 1,
            "Beam": 1,
            "Window": 1,
            None: 1  # The Part::Box has a NULL IfcType, which forms its own group
        }
        self.assertDictEqual(results_dict, expected_counts, "The object counts per IfcType are incorrect.")

    def test_count_all_without_group_by(self):
        """Test COUNT(*) on the whole dataset without grouping."""
        # Add a WHERE clause to exclude test scaffolding objects from the count.
        query = "SELECT COUNT(*) FROM document " \
                "WHERE TypeId != 'App::FeaturePython' AND TypeId != 'Spreadsheet::Sheet'"
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ['COUNT(*)'])
        self.assertEqual(len(results_data), 1, "Non-grouped aggregate should return a single row.")
        self.assertEqual(int(results_data[0][0]), len(self.test_objects_in_doc), "COUNT(*) did not return the total number of test objects.")

    def test_group_by_with_sum(self):
        """Test GROUP BY with SUM() on a numeric property."""
        # Note: We filter for objects that are likely to have a Height property to get a clean sum.
        query = "SELECT IfcType, SUM(Height) FROM document " \
                "WHERE IfcType = 'Wall' OR IfcType = 'Column' " \
                "GROUP BY IfcType"
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ['IfcType', 'SUM(Height)'])
        results_dict = {row[0]: float(row[1]) for row in results_data}

        # Expected sums:
        # Walls: Exterior (3000) + Interior (2500) = 5500
        # Columns: Main Column (2000)
        expected_sums = {
            "Wall": 5500.0,
            "Column": 2000.0,
        }
        self.assertDictEqual(results_dict, expected_sums)
        self.assertNotIn("Window", results_dict, "Groups excluded by WHERE should not appear.")

    def test_min_and_max_functions(self):
        """Test MIN() and MAX() functions on a numeric property."""
        query = "SELECT MIN(Length), MAX(Length) FROM document WHERE IfcType = 'Wall'"
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ['MIN(Length)', 'MAX(Length)'])
        self.assertEqual(len(results_data), 1, "Aggregate query without GROUP BY should return one row.")

        # Expected: Interior wall is 500, Exterior wall is 1000
        min_length = float(results_data[0][0])
        max_length = float(results_data[0][1])

        self.assertAlmostEqual(min_length, 500.0)
        self.assertAlmostEqual(max_length, 1000.0)

    def test_invalid_group_by_raises_error(self):
        """A SELECT column not in GROUP BY and not in an aggregate should fail validation."""
        # 'Label' is not aggregated and not in the 'GROUP BY' clause, making this query invalid.
        query = 'SELECT Label, COUNT(*) FROM document GROUP BY IfcType'

        # _get_query_object should raise an exception for validation errors.
        with self.assertRaises(ArchSql.SqlEngineError) as cm:
            ArchSql._get_query_object(query)

        # Check for the specific, user-friendly error message within the exception.
        self.assertIn("must appear in the GROUP BY clause", str(cm.exception),
                      "The validation error message is not descriptive enough.")

    def test_non_grouped_sum_calculates_correctly(self):
        """
        Tests the SUM() aggregate function without a GROUP BY clause in isolation.
        This test calls the SQL engine directly to ensure the summing logic is correct.
        """
        # The query sums the Height of the two wall objects created in setUp().
        # Expected result: 3000mm + 2500mm = 5500mm.
        query = "SELECT SUM(Height) FROM document WHERE IfcType = 'Wall'"

        # We call the engine directly, bypassing the _run_query_for_objects helper.
        headers, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. An aggregate query without a GROUP BY should always return exactly one row.
        self.assertEqual(len(results_data), 1, "A non-grouped aggregate query should return exactly one row.")

        # 2. The result in that row should be the correct sum.
        actual_sum = float(results_data[0][0])
        expected_sum = 5500.0
        self.assertAlmostEqual(
            actual_sum,
            expected_sum,
            "The SUM() result is incorrect. The engine is not accumulating the values correctly."
        )

    def test_non_grouped_query_with_mixed_extractors(self):
        """
        Tests a non-grouped query with both a static value and a SUM() aggregate.
        """
        query = "SELECT 'Total Height', SUM(Height) FROM document WHERE IfcType = 'Wall'"

        # We call the engine directly to isolate its behavior.
        headers, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. The query should still return exactly one row.
        self.assertEqual(len(results_data), 1, "A non-grouped mixed query should return exactly one row.")

        # 2. Check the content of the single row.
        #    The first column should be the static string.
        self.assertEqual(results_data[0][0], "Total Height")
        #    The second column should be the correct sum (3000 + 2500 = 5500).
        actual_sum = float(results_data[0][1])
        expected_sum = 5500.0
        self.assertAlmostEqual(
            actual_sum,
            expected_sum,
            "The SUM() result in a mixed non-grouped query is incorrect."
        )

    def test_sum_of_space_area_is_correct_and_returns_float(self):
        """
        Tests that SUM() on the 'Area' property of Arch.Space objects
        returns the correct numerical sum as a float.
        """
        # --- Test Setup: Create two Arch.Space objects with discernible areas ---

        # Space 1: Base is a 1000x2000 box, resulting in 2,000,000 mm^2 floor area
        base_box1 = self.doc.addObject("Part::Box", "BaseBox1")
        base_box1.Length = 1000
        base_box1.Width = 2000
        _ = Arch.makeSpace(base_box1, name="Office")

        # Space 2: Base is a 3000x1500 box, resulting in 4,500,000 mm^2 floor area
        base_box2 = self.doc.addObject("Part::Box", "BaseBox2")
        base_box2.Length = 3000
        base_box2.Width = 1500
        _ = Arch.makeSpace(base_box2, name="Workshop")

        self.doc.recompute() # Ensure space areas are calculated

        query = "SELECT SUM(Area) FROM document WHERE IfcType = 'Space'"

        # Call the engine directly to isolate its behavior
        headers, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. An aggregate query should return exactly one row.
        self.assertEqual(len(results_data), 1, "A non-grouped aggregate query should return exactly one row.")

        # 2. The result in the row should be a float. This verifies the engine's
        #    design to return raw numbers for aggregates.
        self.assertIsInstance(results_data[0][0], float, "The result of a SUM() should be a float.")

        # 3. The value of the float should be the correct sum.
        actual_sum = results_data[0][0]
        expected_sum = 6500000.0  # 2,000,000 + 4,500,000

        self.assertAlmostEqual(
            actual_sum,
            expected_sum,
            "The SUM(Area) for Space objects is incorrect."
        )

    def test_min_and_max_aggregates(self):
        """
        Tests the MIN() and MAX() aggregate functions on a numeric property.
        """
        # Note: The test setup already includes two walls with different lengths.
        # Exterior Wall: Length = 1000mm
        # Interior Wall: Length = 500mm
        query = "SELECT MIN(Length), MAX(Length) FROM document WHERE IfcType = 'Wall'"

        headers, results_data = Arch.select(query)

        self.assertEqual(len(results_data), 1, "Aggregate query should return a single row.")
        self.assertIsInstance(results_data[0][0], float, "MIN() should return a float.")
        self.assertIsInstance(results_data[0][1], float, "MAX() should return a float.")

        min_length = results_data[0][0]
        max_length = results_data[0][1]

        self.assertAlmostEqual(min_length, 500.0)
        self.assertAlmostEqual(max_length, 1000.0)

    def test_count_property_vs_count_star(self):
        """
        Tests that COUNT(property) correctly counts only non-null values,
        while COUNT(*) counts all rows.
        """
        # --- Test Setup ---
        # Use a unique property name that is guaranteed not to exist on any other object.
        # This ensures the test is perfectly isolated.
        unique_prop_name = "TestSpecificTag"

        # Add the unique property to exactly two objects.
        self.wall_ext.addProperty("App::PropertyString", unique_prop_name, "BIM")
        setattr(self.wall_ext, unique_prop_name, "Exterior")

        self.column.addProperty("App::PropertyString", unique_prop_name, "BIM")
        setattr(self.column, unique_prop_name, "Structural")

        self.doc.recompute()

        # --- Test COUNT(TestSpecificTag) ---
        # This query should now only find the two objects we explicitly modified.
        query_count_prop = f"SELECT COUNT({unique_prop_name}) FROM document"
        headers_prop, results_prop = Arch.select(query_count_prop)
        self.assertEqual(int(results_prop[0][0]), 2,
                         f"COUNT({unique_prop_name}) should count exactly the 2 objects where the property was added.")

        # --- Test COUNT(*) ---
        # Build the WHERE clause dynamically from the actual object labels.
        # This is the most robust way to ensure the test is correct and not
        # dependent on FreeCAD's internal naming schemes.
        labels_to_count = [
            self.wall_ext.Label,
            self.wall_int.Label,
            self.column.Label,
            self.beam.Label,
            self.window.Label,
            self.part_box.Label
        ]

        # Create a chain of "Label = '...'" conditions
        where_conditions = " OR ".join([f"Label = '{label}'" for label in labels_to_count])
        query_count_star = f"SELECT COUNT(*) FROM document WHERE {where_conditions}"

        headers_star, results_star = Arch.select(query_count_star)
        self.assertEqual(int(results_star[0][0]), 6, "COUNT(*) should count all 6 test objects.")

    def test_bundled_report_templates_are_valid(self):
        """
        Performs an integration test to ensure all bundled report templates
        can be parsed and executed without errors against a sample model.
        """
        import os
        import json

        # Find the bundled templates file using the helper
        template_path = self._find_preset_file("report_templates.json")
        self.assertIsNotNone(template_path, "Bundled report_templates.json not found in build or install directories. Check CMakeLists.txt.")

        with open(template_path, 'r', encoding='utf8') as f:
            templates = json.load(f)

        self.assertIn("Room and Area Schedule", templates)
        self.assertIn("Wall Quantities", templates)

        # Execute every query in every statement of every template
        for template_name, template_data in templates.items():
            for i, statement_data in enumerate(template_data["statements"]):
                query = statement_data["query_string"]

                with self.subTest(template=template_name, statement_index=i):
                    # We only care that the query executes without raising an exception.
                    # This verifies syntax and compatibility with the sample model.
                    try:
                        headers, results_data = Arch.select(query)
                        # A successful query returns a list of headers (even if empty)
                        self.assertIsInstance(headers, list)
                    except Exception as e:
                        self.fail(f"Query '{query}' from template '{template_name}' failed with an exception: {e}")

    def test_bundled_query_presets_are_valid(self):
        """
        Performs an integration test to ensure all bundled single-query presets
        are syntactically valid and executable.
        """
        import json

        # Find the bundled presets file using the robust helper
        preset_path = self._find_preset_file("query_presets.json")
        self.assertIsNotNone(preset_path, "Bundled query_presets.json not found in build or install directories. Check CMakeLists.txt.")

        with open(preset_path, 'r', encoding='utf8') as f:
            presets = json.load(f)

        # Sanity check that the expected presets are present
        self.assertIn("All Walls", presets)
        self.assertIn("Count by IfcType", presets)

        # Execute every query in the presets file
        for preset_name, preset_data in presets.items():
            query = preset_data["query"]

            with self.subTest(preset=preset_name):
                # We only care that the query executes without raising an exception.
                try:
                    headers, results_data = Arch.select(query)
                    self.assertIsInstance(headers, list)
                except Exception as e:
                    self.fail(f"Query '{query}' from preset '{preset_name}' failed with an exception: {e}")

    def test_where_in_clause(self):
        """
        Tests the SQL 'IN' clause for filtering against a list of values.
        """
        # This query should select only the two wall objects from the setup.
        query = "SELECT * FROM document WHERE Label IN ('Exterior Wall', 'Interior partition wall')"

        # This will fail at the parsing stage until the 'IN' keyword is implemented.
        headers, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. The query should return exactly two rows.
        self.assertEqual(len(results_data), 2, "The IN clause should have found exactly two matching objects.")

        # 2. Verify the labels of the returned objects.
        returned_labels = sorted([row[0] for row in results_data])
        expected_labels = sorted([self.wall_ext.Label, self.wall_int.Label])
        self.assertListEqual(returned_labels, expected_labels, "The objects returned by the IN clause are incorrect.")

    def test_type_function(self):
        """
        Tests the custom TYPE() function to ensure it returns the correct
        programmatic class name for both simple and proxy-based objects.
        """
        # --- Query and Execution ---
        # We want the type of the Part::Box and one of the Arch Walls.
        query = "SELECT TYPE(*) FROM document WHERE Name IN ('Generic_Box', 'Wall')"

        headers, results_data = Arch.select(query)

        # --- Assertions ---
        # The query should return two rows, one for each object.
        self.assertEqual(len(results_data), 2, "Query should have found the two target objects.")

        # Convert the results to a simple list for easier checking.
        # The result from the engine is a list of lists, e.g., [['Part.Box'], ['Arch.ArchWall']]
        type_names = sorted([row[0] for row in results_data])

        # 1. Verify the type of the Part::Box.
        #    The expected value is the C++ class name.
        self.assertIn('Part::Box', type_names, "TYPE() failed to identify the Part::Box.")

        # 2. Verify the type of the Arch Wall.
        #    Draft.get_type() returns the user-facing 'Wall' type from the proxy.
        self.assertIn('Wall', type_names, "TYPE() failed to identify the ArchWall proxy class.")

    def test_children_function(self):
        """
        Tests the unified CHILDREN() function for both direct containment (.Group)
        and hosting relationships (.Hosts), including traversal of generic groups.
        """
        import Draft

        # --- Test Setup: Create a mini-model with all relationship types ---
        # 1. A parent Floor for direct containment
        floor = Arch.makeBuildingPart(name="Ground Floor")
        # Use the canonical enumeration label used by the BIM module.
        floor.IfcType = "Building Storey"

        # 2. A host Wall for the hosting relationship
        host_wall = Arch.makeWall(name="Host Wall For Window")

        # 3. Child objects
        space1 = Arch.makeSpace(name="Living Room")
        space2 = Arch.makeSpace(name="Kitchen")
        win_profile = Draft.makeRectangle(length=1000, height=1200)
        window = Arch.makeWindow(baseobj=win_profile, name="Living Room Window")

        # 4. An intermediate generic group
        group = self.doc.addObject("App::DocumentObjectGroup", "Room Group")

        # 5. Establish relationships
        floor.addObject(space1)       # Floor directly contains Space1
        floor.addObject(group)        # Floor also contains the Group
        group.addObject(space2)       # The Group contains Space2
        Arch.addComponents(window, host=host_wall)
        # Ensure the document is recomputed before running the report query
        self.doc.recompute()

        # --- Sub-Test 1: Direct containment and group traversal ---
        with self.subTest(description="Direct containment with group traversal"):
            query = f"SELECT Label FROM CHILDREN(SELECT * FROM document WHERE Label = '{floor.Label}')"
            headers, results = Arch.select(query)

            returned_labels = sorted([row[0] for row in results])
            # The result should contain the spaces, but NOT the intermediate group itself.
            # Build the expected results from the actual object labels
            expected_labels = sorted([space1.Label, space2.Label])
            self.assertListEqual(returned_labels, expected_labels)

        # --- Sub-Test 2: Hosting Relationship (Reverse Lookup) ---
        with self.subTest(description="Hosting relationship"):
            query = f"SELECT Label FROM CHILDREN(SELECT * FROM document WHERE Label = '{host_wall.Label}')"
            headers, results = Arch.select(query)

            self.assertEqual(len(results), 1)
            self.assertEqual(results[0][0], window.Label)

    def test_order_by_label_desc(self):
        """Tests the ORDER BY clause to sort results alphabetically."""
        query = "SELECT Label FROM document WHERE IfcType = 'Wall' ORDER BY Label DESC"
        headers, results_data = Arch.select(query)

        # The results should be a list of lists, e.g., [['Wall 2'], ['Wall 1']]
        self.assertEqual(len(results_data), 2)
        returned_labels = [row[0] for row in results_data]

        # Wall labels from setUp are "Exterior Wall" and "Interior partition wall"
        # In descending order, "Interior..." should come first.
        expected_order = sorted([self.wall_ext.Label, self.wall_int.Label], reverse=True)

        self.assertListEqual(returned_labels, expected_order,
                             "The results were not sorted by Label in descending order.")

    def test_column_aliasing(self):
        """Tests renaming columns using the AS keyword."""
        # This query renames 'Label' to 'Wall Name' and sorts the results for a predictable check.
        query = "SELECT Label AS 'Wall Name' FROM document WHERE IfcType = 'Wall' ORDER BY Label ASC"
        headers, results_data = Arch.select(query)

        # 1. Assert that the header is the alias, not the original property name.
        self.assertEqual(headers, ['Wall Name'])

        # 2. Assert that the data is still correct.
        self.assertEqual(len(results_data), 2)
        returned_labels = [row[0] for row in results_data]
        # Wall labels from setUp: "Exterior Wall", "Interior partition wall". Sorted alphabetically.
        expected_labels = sorted([self.wall_ext.Label, self.wall_int.Label])
        self.assertListEqual(returned_labels, expected_labels)

    def test_string_functions(self):
        """Tests the CONCAT, LOWER, and UPPER string functions."""
        # Use a predictable object for testing, e.g., the Main Column.
        target_obj_name = self.column.Name
        target_obj_label = self.column.Label # "Main Column"
        target_obj_ifctype = self.column.IfcType # "Column"

        with self.subTest(description="LOWER function"):
            query = f"SELECT LOWER(Label) FROM document WHERE Name = '{target_obj_name}'"
            headers, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], target_obj_label.lower())

        with self.subTest(description="UPPER function"):
            query = f"SELECT UPPER(Label) FROM document WHERE Name = '{target_obj_name}'"
            headers, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], target_obj_label.upper())

        with self.subTest(description="CONCAT function with properties and literals"):
            query = f"SELECT CONCAT(Label, ': ', IfcType) FROM document WHERE Name = '{target_obj_name}'"
            headers, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            expected_string = f"{target_obj_label}: {target_obj_ifctype}"
            self.assertEqual(data[0][0], expected_string)

    def test_meaningful_error_on_transformer_failure(self):
        """
        Tests that a low-level VisitError from the transformer is converted
        into a high-level, user-friendly BimSqlSyntaxError.
        """
        # This query is syntactically correct but will fail during transformation
        # because the TYPE function requires '*' as its argument, not a property.
        query = "SELECT TYPE(Label) FROM document"

        with self.assertRaises(ArchSql.BimSqlSyntaxError) as cm:
            ArchSql._get_query_object(query)

        # Assert that the error message is our clean, high-level message
        # and not a raw, confusing traceback from deep inside the library.
        # We check that it contains the key parts of our formatted error.
        error_message = str(cm.exception)
        self.assertIn("Transformer Error", error_message)
        self.assertIn("Failed to process rule 'function'", error_message)
        self.assertIn("requires exactly one argument: '*'", error_message)

    def test_get_sql_keywords(self):
        """Tests the public API for retrieving all SQL keywords."""
        keywords = Arch.getSqlKeywords()
        self.assertIsInstance(keywords, list, "get_sql_keywords should return a list.")
        self.assertGreater(len(keywords), 10, "Should be a significant number of keywords.")

        # Check for the presence of a few key, case-insensitive keywords.
        self.assertIn("SELECT", keywords)
        self.assertIn("FROM", keywords)
        self.assertIn("WHERE", keywords)
        self.assertIn("ORDER", keywords, "The ORDER keyword should be present.")
        self.assertIn("BY", keywords, "The BY keyword should be present.")
        self.assertIn("AS", keywords)
        self.assertIn("COUNT", keywords, "Function names should be included as keywords.")

        # Check that internal/non-keyword tokens are correctly filtered out.
        self.assertNotIn("WS", keywords, "Whitespace token should be filtered out.")
        self.assertNotIn("RPAR", keywords, "Punctuation tokens should be filtered out.")
        self.assertNotIn("CNAME", keywords, "Regex-based tokens should be filtered out.")

    def test_function_in_where_clause(self):
        """Tests using a scalar function (LOWER) in the WHERE clause."""
        # self.column.Label is "Main Column". This query should find it case-insensitively.
        query = f"SELECT Label FROM document WHERE LOWER(Label) = 'main column'"
        _, results_data = Arch.select(query)

        self.assertEqual(len(results_data), 1, "Should find exactly one object.")
        self.assertEqual(results_data[0][0], self.column.Label, "Did not find the correct object.")

        # Also test that an aggregate function raises a proper exception.
        error_query = "SELECT Label FROM document WHERE COUNT(*) > 1"
        with self.assertRaises(ArchSql.SqlEngineError) as cm:
            # Test the public API directly, as it's expected to raise the exception.
            ArchSql._get_query_object(error_query)
        self.assertIn("Aggregate functions (like COUNT, SUM) cannot be used in a WHERE clause", str(cm.exception))

        # 2. Test the "unsafe" public API: select() should re-raise the exception.
        with self.assertRaises(Arch.SqlEngineError) as cm:
             Arch.select(error_query)
        self.assertIn("Aggregate functions (like COUNT, SUM) cannot be used in a WHERE clause", str(cm.exception))

        # 3. Test the "safe" public API: count() should catch the exception and return an error tuple.
        count, error_str = Arch.count(error_query)
        self.assertEqual(count, -1)
        self.assertIn("Aggregate functions", error_str)

    def test_null_as_operand(self):
        """Tests using NULL as a direct operand in a comparison like '= NULL'."""
        # In standard SQL, a comparison `SomeValue = NULL` evaluates to 'unknown'
        # and thus filters out the row. The purpose of this test is to ensure
        # that the query parses and executes without crashing, proving that our
        # NULL terminal transformer is working correctly.
        query = "SELECT * FROM document WHERE IfcRole = NULL"
        _, results_data = Arch.select(query)
        self.assertEqual(len(results_data), 0, "Comparing a column to NULL with '=' should return no rows.")

    def test_arithmetic_in_select_clause(self):
        """Tests arithmetic operations in the SELECT clause."""
        # Use the wall_ext object, which has Length=1000.0 (Quantity)
        target_name = self.wall_ext.Name

        with self.subTest(description="Simple multiplication with Quantity"):
            # Test: 1000.0 * 2.0 = 2000.0
            query = f"SELECT Length * 2 FROM document WHERE Name = '{target_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertAlmostEqual(data[0][0], 2000.0)

        with self.subTest(description="Operator precedence"):
            # Test: 100 + 1000.0 * 2 = 2100.0 (multiplication first)
            query = f"SELECT 100 + Length * 2 FROM document WHERE Name = '{target_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertAlmostEqual(data[0][0], 2100.0)

        with self.subTest(description="Parentheses overriding precedence"):
            # Test: (100 + 1000.0) * 2 = 2200.0 (addition first)
            query = f"SELECT (100 + Length) * 2 FROM document WHERE Name = '{target_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertAlmostEqual(data[0][0], 2200.0)

        with self.subTest(description="Arithmetic with unitless float property"):
            # self.wall_ext.Shape.Volume should be a float (200 * 3000 * 1000 = 600,000,000)
            # Test: 600,000,000 / 1,000,000 = 600.0
            query = f"SELECT Shape.Volume / 1000000 FROM document WHERE Name = '{target_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertAlmostEqual(data[0][0], 600.0)

    def test_convert_function(self):
        """Tests the CONVERT(value, 'unit') function."""
        # Use wall_ext, which has Length = 1000.0 (mm Quantity)
        target_name = self.wall_ext.Name

        # --- Test 1: Successful Conversion ---
        # This part of the test verifies that a valid conversion works correctly.
        query = f"SELECT CONVERT(Length, 'm') FROM document WHERE Name = '{target_name}'"
        headers, data = Arch.select(query)

        self.assertEqual(len(data), 1, "The query should return exactly one row.")
        self.assertEqual(len(data[0]), 1, "The row should contain exactly one column.")
        self.assertIsInstance(data[0][0], float, "The result of CONVERT should be a float.")
        self.assertAlmostEqual(data[0][0], 1.0, msg="1000mm should be converted to 1.0m.")

        # --- Test 2: Invalid Conversion Error Handling ---
        # This part of the test verifies that an invalid conversion (e.g., mm to kg),
        # which is an EXECUTION-TIME error, is handled correctly by the public API.
        error_query = f"SELECT CONVERT(Length, 'kg') FROM document WHERE Name = '{target_name}'"

        # 2a. Test the "unsafe" public API: select() should raise the execution-time error.
        with self.assertRaises(Arch.SqlEngineError) as cm:
            Arch.select(error_query)
        self.assertIn("Unit conversion failed", str(cm.exception))

        # 2b. Test the "safe" public API: count() should catch the execution-time error and return an error tuple.
        count, error_str = Arch.count(error_query)
        self.assertEqual(count, -1)
        self.assertIsInstance(error_str, str)
        self.assertIn("Unit conversion failed", error_str)

    def test_get_syntax_cheatsheet(self):
        """Tests the dynamic generation of the SQL syntax cheatsheet."""
        cheatsheet = Arch.getSqlCheatsheet()

        self.assertIsInstance(cheatsheet, str)
        self.assertIn("# BIM SQL Cheatsheet", cheatsheet)
        self.assertIn("## Clauses", cheatsheet)
        self.assertIn("SELECT", cheatsheet)
        self.assertIn("## Key Functions", cheatsheet)
        self.assertIn("- **Aggregate:**", cheatsheet)
        self.assertIn("COUNT", cheatsheet)
        self.assertIn("- **Hierarchical:**", cheatsheet)
        self.assertIn("CHILDREN", cheatsheet)
        self.assertIn("```sql", cheatsheet, "The cheatsheet should contain a formatted example query.")

    def test_task_panel_on_demand_preview(self):
        """Tests the on-demand 'Preview Results' feature in the ReportTaskPanel."""
        if not FreeCAD.GuiUp:
            self.skipTest("Cannot test ReportTaskPanel without a GUI.")

        # 1. Setup: Create a report object and the task panel
        report_obj = Arch.makeReport(name="PreviewTestReport")
        panel = ArchReport.ReportTaskPanel(report_obj)
        panel._select_statement_in_table(0) # Select the first statement to show the editor

        # 2. Initial State Assertions
        self.assertTrue(hasattr(panel, 'btn_preview_results'), "Panel should have a preview button.")
        self.assertTrue(hasattr(panel, 'table_preview_results'), "Panel should have a preview table.")
        self.assertFalse(panel.table_preview_results.isVisible(), "Preview table should be hidden initially.")

        # 3. Action: Set a valid query and simulate the button click
        # Use a simple query that is guaranteed to return 2 rows and 2 columns
        query = "SELECT Label, IfcType FROM document WHERE IfcType = 'Wall' ORDER BY Label"
        panel.sql_query_edit.setPlainText(query)
        panel._on_preview_results_clicked()

        # 4. Final State Assertions
        self.assertTrue(panel.table_preview_results.isVisible(), "Preview table should be visible after click.")
        self.assertEqual(panel.table_preview_results.columnCount(), 2, "Preview table should have 2 columns.")
        self.assertEqual(panel.table_preview_results.rowCount(), 2, "Preview table should have 2 rows.")

        # Check a cell value for correctness
        self.assertEqual(panel.table_preview_results.item(0, 0).text(), self.wall_ext.Label) # First wall, sorted
        self.assertEqual(panel.table_preview_results.item(1, 1).text(), self.wall_int.IfcType) # Second wall, IfcType

