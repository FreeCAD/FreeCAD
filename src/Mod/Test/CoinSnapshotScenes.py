# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Scene construction and fixture metadata for Coin visual tests."""

import importlib
import importlib.util
import os
import unittest
from dataclasses import dataclass
from enum import Enum, auto
from typing import Callable

from CoinSnapshotHarness import _snapshot_dimensions

_RENDERER_LEGACY = "legacy"
_RENDERER_DRAW_LIST = "draw_list"
ALL_RENDERERS = frozenset({_RENDERER_LEGACY, _RENDERER_DRAW_LIST})
LEGACY_ONLY = frozenset({_RENDERER_LEGACY})

_DEFAULT_FONT_FAMILY = "Noto Sans"
_DEFAULT_FONT_SIZE = 18
_TRANSLUCENCY_BACKGROUND_TOP = (0.20, 0.20, 0.60)
_TRANSLUCENCY_BACKGROUND_BOTTOM = (0.90, 0.90, 1.00)
_SO_FC_BACKGROUND_GRADIENT_LINEAR = 0
_SO_DATUM_LABEL_TYPES = {
    "ANGLE": 0,
    "DISTANCE": 1,
    "DISTANCEX": 2,
    "DISTANCEY": 3,
    "RADIUS": 4,
    "DIAMETER": 5,
    "SYMMETRIC": 6,
    "ARCLENGTH": 7,
}

_NAVI_CUBE_NODE_TYPES = (
    "SoNaviCube",
    "SoNaviCubeTranslucent",
    "SoNaviCubeHiliteFront",
)
_BREP_EDGE_SET_NODE_TYPES = (
    "SoBrepEdgeSet",
    "SoBrepEdgeSetHighlight",
    "SoBrepEdgeSetSelection",
)
_BREP_POINT_SET_NODE_TYPES = (
    "SoBrepPointSet",
    "SoBrepPointSetHighlight",
    "SoBrepPointSetSelection",
)
_BREP_FACE_SET_NODE_TYPES = (
    "SoBrepFaceSet",
    "SoBrepFaceSetHighlight",
    "SoBrepFaceSetSelection",
)
_POLYGON_NODE_TYPES = (
    "SoPolygon",
    "SoPolygonOpen",
    "SoPolygonStartIndex",
    "SoPolygonNonPlanar",
    "SoPolygonTriangle",
)
_INDEXED_FACE_SET_NODE_TYPES = (
    "SoFCIndexedFaceSet",
    "SoFCIndexedFaceSetPerFaceColor",
    "SoFCIndexedFaceSetPerVertexColor",
    "SoFCIndexedFaceSetTranslucent",
)


class CameraPolicy(Enum):
    VIEW_ALL = auto()
    FIXED_OVERLAY = auto()
    VIEW_ALL_WITH_MARGIN = auto()


@dataclass(frozen=True)
class SnapshotRuntime:
    coin: object
    width: int
    height: int
    type_name: str


@dataclass(frozen=True)
class SnapshotFixture:
    builder: Callable
    framing_policy: CameraPolicy = CameraPolicy.VIEW_ALL
    required_modules: tuple[str, ...] = ()
    supported_renderers: frozenset[str] = ALL_RENDERERS

    def build(self, runtime: SnapshotRuntime):
        for module_name in self.required_modules:
            _require_module(module_name, runtime.type_name)
        return self.builder(
            runtime.coin,
            runtime.type_name,
            width=runtime.width,
            height=runtime.height,
        )


def _require_module(module_name: str, node_label: str):
    if importlib.util.find_spec(module_name) is None:
        raise unittest.SkipTest(f"{node_label} requires {module_name}")
    importlib.import_module(module_name)


def _instantiate(coin, type_name: str):
    t = coin.SoType.fromName(type_name)
    if t.isBad():
        raise unittest.SkipTest(f"Coin type not registered: {type_name}")
    node = t.createInstance()
    if node is None:
        raise RuntimeError(f"Failed to instantiate {type_name}")
    return node


def _coin_field(node, field_name: str):
    field = getattr(node, field_name, None)
    if field is not None:
        return field

    get_field = getattr(node, "getField", None)
    if get_field is None:
        return None

    try:
        return get_field(field_name)
    except Exception:
        return None


def _require_coin_field(node, type_name: str, field_name: str):
    field = _coin_field(node, field_name)
    if field is None:
        raise AssertionError(f"{type_name} field '{field_name}' is unavailable")
    return field


def _configure_fc_background_gradient(
    grad,
    top_color,
    bottom_color,
    *,
    mid_color=None,
    gradient_mode=_SO_FC_BACKGROUND_GRADIENT_LINEAR,
):
    type_name = "SoFCBackgroundGradient"
    _require_coin_field(grad, type_name, "gradientMode").setValue(gradient_mode)
    _require_coin_field(grad, type_name, "fromColor").setValue(top_color)
    _require_coin_field(grad, type_name, "toColor").setValue(bottom_color)

    if mid_color is None:
        _require_coin_field(grad, type_name, "useMidColor").setValue(False)
    else:
        _require_coin_field(grad, type_name, "midColor").setValue(mid_color)
        _require_coin_field(grad, type_name, "useMidColor").setValue(True)


