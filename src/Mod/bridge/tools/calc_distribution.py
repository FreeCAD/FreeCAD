import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_distribution(
    ctx: Context,
    name: str,
    operation: str,
    x: Any = None,
    params: list[float] | None = None,
    q: float | None = None,
) -> list[TextContent]:
    """Evaluate a SciPy probability distribution.

    Args:
        name: The scipy.stats distribution name, e.g. 'norm', 'binom', 't'.
        operation: One of pdf, cdf, ppf, stats. pdf/cdf need x, ppf needs q.
        x: The value at which to evaluate pdf or cdf.
        params: Distribution shape, location, and scale parameters in order.
        q: A probability in (0, 1) for ppf (the inverse cdf).
    """
    return json_response(calcinator.distribution(name, operation, x, params, q))
