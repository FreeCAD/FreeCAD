import base64
import discovery
import json
import urllib.error
import urllib.request
from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from bridge import (
    _get_cortex_url,
    _run_approved_code,
    _validate_word_count,
    get_parashell_connection,
    logger,
    mcp,
    text_response,
)


def _workos_access_token() -> str | None:
    try:
        import auth_core

        return auth_core.valid_access_token()
    except Exception:
        return None


@mcp.tool()
def execute_code(
    ctx: Context, code: str, reason: str, expected_action: str, transaction_id: str
) -> list[TextContent | ImageContent]:
    """Execute arbitrary Python code in Parashell.

    Parashell is a fork of FreeCAD with an identical Python API. Import the API
    under its original module names inside the snippet — use 'import FreeCAD' and
    'import FreeCADGui' exactly as you would in FreeCAD. Object TypeIds (e.g.
    'Part::Box', 'PartDesign::Body') are identical to FreeCAD as well.

    Every model edit must run inside an open transaction. Open one with
    transaction_create, pass its id here, then transaction_apply to persist the
    changes or transaction_cancel to roll them back.

    Args:
        code: The Python code to execute inside Parashell's interpreter.
        reason: A non-technical, straight-to-the-point explanation of why you
            are running this code. Must be between 10 and 25 words.
        expected_action: A non-technical, straight-to-the-point description of
            the expected output and logic. Must be between 5 and 15 words.
        transaction_id: Id of the open transaction returned by transaction_create.
    """
    reason_error = _validate_word_count(reason, "reason", 10, 25)
    if reason_error:
        return text_response(reason_error)
    expected_error = _validate_word_count(expected_action, "expected_action", 5, 15)
    if expected_error:
        return text_response(expected_error)

    encoded = base64.b64encode(code.encode("utf-8")).decode("ascii")
    payload = json.dumps({"c": encoded}).encode("utf-8")
    try:
        cortex_url = _get_cortex_url()
    except Exception as e:
        logger.error(f"Cortex endpoint resolution failed: {e}")
        return text_response("Cortex flagged this code as potentially unsafe")
    headers = {
        "Content-Type": "application/json",
        "User-Agent": discovery.BRIDGE_USER_AGENT,
    }
    token = _workos_access_token()
    if token:
        headers["Authorization"] = "Bearer " + token
    cortex_req = urllib.request.Request(
        cortex_url + "/",
        data=payload,
        headers=headers,
        method="POST",
    )
    try:
        with urllib.request.urlopen(
            cortex_req, timeout=15, context=discovery.verifying_ssl_context()
        ) as resp:
            verdict = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        logger.error(f"Cortex returned HTTP {e.code}: {e.reason}")
        return text_response("Cortex flagged this code as potentially unsafe")
    except Exception as e:
        logger.error(f"Cortex check failed: {e}")
        return text_response("Cortex flagged this code as potentially unsafe")

    if not verdict.get("s"):
        tag = verdict.get("t") or ""
        try:
            decision = get_parashell_connection().request_code_approval(
                code, tag, reason, expected_action
            )
        except Exception as e:
            logger.error(f"Approval request failed: {e}")
            return text_response("Cortex flagged this code as potentially unsafe")
        tag_suffix = f", {tag}" if tag else ""
        if not decision.get("approved"):
            return text_response(
                f"Cortex flagged this and the user acknowledged with DENY{tag_suffix}"
            )
        return _run_approved_code(code, tag, transaction_id)

    return _run_approved_code(code, "", transaction_id)
