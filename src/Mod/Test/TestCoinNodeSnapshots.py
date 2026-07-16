# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

# ******************************************************************************
# *                                                                            *
# *   FreeCAD is free software: you can redistribute it and/or modify          *
# *   it under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the     *
# *   License, or (at your option) any later version.                          *
# *                                                                            *
# *   FreeCAD is distributed in the hope that it will be useful, but           *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Lesser General Public License for more details.                      *
# *                                                                            *
# *   You should have received a copy of the GNU Lesser General Public         *
# *   License along with FreeCAD.  If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                         *
# *                                                                            *
# ******************************************************************************

"""
Visual snapshot tests for selected Coin/Inventor nodes.

This test renders a curated set of nodes through the live 3D viewer and
optionally compares the resulting PNGs against checked-in baselines.

The test is intended to be executed via FreeCAD's test runner:

  FreeCAD -t TestCoinNodeSnapshots

Environment variables:
  - FC_VISUAL_OUT_DIR: output directory (writes actual/expected/diff)
  - FC_VISUAL_BASELINE_DIR: baseline directory override (default: tests/visual/baselines/coin-nodes)
  - FC_VISUAL_UPDATE_BASELINE: if truthy, overwrite baselines with actual renders
  - FC_VISUAL_NODES: optional comma-separated node list to run
"""

# This file is intentionally tolerant of missing optional bindings and older
# FreeCAD/Coin/Pivy environments; it is a test harness rather than library code.
# pylint: disable=import-outside-toplevel,broad-exception-caught,deprecated-module
# pylint: disable=too-many-branches,too-many-statements,too-many-locals
# pylint: disable=too-many-return-statements,too-many-arguments,too-many-positional-arguments

import importlib
import importlib.util
import os
import sys
import tempfile
import time
import unittest
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

_BASELINE_REL = Path("tests") / "visual" / "baselines" / "coin-nodes"
_FONT_REL = Path("tests") / "visual" / "fonts"
_DEFAULT_FONT_FAMILY = "Noto Sans"
_DEFAULT_FONT_FILES = ("NotoSans-Regular.ttf",)
_DEFAULT_FONT_SIZE = 18
_TRANSLUCENCY_BACKGROUND_TOP = (0.20, 0.20, 0.60)
_TRANSLUCENCY_BACKGROUND_BOTTOM = (0.90, 0.90, 1.00)
_SNAPSHOT_WIDTH = 512
_SNAPSHOT_HEIGHT = 512
_PIXEL_TOLERANCE = 8
_MAX_MISMATCH_PCT = 0.20
_IGNORE_ALPHA = True


class _CameraPolicy(Enum):
    VIEW_ALL = auto()
    VIEW_ALL_WITH_MARGIN = auto()
    FIXED_OVERLAY = auto()
    COLOR_BAR_OVERLAY = auto()


@dataclass(frozen=True)
class _SnapshotFixture:
    framing_policy: _CameraPolicy = _CameraPolicy.VIEW_ALL
    required_modules: tuple[str, ...] = ()


@dataclass(frozen=True)
class _SnapshotScene:
    root: object
    framing_policy: _CameraPolicy


_PART_GUI_NODES = (
    "SoPreviewShape",
    "SoBrepEdgeSet",
    "SoBrepEdgeSetHighlight",
    "SoBrepEdgeSetSelection",
    "SoBrepPointSet",
    "SoBrepPointSetHighlight",
    "SoBrepPointSetSelection",
    "SoBrepFaceSet",
    "SoBrepFaceSetHighlight",
    "SoBrepFaceSetSelection",
    "SoFCControlPoints",
)
_MESH_GUI_NODES = (
    "SoPolygon",
    "SoPolygonOpen",
    "SoPolygonStartIndex",
    "SoPolygonNonPlanar",
    "SoPolygonTriangle",
    "SoFCIndexedFaceSet",
    "SoFCIndexedFaceSetPerFaceColor",
    "SoFCIndexedFaceSetPerVertexColor",
    "SoFCIndexedFaceSetTranslucent",
)

_SNAPSHOT_FIXTURES = {
    "SoFCBoundingBox": _SnapshotFixture(_CameraPolicy.VIEW_ALL_WITH_MARGIN),
    "SoDrawingGrid": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoRegPoint": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoDatumLabel": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoStringLabel": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoColorBarLabel": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoFrameLabel": _SnapshotFixture(_CameraPolicy.FIXED_OVERLAY),
    "SoFCPlacementIndicatorKit": _SnapshotFixture(),
    "SoAxisCrossKit": _SnapshotFixture(),
    "SoFCBackgroundGradient": _SnapshotFixture(),
    "SoNaviCube": _SnapshotFixture(),
    "SoNaviCubeTranslucent": _SnapshotFixture(),
    "SoNaviCubeHiliteFront": _SnapshotFixture(),
    **{name: _SnapshotFixture(required_modules=("PartGui",)) for name in _PART_GUI_NODES},
    **{name: _SnapshotFixture(required_modules=("MeshGui",)) for name in _MESH_GUI_NODES},
}


def _repo_root() -> Path | None:
    here = Path(__file__).resolve()
    for parent in here.parents:
        if (parent / ".git").exists():
            return parent
    return None


def _find_repo_baseline_dir() -> Path | None:
    repo_root = _repo_root()
    if repo_root is not None:
        candidate = repo_root / _BASELINE_REL
        if candidate.is_dir() and any(candidate.glob("*.png")):
            return candidate

    here = Path(__file__).resolve()
    for parent in here.parents:
        candidate = parent / _BASELINE_REL
        if candidate.is_dir() and any(candidate.glob("*.png")):
            return candidate
    return None


def _baseline_dir(create: bool = False) -> Path:
    baseline_dir_env = os.environ.get("FC_VISUAL_BASELINE_DIR", "").strip()
    if baseline_dir_env:
        p = Path(baseline_dir_env)
        if create:
            p.mkdir(parents=True, exist_ok=True)
        return p

    found = _find_repo_baseline_dir()
    if found is not None:
        if create:
            found.mkdir(parents=True, exist_ok=True)
        return found

    raise FileNotFoundError(
        f"baseline directory not found (expected {_BASELINE_REL}; set FC_VISUAL_BASELINE_DIR to override)"
    )


def _find_repo_font_dir() -> Path | None:
    repo_root = _repo_root()
    if repo_root is not None:
        candidate = repo_root / _FONT_REL
        if candidate.is_dir():
            return candidate

    here = Path(__file__).resolve()
    for parent in here.parents:
        candidate = parent / _FONT_REL
        if candidate.is_dir():
            return candidate
    return None


def _font_dir() -> Path:
    found = _find_repo_font_dir()
    if found is not None:
        return found

    raise FileNotFoundError(f"font directory not found (expected {_FONT_REL})")


def _font_files() -> list[Path]:
    base = _font_dir()
    return [base / n for n in _DEFAULT_FONT_FILES]


def _configure_fontconfig_for_visual_fonts(font_dir: Path):
    if not sys.platform.startswith("linux"):
        return

    tmp = Path(tempfile.mkdtemp(prefix="freecad-visual-fontconfig-"))
    conf = tmp / "fonts.conf"
    family = _DEFAULT_FONT_FAMILY
    conf.write_text(
        "\n".join(
            [
                '<?xml version="1.0"?>',
                "<fontconfig>",
                f"  <dir>{font_dir.as_posix()}</dir>",
                # Keep caches in a per-run location to avoid host-dependent results.
                f"  <cachedir>{tmp.as_posix()}/cache</cachedir>",
                "  <config>",
                "    <rescan><int>0</int></rescan>",
                "  </config>",
                "  <alias>",
                "    <family>sans-serif</family>",
                f"    <prefer><family>{family}</family></prefer>",
                "  </alias>",
                "</fontconfig>",
                "",
            ]
        ),
        encoding="utf-8",
    )

    os.environ["FONTCONFIG_FILE"] = str(conf)
    os.environ["FONTCONFIG_PATH"] = str(tmp)


def _register_visual_fonts():
    font_dir = _font_dir()
    font_files = _font_files()
    missing = [p for p in font_files if not p.is_file()]
    if missing:
        raise FileNotFoundError(f"missing font file(s): {', '.join(str(p) for p in missing)}")

    _configure_fontconfig_for_visual_fonts(font_dir)

    # Register for Qt text rendering (e.g. SoStringLabel uses QPainter).
    from PySide import QtGui  # type: ignore

    for p in font_files:
        QtGui.QFontDatabase.addApplicationFont(str(p))

    # Register for native font backends (important on Windows, where FreeCAD forces Coin
    # not to use FreeType/fontconfig).
    if sys.platform.startswith("win"):
        import ctypes

        FR_PRIVATE = 0x10
        gdi32 = ctypes.windll.gdi32  # type: ignore[attr-defined]
        for p in font_files:
            gdi32.AddFontResourceExW(str(p), FR_PRIVATE, 0)

    elif sys.platform == "darwin":
        import ctypes
        import ctypes.util

        coretext = ctypes.CDLL(ctypes.util.find_library("CoreText"))  # type: ignore[arg-type]
        cf = ctypes.CDLL(ctypes.util.find_library("CoreFoundation"))  # type: ignore[arg-type]

        CFURLRef = ctypes.c_void_p
        CFAllocatorRef = ctypes.c_void_p
        CFErrorRef = ctypes.c_void_p
        Boolean = ctypes.c_ubyte

        CFURLCreateFromFileSystemRepresentation = cf.CFURLCreateFromFileSystemRepresentation
        CFURLCreateFromFileSystemRepresentation.argtypes = [
            CFAllocatorRef,
            ctypes.c_char_p,
            ctypes.c_long,
            Boolean,
        ]
        CFURLCreateFromFileSystemRepresentation.restype = CFURLRef

        CFRelease = cf.CFRelease
        CFRelease.argtypes = [ctypes.c_void_p]
        CFRelease.restype = None

        CTFontManagerRegisterFontsForURL = coretext.CTFontManagerRegisterFontsForURL
        CTFontManagerRegisterFontsForURL.argtypes = [
            CFURLRef,
            ctypes.c_uint32,
            ctypes.POINTER(CFErrorRef),
        ]
        CTFontManagerRegisterFontsForURL.restype = Boolean

        kCTFontManagerScopeProcess = 1
        for p in font_files:
            b = str(p).encode("utf-8")
            url = CFURLCreateFromFileSystemRepresentation(0, b, len(b), 0)
            if url:
                err = CFErrorRef()
                CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, ctypes.byref(err))
                CFRelease(url)


