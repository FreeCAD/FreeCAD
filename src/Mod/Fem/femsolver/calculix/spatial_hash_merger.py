# ***************************************************************************
# *   Copyright (c) 2026 Justin Adams <justin.adams@q2computing.com>        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM CalculiX O(N) Spatial Hash Node Merger"
__author__ = "Justin Adams - Q2 Computing LLC"
__url__ = "https://www.freecad.org"

import re
from itertools import product


class SpatialHashNodeMerger:
    """
    O(N) coincident node merging via spatial hashing.

    Resolves node ID collisions that occur when multiple meshes
    of different dimensionality (1D/2D/3D) are exported to a
    single CalculiX .inp file via writeABAQUS.

    The grid cell size equals the merge tolerance, guaranteeing
    that any coincident node within tolerance falls within the
    27-cell neighborhood. This ensures O(N) worst-case complexity.

    Tolerance guidance:
        FDM 3D printing : 0.2 mm
        CNC milling     : 0.01 mm
        Slip fit        : 0.005 mm

    Parameters
    ----------
    tolerance : float
        Merge threshold in mm. Recommended: sigma/2 where sigma
        is the manufacturing tolerance of the target process.
    """

    def __init__(self, tolerance=0.1):
        self.tolerance = tolerance
        self.grid = {}
        self.new_nodes = []
        self.node_map = {}

    def add_node(self, global_id, x, y, z):
        """
        Add a node. Merges to existing node if within tolerance.
        O(1) per call. O(N) total.
        """
        cell = (
            int(x / self.tolerance),
            int(y / self.tolerance),
            int(z / self.tolerance),
        )

        for delta in product([-1, 0, 1], repeat=3):
            neighbor = tuple(c + d for c, d in zip(cell, delta))
            if neighbor in self.grid:
                existing_id = self.grid[neighbor]
                ex, ey, ez = self.new_nodes[existing_id]
                dist = ((x - ex) ** 2 + (y - ey) ** 2 + (z - ez) ** 2) ** 0.5
                if dist <= self.tolerance:
                    self.node_map[global_id] = existing_id
                    return

        new_id = len(self.new_nodes)
        self.grid[cell] = new_id
        self.new_nodes.append((x, y, z))
        self.node_map[global_id] = new_id

    def get_merged_nodes(self):
        """Returns deduplicated node list and idx->new_id mapping."""
        return self.new_nodes, self.node_map


