# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export, forward_declarations
from PyObjectBase import PyObjectBase
from typing import Final

from CrashFrame import CrashFrame

@export(
    PythonName="FreeCAD.CrashReport",
    Twin="ParsedCrashReport",
    TwinPointer="ParsedCrashReport",
    Include="Base/CrashReporter/Reader.h",
    Constructor=False,
    Delete=True,
)
@forward_declarations("""
    namespace Base {
        using ParsedCrashReport = CrashReporter::ParsedCrashReport;
    }""")
class CrashReport(PyObjectBase):
    """
    A complete crash report.
    """

    path_to_raw_report_file: Final[str] = ""
    fault_address: Final[int | None] = None
    thread_id: Final[int] = 0
    timestamp: Final[float] = 0.0
    process_id: Final[int] = 0
    fault_code: Final[int] = 0
    fault_name: Final[str] = ""
    partial_write: Final[bool] = False
    capture_was_signal_safe: Final[bool] = False
    build_id: Final[str | None] = None
    minidump_path: Final[str | None] = None
    os: Final[str] = "none"
    os_version: Final[str | None] = None
    architecture: Final[str] = "unknown"
    freecad_version: Final[tuple[int, int, int, str]] = (0, 0, 0, "")
    symbolicated: Final[bool] = False
    stack_frames: Final[list[CrashFrame]] = []