def _add_translucency_backplate(root, coin):
    """Add two opaque panels behind the translucent face-set fixture."""
    backplate = coin.SoSeparator()

    light_model = coin.SoLightModel()
    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
    backplate.addChild(light_model)

    coords = coin.SoCoordinate3()
    # fmt: off
    coords.point.setValues(0, 8, [
            coin.SbVec3f(-0.95, -0.85, -0.05),
            coin.SbVec3f(0.0, -0.85, -0.05),
            coin.SbVec3f(0.0, 0.85, -0.05),
            coin.SbVec3f(-0.95, 0.85, -0.05),
            coin.SbVec3f(0.0, -0.85, -0.05),
            coin.SbVec3f(0.95, -0.85, -0.05),
            coin.SbVec3f(0.95, 0.85, -0.05),
            coin.SbVec3f(0.0, 0.85, -0.05),
    ])
    material = coin.SoMaterial()
    material.diffuseColor.setValues(
        0, 2, [coin.SbColor(0.10, 0.25, 0.90), coin.SbColor(1.00, 0.75, 0.05)]
    )
    # fmt: on
    binding = coin.SoMaterialBinding()
    binding.value = coin.SoMaterialBinding.PER_FACE
    faces = coin.SoIndexedFaceSet()
    faces.coordIndex.setValues(0, 10, [0, 1, 2, 3, -1, 4, 5, 6, 7, -1])

    backplate.addChild(coords)
    backplate.addChild(material)
    backplate.addChild(binding)
    backplate.addChild(faces)
    root.addChild(backplate)


def _make_default_snapshot_scene(coin):
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    # Isometric-ish.
    cam.orientation.setValue(coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553))
    root.addChild(cam)

    root.addChild(coin.SoDirectionalLight())

    # Make text rendering less dependent on the host: set an explicit font in the Coin scene.
    font = coin.SoFont()
    font.name.setValue(_DEFAULT_FONT_FAMILY)
    font.size.setValue(float(_DEFAULT_FONT_SIZE))
    root.addChild(font)
    return root, cam


def _configure_overlay_camera(
    coin, cam, *, position_z: float, near: float, far: float, height: float
):
    cam.orientation.setValue(coin.SbRotation())
    cam.position.setValue(0.0, 0.0, position_z)
    cam.nearDistance.setValue(near)
    cam.farDistance.setValue(far)
    cam.height.setValue(height)


def _effective_snapshot_dimensions(
    width: int | None = None, height: int | None = None
) -> tuple[int, int]:
    default_width, default_height = _snapshot_dimensions()
    return (
        width if width is not None else default_width,
        height if height is not None else default_height,
    )


def _build_drawing_grid_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=2.0, near=1.0, far=5.0, height=2.0)
    root.addChild(_instantiate(coin, "SoDrawingGrid"))
    return root


def _build_reg_point_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=2.0, near=1.0, far=5.0, height=2.0)
    probe = _instantiate(coin, "SoRegPoint")
    probe.base.setValue(0.0, 0.0, 0.0)
    probe.normal.setValue(0.6, 0.7, 0.4)
    probe.length.setValue(1.2)
    probe.color.setValue(1.0, 0.45, 0.34)
    probe.text.setValue("SoRegPoint")
    root.addChild(probe)
    return root


def _build_datum_label_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=2.0, near=1.0, far=5.0, height=2.0)
    label = _instantiate(coin, "SoDatumLabel")
    label.string.setValue("SoDatumLabel")
    label.textColor.setValue(1.0, 0.45, 0.34)
    label.name.setValue(_DEFAULT_FONT_FAMILY)
    label.size.setValue(18)
    label.lineWidth.setValue(2.0)
    label.sampling.setValue(2.0)
    # `createInstance()` returns a generic node proxy, so use a local enum map
    # instead of relying on typed Python constants.
    label.datumtype.setValue(_SO_DATUM_LABEL_TYPES["DISTANCE"])
    label.param1.setValue(0.25)
    label.param2.setValue(0.0)
    # fmt: off
    label.pnts.setValues(
        0, 2, [coin.SbVec3f(-0.5, -0.1, 0.0), coin.SbVec3f(0.5, 0.2, 0.0)]
    )
    # fmt: on
    root.addChild(label)
    return root


def _build_string_label_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    label = _instantiate(coin, "SoStringLabel")
    label.string.setValue("SoStringLabel")
    label.name.setValue(_DEFAULT_FONT_FAMILY)
    label.size.setValue(28)
    # Default SoStringLabel textColor is white, which can become invisible on the white
    # snapshot background depending on the GL blending / alpha handling.
    label.textColor.setValue(0.05, 0.05, 0.05)
    root.addChild(label)
    return root


def _build_frame_label_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=2.0, near=1.0, far=5.0, height=2.0)
    label = _instantiate(coin, "SoFrameLabel")
    label.string.setValue("Frame Label")
    label.textColor.setValue(1.0, 1.0, 1.0)
    label.backgroundColor.setValue(0.10, 0.35, 0.85)
    label.name.setValue(_DEFAULT_FONT_FAMILY)
    label.size.setValue(20)
    label.justification.setValue(2)  # SoFrameLabel::CENTER
    label.frame.setValue(True)
    label.border.setValue(True)
    root.addChild(label)
    return root


def _build_background_gradient_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=1.0, near=0.5, far=1.5, height=2.0)

    grad = _instantiate(coin, "SoFCBackgroundGradient")
    _configure_fc_background_gradient(
        grad,
        coin.SbColor(0.2, 0.2, 0.6),
        coin.SbColor(0.9, 0.9, 1.0),
    )
    root.addChild(grad)
    return root


def _find_descendant(coin, root, type_name: str):
    type_id = coin.SoType.fromName(type_name)
    if type_id.isBad():
        raise unittest.SkipTest(f"Coin type not registered: {type_name}")

    search = coin.SoSearchAction()
    search.setType(type_id)
    search.setSearchingAll(False)
    search.apply(root)
    path = search.getPath()
    return None if path is None else path.getTail()


