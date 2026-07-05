from __future__ import annotations

from rpc import *


@rpc_method
def _capture_ortho_batch(
    self,
    targets: list[tuple[str, str]],
    tile_width: int,
    tile_height: int,
    focus_object: str | None,
    display_mode: str | None,
    hide: list[str] | None,
    show_only: list[str] | None,
    highlight: list[str] | None,
    highlight_color: tuple | None,
    camera_mode: str | None,
) -> dict[str, Any]:
    outcomes: dict[str, Any] = {}
    _engage_viewport_lock()
    try:
        for view_name, tmp_path in targets:
            outcomes[view_name] = self._save_active_screenshot(
                tmp_path,
                view_name,
                tile_width,
                tile_height,
                focus_object,
                display_mode,
                hide,
                show_only,
                highlight,
                highlight_color,
                camera_mode,
            )
    finally:
        _release_viewport_lock()
    return outcomes
