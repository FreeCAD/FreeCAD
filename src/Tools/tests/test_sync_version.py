# SPDX-License-Identifier: LGPL-2.1-or-later

import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from sync_version import (
    VersionInfo,
    replace_in_toml_section,
    sync_workspace_pixi_toml,
    sync_fedora_spec,
    sync_desktop_file,
    sync_MainGui,
    sync_linux_create_bundle,
    run,
)


def make_version(
    name="FreeCAD",
    major=1,
    minor=2,
    patch=0,
    suffix="dev",
    build=0,
) -> VersionInfo:
    return VersionInfo(
        name=name, major=major, minor=minor, patch=patch, suffix=suffix, build=build
    )


class TestVersionInfo(unittest.TestCase):
    def test_simple_version(self):
        version = make_version(major=1, minor=2, patch=3)
        self.assertEqual(version.simple, "1.2.3")

    def test_complete_version_with_suffix(self):
        version = make_version(suffix="dev")
        self.assertEqual(version.complete, "1.2.0-dev")

    def test_complete_version_without_suffix(self):
        version = make_version(suffix="")
        self.assertEqual(version.complete, "1.2.0")

    def test_complete_version_rc(self):
        version = make_version(suffix="RC1")
        self.assertEqual(version.complete, "1.2.0-RC1")

    def test_rpm_version_with_suffix(self):
        version = make_version(suffix="dev")
        self.assertEqual(version.rpm, "1.2.0~dev")

    def test_rpm_version_without_suffix(self):
        version = make_version(suffix="")
        self.assertEqual(version.rpm, "1.2.0")

    def test_rpm_version_rc(self):
        version = make_version(suffix="RC1")
        self.assertEqual(version.rpm, "1.2.0~RC1")

    def test_conda_version_with_suffix(self):
        version = make_version(suffix="dev")
        self.assertEqual(version.conda, "1.2.0dev")

    def test_conda_version_without_suffix(self):
        version = make_version(suffix="")
        self.assertEqual(version.conda, "1.2.0")

    def test_conda_version_rc(self):
        version = make_version(suffix="RC1")
        self.assertEqual(version.conda, "1.2.0RC1")

    def test_calver_dev_version(self):
        version = make_version(major=26, minor=3, patch=0, suffix="dev")
        self.assertEqual(version.simple, "26.3.0")
        self.assertEqual(version.complete, "26.3.0-dev")
        self.assertEqual(version.rpm, "26.3.0~dev")
        self.assertEqual(version.conda, "26.3.0dev")

    def test_lowercase_name(self):
        version = make_version(name="FreeCAD")
        self.assertEqual(version.lowercase_name, "freecad")

    def test_from_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "version.json").write_text(
                json.dumps(
                    {
                        "name": "TestApp",
                        "version_major": 3,
                        "version_minor": 4,
                        "version_patch": 5,
                        "version_suffix": "beta",
                        "build_version": 2,
                    }
                ),
                encoding="utf-8",
            )
            version = VersionInfo.from_json(root)
            self.assertEqual(version.name, "TestApp")
            self.assertEqual(version.simple, "3.4.5")
            self.assertEqual(version.complete, "3.4.5-beta")
            self.assertEqual(version.build, 2)


