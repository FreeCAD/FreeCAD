# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Shared discovery and subprocess handling for headless Blender operations."""

import os
from pathlib import Path
import shutil
import subprocess
import sys

import FreeCAD as App


class BlenderBridgeError(RuntimeError):
    """A headless Blender operation could not be completed."""


def _configured_blender():
    executable = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Forms").GetString(
        "BlenderExecutable", ""
    )
    if not executable:
        return None
    if not Path(executable).is_file():
        # An explicit choice must not be replaced by whichever Blender happens
        # to be found on PATH; it may be a different, incompatible version.
        raise BlenderBridgeError(
            f"The Forms preference 'BlenderExecutable' is set to '{executable}', "
            "which is not an existing file. Correct it, or clear it to find Blender "
            "automatically."
        )
    return executable


def _version_key(path):
    try:
        version = path.parent.name.removeprefix("Blender ")
        return tuple(int(part) for part in version.split("."))
    except ValueError:
        return ()


def find_blender_executable():
    """Return a configured or conventionally installed Blender executable.

    Raise BlenderBridgeError when the ``BlenderExecutable`` preference is set
    but does not name an existing file. Return None when no Blender is found.
    """
    configured = _configured_blender()
    if configured:
        return configured
    executable = shutil.which("blender")
    if executable:
        return executable
    candidates = []
    if os.name == "nt":
        for environment_name in ("ProgramFiles", "ProgramW6432"):
            root = os.environ.get(environment_name)
            if root:
                candidates.extend(Path(root).glob("Blender Foundation/Blender */blender.exe"))
    elif sys.platform == "darwin":
        candidates.append(Path("/Applications/Blender.app/Contents/MacOS/Blender"))
    candidates = {candidate.resolve() for candidate in candidates if candidate.is_file()}
    return str(max(candidates, key=_version_key)) if candidates else None


def run_blender_script(
    script,
    script_arguments,
    *,
    source_file=None,
    executable=None,
    timeout=180,
    operation="processing the file",
):
    """Run *script* in isolated background Blender and return its completed process."""
    executable = executable or find_blender_executable()
    if not executable:
        raise BlenderBridgeError(
            "Blender was not found. Install Blender 4.0 or newer, or set the Forms "
            "preference 'BlenderExecutable' to its executable."
        )
    command = [
        str(executable),
        "--background",
        "--factory-startup",
        "--disable-autoexec",
    ]
    if source_file is not None:
        # Blender parses this positional argument like its options, so a
        # relative name such as "-scene.blend" must not reach it verbatim.
        command.append(os.path.abspath(source_file))
    command.extend(
        [
            "--python-exit-code",
            "1",
            "--python",
            str(script),
            "--",
            *(str(argument) for argument in script_arguments),
        ]
    )
    startupinfo = None
    if os.name == "nt":
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            check=False,
            encoding="utf-8",
            errors="replace",
            startupinfo=startupinfo,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        raise BlenderBridgeError(
            f"Blender did not finish {operation} within {timeout} seconds"
        ) from error
    except OSError as error:
        raise BlenderBridgeError(f"Blender could not be started: {error}") from error
    if result.returncode != 0:
        details = (result.stderr or result.stdout).strip()
        if len(details) > 2000:
            details = details[-2000:]
        message = f"Blender failed while {operation}"
        if details:
            message += f":\n\n{details}"
        raise BlenderBridgeError(message)
    return result


__all__ = ["BlenderBridgeError", "find_blender_executable", "run_blender_script"]
