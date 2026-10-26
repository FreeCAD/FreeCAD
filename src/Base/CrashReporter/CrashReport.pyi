# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, forward_declarations
from Base.PyObjectBase import PyObjectBase
from typing import Final, Literal

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

    path_to_raw_report_file: Final[str]
    fault_address: Final[int | None]
    thread_id: Final[int]
    timestamp: Final[float]
    process_id: Final[int]
    fault_code: Final[int]
    fault_name: Final[str]
    partial_write: Final[bool]
    capture_was_signal_safe: Final[bool]
    build_id: Final[str | None]
    minidump_path: Final[str | None]
    os: Final[Literal["linux", "macos", "windows", "bsd", "unknown"]]
    os_version: Final[str | None]
    architecture: Final[Literal["x86_64", "aarch64", "unknown"]]
    freecad_version: Final[tuple[int, int, int, str]]
    symbolicated: Final[bool]
    stack_frames: Final[list[CrashFrame]]
