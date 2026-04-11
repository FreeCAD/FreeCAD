# ============================================================================
# HatchCore.py
# Core hatch generation logic for the FreeCAD Hatch Generator
# ============================================================================
# This file contains the core algorithms for building hatches:
#   - Pattern normalization and transformation
#   - Clipping and boolean operations
#   - Tile generation and placement
#   - Distribution modes (grid, radial, random, etc.)
# ============================================================================

import FreeCAD
import Part
import math
import random

# ============================================================================
# Pattern Normalization and Transformation
# ============================================================================

def normalizePatternShape(patternShape):
    """Normalize pattern shape to origin (0,0) based on its bounding box."""
    bb = patternShape.BoundBox
    dx = -bb.XMin
    dy = -bb.YMin
    mat = FreeCAD.Matrix()
    mat.move(FreeCAD.Vector(dx, dy, 0))
    newShape = patternShape.copy()
    newShape.transformShape(mat)
    return newShape, newShape.BoundBox


def separateFacesAndEdges(s):
    """Separate a shape into face compound and edge compound."""
    if s.isNull():
        return Part.makeCompound([]), Part.makeCompound([])

    faces = []
    edges = []
    for face in s.Faces:
        faces.append(face)
    for edge in s.Edges:
        edges.append(edge)

    faceCompound = Part.Compound(faces) if faces else Part.makeCompound([])
    edgeCompound = Part.Compound(edges) if edges else Part.makeCompound([])
    return faceCompound, edgeCompound


def shapeToEdges(shp):
    """Convert a shape to edges only (discard faces)."""
    edges = shp.Edges
    if not edges:
        return Part.makeCompound([])
    return Part.makeCompound(edges)


def clipShapeToBase(baseShape, repeatedShape, clipMode):
    """
    Clip a repeated shape against the base shape.
    
    Parameters
    ----------
    baseShape : Part.Shape
        The base shape to clip against.
    repeatedShape : Part.Shape
        The pattern/tile shape to be clipped.
    clipMode : str
        "BooleanOnly" - standard boolean common operation.
        "PreserveLinesNoClip" - keep edges even if faces are clipped.
    
    Returns
    -------
    Part.Shape
        The clipped shape.
    """
    if repeatedShape.isNull():
        return Part.makeCompound([])

    if clipMode == "BooleanOnly":
        try:
            return baseShape.common(repeatedShape)
        except:
            return Part.makeCompound([])
    elif clipMode == "PreserveLinesNoClip":
        faceShape, edgeShape = separateFacesAndEdges(repeatedShape)
        clippedFaces = Part.makeCompound([])
        if not faceShape.isNull():
            try:
                clippedFaces = baseShape.common(faceShape)
            except:
                clippedFaces = Part.makeCompound([])
        if edgeShape.isNull():
            finalShape = clippedFaces
        else:
            finalShape = Part.makeCompound([clippedFaces, edgeShape])
        return finalShape
    else:
        return Part.makeCompound([])


# ============================================================================
# Tile Creation and Placement
# ============================================================================

