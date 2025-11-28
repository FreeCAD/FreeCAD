"""
CollisionEngine: sample path model, check simple bounding-sphere collisions
Placeholder: real mesh collision later; current checks use axis limits and holder radius.

Enhanced with Ghost Gunner specific collision detection.
"""
import math


# Ghost Gunner machine envelopes (mm)
GHOST_GUNNER_ENVELOPES = {
    "GhostGunner2": {
        "work_envelope": {"X": (0, 150), "Y": (0, 150), "Z": (0, 80)},
        "fixture_plate": {"height": 5.0, "clearance": 2.0},
        "spindle_nose": {"diameter": 43.0, "length": 60.0},
        "max_tool_length": 50.0
    },
    "GhostGunner3": {
        "work_envelope": {"X": (0, 150), "Y": (0, 150), "Z": (0, 100)},
        "fixture_plate": {"height": 5.0, "clearance": 2.0},
        "spindle_nose": {"diameter": 43.0, "length": 60.0},
        "max_tool_length": 50.0
    },
    "GhostGunner3S": {
        "work_envelope": {"X": (0, 150), "Y": (0, 150), "Z": (0, 100)},
        "fixture_plate": {"height": 5.0, "clearance": 2.0},
        "spindle_nose": {"diameter": 43.0, "length": 60.0},
        "max_tool_length": 50.0
    },
    "GhostGunnerDefense": {
        "work_envelope": {"X": (0, 150), "Y": (0, 150), "Z": (0, 100)},
        "fixture_plate": {"height": 5.0, "clearance": 2.0},
        "spindle_nose": {"diameter": 43.0, "length": 60.0},
        "max_tool_length": 50.0
    },
    "GhostGunnerR": {
        "work_envelope": {"X": (0, 150), "Y": (0, 150), "Z": (0, 100)},
        "fixture_plate": {"height": 5.0, "clearance": 2.0},
        "spindle_nose": {"diameter": 43.0, "length": 60.0},
        "max_tool_length": 50.0,
        "rotary_table": {"diameter": 80.0, "height": 50.0, "center_height": 50.0}
    }
}


def check_limits(toolstate, limits):
    x, y, z = toolstate.pos
    L = limits
    if "X" in L and not (L["X"]["min"] <= x <= L["X"]["max"]): 
        return ("X", x)
    if "Y" in L and not (L["Y"]["min"] <= y <= L["Y"]["max"]): 
        return ("Y", y)
    if "Z" in L and not (L["Z"]["min"] <= z <= L["Z"]["max"]): 
        return ("Z", z)
    return None


def check_holder_interference(toolstate, holder):
    # holder: dict with 'radius' and 'offset' -- simplistic
    x, y, z = toolstate.pos
    hx, hy = holder.get("offset", (0, 0))
    r = holder.get("radius", 0)
    dist2 = (x - hx)**2 + (y - hy)**2
    if dist2 <= (r**2):
        return True
    return False


def check_ghostgunner_envelope(toolstate, machine_name):
    """
    Check if tool position is within Ghost Gunner machine envelope.
    
    Args:
        toolstate: Tool state with position
        machine_name: Ghost Gunner machine name
        
    Returns:
        List of warnings (empty if no violations)
    """
    warnings = []
    
    # Get envelope for this machine
    envelope = GHOST_GUNNER_ENVELOPES.get(machine_name)
    if not envelope:
        return warnings
    
    x, y, z = toolstate.pos
    
    # Check work envelope
    work_env = envelope["work_envelope"]
    for axis, (min_val, max_val) in work_env.items():
        if axis == "X" and not (min_val <= x <= max_val):
            warnings.append(f"X position {x:.2f} outside envelope [{min_val}, {max_val}]")
        elif axis == "Y" and not (min_val <= y <= max_val):
            warnings.append(f"Y position {y:.2f} outside envelope [{min_val}, {max_val}]")
        elif axis == "Z" and not (min_val <= z <= max_val):
            warnings.append(f"Z position {z:.2f} outside envelope [{min_val}, {max_val}]")
    
    # Check fixture plate clearance
    fixture = envelope["fixture_plate"]
    min_z = fixture["height"] + fixture["clearance"]
    if z < min_z:
        warnings.append(f"Z position {z:.2f} too close to fixture plate (min: {min_z:.2f})")
    
    # Check rotary table interference (if present)
    if "rotary_table" in envelope:
        rotary = envelope["rotary_table"]
        # Check if tool is within rotary table cylinder
        dist_from_center = math.sqrt(x**2 + y**2)
        if dist_from_center < (rotary["diameter"] / 2.0):
            if z < rotary["center_height"]:
                warnings.append(
                    f"Tool may collide with rotary table at ({x:.2f}, {y:.2f}, {z:.2f})"
                )
    
    return warnings


