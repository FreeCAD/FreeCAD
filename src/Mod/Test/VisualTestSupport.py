# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Shared GUI fixtures and image comparison for visual tests."""

import os
import sys
import time
import unittest
import warnings
from pathlib import Path

try:
    import FreeCAD  # type: ignore
except Exception as exc:  # pragma: no cover
    FreeCAD = None
    _GUI_IMPORT_ERROR = exc
else:
    _GUI_IMPORT_ERROR = None

try:
    import FreeCADGui  # type: ignore
except Exception as exc:  # pragma: no cover
    FreeCADGui = None
    _GUI_IMPORT_ERROR = _GUI_IMPORT_ERROR or exc

try:
    from pivy import coin  # type: ignore
except Exception as exc:  # pragma: no cover
    coin = None
    _GUI_IMPORT_ERROR = _GUI_IMPORT_ERROR or exc

_FONT_REL = Path("tests") / "visual" / "fonts"
_DEFAULT_FONT_FAMILY = "Noto Sans"
_DEFAULT_FONT_FILES = ("NotoSans-Regular.ttf",)


def _repo_root() -> Path | None:
    here = Path(__file__).resolve()
    for parent in here.parents:
        if (parent / ".git").exists():
            return parent
    return None


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


def _configure_fontconfig(font_dir: Path):
    if not sys.platform.startswith("linux"):
        return

    os.environ["FONTCONFIG_FILE"] = str(font_dir / "fonts.conf")
    os.environ["FONTCONFIG_PATH"] = str(font_dir)


def _register_visual_fonts():
    font_dir = _font_dir()
    font_files = _font_files()
    missing = [p for p in font_files if not p.is_file()]
    if missing:
        raise FileNotFoundError(f"missing font file(s): {', '.join(str(p) for p in missing)}")

    _configure_fontconfig(font_dir)

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


def _skip_headless(message: str, cause: BaseException | None = None):
    warnings.warn(f"{message}; skipping headless test", RuntimeWarning, stacklevel=2)
    if cause is not None:
        raise unittest.SkipTest(message) from cause
    raise unittest.SkipTest(message)


def require_gui():
    if FreeCAD is None or FreeCADGui is None or coin is None:
        message = "Coin node snapshot tests require the FreeCAD GUI and Pivy bindings"
        _skip_headless(message, _GUI_IMPORT_ERROR)

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
            message = "Coin node snapshot tests require a display or headless Qt platform"
            _skip_headless(f"{message} (run under xvfb-run or set QT_QPA_PLATFORM=offscreen)")

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

    # Register vendored fonts so Coin's font backend can discover them.
    try:
        _register_visual_fonts()
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest(f"visual test font setup failed: {exc}") from exc

    return None


class ViewerSnapshotHarness:
    def __init__(self, width: int, height: int):
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
            # Keep viewer-owned 3D annotations out of node-specific snapshots.
            self.view.setAxisCross(False)
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
            FreeCADGui.updateGui()
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

        if self.doc is not None and FreeCAD.getDocument(self.doc.Name):
            document_name = self.doc.Name
            FreeCAD.closeDocument(document_name)
            self._wait_until(
                lambda: document_name not in FreeCAD.listDocuments(),
                f"document {document_name} did not close",
            )
        self.doc = None

        if self.previous_document is not None and FreeCAD.getDocument(self.previous_document.Name):
            FreeCAD.setActiveDocument(self.previous_document.Name)
            FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.previous_document.Name)


def compare_images(
    expected_path: Path,
    actual_path: Path,
    diff_path: Path,
    *,
    tolerance: int,
    ignore_alpha: bool,
    max_mismatched_pixels: int,
) -> tuple[bool, str]:
    from PySide.QtGui import QColor, QImage  # type: ignore

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

    if mismatched > max_mismatched_pixels:
        return (
            False,
            f"mismatched pixels {mismatched} > {max_mismatched_pixels} "
            f"(tolerance={tolerance}, ignore_alpha={ignore_alpha})",
        )

    return True, f"mismatched pixels {mismatched} <= {max_mismatched_pixels}"