def _configure_software_gl_for_snapshots():
    if not sys.platform.startswith("linux"):
        return

    force = os.environ.get("FC_VISUAL_FORCE_SOFTWARE_GL", "1").strip().lower()
    if force in ("", "0", "false", "no"):
        return

    # Make renders deterministic across hosts: software llvmpipe avoids GPU driver differences.
    os.environ.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
    os.environ.setdefault("MESA_LOADER_DRIVER_OVERRIDE", "llvmpipe")


def _require_gui():
    try:
        import FreeCAD  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise RuntimeError("FreeCAD module not available") from exc

    _configure_software_gl_for_snapshots()

    if os.environ.get("CI", "").strip() and not sys.platform.startswith("linux"):
        raise unittest.SkipTest("Coin node snapshot visual tests are only supported on Linux CI")

    # `FreeCADCmd -t 0` runs the base test suite without X11/Wayland. Creating a Qt application
    # in that environment can abort the whole process (instead of raising a Python exception).
    # Skip early unless a display (or explicit headless Qt platform) is configured.
    if sys.platform.startswith("linux"):
        has_display = bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))
        qpa = os.environ.get("QT_QPA_PLATFORM", "").strip().lower()
        headless_qpa = {"offscreen", "minimal", "eglfs", "linuxfb", "vnc"}
        if not has_display and qpa not in headless_qpa:
            raise unittest.SkipTest(
                "No display available (run under xvfb-run or set QT_QPA_PLATFORM=offscreen)"
            )

    try:
        import FreeCADGui  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("FreeCADGui not available in this build") from exc

    # Ensure the GUI subsystem is initialized enough for Coin + offscreen GL.
    # Older setups may not provide this; offscreen rendering can still work.
    setup_without_gui = getattr(FreeCADGui, "setupWithoutGUI", None)
    if callable(setup_without_gui):
        setup_without_gui()

    # Ensure built-in modules (e.g. Part/PartGui) are importable even when the test runner
    # didn't pick up the build's Mod/ paths.
    home = Path(FreeCAD.getHomePath())
    mod_dir = home / "Mod"
    if mod_dir.is_dir():
        for p in (mod_dir, mod_dir / "Part"):
            ps = str(p)
            if ps not in sys.path:
                sys.path.insert(0, ps)

    # Ensure a QGuiApplication exists so an OpenGL context can be created.
    try:
        from PySide import QtGui  # type: ignore

        if QtGui.QGuiApplication.instance() is None:
            QtGui.QGuiApplication(sys.argv)
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("Qt (PySide) not available") from exc

    # Register vendored fonts before importing Coin, so Coin's font backend can discover them.
    try:
        _register_visual_fonts()
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest(f"visual test font setup failed: {exc}") from exc

    try:
        from pivy import coin  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("pivy.coin not available") from exc

    return FreeCAD, FreeCADGui, coin


def _instantiate(coin, type_name: str):
    t = coin.SoType.fromName(type_name)
    if t.isBad():
        raise unittest.SkipTest(f"Coin type not registered: {type_name}")
    node = t.createInstance()
    if node is None:
        raise RuntimeError(f"Failed to instantiate {type_name}")
    return node


def _configure_background_gradient(grad, from_color, to_color):
    grad.gradientMode.setValue(0)
    grad.fromColor.setValue(from_color)
    grad.toColor.setValue(to_color)
    grad.useMidColor.setValue(False)


def _add_gradient_background(root, coin, top, bottom):
    gradient = _instantiate(coin, "SoFCBackgroundGradient")
    _configure_background_gradient(gradient, coin.SbColor(*top), coin.SbColor(*bottom))
    root.addChild(gradient)


def _add_translucency_backplate(root, coin):
    """Add two opaque color panels behind the translucent face-set fixture."""
    backplate = coin.SoSeparator()

    light_model = coin.SoLightModel()
    light_model.model = coin.SoLightModel.BASE_COLOR
    backplate.addChild(light_model)

    coords = coin.SoCoordinate3()
    coords.point.setValues(
        0,
        8,
        [
            coin.SbVec3f(-0.95, -0.85, -0.05),
            coin.SbVec3f(0.0, -0.85, -0.05),
            coin.SbVec3f(0.0, 0.85, -0.05),
            coin.SbVec3f(-0.95, 0.85, -0.05),
            coin.SbVec3f(0.0, -0.85, -0.05),
            coin.SbVec3f(0.95, -0.85, -0.05),
            coin.SbVec3f(0.95, 0.85, -0.05),
            coin.SbVec3f(0.0, 0.85, -0.05),
        ],
    )
    material = coin.SoMaterial()
    material.diffuseColor.setValues(
        0,
        2,
        [coin.SbColor(0.10, 0.25, 0.90), coin.SbColor(1.00, 0.75, 0.05)],
    )
    binding = coin.SoMaterialBinding()
    binding.value = coin.SoMaterialBinding.PER_FACE
    faces = coin.SoIndexedFaceSet()
    faces.coordIndex.setValues(0, 10, [0, 1, 2, 3, -1, 4, 5, 6, 7, -1])

    backplate.addChild(coords)
    backplate.addChild(material)
    backplate.addChild(binding)
    backplate.addChild(faces)
    root.addChild(backplate)


def _load_required_modules(fixture: _SnapshotFixture) -> None:
    for module_name in fixture.required_modules:
        if importlib.util.find_spec(module_name) is None:
            raise unittest.SkipTest(f"required module not available: {module_name}")
        try:
            importlib.import_module(module_name)
        except Exception as exc:
            raise AssertionError(f"failed to import required module: {module_name}") from exc


def _find_descendant(coin, root, type_name: str):
    """Return the first descendant of ``root`` with the requested Coin type."""
    type_id = coin.SoType.fromName(type_name)
    if type_id.isBad():
        raise unittest.SkipTest(f"Coin type not registered: {type_name}")

    search = coin.SoSearchAction()
    search.setType(type_id)
    search.setSearchingAll(False)
    search.apply(root)
    path = search.getPath()
    return None if path is None else path.getTail()


def _configure_preview_geometry(coin, preview) -> None:
    """Populate SoPreviewShape's retained SoFCShape geometry for a test scene."""
    coords = _find_descendant(coin, preview, "SoCoordinate3")
    normals = _find_descendant(coin, preview, "SoNormal")
    faces = _find_descendant(coin, preview, "SoBrepFaceSet")
    lines = _find_descendant(coin, preview, "SoBrepEdgeSet")
    if any(node is None for node in (coords, normals, faces, lines)):
        raise AssertionError("SoPreviewShape retained geometry is incomplete")

    coords.point.setValues(
        0,
        8,
        [
            coin.SbVec3f(-0.65, -0.55, -0.45),
            coin.SbVec3f(0.65, -0.55, -0.45),
            coin.SbVec3f(0.65, 0.55, -0.45),
            coin.SbVec3f(-0.65, 0.55, -0.45),
            coin.SbVec3f(-0.65, -0.55, 0.45),
            coin.SbVec3f(0.65, -0.55, 0.45),
            coin.SbVec3f(0.65, 0.55, 0.45),
            coin.SbVec3f(-0.65, 0.55, 0.45),
        ],
    )
    normals.vector.setValues(
        0,
        8,
        [
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
        ],
    )
    # Six faces, two triangles per face.
    # fmt: off
    faces.coordIndex.setValues(0, 48, [
        0, 1, 2, -1,  0, 2, 3, -1,  # bottom (-Z)
        4, 6, 5, -1,  4, 7, 6, -1,  # top (+Z)
        0, 4, 5, -1,  0, 5, 1, -1,  # front (-Y)
        1, 5, 6, -1,  1, 6, 2, -1,  # right (+X)
        2, 6, 7, -1,  2, 7, 3, -1,  # back (+Y)
        3, 7, 4, -1,  3, 4, 0, -1,  # left (-X)
    ])
    # fmt: on
    faces.partIndex.setValues(0, 6, [2, 2, 2, 2, 2, 2])
    lines.coordIndex.setValues(
        0,
        36,
        [
            0,
            1,
            -1,
            1,
            2,
            -1,
            2,
            3,
            -1,
            3,
            0,
            -1,
            4,
            5,
            -1,
            5,
            6,
            -1,
            6,
            7,
            -1,
            7,
            4,
            -1,
            0,
            4,
            -1,
            1,
            5,
            -1,
            2,
            6,
            -1,
            3,
            7,
            -1,
        ],
    )
    lines.materialIndex.setValues(0, 12, [0] * 12)

    preview.color.setValue(0.78, 0.12, 0.92)
    preview.transparency.setValue(0.45)
    preview.lineWidth.setValue(3.0)
    matrix = coin.SbMatrix()
    matrix.setTranslate(coin.SbVec3f(0.25, 0.1, 0.15))
    preview.transform.setValue(matrix)


