# SPDX-License-Identifier: LGPL-2.1-or-later

"""Export MbDFEM assemblies to FreeCADMbD ASMT text files."""

from pathlib import Path


LENGTH_SCALE = 0.001


def export_assembly(assembly, filename):
    """Write *assembly* to *filename* as native ASMT text and return the path."""
    path = Path(filename)
    path.parent.mkdir(parents=True, exist_ok=True)

    writer = _ASMTWriter()
    model = _Model(assembly)
    model.write(writer)
    path.write_text(writer.text(), encoding="utf-8")
    return str(path)


class _ASMTWriter:
    def __init__(self):
        self.lines = []

    def line(self, level, value):
        self.lines.append(f"{'\t' * level}{value}")

    def key_value(self, key_level, key, value):
        self.line(key_level, key)
        self.line(key_level + 1, value)

    def vector(self, level, values):
        self.line(level, "\t".join(_number(value) for value in values) + "\t")

    def matrix(self, level, rows):
        for row in rows:
            self.vector(level, row)

    def text(self):
        return "\n".join(self.lines) + "\n"


class _Model:
    def __init__(self, assembly):
        self.assembly = assembly
        self.name = _safe_name(assembly)
        self.fixed_parts = _assembly_fixed_parts(assembly)
        self.parts = [
            part
            for part in _assembly_movable_parts(assembly)
            if part not in self.fixed_parts
        ]

    def write(self, writer):
        writer.line(0, "FreeCADMbD")
        writer.line(0, "Assembly")
        writer.key_value(1, "Notes", "(Text string: '' runs: (Core.RunArray new))")
        writer.key_value(1, "Name", self.name)
        _write_spatial_container(writer, 1, self.assembly, self._assembly_markers())
        self._write_parts(writer)
        writer.line(1, "KinematicIJs")
        self._write_constraint_sets(writer)
        writer.line(1, "ForceTorques")
        self._write_gravity(writer)
        self._write_simulation_parameters(writer)
        self._write_animation_parameters(writer)

    def _assembly_markers(self):
        markers = []
        for fixed_part in self.fixed_parts:
            for marker in getattr(fixed_part, "markers", []):
                markers.append(_GroundMarker(marker, fixed_part))
        return markers

    def _write_parts(self, writer):
        writer.line(1, "Parts")
        for part in self.parts:
            _write_part(writer, 2, part)

    def _write_constraint_sets(self, writer):
        writer.line(1, "ConstraintSets")
        writer.line(2, "Joints")
        for joint in getattr(self.assembly, "joints", []):
            joint_type = _joint_type_name(joint)
            marker_i = getattr(joint, "markerI", None)
            marker_j = getattr(joint, "markerJ", None)
            part_i = _part_containing_marker(self.parts + self.fixed_parts, marker_i)
            part_j = _part_containing_marker(self.parts + self.fixed_parts, marker_j)
            if marker_i is None or marker_j is None or part_i is None or part_j is None:
                continue
            _write_joint(
                writer,
                3,
                joint_type,
                _safe_name(joint),
                self._marker_path(part_i, marker_i),
                self._marker_path(part_j, marker_j),
            )
        writer.line(2, "Motions")
        writer.line(2, "GeneralConstraintSets")

    def _marker_path(self, part, marker):
        if part in self.fixed_parts:
            return _ground_marker_path(self.name, part, marker)
        return _marker_path(self.name, part, marker)

    def _write_gravity(self, writer):
        gravity = self.assembly.getGravity() if hasattr(self.assembly, "getGravity") else None
        vector = gravity.gravity if gravity is not None else None
        writer.line(1, "ConstantGravity")
        if vector is None:
            writer.vector(2, [0.0, 0.0, -9.81])
        else:
            writer.vector(2, [vector.x, vector.y, vector.z])

    def _write_simulation_parameters(self, writer):
        parameters = (
            self.assembly.getSimulationParameters()
            if hasattr(self.assembly, "getSimulationParameters")
            else None
        )
        start = float(parameters.startTime) if parameters is not None else 0.0
        end = float(parameters.endTime) if parameters is not None else 1.0
        step = float(parameters.outputInterval) if parameters is not None else 0.01
        step_size = float(parameters.stepSize) if parameters is not None else 0.001
        digits = int(parameters.significantDigits) if parameters is not None else 6

        writer.line(1, "SimulationParameters")
        writer.key_value(2, "tstart", _number(start))
        writer.key_value(2, "tend", _number(end))
        writer.key_value(2, "hmin", _number(max(step_size * 1.0e-6, 1.0e-12)))
        writer.key_value(2, "hmax", _number(max(step_size, step)))
        writer.key_value(2, "hout", _number(step))
        writer.key_value(2, "errorTol", _number(10.0 ** (-digits)))

    def _write_animation_parameters(self, writer):
        parameters = (
            self.assembly.getAnimationParameters()
            if hasattr(self.assembly, "getAnimationParameters")
            else None
        )
        frame_rate = int(parameters.frameRate) if parameters is not None else 30
        writer.line(1, "AnimationParameters")
        writer.key_value(2, "nframe", "1000000")
        writer.key_value(2, "icurrent", "1")
        writer.key_value(2, "istart", "1")
        writer.key_value(2, "iend", "1000000")
        writer.key_value(2, "isForward", "true")
        writer.key_value(2, "framesPerSecond", str(frame_rate))


