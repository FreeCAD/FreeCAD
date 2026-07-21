# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Live FreeCAD/Coin viewer harness and image utilities for visual tests."""

import os
import sys
import tempfile
import time
import unittest
from pathlib import Path

try:
    from PySide6 import QtGui as _QtGuiModule
    from PySide6 import QtCore as _QtCoreModule
    from PySide6 import QtWidgets as _QtWidgetsModule
except Exception:
    try:
        from PySide import QtCore as _QtCoreModule  # type: ignore
        from PySide import QtGui as _QtGuiModule  # type: ignore

        _QtWidgetsModule = _QtGuiModule  # type: ignore[assignment]
    except Exception:
        _QtCoreModule = None
        _QtGuiModule = None
        _QtWidgetsModule = None

_RENDERER_LEGACY = "legacy"
_RENDERER_DRAW_LIST = "draw_list"
_RENDER_PIPELINES = {
    _RENDERER_LEGACY: "LegacyGL",
    _RENDERER_DRAW_LIST: "DrawList",
}
_BASELINE_REL = Path("tests") / "visual" / "baselines" / "coin-nodes"
_FONT_REL = Path("tests") / "visual" / "fonts"
_DEFAULT_FONT_FAMILY = "Noto Sans"
_DEFAULT_FONT_FILES = ("NotoSans-Regular.ttf",)


def _env_truthy(name: str, default: str = "") -> bool:
    return os.environ.get(name, default).strip().lower() not in ("", "0", "false", "no")


def _snapshot_dimensions() -> tuple[int, int]:
    return (
        int(os.environ.get("FC_VISUAL_WIDTH", "512")),
        int(os.environ.get("FC_VISUAL_HEIGHT", "512")),
    )


def _logical_viewport_size(target_width: int, target_height: int, dpr: float) -> tuple[int, int]:
    if dpr <= 0.0:
        raise ValueError(f"device-pixel ratio must be positive, got {dpr}")
    return (
        max(1, round(target_width / dpr)),
        max(1, round(target_height / dpr)),
    )


def _snapshot_out_dir() -> Path:
    return Path(
        os.environ.get(
            "FC_VISUAL_OUT_DIR",
            os.path.join(tempfile.gettempdir(), "FreeCADTesting", "CoinNodeSnapshots"),
        )
    )


def _repo_root() -> Path | None:
    def _is_source_root(candidate: Path) -> bool:
        return (candidate / "src" / "Mod" / "Test" / "TestCoinNodeSnapshots.py").is_file() and (
            candidate / "src" / "Gui" / "View3DInventorViewer.cpp"
        ).is_file()

    here = Path(__file__).resolve()

    for parent in here.parents:
        cache_path = parent / "CMakeCache.txt"
        if not cache_path.is_file():
            continue
        try:
            cache_lines = cache_path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            continue
        for prefix in ("FreeCAD_SOURCE_DIR:STATIC=", "CMAKE_HOME_DIRECTORY:INTERNAL="):
            for line in cache_lines:
                if not line.startswith(prefix):
                    continue
                candidate = Path(line.split("=", 1)[1].strip()).resolve()
                if _is_source_root(candidate):
                    return candidate
        break

    for parent in here.parents:
        if _is_source_root(parent):
            return parent

    return None


def _baseline_tree_has_pngs(candidate: Path) -> bool:
    if not candidate.is_dir():
        return False
    return any(candidate.rglob("*.png"))


def _find_repo_baseline_dir() -> Path | None:
    repo_root = _repo_root()
    if repo_root is not None:
        candidate = repo_root / _BASELINE_REL
        if _baseline_tree_has_pngs(candidate):
            return candidate

    here = Path(__file__).resolve()
    for parent in here.parents:
        candidate = parent / _BASELINE_REL
        if _baseline_tree_has_pngs(candidate):
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
    if _QtGuiModule is None:
        raise RuntimeError("Qt bindings unavailable")
    font_database = getattr(_QtGuiModule, "QFontDatabase", None)
    if font_database is None:
        raise RuntimeError("QFontDatabase is unavailable")
    for p in font_files:
        font_database.addApplicationFont(str(p))

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

    if not _env_truthy("FC_VISUAL_FORCE_SOFTWARE_GL", "1"):
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
        mod_paths = [mod_dir]
        mod_paths.extend(sorted(p for p in mod_dir.iterdir() if p.is_dir()))
        for p in mod_paths:
            ps = str(p)
            if ps not in sys.path:
                sys.path.insert(0, ps)

    # Ensure a QGuiApplication exists so an OpenGL context can be created.
    try:
        if _QtGuiModule is None:
            raise RuntimeError("Qt bindings unavailable")
        if _QtGuiModule.QGuiApplication.instance() is None:
            _QtGuiModule.QGuiApplication(sys.argv)
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