def _configure_camera(coin, cam, policy: _CameraPolicy) -> None:
    if policy in (_CameraPolicy.VIEW_ALL, _CameraPolicy.VIEW_ALL_WITH_MARGIN):
        return

    # These nodes are tested in isolation. Their geometry is either
    # screen-space or explicitly positioned, so a host cube would only
    # obscure the behavior being snapshotted.
    cam.position.setValue(0.0, 0.0, 5.0)
    cam.orientation.setValue(coin.SbRotation())
    cam.height.setValue(10.0 if policy is _CameraPolicy.COLOR_BAR_OVERLAY else 2.0)
    cam.nearDistance.setValue(0.1)
    cam.farDistance.setValue(100.0)


def _make_scene_for_node(coin, type_name: str, fixture: _SnapshotFixture):
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    # Isometric-ish.
    cam.orientation.setValue(coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553))
    root.addChild(cam)

    # Light + base color model so nodes render consistently.
    light = coin.SoDirectionalLight()
    root.addChild(light)

    # Make text rendering less dependent on the host: set an explicit font in the Coin scene.
    font = coin.SoFont()
    font.name.setValue(_DEFAULT_FONT_FAMILY)
    font.size.setValue(float(_DEFAULT_FONT_SIZE))
    root.addChild(font)

    _configure_camera(coin, cam, fixture.framing_policy)
    _load_required_modules(fixture)

    if type_name == "SoFCBoundingBox":
        color = coin.SoBaseColor()
        color.rgb.setValue(0.08, 0.12, 0.22)
        style = coin.SoDrawStyle()
        style.lineWidth.setValue(2.0)
        box = _instantiate(coin, type_name)
        box.minBounds.setValue(-1.5, -1.1, -0.75)
        box.maxBounds.setValue(1.5, 1.1, 0.75)
        box.coordsOn.setValue(True)
        box.dimensionsOn.setValue(True)
        # Include the box in viewAll; the production default intentionally
        # excludes it from parent bounding-box calculations.
        box.skipBoundingBox.setValue(False)
        root.addChild(color)
        root.addChild(style)
        root.addChild(box)
        return root

    if type_name == "SoPreviewShape":
        preview = _instantiate(coin, type_name)
        root.addChild(preview)
        _configure_preview_geometry(coin, preview)
        return root

    if type_name == "SoDrawingGrid":
        root.addChild(_instantiate(coin, "SoDrawingGrid"))
        return root

    if type_name == "SoRegPoint":
        probe = _instantiate(coin, "SoRegPoint")
        probe.base.setValue(0.0, 0.0, 0.0)
        probe.normal.setValue(0.6, 0.7, 0.4)
        probe.length.setValue(1.2)
        probe.color.setValue(1.0, 0.45, 0.34)
        probe.text.setValue("SoRegPoint")
        root.addChild(probe)
        return root

    if type_name == "SoDatumLabel":
        label = _instantiate(coin, "SoDatumLabel")
        label.string.setValue("SoDatumLabel")
        label.textColor.setValue(1.0, 0.45, 0.34)
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(18)
        label.lineWidth.setValue(2.0)
        label.sampling.setValue(2.0)
        # `createInstance()` returns a generic node proxy (not a typed Python class),
        # so enum constants / helper methods may not be available.
        # `Gui::SoDatumLabel::Type::DISTANCE == 1`.
        label.datumtype.setValue(1)
        label.param1.setValue(0.25)
        label.param2.setValue(0.0)
        label.pnts.setValues(
            0,
            2,
            [coin.SbVec3f(-0.5, -0.1, 0.0), coin.SbVec3f(0.5, 0.2, 0.0)],
        )
        root.addChild(label)
        return root

    if type_name == "SoStringLabel":
        label = _instantiate(coin, "SoStringLabel")
        label.string.setValue("SoStringLabel")
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(28)
        # Default SoStringLabel textColor is white, which can become invisible on the white
        # snapshot background depending on the GL blending / alpha handling.
        label.textColor.setValue(0.05, 0.05, 0.05)
        root.addChild(label)
        return root

    if type_name == "SoColorBarLabel":
        font.size.setValue(28.0)
        color = coin.SoBaseColor()
        color.rgb.setValue(0.05, 0.05, 0.05)
        label = _instantiate(coin, "SoColorBarLabel")
        label.string.setValues(0, 2, ["Low", "High"])
        root.addChild(color)
        root.addChild(label)
        return root

    if type_name == "SoFrameLabel":
        label = _instantiate(coin, "SoFrameLabel")
        label.string.setValue("Frame Label")
        label.textColor.setValue(1.0, 1.0, 1.0)
        label.backgroundColor.setValue(0.10, 0.35, 0.85)
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(20)
        label.justification.setValue(2)  # SoFrameLabel::CENTER
        label.frame.setValue(True)
        label.border.setValue(True)
        root.addChild(label)
        return root

    if type_name == "SoFCPlacementIndicatorKit":
        indicator = _instantiate(coin, "SoFCPlacementIndicatorKit")
        indicator.parts.setValue(31)  # SoFCPlacementIndicatorKit::AllParts
        indicator.axes.setValue(7)  # SoFCPlacementIndicatorKit::AllAxes
        indicator.axisLength.setValue(0.8)
        indicator.scaleFactor.setValue(70.0)
        indicator.axisLabels.setValues(0, 3, ["X", "Y", "Z"])
        root.addChild(indicator)
        return root

    if type_name == "SoAxisCrossKit":
        root.addChild(_instantiate(coin, "SoAxisCrossKit"))
        return root

    if type_name == "SoFCBackgroundGradient":
        grad = _instantiate(coin, "SoFCBackgroundGradient")
        _configure_background_gradient(
            grad,
            coin.SbColor(0.2, 0.2, 0.6),
            coin.SbColor(0.9, 0.9, 1.0),
        )
        root.addChild(grad)
        return root

    if type_name in ("SoNaviCube", "SoNaviCubeTranslucent", "SoNaviCubeHiliteFront"):
        if type_name == "SoNaviCubeTranslucent":
            # Provide visible background so translucency can be verified.
            grad = _instantiate(coin, "SoFCBackgroundGradient")
            _configure_background_gradient(
                grad,
                coin.SbColor(0.15, 0.15, 0.20),
                coin.SbColor(0.45, 0.45, 0.55),
            )
            root.addChild(grad)

        cube = _instantiate(coin, "SoNaviCube")
        cube.size.setValue(1.0)
        cube.opacity.setValue(0.55 if type_name == "SoNaviCubeTranslucent" else 1.0)
        cube.borderWidth.setValue(0.02)
        cube.showCoordinateSystem.setValue(True)
        cube.cameraIsOrthographic.setValue(True)
        if type_name == "SoNaviCubeHiliteFront":
            # Gui::SoNaviCube::PickId::Front (see `src/Gui/Inventor/SoNaviCube.h`).
            cube.hiliteId.setValue(1)
        width = float(_SNAPSHOT_WIDTH)
        height = float(_SNAPSHOT_HEIGHT)

        # Render like a real overlay: small square in the corner.
        overlay = max(64.0, min(width, height) * 0.60)
        margin = 8.0
        cube.viewportRect.setValue(
            width - overlay - margin,
            height - overlay - margin,
            overlay,
            overlay,
        )

        # Mimic what the controller does: orient the cube from the viewer camera.
        cube.cameraOrientation.setValue(cam.orientation.getValue())
        root.addChild(cube)
        return root

    if type_name in (
        "SoPolygon",
        "SoPolygonOpen",
        "SoPolygonStartIndex",
        "SoPolygonNonPlanar",
        "SoPolygonTriangle",
    ):
        coords = coin.SoCoordinate3()
        if type_name == "SoPolygonStartIndex":
            # Use a non-zero startIndex and a non-trivial vertex slice.
            coords.point.setValues(
                0,
                6,
                [
                    coin.SbVec3f(-0.9, -0.9, 0.0),  # unused (startIndex=1)
                    coin.SbVec3f(-0.6, -0.4, 0.0),
                    coin.SbVec3f(0.7, -0.6, 0.0),
                    coin.SbVec3f(0.6, 0.6, 0.0),
                    coin.SbVec3f(-0.4, 0.7, 0.0),
                    coin.SbVec3f(0.9, 0.9, 0.0),  # unused
                ],
            )
        elif type_name == "SoPolygonTriangle":
            coords.point.setValues(
                0,
                5,
                [
                    coin.SbVec3f(-0.7, -0.5, 0.0),
                    coin.SbVec3f(0.8, -0.6, 0.0),
                    coin.SbVec3f(-0.1, 0.8, 0.0),
                    coin.SbVec3f(0.9, 0.9, 0.0),  # unused
                    coin.SbVec3f(-0.9, 0.9, 0.0),  # unused
                ],
            )
        elif type_name == "SoPolygonNonPlanar":
            coords.point.setValues(
                0,
                5,
                [
                    coin.SbVec3f(-0.7, -0.7, 0.0),
                    coin.SbVec3f(0.7, -0.7, 0.4),
                    coin.SbVec3f(0.8, 0.6, -0.2),
                    coin.SbVec3f(-0.6, 0.8, 0.3),
                    coin.SbVec3f(-0.7, -0.7, 0.0),
                ],
            )
        elif type_name == "SoPolygonOpen":
            coords.point.setValues(
                0,
                4,
                [
                    coin.SbVec3f(-0.8, -0.6, 0.0),
                    coin.SbVec3f(0.6, -0.7, 0.0),
                    coin.SbVec3f(0.8, 0.4, 0.0),
                    coin.SbVec3f(-0.4, 0.7, 0.0),
                ],
            )
        else:
            # Closed loop (last point repeats).
            coords.point.setValues(
                0,
                5,
                [
                    coin.SbVec3f(-0.7, -0.7, 0.0),
                    coin.SbVec3f(0.7, -0.7, 0.0),
                    coin.SbVec3f(0.7, 0.7, 0.0),
                    coin.SbVec3f(-0.7, 0.7, 0.0),
                    coin.SbVec3f(-0.7, -0.7, 0.0),
                ],
            )
        material = coin.SoMaterial()
        if type_name == "SoPolygonOpen":
            material.diffuseColor.setValue(0.80, 0.20, 0.10)
        elif type_name == "SoPolygonStartIndex":
            material.diffuseColor.setValue(0.10, 0.60, 0.20)
        elif type_name == "SoPolygonTriangle":
            material.diffuseColor.setValue(0.95, 0.55, 0.10)
        elif type_name == "SoPolygonNonPlanar":
            material.diffuseColor.setValue(0.65, 0.20, 0.75)
        else:
            material.diffuseColor.setValue(0.10, 0.25, 0.80)
        poly = _instantiate(coin, "SoPolygon")
        if type_name == "SoPolygonStartIndex":
            poly.startIndex.setValue(1)
            poly.numVertices.setValue(4)
        elif type_name == "SoPolygonTriangle":
            poly.startIndex.setValue(0)
            poly.numVertices.setValue(3)
        else:
            poly.startIndex.setValue(0)
            poly.numVertices.setValue(coords.point.getNum())
        poly.render.setValue(True)
        root.addChild(coords)
        root.addChild(material)
        root.addChild(poly)
        return root

    if type_name in (
        "SoFCIndexedFaceSet",
        "SoFCIndexedFaceSetPerFaceColor",
        "SoFCIndexedFaceSetPerVertexColor",
        "SoFCIndexedFaceSetTranslucent",
    ):
        # SoFCIndexedFaceSet test geometry does not provide normals. Without normals, lighting
        # contributes only ambient, which makes all material variants appear nearly black.
        # Provide one normal per face (triangle) so PER_FACE/PER_VERTEX material bindings
        # visibly differ under the default directional light.
        normals = coin.SoNormal()
        normals.vector.setValues(
            0,
            12,
            [
                coin.SbVec3f(0.0, 0.0, -1.0),
                coin.SbVec3f(0.0, 0.0, -1.0),
                coin.SbVec3f(0.0, 0.0, 1.0),
                coin.SbVec3f(0.0, 0.0, 1.0),
                coin.SbVec3f(0.0, -1.0, 0.0),
                coin.SbVec3f(0.0, -1.0, 0.0),
                coin.SbVec3f(1.0, 0.0, 0.0),
                coin.SbVec3f(1.0, 0.0, 0.0),
                coin.SbVec3f(0.0, 1.0, 0.0),
                coin.SbVec3f(0.0, 1.0, 0.0),
                coin.SbVec3f(-1.0, 0.0, 0.0),
                coin.SbVec3f(-1.0, 0.0, 0.0),
            ],
        )
        normal_bind = coin.SoNormalBinding()
        normal_bind.value = coin.SoNormalBinding.PER_FACE
        root.addChild(normals)
        root.addChild(normal_bind)

        if type_name == "SoFCIndexedFaceSetTranslucent":
            # A single quad makes the material's alpha directly observable.
            # A closed cube would blend its front and back faces together,
            # making the result look nearly opaque.
            _add_gradient_background(
                root,
                coin,
                _TRANSLUCENCY_BACKGROUND_TOP,
                _TRANSLUCENCY_BACKGROUND_BOTTOM,
            )
            _add_translucency_backplate(root, coin)

        coords = coin.SoCoordinate3()
        faces = _instantiate(coin, "SoFCIndexedFaceSet")
        if type_name == "SoFCIndexedFaceSetTranslucent":
            coords.point.setValues(
                0,
                4,
                [
                    coin.SbVec3f(-0.7, -0.7, 0.0),
                    coin.SbVec3f(0.7, -0.7, 0.0),
                    coin.SbVec3f(0.7, 0.7, 0.0),
                    coin.SbVec3f(-0.7, 0.7, 0.0),
                ],
            )
            faces.coordIndex.setValues(0, 8, [0, 1, 2, -1, 0, 2, 3, -1])
        else:
            # Simple cube: 12 triangles (each terminated by -1).
            coords.point.setValues(
                0,
                8,
                [
                    coin.SbVec3f(-0.6, -0.6, -0.6),
                    coin.SbVec3f(0.6, -0.6, -0.6),
                    coin.SbVec3f(0.6, 0.6, -0.6),
                    coin.SbVec3f(-0.6, 0.6, -0.6),
                    coin.SbVec3f(-0.6, -0.6, 0.6),
                    coin.SbVec3f(0.6, -0.6, 0.6),
                    coin.SbVec3f(0.6, 0.6, 0.6),
                    coin.SbVec3f(-0.6, 0.6, 0.6),
                ],
            )
            # fmt: off
            faces.coordIndex.setValues(0, 48, [
                0, 1, 2, -1,  0, 2, 3, -1,  # bottom (-Z)
                4, 6, 5, -1,  4, 7, 6, -1,  # top (+Z)
                0, 4, 5, -1,  0, 5, 1, -1,  # -Y
                1, 5, 6, -1,  1, 6, 2, -1,  # +X
                2, 6, 7, -1,  2, 7, 3, -1,  # +Y
                3, 7, 4, -1,  3, 4, 0, -1,  # -X
            ])
            # fmt: on
        material = coin.SoMaterial()
        if type_name == "SoFCIndexedFaceSetPerFaceColor":
            # 12 faces (triangles).
            material.diffuseColor.setValues(
                0,
                12,
                [
                    coin.SbColor(0.90, 0.25, 0.25),
                    coin.SbColor(0.90, 0.55, 0.25),
                    coin.SbColor(0.90, 0.80, 0.25),
                    coin.SbColor(0.65, 0.90, 0.25),
                    coin.SbColor(0.25, 0.90, 0.25),
                    coin.SbColor(0.25, 0.90, 0.65),
                    coin.SbColor(0.25, 0.90, 0.90),
                    coin.SbColor(0.25, 0.65, 0.90),
                    coin.SbColor(0.25, 0.25, 0.90),
                    coin.SbColor(0.65, 0.25, 0.90),
                    coin.SbColor(0.90, 0.25, 0.90),
                    coin.SbColor(0.90, 0.25, 0.65),
                ],
            )
            bind = coin.SoMaterialBinding()
            bind.value = coin.SoMaterialBinding.PER_FACE
            root.addChild(bind)
        elif type_name == "SoFCIndexedFaceSetPerVertexColor":
            material.diffuseColor.setValues(
                0,
                8,
                [
                    coin.SbColor(0.95, 0.25, 0.25),
                    coin.SbColor(0.95, 0.75, 0.25),
                    coin.SbColor(0.25, 0.95, 0.25),
                    coin.SbColor(0.25, 0.95, 0.95),
                    coin.SbColor(0.25, 0.25, 0.95),
                    coin.SbColor(0.95, 0.25, 0.95),
                    coin.SbColor(0.70, 0.70, 0.70),
                    coin.SbColor(0.15, 0.15, 0.15),
                ],
            )
            bind = coin.SoMaterialBinding()
            # IndexedFaceSet colors are bound by index; use coordIndex as the color index
            # so each corner uses the corresponding material color.
            bind.value = coin.SoMaterialBinding.PER_VERTEX_INDEXED
            coord_index_values = list(faces.coordIndex.getValues(0))
            faces.materialIndex.setValues(
                0,
                faces.coordIndex.getNum(),
                coord_index_values,
            )
            root.addChild(bind)
        else:
            material.diffuseColor.setValue(0.70, 0.70, 0.75)
            if type_name == "SoFCIndexedFaceSetTranslucent":
                material.transparency.setValue(0.55)
                light_model = coin.SoLightModel()
                light_model.model = coin.SoLightModel.BASE_COLOR
                root.addChild(light_model)
        root.addChild(coords)
        root.addChild(material)
        root.addChild(faces)
        return root

    if type_name in ("SoBrepEdgeSet", "SoBrepEdgeSetHighlight", "SoBrepEdgeSetSelection"):
        coords = coin.SoCoordinate3()
        coords.point.setValues(
            0,
            5,
            [
                coin.SbVec3f(-0.6, -0.6, 0.0),
                coin.SbVec3f(0.6, -0.6, 0.0),
                coin.SbVec3f(0.6, 0.6, 0.0),
                coin.SbVec3f(-0.6, 0.6, 0.0),
                coin.SbVec3f(0.0, 0.0, 0.0),
            ],
        )
        style = coin.SoDrawStyle()
        style.lineWidth.setValue(3.0)
        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.05, 0.05, 0.05)

        edges = _instantiate(coin, "SoBrepEdgeSet")
        # Square + diagonals.
        edges.coordIndex.setValues(0, 14, [0, 1, 2, 3, 0, -1, 0, 2, -1, 1, 3, -1, 4, -1])
        if type_name == "SoBrepEdgeSetHighlight":
            edges.highlightCoordIndex.setValues(0, 3, [0, 2, -1])
            edges.highlightColor.setValue(1.0, 0.0, 0.0)
        elif type_name == "SoBrepEdgeSetSelection":
            edges.selectionCoordIndex.setValues(0, 3, [1, 3, -1])
            edges.selectionColor.setValue(0.0, 0.6, 0.0)

        root.addChild(coords)
        root.addChild(style)
        root.addChild(material)
        root.addChild(edges)
        return root

    if type_name in ("SoBrepPointSet", "SoBrepPointSetHighlight", "SoBrepPointSetSelection"):
        coords = coin.SoCoordinate3()
        coords.point.setValues(
            0,
            9,
            [
                coin.SbVec3f(-0.6, -0.6, 0.0),
                coin.SbVec3f(0.0, -0.6, 0.0),
                coin.SbVec3f(0.6, -0.6, 0.0),
                coin.SbVec3f(-0.6, 0.0, 0.0),
                coin.SbVec3f(0.0, 0.0, 0.0),
                coin.SbVec3f(0.6, 0.0, 0.0),
                coin.SbVec3f(-0.6, 0.6, 0.0),
                coin.SbVec3f(0.0, 0.6, 0.0),
                coin.SbVec3f(0.6, 0.6, 0.0),
            ],
        )
        style = coin.SoDrawStyle()
        style.pointSize.setValue(7.0)
        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.05, 0.05, 0.05)

        pts = _instantiate(coin, "SoBrepPointSet")
        pts.startIndex.setValue(0)
        pts.numPoints.setValue(-1)
        if type_name == "SoBrepPointSetHighlight":
            pts.highlightCoordIndex.setValues(0, 1, [4])
            pts.highlightColor.setValue(1.0, 0.0, 0.0)
        elif type_name == "SoBrepPointSetSelection":
            pts.selectionCoordIndex.setValues(0, 4, [0, 2, 6, 8])
            pts.selectionColor.setValue(0.0, 0.6, 0.0)

        root.addChild(coords)
        root.addChild(style)
        root.addChild(material)
        root.addChild(pts)
        return root

    if type_name in ("SoBrepFaceSet", "SoBrepFaceSetHighlight", "SoBrepFaceSetSelection"):
        # Simple cube: 6 parts (faces), 2 triangles each.
        coords = coin.SoCoordinate3()
        coords.point.setValues(
            0,
            8,
            [
                coin.SbVec3f(-0.5, -0.5, -0.5),  # 0
                coin.SbVec3f(0.5, -0.5, -0.5),  # 1
                coin.SbVec3f(0.5, 0.5, -0.5),  # 2
                coin.SbVec3f(-0.5, 0.5, -0.5),  # 3
                coin.SbVec3f(-0.5, -0.5, 0.5),  # 4
                coin.SbVec3f(0.5, -0.5, 0.5),  # 5
                coin.SbVec3f(0.5, 0.5, 0.5),  # 6
                coin.SbVec3f(-0.5, 0.5, 0.5),  # 7
            ],
        )

        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.75, 0.75, 0.78)

        faces = _instantiate(coin, "SoBrepFaceSet")
        # Each triangle ends with -1; partIndex counts triangles per part.
        # fmt: off
        faces.coordIndex.setValues(0, 48, [
            # Bottom (z=-0.5)
            0, 1, 2, -1,  0, 2, 3, -1,
            # Top (z=+0.5)
            4, 6, 5, -1,  4, 7, 6, -1,
            # Front (y=-0.5)
            0, 5, 1, -1,  0, 4, 5, -1,
            # Back (y=+0.5)
            3, 2, 6, -1,  3, 6, 7, -1,
            # Left (x=-0.5)
            0, 3, 7, -1,  0, 7, 4, -1,
            # Right (x=+0.5)
            1, 5, 6, -1,  1, 6, 2, -1,
        ])
        # fmt: on
        faces.partIndex.setValues(0, 6, [2, 2, 2, 2, 2, 2])

        if type_name == "SoBrepFaceSetHighlight":
            # Highlight the front face (part 2).
            faces.highlightPartIndex.setValues(0, 1, [2])
            faces.highlightColor.setValue(1.0, 0.0, 0.0)
        elif type_name == "SoBrepFaceSetSelection":
            # Select a couple of faces to exercise the selection overlay.
            faces.selectionPartIndex.setValues(0, 2, [1, 5])
            faces.selectionColor.setValue(0.0, 0.6, 0.0)

        root.addChild(coords)
        root.addChild(material)
        root.addChild(faces)
        return root

    if type_name == "SoFCControlPoints":
        # A small 3x3 pole grid with 2x2 knots appended.  Use a darker net
        # color so the snapshot clearly covers both the net and knot markers.
        coords = coin.SoCoordinate3()
        pts = []
        for u in (-0.6, 0.0, 0.6):
            for v in (-0.6, 0.0, 0.6):
                pts.append(coin.SbVec3f(u, v, 0.0))
        for u in (-0.3, 0.3):
            for v in (-0.3, 0.3):
                pts.append(coin.SbVec3f(u, v, 0.15))
        coords.point.setValues(0, len(pts), pts)

        cp = _instantiate(coin, "SoFCControlPoints")
        cp.numPolesU.setValue(3)
        cp.numPolesV.setValue(3)
        cp.numKnotsU.setValue(2)
        cp.numKnotsV.setValue(2)
        cp.lineColor.setValue(0.45, 0.12, 0.05)

        root.addChild(coords)
        root.addChild(cp)
        return root

    root.addChild(_instantiate(coin, type_name))
    return root


