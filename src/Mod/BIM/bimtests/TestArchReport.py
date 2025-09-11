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
        self.wall_int = Arch.makeWall(length=500, name="Interior partition wall")
        self.wall_int.IfcType = "Wall"
        self.column = Arch.makeStructure(length=300, width=300, height=2000, name="Main Column")
        self.column.IfcType = "Column"
        self.beam = Arch.makeStructure(length=2000, width=200, height=400, name="Main Beam")
        self.beam.IfcType = "Beam"
        self.window = Arch.makeWindow(name="Living Room Window")
        self.window.IfcType = "Window"
        self.part_box = self.doc.addObject("Part::Box", "Generic Box")
        self.queryable_objects = [
            self.wall_ext, self.wall_int, self.column, self.beam, self.window, self.part_box
        ]
        self.spreadsheet = self.doc.addObject("Spreadsheet::Sheet", "ReportTarget")
        self.doc.recompute()

    def _run_query_for_objects(self, query_string):
        report = Arch.makeReport()
        report.Target = self.spreadsheet
        report.Query = query_string
        self.doc.recompute()
        statement, error = ArchSql._get_query_object(query_string)
        self.assertIsNone(error, f"Query execution should not produce an error for: {query_string}")
        return statement.execute()

    def test_makeReport_default(self):
        report = Arch.makeReport()
        self.assertIsNotNone(report, "makeReport failed to create an object.")
        self.assertEqual(report.Label, "Report", "Default report label is incorrect.")

    def test_report_properties(self):
        report = Arch.makeReport()
        self.assertTrue(hasattr(report, "Query"), "Report object is missing 'Query' property.")
        self.assertTrue(hasattr(report, "Target"), "Report object is missing 'Target' property.")

    def test_select_all_from_document(self):
        results = self._run_query_for_objects('SELECT * FROM document')
        queryable_results = [o for o in results if o in self.queryable_objects]
        self.assertCountEqual([o.Label for o in queryable_results],
                              [o.Label for o in self.queryable_objects])

    def test_where_equals_string(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE IfcType = "Wall"')
        self.assertEqual(len(results), 2)
        self.assertCountEqual([o.Label for o in results], [self.wall_ext.Label, self.wall_int.Label])

    def test_where_not_equals_string(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE IfcType != "Wall"')
        expected_labels = [self.column.Label, self.beam.Label, self.window.Label]
        self.assertEqual(len(results), 3)
        self.assertCountEqual([o.Label for o in results], expected_labels)

    def test_where_is_null(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NULL')
        self.assertIn(self.part_box, results, "The part_box should be found by 'IS NULL'.")
        self.assertGreaterEqual(len(results), 1)

    def test_where_is_not_null(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE IfcType IS NOT NULL')
        self.assertEqual(len(results), 5)
        self.assertNotIn(self.part_box.Label, [o.Label for o in results])

    def test_where_like_case_insensitive(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "exterior wall"')
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0].Label, self.wall_ext.Label)

    def test_where_like_wildcard_middle(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "%wall%"')
        self.assertEqual(len(results), 2)
        self.assertCountEqual([o.Label for o in results], [self.wall_ext.Label, self.wall_int.Label])

    def test_where_like_wildcard_end(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE Label LIKE "Exterior%"')
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0].Label, self.wall_ext.Label)

    def test_where_boolean_and(self):
        query = 'SELECT * FROM document WHERE IfcType = "Wall" AND Label LIKE "%Exterior%"'
        results = self._run_query_for_objects(query)
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0].Label, self.wall_ext.Label)

    def test_where_boolean_or(self):
        query = 'SELECT * FROM document WHERE IfcType = "Window" OR IfcType = "Column"'
        results = self._run_query_for_objects(query)
        self.assertEqual(len(results), 2)
        self.assertCountEqual([o.Label for o in results], [self.window.Label, self.column.Label])

    def test_query_no_results(self):
        results = self._run_query_for_objects('SELECT * FROM document WHERE Label = "NonExistentObject"')
        self.assertEqual(len(results), 0)

    def test_query_invalid_syntax(self):
        """
        Test that invalid syntax is handled gracefully at both the internal
        and public API levels.
        """
        # 1. Test the internal function (_get_query_object)
        # It should return a raw exception object, not a string.
        statement, error_obj = ArchSql._get_query_object('SELECT FROM document WHERE')
        self.assertIsNone(statement, "Internal function should not return a statement on error.")
        self.assertIsInstance(error_obj, Exception, "Internal function should return an exception object.")

        # 2. Test the public function for the UI (run_query_for_count)
        # It should catch the exception and return a user-friendly string.
        count, error_str = ArchSql.run_query_for_count('SELECT FROM document WHERE')
        self.assertEqual(count, -1, "Public function should return count of -1 on error.")
        self.assertIsInstance(error_str, str, "Public function should return a string error message.")
        self.assertTrue('Syntax Error' in error_str, "User-facing error message is incorrect.")

    def test_incomplete_queries_are_handled_gracefully(self):
        """Test that various incomplete queries return the 'INCOMPLETE' status."""
        incomplete_queries = [
            "SELECT",
            "SELECT *",
            "SELECT * FROM",
            "SELECT * FROM document WHERE",
            "SELECT * FROM document WHERE Label =",
            "SELECT * FROM document WHERE Label LIKE",
        ]

        for query in incomplete_queries:
            with self.subTest(query=query):
                count, error = ArchSql.run_query_for_count(query)
                self.assertEqual(error, "INCOMPLETE", f"Query '{query}' should be marked as INCOMPLETE.")

    def test_invalid_partial_tokens_are_errors(self):
        """
        Test that queries with partial/mistyped keywords are correctly
        identified as syntax errors, not as incomplete.
        """
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
            report.Target = None
            report.Query = 'SELECT * FROM document'
            self.doc.recompute()
        except Exception as e:
            self.fail(f"Recomputing a report with no Target raised an unexpected exception: {e}")
