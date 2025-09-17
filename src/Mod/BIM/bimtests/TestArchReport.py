# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

"""Unit tests for the ArchReport and ArchSql modules."""

from unittest.mock import patch
import FreeCAD
import Arch
import ArchReport
from bimtests import TestArchBase
import ArchSql

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

        headers, results_data_from_sql = ArchSql.run_query_for_objects(query_string)

        self.assertIsInstance(headers, list, f"Headers should be a list for: {query_string}")
        self.assertIsInstance(results_data_from_sql, list, f"Results data should be a list for: {query_string}")

        # For aggregate queries (e.g., containing COUNT, GROUP BY), the results are summaries,
        # not direct object properties. The filtering logic below is incorrect for them.
        # We can detect an aggregate query if the headers contain typical aggregate function names.
        is_aggregate_query = any('COUNT' in h or 'SUM' in h for h in headers)
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
            ["Exterior Wall", "Wall", self.wall_ext.Height.UserString],
            ["Interior partition wall", "Wall", self.wall_int.Height.UserString],
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
        statement, error_obj = ArchSql._get_query_object('SELECT FROM document WHERE')
        self.assertIsNone(statement)
        self.assertIsInstance(error_obj, Exception)

        count, error_str = ArchSql.run_query_for_count('SELECT FROM document WHERE')
        self.assertEqual(count, -1)
        self.assertIsInstance(error_str, str)
        self.assertTrue('Syntax Error' in error_str)

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
                count, error = ArchSql.run_query_for_count(query)
                self.assertEqual(error, "INCOMPLETE", f"Query '{query}' should be marked as INCOMPLETE.")

    def test_invalid_partial_tokens_are_errors(self):
        invalid_queries = {
            "Partial keyword": "SELEC",
            "Mistyped keyword": "SELECT * FRM document",
        }

        for name, query in invalid_queries.items():
            with self.subTest(name=name, query=query):
                count, error = ArchSql.run_query_for_count(query)
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