def _configure_preview_geometry(coin, preview) -> None:
    """Populate SoPreviewShape's retained SoFCShape geometry for a test scene."""
    coords = _find_descendant(coin, preview, "SoCoordinate3")
    normals = _find_descendant(coin, preview, "SoNormal")
    faces = _find_descendant(coin, preview, "SoBrepFaceSet")
    lines = _find_descendant(coin, preview, "SoBrepEdgeSet")
    if any(node is None for node in (coords, normals, faces, lines)):
        raise AssertionError("SoPreviewShape retained geometry is incomplete")

    # fmt: off
    coords.point.setValues(0, 8, [
            coin.SbVec3f(-0.65, -0.55, -0.45),
            coin.SbVec3f(0.65, -0.55, -0.45),
            coin.SbVec3f(0.65, 0.55, -0.45),
            coin.SbVec3f(-0.65, 0.55, -0.45),
            coin.SbVec3f(-0.65, -0.55, 0.45),
            coin.SbVec3f(0.65, -0.55, 0.45),
            coin.SbVec3f(0.65, 0.55, 0.45),
            coin.SbVec3f(-0.65, 0.55, 0.45),
    ])
    normals.vector.setValues(0, 8, [
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
    ])
    # fmt: on
    # fmt: off
    faces.coordIndex.setValues(0, 48, [
        0, 1, 2, -1,  0, 2, 3, -1,
        4, 6, 5, -1,  4, 7, 6, -1,
        0, 4, 5, -1,  0, 5, 1, -1,
        1, 5, 6, -1,  1, 6, 2, -1,
        2, 6, 7, -1,  2, 7, 3, -1,
        3, 7, 4, -1,  3, 4, 0, -1,
    ])
    # fmt: on
    faces.partIndex.setValues(0, 6, [2, 2, 2, 2, 2, 2])
    # fmt: off
    lines.coordIndex.setValues(0, 36, [
        0, 1, -1,  1, 2, -1,  2, 3, -1,  3, 0, -1,
        4, 5, -1,  5, 6, -1,  6, 7, -1,  7, 4, -1,
        0, 4, -1,  1, 5, -1,  2, 6, -1,  3, 7, -1,
    ])
    # fmt: on
    lines.materialIndex.setValues(0, 12, [0] * 12)

    preview.color.setValue(0.78, 0.12, 0.92)
    preview.transparency.setValue(0.45)
    preview.lineWidth.setValue(3.0)
    matrix = coin.SbMatrix()
    matrix.setTranslate(coin.SbVec3f(0.25, 0.1, 0.15))
    preview.transform.setValue(matrix)


def _build_bounding_box_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    color = coin.SoBaseColor()
    color.rgb.setValue(0.08, 0.12, 0.22)
    style = coin.SoDrawStyle()
    style.lineWidth.setValue(2.0)
    box = _instantiate(coin, "SoFCBoundingBox")
    box.minBounds.setValue(-1.5, -1.1, -0.75)
    box.maxBounds.setValue(1.5, 1.1, 0.75)
    box.coordsOn.setValue(True)
    box.dimensionsOn.setValue(True)
    box.skipBoundingBox.setValue(False)
    root.addChild(color)
    root.addChild(style)
    root.addChild(box)
    return root


def _build_preview_shape_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    preview = _instantiate(coin, "SoPreviewShape")
    root.addChild(preview)
    _configure_preview_geometry(coin, preview)
    return root


def _build_color_bar_label_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=5.0, near=0.1, far=100.0, height=10.0)
    font = _find_descendant(coin, root, "SoFont")
    font.size.setValue(28.0)
    color = coin.SoBaseColor()
    color.rgb.setValue(0.05, 0.05, 0.05)
    label = _instantiate(coin, "SoColorBarLabel")
    label.string.setValues(0, 2, ["Low", "High"])
    root.addChild(color)
    root.addChild(label)
    return root


def _build_color_bar_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, cam = _make_default_snapshot_scene(coin)
    _configure_overlay_camera(coin, cam, position_z=5.0, near=0.1, far=100.0, height=10.0)
    grad = _instantiate(coin, "SoFCBackgroundGradient")
    _configure_fc_background_gradient(
        grad,
        coin.SbColor(0.07, 0.09, 0.15),
        coin.SbColor(0.16, 0.19, 0.28),
    )
    root.addChild(grad)
    root.addChild(_instantiate(coin, "SoFCColorBar"))
    return root


def _build_3d_annotation_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    light_model = coin.SoLightModel()
    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
    root.addChild(light_model)

    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.12, 0.30, 0.72)
    transform = coin.SoTransform()
    transform.scaleFactor.setValue(1.4, 1.0, 0.18)
    plate = coin.SoCube()
    plate_group = coin.SoSeparator()
    plate_group.addChild(material)
    plate_group.addChild(transform)
    plate_group.addChild(plate)
    root.addChild(plate_group)

    annotation = _instantiate(coin, "So3DAnnotation")
    line_material = coin.SoMaterial()
    line_material.diffuseColor.setValue(0.95, 0.16, 0.05)
    line_style = coin.SoDrawStyle()
    line_style.style.setValue(1)
    line_style.lineWidth.setValue(3.0)
    line_transform = coin.SoTransform()
    line_transform.translation.setValue(0.0, 0.0, -0.55)
    line_transform.scaleFactor.setValue(0.95, 0.72, 0.55)
    line_group = coin.SoSeparator()
    line_group.addChild(line_material)
    line_group.addChild(line_style)
    line_group.addChild(line_transform)
    line_group.addChild(coin.SoCube())
    annotation.addChild(line_group)
    root.addChild(annotation)
    return root