class TestReplaceInTomlSection(unittest.TestCase):
    SAMPLE = (
        '[workspace]\nname = "FreeCAD"\nversion = "1.0.0"\n\n'
        '[dependencies]\nfoo = "*"\n'
    )

    def test_replaces_field_in_correct_section(self):
        result = replace_in_toml_section(self.SAMPLE, "[workspace]", "version", "2.0.0")
        self.assertIn('version = "2.0.0"', result)
        self.assertIn('name = "FreeCAD"', result)

    def test_does_not_modify_other_sections(self):
        result = replace_in_toml_section(self.SAMPLE, "[workspace]", "version", "2.0.0")
        self.assertIn('foo = "*"', result)

    def test_no_match_returns_unchanged(self):
        result = replace_in_toml_section(
            self.SAMPLE, "[nonexistent]", "version", "2.0.0"
        )
        self.assertEqual(result, self.SAMPLE)

    def test_key_not_in_section_returns_unchanged(self):
        result = replace_in_toml_section(self.SAMPLE, "[workspace]", "missing_key", "x")
        self.assertEqual(result, self.SAMPLE)

    def test_does_not_replace_key_in_wrong_section(self):
        content = '[section_a]\nversion = "1.0"\n\n[section_b]\nversion = "2.0"\n'
        result = replace_in_toml_section(content, "[section_a]", "version", "9.9")
        self.assertIn('version = "9.9"', result.split("[section_b]")[0])
        self.assertIn('version = "2.0"', result.split("[section_b]")[1])


def write_temp_file(directory: Path, name: str, content: str) -> Path:
    filepath = directory / name
    filepath.parent.mkdir(parents=True, exist_ok=True)
    filepath.write_text(content, encoding="utf-8")
    return filepath


WORKSPACE_PIXI_TOML = """\
[workspace]
name = "FreeCAD"
version = "1.0.0"
description = "pixi instructions for FreeCAD"

[dependencies]
cmake = "*"
"""

FEDORA_SPEC = """\
Name:           freecad
Epoch:          1
Version:        1.1.0~dev
Release:        1%{?dist}

Summary:        A general purpose 3D CAD modeler
"""


class TestSyncWorkspacePixiToml(unittest.TestCase):
    def test_updates_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "pixi.toml", WORKSPACE_PIXI_TOML)
            version = make_version()
            result, changed = sync_workspace_pixi_toml(filepath, version)
            self.assertTrue(changed)
            self.assertIn('version = "1.2.0"', result)

    def test_already_in_sync(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "pixi.toml", WORKSPACE_PIXI_TOML)
            version = make_version(major=1, minor=0, patch=0, suffix="")
            result, changed = sync_workspace_pixi_toml(filepath, version)
            self.assertFalse(changed)

    def test_ignores_suffix(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "pixi.toml", WORKSPACE_PIXI_TOML)
            version = make_version(suffix="RC1")
            result, changed = sync_workspace_pixi_toml(filepath, version)
            self.assertIn('version = "1.2.0"', result)
            self.assertNotIn("RC1", result)


class TestSyncFedoraSpec(unittest.TestCase):
    def test_updates_name_and_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "freecad.spec", FEDORA_SPEC)
            version = make_version()
            result, changed = sync_fedora_spec(filepath, version)
            self.assertTrue(changed)
            self.assertIn("Name:           freecad", result)
            self.assertIn("Version:        1.2.0~dev", result)

    def test_release_version_no_tilde(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "freecad.spec", FEDORA_SPEC)
            version = make_version(suffix="")
            result, changed = sync_fedora_spec(filepath, version)
            self.assertIn("Version:        1.2.0", result)
            self.assertNotIn("~", result.split("Version:")[1].split("\n")[0])

    def test_rc_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "freecad.spec", FEDORA_SPEC)
            version = make_version(suffix="RC1")
            result, changed = sync_fedora_spec(filepath, version)
            self.assertIn("Version:        1.2.0~RC1", result)

    def test_updates_name_for_fork(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "freecad.spec", FEDORA_SPEC)
            version = make_version(name="MyCAD")
            result, changed = sync_fedora_spec(filepath, version)
            self.assertIn("Name:           mycad", result)

    def test_preserves_other_fields(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "freecad.spec", FEDORA_SPEC)
            version = make_version()
            result, changed = sync_fedora_spec(filepath, version)
            self.assertIn("Epoch:          1", result)
            self.assertIn("Release:        1%{?dist}", result)


