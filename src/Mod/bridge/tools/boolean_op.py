from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Literal
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    json_response,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def boolean_op(
    ctx: Context,
    doc_name: str,
    operation: Literal["union", "cut", "intersect"],
    operands: list[str],
    result_name: str | None = None,
    delete_operands: bool = False,
    recompute: bool = True,
    verify_growth: bool = True,
) -> list[TextContent | ImageContent]:
    """Run a single boolean operation on N operands in one call.

    Operations:
      - "union" / "fuse":  result = operand[0].fuse(operand[1..N-1])
      - "cut" / "difference":  result = operand[0] minus everything that follows
      - "intersect" / "common":  intersection of all operands

    Replaces the chain of intermediate Part::Fuse / Part::Cut feature objects
    when you only care about the final shape. With delete_operands=True the
    source objects are removed after the boolean, leaving a clean tree.

    Operands accept names or take_snapshot uids.

    Growth verification (verify_growth=True, default) measures the operand and
    result volumes itself instead of trusting the kernel's success flag. It
    rejects results that are physically impossible for the requested operation:
    a union whose volume is smaller than its largest operand (the classic
    silently-dropped-operand / hollow-result failure), a union larger than the
    sum of its operands, an intersection or cut larger than its inputs, or any
    result that collapses to zero volume. When a violation is detected the
    boolean is aborted, no result object is created, and the response reports
    the measured volumes so the failure is caught programmatically rather than
    only on a section cut. Set verify_growth=False to bypass the guard.

    Args:
        doc_name: Document containing the operands.
        operation: "union", "cut", or "intersect".
        operands: List of object names or uids (>= 2). Order matters for cuts.
        result_name: Optional name for the result Part::Feature. Auto-generated
                     when omitted.
        delete_operands: When True, remove the source operands after the boolean.
                         Default False.
        recompute: Whether to recompute after creation. Default True.
        verify_growth: When True (default), assert the result volume is
                       consistent with the operation and abort on violation.
    """
    try:
        parashell = get_parashell_connection()
        res = parashell.boolean_op(
            doc_name,
            operation,
            operands,
            result_name,
            delete_operands,
            recompute,
            verify_growth,
        )
        screenshot = parashell.get_active_screenshot()
        response = json_response({k: v for k, v in res.items() if k != "success"})
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"boolean_op failed: {e}")
        return text_response(f"boolean_op failed: {e}")
