from mcp.server.fastmcp import Context
from mcp.types import ImageContent, TextContent
from typing import Any
from bridge import (
    add_screenshot_if_available,
    get_parashell_connection,
    logger,
    mcp,
    state,
    text_response,
)


@mcp.tool()
def create_object(
    ctx: Context,
    doc_name: str,
    obj_type: str,
    obj_name: str,
    transaction_id: str,
    analysis_name: str | None = None,
    obj_properties: dict[str, Any] = None,
) -> list[TextContent | ImageContent]:
    """Create a new object in Parashell. Object type starts with 'Part::', 'Draft::', 'PartDesign::', or 'Fem::'.

    Every model edit must run inside an open transaction. Open one with
    transaction_create first, pass its id here, then transaction_apply to persist
    or transaction_cancel to roll back.

    Args:
        doc_name: The name of the document to create the object in.
        obj_type: The type of the object to create (e.g. 'Part::Box', 'Part::Cylinder', 'Draft::Circle').
        obj_name: The name of the object to create.
        transaction_id: Id of the open transaction returned by transaction_create.
        analysis_name: Optional FEM analysis container name to add the object to.
        obj_properties: The properties of the object to create.
    """
    try:
        obj_data = {
            "Name": obj_name,
            "Type": obj_type,
            "Properties": obj_properties or {},
            "Analysis": analysis_name,
        }
        parashell = get_parashell_connection()
        res = parashell.create_object(doc_name, obj_data, transaction_id)
        if res["success"]:
            created_name = res["object_name"]
            if str(obj_type).startswith("Sketcher::SketchObject"):
                screenshot = parashell.get_sketch_view(doc_name, created_name)
            else:
                screenshot = parashell.get_active_screenshot()
            response = text_response(f"Object '{created_name}' created successfully")
        else:
            screenshot = parashell.get_active_screenshot()
            response = text_response(f"Failed to create object: {res['error']}")
        return add_screenshot_if_available(
            response, screenshot, state.only_text_feedback
        )
    except Exception as e:
        logger.error(f"Failed to create object: {str(e)}")
        return text_response(f"Failed to create object: {str(e)}")
