import json
from mcp.server.fastmcp import Context
from mcp.types import TextContent
from bridge import get_parashell_connection, logger, mcp, text_response


@mcp.tool()
def analyze_mass_properties(
    ctx: Context,
    doc_name: str,
    target: str,
    density: float | None = None,
) -> list[TextContent]:
    """Compute mass/inertia properties of a solid for engineering validation.

    Answers the mathematical "does this part have the physical properties I
    expect?" question. For the target shape it reports:
      - volume (mm^3), area (mm^2), length (mm, for wire/edge targets)
      - center_of_mass (x, y, z)
      - matrix_of_inertia: the full 4x4 inertia matrix as nested lists
      - principal_properties: Parashell's PrincipalProperties — symmetry point,
        principal moments of inertia, principal axes (first/second/third),
        radius of gyration, and moments about the center of mass — serialized
        as plain numbers and vectors
      - bbox: full bounding box with center and diagonal

    When density (kg/mm^3 or any consistent unit) is supplied, also returns mass
    (volume * density) and density-scaled principal moments, so you can sanity
    check weight and balance.

    Use to verify a created part is solid (non-zero volume), centered where you
    intended, or symmetric about the expected axis.

    Args:
        doc_name: Document containing the object.
        target: Object name, 'Object#Face3'/'Object#Edge2' subelement reference,
                or a snapshot uid.
        density: Optional material density. When provided, mass and density-scaled
                 moments are included. Keep units consistent with the model
                 (mm-based volume).
    """
    try:
        res = get_parashell_connection().analyze_mass_properties(
            doc_name, target, density
        )
        if not res.get("success"):
            return text_response(
                f"Failed to analyze mass properties: {res.get('error')}"
            )

        volume = res.get("volume")
        mass = res.get("mass")
        com = res.get("center_of_mass") or {}
        com_text = (
            f"({com.get('x', 0):.3f}, {com.get('y', 0):.3f}, {com.get('z', 0):.3f})"
            if com
            else "n/a"
        )
        mass_text = f", mass={mass:.6g}" if mass is not None else ""
        vol_text = f"{volume:.6g}" if isinstance(volume, (int, float)) else "n/a"
        header = (
            f"Mass properties of '{res.get('object')}': volume={vol_text} mm^3, "
            f"center_of_mass={com_text}{mass_text}."
        )
        payload = {k: v for k, v in res.items() if k != "success"}
        return [
            TextContent(type="text", text=header),
            TextContent(
                type="text",
                text=json.dumps(payload, ensure_ascii=False, indent=2, default=str),
            ),
        ]
    except Exception as e:
        logger.error(f"Failed to analyze mass properties: {e}")
        return text_response(f"Failed to analyze mass properties: {e}")