def makeTileAndClip(baseShape,
                    normedPattern,
                    tileX, tileY,
                    baseScale, baseRotRad,
                    randomize,
                    randOffRange,
                    randRotMin,
                    randRotMax,
                    randSmin,
                    randSmax,
                    showFaces,
                    shapeDistortion,
                    enableColorVar,
                    colorVarInt,
                    placement_mode="Center",
                    clipMode="BooleanOnly"):
    """
    Create a single tile instance, transform it, and clip it against the base shape.
    
    Returns
    -------
    tuple (Part.Shape or None, randomColor or None)
    """
    cpy = normedPattern.copy()
    bb = cpy.BoundBox
    w = bb.XLength
    h = bb.YLength

    # Apply scaling
    finalScale = baseScale
    if randomize and randSmax >= randSmin and randSmin > 0:
        scaleFactor = random.uniform(max(0.0001, randSmin), randSmax)
        finalScale *= scaleFactor

    matScale = FreeCAD.Matrix()
    matScale.scale(finalScale, finalScale, finalScale)
    cpy.transformShape(matScale)

    # Apply rotation
    finalRot = baseRotRad
    if randomize and randRotMax > randRotMin:
        finalRot += math.radians(random.uniform(randRotMin, randRotMax))

    matRot = FreeCAD.Matrix()
    matRot.rotateZ(finalRot)
    cpy.transformShape(matRot)

    # Apply placement mode (alignment within tile)
    if placement_mode == "Center":
        dx, dy = -w/2, -h/2
    elif placement_mode == "TopLeft":
        dx, dy = 0, -h
    elif placement_mode == "TopRight":
        dx, dy = -w, -h
    elif placement_mode == "BottomLeft":
        dx, dy = 0, 0
    elif placement_mode == "BottomRight":
        dx, dy = -w, 0
    elif placement_mode == "TopCenter":
        dx, dy = -w/2, -h
    elif placement_mode == "BottomCenter":
        dx, dy = -w/2, 0
    elif placement_mode == "LeftCenter":
        dx, dy = 0, -h/2
    elif placement_mode == "RightCenter":
        dx, dy = -w, -h/2
    else:  # Origin
        dx, dy = 0, 0

    mat_placement = FreeCAD.Matrix()
    mat_placement.move(FreeCAD.Vector(dx, dy, 0))
    cpy.transformShape(mat_placement)

    # Apply random jitter
    if randomize and randOffRange > 0:
        jitter_x = random.uniform(-randOffRange, randOffRange)
        jitter_y = random.uniform(-randOffRange, randOffRange)
        matTrans = FreeCAD.Matrix()
        matTrans.move(FreeCAD.Vector(jitter_x, jitter_y, 0))
        cpy.transformShape(matTrans)

    # Apply final translation to tile position
    matTrans = FreeCAD.Matrix()
    matTrans.move(FreeCAD.Vector(tileX, tileY, 0))
    cpy.transformShape(matTrans)

    # Convert to faces if requested
    if showFaces:
        try:
            faces = []
            for sub in cpy.SubShapes:
                if isinstance(sub, Part.Wire):
                    faces.append(Part.Face(sub))
                elif isinstance(sub, Part.Face):
                    faces.append(sub)
            if faces:
                cpy = Part.makeCompound(faces)
        except Exception as ex:
            FreeCAD.Console.PrintWarning(f"makeFace() failed: {str(ex)}\n")
    else:
        cpy = shapeToEdges(cpy)

    # Clip against base shape
    try:
        result = clipShapeToBase(baseShape, cpy, clipMode)
    except Exception as e:
        FreeCAD.Console.PrintError(f"Boolean operation error: {str(e)}\n")
        return None, None

    # Generate random color if variation enabled
    randomColor = None
    if enableColorVar:
        import colorsys
        hue = random.random()
        sat = 0.5 + 0.5 * colorVarInt
        val = 1.0
        randomColor = colorsys.hsv_to_rgb(hue, sat, val)

    return result, randomColor


# ============================================================================
# Main Hatch Building Function
# ============================================================================