def _make_snapshot_scene(coin, type_name: str) -> _SnapshotScene:
    fixture = _SNAPSHOT_FIXTURES.get(type_name, _SnapshotFixture())
    return _SnapshotScene(
        root=_make_scene_for_node(coin, type_name, fixture),
        framing_policy=fixture.framing_policy,
    )


class _ViewerSnapshotHarness:
    def __init__(self, FreeCAD, FreeCADGui, width: int, height: int):
        self.FreeCAD = FreeCAD
        self.FreeCADGui = FreeCADGui
        self.width = width
        self.height = height
        self.previous_document = FreeCAD.activeDocument()
        self.doc = None
        self.view = None
        self.viewer = None

        try:
            self.doc = FreeCAD.newDocument("CoinNodeSnapshotHarness")
            FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
            FreeCADGui.updateGui()
            gui_doc = FreeCADGui.getDocument(self.doc.Name)
            if gui_doc is None:
                raise AssertionError(f"failed to resolve GUI document for {self.doc.Name}")

            self.view = gui_doc.ActiveView
            self.viewer = self.view.getViewer()
            self.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            self.viewer.setGradientBackground("NONE")
            graphics_view = self.view.graphicsView()
            graphics_view.resize(width, height)
            graphics_view.show()
            self._wait_until(
                lambda: graphics_view.isVisible()
                and graphics_view.width() > 0
                and graphics_view.height() > 0,
                "3D view did not become ready",
            )
        except Exception:
            self.close()
            raise

    def __enter__(self):
        return self

    def __exit__(self, _exc_type, _exc_value, _traceback):
        self.close()

    def _wait_until(self, predicate, message: str, *, timeout: float = 5.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self.FreeCADGui.updateGui()
            if predicate():
                return
            time.sleep(0.01)
        raise AssertionError(message)

    def render(self, root, out_path: Path):
        self.viewer.setSceneGraph(root)
        image = None

        def capture_ready():
            nonlocal image
            image = self.viewer.renderToImage(
                width=self.width,
                height=self.height,
                samples=0,
                includeViewerLighting=False,
            )
            return not image.isNull() and image.width() > 0 and image.height() > 0

        self._wait_until(capture_ready, "viewer framebuffer did not become ready")
        if image.width() != self.width or image.height() != self.height:
            raise AssertionError(
                "viewer framebuffer dimensions do not match the requested snapshot size: "
                f"expected {self.width}x{self.height}, got {image.width()}x{image.height()}"
            )
        out_path.parent.mkdir(parents=True, exist_ok=True)
        if not image.save(str(out_path)):
            raise AssertionError(f"failed to save snapshot image: {out_path}")

    def close(self):
        self.viewer = None
        self.view = None

        if self.doc is not None and self.FreeCAD.getDocument(self.doc.Name):
            document_name = self.doc.Name
            self.FreeCAD.closeDocument(document_name)
            self._wait_until(
                lambda: document_name not in self.FreeCAD.listDocuments(),
                f"document {document_name} did not close",
            )
        self.doc = None

        if self.previous_document is not None and self.FreeCAD.getDocument(
            self.previous_document.Name
        ):
            self.FreeCAD.setActiveDocument(self.previous_document.Name)
            self.FreeCADGui.ActiveDocument = self.FreeCADGui.getDocument(
                self.previous_document.Name
            )


def _render_png(
    harness,
    coin,
    root,
    out_path: Path,
    width: int,
    height: int,
    *,
    framing_policy: _CameraPolicy = _CameraPolicy.VIEW_ALL,
) -> None:
    viewport = coin.SbViewportRegion(width, height)
    if framing_policy in (_CameraPolicy.VIEW_ALL, _CameraPolicy.VIEW_ALL_WITH_MARGIN):
        # Tighten the camera to what we're rendering.
        cam = root.getChild(0)
        # Some GUI helper nodes (e.g. SoDatumLabel) compute a camera-dependent bounding box.
        # That makes `SoCamera::viewAll()` unstable and can shift the framing of the snapshot.
        # For these, frame the camera from the rest of the scene and render the full graph.
        removed = None
        dtype = coin.SoType.fromName("SoDatumLabel")
        if not dtype.isBad():
            search = coin.SoSearchAction()
            search.setType(dtype)
            search.setSearchingAll(False)
            search.apply(root)
            path = search.getPath()
            if path is not None and path.getLength() >= 2:
                label = path.getTail()
                parent = path.getNode(path.getLength() - 2)
                idx = parent.findChild(label)
                if idx >= 0:
                    # Keep the node alive while it's detached (Coin ref-counting).
                    label.ref()
                    parent.removeChild(idx)
                    removed = (parent, idx, label)

        cam.viewAll(root, viewport)

        if removed is not None:
            parent, idx, label = removed
            parent.insertChild(label, idx)
            label.unref()
        # `SoCamera::viewAll()` can choose a near plane that clips geometry located near the origin.
        # This shows up particularly with `SoText2` (text draws, but gets clipped away).
        cam.nearDistance.setValue(min(cam.nearDistance.getValue(), 0.1))
        if framing_policy is _CameraPolicy.VIEW_ALL_WITH_MARGIN:
            cam.height.setValue(cam.height.getValue() * 1.25)

    harness.render(root, out_path)


def _non_background_pixel_count(path: Path) -> int:
    from PySide.QtGui import QImage  # type: ignore

    img = QImage(str(path))
    if img.isNull():
        return 0
    white = 0xFFFFFFFF
    count = 0
    for y in range(img.height()):
        for x in range(img.width()):
            if img.pixel(x, y) != white:
                count += 1
    return count


def _pixel_bbox(path: Path, predicate):
    from PySide.QtGui import QImage  # type: ignore

    img = QImage(str(path))
    if img.isNull():
        return None

    min_x = img.width()
    min_y = img.height()
    max_x = -1
    max_y = -1

    for y in range(img.height()):
        for x in range(img.width()):
            pixel = img.pixel(x, y)
            r = (pixel >> 16) & 0xFF
            g = (pixel >> 8) & 0xFF
            b = pixel & 0xFF
            a = (pixel >> 24) & 0xFF
            if not predicate(r, g, b, a):
                continue

            min_x = min(min_x, x)
            min_y = min(min_y, y)
            max_x = max(max_x, x)
            max_y = max(max_y, y)

    if max_x < min_x or max_y < min_y:
        return None

    return (min_x, min_y, max_x, max_y)


def _mean_rgb(path: Path, x: int, y: int, radius: int = 2) -> tuple[float, float, float]:
    from PySide.QtGui import QImage  # type: ignore

    img = QImage(str(path)).convertToFormat(QImage.Format_ARGB32)
    if img.isNull():
        raise AssertionError(f"unreadable image: {path}")

    r_total = 0
    g_total = 0
    b_total = 0
    count = 0
    for yy in range(max(0, y - radius), min(img.height(), y + radius + 1)):
        for xx in range(max(0, x - radius), min(img.width(), x + radius + 1)):
            pixel = img.pixel(xx, yy)
            r_total += (pixel >> 16) & 0xFF
            g_total += (pixel >> 8) & 0xFF
            b_total += pixel & 0xFF
            count += 1

    if count == 0:
        raise AssertionError(f"sample window around ({x}, {y}) is empty for {path}")

    return (r_total / count, g_total / count, b_total / count)


def _bbox_relative_point(
    bbox: tuple[int, int, int, int], x_ratio: float, y_ratio: float
) -> tuple[int, int]:
    min_x, min_y, max_x, max_y = bbox
    width = max(1, max_x - min_x)
    height = max(1, max_y - min_y)
    x = min_x + int(width * x_ratio)
    y = min_y + int(height * y_ratio)
    return (x, y)


def _make_top_view_scene(coin, node, *, center: tuple[float, float, float], camera_height: float):
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    cam.position.setValue(center[0], center[1], center[2] + 30.0)
    cam.nearDistance.setValue(1.0)
    cam.farDistance.setValue(100.0)
    cam.height.setValue(camera_height)
    root.addChild(cam)

    light_model = coin.SoLightModel()
    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
    root.addChild(light_model)

    root.addChild(node)
    return root


def _compare_images(
    expected_path: Path,
    actual_path: Path,
    diff_path: Path,
    *,
    tolerance: int,
    ignore_alpha: bool,
    max_mismatched_pixels: int,
) -> tuple[bool, str]:
    from PySide.QtGui import QColor, QImage  # type: ignore

    # Structural Similarity Index (SSIM) provides a more robust signal than raw per-pixel diffs
    # when tiny rasterization differences occur across platforms/drivers.
    #
    # This is a lightweight block-SSIM implementation to avoid non-stdlib deps (e.g. numpy).
    _SSIM_MIN = 0.97
    _SSIM_BLOCK = 8
    _SSIM_C1 = (0.01 * 255.0) ** 2
    _SSIM_C2 = (0.03 * 255.0) ** 2

    expected = QImage(str(expected_path))
    actual = QImage(str(actual_path))

    if expected.isNull():
        return False, f"baseline is not a readable image: {expected_path}"
    if actual.isNull():
        return False, f"actual is not a readable image: {actual_path}"

    if expected.size() != actual.size():
        diff_path.parent.mkdir(parents=True, exist_ok=True)
        diff = QImage(actual.size(), QImage.Format_ARGB32)
        diff.fill(QColor(255, 0, 255, 255))
        diff.save(str(diff_path))
        msg = (
            f"size mismatch: expected {expected.width()}x{expected.height()} "
            f"vs actual {actual.width()}x{actual.height()}"
        )
        return (
            False,
            msg,
        )

    expected = expected.convertToFormat(QImage.Format_ARGB32)
    actual = actual.convertToFormat(QImage.Format_ARGB32)

    diff = QImage(actual.size(), QImage.Format_ARGB32)
    mismatched = 0

    blocks_w = (actual.width() + _SSIM_BLOCK - 1) // _SSIM_BLOCK
    blocks_h = (actual.height() + _SSIM_BLOCK - 1) // _SSIM_BLOCK
    blocks_n = blocks_w * blocks_h
    sum_e = [0.0] * blocks_n
    sum_a = [0.0] * blocks_n
    sumsq_e = [0.0] * blocks_n
    sumsq_a = [0.0] * blocks_n
    sum_cross = [0.0] * blocks_n
    count = [0] * blocks_n

    def _luma(r: int, g: int, b: int) -> float:
        # Integer-ish Rec.709 luma approximation (coeffs sum to 256).
        return float((54 * r + 183 * g + 19 * b) >> 8)

    for y in range(actual.height()):
        for x in range(actual.width()):
            ep = expected.pixel(x, y)
            ap = actual.pixel(x, y)

            er = (ep >> 16) & 0xFF
            eg = (ep >> 8) & 0xFF
            eb = ep & 0xFF
            ea = (ep >> 24) & 0xFF

            ar = (ap >> 16) & 0xFF
            ag = (ap >> 8) & 0xFF
            ab = ap & 0xFF
            aa = (ap >> 24) & 0xFF

            by = y // _SSIM_BLOCK
            bx = x // _SSIM_BLOCK
            bi = by * blocks_w + bx
            ye = _luma(er, eg, eb)
            ya = _luma(ar, ag, ab)
            sum_e[bi] += ye
            sum_a[bi] += ya
            sumsq_e[bi] += ye * ye
            sumsq_a[bi] += ya * ya
            sum_cross[bi] += ye * ya
            count[bi] += 1

            dr = abs(er - ar)
            dg = abs(eg - ag)
            db = abs(eb - ab)
            da = abs(ea - aa) if not ignore_alpha else 0

            dmax = max(dr, dg, db, da)
            if dmax > tolerance:
                mismatched += 1
                diff.setPixel(x, y, QColor(255, 0, 0, 255).rgba())
            else:
                # Keep a lightly-dimmed version of the actual pixel for context.
                diff.setPixel(x, y, QColor(ar // 2, ag // 2, ab // 2, 255).rgba())

    if mismatched > 0:
        diff_path.parent.mkdir(parents=True, exist_ok=True)
        diff.save(str(diff_path))

    ssim_total = 0.0
    ssim_blocks = 0
    for i in range(blocks_n):
        n = count[i]
        if n <= 0:
            continue
        mu_e = sum_e[i] / float(n)
        mu_a = sum_a[i] / float(n)
        var_e = (sumsq_e[i] / float(n)) - (mu_e * mu_e)
        var_a = (sumsq_a[i] / float(n)) - (mu_a * mu_a)
        cov = (sum_cross[i] / float(n)) - (mu_e * mu_a)
        if var_e < 0.0:
            var_e = 0.0
        if var_a < 0.0:
            var_a = 0.0

        num = (2.0 * mu_e * mu_a + _SSIM_C1) * (2.0 * cov + _SSIM_C2)
        den = (mu_e * mu_e + mu_a * mu_a + _SSIM_C1) * (var_e + var_a + _SSIM_C2)
        if den == 0.0:
            # Identical constant blocks.
            ssim = 1.0 if num == 0.0 else 0.0
        else:
            ssim = num / den
        ssim_total += ssim
        ssim_blocks += 1

    ssim_avg = ssim_total / float(ssim_blocks or 1)

    if mismatched > max_mismatched_pixels:
        if ssim_avg >= _SSIM_MIN:
            return (
                True,
                f"mismatched pixels {mismatched} > {max_mismatched_pixels}, "
                f"but SSIM={ssim_avg:.4f} >= {_SSIM_MIN:.2f}",
            )

        return (
            False,
            f"mismatched pixels {mismatched} > {max_mismatched_pixels} "
            f"(tolerance={tolerance}, ignore_alpha={ignore_alpha}, SSIM={ssim_avg:.4f} < {_SSIM_MIN:.2f})",
        )

    return True, f"mismatched pixels {mismatched} <= {max_mismatched_pixels} (SSIM={ssim_avg:.4f})"


class CoinNodeSnapshotTestCase(unittest.TestCase):
    """Render Coin nodes through the live viewer and compare against PNG baselines."""

    def test_so_reg_point_unpickable_regression(self):
        _, _, coin = _require_gui()

        root = coin.SoSeparator()
        probe = _instantiate(coin, "SoRegPoint")
        probe.base.setValue(0.0, 0.0, 0.0)
        probe.normal.setValue(0.0, 0.0, 1.0)
        probe.length.setValue(1.0)
        probe.text.setValue("probe")
        root.addChild(probe)

        pick = coin.SoRayPickAction(coin.SbViewportRegion(512, 512))
        pick.setRadius(8.0)
        pick.setPickAll(True)
        pick.setRay(coin.SbVec3f(0.0, 0.0, 5.0), coin.SbVec3f(0.0, 0.0, -1.0), 0.0, 10.0)
        pick.apply(root)

        self.assertIsNone(
            pick.getPickedPoint(),
            "SoRegPoint should stay unpickable so manual-alignment markers do not intercept clicks",
        )

    def test_so_datum_label_ignores_parent_cull_face(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width = _SNAPSHOT_WIDTH
        height = _SNAPSHOT_HEIGHT
        out_dir = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
            )
        )
        actual_path = out_dir / "actual" / "SoDatumLabelCullFaceRegression.png"

        root = coin.SoSeparator()

        cam = coin.SoOrthographicCamera()
        cam.position.setValue(0.0, 0.0, 2.0)
        cam.nearDistance.setValue(1.0)
        cam.farDistance.setValue(5.0)
        cam.height.setValue(2.0)
        root.addChild(cam)

        light = coin.SoDirectionalLight()
        root.addChild(light)

        callback = coin.SoCallback()

        def _enable_backface_culling(userdata, action):
            if not action.isOfType(coin.SoGLRenderAction.getClassTypeId()):
                return
            coin.SoLazyElement.setBackfaceCulling(action.getState(), True)

        callback.setCallback(_enable_backface_culling, None)
        root.addChild(callback)

        transform = coin.SoTransform()
        transform.rotation.setValue(coin.SbRotation(coin.SbVec3f(0.0, 1.0, 0.0), 3.141592653589793))
        root.addChild(transform)

        label = _instantiate(coin, "SoDatumLabel")
        label.string.setValue("CullFace")
        label.textColor.setValue(0.0, 0.4, 0.9)
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(28)
        label.lineWidth.setValue(1.0)
        label.sampling.setValue(2.0)
        # `Gui::SoDatumLabel::Type::DISTANCE == 1`.
        label.datumtype.setValue(1)
        label.param1.setValue(0.18)
        label.param2.setValue(0.0)
        label.pnts.setValues(
            0,
            2,
            [coin.SbVec3f(-0.08, -0.02, 0.0), coin.SbVec3f(0.08, 0.02, 0.0)],
        )
        root.addChild(label)

        with _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height) as harness:
            _render_png(
                harness,
                coin,
                root,
                actual_path,
                width,
                height,
                framing_policy=_CameraPolicy.FIXED_OVERLAY,
            )
        self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
        self.assertGreater(
            _non_background_pixel_count(actual_path),
            50,
            f"SoDatumLabel text disappeared under inherited face culling: {actual_path}",
        )

    def test_so_string_label_anchor_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width = _SNAPSHOT_WIDTH
        height = _SNAPSHOT_HEIGHT
        out_dir = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
            )
        )
        actual_path = out_dir / "actual" / "SoStringLabelAnchorRegression.png"

        root = coin.SoSeparator()

        cam = coin.SoOrthographicCamera()
        cam.position.setValue(0.0, 0.0, 2.0)
        cam.nearDistance.setValue(1.0)
        cam.farDistance.setValue(5.0)
        cam.height.setValue(2.0)
        root.addChild(cam)

        light = coin.SoDirectionalLight()
        root.addChild(light)

        font = coin.SoFont()
        font.name.setValue(_DEFAULT_FONT_FAMILY)
        font.size.setValue(28.0)
        root.addChild(font)

        marker_material = coin.SoMaterial()
        marker_material.diffuseColor.setValue(1.0, 0.0, 0.0)
        root.addChild(marker_material)

        marker_style = coin.SoDrawStyle()
        marker_style.pointSize.setValue(11.0)
        root.addChild(marker_style)

        marker_coords = coin.SoCoordinate3()
        marker_coords.point.setValues(0, 1, [coin.SbVec3f(0.0, 0.0, 0.0)])
        root.addChild(marker_coords)

        marker = coin.SoPointSet()
        marker.numPoints.setValue(1)
        root.addChild(marker)

        text_material = coin.SoMaterial()
        text_material.diffuseColor.setValue(0.05, 0.05, 0.05)
        root.addChild(text_material)

        label = _instantiate(coin, "SoStringLabel")
        label.string.setValues(0, 2, ["OOOO", "OOOO"])
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(28)
        label.textColor.setValue(0.0, 0.0, 0.0)
        root.addChild(label)

        with _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height) as harness:
            _render_png(
                harness,
                coin,
                root,
                actual_path,
                width,
                height,
                framing_policy=_CameraPolicy.FIXED_OVERLAY,
            )
        self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")

        marker_bbox = _pixel_bbox(
            actual_path,
            lambda r, g, b, _a: r >= 200 and g <= 80 and b <= 80,
        )
        text_bbox = _pixel_bbox(
            actual_path,
            lambda r, g, b, _a: max(r, g, b) <= 100 and (max(r, g, b) - min(r, g, b)) <= 24,
        )

        self.assertIsNotNone(marker_bbox, f"origin marker not visible: {actual_path}")
        self.assertIsNotNone(text_bbox, f"text pixels not visible: {actual_path}")

        marker_min_x, marker_min_y, marker_max_x, marker_max_y = marker_bbox
        text_min_x, text_min_y, text_max_x, _text_max_y = text_bbox

        marker_center_x = (marker_min_x + marker_max_x) / 2.0
        marker_center_y = (marker_min_y + marker_max_y) / 2.0
        text_center_x = (text_min_x + text_max_x) / 2.0

        self.assertLessEqual(
            abs(text_center_x - marker_center_x),
            6.0,
            (
                "SoStringLabel should stay horizontally centered on its projected origin "
                f"(marker bbox={marker_bbox}, text bbox={text_bbox}, image={actual_path})"
            ),
        )
        self.assertGreaterEqual(
            text_min_y,
            marker_center_y - 2.0,
            (
                "SoStringLabel should extend downward from its projected origin "
                f"(marker bbox={marker_bbox}, text bbox={text_bbox}, image={actual_path})"
            ),
        )

    def test_so_brep_face_set_per_part_material_binding_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        _load_required_modules(_SnapshotFixture(required_modules=("PartGui",)))

        width = _SNAPSHOT_WIDTH
        height = _SNAPSHOT_HEIGHT
        out_dir = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
            )
        )
        actual_path = out_dir / "actual" / "SoBrepFaceSetPerPartMaterialBindingRegression.png"

        root = coin.SoSeparator()

        cam = coin.SoOrthographicCamera()
        cam.position.setValue(0.0, 0.0, 2.0)
        cam.nearDistance.setValue(1.0)
        cam.farDistance.setValue(5.0)
        cam.height.setValue(2.0)
        root.addChild(cam)

        root.addChild(coin.SoDirectionalLight())

        light_model = coin.SoLightModel()
        light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
        root.addChild(light_model)

        coords = coin.SoCoordinate3()
        coords.point.setValues(
            0,
            8,
            [
                coin.SbVec3f(-0.7, -0.7, 0.0),
                coin.SbVec3f(0.7, -0.7, 0.0),
                coin.SbVec3f(0.7, 0.7, 0.0),
                coin.SbVec3f(-0.7, 0.7, 0.0),
                coin.SbVec3f(-0.7, -0.7, -0.2),
                coin.SbVec3f(0.7, -0.7, -0.2),
                coin.SbVec3f(0.7, 0.7, -0.2),
                coin.SbVec3f(-0.7, 0.7, -0.2),
            ],
        )
        root.addChild(coords)

        material = coin.SoMaterial()
        material.diffuseColor.setValues(
            0,
            2,
            [
                coin.SbColor(0.90, 0.15, 0.15),
                coin.SbColor(0.15, 0.75, 0.20),
            ],
        )
        root.addChild(material)

        material_binding = coin.SoMaterialBinding()
        material_binding.value = coin.SoMaterialBinding.PER_PART
        root.addChild(material_binding)

        faces = _instantiate(coin, "SoBrepFaceSet")
        faces.coordIndex.setValues(
            0,
            16,
            [
                0,
                1,
                2,
                -1,
                0,
                2,
                3,
                -1,
                4,
                5,
                6,
                -1,
                4,
                6,
                7,
                -1,
            ],
        )
        faces.partIndex.setValues(0, 2, [2, 2])
        root.addChild(faces)

        with _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height) as harness:
            _render_png(
                harness,
                coin,
                root,
                actual_path,
                width,
                height,
                framing_policy=_CameraPolicy.FIXED_OVERLAY,
            )
        self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
        self.assertGreater(
            _non_background_pixel_count(actual_path),
            1000,
            f"SoBrepFaceSet per-part regression render seems empty: {actual_path}",
        )

        top_left_rgb = _mean_rgb(actual_path, int(width * 0.35), int(height * 0.35), radius=8)
        bottom_right_rgb = _mean_rgb(actual_path, int(width * 0.65), int(height * 0.65), radius=8)

        for label, rgb in (
            ("top-left triangle", top_left_rgb),
            ("bottom-right triangle", bottom_right_rgb),
        ):
            r, g, b = rgb
            self.assertGreater(
                r,
                g + 60.0,
                f"{label} should keep the front face's red material (got {rgb}, image={actual_path})",
            )
            self.assertGreater(
                r,
                b + 60.0,
                f"{label} should keep the front face's red material (got {rgb}, image={actual_path})",
            )
            self.assertGreater(
                r,
                120.0,
                f"{label} should stay visibly red (got {rgb}, image={actual_path})",
            )

    def test_so_brep_face_set_partial_render_transparency_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        _load_required_modules(_SnapshotFixture(required_modules=("Part", "PartGui")))

        width = _SNAPSHOT_WIDTH
        height = _SNAPSHOT_HEIGHT
        out_dir = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
            )
        )
        actual_path = out_dir / "actual" / "SoBrepFaceSetPartialRenderTransparencyRegression.png"

        doc = FreeCAD.newDocument("SoBrepFaceSetPartialRenderTransparencyRegression")
        try:
            box = doc.addObject("Part::Box", "Box")
            doc.recompute()
            FreeCADGui.setActiveDocument(doc.Name)
            gui_doc = FreeCADGui.getDocument(doc.Name) or FreeCADGui.activeDocument()
            if gui_doc is None:
                raise AssertionError(f"failed to resolve Gui document for {doc.Name}")

            box.ViewObject.DiffuseColor = [
                (0.90, 0.18, 0.18, 1.0),
                (0.18, 0.78, 0.22, 1.0),
                (0.18, 0.38, 0.92, 1.0),
                (0.94, 0.82, 0.18, 1.0),
                (0.82, 0.28, 0.86, 1.0),
                (0.92, 0.18, 0.18, 1.0),
            ]
            box.ViewObject.Transparency = 35
            box.ViewObject.partialRender(["Face6"], False)
            FreeCADGui.updateGui()

            root = _make_top_view_scene(
                coin,
                box.ViewObject.RootNode,
                center=(5.0, 5.0, 5.0),
                camera_height=14.0,
            )
            with _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height) as harness:
                _render_png(
                    harness,
                    coin,
                    root,
                    actual_path,
                    width,
                    height,
                    framing_policy=_CameraPolicy.FIXED_OVERLAY,
                )

            self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
            self.assertGreater(
                _non_background_pixel_count(actual_path),
                1000,
                f"partial transparent face render seems empty: {actual_path}",
            )

            bbox = _pixel_bbox(actual_path, lambda r, g, b, _a: min(r, g, b) < 250)
            self.assertIsNotNone(
                bbox, f"partial transparent face render produced no visible pixels: {actual_path}"
            )

            top_left_rgb = _mean_rgb(actual_path, *_bbox_relative_point(bbox, 0.35, 0.35), radius=8)
            bottom_right_rgb = _mean_rgb(
                actual_path, *_bbox_relative_point(bbox, 0.65, 0.65), radius=8
            )

            for label, rgb in (
                ("top-left triangle", top_left_rgb),
                ("bottom-right triangle", bottom_right_rgb),
            ):
                r, g, b = rgb
                self.assertGreater(
                    r,
                    g + 30.0,
                    f"{label} should keep the partially rendered top face's red color (got {rgb}, image={actual_path})",
                )
                self.assertGreater(
                    r,
                    b + 30.0,
                    f"{label} should keep the partially rendered top face's red color (got {rgb}, image={actual_path})",
                )
                self.assertGreater(
                    r,
                    150.0,
                    f"{label} should stay visibly red even with transparency (got {rgb}, image={actual_path})",
                )
        finally:
            if getattr(FreeCADGui, "Selection", None) is not None:
                FreeCADGui.Selection.clearSelection()
            if FreeCAD.getDocument(doc.Name):
                FreeCAD.closeDocument(doc.Name)

    def test_coin_node_snapshots(self):
        """Render each configured node and compare against baseline images."""
        is_ci = bool(os.environ.get("CI", "").strip())
        is_linux = sys.platform.startswith("linux")
        smoke_mode = is_ci and not is_linux

        FreeCAD, FreeCADGui, coin = _require_gui()

        nodes_env = os.environ.get("FC_VISUAL_NODES", "")
        if nodes_env.strip():
            node_types = [n.strip() for n in nodes_env.split(",") if n.strip()]
        else:
            node_types = list(_SNAPSHOT_FIXTURES)

        width = _SNAPSHOT_WIDTH
        height = _SNAPSHOT_HEIGHT

        out_dir = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
            )
        )

        update_baseline = os.environ.get("FC_VISUAL_UPDATE_BASELINE", "").strip() not in (
            "",
            "0",
            "false",
            "False",
        )
        try:
            baseline_dir = _baseline_dir(create=update_baseline)
        except FileNotFoundError as exc:
            raise unittest.SkipTest(str(exc)) from exc

        if not baseline_dir.is_dir():
            self.fail(
                f"baseline directory not found: {baseline_dir} "
                "(set FC_VISUAL_BASELINE_DIR or run with FC_VISUAL_UPDATE_BASELINE=1)"
            )

        max_mismatched_pixels = int((width * height) * (_MAX_MISMATCH_PCT / 100.0))

        did_render = False
        with _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height) as harness:
            for type_name in node_types:
                with self.subTest(node=type_name):
                    scene = _make_snapshot_scene(coin, type_name)
                    actual_dir = out_dir / "actual"
                    expected_dir = out_dir / "expected"
                    diff_dir = out_dir / "diff"

                    actual_path = actual_dir / f"{type_name}.png"
                    _render_png(
                        harness,
                        coin,
                        scene.root,
                        actual_path,
                        width,
                        height,
                        framing_policy=scene.framing_policy,
                    )
                    self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                    self.assertGreater(
                        actual_path.stat().st_size, 0, f"empty snapshot: {actual_path}"
                    )
                    self.assertGreater(
                        _non_background_pixel_count(actual_path),
                        10,
                        f"snapshot seems empty (all background): {actual_path}",
                    )
                    did_render = True
                    baseline_path = baseline_dir / f"{type_name}.png"

                    if update_baseline:
                        baseline_path.write_bytes(actual_path.read_bytes())
                        continue

                    if not baseline_path.exists():
                        if smoke_mode:
                            continue
                        self.fail(
                            f"missing baseline: {baseline_path} "
                            "(run with FC_VISUAL_UPDATE_BASELINE=1)"
                        )

                    expected_path = expected_dir / f"{type_name}.png"
                    expected_dir.mkdir(parents=True, exist_ok=True)
                    expected_path.write_bytes(baseline_path.read_bytes())

                    ok, msg = _compare_images(
                        expected_path,
                        actual_path,
                        diff_dir / f"{type_name}.png",
                        tolerance=_PIXEL_TOLERANCE,
                        ignore_alpha=_IGNORE_ALPHA,
                        max_mismatched_pixels=max_mismatched_pixels,
                    )
                    if smoke_mode:
                        if not ok:
                            print(f"SMOKE mismatch for {type_name}: {msg}")
                    else:
                        self.assertTrue(ok, msg)
        self.assertTrue(did_render, "No snapshots were rendered")