DESKTOP_FILE_UNSYNCED = """\
[Desktop Entry]
Name=FreeCAD
StartupWMClass=FreeCAD
Type=Application
 con=org.freecad.FreeCAD
X-AppImage-Name=FreeCAD
X-AppImage-Version=VERSION
"""

DESKTOP_FILE_SYNCED = """\
[Desktop Entry]
Name=FreeCAD
StartupWMClass=FreeCAD-1.2.0
Type=Application
 con=org.freecad.FreeCAD
X-AppImage-Name=FreeCAD
X-AppImage-Version=1.2.0
"""

DESKTOP_FILE_PARTIALLY_SYNCED = """\
[Desktop Entry]
Name=FreeCAD
StartupWMClass=FreeCAD-1.2.0
Type=Application
 con=org.freecad.FreeCAD
X-AppImage-Name=FreeCAD
X-AppImage-Version=VERSION
"""


class TestSyncDesktopFile(unittest.TestCase):
    def test_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp), "org.freecad.Freecad.desktop", DESKTOP_FILE_SYNCED
            )
            version = make_version()
            result, changed = sync_desktop_file(filepath, version)
            self.assertIn("StartupWMClass=FreeCAD-1.2.0", result)
            self.assertIn("X-AppImage-Version=1.2.0", result)
            self.assertFalse(changed)

    def test_unsynced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp), "org.freecad.Freecad.desktop", DESKTOP_FILE_UNSYNCED
            )
            version = make_version()
            result, changed = sync_desktop_file(filepath, version)
            self.assertIn("StartupWMClass=FreeCAD-1.2.0", result)
            self.assertIn("X-AppImage-Version=1.2.0", result)
            self.assertTrue(changed)

    def test_partially_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp), "org.freecad.Freecad.desktop", DESKTOP_FILE_PARTIALLY_SYNCED
            )
            version = make_version()
            result, changed = sync_desktop_file(filepath, version)
            self.assertTrue(changed)


MAINGUI_CPP_SYNCED = """\
App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";
App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";
argv[0] = const_cast<char*>("FreeCAD-1.2.0");
"""


MAINGUI_CPP_UNSYNCED = """\
App::Application::Config()["ExeName"] = "FreeCAD";
App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD";
argv[0] = const_cast<char*>("FreeCAD");
"""

MAINGUI_CPP_PARTIALLY_SYNCED = """\
App::Application::Config()["ExeName"] = "FreeCAD";
App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";
argv[0] = const_cast<char*>("FreeCAD");
"""


class TestMainGui(unittest.TestCase):
    def test_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "MainGui.cpp", MAINGUI_CPP_SYNCED)
            version = make_version()
            result, changed = sync_MainGui(filepath, version)
            self.assertIn(
                'App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";', result
            )
            self.assertIn(
                'App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";',
                result,
            )
            self.assertIn('argv[0] = const_cast<char*>("FreeCAD-1.2.0");', result)
            self.assertFalse(changed)

    def test_unsynced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(Path(tmp), "MainGui.cpp", MAINGUI_CPP_UNSYNCED)
            version = make_version()
            result, changed = sync_MainGui(filepath, version)
            self.assertIn(
                'App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";', result
            )
            self.assertIn(
                'App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";',
                result,
            )
            self.assertIn('argv[0] = const_cast<char*>("FreeCAD-1.2.0");', result)
            self.assertTrue(changed)

    def test_partially_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp), "MainGui.cpp", MAINGUI_CPP_PARTIALLY_SYNCED
            )
            version = make_version()
            result, changed = sync_MainGui(filepath, version)
            self.assertIn(
                'App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";', result
            )
            self.assertIn(
                'App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";',
                result,
            )
            self.assertIn('argv[0] = const_cast<char*>("FreeCAD-1.2.0");', result)
            self.assertTrue(changed)


LINUX_CREATE_BUNDLE_SYNCED = """\
cp ${conda_env}/share/applications/org.freecad.FreeCAD-1.2.0.desktop AppDir/
sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD-1.2.0.desktop
"""