def _pump_gui_events(FreeCADGui, *, cycles: int = 4):
    if _QtWidgetsModule is None:
        return

    # MDI view creation/destruction is event-driven; snapshot harness setup and teardown
    # needs to let Qt finish those transitions before touching viewer widgets again.
    for _ in range(cycles):
        FreeCADGui.updateGui()
        _QtWidgetsModule.QApplication.processEvents()
        time.sleep(0.05)


def _render_pipelines(viewer) -> list[str]:
    requested = os.environ.get("FC_VISUAL_RENDERERS", "").strip().lower()

    if not requested:
        # Exercise draw-list traversal from a fresh retained scene before legacy
        # rendering can initialize any lazily prepared child geometry.
        return [_RENDERER_DRAW_LIST, _RENDERER_LEGACY]

    modes = []
    seen = set()
    for token in (item.strip() for item in requested.split(",")):
        if not token or token in seen:
            continue
        seen.add(token)
        if token == _RENDERER_LEGACY:
            modes.append(token)
            continue
        if token == _RENDERER_DRAW_LIST:
            modes.append(token)
            continue
        raise ValueError(f"unknown FC_VISUAL_RENDERERS entry: {token!r}")

    if not modes:
        raise unittest.SkipTest("FC_VISUAL_RENDERERS did not select any renderer")
    return modes


def _render_pipelines_for_support(harness, supported_renderers, *, label: str) -> list[str]:
    modes = [name for name in harness.render_pipelines() if name in supported_renderers]
    if modes:
        return modes

    requested = os.environ.get("FC_VISUAL_RENDERERS", "").strip() or "current renderer selection"
    raise unittest.SkipTest(f"{label} does not support {requested}")


def _renderer_output_path(out_dir: Path, renderer_name: str, kind: str, filename: str) -> Path:
    return out_dir / renderer_name / kind / filename


def _comparison_baseline_path(baseline_dir: Path, renderer_name: str, type_name: str) -> Path:
    renderer_path = baseline_dir / renderer_name / f"{type_name}.png"
    if renderer_path.exists():
        return renderer_path

    shared_path = baseline_dir / f"{type_name}.png"
    return shared_path if shared_path.exists() else renderer_path


def _update_baseline_path(baseline_dir: Path, renderer_name: str, type_name: str) -> Path:
    return baseline_dir / renderer_name / f"{type_name}.png"


