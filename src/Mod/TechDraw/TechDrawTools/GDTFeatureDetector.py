import Part
import FreeCAD

# GD&T Symbol mapping per feature type
# Primary standard: ASME Y14.5-2018
# ISO GPS support planned via preference switch
GDT_MAP = {
    'cylinder': ['cylindricity', 'circularity', 'straightness', 'position'],
    'plane': ['flatness', 'parallelism', 'perpendicularity', 'angularity'],
    'cone': ['circularity', 'angularity'],
    'sphere': ['circularity', 'sphericity'],
}

def detect_features(shape, projection_direction=FreeCAD.Vector(0, 0, 1)):
    """
    Analyzes a Part.Shape and returns GD&T annotation candidates.
    
    Args:
        shape: Part.Shape object from FreeCAD Part feature
        projection_direction: View projection direction vector.
                            Faces parallel to this direction 
                            are culled (won't appear in drawing).
    
    Returns:
        List of candidate dicts with feature type, geometry info,
        and valid GD&T symbols per ASME Y14.5-2018.
        ISO GPS support planned via preference switch.
    
    Note:
        Face culling check: if face normal is parallel to 
        projection direction, face won't appear in the 2D 
        drawing view and is excluded from candidates.
    """
    candidates = []
    
    for i, face in enumerate(shape.Faces):
        surface = face.Surface

        # Face culling — skip faces parallel to projection direction
        # These faces won't appear in the 2D drawing view
        try:
            normal = face.normalAt(0, 0)
            dot = abs(normal.dot(projection_direction))
            if dot < 0.1:  # face is parallel to projection — cull it
                continue
        except Exception:
            pass  # if normal check fails, include the face

        if isinstance(surface, Part.Cylinder):
            candidates.append({
                'index': i,
                'type': 'cylinder',
                'radius': round(surface.Radius, 4),
                'valid_gdt': GDT_MAP['cylinder'],
                'default': 'cylindricity'
            })
        elif isinstance(surface, Part.Plane):
            candidates.append({
                'index': i,
                'type': 'plane',
                'normal': tuple(surface.Axis),
                'valid_gdt': GDT_MAP['plane'],
                'default': 'flatness'
            })
        elif isinstance(surface, Part.Cone):
            candidates.append({
                'index': i,
                'type': 'cone',
                'valid_gdt': GDT_MAP['cone'],
                'default': 'circularity'
            })

    return candidates