def _build_placement_indicator_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    indicator = _instantiate(coin, "SoFCPlacementIndicatorKit")
    indicator.parts.setValue(31)
    indicator.axes.setValue(7)
    indicator.axisLength.setValue(0.8)
    indicator.scaleFactor.setValue(70.0)
    indicator.axisLabels.setValues(0, 3, ["X", "Y", "Z"])
    root.addChild(indicator)
    return root


def _build_axis_cross_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    root.addChild(_instantiate(coin, "SoAxisCrossKit"))
    return root


def _build_navi_cube_scene(coin, type_name: str, *, width=None, height=None):
    root, cam = _make_default_snapshot_scene(coin)
    if type_name == "SoNaviCubeTranslucent":
        # Provide visible background so translucency can be verified.
        grad = _instantiate(coin, "SoFCBackgroundGradient")
        top = coin.SbColor(0.15, 0.15, 0.20)
        bottom = coin.SbColor(0.45, 0.45, 0.55)
        _configure_fc_background_gradient(grad, top, bottom)
        root.addChild(grad)

    cube = _instantiate(coin, "SoNaviCube")
    cube.size.setValue(1.0)
    cube.opacity.setValue(0.55 if type_name == "SoNaviCubeTranslucent" else 1.0)
    cube.borderWidth.setValue(0.02)
    cube.showCoordinateSystem.setValue(True)
    cube.cameraIsOrthographic.setValue(True)
    if type_name == "SoNaviCubeHiliteFront":
        # Gui::SoNaviCube::PickId::Front (see `src/Gui/Inventor/SoNaviCube.h`).
        cube.hiliteId.setValue(1)

    scene_width, scene_height = _effective_snapshot_dimensions(width, height)
    overlay = max(64.0, min(float(scene_width), float(scene_height)) * 0.60)
    margin = 8.0

    # Render like a real overlay: small square in the corner.
    cube.viewportRect.setValue(
        float(scene_width) - overlay - margin,
        float(scene_height) - overlay - margin,
        overlay,
        overlay,
    )

    # Mimic what the controller does: orient the cube from the viewer camera.
    cube.cameraOrientation.setValue(cam.orientation.getValue())
    root.addChild(cube)
    return root


def _polygon_points(coin, type_name: str):
    if type_name == "SoPolygonStartIndex":
        # Use a non-zero startIndex and a non-trivial vertex slice.
        return [
            coin.SbVec3f(-0.9, -0.9, 0.0),  # unused (startIndex=1)
            coin.SbVec3f(-0.6, -0.4, 0.0),
            coin.SbVec3f(0.7, -0.6, 0.0),
            coin.SbVec3f(0.6, 0.6, 0.0),
            coin.SbVec3f(-0.4, 0.7, 0.0),
            coin.SbVec3f(0.9, 0.9, 0.0),  # unused
        ]
    if type_name == "SoPolygonTriangle":
        return [
            coin.SbVec3f(-0.7, -0.5, 0.0),
            coin.SbVec3f(0.8, -0.6, 0.0),
            coin.SbVec3f(-0.1, 0.8, 0.0),
            coin.SbVec3f(0.9, 0.9, 0.0),  # unused
            coin.SbVec3f(-0.9, 0.9, 0.0),  # unused
        ]
    if type_name == "SoPolygonNonPlanar":
        return [
            coin.SbVec3f(-0.7, -0.7, 0.0),
            coin.SbVec3f(0.7, -0.7, 0.4),
            coin.SbVec3f(0.8, 0.6, -0.2),
            coin.SbVec3f(-0.6, 0.8, 0.3),
            coin.SbVec3f(-0.7, -0.7, 0.0),
        ]
    if type_name == "SoPolygonOpen":
        return [
            coin.SbVec3f(-0.8, -0.6, 0.0),
            coin.SbVec3f(0.6, -0.7, 0.0),
            coin.SbVec3f(0.8, 0.4, 0.0),
            coin.SbVec3f(-0.4, 0.7, 0.0),
        ]

    # Closed loop (last point repeats).
    return [
        coin.SbVec3f(-0.7, -0.7, 0.0),
        coin.SbVec3f(0.7, -0.7, 0.0),
        coin.SbVec3f(0.7, 0.7, 0.0),
        coin.SbVec3f(-0.7, 0.7, 0.0),
        coin.SbVec3f(-0.7, -0.7, 0.0),
    ]


def _polygon_color(type_name: str) -> tuple[float, float, float]:
    return {
        "SoPolygonOpen": (0.80, 0.20, 0.10),
        "SoPolygonStartIndex": (0.10, 0.60, 0.20),
        "SoPolygonTriangle": (0.95, 0.55, 0.10),
        "SoPolygonNonPlanar": (0.65, 0.20, 0.75),
    }.get(type_name, (0.10, 0.25, 0.80))


def _build_polygon_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    points = _polygon_points(coin, type_name)
    coords = coin.SoCoordinate3()
    coords.point.setValues(0, len(points), points)

    material = coin.SoMaterial()
    material.diffuseColor.setValue(*_polygon_color(type_name))

    poly = _instantiate(coin, "SoPolygon")
    if type_name == "SoPolygonStartIndex":
        poly.startIndex.setValue(1)
        poly.numVertices.setValue(4)
    elif type_name == "SoPolygonTriangle":
        poly.startIndex.setValue(0)
        poly.numVertices.setValue(3)
    else:
        poly.startIndex.setValue(0)
        poly.numVertices.setValue(coords.point.getNum())
    poly.getField("render").setValue(True)

    root.addChild(coords)
    root.addChild(material)
    root.addChild(poly)
    return root


