# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#  *                                                                          *
#  *   Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
#  *                                                                          *
#  *   This file is part of FreeCAD.                                          *
#  *                                                                          *
#  *   FreeCAD is free software: you can redistribute it and/or modify it     *
#  *   under the terms of the GNU Lesser General Public License as            *
#  *   published by the Free Software Foundation, either version 2.1 of the   *
#  *   License, or (at your option) any later version.                        *
#  *                                                                          *
#  *   FreeCAD is distributed in the hope that it will be useful, but         *
#  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
#  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#  *   Lesser General Public License for more details.                        *
#  *                                                                          *
#  *   You should have received a copy of the GNU Lesser General Public       *
#  *   License along with FreeCAD. If not, see                                *
#  *   <https://www.gnu.org/licenses/>.                                       *
#  *                                                                          *
#  ***************************************************************************/

import os
import time
import tempfile

import unittest

import zipfile
import shutil
import difflib

import FreeCAD


THUMBNAIL_FILE = "thumbnails/Thumbnail.png"
GUI_DOCUMENT_FILE = "GuiDocument.xml"
DOCUMENT_FILE = "Document.xml"
LINE_COLOR_ARRAY_FILE = "LineColorArray"
POINT_COLOR_ARRAY_FILE = "PointColorArray"
SHAPE_APPEARANCE_FILE = "ShapeAppearance"


def get_document_cache_files_from_file(filepath: str) -> set[str]:
    """Get the files that potentially target the document cache from a FreeCAD file.

    If there are files in the FreeCAD file that can potentially be cached (for
    example BRep files or thumbnails), then this function returns these.

    @param[in] filepath The file path of the FreeCAD document.
    @return The set of file names
    """

    zip_file = zipfile.ZipFile(filepath, "r")
    all_files = zip_file.namelist()
    return {f for f in all_files if f.endswith(".brp") or f == THUMBNAIL_FILE}


def get_document_cache_files_from_cache_dir(doc_cache_dir: str) -> set[str]:
    """Get the files from the document cache directory.

    The way this function returns the file names should be similar to how
    zip_file.namelist() returns them.

    @param[in] doc_cache_dir The document cache directory.
    @return The set of file names
    """
    base = os.fspath(doc_cache_dir)
    cache_files = set()
    for root, _, files in os.walk(base):
        rel_root = os.path.relpath(root, base)
        prefix = "" if rel_root == "." else rel_root.replace(os.sep, "/") + "/"
        for file in files:
            cache_files.add(prefix + file)
    return cache_files


def get_default_files() -> set[str]:
    """Get the default files that are expected to be in the document cache.

    @return The set of default file names
    """
    return {THUMBNAIL_FILE} if FreeCAD.GuiUp else set()


def get_bytes_zip_file(zipfile_path: str, filename: str) -> bytes:
    """Get the contents of a file from a zip file.

    @param[in] filepath The file path of the zip file.
    @param[in] filename The name of the file in the zip file.
    @return The content of the file.
    """
    with zipfile.ZipFile(zipfile_path, "r") as zip_file:
        with zip_file.open(filename) as f:
            return f.read()


def get_text_zip_file(zipfile_path: str, filename: str) -> str:
    """Get the content of a text file from a zip file.

    @param[in] filepath The file path of the zip file.
    @param[in] filename The name of the file in the zip file.
    @return The content of the text file.
    """
    return get_bytes_zip_file(zipfile_path, filename).decode("utf-8")


def get_diff(zipfile1_path: str, zipfile2_path: str, filename: str) -> str:
    """Get the diff of a file in two zip files.

    The result is a list of tuples, where each tuple contains the removed line
    and the added line.

    @param[in] zipfile1_path The file path of the first zip file.
    @param[in] zipfile2_path The file path of the second zip file.
    @return The diff of the file in the two zip files.

    """
    text_file1 = get_text_zip_file(zipfile1_path, filename)
    text_file2 = get_text_zip_file(zipfile2_path, filename)
    lines1 = text_file1.splitlines(keepends=True)
    lines2 = text_file2.splitlines(keepends=True)

    diff_output = list(
        difflib.unified_diff(lines1, lines2, fromfile=zipfile1_path, tofile=zipfile2_path, n=1)
    )
    added_lines = [line for line in diff_output if line.startswith("+ ")]
    removed_lines = [line for line in diff_output if line.startswith("- ")]

    def strip_diff_line(line):
        return line[2:].strip()

    return list(zip(map(strip_diff_line, removed_lines), map(strip_diff_line, added_lines)))


class VersioningTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_no_cache_dir(self):
        """Test whether the expected files are stored in the FreeCAD file.

        Test that when no cache directory is used, the files that are expected
        to be cached are actually stored in the FreeCAD file.
        """

        doc = FreeCAD.newDocument("VersioningTest")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()

        self.assertTrue(hasattr(box, "CanComputeShape"))
        self.assertTrue(box.CanComputeShape)
        self.assertTrue(hasattr(doc, "DocumentCacheDir"))
        self.assertTrue(doc.DocumentCacheDir == "")

        temp_dir = tempfile.mkdtemp()
        temp_file = os.path.join(temp_dir, "VersioningTest.FCStd")
        doc.saveAs(temp_file)
        FreeCAD.closeDocument(doc.Name)

        self.assertSetEqual(
            get_document_cache_files_from_file(temp_file),
            get_default_files().union({"Box.Shape.brp"}),
        )

        shutil.rmtree(temp_dir)

    def test_with_cache_dir(self):
        """Test whether the expected files are stored in the document cache.

        Test that when the document cache is used, the files that are expected
        to be cached are stored in the cache and not in the FreeCAD file.
        """
        doc = FreeCAD.newDocument("VersioningTest")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()

        self.assertTrue(hasattr(box, "CanComputeShape"))
        self.assertTrue(box.CanComputeShape)
        self.assertTrue(hasattr(doc, "DocumentCacheDir"))

        temp_dir = tempfile.mkdtemp()
        doc_cache_dir = os.path.join(temp_dir, "document-cache")
        os.mkdir(doc_cache_dir)

        doc.DocumentCacheDir = doc_cache_dir

        temp_file = os.path.join(temp_dir, "VersioningTest.FCStd")
        doc.saveAs(temp_file)
        FreeCAD.closeDocument(doc.Name)

        # Expected files not in the FreeCAD file.
        self.assertSetEqual(get_document_cache_files_from_file(temp_file), set())

        # Expected files in the document cache directory.
        self.assertSetEqual(
            get_document_cache_files_from_cache_dir(doc_cache_dir),
            get_default_files().union({"Box.Shape.brp"}),
        )

        shutil.rmtree(temp_dir)

    def test_diff(self):
        """Test whether the diff contains the expected differences.

        Test that when the document cache is used, the diff is minimal and only
        contains the expected differences.  In this case we only expect the
        change of the box length and the file name of the FreeCAD document.
        """
        doc = FreeCAD.newDocument("VersioningTest")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()

        self.assertTrue(hasattr(box, "CanComputeShape"))
        self.assertTrue(box.CanComputeShape)
        self.assertTrue(hasattr(doc, "DocumentCacheDir"))

        temp_dir = tempfile.mkdtemp()
        doc_cache_dir = os.path.join(temp_dir, "document-cache")
        os.mkdir(doc_cache_dir)

        # Prepare for precise diffs
        doc.DocumentCacheDir = doc_cache_dir
        doc.setPropertyStatus("LastModifiedDate", "Transient")

        temp_file_v1 = os.path.join(temp_dir, "VersioningTest1.FCStd")
        doc.saveAs(temp_file_v1)

        box.Length = 20
        doc.recompute()
        time.sleep(1)

        temp_file_v2 = os.path.join(temp_dir, "VersioningTest2.FCStd")
        doc.saveAs(temp_file_v2)
        FreeCAD.closeDocument(doc.Name)

        # Expected files not in the FreeCAD file.
        self.assertSetEqual(get_document_cache_files_from_file(temp_file_v1), set())
        self.assertSetEqual(get_document_cache_files_from_file(temp_file_v2), set())

        # Expected files in the document cache directory.
        self.assertSetEqual(
            get_document_cache_files_from_cache_dir(doc_cache_dir),
            get_default_files().union({"Box.Shape.brp"}),
        )

        if FreeCAD.GuiUp:
            self.assertEqual(
                get_text_zip_file(temp_file_v1, GUI_DOCUMENT_FILE),
                get_text_zip_file(temp_file_v2, GUI_DOCUMENT_FILE),
            )
            self.assertEqual(
                get_bytes_zip_file(temp_file_v1, LINE_COLOR_ARRAY_FILE),
                get_bytes_zip_file(temp_file_v2, LINE_COLOR_ARRAY_FILE),
            )
            self.assertEqual(
                get_bytes_zip_file(temp_file_v1, POINT_COLOR_ARRAY_FILE),
                get_bytes_zip_file(temp_file_v2, POINT_COLOR_ARRAY_FILE),
            )
            self.assertEqual(
                get_bytes_zip_file(temp_file_v1, SHAPE_APPEARANCE_FILE),
                get_bytes_zip_file(temp_file_v2, SHAPE_APPEARANCE_FILE),
            )

        diff = get_diff(temp_file_v1, temp_file_v2, DOCUMENT_FILE)
        expected_diff = [
            ('<String value="VersioningTest1"/>', '<String value="VersioningTest2"/>'),
            ('<Float value="10.0000000000000000"/>', '<Float value="20.0000000000000000"/>'),
        ]
        self.assertListEqual(diff, expected_diff)

        shutil.rmtree(temp_dir)

    def test_mark_to_recompute(self):
        """Tests whether objects without shapes are marked to be recomputed.

        When the document cache is used for a file, but shapes of objects are
        missing (for example if the file is opened on a different computer),
        then on opening the file, the objects that have missing shapes should
        be marked to be recomputed.
        """
        doc = FreeCAD.newDocument("VersioningTest")
        box = doc.addObject("Part::Box", "Box")
        doc.recompute()

        self.assertTrue(hasattr(box, "CanComputeShape"))
        self.assertTrue(box.CanComputeShape)
        self.assertTrue(hasattr(doc, "DocumentCacheDir"))

        temp_dir = tempfile.mkdtemp()
        doc_cache_dir = os.path.join(temp_dir, "document-cache")
        os.mkdir(doc_cache_dir)

        doc.DocumentCacheDir = doc_cache_dir

        temp_file = os.path.join(temp_dir, "VersioningTest.FCStd")
        doc.saveAs(temp_file)
        FreeCAD.closeDocument(doc.Name)

        os.remove(os.path.join(doc_cache_dir, "Box.Shape.brp"))

        doc = FreeCAD.open(temp_file)
        self.assertIn("Touched", doc.Box.State)