def check_ghostgunner_spindle_clearance(toolstate, machine_name, tool_length=25.0):
    """
    Check spindle nose clearance for Ghost Gunner machines.
    
    Args:
        toolstate: Tool state with position
        machine_name: Ghost Gunner machine name
        tool_length: Tool length (mm)
        
    Returns:
        List of warnings
    """
    warnings = []
    
    envelope = GHOST_GUNNER_ENVELOPES.get(machine_name)
    if not envelope:
        return warnings
    
    x, y, z = toolstate.pos
    
    # Check if tool length exceeds maximum
    max_tool_length = envelope.get("max_tool_length", 50.0)
    if tool_length > max_tool_length:
        warnings.append(
            f"Tool length {tool_length:.1f}mm exceeds maximum {max_tool_length:.1f}mm"
        )
    
    # Check spindle nose clearance
    spindle = envelope["spindle_nose"]
    spindle_bottom_z = z + tool_length + spindle["length"]
    
    # Warn if spindle might hit fixture
    fixture = envelope["fixture_plate"]
    if spindle_bottom_z < fixture["height"]:
        warnings.append(
            f"Spindle nose may collide with fixture at Z={z:.2f}"
        )
    
    return warnings


def run_collision_checks(model, machine_cfg, holder):
    warnings = []
    sampled = model.interpolate()
    
    # Detect if this is a Ghost Gunner machine
    machine_name = machine_cfg.get('name', '')
    is_ghost_gunner = any(gg in machine_name for gg in [
        'GhostGunner', 'ghostgunner', 'GG2', 'GG3', 'GGR', 'GGD'
    ])
    
    # Get tool length if available
    tool_info = machine_cfg.get('default_tool', {})
    tool_length = tool_info.get('length', 25.0)
    
    for s in sampled:
        # Standard limit checks
        lim = check_limits(s, machine_cfg.get('limits', {}))
        if lim:
            warnings.append(("limit", lim))
        
        # Standard holder interference
        if check_holder_interference(s, holder):
            warnings.append(("holder", s.pos))
        
        # Ghost Gunner specific checks
        if is_ghost_gunner:
            # Envelope checks
            gg_warnings = check_ghostgunner_envelope(s, machine_name)
            for w in gg_warnings:
                warnings.append(("ghostgunner_envelope", w))
            
            # Spindle clearance checks
            spindle_warnings = check_ghostgunner_spindle_clearance(
                s, machine_name, tool_length
            )
            for w in spindle_warnings:
                warnings.append(("ghostgunner_spindle", w))
    
    return warnings


def get_ghostgunner_collision_table(machine_name):
    """
    Get collision table for Ghost Gunner machine.
    
    Args:
        machine_name: Ghost Gunner machine name
        
    Returns:
        Dictionary with collision information
    """
    envelope = GHOST_GUNNER_ENVELOPES.get(machine_name, {})
    
    return {
        "machine": machine_name,
        "work_envelope": envelope.get("work_envelope", {}),
        "fixture_plate": envelope.get("fixture_plate", {}),
        "spindle_nose": envelope.get("spindle_nose", {}),
        "max_tool_length": envelope.get("max_tool_length", 50.0),
        "has_rotary": "rotary_table" in envelope,
        "rotary_table": envelope.get("rotary_table", {})
    }