def _build_indexed_face_set_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    # SoFCIndexedFaceSet test geometry does not provide normals. Without normals, lighting
    # contributes only ambient, which makes all material variants appear nearly black.
    # Provide one normal per face (triangle) so PER_FACE/PER_VERTEX material bindings
    # visibly differ under the default directional light.
    normals = coin.SoNormal()
    # fmt: off
    normals.vector.setValues(0, 12, [
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
    ])
    # fmt: on
    normal_bind = coin.SoNormalBinding()
    normal_bind.value = coin.SoNormalBinding.PER_FACE
    root.addChild(normals)
    root.addChild(normal_bind)

    if type_name == "SoFCIndexedFaceSetTranslucent":
        # A single quad makes the material alpha directly observable. A closed
        # cube blends its front and back faces and appears nearly opaque.
        grad = _instantiate(coin, "SoFCBackgroundGradient")
        _configure_fc_background_gradient(
            grad,
            coin.SbColor(*_TRANSLUCENCY_BACKGROUND_TOP),
            coin.SbColor(*_TRANSLUCENCY_BACKGROUND_BOTTOM),
        )
        root.addChild(grad)
        _add_translucency_backplate(root, coin)

    coords = coin.SoCoordinate3()
    faces = _instantiate(coin, "SoFCIndexedFaceSet")
    if type_name == "SoFCIndexedFaceSetTranslucent":
        # fmt: off
        coords.point.setValues(0, 4, [
                coin.SbVec3f(-0.7, -0.7, 0.0),
                coin.SbVec3f(0.7, -0.7, 0.0),
                coin.SbVec3f(0.7, 0.7, 0.0),
                coin.SbVec3f(-0.7, 0.7, 0.0),
        ])
        faces.coordIndex.setValues(0, 8, [0, 1, 2, -1, 0, 2, 3, -1])
    else:
        # fmt: off
        coords.point.setValues(0, 8, [
                coin.SbVec3f(-0.6, -0.6, -0.6),
                coin.SbVec3f(0.6, -0.6, -0.6),
                coin.SbVec3f(0.6, 0.6, -0.6),
                coin.SbVec3f(-0.6, 0.6, -0.6),
                coin.SbVec3f(-0.6, -0.6, 0.6),
                coin.SbVec3f(0.6, -0.6, 0.6),
                coin.SbVec3f(0.6, 0.6, 0.6),
                coin.SbVec3f(-0.6, 0.6, 0.6),
        ])
        # fmt: on
        # Six faces, two triangles per face.
        # fmt: off
        faces.coordIndex.setValues(0, 48, [
            0, 1, 2, -1,  0, 2, 3, -1,  # bottom (-Z)
            4, 6, 5, -1,  4, 7, 6, -1,  # top (+Z)
            0, 4, 5, -1,  0, 5, 1, -1,  # -Y
            1, 5, 6, -1,  1, 6, 2, -1,  # +X
            2, 6, 7, -1,  2, 7, 3, -1,  # +Y
            3, 7, 4, -1,  3, 4, 0, -1,  # -X
        ])
        # fmt: on
    material = coin.SoMaterial()
    if type_name == "SoFCIndexedFaceSetPerFaceColor":
        # 12 faces (triangles).
        # fmt: off
        material.diffuseColor.setValues(0, 12, [
                coin.SbColor(0.90, 0.25, 0.25),
                coin.SbColor(0.90, 0.55, 0.25),
                coin.SbColor(0.90, 0.80, 0.25),
                coin.SbColor(0.65, 0.90, 0.25),
                coin.SbColor(0.25, 0.90, 0.25),
                coin.SbColor(0.25, 0.90, 0.65),
                coin.SbColor(0.25, 0.90, 0.90),
                coin.SbColor(0.25, 0.65, 0.90),
                coin.SbColor(0.25, 0.25, 0.90),
                coin.SbColor(0.65, 0.25, 0.90),
                coin.SbColor(0.90, 0.25, 0.90),
                coin.SbColor(0.90, 0.25, 0.65),
        ])
        # fmt: on
        bind = coin.SoMaterialBinding()
        bind.value = coin.SoMaterialBinding.PER_FACE
        root.addChild(bind)
    elif type_name == "SoFCIndexedFaceSetPerVertexColor":
        # fmt: off
        material.diffuseColor.setValues(0, 8, [
                coin.SbColor(0.95, 0.25, 0.25),
                coin.SbColor(0.95, 0.75, 0.25),
                coin.SbColor(0.25, 0.95, 0.25),
                coin.SbColor(0.25, 0.95, 0.95),
                coin.SbColor(0.25, 0.25, 0.95),
                coin.SbColor(0.95, 0.25, 0.95),
                coin.SbColor(0.70, 0.70, 0.70),
                coin.SbColor(0.15, 0.15, 0.15),
        ])
        # fmt: on
        bind = coin.SoMaterialBinding()
        # IndexedFaceSet colors are bound by index; use coordIndex as the color index
        # so each corner uses the corresponding material color.
        bind.value = coin.SoMaterialBinding.PER_VERTEX_INDEXED
        coord_index_values = list(faces.coordIndex.getValues(0))
        # fmt: off
        faces.materialIndex.setValues(0, faces.coordIndex.getNum(), coord_index_values)
        # fmt: on
        root.addChild(bind)
    else:
        material.diffuseColor.setValue(0.70, 0.70, 0.75)
        if type_name == "SoFCIndexedFaceSetTranslucent":
            material.transparency.setValue(0.55)
            light_model = coin.SoLightModel()
            light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
            root.addChild(light_model)

    root.addChild(coords)
    root.addChild(material)
    root.addChild(faces)
    return root


