from __future__ import annotations

from rpc import *


@rpc_method
def render_techdraw_page(
    self,
    page_name: str | None = None,
    doc_name: str | None = None,
    width: int | None = None,
    height: int | None = None,
) -> dict[str, Any]:
    fd, svg_path = tempfile.mkstemp(suffix=".svg")
    os.close(fd)
    fd, png_path = tempfile.mkstemp(suffix=".png")
    os.close(fd)

    def render_task():
        return self._render_techdraw_page_gui(
            page_name, doc_name, svg_path, png_path, width, height
        )

    rpc_request_queue.put(render_task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    try:
        if res is True:
            encoded = _read_valid_screenshot(png_path)
            if encoded is None:
                return {
                    "success": False,
                    "error": "The rendered page produced an empty or invalid PNG.",
                }
            return {"success": True, "image": encoded}
        return {"success": False, "error": str(res)}
    finally:
        for path in (svg_path, png_path):
            if os.path.exists(path):
                try:
                    os.remove(path)
                except OSError:
                    pass