LINUX_CREATE_BUNDLE_UNSYNCED = """\
cp ${conda_env}/share/applications/org.freecad.FreeCAD.desktop AppDir/
sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD.desktop
"""


LINUX_CREATE_BUNDLE_PARTIALLY_SYNCED = """\
cp ${conda_env}/share/applications/org.freecad.FreeCAD-1.2.0.desktop AppDir/
sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD.desktop
"""


class TestSyncLinuxCreateBundle(unittest.TestCase):
    def test_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp),
                "create_bundle.sh",
                LINUX_CREATE_BUNDLE_SYNCED,
            )
            version = make_version()

            result, changed = sync_linux_create_bundle(filepath, version)

            self.assertIn(
                "cp ${conda_env}/share/applications/org.freecad.FreeCAD-1.2.0.desktop AppDir/",
                result,
            )
            self.assertIn(
                "sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD-1.2.0.desktop",
                result,
            )
            self.assertFalse(changed)

    def test_unsynced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp),
                "create_bundle.sh",
                LINUX_CREATE_BUNDLE_UNSYNCED,
            )
            version = make_version()

            result, changed = sync_linux_create_bundle(filepath, version)

            self.assertIn(
                "cp ${conda_env}/share/applications/org.freecad.FreeCAD-1.2.0.desktop AppDir/",
                result,
            )
            self.assertIn(
                "sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD-1.2.0.desktop",
                result,
            )
            self.assertTrue(changed)

    def test_partially_synced(self):
        with tempfile.TemporaryDirectory() as tmp:
            filepath = write_temp_file(
                Path(tmp),
                "create_bundle.sh",
                LINUX_CREATE_BUNDLE_PARTIALLY_SYNCED,
            )
            version = make_version()

            result, changed = sync_linux_create_bundle(filepath, version)

            self.assertIn(
                "cp ${conda_env}/share/applications/org.freecad.FreeCAD-1.2.0.desktop AppDir/",
                result,
            )
            self.assertIn(
                "sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD-1.2.0.desktop",
                result,
            )
            self.assertTrue(changed)