class _ViewerSnapshotHarness:
    def __init__(self, FreeCAD, FreeCADGui, width: int, height: int):
        self.FreeCAD = FreeCAD
        self.FreeCADGui = FreeCADGui
        self.width = width
        self.height = height
        self.doc = None
        self.gui_doc = None
        self.view = None
        self.viewer = None
        self._closed = False
        active_document = getattr(FreeCAD, "ActiveDocument", None)
        self._previous_document_name = getattr(active_document, "Name", None)

        try:
            self.doc = FreeCAD.newDocument("CoinNodeSnapshotHarness")
            FreeCADGui.setActiveDocument(self.doc.Name)
            _pump_gui_events(FreeCADGui, cycles=4)
            self.gui_doc = FreeCADGui.getDocument(self.doc.Name) or FreeCADGui.activeDocument()
            if self.gui_doc is None:
                raise AssertionError(f"failed to resolve Gui document for {self.doc.Name}")

            self.view = self.gui_doc.ActiveView
            self.viewer = self.view.getViewer()

            self._had_axis_cross = self.view.hasAxisCross()
            self._had_corner_cross = self.view.isCornerCrossVisible()
            self._had_navi_cube = self.viewer.isEnabledNaviCube()
            self._had_render_pipeline = self._get_render_pipeline()
            override_getter = getattr(self.viewer, "getOverrideMode", None)
            self._had_override_mode = override_getter() if callable(override_getter) else None
            background_getter = getattr(self.viewer, "getBackgroundColor", None)
            self._had_background = background_getter() if callable(background_getter) else None
            gradient_getter = getattr(self.viewer, "getGradientBackground", None)
            self._had_gradient = gradient_getter() if callable(gradient_getter) else None

            self.view.setAxisCross(False)
            self.view.setCornerCrossVisible(False)
            self.viewer.setEnabledNaviCube(False)
            self.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            self.viewer.setGradientBackground("NONE")

            self._resize_viewport()
            self._graphics_view().show()
            self.flush(cycles=8)
        except Exception:
            self.close()
            raise

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def close(self):
        if self._closed:
            return
        self._closed = True

        try:
            if self.viewer is not None:
                if self._had_render_pipeline is not None:
                    self.viewer.setRenderPipeline(self._had_render_pipeline)
                if self._had_override_mode is not None:
                    self.viewer.setOverrideMode(self._had_override_mode)
                if self._had_background is not None:
                    self.viewer.setBackgroundColor(*self._had_background[:3])
                if self._had_gradient is not None:
                    self.viewer.setGradientBackground(self._had_gradient)
            if self.view is not None:
                self.view.setAxisCross(self._had_axis_cross)
                self.view.setCornerCrossVisible(self._had_corner_cross)
            if self.viewer is not None:
                self.viewer.setEnabledNaviCube(self._had_navi_cube)
        except Exception:
            # Teardown must still close the temporary document if a binding has already
            # invalidated one of the saved wrappers.
            pass

        try:
            if self.doc is not None and self.FreeCAD.getDocument(self.doc.Name):
                self.FreeCAD.closeDocument(self.doc.Name)
            if self._previous_document_name:
                self.FreeCADGui.setActiveDocument(self._previous_document_name)
        finally:
            self.viewer = None
            self.view = None
            self.gui_doc = None
            self.doc = None
            _pump_gui_events(self.FreeCADGui, cycles=6)

    def _graphics_view(self):
        last_exc = None
        for _ in range(4):
            try:
                # Re-wrap the live viewer widget on demand instead of holding a long-lived
                # Shiboken wrapper across document teardown.
                return self.view.graphicsView()
            except RuntimeError as exc:
                last_exc = exc
                _pump_gui_events(self.FreeCADGui, cycles=2)
                self.gui_doc = (
                    self.FreeCADGui.getDocument(self.doc.Name) or self.FreeCADGui.activeDocument()
                )
                if self.gui_doc is not None:
                    self.view = self.gui_doc.ActiveView
                    self.viewer = self.view.getViewer()
        if last_exc is not None:
            raise last_exc
        raise AssertionError("failed to resolve graphics view for snapshot harness")

    def _resize_viewport(self):
        graphics_view = self._graphics_view()
        ratio_getter = getattr(graphics_view, "devicePixelRatioF", None)
        ratio = float(ratio_getter()) if callable(ratio_getter) else 1.0
        self.device_pixel_ratio = max(ratio, 1.0)
        self.logical_width, self.logical_height = _logical_viewport_size(
            self.width,
            self.height,
            self.device_pixel_ratio,
        )
        graphics_view.resize(self.logical_width, self.logical_height)

    def _capture_framebuffer(self):
        image = self.viewer.grabFramebuffer()
        if image.isNull() or image.width() <= 0 or image.height() <= 0:
            raise AssertionError("viewer framebuffer is empty")
        return image

    def capture_framebuffer(self):
        captures = [self._capture_framebuffer()]
        initial_size = (captures[0].width(), captures[0].height())
        if initial_size == (self.width, self.height):
            return captures[0]

        adjusted_width = self.logical_width
        adjusted_height = self.logical_height
        if captures[0].width() != self.width:
            adjusted_width += -1 if captures[0].width() > self.width else 1
        if captures[0].height() != self.height:
            adjusted_height += -1 if captures[0].height() > self.height else 1

        self._resize_viewport_to(adjusted_width, adjusted_height)
        self.flush(cycles=8)
        captures.append(self._capture_framebuffer())

        valid = [
            image
            for image in captures
            if self.width <= image.width() <= self.width + 1
            and self.height <= image.height() <= self.height + 1
        ]
        if not valid:
            raise AssertionError(
                "viewer framebuffer does not match the requested snapshot size after DPR retry: "
                f"expected {self.width}x{self.height}, got "
                f"{[(image.width(), image.height()) for image in captures]} "
                f"(initial={initial_size}, DPR={self.device_pixel_ratio}, "
                f"logical={self.logical_width}x{self.logical_height})"
            )

        image = min(
            valid,
            key=lambda candidate: (
                candidate.width() - self.width + candidate.height() - self.height
            ),
        )
        if image.width() != self.width or image.height() != self.height:
            image = image.copy(0, 0, self.width, self.height)
            if image.isNull():
                raise AssertionError("failed to crop the one-pixel DPR excess from the framebuffer")
        return image

    def _resize_viewport_to(self, logical_width: int, logical_height: int):
        graphics_view = self._graphics_view()
        self.logical_width = max(1, logical_width)
        self.logical_height = max(1, logical_height)
        graphics_view.resize(self.logical_width, self.logical_height)

    def flush(self, *, cycles: int = 4):
        for _ in range(cycles):
            _pump_gui_events(self.FreeCADGui, cycles=1)
            self.view.redraw()

    def render_pipelines(self) -> list[str]:
        return _render_pipelines(self.viewer)

    def supported_pipelines(self, fixture) -> list[str]:
        supported = getattr(fixture, "supported_renderers", frozenset())
        modes = [name for name in self.render_pipelines() if name in supported]
        if modes:
            return modes
        requested = (
            os.environ.get("FC_VISUAL_RENDERERS", "").strip() or "current renderer selection"
        )
        raise unittest.SkipTest(f"fixture does not support {requested}")

    def _get_render_pipeline(self):
        return self.viewer.getRenderPipeline()

    def set_render_pipeline(self, renderer_name: str):
        render_pipeline = _RENDER_PIPELINES[renderer_name]
        self.viewer.setRenderPipeline(render_pipeline)
        self.flush(cycles=6)
        if self.viewer.getRenderPipeline() != render_pipeline:
            raise AssertionError(f"viewer did not switch render pipeline: renderer={renderer_name}")

    def render_png(
        self, coin, root, out_path: Path, *, renderer_name: str, frame_camera: bool = True
    ):
        # Pivy-created top-level nodes start with a zero reference count. Hold
        # the caller reference across framing and setSceneGraph(); the manager
        # acquires its own reference in setSceneGraph().
        root.ref()
        try:
            if frame_camera:
                from CoinSnapshotScenes import _frame_scene_camera

                _frame_scene_camera(coin, root, self.width, self.height)

            self.set_render_pipeline(renderer_name)
            self._resize_viewport()
            self.viewer.setSceneGraph(root)
            self.view.redraw()
            self.flush(cycles=8)

            image = self.capture_framebuffer()
            out_path.parent.mkdir(parents=True, exist_ok=True)
            if not image.save(str(out_path)):
                raise AssertionError(f"failed to save snapshot image: {out_path}")
        finally:
            root.unref()

    def render_snapshot(
        self,
        scene,
        pipeline: str,
        *,
        coin,
        out_path: Path | None = None,
        frame_camera: bool = True,
    ):
        if out_path is not None:
            self.render_png(
                coin, scene, out_path, renderer_name=pipeline, frame_camera=frame_camera
            )
            return _QtGuiModule.QImage(str(out_path))

        scene.ref()
        try:
            if frame_camera:
                from CoinSnapshotScenes import _frame_scene_camera

                _frame_scene_camera(coin, scene, self.width, self.height)
            self.set_render_pipeline(pipeline)
            self._resize_viewport()
            self.viewer.setSceneGraph(scene)
            self.view.redraw()
            self.flush(cycles=8)
            return self.capture_framebuffer()
        finally:
            scene.unref()