def buildHatchShape(baseShape,
                    patternShape,
                    overrideBB=None,
                    distributionMode="SeamlessTiling",
                    autoScaleToFitBase=False,
                    patternScale=1.0,
                    rotationDeg=0.0,
                    baseSpacing=10.0,
                    repX=5,
                    repY=5,
                    randRotMin=0.0,
                    randRotMax=0.0,
                    randomizePlacement=False,
                    randomOffsetRange=0.0,
                    randomScaleMin=1.0,
                    randomScaleMax=1.0,
                    radialCount=8,
                    radialRadius=50.0,
                    concentricCount=5,
                    concentricSpacing=10.0,
                    randomCount=30,
                    offsetX=0.0,
                    offsetY=0.0,
                    scaleMode="Absolute",
                    tileShape=None,
                    tileVisibility=False,
                    showFaces=False,
                    maxTiles=1000,
                    densityFactor=1.0,
                    enableColorVar=False,
                    colorVarInt=0.5,
                    spacingVariation=0.0,
                    shapeDistortion=False,
                    apply3D=False,
                    placement_mode="Center",
                    clipMode="BooleanOnly"):
    """
    Generate a hatch pattern by tiling and clipping.
    
    Parameters
    ----------
    baseShape : Part.Shape
        The base shape to fill with the pattern.
    patternShape : Part.Shape
        The pattern/tile shape to repeat.
    overrideBB : Part.BoundBox, optional
        Override bounding box for grid calculation (used for multiple base shapes).
    distributionMode : str
        How to distribute tiles: "CenteredTiling", "RelativeSpacing", "SeamlessTiling",
        "LinearGrid", "RadialDistribution", "ConcentricDistribution", 
        "RandomDistribution", "AdaptiveDistribution".
    autoScaleToFitBase : bool
        Automatically scale pattern to fit base shape.
    patternScale : float
        Manual scale factor.
    rotationDeg : float
        Rotation angle in degrees.
    baseSpacing : float
        Spacing between tiles (mm or percentage).
    repX, repY : int
        Number of repetitions in X and Y directions.
    randRotMin, randRotMax : float
        Random rotation range in degrees.
    randomizePlacement : bool
        Apply random transforms to each tile.
    randomOffsetRange : float
        Maximum random offset in mm.
    randomScaleMin, randomScaleMax : float
        Random scale factor range.
    radialCount : int
        Number of copies for radial distribution.
    radialRadius : float
        Radius for radial distribution.
    concentricCount : int
        Number of rings for concentric distribution.
    concentricSpacing : float
        Spacing between rings.
    randomCount : int
        Number of random placements.
    offsetX, offsetY : float
        Offset for pattern placement.
    scaleMode : str
        "Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim".
    tileShape : Part.Shape, optional
        Optional tile shape for bounding box calculation.
    tileVisibility : bool
        Whether to generate separate tile objects.
    showFaces : bool
        Convert wires to faces where possible.
    maxTiles : int
        Maximum number of tiles to generate.
    densityFactor : float
        Density factor for random distribution (0-1).
    enableColorVar : bool
        Enable random color variation.
    colorVarInt : float
        Intensity of color variation (0-1).
    spacingVariation : float
        Random factor for spacing variation (0-1).
    shapeDistortion : bool
        Enable shape distortion (experimental).
    apply3D : bool
        Apply to 3D surfaces (placeholder).
    placement_mode : str
        How to align pattern within tile bounds.
    clipMode : str
        "BooleanOnly" or "PreserveLinesNoClip".
    
    Returns
    -------
    tuple (Part.Shape, int)
        The combined hatch shape and the number of tiles generated.
    """
    if baseShape.isNull() or patternShape.isNull():
        return Part.makeCompound([]), 0

    # Normalize pattern shape
    normedPattern, pBB = normalizePatternShape(patternShape)
    patternW = pBB.XMax - pBB.XMin
    patternH = pBB.YMax - pBB.YMin

    # Handle tile shape if provided
    if tileShape and not tileShape.isNull():
        normedTile, tileBB = normalizePatternShape(tileShape)
        if placement_mode != "Origin":
            tile_w = tileBB.XLength
            tile_h = tileBB.YLength
            if placement_mode == "Center":
                dx, dy = -tile_w/2, -tile_h/2
            elif placement_mode == "TopLeft":
                dx, dy = 0, -tile_h
            elif placement_mode == "TopRight":
                dx, dy = -tile_w, -tile_h
            elif placement_mode == "BottomLeft":
                dx, dy = 0, 0
            elif placement_mode == "BottomRight":
                dx, dy = -tile_w, 0
            elif placement_mode == "TopCenter":
                dx, dy = -tile_w/2, -tile_h
            elif placement_mode == "BottomCenter":
                dx, dy = -tile_w/2, 0
            elif placement_mode == "LeftCenter":
                dx, dy = 0, -tile_h/2
            elif placement_mode == "RightCenter":
                dx, dy = -tile_w, -tile_h/2
            else:
                dx, dy = 0, 0

            mat = FreeCAD.Matrix()
            mat.move(FreeCAD.Vector(dx, dy, 0))
            normedTile = normedTile.transformGeometry(mat)
            tileBB = normedTile.BoundBox
        tileW = tileBB.XLength
        tileH = tileBB.YLength
    else:
        tileW = patternW
        tileH = patternH

    # Get bounding box (use overrideBB if provided for unified grid across multiple base shapes)
    bBB = overrideBB if overrideBB is not None else baseShape.BoundBox
    w = bBB.XMax - bBB.XMin
    h = bBB.YMax - bBB.YMin

    # Apply scale mode
    if scaleMode == "FitWidth":
        if tileW > 1e-9:
            patternScale = max(0.0001, (w / tileW) * patternScale)
    elif scaleMode == "FitHeight":
        if tileH > 1e-9:
            patternScale = max(0.0001, (h / tileH) * patternScale)
    elif scaleMode == "FitMinDim":
        minDim = min(w, h)
        patDim = max(tileW, tileH)
        if patDim > 1e-9:
            patternScale = max(0.0001, (minDim / patDim) * patternScale)
    elif scaleMode == "FitMaxDim":
        maxDim = max(w, h)
        patDim = max(tileW, tileH)
        if patDim > 1e-9:
            patternScale = max(0.0001, (maxDim / patDim) * patternScale)

    # Auto scale to fit base
    if autoScaleToFitBase:
        if distributionMode in ["CenteredTiling", "RelativeSpacing"]:
            if tileW > 1e-9 and tileH > 1e-9 and repX > 0 and repY > 0:
                targetW = w / repX
                targetH = h / repY
                patternScale = min(patternScale, min(targetW/tileW, targetH/tileH))
        elif distributionMode == "SeamlessTiling":
            if tileW > 1e-9 and tileH > 1e-9 and repX > 0 and repY > 0:
                scaleX = w/(tileW*patternScale*repX)
                scaleY = h/(tileH*patternScale*repY)
                patternScale = min(patternScale, min(scaleX, scaleY))

    rotRad = math.radians(rotationDeg)
    patternList = []
    tileCount = 0

    # Helper for spacing variation
    def getSpacing(base, iterationX=0, iterationY=0):
        if spacingVariation <= 0.0:
            return base
        rand_factor = random.uniform(-base*spacingVariation, base*spacingVariation)
        return base + rand_factor

    # Helper to fuse shapes
    def fuseShapes(shapeList):
        if not shapeList:
            return Part.makeCompound([])
        fused = shapeList[0]
        for shp in shapeList[1:]:
            fused = fused.fuse(shp)
        if not fused.isValid():
            fused = fused.removeSplitter()
        return fused

    # Helper to create and clip a single tile
    def tileAndClip(tileX, tileY, baseScale, baseRotRad, placement_mode):
        nonlocal tileCount
        halfW = (tileW * baseScale) * 0.5
        halfH = (tileH * baseScale) * 0.5
        minx = tileX - halfW
        maxx = tileX + halfW
        miny = tileY - halfH
        maxy = tileY + halfH

        # Early exit if tile is completely outside base bounding box
        if (maxx < bBB.XMin or minx > bBB.XMax or 
            maxy < bBB.YMin or miny > bBB.YMax):
            return None, None

        shp, color = makeTileAndClip(
            baseShape,
            normedPattern,
            tileX, tileY,
            baseScale,
            baseRotRad,
            randomizePlacement,
            randomOffsetRange,
            randRotMin,
            randRotMax,
            randomScaleMin,
            randomScaleMax,
            showFaces,
            shapeDistortion,
            enableColorVar,
            colorVarInt,
            placement_mode=placement_mode,
            clipMode=clipMode
        )
        tileCount += 1
        return shp, color

    # Adjust random count based on density factor
    if distributionMode == "RandomDistribution":
        randomCount = int(randomCount * densityFactor)
        if randomCount < 1:
            randomCount = 1

    # ========================================================================
    # Distribution Modes
    # ========================================================================

    if distributionMode == "CenteredTiling":
        startX = (bBB.XMin + bBB.XMax) * 0.5
        startY = (bBB.YMin + bBB.YMax) * 0.5
        for ix in range(repX):
            for iy in range(repY):
                if tileCount >= maxTiles:
                    break
                spacingX = getSpacing(baseSpacing, ix, iy)
                spacingY = getSpacing(baseSpacing, ix, iy)
                x = startX - ((repX-1) * spacingX * 0.5) + ix * spacingX + offsetX
                y = startY - ((repY-1) * spacingY * 0.5) + iy * spacingY + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "RelativeSpacing":
        minDim = min(w, h)
        spacing = (baseSpacing / 100.0) * minDim
        for ix in range(repX):
            for iy in range(repY):
                if tileCount >= maxTiles:
                    break
                spacingX = getSpacing(spacing, ix, iy)
                x = (bBB.XMin + bBB.XMax) * 0.5 - ((repX-1) * spacingX * 0.5) + ix * spacingX + offsetX
                spacingY = getSpacing(spacing, ix, iy)
                y = (bBB.YMin + bBB.YMax) * 0.5 - ((repY-1) * spacingY * 0.5) + iy * spacingY + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "SeamlessTiling":
        if tileW < 1e-9 or tileH < 1e-9:
            return Part.makeCompound([]), 0
        neededX = max(1, int(math.ceil(w / (tileW * patternScale))))
        neededY = max(1, int(math.ceil(h / (tileH * patternScale))))
        for ix in range(neededX):
            for iy in range(neededY):
                if tileCount >= maxTiles:
                    break
                x = bBB.XMin + ix * (tileW * patternScale) + offsetX
                y = bBB.YMin + iy * (tileH * patternScale) + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "LinearGrid":
        stepX = baseSpacing if baseSpacing > 0 else 10
        stepY = baseSpacing if baseSpacing > 0 else 10
        for ix in range(repX):
            for iy in range(repY):
                if tileCount >= maxTiles:
                    break
                dx = getSpacing(stepX, ix, iy)
                dy = getSpacing(stepY, ix, iy)
                x = bBB.XMin + ix * dx + offsetX
                y = bBB.YMin + iy * dy + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "RadialDistribution":
        cx = 0.5 * (bBB.XMax + bBB.XMin) + offsetX
        cy = 0.5 * (bBB.YMax + bBB.YMin) + offsetY
        for i in range(radialCount):
            if tileCount >= maxTiles:
                break
            angle = i * (2.0 * math.pi / float(radialCount))
            x = cx + radialRadius * math.cos(angle)
            y = cy + radialRadius * math.sin(angle)
            shp, color = tileAndClip(x, y, patternScale, rotRad + angle, placement_mode)
            if shp and not shp.isNull():
                patternList.append(shp)

    elif distributionMode == "ConcentricDistribution":
        cx = 0.5 * (bBB.XMax + bBB.XMin) + offsetX
        cy = 0.5 * (bBB.YMax + bBB.YMin) + offsetY
        for ring in range(concentricCount):
            radius = (ring + 1) * concentricSpacing
            countRing = ring + 3
            for i in range(countRing):
                if tileCount >= maxTiles:
                    break
                angle = i * (2.0 * math.pi / float(countRing))
                x = cx + radius * math.cos(angle)
                y = cy + radius * math.sin(angle)
                shp, color = tileAndClip(x, y, patternScale, rotRad + angle, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "RandomDistribution":
        for i in range(randomCount):
            if tileCount >= maxTiles:
                break
            x = random.uniform(bBB.XMin, bBB.XMax) + offsetX
            y = random.uniform(bBB.YMin, bBB.YMax) + offsetY
            shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
            if shp and not shp.isNull():
                patternList.append(shp)

    elif distributionMode == "AdaptiveDistribution":
        perimeterPoints = []
        for wire in baseShape.Wires:
            pts = wire.discretize(25)
            perimeterPoints.extend(pts)
        if densityFactor < 1.0:
            keepCount = int(len(perimeterPoints) * densityFactor)
            random.shuffle(perimeterPoints)
            perimeterPoints = perimeterPoints[:keepCount]
        for pt in perimeterPoints:
            if tileCount >= maxTiles:
                break
            x = pt.x + offsetX
            y = pt.y + offsetY
            shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
            if shp and not shp.isNull():
                patternList.append(shp)

    # Fuse all tiles together
    finalShape = fuseShapes(patternList)
    return finalShape, tileCount