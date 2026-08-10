# SPDX-License-Identifier: LGPL-2.1-or-later

"""Process backend for running FreeCADMbD from MbDFEM."""

import os
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import FreeCAD as App

import FreeCADMbDExporter
import FreeCADMbDResults


PREF_GROUP = "User parameter:BaseApp/Preferences/Mod/MbDFEM"
EXE_PREF = "FreeCADMbDExecutable"


@dataclass
class SolveResult:
    asmt_file: str
    result_file: str | None
    return_code: int
    stdout: str
    stderr: str


class FreeCADMbDProcessBackend:
    """Run FreeCADMbD as a separate process."""

    def __init__(self, executable_path=None, work_dir=None, timeout=None):
        self.executable_path = executable_path or configured_executable()
        self.work_dir = Path(work_dir) if work_dir else None
        self.timeout = timeout

    def solve(self, assembly, asmt_file=None):
        if not self.executable_path:
            raise RuntimeError(
                "FreeCADMbD executable is not configured. Set the MbDFEM "
                f"preference '{EXE_PREF}' or the FREECADMBD_EXE environment variable."
            )

        executable = Path(self.executable_path)
        if not executable.exists():
            raise RuntimeError(f"FreeCADMbD executable does not exist: {executable}")

        asmt_path = Path(asmt_file) if asmt_file else default_asmt_path(assembly)
        FreeCADMbDExporter.export_assembly(assembly, asmt_path)

        solved_asmt_path = asmt_path.with_suffix(".solved.asmt")
        command = [str(executable), str(asmt_path), str(solved_asmt_path)]
        completed = subprocess.run(
            command,
            cwd=str(self.work_dir or asmt_path.parent),
            text=True,
            capture_output=True,
            timeout=self.timeout,
            check=False,
        )

        if completed.returncode != 0:
            raise RuntimeError(
                "FreeCADMbD solve failed with exit code "
                f"{completed.returncode}.\n"
                f"Command: {' '.join(command)}\n"
                f"Working directory: {self.work_dir or asmt_path.parent}\n"
                f"ASMT file: {asmt_path}\n"
                f"stdout:\n{completed.stdout.strip()}\n"
                f"stderr:\n{completed.stderr.strip()}"
            )

        if solved_asmt_path.exists():
            FreeCADMbDResults.import_results(assembly, solved_asmt_path)

        return SolveResult(
            asmt_file=str(asmt_path),
            result_file=str(solved_asmt_path) if solved_asmt_path.exists() else None,
            return_code=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
        )


def configured_executable():
    env_path = os.environ.get("FREECADMBD_EXE")
    if env_path:
        return env_path

    parameter_group = App.ParamGet(PREF_GROUP)
    configured = parameter_group.GetString(EXE_PREF, "")
    if configured:
        return configured

    return shutil.which("FreeCADMbD.exe") or shutil.which("FreeCADMbD")


def default_asmt_path(assembly):
    document = assembly.Document
    if document and document.FileName:
        return Path(document.FileName).with_suffix(".asmt")
    return Path(App.getUserAppDataDir()) / f"{assembly.Name}.asmt"
