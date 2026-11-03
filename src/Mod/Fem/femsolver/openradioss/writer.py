# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Write an LS-DYNA format deck for OpenRadioss from a neutral FEM model.

No FreeCAD imports: the model is plain Python data, built from a FreeCAD FEM
analysis by ``model.py``. Units are mm, t, s, N, MPa throughout.

Model layout (dict)::

    title          str
    end_time       float  [s]
    output_interval float [s]  (animation frames)
    load_curve     "ramp" (0 -> 1 over end_time) or "constant"
    mass_scaling_dt float [s]  0 = off; otherwise mass is added where needed so the
                   time step does not drop below this value (*CONTROL_TIMESTEP DT2MS < 0)
    nodes          {node id: (x, y, z)}
    parts          [{"pid", "title", "kind": "solid" | "shell", "elform",
                     "thickness" (shells), "material": {"rho", "E", "nu"},
                     "elements": [(element id, [node ids])]}]
    fixed          [{"title", "nodes": [...], "dofs": (x, y, z, rx, ry, rz)}]  1 = fixed
    node_forces    [{"title", "loads": {node id: (fx, fy, fz)}}]
    pressures      [{"title", "pressure", "segments": [[n1, n2, n3(, n4)], ...]}]
    gravity        (gx, gy, gz) acceleration [mm/s^2] or None
    extra          str  additional keyword text inserted before *END

Pressure segments must be ordered so that their right-hand normal points out of
the body; a positive pressure then pushes into the body (LS-DYNA convention,
the same as FreeCAD's default pressure direction).
"""

__title__ = "FreeCAD FEM solver OpenRadioss deck writer"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

LOAD_CURVE_ID = 1
DOF_NAMES = ("X", "Y", "Z")


def _chunks(values, size=8):
    for i in range(0, len(values), size):
        yield values[i : i + size]


def _num(value):
    """Compact number formatting that LS-DYNA free format reads."""
    return f"{value:.9g}"


def _strip_frame(text):
    """Drop *KEYWORD / *END lines of additional keyword text."""
    return "\n".join(
        line for line in text.splitlines() if line.strip().upper() not in ("*KEYWORD", "*END")
    ).strip()


def write_deck(model):
    """Return the deck text for ``model`` (see module docstring)."""
    out = []
    add = out.append
    end_time = float(model["end_time"])
    add("*KEYWORD")
    add("*TITLE")
    add(model.get("title") or "FreeCAD FEM analysis")
    add("$ Written by FreeCAD FEM (units: mm, t, s, N, MPa)")

    add("*CONTROL_TERMINATION")
    add(_num(end_time))
    mass_dt = float(model.get("mass_scaling_dt") or 0.0)
    if mass_dt > 0:
        add("*CONTROL_TIMESTEP")
        add("$  dtinit    tssfac      isdo    tslimt     dt2ms")
        add(f"0.0, 0.9, 0, 0.0, {_num(-mass_dt)}")
    add("*DATABASE_BINARY_D3PLOT")
    add(_num(float(model.get("output_interval") or end_time / 20)))

    add("*DEFINE_CURVE")
    add(f"$ load curve: {model.get('load_curve', 'ramp')}")
    add(f"{LOAD_CURVE_ID}")
    if model.get("load_curve", "ramp") == "constant":
        add(f"0.0, 1.0\n{_num(end_time)}, 1.0")
    else:
        add(f"0.0, 0.0\n{_num(end_time)}, 1.0")

    used_nodes = set()
    for part in model["parts"]:
        pid = part["pid"]
        mat = part["material"]
        add("*PART")
        add(part.get("title") or f"Part {pid}")
        add(f"{pid}, {pid}, {pid}")
        if part["kind"] == "solid":
            add("*SECTION_SOLID")
            add(f"{pid}, {part.get('elform', 1)}")
        else:
            thickness = _num(float(part["thickness"]))
            add("*SECTION_SHELL")
            add(f"{pid}, {part.get('elform', 2)}")
            add(", ".join([thickness] * 4))
        add("*MAT_ELASTIC")
        add(f"{pid}, {_num(mat['rho'])}, {_num(mat['E'])}, {_num(mat['nu'])}")
        for _eid, nodes in part["elements"]:
            used_nodes.update(nodes)

    add("*NODE")
    for nid in sorted(used_nodes):
        x, y, z = model["nodes"][nid]
        add(f"{nid}, {_num(x)}, {_num(y)}, {_num(z)}")

    for part in model["parts"]:
        pid = part["pid"]
        if part["kind"] == "solid":
            add("*ELEMENT_SOLID")
            for eid, nodes in part["elements"]:
                add(f"{eid}, {pid}")
                add(", ".join(map(str, nodes)))
        else:
            add("*ELEMENT_SHELL")
            for eid, nodes in part["elements"]:
                # triangles repeat the last node
                quad = list(nodes) + [nodes[-1]] * (4 - len(nodes))
                add(f"{eid}, {pid}, " + ", ".join(map(str, quad)))

    set_id = 0
    for fixed in model.get("fixed", []):
        nodes = sorted(set(fixed["nodes"]) & used_nodes)
        if not nodes:
            continue
        set_id += 1
        add(f"$ fixed: {fixed.get('title', '')}")
        add("*SET_NODE_LIST")
        add(f"{set_id}")
        for chunk in _chunks(nodes):
            add(", ".join(map(str, chunk)))
        add("*BOUNDARY_SPC_SET")
        add(f"{set_id}, 0, " + ", ".join(str(int(d)) for d in fixed["dofs"]))

    for force in model.get("node_forces", []):
        lines = []
        for nid in sorted(force["loads"]):
            if nid not in used_nodes:
                continue
            for dof, component in enumerate(force["loads"][nid], 1):
                if component:
                    lines.append(f"{nid}, {dof}, {LOAD_CURVE_ID}, {_num(component)}")
        if lines:
            add(f"$ force: {force.get('title', '')}")
            add("*LOAD_NODE_POINT")
            out.extend(lines)

    for pressure in model.get("pressures", []):
        if not pressure["segments"]:
            continue
        add(f"$ pressure: {pressure.get('title', '')}")
        add("*LOAD_SEGMENT")
        p = _num(float(pressure["pressure"]))
        for segment in pressure["segments"]:
            nodes = list(segment) + [segment[-1]] * (4 - len(segment))
            add(f"{LOAD_CURVE_ID}, {p}, 0.0, " + ", ".join(map(str, nodes)))

    gravity = model.get("gravity")
    if gravity:
        for axis, g in zip(DOF_NAMES, gravity):
            if g:
                # LS-DYNA body loads give the acceleration of the reference frame:
                # gravity acting in -Z is *LOAD_BODY_Z with a positive value.
                add(f"*LOAD_BODY_{axis}")
                add(f"{LOAD_CURVE_ID}, {_num(-g)}")

    extra = _strip_frame(model.get("extra") or "")
    if extra:
        add("$ --- additional keywords ---")
        add(extra)

    add("*END")
    return "\n".join(out) + "\n"


##  @}
