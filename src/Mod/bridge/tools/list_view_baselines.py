from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp, state


@mcp.tool()
def list_view_baselines(ctx: Context) -> list[TextContent]:
    """List the labels currently stored by capture_view_baseline.

    Returns the set of identifiers that compare_views can reference.
    """
    return json_response(
        {
            "count": len(state.view_baselines),
            "labels": sorted(state.view_baselines.keys()),
        }
    )
