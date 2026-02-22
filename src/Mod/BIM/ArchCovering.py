# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

"""The Covering object and tools.

This module provides tools to build Covering objects. Coverings are claddings, floorings,
wallpapers, etc. applied to other construction elements, but can also be independent. They support
solid 3D tiles, parametric 2D patterns, and hatch patterns.
"""

import FreeCAD
import Part
import ArchComponent
import Arch
import ArchTessellation
from draftutils import params

MIN_DIMENSION = 0.1


# Translation shims for headless and static analysis
def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    # Runtime override using native FreeCAD.Qt abstraction
    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class _Covering(ArchComponent.Component):
    """
    A parametric object representing an architectural surface finish.

    This class manages the generation of surface treatments such as tiles, panels, flooring, or
    hatch patterns. Coverings are typically linked to a specific face of a base object and remain
    parametric, updating automatically if the underlying geometry changes.

    Parameters
    ----------
    obj : App::FeaturePython
        The base C++ object to be initialized as a Covering.

    Notes
    -----
    While the standard FreeCAD `TypeId` attribute identifies the underlying C++ class, the `Type`
    attribute is used by the Arch module to distinguish functional object types that share the same
    C++ implementation.

    Examples
    --------
    >>> print(obj.TypeId, "->", obj.Proxy.Type)
    Part::FeaturePython -> Covering
    >>> import Draft; Draft.get_type(obj)
    'Covering'
    """

    def __init__(self, obj):
        super().__init__(obj)

        # Override the parent's object type to set a specific one for Covering
        self.Type = "Covering"

        # Apply the property schema
        self.setProperties(obj)

        # Initialize properties with user preferences (params) upon object creation (but not on
        # document restore).
        # The properties mapped to parameters here are generally the ones that are editable in the
        # object's Task Panel. Since the UI binds to these properties, the Task Panel will
        # automatically display these values as the defaults without extra UI logic.
        obj.TileLength = params.get_param_arch("CoveringLength")
        obj.TileWidth = params.get_param_arch("CoveringWidth")
        obj.TileThickness = params.get_param_arch("CoveringThickness")
        obj.JointWidth = params.get_param_arch("CoveringJoint")
        obj.Rotation = params.get_param_arch("CoveringRotation")
        obj.TileAlignment = params.get_param_arch("CoveringAlignment")
        obj.FinishMode = params.get_param_arch("CoveringFinishMode")

    def setProperties(self, obj):
        """
        Overrides the parent method to define properties specific to the Covering object, including
        tiling and pattern schema, and ensures 'Base' supports sub-element face targeting.
        """
        # Override parent properties to ensure 'Base' is LinkSub (parent defines it as Link).
        # Covering objects use sub-element links (LinkSub) because they need to target specific
        # faces.
        if (
            "Base" in obj.PropertiesList
            and obj.getTypeIdOfProperty("Base") != "App::PropertyLinkSub"
        ):
            obj.setPropertyStatus("Base", "-LockDynamic")
            obj.removeProperty("Base")
        if "Base" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkSub",
                "Base",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The object or face this covering is applied to"
                ),
                locked=True,
            )

        # Apply the parent's property schema.
        super().setProperties(obj)

        # Snapshot properties once
        properties_list = obj.PropertiesList

        # Apply the local property schema
        # (Type, Name, Group, Tooltip, InitialValue)
        properties_schema = [
            (
                "App::PropertyEnumeration",
                "FinishMode",
                "Covering",
                "The type of finish to create",
                ["Solid Tiles", "Parametric Pattern", "Hatch Pattern"],
            ),
            (
                "App::PropertyEnumeration",
                "TileAlignment",
                "Covering",
                "The alignment of the tile grid",
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"],
            ),
            ("App::PropertyAngle", "Rotation", "Covering", "Rotation of the finish", 0),
            ("App::PropertyLength", "TileLength", "Tiles", "The length of the tiles", 0),
            ("App::PropertyLength", "TileWidth", "Tiles", "The width of the tiles", 0),
            ("App::PropertyLength", "TileThickness", "Tiles", "The thickness of the tiles", 0),
            ("App::PropertyLength", "JointWidth", "Tiles", "The width of the joints", None),
            ("App::PropertyVector", "TileOffset", "Tiles", "The offset of alternating rows", None),
            (
                "App::PropertyVector",
                "AlignmentOffset",
                "Tiles",
                "A manual offset to shift the grid origin (X=U, Y=V). The Z component is ignored",
                None,
            ),
            ("App::PropertyArea", "NetArea", "Quantities", "The surface area of the base face", 0),
            (
                "App::PropertyArea",
                "GrossArea",
                "Quantities",
                "Total area of material units consumed (Full + Partial)",
                0,
            ),
            (
                "App::PropertyArea",
                "WasteArea",
                "Quantities",
                "The area of discarded material (Gross - Net)",
                0,
            ),
            (
                "App::PropertyLength",
                "TotalJointLength",
                "Quantities",
                "The total linear length of all joints",
                0,
            ),
            (
                "App::PropertyLength",
                "PerimeterLength",
                "Quantities",
                "The length of the substrate perimeter",
                0,
            ),
            ("App::PropertyInteger", "CountFullTiles", "Quantities", "The number of full tiles", 0),
            (
                "App::PropertyInteger",
                "CountPartialTiles",
                "Quantities",
                "The number of cut/partial tiles",
                0,
            ),
            (
                "App::PropertyFile",
                "PatternFile",
                "Pattern",
                "The PAT file to use for hatching",
                None,
            ),
            (
                "App::PropertyString",
                "PatternName",
                "Pattern",
                "The name of the pattern in the PAT file",
                "",
            ),
            (
                "App::PropertyFloat",
                "PatternScale",
                "Pattern",
                "The scale of the hatch pattern",
                1.0,
            ),
            (
                "App::PropertyEnumeration",
                "IfcPredefinedType",
                "IFC",
                "The specific type of covering",
                [
                    "FLOORING",
                    "CLADDING",
                    "ROOFING",
                    "MOLDING",
                    "SKIRTINGBOARD",
                    "CEILING",
                    "WRAPPING",
                    "NOTDEFINED",
                ],
            ),
        ]

        for prop_type, name, group, tooltip, default in properties_schema:
            if name not in properties_list:
                obj.addProperty(
                    prop_type, name, group, QT_TRANSLATE_NOOP("App::Property", tooltip), locked=True
                )

                # Apply defined default values
                if default is not None:
                    setattr(obj, name, default)

        # Property status configuration (Read-Only fields)
        obj.setEditorMode("NetArea", 1)
        obj.setEditorMode("GrossArea", 1)
        obj.setEditorMode("WasteArea", 1)
        obj.setEditorMode("TotalJointLength", 1)
        obj.setEditorMode("PerimeterLength", 1)
        obj.setEditorMode("CountFullTiles", 1)
        obj.setEditorMode("CountPartialTiles", 1)

    def loads(self, state):
        """
        Overrides the parent callback used by FreeCAD's persistence engine to restore the
        Python proxy instance and reset non-persistent internal attributes like 'Type'.
        """
        self.Type = "Covering"

    def onDocumentRestored(self, obj):
        """
        Overrides the parent callback triggered after the document is fully restored. Used to
        ensure property schema consistency and perform backward compatibility migrations.
        """
        super().onDocumentRestored(obj)
        self.setProperties(obj)

    def onChanged(self, obj, prop):
        """Method called when a property is changed."""
        ArchComponent.Component.onChanged(self, obj, prop)

    def execute(self, obj):
        """
        Calculates the geometry and updates the shape of the object.

        This is a standard FreeCAD C++ callback triggered during a document recompute. It translates
        the numerical and textual properties of the object into a geometric representation (solids,
        faces, or wires) assigned to the `Shape` attribute.

        Parameters
        ----------
        obj : Part::FeaturePython
            The base C++ object whose shape is updated.
        """
        if self.clone(obj):
            return

        # Resolve target geometry upon which to apply the covering
        base_face = Arch.getFaceGeometry(obj.Base)
        if not base_face:
            return

        # Establish local coordinate system and grid alignment
        u_vec, v_vec, normal, center_point = Arch.getFaceUV(base_face)
        origin = Arch.getFaceGridOrigin(
            base_face,
            center_point,
            u_vec,
            v_vec,
            obj.TileAlignment,
            obj.AlignmentOffset,
        )

        # Instantiate the tessellator
        # For parametric patterns, we want 2D geometry (wires/faces), not solids.
        # We force thickness to 0.0 to instruct the tessellator to skip extrusion.
        config = {
            "TileLength": obj.TileLength.Value,
            "TileWidth": obj.TileWidth.Value,
            "JointWidth": obj.JointWidth.Value,
            "TileOffset": obj.TileOffset,
            "Rotation": obj.Rotation.Value,
            "PatternFile": obj.PatternFile,
            "PatternName": obj.PatternName,
            "PatternScale": obj.PatternScale,
            "TileThickness": (
                0.0 if obj.FinishMode == "Parametric Pattern" else obj.TileThickness.Value
            ),
        }
        tessellator = ArchTessellation.create_tessellator(obj.FinishMode, config)

        # Generate pattern cutters and metadata
        res = tessellator.compute(base_face, origin, u_vec, v_vec, normal)

        match res.status:
            case ArchTessellation.TessellationStatus.INVALID_DIMENSIONS:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The specified tile size is too "
                        "small to be modeled. The covering shape has been reset.",
                    )
                    + "\n"
                )
                obj.Shape = Part.Shape()
                return
            case ArchTessellation.TessellationStatus.JOINT_TOO_SMALL:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The joint width is too small to "
                        "model individual units. The covering will be shown as a continuous surface.",
                    )
                    + "\n"
                )
            case ArchTessellation.TessellationStatus.COUNT_TOO_HIGH:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The number of tiles is too high "
                        "for individual units to be modeled. The covering will be shown as a "
                        "continuous surface for better performance.",
                    )
                    + "\n"
                )
            case ArchTessellation.TessellationStatus.EXTREME_COUNT:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The number of tiles is extremely "
                        "high. Layout lines are hidden to maintain 3D performance.",
                    )
                    + "\n"
                )
            case _:
                pass

        obj.Shape = res.geometry

        # Sync quantities
        obj.CountFullTiles = res.quantities.count_full
        obj.CountPartialTiles = res.quantities.count_partial
        obj.NetArea = res.quantities.area_net
        obj.GrossArea = res.quantities.area_gross
        obj.WasteArea = res.quantities.waste_area
        obj.TotalJointLength = res.quantities.length_joints
        obj.PerimeterLength = res.quantities.length_perimeter
