# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, forward_declarations
from Base.PyObjectBase import PyObjectBase
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

    address: Final[int]

    module_offset: Final[int | None]

    module: Final[str]

    symbol: Final[str | None]

    file: Final[str | None]

    line: Final[int | None]

    is_inline: Final[bool]
