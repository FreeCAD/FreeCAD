import calcinator
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import json_response, mcp


@mcp.tool()
def calc_statistics(
    ctx: Context,
    operation: str,
    data: list[float],
    data_y: list[float] | None = None,
    confidence: float = 0.95,
    popmean: float = 0.0,
) -> list[TextContent]:
    """Compute statistics, correlation, regression, and hypothesis tests.

    Operations: describe, mean, median, std, variance, sum, min, max,
    correlation, linear_regression, confidence_interval, t_test_1samp,
    t_test_ind. The paired operations (correlation, linear_regression,
    t_test_ind) require data_y.

    Args:
        operation: The operation name from the list above.
        data: The primary numeric sample.
        data_y: The second sample for paired operations.
        confidence: Confidence level in (0, 1) for confidence_interval. Default 0.95.
        popmean: Population mean tested by t_test_1samp. Default 0.0.
    """
    return json_response(
        calcinator.statistics(operation, data, data_y, confidence, popmean)
    )
