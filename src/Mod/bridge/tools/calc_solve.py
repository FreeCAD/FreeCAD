import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from typing import Any
from bridge import json_response, mcp


@mcp.tool()
def calc_solve(ctx: Context, equations: Any, unknowns: Any = None) -> list[TextContent]:
    """Solve an equation or a system of equations symbolically.

    Args:
        equations: A single equation string or a list of them. Each may use '='
            (e.g. 'x**2 - 5*x + 6 = 0') or be an expression assumed equal to zero.
        unknowns: Optional symbol name or list of names to solve for. When omitted,
            every free symbol is solved for.
    """
    return json_response(calcinator.solve_equation(equations, unknowns))
