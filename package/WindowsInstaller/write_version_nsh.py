# SPDX-License-Identifier: LGPL-2.1-or-later
# NSIS runs this script through FreeCADCmd to generate version.nsh next to it.

from datetime import date
from pathlib import Path, PureWindowsPath
import sys

import FreeCAD


PAYLOAD_DIRS = ('bin', 'data', 'doc', 'Ext', 'lib', 'Mod')


def installed_path(path):
    return '$INSTDIR\\' + str(path).replace('$', '$$')


def remove_tree(source, dest, extra_files=()):
    for item in sorted(source.iterdir()):
        path = dest / item.name
        if item.is_dir():
            yield from remove_tree(item, path)
        else:
            yield f'Delete "{installed_path(path)}"'
    for item in extra_files:
        yield f'Delete "{installed_path(dest / item.name)}"'
    yield f'RMDir "{installed_path(dest)}"'


def payload_macros(root, deps):
    yield '!macro MatchPayloadDir NAME MATCH'
    for folder in PAYLOAD_DIRS:
        yield f'StrCmp ${{NAME}} "{folder}" ${{MATCH}}'
    yield '!macroend'

    yield '!macro InstallPayload'
    for folder in PAYLOAD_DIRS:
        yield f'SetOutPath "{installed_path(folder)}"'
        yield f'File /r "${{FILES_FREECAD}}\\{folder}\\*.*"'
        if folder == 'bin' and deps:
            yield r'File "${FILES_DEPS}\*.*"'
    yield '!macroend'

    # Use the same staging directories for both actions. Delete only shipped files.
    yield '!macro RemovePayload'
    redistributables = sorted(item for item in Path(deps).iterdir() if item.is_file()) if deps else []
    for folder in PAYLOAD_DIRS:
        yield from remove_tree(
            root / folder, PureWindowsPath(folder), redistributables if folder == 'bin' else ()
        )
    yield '!macroend'


root, deps = sys.argv[sys.argv.index('--pass') + 1:]
version = FreeCAD.Version()
with Path(__file__).with_name('version.nsh').open('w', encoding='utf-8') as output:
    output.write(f'''\
!define COPYRIGHT_YEAR {date.today().year}
!define APP_VERSION_MAJOR "{version[0]}"
!define APP_VERSION_MINOR "{version[1]}"
!define APP_VERSION_PATCH "{version[2]}"
!define APP_VERSION_REVISION "{version[3].split()[0]}"
!define APP_NAME "{FreeCAD.ConfigGet('ExeName')}"
''')
    output.write('\n'.join(payload_macros(Path(root), deps)) + '\n')