def _render_png(
    harness, coin, root, out_path: Path, renderer_name: str, *, frame_camera: bool = True
) -> None:
    harness.render_png(
        coin,
        root,
        out_path,
        renderer_name=renderer_name,
        frame_camera=frame_camera,
    )


def _non_background_pixel_count(path: Path) -> int:
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")
    QImage = _QtGuiModule.QImage

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
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")
    QImage = _QtGuiModule.QImage

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
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")
    QImage = _QtGuiModule.QImage

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


def _region_pixels(path: Path, region: tuple[int, int, int, int]):
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")
    image = _QtGuiModule.QImage(str(path)).convertToFormat(_QtGuiModule.QImage.Format_ARGB32)
    if image.isNull():
        raise AssertionError(f"unreadable image: {path}")
    x0, y0, x1, y1 = region
    pixels = []
    for y in range(max(0, y0), min(image.height(), y1)):
        for x in range(max(0, x0), min(image.width(), x1)):
            pixel = image.pixel(x, y)
            pixels.append(((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF))
    return pixels


def _region_color_count(
    path: Path,
    region: tuple[int, int, int, int],
    *,
    background: tuple[int, int, int] | None = None,
    threshold: int = 8,
) -> int:
    pixels = _region_pixels(path, region)
    if background is not None:
        pixels = [
            pixel
            for pixel in pixels
            if max(abs(pixel[channel] - background[channel]) for channel in range(3)) > threshold
        ]
    return len(set(pixels))


def _region_luminance_range(
    path: Path,
    region: tuple[int, int, int, int],
    *,
    background: tuple[int, int, int] | None = None,
    threshold: int = 8,
) -> tuple[int, int]:
    pixels = _region_pixels(path, region)
    if background is not None:
        pixels = [
            pixel
            for pixel in pixels
            if max(abs(pixel[channel] - background[channel]) for channel in range(3)) > threshold
        ]
    if not pixels:
        return (0, 0)
    values = [(54 * r + 183 * g + 19 * b) >> 8 for r, g, b in pixels]
    return min(values), max(values)


def _region_pixels_identical(
    lhs_path: Path, rhs_path: Path, region: tuple[int, int, int, int]
) -> bool:
    return _region_pixels(lhs_path, region) == _region_pixels(rhs_path, region)


def _region_pixels_similar(
    lhs_path: Path,
    rhs_path: Path,
    region: tuple[int, int, int, int],
    *,
    tolerance: int = 8,
    max_mismatched_fraction: float = 0.0,
) -> bool:
    lhs = _region_pixels(lhs_path, region)
    rhs = _region_pixels(rhs_path, region)
    if len(lhs) != len(rhs):
        return False

    mismatched = sum(
        any(abs(left[channel] - right[channel]) > tolerance for channel in range(3))
        for left, right in zip(lhs, rhs)
    )
    return mismatched <= len(lhs) * max_mismatched_fraction


def _compare_images(
    expected_path: Path,
    actual_path: Path,
    diff_path: Path,
    *,
    tolerance: int,
    ignore_alpha: bool,
    max_mismatched_pixels: int,
) -> tuple[bool, str]:
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")
    QColor = _QtGuiModule.QColor
    QImage = _QtGuiModule.QImage

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


def _images_are_pixel_identical(lhs_path: Path, rhs_path: Path) -> bool:
    if _QtGuiModule is None:
        return False

    QImage = _QtGuiModule.QImage
    lhs = QImage(str(lhs_path))
    rhs = QImage(str(rhs_path))

    if lhs.isNull() or rhs.isNull() or lhs.size() != rhs.size():
        return False

    lhs = lhs.convertToFormat(QImage.Format_ARGB32)
    rhs = rhs.convertToFormat(QImage.Format_ARGB32)

    for y in range(lhs.height()):
        for x in range(lhs.width()):
            if lhs.pixel(x, y) != rhs.pixel(x, y):
                return False

    return True


def _images_are_similar(
    lhs_path: Path,
    rhs_path: Path,
    *,
    tolerance: int = 8,
    max_mismatched_fraction: float = 0.0,
) -> bool:
    if _QtGuiModule is None:
        return False

    QImage = _QtGuiModule.QImage
    lhs = QImage(str(lhs_path))
    rhs = QImage(str(rhs_path))
    if lhs.isNull() or rhs.isNull() or lhs.size() != rhs.size():
        return False

    return _region_pixels_similar(
        lhs_path,
        rhs_path,
        (0, 0, lhs.width(), lhs.height()),
        tolerance=tolerance,
        max_mismatched_fraction=max_mismatched_fraction,
    )


def _send_mouse_motion(viewport, position) -> None:
    """Send a real mouse-motion event through the live viewer viewport."""
    app = _QtWidgetsModule.QApplication.instance()
    global_position = viewport.mapToGlobal(position)
    _QtGuiModule.QCursor.setPos(global_position)
    event = _QtGuiModule.QMouseEvent(
        _QtCoreModule.QEvent.MouseMove,
        position,
        global_position,
        _QtCoreModule.Qt.NoButton,
        _QtCoreModule.Qt.NoButton,
        _QtCoreModule.Qt.NoModifier,
    )
    app.sendEvent(viewport, event)
