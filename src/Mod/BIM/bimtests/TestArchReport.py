# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

"""Unit tests for the ArchReport and ArchSql modules."""
import FreeCAD
import Arch
import Draft
import ArchSql
import ArchReport
from unittest.mock import patch
from bimtests import TestArchBase
from bimtests.fixtures.BimFixtures import create_test_model


class TestArchReport(TestArchBase.TestArchBase):

    def setUp(self):
        super().setUp()
        self.doc = self.document

        self.wall_ext = Arch.makeWall(length=1000, name="Exterior Wall")
        self.wall_ext.IfcType = "Wall"
        self.wall_ext.Height = FreeCAD.Units.Quantity(
            3000, "mm"
        )  # Store as Quantity for robust comparison

        self.wall_int = Arch.makeWall(length=500, name="Interior partition wall")
        self.wall_int.IfcType = "Wall"
        self.wall_int.Height = FreeCAD.Units.Quantity(2500, "mm")  # Store as Quantity

        self.column = Arch.makeStructure(length=300, width=330, height=2000, name="Main Column")
        self.column.IfcType = "Column"

        self.beam = Arch.makeStructure(length=2000, width=200, height=400, name="Main Beam")
        self.beam.IfcType = "Beam"

        self.window = Arch.makeWindow(name="Living Room Window")
        self.window.IfcType = "Window"

        self.part_box = self.doc.addObject(
            "Part::Box", "Generic Box"
        )  # This object has no IfcType property

        # Define a clean list of *only* the objects created by the test setUp
        self.test_objects_in_doc = [
            self.wall_ext,
            self.wall_int,
            self.column,
            self.beam,
            self.window,
            self.part_box,
        ]
        self.test_object_labels = sorted([o.Label for o in self.test_objects_in_doc])

        # We create the spreadsheet here, but it's part of the test setup, not a queryable object
        self.spreadsheet = self.doc.addObject("Spreadsheet::Sheet", "ReportTarget")
        self.doc.recompute()

    def _run_query_for_objects(self, query_string):
        """
        Helper method to run a query using the public API and return filtered results.
        This version is simplified to directly use Arch.select(), avoiding the
        creation of a Report object and thus preventing the "still touched" error.
        """
        # Directly use the public API to execute the read-only query.
        # This does not modify any objects in the document.
        try:
            headers, results_data_from_sql = Arch.select(query_string)
        except (ArchSql.BimSqlSyntaxError, ArchSql.SqlEngineError) as e:
            self.fail(f"The query '{query_string}' failed to execute with an exception: {e}")

        self.assertIsInstance(headers, list, f"Headers should be a list for: {query_string}")
        self.assertIsInstance(
            results_data_from_sql, list, f"Results data should be a list for: {query_string}"
        )

        # For aggregate queries (e.g., containing COUNT, GROUP BY), the results are summaries,
        # not direct object properties. The filtering logic below does not apply.
        is_aggregate_query = any(
            agg in h for h in headers for agg in ["COUNT", "SUM", "MIN", "MAX"]
        )
        if is_aggregate_query:
            return headers, results_data_from_sql

        # If SELECT *, results_data_from_sql is a list of lists, e.g., [['Exterior Wall'], ...].
        # Extract a flat list of labels for easier assertion.
        if headers == ["Object Label"]:
            extracted_labels = [row[0] for row in results_data_from_sql]
            # Filter against our defined test objects only.
            filtered_labels = [
                label for label in extracted_labels if label in self.test_object_labels
            ]
            return headers, filtered_labels

        # For specific column selections, results_data_from_sql is a list of lists of values.
        # Filter these rows based on whether their first element (assumed to be the label)
        # is one of our test objects.
        filtered_results_for_specific_columns = []
        if results_data_from_sql and len(results_data_from_sql[0]) > 0:
            for row in results_data_from_sql:
                if row[0] in self.test_object_labels:
                    filtered_results_for_specific_columns.append(row)

        return headers, filtered_results_for_specific_columns

    # Category 1: Basic Object Creation and Validation
    def test_makeReport_default(self):
        report = Arch.makeReport()
        self.assertIsNotNone(report, "makeReport failed to create an object.")
        self.assertEqual(report.Label, "Report", "Default report label is incorrect.")

    def test_report_properties(self):
        report = Arch.makeReport()
        self.assertTrue(
            hasattr(report, "Statements"), "Report object is missing 'Statements' property."
        )
        self.assertTrue(hasattr(report, "Target"), "Report object is missing 'Target' property.")

    # Category 2: Core SELECT Functionality
    def test_select_all_from_document(self):
        """Test a 'SELECT * FROM document' query."""
        headers, results_labels = self._run_query_for_objects("SELECT * FROM document")

        self.assertEqual(headers, ["Object Label"])
        self.assertCountEqual(
            results_labels, self.test_object_labels, "Should find all queryable objects."
        )

    def test_select_specific_columns_from_document(self):
        """Test a 'SELECT Label, IfcType, Height FROM document' query."""
        query_string = 'SELECT Label, IfcType, Height FROM document WHERE IfcType = "Wall"'
        headers, results_data = self._run_query_for_objects(query_string)

        self.assertEqual(headers, ["Label", "IfcType", "Height"])
        self.assertEqual(len(results_data), 2)

        expected_rows = [
            ["Exterior Wall", "Wall", self.wall_ext.Height],
            ["Interior partition wall", "Wall", self.wall_int.Height],
        ]
        self.assertCountEqual(results_data, expected_rows, "Specific column data mismatch.")

    # Category 3: WHERE Clause Filtering
    def test_where_equals_string(self):
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE IfcType = "Wall"'
        )
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.wall_ext.Label, self.wall_int.Label])

    def test_where_not_equals_string(self):
        """Test a WHERE clause with a not-equals check."""
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE IfcType != "Wall"'
        )
        # Strict SQL semantics: comparisons with NULL are treated as UNKNOWN
        # and therefore excluded. Use IS NULL / IS NOT NULL to test for nulls.
        expected_labels = [self.column.Label, self.beam.Label, self.window.Label]
        self.assertEqual(len(results_labels), 3)
        self.assertCountEqual(results_labels, expected_labels)

    def test_where_is_null(self):
        """Test a WHERE clause with an IS NULL check."""
        _, results_labels = self._run_query_for_objects(
            "SELECT * FROM document WHERE IfcType IS NULL"
        )
        # This expects only self.part_box as it's the only one in self.test_objects_in_doc with IfcType=None.
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.part_box.Label)

    def test_where_is_not_null(self):
        _, results_labels = self._run_query_for_objects(
            "SELECT * FROM document WHERE IfcType IS NOT NULL"
        )
        self.assertEqual(len(results_labels), 5)
        self.assertNotIn(self.part_box.Label, results_labels)

    def test_where_like_case_insensitive(self):
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE Label LIKE "exterior wall"'
        )
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_like_wildcard_middle(self):
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE Label LIKE "%wall%"'
        )
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.wall_ext.Label, self.wall_int.Label])

    def test_null_equality_is_excluded(self):
        """Strict SQL: comparisons with NULL should be excluded; use IS NULL."""
        _, results = self._run_query_for_objects("SELECT * FROM document WHERE IfcType = NULL")
        # '=' with NULL should not match (UNKNOWN -> excluded)
        self.assertEqual(len(results), 0)

    def test_null_inequality_excludes_nulls(self):
        """Strict SQL: IfcType != 'Wall' should exclude rows where IfcType is NULL."""
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE IfcType != "Wall"'
        )
        expected_labels = [self.column.Label, self.beam.Label, self.window.Label]
        self.assertCountEqual(results_labels, expected_labels)

    def test_is_null_and_is_not_null_behaviour(self):
        _, isnull_labels = self._run_query_for_objects(
            "SELECT * FROM document WHERE IfcType IS NULL"
        )
        self.assertIn(self.part_box.Label, isnull_labels)

        _, isnotnull_labels = self._run_query_for_objects(
            "SELECT * FROM document WHERE IfcType IS NOT NULL"
        )
        self.assertNotIn(self.part_box.Label, isnotnull_labels)

    def test_where_like_wildcard_end(self):
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE Label LIKE "Exterior%"'
        )
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_boolean_and(self):
        query = 'SELECT * FROM document WHERE IfcType = "Wall" AND Label LIKE "%Exterior%"'
        _, results_labels = self._run_query_for_objects(query)
        self.assertEqual(len(results_labels), 1)
        self.assertEqual(results_labels[0], self.wall_ext.Label)

    def test_where_boolean_or(self):
        query = 'SELECT * FROM document WHERE IfcType = "Window" OR IfcType = "Column"'
        _, results_labels = self._run_query_for_objects(query)
        self.assertEqual(len(results_labels), 2)
        self.assertCountEqual(results_labels, [self.window.Label, self.column.Label])

    # Category 4: Edge Cases and Error Handling
    def test_query_no_results(self):
        _, results_labels = self._run_query_for_objects(
            'SELECT * FROM document WHERE Label = "NonExistentObject"'
        )
        self.assertEqual(len(results_labels), 0)

    @patch("FreeCAD.Console.PrintError")
    def test_query_invalid_syntax(self, mock_print_error):
        # The low-level function now raises an exception on failure.
        with self.assertRaises(Arch.BimSqlSyntaxError) as cm:
            Arch.select("SELECT FROM document WHERE")
        self.assertFalse(
            cm.exception.is_incomplete, "A syntax error should not be marked as incomplete."
        )

        # The high-level function for the UI catches it and returns a simple string.
        count, error_str = Arch.count("SELECT FROM document WHERE")
        self.assertEqual(count, -1)
        self.assertIsInstance(error_str, str)
        self.assertIn("Syntax Error", error_str)

    def test_incomplete_queries_are_handled_gracefully(self):
        incomplete_queries = [
            "SELECT",
            "SELECT *",
            "SELECT * FROM",
            "SELECT * FROM document WHERE",
            "SELECT * FROM document WHERE Label =",
            "SELECT * FROM document WHERE Label LIKE",
            'SELECT * FROM document WHERE Label like "%wa',  # Test case for incomplete string literal
        ]

        for query in incomplete_queries:
            with self.subTest(query=query):
                count, error = Arch.count(query)
                self.assertEqual(
                    error, "INCOMPLETE", f"Query '{query}' should be marked as INCOMPLETE."
                )

    def test_invalid_partial_tokens_are_errors(self):
        invalid_queries = {
            "Mistyped keyword": "SELECT * FRM document",
        }

        for name, query in invalid_queries.items():
            with self.subTest(name=name, query=query):
                _, error = Arch.count(query)
                self.assertNotEqual(
                    error,
                    "INCOMPLETE",
                    f"Query '{query}' should be a syntax error, not incomplete.",
                )
                self.assertIsNotNone(error, f"Query '{query}' should have returned an error.")

    def test_report_no_target(self):
        try:
            report = Arch.makeReport()
            # Creation initializes a target spreadsheet; verify it's set
            self.assertIsNotNone(report.Target, "Report Target should be set on creation.")
            # Set the first statement's query string
            # Prefer operating on the proxy runtime objects when available
            if hasattr(report, "Proxy"):
                # Ensure live statements are hydrated from persisted storage
                report.Proxy.hydrate_live_statements(report)

                if not getattr(report.Proxy, "live_statements", None):
                    # No live statements: create a persisted starter and hydrate again
                    report.Statements = [
                        ArchReport.ReportStatement(
                            description="Statement 1", query_string="SELECT * FROM document"
                        ).dumps()
                    ]
                    report.Proxy.hydrate_live_statements(report)
                else:
                    report.Proxy.live_statements[0].query_string = "SELECT * FROM document"
                    report.Proxy.commit_statements()
            else:
                # Fallback for environments without a proxy: persist a dict
                if not hasattr(report, "Statements") or not report.Statements:
                    report.Statements = [
                        ArchReport.ReportStatement(
                            description="Statement 1", query_string="SELECT * FROM document"
                        ).dumps()
                    ]
                else:
                    # Persist a fresh statement dict in the fallback path
                    report.Statements = [
                        ArchReport.ReportStatement(
                            description="Statement 1", query_string="SELECT * FROM document"
                        ).dumps()
                    ]
            self.doc.recompute()
        except Exception as e:
            self.fail(f"Recomputing a report with no Target raised an unexpected exception: {e}")

        # UX: when the report runs without a pre-set Target, it should create
        # a spreadsheet, set the sheet.ReportName, and persist the Target link
        # so subsequent runs are deterministic.
        self.assertIsNotNone(
            report.Target, "Report Target should be set after running with no pre-existing Target."
        )
        self.assertEqual(getattr(report.Target, "ReportName", None), report.Name)

    def test_group_by_ifctype_with_count(self):
        """Test GROUP BY with COUNT(*) to summarize objects by type."""
        # Add a WHERE clause to exclude test scaffolding objects from the count.
        query = (
            "SELECT IfcType, COUNT(*) FROM document "
            "WHERE TypeId != 'App::FeaturePython' AND TypeId != 'Spreadsheet::Sheet' "
            "GROUP BY IfcType"
        )
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ["IfcType", "COUNT(*)"])

        # Convert results to a dict for easy, order-independent comparison.
        # Handle the case where IfcType is None, which becomes a key in the dict.
        results_dict = {row[0] if row[0] != "None" else None: int(row[1]) for row in results_data}

        expected_counts = {
            "Wall": 2,
            "Column": 1,
            "Beam": 1,
            "Window": 1,
            None: 1,  # The Part::Box has a NULL IfcType, which forms its own group
        }
        self.assertDictEqual(
            results_dict, expected_counts, "The object counts per IfcType are incorrect."
        )

    def test_count_all_without_group_by(self):
        """Test COUNT(*) on the whole dataset without grouping."""
        # Add a WHERE clause to exclude test scaffolding objects from the count.
        query = (
            "SELECT COUNT(*) FROM document "
            "WHERE TypeId != 'App::FeaturePython' AND TypeId != 'Spreadsheet::Sheet'"
        )
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ["COUNT(*)"])
        self.assertEqual(len(results_data), 1, "Non-grouped aggregate should return a single row.")
        self.assertEqual(
            int(results_data[0][0]),
            len(self.test_objects_in_doc),
            "COUNT(*) did not return the total number of test objects.",
        )

    def test_group_by_with_sum(self):
        """Test GROUP BY with SUM() on a numeric property."""
        # Note: We filter for objects that are likely to have a Height property to get a clean sum.
        query = (
            "SELECT IfcType, SUM(Height) FROM document "
            "WHERE IfcType = 'Wall' OR IfcType = 'Column' "
            "GROUP BY IfcType"
        )
        headers, results_data = self._run_query_for_objects(query)

        self.assertEqual(headers, ["IfcType", "SUM(Height)"])
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

        self.assertEqual(headers, ["MIN(Length)", "MAX(Length)"])
        self.assertEqual(
            len(results_data), 1, "Aggregate query without GROUP BY should return one row."
        )

        # Expected: Interior wall is 500, Exterior wall is 1000
        min_length = float(results_data[0][0])
        max_length = float(results_data[0][1])

        self.assertAlmostEqual(min_length, 500.0)
        self.assertAlmostEqual(max_length, 1000.0)

    def test_invalid_group_by_raises_error(self):
        """A SELECT column not in GROUP BY and not in an aggregate should fail validation."""
        # 'Label' is not aggregated and not in the 'GROUP BY' clause, making this query invalid.
        query = "SELECT Label, COUNT(*) FROM document GROUP BY IfcType"

        # _run_query should raise an exception for validation errors.
        with self.assertRaises(ArchSql.SqlEngineError) as cm:
            Arch.select(query)

        # Check for the specific, user-friendly error message within the exception.
        self.assertIn(
            "must appear in the GROUP BY clause",
            str(cm.exception),
            "The validation error message is not descriptive enough.",
        )

    def test_non_grouped_sum_calculates_correctly(self):
        """
        Tests the SUM() aggregate function without a GROUP BY clause in isolation.
        This test calls the SQL engine directly to ensure the summing logic is correct.
        """
        # The query sums the Height of the two wall objects created in setUp().
        # Expected result: 3000mm + 2500mm = 5500mm.
        query = "SELECT SUM(Height) FROM document WHERE IfcType = 'Wall'"

        # We call the engine directly, bypassing the _run_query_for_objects helper.
        _, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. An aggregate query without a GROUP BY should always return exactly one row.
        self.assertEqual(
            len(results_data), 1, "A non-grouped aggregate query should return exactly one row."
        )

        # 2. The result in that row should be the correct sum.
        actual_sum = float(results_data[0][0])
        expected_sum = 5500.0
        self.assertAlmostEqual(
            actual_sum,
            expected_sum,
            "The SUM() result is incorrect. The engine is not accumulating the values correctly.",
        )

    def test_non_grouped_query_with_mixed_extractors(self):
        """
        Tests a non-grouped query with both a static value and a SUM() aggregate.
        """
        query = "SELECT 'Total Height', SUM(Height) FROM document WHERE IfcType = 'Wall'"

        # We call the engine directly to isolate its behavior.
        _, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. The query should still return exactly one row.
        self.assertEqual(
            len(results_data), 1, "A non-grouped mixed query should return exactly one row."
        )

        # 2. Check the content of the single row.
        #    The first column should be the static string.
        self.assertEqual(results_data[0][0], "Total Height")
        #    The second column should be the correct sum (3000 + 2500 = 5500).
        actual_sum = float(results_data[0][1])
        expected_sum = 5500.0
        self.assertAlmostEqual(
            actual_sum, expected_sum, "The SUM() result in a mixed non-grouped query is incorrect."
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

        self.doc.recompute()  # Ensure space areas are calculated

        query = "SELECT SUM(Area) FROM document WHERE IfcType = 'Space'"

        # Call the engine directly to isolate its behavior
        _, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. An aggregate query should return exactly one row.
        self.assertEqual(
            len(results_data), 1, "A non-grouped aggregate query should return exactly one row."
        )

        # 2. The result in the row should be a float. This verifies the engine's
        #    design to return raw numbers for aggregates.
        self.assertIsInstance(results_data[0][0], float, "The result of a SUM() should be a float.")

        # 3. The value of the float should be the correct sum.
        actual_sum = results_data[0][0]
        expected_sum = 6500000.0  # 2,000,000 + 4,500,000

        self.assertAlmostEqual(
            actual_sum, expected_sum, "The SUM(Area) for Space objects is incorrect."
        )

    def test_min_and_max_aggregates(self):
        """
        Tests the MIN() and MAX() aggregate functions on a numeric property.
        """
        # Note: The test setup already includes two walls with different lengths.
        # Exterior Wall: Length = 1000mm
        # Interior Wall: Length = 500mm
        query = "SELECT MIN(Length), MAX(Length) FROM document WHERE IfcType = 'Wall'"

        _, results_data = Arch.select(query)

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
        self.assertEqual(
            int(results_prop[0][0]),
            2,
            f"COUNT({unique_prop_name}) should count exactly the 2 objects where the property was added.",
        )

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
            self.part_box.Label,
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
        # 1. Load presets.
        report_presets = ArchReport._get_presets("report")
        self.assertGreater(
            len(report_presets),
            0,
            "No bundled report templates were found. Check CMakeLists.txt and file paths.",
        )

        # 2. Verify that the expected templates were loaded by their display name.
        loaded_template_names = {preset["name"] for preset in report_presets.values()}
        self.assertIn("Room and Area Schedule", loaded_template_names)
        self.assertIn("Wall Quantities", loaded_template_names)

        # 3. Execute every query in every statement of every template.
        for filename, preset in report_presets.items():
            # This test should only validate bundled system presets.
            if preset.get("is_user"):
                continue

            template_name = preset["name"]
            statements = preset["data"].get("statements", [])
            self.assertGreater(
                len(statements), 0, f"Template '{template_name}' contains no statements."
            )

            for i, statement_data in enumerate(statements):
                query = statement_data.get("query_string")
                self.assertIsNotNone(
                    query, f"Statement {i} in '{template_name}' is missing a 'query_string'."
                )

                with self.subTest(template=template_name, statement_index=i):
                    # We only care that the query executes without raising an exception.
                    try:
                        headers, _ = Arch.select(query)
                        self.assertIsInstance(headers, list)
                    except Exception as e:
                        self.fail(
                            f"Query '{query}' from template '{template_name}' (file: {filename}) failed with an exception: {e}"
                        )

    def test_bundled_query_presets_are_valid(self):
        """
        Performs an integration test to ensure all bundled single-query presets
        are syntactically valid and executable.
        """
        # 1. Load presets using the new, correct backend function.
        query_presets = ArchReport._get_presets("query")
        self.assertGreater(
            len(query_presets),
            0,
            "No bundled query presets were found. Check CMakeLists.txt and file paths.",
        )

        # 2. Verify that the expected presets were loaded.
        loaded_preset_names = {preset["name"] for preset in query_presets.values()}
        self.assertIn("All Walls", loaded_preset_names)
        self.assertIn("Count by IfcType", loaded_preset_names)

        # 3. Execute every query in the presets file.
        for filename, preset in query_presets.items():
            # This test should only validate bundled system presets.
            if preset.get("is_user"):
                continue

            preset_name = preset["name"]
            query = preset["data"].get("query")
            self.assertIsNotNone(query, f"Preset '{preset_name}' is missing a 'query'.")

            with self.subTest(preset=preset_name):
                # We only care that the query executes without raising an exception.
                try:
                    headers, _ = Arch.select(query)
                    self.assertIsInstance(headers, list)
                except Exception as e:
                    self.fail(
                        f"Query '{query}' from preset '{preset_name}' (file: {filename}) failed with an exception: {e}"
                    )

    def test_where_in_clause(self):
        """
        Tests the SQL 'IN' clause for filtering against a list of values.
        """
        # This query should select only the two wall objects from the setup.
        query = "SELECT * FROM document WHERE Label IN ('Exterior Wall', 'Interior partition wall')"

        # This will fail at the parsing stage until the 'IN' keyword is implemented.
        _, results_data = Arch.select(query)

        # --- Assertions ---
        # 1. The query should return exactly two rows.
        self.assertEqual(
            len(results_data), 2, "The IN clause should have found exactly two matching objects."
        )

        # 2. Verify the labels of the returned objects.
        returned_labels = sorted([row[0] for row in results_data])
        expected_labels = sorted([self.wall_ext.Label, self.wall_int.Label])
        self.assertListEqual(
            returned_labels, expected_labels, "The objects returned by the IN clause are incorrect."
        )

    def test_type_function(self):
        """
        Tests the custom TYPE() function to ensure it returns the correct
        programmatic class name for both simple and proxy-based objects.
        """
        # --- Query and Execution ---
        # We want the type of the Part::Box and one of the Arch Walls.
        query = "SELECT TYPE(*) FROM document WHERE Name IN ('Generic_Box', 'Wall')"

        _, results_data = Arch.select(query)

        # --- Assertions ---
        # The query should return two rows, one for each object.
        self.assertEqual(len(results_data), 2, "Query should have found the two target objects.")

        # Convert the results to a simple list for easier checking.
        # The result from the engine is a list of lists, e.g., [['Part.Box'], ['Arch.ArchWall']]
        type_names = sorted([row[0] for row in results_data])

        # 1. Verify the type of the Part::Box.
        #    The expected value is the C++ class name.
        self.assertIn("Part::Box", type_names, "TYPE() failed to identify the Part::Box.")

        # 2. Verify the type of the Arch Wall.
        #    Draft.get_type() returns the user-facing 'Wall' type from the proxy.
        self.assertIn("Wall", type_names, "TYPE() failed to identify the ArchWall proxy class.")

    def test_children_function(self):
        """
        Tests the unified CHILDREN() function for both direct containment (.Group)
        and hosting relationships (.Hosts), including traversal of generic groups.
        """

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
        floor.addObject(space1)  # Floor directly contains Space1
        floor.addObject(group)  # Floor also contains the Group
        group.addObject(space2)  # The Group contains Space2
        Arch.addComponents(window, host=host_wall)
        # Ensure the document is recomputed before running the report query
        self.doc.recompute()

        # --- Sub-Test 1: Direct containment and group traversal ---
        with self.subTest(description="Direct containment with group traversal"):
            query = (
                f"SELECT Label FROM CHILDREN(SELECT * FROM document WHERE Label = '{floor.Label}')"
            )
            _, results = Arch.select(query)

            returned_labels = sorted([row[0] for row in results])
            # The result should contain the spaces, but NOT the intermediate group itself.
            # Build the expected results from the actual object labels
            expected_labels = sorted([space1.Label, space2.Label])
            self.assertListEqual(returned_labels, expected_labels)

        # --- Sub-Test 2: Hosting Relationship (Reverse Lookup) ---
        with self.subTest(description="Hosting relationship"):
            query = f"SELECT Label FROM CHILDREN(SELECT * FROM document WHERE Label = '{host_wall.Label}')"
            _, results = Arch.select(query)

            self.assertEqual(len(results), 1)
            self.assertEqual(results[0][0], window.Label)

    def test_order_by_label_desc(self):
        """Tests the ORDER BY clause to sort results alphabetically."""
        query = "SELECT Label FROM document WHERE IfcType = 'Wall' ORDER BY Label DESC"
        _, results_data = Arch.select(query)

        # The results should be a list of lists, e.g., [['Wall 2'], ['Wall 1']]
        self.assertEqual(len(results_data), 2)
        returned_labels = [row[0] for row in results_data]

        # Wall labels from setUp are "Exterior Wall" and "Interior partition wall"
        # In descending order, "Interior..." should come first.
        expected_order = sorted([self.wall_ext.Label, self.wall_int.Label], reverse=True)

        self.assertListEqual(
            returned_labels,
            expected_order,
            "The results were not sorted by Label in descending order.",
        )

    def test_column_aliasing(self):
        """Tests renaming columns using the AS keyword."""
        # This query renames 'Label' to 'Wall Name' and sorts the results for a predictable check.
        query = "SELECT Label AS 'Wall Name' FROM document WHERE IfcType = 'Wall' ORDER BY 'Wall Name' ASC"
        headers, results_data = Arch.select(query)

        # 1. Assert that the header is the alias, not the original property name.
        self.assertEqual(headers, ["Wall Name"])

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
        target_obj_label = self.column.Label  # "Main Column"
        target_obj_ifctype = self.column.IfcType  # "Column"

        with self.subTest(description="LOWER function"):
            query = f"SELECT LOWER(Label) FROM document WHERE Name = '{target_obj_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], target_obj_label.lower())

        with self.subTest(description="UPPER function"):
            query = f"SELECT UPPER(Label) FROM document WHERE Name = '{target_obj_name}'"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], target_obj_label.upper())

        with self.subTest(description="CONCAT function with properties and literals"):
            query = f"SELECT CONCAT(Label, ': ', IfcType) FROM document WHERE Name = '{target_obj_name}'"
            _, data = Arch.select(query)
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
            Arch.select(query)

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

        # 1. Test the "unsafe" public API: select() should re-raise the exception.
        with self.assertRaises(Arch.SqlEngineError) as cm:
            Arch.select(error_query)
        self.assertIn(
            "Aggregate functions (like COUNT, SUM) cannot be used in a WHERE clause",
            str(cm.exception),
        )

        # 2. Test the "safe" public API: count() should catch the exception and return an error tuple.
        count, error_str = Arch.count(error_query)
        self.assertEqual(count, -1)
        self.assertIn("Aggregate functions", error_str)

    def test_null_as_operand(self):
        """Tests using NULL as a direct operand in a comparison like '= NULL'."""
        # In standard SQL, a comparison `SomeValue = NULL` evaluates to 'unknown'
        # and thus filters out the row. The purpose of this test is to ensure
        # that the query parses and executes without crashing, proving that our
        # NULL terminal transformer is working correctly.
        query = "SELECT * FROM document WHERE IfcType = NULL"
        _, results_data = Arch.select(query)
        self.assertEqual(
            len(results_data), 0, "Comparing a column to NULL with '=' should return no rows."
        )

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
        _, data = Arch.select(query)

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

    def test_get_sql_api_documentation(self):
        """Tests the data structure returned by the SQL documentation API."""
        api_data = Arch.getSqlApiDocumentation()

        self.assertIsInstance(api_data, dict)
        self.assertIn("clauses", api_data)
        self.assertIn("functions", api_data)

        # Check for a known clause and a known function category
        self.assertIn("SELECT", api_data["clauses"])
        self.assertIn("Aggregate", api_data["functions"])

        # Check for a specific function's data
        count_func = next(
            (f for f in api_data["functions"]["Aggregate"] if f["name"] == "COUNT"), None
        )
        self.assertIsNotNone(count_func)
        self.assertIn("description", count_func)
        self.assertIn("snippet", count_func)
        self.assertGreater(len(count_func["description"]), 0)

    # GUI-specific tests were moved to TestArchReportGui.py

    def test_count_with_group_by_is_correct_and_fast(self):
        """
        Ensures that Arch.count() on a GROUP BY query returns the number of
        final groups (output rows), not the number of input objects.
        This validates the performance refactoring.
        """
        # This query will have 5 input objects with an IfcType
        # but only 4 output rows/groups (Wall, Column, Beam, Window).
        query = "SELECT IfcType, COUNT(*) FROM document WHERE IfcType IS NOT NULL GROUP BY IfcType"

        # The count() function should be fast and correct.
        count, error = Arch.count(query)

        self.assertIsNone(error, "The query should be valid.")
        self.assertEqual(
            count, 4, "Count should return the number of groups, not the number of objects."
        )

    def test_sql_comment_support(self):
        """Tests that single-line and multi-line SQL comments are correctly ignored."""

        with self.subTest(description="Single-line comments with --"):
            # This query uses comments to explain and to disable the ORDER BY clause.
            # The engine should ignore them and return an unsorted result.
            query = """
                SELECT Label           -- Select the object's label
                FROM document
                WHERE IfcType = 'Wall' -- Only select walls
                -- ORDER BY Label DESC
            """
            _, data = Arch.select(query)

            # The query should run as if the comments were not there.
            self.assertEqual(len(data), 2, "Should find the two wall objects.")
            # Verify the content without assuming a specific order.
            found_labels = {row[0] for row in data}
            expected_labels = {self.wall_ext.Label, self.wall_int.Label}
            self.assertSetEqual(found_labels, expected_labels)

        with self.subTest(description="Multi-line comments with /* ... */"):
            # This query uses a block comment to completely disable the WHERE clause.
            query = """
                SELECT Label
                FROM document
                /*
                WHERE IfcType = 'Wall'
                ORDER BY Label
                */
            """
            _, data = Arch.select(query)
            # Without the WHERE clause, it should return all test objects.
            # The assertion must compare against all objects in the document,
            # not just the list of BIM objects, as the setup also creates
            # a spreadsheet.
            self.assertEqual(len(data), len(self.doc.Objects))

    def test_query_with_non_ascii_property_name(self):
        """
        Tests that the SQL engine can correctly handle non-ASCII (Unicode)
        characters in property names, which is crucial for international users.
        """
        # --- Test Setup ---
        # Add a dynamic property with a German name containing a non-ASCII character.
        # This simulates a common international use case.
        prop_name_unicode = "Fläche"  # "Area" in German
        self.column.addProperty("App::PropertyFloat", prop_name_unicode, "BIM")
        setattr(self.column, prop_name_unicode, 42.5)
        self.doc.recompute()

        # --- The Query ---
        # This query will fail at the parsing (lexing) stage with the old grammar.
        query = f"SELECT {prop_name_unicode} FROM document WHERE Name = '{self.column.Name}'"

        # --- Test Execution ---
        # We call the "unsafe" select() API, as it should raise the parsing
        # exception with the old grammar, and succeed with the new one.
        try:
            headers, results_data = Arch.select(query)
            # --- Assertions for when the test passes ---
            self.assertEqual(
                len(results_data), 1, "The query should find the single target object."
            )
            self.assertEqual(headers, [prop_name_unicode])
            self.assertAlmostEqual(results_data[0][0], 42.5)

        except Arch.BimSqlSyntaxError as e:
            # --- Assertion for when the test fails ---
            # This makes the test's purpose clear: it's expected to fail
            # with a syntax error until the grammar is fixed.
            self.fail(f"Parser failed to handle Unicode identifier. Error: {e}")

    def test_order_by_multiple_columns(self):
        """Tests sorting by multiple columns in the ORDER BY clause."""
        # This query selects a subset of objects and sorts them first by their
        # IfcType alphabetically, and then by their Label alphabetically within
        # each IfcType group. This requires a multi-column sort to verify.
        query = """
            SELECT Label, IfcType
            FROM document
            WHERE IfcType IN ('Wall', 'Column', 'Beam')
            ORDER BY IfcType, Label ASC
        """
        _, data = Arch.select(query)

        self.assertEqual(len(data), 4, "Should find the two walls, one column, and one beam.")

        # Verify the final, multi-level sorted order.
        # The engine should sort by IfcType first ('Beam' < 'Column' < 'Wall'),
        # and then by Label for the two 'Wall' objects.
        expected_order = [
            [self.beam.Label, self.beam.IfcType],  # Type: Beam
            [self.column.Label, self.column.IfcType],  # Type: Column
            [self.wall_ext.Label, self.wall_ext.IfcType],  # Type: Wall, Label: Exterior...
            [self.wall_int.Label, self.wall_int.IfcType],  # Type: Wall, Label: Interior...
        ]

        # We sort our expected list's inner items to be sure, in case the test setup changes.
        expected_order = sorted(expected_order, key=lambda x: (x[1], x[0]))

        self.assertListEqual(data, expected_order)

    def test_parent_function_and_chaining(self):
        """
        Tests the PARENT(*) function with simple and chained calls,
        and verifies the logic for transparently skipping generic groups.
        """
        # 1. ARRANGE: Create a comprehensive hierarchy
        site = Arch.makeSite(name="Test Site")
        building = Arch.makeBuilding(name="Test Building")
        floor = Arch.makeFloor(name="Test Floor")
        wall = Arch.makeWall(name="Test Wall")
        win_profile = Draft.makeRectangle(1000, 1000)
        window = Arch.makeWindow(win_profile, name="Test Window")

        generic_group = self.doc.addObject("App::DocumentObjectGroup", "Test Generic Group")
        space_profile = Draft.makeRectangle(2000, 2000)
        space = Arch.makeSpace(space_profile, name="Test Space")

        site.addObject(building)
        building.addObject(floor)
        floor.addObject(wall)
        floor.addObject(generic_group)
        generic_group.addObject(space)
        Arch.addComponents(window, wall)
        self.doc.recompute()

        # 2. ACT & ASSERT

        # Sub-Test A: Skipping a generic group
        # The PARENT of the Space should be the Floor, not the Generic Group.
        with self.subTest(description="Skipping generic group"):
            query = f"SELECT PARENT(*).Label FROM document WHERE Label = '{space.Label}'"
            _, data = Arch.select(query)
            self.assertEqual(
                data[0][0], floor.Label, "PARENT(Space) should skip the group and return the Floor."
            )

        # Sub-Test B: Chained parent finding for a contained object
        # The significant grandparent of the Wall (Wall -> Floor -> Building) is the Building.
        with self.subTest(description="Chained PARENT of Wall"):
            query = f"SELECT PARENT(*).PARENT(*).Label FROM document WHERE Label = '{wall.Label}'"
            _, data = Arch.select(query)
            self.assertEqual(data[0][0], building.Label)

        # Sub-Test C: Chained parent finding for a hosted object
        # The significant great-grandparent of the Window (Window -> Wall -> Floor -> Building) is the Building.
        with self.subTest(description="Chained PARENT of Window"):
            query = f"SELECT PARENT(*).PARENT(*).PARENT(*).Label FROM document WHERE Label = '{window.Label}'"
            _, data = Arch.select(query)
            self.assertEqual(data[0][0], building.Label)

        # Sub-Test D: Filtering by a logical grandparent
        # This query should find all objects whose significant grandparent is the Building.
        # This includes the Space (grandparent is Floor's parent) and the Wall (grandparent is Floor's parent).
        with self.subTest(description="Filtering by logical grandparent"):
            query = (
                f"SELECT Label FROM document WHERE PARENT(*).PARENT(*).Label = '{building.Label}'"
            )
            _, data = Arch.select(query)

            found_labels = sorted([row[0] for row in data])
            expected_labels = sorted(
                [space.Label, wall.Label, generic_group.Label]
            )  # The group's logical grandparent is also the building.
            self.assertListEqual(
                found_labels,
                expected_labels,
                "Query did not find all objects with the correct logical grandparent.",
            )

    def test_ppa_and_query_permutations(self):
        """
        Runs a suite of integration tests against a complex model to
        validate Pythonic Property Access and other query features.
        """
        # --- 1. ARRANGE: Create a complex model ---
        # Build the model using the generator function
        model = create_test_model(self.document)

        # Get references to key objects from the returned dictionary
        ground_floor = model["ground_floor"]
        upper_floor = model["upper_floor"]
        front_door = model["front_door"]
        living_window = model["living_window"]
        office_space = model["office_space"]
        living_space = model["living_space"]
        interior_wall = model["interior_wall"]
        exterior_wall = model["exterior_wall"]

        # --- 2. ACT & ASSERT: Run query permutations ---

        # Sub-Test A: Chained PARENT in SELECT clause
        with self.subTest(description="PPA in SELECT clause"):
            query = (
                f"SELECT PARENT(*).PARENT(*).Label FROM document WHERE Label = '{front_door.Label}'"
            )
            _, data = Arch.select(query)
            self.assertEqual(
                data[0][0], ground_floor.Label, "Grandparent of Front Door should be Ground Floor"
            )

        # Sub-Test B: Chained PARENT in WHERE clause
        with self.subTest(description="PPA in WHERE clause"):
            query = f"SELECT Label FROM document WHERE PARENT(*).PARENT(*).Label = '{ground_floor.Label}'"
            _, data = Arch.select(query)
            found_labels = sorted([row[0] for row in data])
            expected_labels = sorted([front_door.Label, living_window.Label])
            self.assertListEqual(
                found_labels, expected_labels, "Should find the Door and Window on the Ground Floor"
            )

        # Sub-Test C: Chained PARENT in ORDER BY clause
        with self.subTest(description="PPA in ORDER BY clause"):
            # Create a proper 3D solid volume for the new space.
            upper_box = self.document.addObject("Part::Box", "UpperSpaceVolume")
            upper_box.Length, upper_box.Width, upper_box.Height = 1000.0, 1000.0, 3000.0

            upper_space = Arch.makeSpace(baseobj=upper_box, name="Upper Space")
            upper_floor.addObject(upper_space)
            self.document.recompute()

            # The query now selects both the space's label and its parent's label.
            # This is the robust way to verify the sort order.
            query = f"SELECT Label, PARENT(*).Label AS ParentLabel FROM document WHERE IfcType = 'Space' ORDER BY ParentLabel DESC"
            _, data = Arch.select(query)

            # data is now a list of lists, e.g., [['Upper Space', 'Upper Floor'], ['Office Space', 'Ground Floor'], ...]

            # The assertion now directly checks the parent label returned by the query.
            # This is self-contained and does not require error-prone lookups.
            parent_label_of_first_result = data[0][1]
            self.assertEqual(
                parent_label_of_first_result,
                upper_floor.Label,
                "The first item in the sorted list should belong to the Upper Floor.",
            )

        # Sub-Test D: Accessing a sub-property of a parent
        with self.subTest(description="PPA with sub-property access"):
            # The Floor's placement Z is 0.0
            query = f"SELECT Label FROM document WHERE PARENT(*).Placement.Base.z = 0.0 AND IfcType = 'Space'"
            _, data = Arch.select(query)
            found_labels = sorted([row[0] for row in data])
            expected_labels = sorted([office_space.Label, living_space.Label])
            self.assertListEqual(
                found_labels,
                expected_labels,
                "Should find spaces on the ground floor by parent's placement",
            )

        # === Advanced Cross-Feature Permutation Tests ===

        with self.subTest(description="Permutation: GROUP BY on a PPA result"):
            query = "SELECT PARENT(*).Label AS FloorName, COUNT(*) FROM document WHERE IfcType = 'Space' GROUP BY PARENT(*).Label ORDER BY FloorName"
            _, data = Arch.select(query)
            # Expected: Ground Floor has 2 spaces, Upper Floor has 1.
            self.assertEqual(len(data), 2)
            self.assertEqual(data[0][0], ground_floor.Label)
            self.assertEqual(data[0][1], 2)
            self.assertEqual(data[1][0], upper_floor.Label)
            self.assertEqual(data[1][1], 1)

        with self.subTest(description="Permutation: GROUP BY on a Function result"):
            query = "SELECT TYPE(*) AS BimType, COUNT(*) FROM document WHERE IfcType IS NOT NULL GROUP BY TYPE(*) ORDER BY BimType"
            _, data = Arch.select(query)
            results_dict = {row[0]: row[1] for row in data}
            self.assertGreaterEqual(results_dict.get("Wall", 0), 2)
            self.assertGreaterEqual(results_dict.get("Space", 0), 2)

        with self.subTest(description="Permutation: Complex WHERE with PPA and Functions"):
            query = f"SELECT Label FROM document WHERE TYPE(*) = 'Wall' AND LOWER(PARENT(*).Label) = 'ground floor' AND FireRating IS NOT NULL"
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], exterior_wall.Label)

        with self.subTest(description="Permutation: Filtering by a custom property on a parent"):
            query = "SELECT Label FROM document WHERE PARENT(*).FireRating = '60 minutes' AND IfcType IN ('Door', 'Window')"
            _, data = Arch.select(query)
            found_labels = sorted([row[0] for row in data])
            expected_labels = sorted([front_door.Label, living_window.Label])
            self.assertListEqual(found_labels, expected_labels)

        with self.subTest(description="Permutation: Arithmetic with parent properties"):
            # The Interior Partition has height 3000, its parent (Ground Floor) has height 3200.
            query = (
                f"SELECT Label FROM document WHERE TYPE(*) = 'Wall' AND Height < PARENT(*).Height"
            )
            _, data = Arch.select(query)
            self.assertEqual(len(data), 1)
            self.assertEqual(data[0][0], interior_wall.Label)

    def test_group_by_with_function_and_count(self):
        """
        Tests that GROUP BY correctly partitions results based on a function (TYPE)
        and aggregates them with another function (COUNT). This is the canonical
        non-regression test for the core GROUP BY functionality.
        """
        # ARRANGE: Create a simple, self-contained model for this test.
        # This makes the test independent of the main setUp fixture.
        doc = self.document  # Use the document created by TestArchBase
        Arch.makeWall(name="Unit Test Wall 1")
        Arch.makeWall(name="Unit Test Wall 2")
        Arch.makeSpace(baseobj=doc.addObject("Part::Box"), name="Unit Test Space")
        doc.recompute()

        # ACT: Run the query with GROUP BY a function expression.
        query = "SELECT TYPE(*) AS BimType, COUNT(*) FROM document WHERE Label LIKE 'Unit Test %' AND IfcType IS NOT NULL GROUP BY TYPE(*)"
        _, data = Arch.select(query)
        engine_results_dict = {row[0]: row[1] for row in data}

        # ASSERT: The results must be correctly grouped and counted.
        # We only check for the objects created within this test.
        expected_counts = {
            "Wall": 2,
            "Space": 1,
        }

        # The assertion should check that the expected items are a subset of the results,
        # as the main test fixture might still be present.
        self.assertDictContainsSubset(expected_counts, engine_results_dict)

    def test_group_by_chained_parent_function(self):
        """
        Tests GROUP BY on a complex expression involving a chained function
        call (PPA), ensuring the engine's signature generation and grouping
        logic can handle nested extractors.
        """
        # ARRANGE: Use the complex model from the ppa_and_query_permutations test
        model = create_test_model(self.document)
        ground_floor = model["ground_floor"]
        upper_floor = model["upper_floor"]  # Has one space

        # Add one more space to the upper floor for a meaningful group
        upper_box = self.document.addObject("Part::Box", "UpperSpaceVolume2")
        upper_box.Length, upper_box.Width, upper_box.Height = 1000.0, 1000.0, 3000.0
        upper_space2 = Arch.makeSpace(baseobj=upper_box, name="Upper Space 2")
        upper_floor.addObject(upper_space2)
        self.document.recompute()

        # ACT: Group windows and doors by the Label of their great-grandparent (the Floor)
        query = """
            SELECT PARENT(*).PARENT(*).Label AS FloorName, COUNT(*)
            FROM document
            WHERE IfcType IN ('Door', 'Window')
            GROUP BY PARENT(*).PARENT(*).Label
        """
        _, data = Arch.select(query)
        results_dict = {row[0]: row[1] for row in data}

        # ASSERT: The ground floor should contain 2 items (1 door, 1 window)
        self.assertEqual(results_dict.get(ground_floor.Label), 2)

    def test_group_by_multiple_mixed_columns(self):
        """
        Tests GROUP BY with multiple columns of different types (a property
        and a function result) to verify multi-part key generation.
        """
        # ARRANGE: Add a second column to the test fixture for a better test case
        Arch.makeStructure(length=300, width=330, height=2500, name="Second Column")
        self.document.recompute()

        # ACT
        query = "SELECT IfcType, TYPE(*), COUNT(*) FROM document GROUP BY IfcType, TYPE(*)"
        _, data = Arch.select(query)

        # ASSERT: Find the specific row for IfcType='Column' and TYPE='Column'
        column_row = next((row for row in data if row[0] == "Column" and row[1] == "Column"), None)
        self.assertIsNotNone(column_row, "A group for (Column, Column) should exist.")
        self.assertEqual(column_row[2], 2, "The count for (Column, Column) should be 2.")

    def test_invalid_group_by_with_aggregate_raises_error(self):
        """
        Ensures the engine's validation correctly rejects an attempt to
        GROUP BY an aggregate function, which is invalid SQL.
        """
        query = "SELECT IfcType, COUNT(*) FROM document GROUP BY COUNT(*)"

        # The "unsafe" select() API should raise the validation error
        with self.assertRaisesRegex(ArchSql.SqlEngineError, "must appear in the GROUP BY clause"):
            Arch.select(query)

    def test_where_clause_with_arithmetic(self):
        """
        Tests that the WHERE clause can correctly filter rows based on an
        arithmetic calculation involving multiple properties. This verifies
        that the arithmetic engine is correctly integrated into the filtering
        logic.
        """
        # ARRANGE: Create two walls with different dimensions.
        # Wall 1 Area = 1000 * 200 = 200,000
        large_wall = Arch.makeWall(name="Unit Test Large Wall", length=1000, width=200)
        # Wall 2 Area = 500 * 200 = 100,000
        _ = Arch.makeWall(name="Unit Test Small Wall", length=500, width=200)
        self.document.recompute()

        # ACT: Select walls where the calculated area is greater than 150,000.
        query = (
            "SELECT Label FROM document WHERE Label LIKE 'Unit Test %' AND Length * Width > 150000"
        )
        _, data = Arch.select(query)
        print(data)

        # ASSERT: Only the "Large Wall" should be returned.
        self.assertEqual(len(data), 1, "The query should find exactly one matching wall.")
        self.assertEqual(
            data[0][0], f"{large_wall.Label}", "The found wall should be the large one."
        )

    def test_select_with_nested_functions(self):
        """
        Tests the engine's ability to handle a function (CONCAT) whose
        arguments are a mix of properties, literals, and another function
        (TYPE). This is a stress test for the recursive expression evaluator
        and signature generator.
        """
        # ARRANGE: Create a single, predictable object.
        Arch.makeWall(name="My Test Wall")
        self.document.recompute()

        # ACT: Construct a complex string using nested function calls.
        query = "SELECT CONCAT(Label, ' (Type: ', TYPE(*), ')') FROM document WHERE Label = 'My Test Wall'"
        _, data = Arch.select(query)

        # ASSERT: The engine should correctly evaluate all parts and concatenate them.
        self.assertEqual(len(data), 1, "The query should have found the target object.")
        expected_string = "My Test Wall (Type: Wall)"
        self.assertEqual(
            data[0][0],
            expected_string,
            "The nested function expression was not evaluated correctly.",
        )

    def test_group_by_with_alias_is_not_supported(self):
        """
        Tests that GROUP BY with a column alias is not supported, as per the
        dialect's known limitations. This test verifies that the engine's
        validation correctly rejects this syntax.
        """
        # ARRANGE: A single object is sufficient for this validation test.
        Arch.makeWall(name="Test Wall For Alias")
        self.document.recompute()

        # ACT: Use the "incorrect" syntax where GROUP BY refers to an alias.
        query = "SELECT TYPE(*) AS BimType, COUNT(*) FROM document GROUP BY BimType"

        # ASSERT: The engine's validator must raise an SqlEngineError because
        # the signature of the SELECT column ('TYPE(*)') does not match the
        # signature of the GROUP BY column ('BimType').
        with self.assertRaisesRegex(ArchSql.SqlEngineError, "must appear in the GROUP BY clause"):
            Arch.select(query)

    def test_order_by_with_alias_is_supported(self):
        """
        Tests the supported ORDER BY behavior: sorting by an alias of a
        function expression that is present in the SELECT list.
        """
        # ARRANGE: Create objects that require case-insensitive sorting.
        Arch.makeWall(name="Wall_C")
        Arch.makeWall(name="wall_b")
        Arch.makeWall(name="WALL_A")
        self.document.recompute()

        # ACT: Use the correct syntax: include the expression in SELECT with an
        # alias, and then ORDER BY that alias.
        query = "SELECT Label, LOWER(Label) AS sort_key FROM document WHERE Label LIKE 'Wall_%' ORDER BY sort_key ASC"
        _, data = Arch.select(query)

        # Extract the original labels from the correctly sorted results.
        sorted_labels = [row[0] for row in data]

        # ASSERT: The results must be sorted correctly, proving the logic works.
        expected_order = ["WALL_A", "wall_b", "Wall_C"]
        self.assertListEqual(sorted_labels, expected_order)

    def test_order_by_with_raw_expression_is_not_supported(self):
        """
        Tests the unsupported ORDER BY behavior, documenting that the engine
        correctly rejects a query that tries to sort by a raw expression
        not present in the SELECT list.
        """
        # ARRANGE: A single object is sufficient for this validation test.
        Arch.makeWall(name="Test Wall")
        self.document.recompute()

        # ACT: Use the incorrect syntax.
        query = "SELECT Label FROM document ORDER BY LOWER(Label) ASC"

        # ASSERT: The engine's transformer must raise an error with a clear
        # message explaining the correct syntax.
        with self.assertRaisesRegex(
            ArchSql.SqlEngineError, "ORDER BY expressions are not supported directly"
        ):
            Arch.select(query)

    def test_core_engine_enhancements_for_pipeline(self):
        """
        Tests the Stage 1 enhancements to the internal SQL engine.
        This test validates both regression (ensuring old functions still work)
        and the new ability to query against a pre-filtered list of objects.
        """
        # --- 1. ARRANGE: Create a specific subset of objects for the test ---
        # The main test setup already provides a diverse set of objects.
        # We will create a specific list to act as our pipeline's source data.
        pipeline_source_objects = [self.wall_ext, self.wall_int, self.window]
        pipeline_source_labels = sorted([o.Label for o in pipeline_source_objects])
        self.assertEqual(
            len(pipeline_source_objects),
            3,
            "Pre-condition failed: Source object list should have 3 items.",
        )

        # --- 2. ACT & ASSERT (REGRESSION TEST) ---
        # First, prove that the existing public APIs still work perfectly.
        # This test implicitly calls the original code path of _run_query where
        # source_objects is None.
        with self.subTest(description="Regression test for Arch.select"):
            _, results_data = Arch.select('SELECT Label FROM document WHERE IfcType = "Wall"')
            found_labels = sorted([row[0] for row in results_data])
            self.assertListEqual(found_labels, sorted([self.wall_ext.Label, self.wall_int.Label]))

        with self.subTest(description="Regression test for Arch.count"):
            count, error = Arch.count('SELECT * FROM document WHERE IfcType = "Wall"')
            self.assertIsNone(error)
            self.assertEqual(count, 2)

        # --- 3. ACT & ASSERT (NEW FUNCTIONALITY TEST) ---
        # Now, test the new core functionality by calling the enhanced _run_query directly.
        with self.subTest(description="Test _run_query with a source_objects list"):
            # This query selects all objects (*) but should only run on our source list.
            query = "SELECT * FROM document"

            # Execute the query, passing our specific list as the source.
            _, data_rows, resulting_objects = ArchSql._run_query(
                query, mode="full_data", source_objects=pipeline_source_objects
            )

            # Assertions for the new behavior:
            # a) The number of data rows should match the size of our source list.
            self.assertEqual(
                len(data_rows),
                3,
                "_run_query did not return the correct number of rows for the provided source.",
            )

            # b) The content of the data should match the objects from our source list.
            found_labels = sorted([row[0] for row in data_rows])
            self.assertListEqual(
                found_labels,
                pipeline_source_labels,
                "The data returned does not match the source objects.",
            )

            # c) The new third return value, `resulting_objects`, should contain the correct FreeCAD objects.
            self.assertEqual(
                len(resulting_objects), 3, "The returned object list has the wrong size."
            )
            self.assertIsInstance(
                resulting_objects[0],
                FreeCAD.DocumentObject,
                "The resulting_objects list should contain DocumentObject instances.",
            )
            resulting_object_labels = sorted([o.Label for o in resulting_objects])
            self.assertListEqual(
                resulting_object_labels,
                pipeline_source_labels,
                "The list of resulting objects is incorrect.",
            )

        with self.subTest(description="Test _run_query with filtering on a source_objects list"):
            # This query applies a WHERE clause to the pre-filtered source list.
            query = "SELECT Label FROM document WHERE IfcType = 'Wall'"

            _, data_rows, resulting_objects = ArchSql._run_query(
                query, mode="full_data", source_objects=pipeline_source_objects
            )

            # Of the 3 source objects, only the 2 walls should be returned.
            self.assertEqual(len(data_rows), 2, "Filtering on the source object list failed.")
            found_labels = sorted([row[0] for row in data_rows])
            expected_labels = sorted([self.wall_ext.Label, self.wall_int.Label])
            self.assertListEqual(
                found_labels,
                expected_labels,
                "The data returned after filtering the source is incorrect.",
            )
            self.assertEqual(
                len(resulting_objects),
                2,
                "The object list returned after filtering the source is incorrect.",
            )

    def test_execute_pipeline_orchestrator(self):
        """
        Tests the new `execute_pipeline` orchestrator function in ArchSql.
        """

        # --- ARRANGE: Create a set of statements for various scenarios ---

        # Statement 1: Get all Wall objects. (Result: 2 objects)
        stmt1 = ArchSql.ReportStatement(
            query_string="SELECT * FROM document WHERE IfcType = 'Wall'", is_pipelined=False
        )

        # Statement 2: From the walls, get the one with "Exterior" in its name. (Result: 1 object)
        stmt2 = ArchSql.ReportStatement(
            query_string="SELECT * FROM document WHERE Label LIKE '%Exterior%'", is_pipelined=True
        )

        # Statement 3: A standalone query to get the Column object. (Result: 1 object)
        stmt3 = ArchSql.ReportStatement(
            query_string="SELECT * FROM document WHERE IfcType = 'Column'", is_pipelined=False
        )

        # Statement 4: A pipelined query that will run on an empty set from a failing previous step.
        stmt4_failing = ArchSql.ReportStatement(
            query_string="SELECT * FROM document WHERE IfcType = 'NonExistentType'",
            is_pipelined=False,
        )
        stmt5_piped_from_fail = ArchSql.ReportStatement(
            query_string="SELECT * FROM document", is_pipelined=True
        )

        # --- ACT & ASSERT ---

        with self.subTest(description="Test a simple two-step pipeline"):
            statements = [stmt1, stmt2]
            results_generator = ArchSql.execute_pipeline(statements)

            # The generator should yield exactly one result: the final one from stmt2.
            output_list = list(results_generator)
            self.assertEqual(
                len(output_list), 1, "A simple pipeline should only yield one final result."
            )

            # Check the content of the single yielded result.
            result_stmt, _, result_data = output_list[0]
            self.assertIs(
                result_stmt, stmt2, "The yielded statement should be the last one in the chain."
            )
            self.assertEqual(
                len(result_data), 1, "The final pipeline result should contain one row."
            )
            self.assertEqual(
                result_data[0][0],
                self.wall_ext.Label,
                "The final result is not the expected 'Exterior Wall'.",
            )

        with self.subTest(description="Test a mixed report with pipeline and standalone"):
            statements = [stmt1, stmt2, stmt3]
            results_generator = ArchSql.execute_pipeline(statements)

            # The generator should yield two results: the end of the first pipeline (stmt2)
            # and the standalone statement (stmt3).
            output_list = list(results_generator)
            self.assertEqual(len(output_list), 2, "A mixed report should yield two results.")

            # Check the first result (from the pipeline)
            self.assertEqual(output_list[0][2][0][0], self.wall_ext.Label)
            # Check the second result (from the standalone query)
            self.assertEqual(output_list[1][2][0][0], self.column.Label)

        with self.subTest(description="Test a pipeline that runs dry"):
            statements = [stmt4_failing, stmt5_piped_from_fail]
            results_generator = ArchSql.execute_pipeline(statements)
            output_list = list(results_generator)

            # The generator should yield only one result: the final, empty output
            # of the pipeline. The intermediate step's result should be suppressed.
            self.assertEqual(len(output_list), 1)

            # Check that the single yielded result has zero data rows.
            result_stmt, _, result_data = output_list[0]
            self.assertIs(result_stmt, stmt5_piped_from_fail)
            self.assertEqual(
                len(result_data), 0, "The final pipelined statement should yield 0 rows."
            )

    def test_public_api_for_pipelines(self):
        """
        Tests the new and enhanced public API functions for Stage 3.
        """
        # --- Test 1: Enhanced Arch.count() with source_objects ---
        with self.subTest(description="Test Arch.count with a source_objects list"):
            # Create a source list containing only the two wall objects.
            source_list = [self.wall_ext, self.wall_int]

            # This query would normally find 1 object (the column) in the full document.
            query = "SELECT * FROM document WHERE IfcType = 'Column'"

            # Run the count against our pre-filtered source list.
            count, error = ArchSql.count(query, source_objects=source_list)

            self.assertIsNone(error)
            # The count should be 0, because there are no 'Column' objects in our source_list.
            self.assertEqual(count, 0, "Arch.count failed to respect the source_objects list.")

        # --- Test 2: New Arch.selectObjectsFromPipeline() ---
        with self.subTest(description="Test Arch.selectObjectsFromPipeline"):
            # Define a simple two-step pipeline.
            stmt1 = ArchSql.ReportStatement(
                query_string="SELECT * FROM document WHERE IfcType = 'Wall'", is_pipelined=False
            )
            stmt2 = ArchSql.ReportStatement(
                query_string="SELECT * FROM document WHERE Label LIKE '%Exterior%'",
                is_pipelined=True,
            )

            # Execute the pipeline via the new high-level API.
            resulting_objects = Arch.selectObjectsFromPipeline([stmt1, stmt2])

            # Assert that the result is correct.
            self.assertIsInstance(resulting_objects, list)
            self.assertEqual(
                len(resulting_objects), 1, "Pipeline should result in one final object."
            )
            self.assertIsInstance(resulting_objects[0], FreeCAD.DocumentObject)
            self.assertEqual(
                resulting_objects[0].Name,
                self.wall_ext.Name,
                "The final object from the pipeline is incorrect.",
            )

    def test_pipeline_with_children_function(self):
        """
        Tests that the CHILDREN function correctly uses the input from a
        previous pipeline step instead of running its own subquery.
        """
        # --- ARRANGE ---
        # Create a parent Floor and a Wall that is a child of that floor.
        floor = Arch.makeFloor(name="Pipeline Test Floor")
        wall = Arch.makeWall(name="Wall on Test Floor")
        floor.addObject(wall)

        # Create a "distractor" wall that is NOT a child, to prove the filter works.
        _ = Arch.makeWall(name="Unrelated Distractor Wall")
        self.doc.recompute()

        # Define a two-step pipeline.
        # Step 1: Select only the 'Pipeline Test Floor'.
        stmt1 = ArchReport.ReportStatement(
            query_string="SELECT * FROM document WHERE Label = 'Pipeline Test Floor'",
            is_pipelined=False,
        )

        # Step 2: Use CHILDREN to get the walls from the previous step's result.
        stmt2 = ArchReport.ReportStatement(
            query_string="SELECT * FROM CHILDREN(SELECT * FROM document) WHERE IfcType = 'Wall'",
            is_pipelined=True,
        )

        # --- ACT ---
        # Execute the pipeline and get the final list of objects.
        resulting_objects = Arch.selectObjectsFromPipeline([stmt1, stmt2])

        # --- ASSERT ---
        # With the bug present, `resulting_objects` will be an empty list,
        # causing this assertion to fail as expected.
        self.assertEqual(
            len(resulting_objects),
            1,
            "The pipeline should have resulted in exactly one child object.",
        )
        self.assertEqual(
            resulting_objects[0].Name, wall.Name, "The object found via the pipeline is incorrect."
        )

    def test_group_by_with_function_and_literal_argument(self):
        """
        Tests that a GROUP BY clause with a function that takes a literal
        string argument (e.g., CONVERT(Area, 'm^2')) does not crash the
        validation engine. This is the non-regression test for the TypeError
        found in the _get_extractor_signature method.
        """
        # ARRANGE: Create a single object with a Quantity property.
        # A 1000x1000 box gives an area of 1,000,000 mm^2, which is 1 m^2.
        base_box = self.doc.addObject("Part::Box", "BaseBoxForConvertTest")
        base_box.Length = 1000
        base_box.Width = 1000
        space = Arch.makeSpace(base_box, name="SpaceForGroupByConvertTest")
        self.doc.recompute()

        # ACT: Construct the query that was causing the crash.
        query = """
            SELECT
                Label,
                CONVERT(Area, 'm^2')
            FROM
                document
            WHERE
                Label = 'SpaceForGroupByConvertTest'
            GROUP BY
                Label, CONVERT(Area, 'm^2')
        """

        # ASSERT: The query should now execute without any exceptions.
        headers, results_data = Arch.select(query)

        # Assertions for the passing test
        self.assertEqual(len(results_data), 1, "The query should return exactly one row.")
        self.assertEqual(headers, ["Label", "CONVERT(Area, 'm^2')"])

        # Check the content of the result
        self.assertEqual(results_data[0][0], space.Label)
        # Correctly call assertAlmostEqual with the message as a keyword argument
        self.assertAlmostEqual(results_data[0][1], 1.0, msg="The converted area should be 1.0 m^2.")

    def test_traverse_finds_all_descendants(self):
        """
        Tests that the basic recursive traversal finds all nested objects in a
        simple hierarchy, following both containment (.Group) and hosting (.Hosts)
        relationships. This is the first validation step for the new core
        traversal function.
        """
        # ARRANGE: Create a multi-level hierarchy (Floor -> Wall -> Window)
        floor = Arch.makeFloor(name="TraversalTestFloor")
        wall = Arch.makeWall(name="TraversalTestWall")
        win_profile = Draft.makeRectangle(1000, 1000)
        window = Arch.makeWindow(win_profile, name="TraversalTestWindow")

        # Establish the relationships
        floor.addObject(wall)  # Floor contains Wall
        Arch.addComponents(window, host=wall)  # Wall hosts Window
        self.doc.recompute()

        # ACT: Run the traversal starting from the top-level object
        # We expect the initial object to be included in the results by default.
        results = ArchSql._traverse_architectural_hierarchy([floor])
        result_labels = sorted([obj.Label for obj in results])

        # ASSERT: The final list must contain the initial object and all its descendants.
        expected_labels = sorted(["TraversalTestFloor", "TraversalTestWall", "TraversalTestWindow"])

        self.assertEqual(len(results), 3, "The traversal should have found 3 objects.")
        self.assertListEqual(
            result_labels,
            expected_labels,
            "The list of discovered objects does not match the expected hierarchy.",
        )

    def test_traverse_skips_generic_groups_in_results(self):
        """
        Tests that the traversal function transparently navigates through
        generic App::DocumentObjectGroup objects but does not include them
        in the final result set, ensuring the output is architecturally
        significant.
        """
        # ARRANGE: Create a hierarchy with a generic group in the middle
        # Floor -> Generic Group -> Space
        floor = Arch.makeFloor(name="GroupTestFloor")
        group = self.doc.addObject("App::DocumentObjectGroup", "GenericTestGroup")
        space_profile = Draft.makeRectangle(500, 500)
        space = Arch.makeSpace(space_profile, name="GroupTestSpace")

        # Establish the relationships
        floor.addObject(group)
        group.addObject(space)
        self.doc.recompute()

        # ACT: Run the traversal, but this time with a flag to exclude groups
        # The new `include_groups_in_result=False` parameter will be used here.
        results = ArchSql._traverse_architectural_hierarchy([floor], include_groups_in_result=False)
        result_labels = sorted([obj.Label for obj in results])

        # ASSERT: The final list must contain the floor and the space,
        # but NOT the generic group.
        expected_labels = sorted(["GroupTestFloor", "GroupTestSpace"])

        self.assertEqual(
            len(results), 2, "The traversal should have found 2 objects (and skipped the group)."
        )
        self.assertListEqual(
            result_labels,
            expected_labels,
            "The traversal incorrectly included the generic group in its results.",
        )

    def test_traverse_respects_max_depth(self):
        """
        Tests that the `max_depth` parameter correctly limits the depth of the
        hierarchical traversal.
        """
        # ARRANGE: Create a 3-level hierarchy (Floor -> Wall -> Window)
        floor = Arch.makeFloor(name="DepthTestFloor")
        wall = Arch.makeWall(name="DepthTestWall")
        win_profile = Draft.makeRectangle(1000, 1000)
        window = Arch.makeWindow(win_profile, name="DepthTestWindow")

        floor.addObject(wall)
        Arch.addComponents(window, host=wall)
        self.doc.recompute()

        # --- ACT & ASSERT ---

        # Sub-Test 1: max_depth = 1 (should find direct children only)
        with self.subTest(depth=1):
            results_depth_1 = ArchSql._traverse_architectural_hierarchy([floor], max_depth=1)
            labels_depth_1 = sorted([o.Label for o in results_depth_1])
            expected_labels_1 = sorted(["DepthTestFloor", "DepthTestWall"])
            self.assertListEqual(
                labels_depth_1,
                expected_labels_1,
                "With max_depth=1, should only find direct children.",
            )

        # Sub-Test 2: max_depth = 2 (should find grandchildren)
        with self.subTest(depth=2):
            results_depth_2 = ArchSql._traverse_architectural_hierarchy([floor], max_depth=2)
            labels_depth_2 = sorted([o.Label for o in results_depth_2])
            expected_labels_2 = sorted(["DepthTestFloor", "DepthTestWall", "DepthTestWindow"])
            self.assertListEqual(
                labels_depth_2, expected_labels_2, "With max_depth=2, should find grandchildren."
            )

        # Sub-Test 3: max_depth = 0 (unlimited, should find all)
        with self.subTest(depth=0):
            results_depth_0 = ArchSql._traverse_architectural_hierarchy([floor], max_depth=0)
            labels_depth_0 = sorted([o.Label for o in results_depth_0])
            expected_labels_0 = sorted(["DepthTestFloor", "DepthTestWall", "DepthTestWindow"])
            self.assertListEqual(
                labels_depth_0, expected_labels_0, "With max_depth=0, should find all descendants."
            )

    def test_sql_children_and_children_recursive_functions(self):
        """
        Performs a full integration test of the CHILDREN and CHILDREN_RECURSIVE
        SQL functions, ensuring they are correctly registered with the engine
        and call the traversal function with the correct parameters.
        """
        # ARRANGE: Create a multi-level hierarchy with a generic group
        # Building -> Floor -> Generic Group -> Wall -> Window
        building = Arch.makeBuilding(name="SQLFuncTestBuilding")
        floor = Arch.makeFloor(name="SQLFuncTestFloor")
        group = self.doc.addObject("App::DocumentObjectGroup", "SQLFuncTestGroup")
        wall = Arch.makeWall(name="SQLFuncTestWall")
        win_profile = Draft.makeRectangle(1000, 1000)
        window = Arch.makeWindow(win_profile, name="SQLFuncTestWindow")

        building.addObject(floor)
        floor.addObject(group)
        group.addObject(wall)
        Arch.addComponents(window, host=wall)
        self.doc.recompute()

        # --- Sub-Test 1: CHILDREN (non-recursive, depth=1) ---
        with self.subTest(function="CHILDREN"):
            query_children = """
                SELECT Label FROM CHILDREN(SELECT * FROM document WHERE Label = 'SQLFuncTestBuilding')
            """
            _, data = Arch.select(query_children)
            labels = sorted([row[0] for row in data])
            # Should only find the direct child (Floor), and not the group.
            self.assertListEqual(labels, ["SQLFuncTestFloor"])

        # --- Sub-Test 2: CHILDREN_RECURSIVE (default depth) ---
        with self.subTest(function="CHILDREN_RECURSIVE"):
            query_recursive = """
                SELECT Label FROM CHILDREN_RECURSIVE(SELECT * FROM document WHERE Label = 'SQLFuncTestBuilding')
            """
            _, data = Arch.select(query_recursive)
            labels = sorted([row[0] for row in data])
            # Should find all descendants, but skip the generic group.
            expected = sorted(["SQLFuncTestFloor", "SQLFuncTestWall", "SQLFuncTestWindow"])
            self.assertListEqual(labels, expected)

        # --- Sub-Test 3: CHILDREN_RECURSIVE (with max_depth parameter) ---
        with self.subTest(function="CHILDREN_RECURSIVE with depth=2"):
            query_recursive_depth = """
                SELECT Label FROM CHILDREN_RECURSIVE(SELECT * FROM document WHERE Label = 'SQLFuncTestBuilding', 2)
            """
            _, data = Arch.select(query_recursive_depth)
            labels = sorted([row[0] for row in data])
            # Should find Floor (depth 1) and Wall (depth 2), but not Window (depth 3).
            # The generic group at depth 2 is traversed but skipped in results.
            expected = sorted(["SQLFuncTestFloor", "SQLFuncTestWall"])
            self.assertListEqual(labels, expected)

    def test_default_header_uses_internal_units(self):
        """
        Tests that when a Quantity property is selected, the generated header
        uses the object's internal unit (e.g., 'mm') to match the raw data.
        This test temporarily changes the unit schema to ensure it is
        independent of user preferences.
        """
        # ARRANGE: Get the user's current schema to restore it later.
        original_schema_index = FreeCAD.Units.getSchema()

        try:
            # Get the list of available schema names.
            schema_names = FreeCAD.Units.listSchemas()
            # Find the index for "Meter decimal", which is guaranteed to use 'm'.
            meter_schema_index = schema_names.index("MeterDecimal")

            # Set the schema, forcing getUserPreferred() to return 'm'.
            FreeCAD.Units.setSchema(meter_schema_index)

            # ARRANGE: Create a simple object with a known internal unit ('mm').
            box = self.doc.addObject("Part::Box", "UnitHeaderTestBox")
            box.Length = 1500.0  # This is 1500 mm
            self.doc.recompute()

            report = Arch.makeReport(name="UnitHeaderTestReport")
            report.Proxy.live_statements[0].query_string = (
                "SELECT Label, Length FROM document WHERE Name = 'UnitHeaderTestBox'"
            )
            report.Proxy.commit_statements()

            # ACT: Execute the report.
            self.doc.recompute()

            # ASSERT: Check the headers in the resulting spreadsheet.
            spreadsheet = report.Target
            self.assertIsNotNone(spreadsheet)

            header_length = spreadsheet.get("B1")

            self.assertEqual(header_length, "Length (mm)")

        finally:
            # CLEANUP: Always restore the user's original schema.
            FreeCAD.Units.setSchema(original_schema_index)

    def test_numeric_comparisons_on_quantities(self):
        """
        Tests that all numeric comparison operators (>, <, >=, <=, =, !=)
        work correctly on Quantity properties, independent of the current
        unit schema. This ensures numeric comparisons are not affected by
        string formatting or locales.
        """
        # ARRANGE: Get the user's current schema to restore it later.
        original_schema_index = FreeCAD.Units.getSchema()

        try:
            # Set a "smart" schema (MKS) that uses different display units
            # based on thresholds. This creates the most challenging scenario
            # for string-based comparisons.
            schema_names = FreeCAD.Units.listSchemas()
            mks_schema_index = schema_names.index("MKS")
            FreeCAD.Units.setSchema(mks_schema_index)

            # ARRANGE: Create a set of objects above, below, and at the threshold.
            threshold = 8000.0
            test_prefix = "NumericTestWall_"
            Arch.makeWall(name=test_prefix + "TallWall", height=threshold + 2000)
            Arch.makeWall(name=test_prefix + "ShortWall", height=threshold - 1000)
            Arch.makeWall(name=test_prefix + "ExactWall", height=threshold)
            self.doc.recompute()

            test_cases = {
                ">": [test_prefix + "TallWall"],
                "<": [test_prefix + "ShortWall"],
                ">=": [test_prefix + "TallWall", test_prefix + "ExactWall"],
                "<=": [test_prefix + "ShortWall", test_prefix + "ExactWall"],
                "=": [test_prefix + "ExactWall"],
                "!=": [test_prefix + "TallWall", test_prefix + "ShortWall"],
            }

            for op, expected_names in test_cases.items():
                with self.subTest(operator=op):
                    # ACT: The query is isolated to only the walls from this test.
                    query = f"SELECT Label FROM document WHERE Label LIKE '{test_prefix}%' AND Height {op} {threshold}"
                    _, results_data = Arch.select(query)

                    # ASSERT: Check that the correct objects were returned.
                    result_labels = [row[0] for row in results_data]
                    self.assertCountEqual(
                        result_labels,
                        expected_names,
                        f"Query with operator '{op}' returned incorrect objects.",
                    )

        finally:
            # CLEANUP: Always restore the user's original schema.
            FreeCAD.Units.setSchema(original_schema_index)