@patch("sys.stdout", new_callable=io.StringIO)
class TestRun(unittest.TestCase):
    def _create_repo(self, tmp: str, version: VersionInfo) -> Path:
        root = Path(tmp)
        (root / "version.json").write_text(
            json.dumps(
                {
                    "name": version.name,
                    "version_major": version.major,
                    "version_minor": version.minor,
                    "version_patch": version.patch,
                    "version_suffix": version.suffix,
                    "build_version": version.build,
                }
            ),
            encoding="utf-8",
        )
        write_temp_file(root, "pixi.toml", WORKSPACE_PIXI_TOML)
        write_temp_file(root, "package/fedora/freecad.spec", FEDORA_SPEC)
        write_temp_file(
            root, "src/XDGData/org.freecad.FreeCAD-1.2.0.desktop", DESKTOP_FILE_UNSYNCED
        )
        write_temp_file(root, "src/Main/MainGui.cpp", MAINGUI_CPP_UNSYNCED)
        write_temp_file(
            root, "package/bundle/linux/create_bundle.sh", LINUX_CREATE_BUNDLE_UNSYNCED
        )
        return root

    def test_check_detects_out_of_sync(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())
            result = run(root, check_only=True)
            self.assertFalse(result)

    def test_update_then_check_is_synced(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())
            run(root, check_only=False)
            result = run(root, check_only=True)
            self.assertTrue(result)

    def test_check_when_already_synced(self, _stdout):
        version = make_version(major=1, minor=0, patch=0, suffix="")
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, version)
            result = run(root, check_only=True)
            # pixi.toml starts with "1.0.0" which matches this version,
            # but other files have "1.1.0dev" so they will be out of sync
            self.assertFalse(result)

    def test_skips_missing_files(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "version.json").write_text(
                json.dumps(
                    {
                        "name": "FreeCAD",
                        "version_major": 1,
                        "version_minor": 2,
                        "version_patch": 0,
                        "version_suffix": "dev",
                        "build_version": 0,
                    }
                ),
                encoding="utf-8",
            )
            # Only create pixi.toml, skip the rest
            write_temp_file(root, "pixi.toml", WORKSPACE_PIXI_TOML)
            result = run(root, check_only=True)
            # Should not crash, just skip missing files
            self.assertFalse(result)

    def test_update_does_not_modify_synced_files(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())
            run(root, check_only=False)
            # Record file contents after first update
            contents = {}
            for child in root.rglob("*"):
                if child.is_file() and child.name != "version.json":
                    contents[child] = child.read_text(encoding="utf-8")
            # Run update again
            run(root, check_only=False)
            # Verify nothing changed
            for filepath, original in contents.items():
                self.assertEqual(
                    filepath.read_text(encoding="utf-8"),
                    original,
                    f"{filepath.name} was modified on second run",
                )

    def test_check_does_not_rename_desktop_file(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())

            old_path = root / "src/XDGData/org.freecad.FreeCAD.desktop"
            new_path = root / "src/XDGData/org.freecad.FreeCAD-1.2.0.desktop"

            new_path.rename(old_path)

            run(root, check_only=True)

            self.assertTrue(old_path.exists())
            self.assertFalse(new_path.exists())

    def test_update_renames_desktop_file(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())
            run(root, check_only=False)
            self.assertFalse(
                (root / "src/XDGData/org.freecad.FreeCAD.desktop").exists()
            )
            self.assertTrue(
                (root / "src/XDGData/org.freecad.FreeCAD-1.2.0.desktop").exists()
            )

    def test_already_renamed_is_ok(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())
            run(root, check_only=False)
            result = run(root, check_only=True)
            self.assertTrue(result)

    def test_update_patches_maingui(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())

            run(root, check_only=False)

            content = (root / "src/Main/MainGui.cpp").read_text(encoding="utf-8")

            self.assertIn(
                'App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";',
                content,
            )
            self.assertIn(
                'App::Application::Config()["DesktopFileName"] = "org.freecad.FreeCAD-1.2.0";',
                content,
            )
            self.assertIn(
                'argv[0] = const_cast<char*>("FreeCAD-1.2.0");',
                content,
            )

    def test_check_detects_unsynced_maingui(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())

            result = run(root, check_only=True)

            self.assertFalse(result)

            content = (root / "src/Main/MainGui.cpp").read_text(encoding="utf-8")

            self.assertIn(
                'App::Application::Config()["ExeName"] = "FreeCAD";',
                content,
            )
            self.assertNotIn(
                'App::Application::Config()["ExeName"] = "FreeCAD-1.2.0";',
                content,
            )

    def test_update_patches_linux_create_bundle(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())

            run(root, check_only=False)

            content = (root / "package/bundle/linux/create_bundle.sh").read_text(
                encoding="utf-8"
            )

            self.assertIn(
                "cp ${conda_env}/share/applications/"
                "org.freecad.FreeCAD-1.2.0.desktop AppDir/",
                content,
            )

            self.assertIn(
                "sed -i 's/Exec=FreeCAD/Exec=AppRun/g' "
                "AppDir/org.freecad.FreeCAD-1.2.0.desktop",
                content,
            )

    def test_check_does_not_modify_linux_create_bundle(self, _stdout):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._create_repo(tmp, make_version())

            filepath = root / "package/bundle/linux/create_bundle.sh"
            original = filepath.read_text(encoding="utf-8")

            result = run(root, check_only=True)

            self.assertFalse(result)
            self.assertEqual(
                filepath.read_text(encoding="utf-8"),
                original,
            )


if __name__ == "__main__":
    unittest.main()