def _build_lighting_equivalence_scene(coin):
    """Build a lit cube whose directional light can be changed after traversal."""
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    cam.orientation.setValue(coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553))
    root.addChild(cam)

    light = coin.SoDirectionalLight()
    light.direction.setValue(coin.SbVec3f(0.25, -0.35, -1.0))
    root.addChild(light)

    coords = coin.SoCoordinate3()
    # fmt: off
    coords.point.setValues(0, 8, [
            coin.SbVec3f(-0.6, -0.6, -0.6),
            coin.SbVec3f(0.6, -0.6, -0.6),
            coin.SbVec3f(0.6, 0.6, -0.6),
            coin.SbVec3f(-0.6, 0.6, -0.6),
            coin.SbVec3f(-0.6, -0.6, 0.6),
            coin.SbVec3f(0.6, -0.6, 0.6),
            coin.SbVec3f(0.6, 0.6, 0.6),
            coin.SbVec3f(-0.6, 0.6, 0.6),
    ])
    root.addChild(coords)

    normals = coin.SoNormal()
    normals.vector.setValues(0, 12, [
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, -1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, 0.0, 1.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(0.0, -1.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(1.0, 0.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
            coin.SbVec3f(-1.0, 0.0, 0.0),
    ])
    # fmt: on
    root.addChild(normals)

    normal_binding = coin.SoNormalBinding()
    normal_binding.value = coin.SoNormalBinding.PER_FACE
    root.addChild(normal_binding)

    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.72, 0.76, 0.84)
    root.addChild(material)

    faces = coin.SoIndexedFaceSet()
    # fmt: off
    faces.coordIndex.setValues(0, 48, [
        0, 1, 2, -1,  0, 2, 3, -1,
        4, 6, 5, -1,  4, 7, 6, -1,
        0, 4, 5, -1,  0, 5, 1, -1,
        1, 5, 6, -1,  1, 6, 2, -1,
        2, 6, 7, -1,  2, 7, 3, -1,
        3, 7, 4, -1,  3, 4, 0, -1,
    ])
    # fmt: on
    root.addChild(faces)
    return root, light


def _build_brep_edge_set_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    coords = coin.SoCoordinate3()
    # fmt: off
    coords.point.setValues(0, 5, [
            coin.SbVec3f(-0.6, -0.6, 0.0),
            coin.SbVec3f(0.6, -0.6, 0.0),
            coin.SbVec3f(0.6, 0.6, 0.0),
            coin.SbVec3f(-0.6, 0.6, 0.0),
            coin.SbVec3f(0.0, 0.0, 0.0),
    ])
    style = coin.SoDrawStyle()
    style.lineWidth.setValue(3.0)
    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.05, 0.05, 0.05)

    edges = _instantiate(coin, "SoBrepEdgeSet")
    # Square + diagonals.
    edges.coordIndex.setValues(0, 15, [0, 1, 2, 3, 0, -1, 0, 2, -1, 1, 3, -1, 4, 0, -1])
    # fmt: on
    if type_name == "SoBrepEdgeSetHighlight":
        edges.highlightEdgeIndex.setValues(0, 1, [1])
        edges.highlightColor.setValue(1.0, 0.0, 0.0)
    elif type_name == "SoBrepEdgeSetSelection":
        edges.selectionEdgeIndex.setValues(0, 1, [2])
        edges.selectionColor.setValue(0.0, 0.6, 0.0)

    root.addChild(coords)
    root.addChild(style)
    root.addChild(material)
    root.addChild(edges)
    return root


def _build_brep_point_set_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    coords = coin.SoCoordinate3()
    # fmt: off
    coords.point.setValues(0, 9, [
            coin.SbVec3f(-0.6, -0.6, 0.0),
            coin.SbVec3f(0.0, -0.6, 0.0),
            coin.SbVec3f(0.6, -0.6, 0.0),
            coin.SbVec3f(-0.6, 0.0, 0.0),
            coin.SbVec3f(0.0, 0.0, 0.0),
            coin.SbVec3f(0.6, 0.0, 0.0),
            coin.SbVec3f(-0.6, 0.6, 0.0),
            coin.SbVec3f(0.0, 0.6, 0.0),
            coin.SbVec3f(0.6, 0.6, 0.0),
    ])
    # fmt: on
    style = coin.SoDrawStyle()
    style.pointSize.setValue(7.0)
    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.05, 0.05, 0.05)

    pts = _instantiate(coin, "SoBrepPointSet")
    pts.startIndex.setValue(0)
    pts.numPoints.setValue(-1)
    if type_name == "SoBrepPointSetHighlight":
        pts.highlightCoordIndex.setValues(0, 1, [4])
        pts.highlightColor.setValue(1.0, 0.0, 0.0)
    elif type_name == "SoBrepPointSetSelection":
        pts.selectionCoordIndex.setValues(0, 4, [0, 2, 6, 8])
        pts.selectionColor.setValue(0.0, 0.6, 0.0)

    root.addChild(coords)
    root.addChild(style)
    root.addChild(material)
    root.addChild(pts)
    return root


