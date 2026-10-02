# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export, forward_declarations
from PyObjectBase import PyObjectBase
from typing import Final

@export(
    PythonName="FreeCAD.CrashFrame",
    Twin="ParsedFrame",
    TwinPointer="ParsedFrame",
    Include="Base/CrashReporter/Reader.h",
    Constructor=False,
    Delete=True,
)
@forward_declarations("""
    namespace Base {
        using ParsedFrame = CrashReporter::ParsedFrame;
    }""")
class CrashFrame(PyObjectBase):
    """
    A single frame of data from a (possibly symbolicated) crash report.
    """

    address: Final[int] = 0

    module_offset: Final[int | None] = None

    module: Final[str] = ""

    symbol: Final[str | None] = None

    file: Final[str | None] = None

    line: Final[int | None] = None

    is_inline: Final[bool] = False
