# SPDX-License-Identifier: LGPL-2.1-or-later

"""
Visual snapshot tests for selected Coin/Inventor nodes.

This test renders a curated set of nodes offscreen and optionally compares the
resulting PNGs against checked-in baselines.

The test is intended to be executed via FreeCAD's test runner:

  FreeCAD -t TestCoinNodeSnapshots

Environment variables:
  - FC_VISUAL_OUT_DIR: output directory (writes actual/expected/diff)
  - FC_VISUAL_BASELINE_DIR: baseline directory override (default: tests/visual/baselines/coin-nodes)
  - FC_VISUAL_UPDATE_BASELINE: if truthy, overwrite baselines with actual renders
  - FC_VISUAL_WIDTH / FC_VISUAL_HEIGHT: render size (default: 512x512)
  - FC_VISUAL_TOLERANCE: per-channel tolerance (default: 8)
  - FC_VISUAL_MAX_MISMATCH_PCT: mismatch threshold percent (default: 0.20)
  - FC_VISUAL_IGNORE_ALPHA: ignore alpha differences (default: 1)
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
import unittest
from pathlib import Path


_BASELINE_REL = Path("tests") / "visual" / "baselines" / "coin-nodes"
_FONT_REL = Path("tests") / "visual" / "fonts"
_DEFAULT_FONT_FAMILY = "Noto Sans"
_DEFAULT_FONT_FILES = ("NotoSans-Regular.ttf",)
_DEFAULT_FONT_SIZE = 18


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


def _make_scene_for_node(coin, type_name: str):
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

    if type_name == "SoDrawingGrid":
        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.7, 0.7, 0.75)
        cube_trans = coin.SoTranslation()
        cube_trans.translation.setValue(0.0, 0.0, -0.5)
        cube = coin.SoCube()
        cube.width = 1.2
        cube.height = 1.2
        cube.depth = 1.2
        root.addChild(material)
        root.addChild(cube_trans)
        root.addChild(cube)
        root.addChild(_instantiate(coin, "SoDrawingGrid"))
        return root

    if type_name == "SoRegPoint":
        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.7, 0.7, 0.75)
        cube_trans = coin.SoTranslation()
        cube_trans.translation.setValue(0.0, 0.0, -0.5)
        cube = coin.SoCube()
        cube.width = 1.2
        cube.height = 1.2
        cube.depth = 1.2
        root.addChild(material)
        root.addChild(cube_trans)
        root.addChild(cube)
        probe = _instantiate(coin, "SoRegPoint")
        probe.base.setValue(0.0, 0.0, 0.0)
        probe.normal.setValue(0.6, 0.7, 0.4)
        probe.length.setValue(1.2)
        probe.color.setValue(1.0, 0.45, 0.34)
        probe.text.setValue("SoRegPoint")
        root.addChild(probe)
        return root

    if type_name == "SoDatumLabel":
        material = coin.SoMaterial()
        material.diffuseColor.setValue(0.7, 0.7, 0.75)
        cube_trans = coin.SoTranslation()
        cube_trans.translation.setValue(0.0, 0.0, -0.5)
        cube = coin.SoCube()
        cube.width = 1.2
        cube.height = 1.2
        cube.depth = 1.2
        root.addChild(material)
        root.addChild(cube_trans)
        root.addChild(cube)
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

    if type_name == "SoTextLabel":
        # Ensure the label isn't white-on-white (SoTextLabel background defaults to white and the
        # inherited material may also be white depending on state defaults).
        mat = coin.SoMaterial()
        mat.diffuseColor.setValue(0.05, 0.05, 0.05)
        root.addChild(mat)

        label = _instantiate(coin, "SoTextLabel")
        label.string.setValues(0, 2, ["SoTextLabel", "Coin geometry"])
        label.background.setValue(True)
        label.backgroundColor.setValue(0.95, 0.95, 0.85)
        label.frameSize.setValue(8.0)
        root.addChild(label)
        return root

    if type_name == "SoStringLabel":
        label = _instantiate(coin, "SoStringLabel")
        label.string.setValue("SoStringLabel")
        label.name.setValue(_DEFAULT_FONT_FAMILY)
        label.size.setValue(18)
        # Default SoStringLabel textColor is white, which can become invisible on the white
        # snapshot background depending on the GL blending / alpha handling.
        label.textColor.setValue(0.05, 0.05, 0.05)
        root.addChild(label)
        return root

    if type_name == "SoFCBackgroundGradient":
        grad = _instantiate(coin, "SoFCBackgroundGradient")
        if hasattr(grad, "setColorGradient"):
            grad.setColorGradient(
                coin.SbColor(0.2, 0.2, 0.6),
                coin.SbColor(0.9, 0.9, 1.0),
            )
        root.addChild(grad)
        return root

    if type_name in ("SoNaviCube", "SoNaviCubeTranslucent", "SoNaviCubeHiliteFront"):
        if type_name == "SoNaviCubeTranslucent":
            # Provide visible background so translucency can be verified.
            grad = _instantiate(coin, "SoFCBackgroundGradient")
            if hasattr(grad, "setColorGradient"):
                top = coin.SbColor(0.15, 0.15, 0.20)
                bottom = coin.SbColor(0.45, 0.45, 0.55)
                grad.setColorGradient(top, bottom)
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
        width = float(int(os.environ.get("FC_VISUAL_WIDTH", "512")))
        height = float(int(os.environ.get("FC_VISUAL_HEIGHT", "512")))

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
    ):
        # These are provided by PartGui, so ensure it is imported to register the Coin types.
        if importlib.util.find_spec("PartGui") is not None:
            importlib.import_module("PartGui")

    if type_name in (
        "SoPolygon",
        "SoPolygonOpen",
        "SoPolygonStartIndex",
        "SoPolygonNonPlanar",
        "SoPolygonTriangle",
        "SoFCIndexedFaceSet",
        "SoFCIndexedFaceSetPerFaceColor",
        "SoFCIndexedFaceSetPerVertexColor",
        "SoFCIndexedFaceSetTranslucent",
    ):
        # These are provided by MeshGui, so ensure it is imported to register the Coin types.
        if importlib.util.find_spec("MeshGui") is not None:
            importlib.import_module("MeshGui")

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
            # Provide visible background so translucency can be verified.
            grad = _instantiate(coin, "SoFCBackgroundGradient")
            if hasattr(grad, "setColorGradient"):
                top = coin.SbColor(0.15, 0.15, 0.20)
                bottom = coin.SbColor(0.95, 0.95, 1.0)
                grad.setColorGradient(top, bottom)
            root.addChild(grad)

        # Simple cube: 12 triangles (each terminated by -1).
        coords = coin.SoCoordinate3()
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
        faces = _instantiate(coin, "SoFCIndexedFaceSet")
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
            faces.highlightPartIndex.setValue(2)
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
        # A small 3x3 pole grid with 2x2 knots appended.
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

        root.addChild(coords)
        root.addChild(cp)
        return root

    root.addChild(_instantiate(coin, type_name))
    return root


def _render_png(FreeCADGui, coin, root, out_path: Path, width: int, height: int) -> None:
    viewport = coin.SbViewportRegion(width, height)
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
    # This shows up particularly with `SoText2`/`SoTextLabel` (text draws, but gets clipped away).
    cam.nearDistance.setValue(min(cam.nearDistance.getValue(), 0.1))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    off = FreeCADGui.SoQtOffscreenRenderer(width, height)
    off.setBackgroundColor(1, 1, 1)
    root.ref()
    off.render(root)
    off.writeToImage(str(out_path))
    root.unref()


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
    """Render Coin nodes offscreen and compare against PNG baselines."""

    def test_coin_node_snapshots(self):
        """Render each configured node and compare against baseline images."""
        is_ci = bool(os.environ.get("CI", "").strip())
        is_linux = sys.platform.startswith("linux")
        smoke_mode = is_ci and not is_linux

        _, FreeCADGui, coin = _require_gui()

        nodes_env = os.environ.get("FC_VISUAL_NODES", "")
        if nodes_env.strip():
            node_types = [n.strip() for n in nodes_env.split(",") if n.strip()]
        else:
            node_types = [
                "SoDrawingGrid",
                "SoRegPoint",
                "SoDatumLabel",
                "SoTextLabel",
                "SoStringLabel",
                "SoFCBackgroundGradient",
                "SoNaviCube",
                "SoNaviCubeTranslucent",
                "SoNaviCubeHiliteFront",
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
                "SoPolygon",
                "SoPolygonOpen",
                "SoPolygonStartIndex",
                "SoPolygonNonPlanar",
                "SoPolygonTriangle",
                "SoFCIndexedFaceSet",
                "SoFCIndexedFaceSetPerFaceColor",
                "SoFCIndexedFaceSetPerVertexColor",
                "SoFCIndexedFaceSetTranslucent",
            ]

        width = int(os.environ.get("FC_VISUAL_WIDTH", "512"))
        height = int(os.environ.get("FC_VISUAL_HEIGHT", "512"))

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

        tolerance = int(os.environ.get("FC_VISUAL_TOLERANCE", "8"))
        tolerance = max(0, min(tolerance, 255))
        ignore_alpha = os.environ.get("FC_VISUAL_IGNORE_ALPHA", "1").strip() not in (
            "",
            "0",
            "false",
            "False",
        )
        max_mismatch_pct = float(os.environ.get("FC_VISUAL_MAX_MISMATCH_PCT", "0.20"))
        max_mismatch_pct = max(0.0, min(max_mismatch_pct, 100.0))
        max_mismatched_pixels = int((width * height) * (max_mismatch_pct / 100.0))

        did_render = False
        for type_name in node_types:
            with self.subTest(node=type_name):
                root = _make_scene_for_node(coin, type_name)
                actual_dir = out_dir / "actual"
                expected_dir = out_dir / "expected"
                diff_dir = out_dir / "diff"

                actual_path = actual_dir / f"{type_name}.png"
                _render_png(FreeCADGui, coin, root, actual_path, width, height)
                self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                self.assertGreater(actual_path.stat().st_size, 0, f"empty snapshot: {actual_path}")
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
                        f"missing baseline: {baseline_path} (run with FC_VISUAL_UPDATE_BASELINE=1)"
                    )

                expected_path = expected_dir / f"{type_name}.png"
                expected_dir.mkdir(parents=True, exist_ok=True)
                expected_path.write_bytes(baseline_path.read_bytes())

                ok, msg = _compare_images(
                    expected_path,
                    actual_path,
                    diff_dir / f"{type_name}.png",
                    tolerance=tolerance,
                    ignore_alpha=ignore_alpha,
                    max_mismatched_pixels=max_mismatched_pixels,
                )
                if smoke_mode:
                    if not ok:
                        print(f"SMOKE mismatch for {type_name}: {msg}")
                else:
                    self.assertTrue(ok, msg)
        self.assertTrue(did_render, "No snapshots were rendered")
