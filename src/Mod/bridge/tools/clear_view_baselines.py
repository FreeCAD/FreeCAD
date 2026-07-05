from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import mcp, state, text_response


@mcp.tool()
def clear_view_baselines(
    ctx: Context,
    label: str | None = None,
) -> list[TextContent]:
    """Remove stored baselines.

    Pass a 'label' to remove a single baseline, or omit to clear all of them.

    Args:
        label: Specific baseline label to remove. None / omitted clears all.
    """
    if label is None:
        n = len(state.view_baselines)
        state.view_baselines.clear()
        return text_response(f"Cleared {n} baseline(s).")
    if label not in state.view_baselines:
        return text_response(f"Baseline '{label}' not found.")
    del state.view_baselines[label]
    return text_response(
        f"Removed baseline '{label}'. Remaining: {len(state.view_baselines)}."
    )