def merge_inp_nodes(inp_file_path, tolerance=0.1):
    """
    Post-process a CalculiX .inp file to resolve coincident
    node ID collisions using O(N) spatial hashing.

    Preserves the full element-to-node dependency chain across
    all mesh blocks. Elements are remapped using the block they
    belong to, so node ID 2 in the line mesh resolves to the
    correct physical node even if node ID 2 also exists in the
    cube mesh at a different location.

    Parameters
    ----------
    inp_file_path : str
        Absolute path to the .inp file to process.
    tolerance : float
        Merge threshold in mm. Default 0.1mm (FDM printing).
    """
    import FreeCAD
    FreeCAD.Console.PrintMessage(
        f"Q2 Computing: Merging nodes (tolerance={tolerance}mm)...\n"
    )

    node_pattern = re.compile(
        r"^\s*(\d+)\s*,\s*([-\d.eE+]+)\s*,\s*([-\d.eE+]+)\s*,\s*([-\d.eE+]+)"
    )

    with open(inp_file_path, "r", encoding="latin-1") as f:
        lines = f.readlines()

    # --- Phase 1: Parse all *NODE blocks, building a block-aware index ---
    #
    # raw_nodes: list of (idx, nid, x, y, z)
    #   idx  = unique global position across all blocks (used as merger key)
    #   nid  = original node ID within its block (used for element remapping)
    #
    # block_node_map: list of dicts, one per block
    #   block_node_map[block_index][nid] = idx
    #   This preserves the dependency chain: element node references are
    #   resolved using the block the element belongs to, not a global nid.

    raw_nodes = []
    block_node_map = []
    in_node_block = False
    current_block_map = {}
    idx = 0

    for line in lines:
        stripped = line.strip().upper()

        if stripped.startswith("*NODE"):
            if in_node_block or current_block_map:
                block_node_map.append(current_block_map)
                current_block_map = {}
            in_node_block = True
            continue

        if in_node_block:
            if stripped.startswith("*"):
                in_node_block = False
                block_node_map.append(current_block_map)
                current_block_map = {}
                continue
            match = node_pattern.match(line)
            if match:
                nid = int(match.group(1))
                if nid not in current_block_map:
                    current_block_map[nid] = idx
                    raw_nodes.append((
                        idx,
                        nid,
                        float(match.group(2)),
                        float(match.group(3)),
                        float(match.group(4))
                    ))
                    idx += 1

    if current_block_map:
        block_node_map.append(current_block_map)

    # --- Phase 2: Build spatial hash using global idx as key ---
    merger = SpatialHashNodeMerger(tolerance=tolerance)
    for entry_idx, nid, x, y, z in raw_nodes:
        merger.add_node(entry_idx, x, y, z)

    merged_nodes, index_map = merger.get_merged_nodes()

    # Build per-block remapping tables:
    # remap[block_index][original_nid] = new_sequential_id (1-based)
    remap = []
    for block_map in block_node_map:
        block_remap = {}
        for nid, entry_idx in block_map.items():
            block_remap[nid] = index_map[entry_idx] + 1
        remap.append(block_remap)

    FreeCAD.Console.PrintMessage(
        f"Q2 Computing: {len(raw_nodes)} raw -> {len(merged_nodes)} unique\n"
    )

    # --- Phase 3: Rewrite the .inp file ---
    # First *NODE block is replaced with the full merged node list.
    # Subsequent *NODE blocks are removed entirely.
    # Element lines are remapped using the block they belong to,
    # preserving the full element-to-node dependency chain.

    new_lines = []
    in_node_block = False
    node_block_written = False
    current_block_index = -1

    for line in lines:
        stripped = line.strip().upper()

        # First *NODE block — write all merged nodes here
        if stripped.startswith("*NODE") and not node_block_written:
            in_node_block = True
            current_block_index += 1
            new_lines.append(line)
            for new_id, (x, y, z) in enumerate(merged_nodes):
                new_lines.append(
                    f"{new_id + 1}, {x:.13G}, {y:.13G}, {z:.13G}\n"
                )
            node_block_written = True
            continue

        # Subsequent *NODE blocks — skip header, advance block counter
        if stripped.startswith("*NODE") and node_block_written:
            in_node_block = True
            current_block_index += 1
            continue

        if in_node_block:
            if stripped.startswith("*"):
                # End of node block — write the terminating keyword line
                in_node_block = False
                new_lines.append(line)
            # Skip all original node data lines
            continue

        # Remap element connectivity using the current block's map
        if node_block_written and not stripped.startswith("*"):
            parts = line.split(",")
            if len(parts) > 1:
                try:
                    block_remap = (
                        remap[current_block_index]
                        if current_block_index < len(remap)
                        else {}
                    )
                    remapped = [parts[0]]
                    for part in parts[1:]:
                        ps = part.strip()
                        if ps.isdigit():
                            old_id = int(ps)
                            new_id = block_remap.get(old_id, old_id)
                            remapped.append(f" {new_id}")
                        else:
                            remapped.append(part)
                    result = ",".join(remapped)
                    if not result.endswith("\n"):
                        result += "\n"
                    new_lines.append(result)
                    continue
                except (ValueError, KeyError):
                    pass

        new_lines.append(line)

    with open(inp_file_path, "w", encoding="latin-1") as f:
        f.writelines(new_lines)

    FreeCAD.Console.PrintMessage("Merge complete.\n")