def _build_brep_face_set_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    # Simple cube: 6 parts (faces), 2 triangles each.
    coords = coin.SoCoordinate3()
    # fmt: off
    coords.point.setValues(0, 8, [
            coin.SbVec3f(-0.5, -0.5, -0.5),  # 0
            coin.SbVec3f(0.5, -0.5, -0.5),  # 1
            coin.SbVec3f(0.5, 0.5, -0.5),  # 2
            coin.SbVec3f(-0.5, 0.5, -0.5),  # 3
            coin.SbVec3f(-0.5, -0.5, 0.5),  # 4
            coin.SbVec3f(0.5, -0.5, 0.5),  # 5
            coin.SbVec3f(0.5, 0.5, 0.5),  # 6
            coin.SbVec3f(-0.5, 0.5, 0.5),  # 7
    ])
    # fmt: on

    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.75, 0.75, 0.78)

    light_model = coin.SoLightModel()
    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)

    faces = _instantiate(coin, "SoBrepFaceSet")
    # Each triangle ends with -1; partIndex counts triangles per part.
    # fmt: off
    faces.coordIndex.setValues(0, 48, [
        # Bottom (z=-0.5)
        0, 1, 2, -1,  0, 2, 3, -1,
        # Top (z=+0.5)
        4, 6, 5, -1,  4, 7, 6, -1,
        # Front (y=-0.5)
        0, 5, 1, -1,  0, 4, 5, -1,
        # Back (y=+0.5)
        3, 2, 6, -1,  3, 6, 7, -1,
        # Left (x=-0.5)
        0, 3, 7, -1,  0, 7, 4, -1,
        # Right (x=+0.5)
        1, 5, 6, -1,  1, 6, 2, -1,
    ])
    # fmt: on
    faces.partIndex.setValues(0, 6, [2, 2, 2, 2, 2, 2])

    if type_name == "SoBrepFaceSetHighlight":
        # Highlight the front face (part 2).
        faces.highlightPartIndex.setValues(0, 1, [2])
        faces.highlightColor.setValue(1.0, 0.0, 0.0)
    elif type_name == "SoBrepFaceSetSelection":
        # Select a couple of faces to exercise the selection overlay.
        faces.selectionPartIndex.setValues(0, 2, [1, 5])
        faces.selectionColor.setValue(0.0, 0.6, 0.0)

    root.addChild(coords)
    root.addChild(light_model)
    root.addChild(material)
    root.addChild(faces)
    return root


def _build_control_points_scene(coin, _type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)

    # A small 3x3 pole grid with 2x2 knots appended.
    coords = coin.SoCoordinate3()
    pts = []
    for u in (-0.6, 0.0, 0.6):
        for v in (-0.6, 0.0, 0.6):
            pts.append(coin.SbVec3f(u, v, 0.0))
    for u in (-0.3, 0.3):
        for v in (-0.3, 0.3):
            pts.append(coin.SbVec3f(u, v, 0.15))
    coords.point.setValues(0, len(pts), pts)

    cp = _instantiate(coin, "SoFCControlPoints")
    cp.numPolesU.setValue(3)
    cp.numPolesV.setValue(3)
    cp.numKnotsU.setValue(2)
    cp.numKnotsV.setValue(2)

    root.addChild(coords)
    root.addChild(cp)
    return root


def _build_generic_scene(coin, type_name: str, *, width=None, height=None):
    del width, height
    root, _cam = _make_default_snapshot_scene(coin)
    root.addChild(_instantiate(coin, type_name))
    return root


def _make_top_view_scene(coin, node, *, center: tuple[float, float, float], camera_height: float):
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    cam.position.setValue(center[0], center[1], center[2] + 30.0)
    cam.nearDistance.setValue(1.0)
    cam.farDistance.setValue(100.0)
    cam.height.setValue(camera_height)
    root.addChild(cam)

    light_model = coin.SoLightModel()
    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
    root.addChild(light_model)

    root.addChild(node)
    return root


def _make_so_datum_label_symmetric_scene(coin):
    root = coin.SoSeparator()

    cam = coin.SoOrthographicCamera()
    cam.position.setValue(0.0, 0.0, 2.0)
    cam.nearDistance.setValue(1.0)
    cam.farDistance.setValue(5.0)
    cam.height.setValue(2.0)
    root.addChild(cam)

    root.addChild(coin.SoDirectionalLight())

    label = _instantiate(coin, "SoDatumLabel")
    label.string.setValue("")
    label.textColor.setValue(0.05, 0.35, 0.90)
    label.name.setValue(_DEFAULT_FONT_FAMILY)
    label.size.setValue(18)
    label.lineWidth.setValue(2.0)
    label.sampling.setValue(2.0)
    label.datumtype.setValue(_SO_DATUM_LABEL_TYPES["SYMMETRIC"])
    # fmt: off
    label.pnts.setValues(0, 2, [
        coin.SbVec3f(-0.38, 0.0, 0.0),
        coin.SbVec3f(0.38, 0.0, 0.0),
    ])
    # fmt: on
    root.addChild(label)

    return root


def _camera_type(coin):
    t = coin.SoType.fromName("SoCamera")
    if t.isBad():
        raise unittest.SkipTest("Coin SoCamera type is not registered")
    return t


def _find_scene_camera(coin, root):
    search = coin.SoSearchAction()
    search.setType(_camera_type(coin))
    search.setSearchingAll(False)
    search.apply(root)
    path = search.getPath()
    return path.getTail() if path is not None else None