class _GroundMarker:
    def __init__(self, marker, part):
        self.Name = f"Ground_{_safe_name(part)}_{_safe_name(marker)}"
        self.RefPointPlacement = getattr(part, "Placement", None)
        self.Placement = getattr(marker, "Placement", None)


def _write_part(writer, level, part):
    writer.line(level, "Part")
    writer.key_value(level + 1, "Name", _safe_name(part))
    _write_spatial_kinematics(writer, level + 1, part)
    writer.line(level + 1, "FeatureOrder")
    _write_principal_mass_marker(writer, level + 1)
    _write_references(writer, level + 1, getattr(part, "markers", []), lambda _marker: None)


def _write_spatial_container(writer, level, obj, markers):
    _write_spatial_kinematics(writer, level, obj)
    _write_references(writer, level, markers)


def _write_spatial_kinematics(writer, level, obj):
    _write_spatial_item(writer, level, obj)
    writer.line(level, "Velocity3D")
    writer.vector(level + 1, [0.0, 0.0, 0.0])
    writer.line(level, "Omega3D")
    writer.vector(level + 1, [0.0, 0.0, 0.0])


def _write_references(writer, level, markers, refpoint_placement=None):
    if refpoint_placement is None:
        refpoint_placement = _refpoint_placement
    writer.line(level, "RefPoints")
    for marker in markers:
        writer.line(level + 1, "RefPoint")
        _write_placement(writer, level + 2, refpoint_placement(marker))
        writer.line(level + 2, "Markers")
        writer.line(level + 3, "Marker")
        writer.key_value(level + 4, "Name", _safe_name(marker))
        _write_spatial_item(writer, level + 4, marker)
    writer.line(level, "RefCurves")
    writer.line(level, "RefSurfaces")


def _write_spatial_item(writer, level, obj):
    placement = getattr(obj, "Placement", None)
    _write_placement(writer, level, placement)


def _write_placement(writer, level, placement):
    base = placement.Base if placement is not None else None
    writer.line(level, "Position3D")
    writer.vector(level + 1, _position_values(base))
    writer.line(level, "RotationMatrix")
    writer.matrix(level + 1, _rotation_rows(placement))


def _write_principal_mass_marker(writer, level):
    writer.line(level, "PrincipalMassMarker")
    writer.key_value(level + 1, "Name", "MassMarker")
    writer.line(level + 1, "Position3D")
    writer.vector(level + 2, [0.0, 0.0, 0.0])
    writer.line(level + 1, "RotationMatrix")
    writer.matrix(level + 2, _identity_rows())
    writer.key_value(level + 1, "Mass", "1")
    writer.line(level + 1, "MomentOfInertias")
    writer.vector(level + 2, [1.0, 1.0, 1.0])
    writer.key_value(level + 1, "Density", "1")


def _write_joint(writer, level, joint_type, name, marker_i, marker_j):
    writer.line(level, joint_type)
    writer.key_value(level + 1, "Name", name)
    writer.key_value(level + 1, "MarkerI", marker_i)
    writer.key_value(level + 1, "MarkerJ", marker_j)


def _joint_type_name(joint):
    mapping = {
        "Fixed": "FixedJoint",
        "Revolute": "RevoluteJoint",
        "Prismatic": "TranslationalJoint",
        "Translational": "TranslationalJoint",
        "Cylindrical": "CylindricalJoint",
        "Spherical": "SphericalJoint",
        "Universal": "UniversalJoint",
    }
    return mapping.get(str(getattr(joint, "jointType", "Fixed")), "FixedJoint")


def _part_containing_marker(parts, marker):
    for part in parts:
        if marker in getattr(part, "markers", []):
            return part
    return None


def _assembly_movable_parts(assembly):
    folder_parts = _folder_group(assembly, "getPartsFolder")
    property_parts = list(getattr(assembly, "parts", []))
    return _unique(folder_parts or property_parts)


def _assembly_fixed_parts(assembly):
    folder_fixed_parts = _folder_group(assembly, "getFixedPartsFolder")
    property_fixed_parts = list(getattr(assembly, "fixedparts", []))
    return _unique(folder_fixed_parts + property_fixed_parts)


def _folder_group(assembly, getter_name):
    getter = getattr(assembly, getter_name, None)
    if getter is None:
        return []
    folder = getter()
    return list(getattr(folder, "Group", [])) if folder is not None else []


def _unique(items):
    result = []
    for item in items:
        if item not in result:
            result.append(item)
    return result


def _marker_path(assembly_name, part, marker):
    return f"/{assembly_name}/{_safe_name(part)}/{_safe_name(marker)}"


def _ground_marker_path(assembly_name, part, marker):
    return f"/{assembly_name}/{_safe_name(_GroundMarker(marker, part))}"


def _refpoint_placement(marker):
    return getattr(marker, "RefPointPlacement", getattr(marker, "Placement", None))



def _rotation_rows(placement):
    if placement is None:
        return _identity_rows()
    matrix = placement.Rotation.toMatrix()
    return [
        [matrix.A11, matrix.A12, matrix.A13],
        [matrix.A21, matrix.A22, matrix.A23],
        [matrix.A31, matrix.A32, matrix.A33],
    ]


def _identity_rows():
    return [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]


def _safe_name(obj):
    return getattr(obj, "Name", str(obj)).replace(" ", "_")


def _position_values(base):
    if base is None:
        return [0.0, 0.0, 0.0]
    return [base.x * LENGTH_SCALE, base.y * LENGTH_SCALE, base.z * LENGTH_SCALE]


def _number(value):
    return format(float(value), ".15g")
