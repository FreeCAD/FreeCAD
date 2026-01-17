from __future__ import annotations

from pathlib import Path


def repo_root() -> Path:
    # tools/lint/<this_file>.py -> tools/lint -> tools -> <repo root>
    return Path(__file__).resolve().parents[2]


DEFAULT_CLANG_STYLE = "file"

# Mirrors .github/workflows/sub_lint.yml defaults (keep in one place).
DEFAULT_CLAZY_CHECKS = "level2,no-non-pod-global-static,no-copyable-polymorphic"
DEFAULT_CLAZY_QT6_CHECKS = (
    "qt6-deprecated-api-fixes,qt6-header-fixes,qt6-qhash-signature,"
    "qt6-fwd-fixes,missing-qobject-macro"
)

DEFAULT_CODESPELL_IGNORE_WORDS_REL = ".github/codespellignore"
DEFAULT_CODESPELL_SKIP = (
    "./.git*,*.po,*.ts,*.svg,./src/3rdParty,./src/Base/swig*,./src/Mod/Robot/App/kdl_cp,"
    "./src/Mod/Import/App/SCL*,./src/Doc/FreeCAD.uml,./build/,./.devstack/"
)