def _frame_scene_camera_impl(coin, root, width: int, height: int):
    cam = _find_scene_camera(coin, root)
    if cam is None:
        raise AssertionError("scene graph does not contain a camera for snapshot framing")

    viewport = coin.SbViewportRegion(width, height)

    # Some GUI helper nodes (e.g. SoDatumLabel) compute a camera-dependent bounding box.
    # That makes `SoCamera::viewAll()` unstable and can shift the framing of the snapshot.
    # For these, frame the camera from the rest of the scene and render the full graph.
    removed = None
    dtype = coin.SoType.fromName("SoDatumLabel")
    if not dtype.isBad():
        search = coin.SoSearchAction()
        search.setType(dtype)
        search.setSearchingAll(False)
        search.apply(root)
        path = search.getPath()
        if path is not None and path.getLength() >= 2:
            label = path.getTail()
            parent = path.getNode(path.getLength() - 2)
            idx = parent.findChild(label)
            if idx >= 0:
                label.ref()
                parent.removeChild(idx)
                removed = (parent, idx, label)

    cam.viewAll(root, viewport)

    if removed is not None:
        parent, idx, label = removed
        parent.insertChild(label, idx)
        label.unref()

    # `SoCamera::viewAll()` can choose a near plane that clips geometry located near the origin.
    # This shows up particularly with `SoText2` (text draws, but gets clipped away).
    cam.nearDistance.setValue(min(cam.nearDistance.getValue(), 0.1))


def _frame_scene_camera(coin, root, width: int, height: int):
    _frame_scene_camera_impl(coin, root, width, height)


def _fixture(
    builder,
    *,
    framing_policy=CameraPolicy.VIEW_ALL,
    required_modules=(),
    supported_renderers=ALL_RENDERERS,
):
    return SnapshotFixture(
        builder=builder,
        framing_policy=framing_policy,
        required_modules=tuple(required_modules),
        supported_renderers=frozenset(supported_renderers),
    )


SNAPSHOT_FIXTURES = {
    "SoFCBoundingBox": _fixture(
        _build_bounding_box_scene,
        framing_policy=CameraPolicy.VIEW_ALL_WITH_MARGIN,
        supported_renderers=LEGACY_ONLY,
    ),
    "SoPreviewShape": _fixture(
        _build_preview_shape_scene,
        required_modules=("PartGui",),
        supported_renderers=LEGACY_ONLY,
    ),
    "SoDrawingGrid": _fixture(_build_drawing_grid_scene, framing_policy=CameraPolicy.FIXED_OVERLAY),
    "SoRegPoint": _fixture(_build_reg_point_scene, framing_policy=CameraPolicy.FIXED_OVERLAY),
    "SoDatumLabel": _fixture(_build_datum_label_scene, framing_policy=CameraPolicy.FIXED_OVERLAY),
    "SoStringLabel": _fixture(_build_string_label_scene),
    "SoColorBarLabel": _fixture(
        _build_color_bar_label_scene,
        framing_policy=CameraPolicy.FIXED_OVERLAY,
        supported_renderers=LEGACY_ONLY,
    ),
    "SoFrameLabel": _fixture(_build_frame_label_scene, framing_policy=CameraPolicy.FIXED_OVERLAY),
    "SoFCColorBar": _fixture(
        _build_color_bar_scene,
        framing_policy=CameraPolicy.FIXED_OVERLAY,
        supported_renderers=LEGACY_ONLY,
    ),
    "So3DAnnotation": _fixture(
        _build_3d_annotation_scene,
        supported_renderers=LEGACY_ONLY,
    ),
    "SoFCPlacementIndicatorKit": _fixture(
        _build_placement_indicator_scene,
        supported_renderers=LEGACY_ONLY,
    ),
    "SoAxisCrossKit": _fixture(
        _build_axis_cross_scene,
        supported_renderers=LEGACY_ONLY,
    ),
    "SoFCBackgroundGradient": _fixture(
        _build_background_gradient_scene,
        framing_policy=CameraPolicy.FIXED_OVERLAY,
    ),
    **{name: _fixture(_build_navi_cube_scene) for name in _NAVI_CUBE_NODE_TYPES},
    **{
        name: _fixture(_build_brep_edge_set_scene, required_modules=("PartGui",))
        for name in _BREP_EDGE_SET_NODE_TYPES
    },
    **{
        name: _fixture(_build_brep_point_set_scene, required_modules=("PartGui",))
        for name in _BREP_POINT_SET_NODE_TYPES
    },
    **{
        name: _fixture(_build_brep_face_set_scene, required_modules=("PartGui",))
        for name in _BREP_FACE_SET_NODE_TYPES
    },
    "SoFCControlPoints": _fixture(
        _build_control_points_scene,
        required_modules=("PartGui",),
    ),
    **{
        name: _fixture(_build_polygon_scene, required_modules=("MeshGui",))
        for name in _POLYGON_NODE_TYPES
    },
    **{
        name: _fixture(_build_indexed_face_set_scene, required_modules=("MeshGui",))
        for name in _INDEXED_FACE_SET_NODE_TYPES
    },
}


def get_snapshot_fixture(type_name: str) -> SnapshotFixture:
    try:
        return SNAPSHOT_FIXTURES[type_name]
    except KeyError as exc:
        raise unittest.SkipTest(f"no Coin snapshot fixture is registered for {type_name}") from exc


def selected_snapshot_nodes() -> list[str]:
    nodes_env = os.environ.get("FC_VISUAL_NODES", "")
    if nodes_env.strip():
        return [name.strip() for name in nodes_env.split(",") if name.strip()]
    return list(SNAPSHOT_FIXTURES)


def frame_snapshot_camera(runtime: SnapshotRuntime, root, policy: CameraPolicy):
    if policy is CameraPolicy.FIXED_OVERLAY:
        return
    _frame_scene_camera(runtime.coin, root, runtime.width, runtime.height)
    if policy is CameraPolicy.VIEW_ALL_WITH_MARGIN:
        camera = _find_scene_camera(runtime.coin, root)
        if camera is None:
            raise AssertionError("scene graph does not contain a camera for snapshot framing")
        camera.height.setValue(camera.height.getValue() * 1.25)
