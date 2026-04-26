import Part

GDT_MAP = {
    'cylinder': ['cylindricity', 'circularity', 'straightness', 'position'],
    'plane': ['flatness', 'parallelism', 'perpendicularity', 'angularity'],
    'cone': ['circularity', 'angularity'],
    'sphere': ['circularity', 'sphericity'],
}

def detect_features(shape):
    candidates = []
    for i, face in enumerate(shape.Faces):
        surface = face.Surface
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
