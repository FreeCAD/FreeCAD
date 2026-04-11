#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ============================================================================
# FreeCAD Hatch Generator Addon
# ============================================================================
# Filename: HatchGenerator.py
# Description: A parametric hatch generator for 3D surfaces, walls, and roofs.
# Author: Regis Benoit Brice Nde Tene
# Version: 2.0
# ============================================================================

import FreeCAD
import FreeCADGui
import Part
import math, random
from PySide import QtCore, QtGui, QtWidgets
import os
import json
import datetime

# ================================
# Helper Functions
# ================================
def convertBaseSpacingValue(spacingValue, useUnits, selectedUnitSystem):
    """
    Converts the user-entered spacingValue into millimeters (or an internal unit).
    If useUnits=False, just return spacingValue as-is.
    If useUnits=True, interpret based on selectedUnitSystem.
    """
    if not useUnits:
        return spacingValue

    if selectedUnitSystem == "FreeCAD Default":
        return spacingValue
    elif selectedUnitSystem == "Metric (m)":
        return spacingValue * 1000.0
    elif selectedUnitSystem == "Imperial (ft)":
        return spacingValue * 304.8
    elif selectedUnitSystem == "BIM Workbench Unit":
        return spacingValue * 25.4
    else:
        return spacingValue

def getFaceLocalFrame(face):
    """
    Returns (origin, x_axis, y_axis, normal) for the given face.
    These are all FreeCAD.Vector in world coordinates.
    """
    # Surface normal at parametric center
    try:
        normal = face.normalAt(0, 0).normalize()
    except:
        # Fallback for faces that might not have proper parameterization
        normal = face.normalAt(face.Surface.parameter(face.CenterOfMass)[0], 
                               face.Surface.parameter(face.CenterOfMass)[1]).normalize()

    # Pick a reference up-vector that is not parallel to normal
    up = FreeCAD.Vector(0, 0, 1)
    if abs(normal.dot(up)) > 0.99:
        up = FreeCAD.Vector(0, 1, 0)

    # Build orthonormal local frame
    x_axis = up.cross(normal).normalize()
    y_axis = normal.cross(x_axis).normalize()

    # Origin = center of mass of the face
    origin = face.CenterOfMass

    return origin, x_axis, y_axis, normal

def buildSurfaceTransforms(face):
    """
    Returns a FreeCAD.Matrix that transforms world XYZ → local UV (2D on face),
    and its inverse that transforms local UV → world XYZ.
    """
    origin, x_axis, y_axis, normal = getFaceLocalFrame(face)

    # world_to_local: rotate world coords so that x_axis→X, y_axis→Y, normal→Z
    # then subtract origin
    to_local = FreeCAD.Matrix()
    # Column vectors of the rotation are the local axes
    to_local.A11 = x_axis.x;  to_local.A12 = x_axis.y;  to_local.A13 = x_axis.z
    to_local.A21 = y_axis.x;  to_local.A22 = y_axis.y;  to_local.A23 = y_axis.z
    to_local.A31 = normal.x;  to_local.A32 = normal.y;  to_local.A33 = normal.z
    
    # Create translation matrix
    translation = FreeCAD.Matrix()
    translation.move(FreeCAD.Vector(-origin.x, -origin.y, -origin.z))
    
    # Combine: first translate, then rotate
    to_local = to_local.multiply(translation)

    # local_to_world is the inverse
    to_world = to_local.inverse()

    return to_local, to_world

def projectShapeToLocal(shape, to_local_matrix):
    """Transform shape into the face's local coordinate system."""
    projected = shape.copy()
    projected.transformShape(to_local_matrix)
    return projected

def unprojectShapeToWorld(shape, to_world_matrix):
    """Transform shape from local back to world coordinates."""
    worldShape = shape.copy()
    worldShape.transformShape(to_world_matrix)
    return worldShape

def getShapeFaceNormal(shape):
    """Get the normal of the first face in a shape, or None if no faces."""
    if shape and shape.Faces:
        try:
            return shape.Faces[0].normalAt(0, 0).normalize()
        except:
            pass
    return None

def applyTileViewOverrides(tileObj, vp, overrideColor=None):
    """
    Ensures the tile rep is displayed in a certain style
    without triggering recomputes.
    """
    if not tileObj or not hasattr(tileObj, 'ViewObject'):
        return

    # If overrideColor was passed in, use it; otherwise use the tileObj's property
    if overrideColor is not None:
        color = overrideColor
    else:
        # If the tile's parent is our hatch object, we can read .TileRepColor
        hatchObj = tileObj.Document.getObject(tileObj.Name.replace("_TileRep",""))
        if hatchObj and hasattr(hatchObj, "TileRepColor"):
            color = hatchObj.TileRepColor
        else:
            color = (1.0, 0.0, 0.0)  # fallback color

    vp.ShapeColor = color
    vp.DisplayMode = "Wireframe"  # or "Flat Lines"
    vp.LineWidth = 2.0

def safeSetDisplayMode(vobj, desiredMode="Flat Lines"):
    modes = []
    try:
        modes = vobj.getDisplayModes()
    except Exception:
        pass
    if desiredMode in modes:
        vobj.DisplayMode = desiredMode
    elif modes:
        vobj.DisplayMode = modes[0]

def makeRectangle(width, height):
    """Creates a Face covering 'width' x 'height' in the XY plane."""
    p0 = FreeCAD.Vector(0,0,0)
    p1 = FreeCAD.Vector(width,0,0)
    p2 = FreeCAD.Vector(width,height,0)
    p3 = FreeCAD.Vector(0,height,0)
    poly = Part.makePolygon([p0, p1, p2, p3, p0])
    face = Part.Face(poly)
    return face

# ================================
# Built-in Patterns
# ================================

def generateBuiltInPatternShape(patternType):
    """
    Generate built-in patterns for demonstration.
    """
    grid_size = 10
    hex_y     = math.sqrt(3)
    tile_size = 2.5

    baseRect = makeRectangle(10, 10)

    if patternType == "SolidFill":
        return baseRect

    elif patternType == "HorizontalLines":
        comp = []
        for y in range(0, grid_size+1, 2):
            comp.append(Part.makeLine(
                FreeCAD.Vector(0, y, 0),
                FreeCAD.Vector(grid_size, y, 0)
            ))
        return Part.makeCompound(comp)

    elif patternType == "VerticalLines":
        comp = []
        for x in range(0, grid_size+1, 2):
            comp.append(Part.makeLine(
                FreeCAD.Vector(x, 0, 0),
                FreeCAD.Vector(x, grid_size, 0)
            ))
        return Part.makeCompound(comp)

    elif patternType == "Crosshatch":
        horiz = generateBuiltInPatternShape("HorizontalLines")
        vert  = generateBuiltInPatternShape("VerticalLines")
        return horiz.fuse(vert)

    elif patternType == "Herringbone":
        comp = []
        for row in range(0, grid_size, 2):
            line1 = Part.makeLine(
                FreeCAD.Vector(0, row, 0),
                FreeCAD.Vector(grid_size, row+2, 0)
            )
            line2 = Part.makeLine(
                FreeCAD.Vector(grid_size, row, 0),
                FreeCAD.Vector(0, row+2, 0)
            )
            comp.extend([line1, line2])
        return Part.makeCompound(comp)

    elif patternType == "BrickPattern":
        comp = []
        for y in [0, 5, 10]:
            comp.append(Part.makeLine(
                FreeCAD.Vector(0,y,0),
                FreeCAD.Vector(grid_size,y,0)
            ))
        for x in [0,5,10]:
            comp.append(Part.makeLine(
                FreeCAD.Vector(x,0,0),
                FreeCAD.Vector(x,5,0)
            ))
        for x in [2.5,7.5]:
            comp.append(Part.makeLine(
                FreeCAD.Vector(x,5,0),
                FreeCAD.Vector(x,10,0)
            ))
        return Part.makeCompound(comp)

    elif patternType == "RandomDots":
        comp = []
        for i in range(10):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            comp.append(Part.makeCircle(0.5, FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(comp)

    elif patternType == "OverlappingSquares":
        comp = []
        tile_size_int = int(tile_size)
        for x in range(0, grid_size, tile_size_int):
            for y in range(0, grid_size, tile_size_int):
                if ((x//tile_size_int) + (y//tile_size_int)) % 2 == 0:
                    square = makeRectangle(tile_size, tile_size)
                    mat = FreeCAD.Matrix()
                    mat.move(FreeCAD.Vector(x, y, 0))
                    shifted = square.copy()
                    shifted.transformShape(mat)
                    comp.append(shifted)
        return Part.makeCompound(comp)

    elif patternType == "Checkerboard":
        comp = []
        tile_size = float(tile_size)
        num_tiles = int(grid_size / tile_size)

        centered = True

        for row in range(num_tiles):
            for col in range(num_tiles):
                if (row + col) % 2 == 0:
                    square = makeRectangle(tile_size, tile_size)
                    placement = FreeCAD.Placement()
                    if centered:
                        x_shift = col * tile_size + tile_size / 2
                        y_shift = row * tile_size + tile_size / 2
                    else:
                        x_shift = col * tile_size
                        y_shift = row * tile_size
                    placement.move(FreeCAD.Vector(x_shift, y_shift, 0))
                    square.Placement = placement
                    comp.append(square)
        return Part.makeCompound(comp)

    elif patternType == "CheckerboardCircles":
        comp = []
        tile_size = 3.0
        for x in range(0, grid_size, int(tile_size)):
            for y in range(0, grid_size, int(tile_size)):
                if (x//tile_size + y//tile_size) % 2 == 0:
                    comp.append(Part.makeCircle(tile_size/3, 
                        FreeCAD.Vector(x + tile_size/2, y + tile_size/2, 0)))
        return Part.makeCompound(comp)

    elif patternType == "RotatingHexagons":
        def create_hexagon(center, size, rotation):
            points = []
            for i in range(7):
                angle = math.radians(60 * i + rotation)
                point = center + FreeCAD.Vector(
                    size * math.cos(angle),
                    size * math.sin(angle),
                    0
                )
                points.append(point)
            return Part.makePolygon(points)
        
        shapes = []
        spacing = grid_size/6
        size = spacing/2
        
        for row in range(7):
            offset = spacing/2 if row % 2 else 0
            for col in range(7):
                center = FreeCAD.Vector(
                    col * spacing + offset,
                    row * spacing * 0.866,
                    0
                )
                rotation = 30 * ((row + col) % 4)
                shapes.append(create_hexagon(center, size, rotation))
        
        return Part.makeCompound(shapes)

    elif patternType == "NestedTriangles":
        def create_nested_triangle(center, size, levels):
            shapes = []
            for i in range(levels):
                scale = 1 - (i * 0.25)
                curr_size = size * scale
                points = []
                for j in range(4):
                    angle = math.radians(120 * j + 30)
                    point = center + FreeCAD.Vector(
                        curr_size * math.cos(angle),
                        curr_size * math.sin(angle),
                        0
                    )
                    points.append(point)
                shapes.append(Part.makePolygon(points))
            return shapes
        
        patterns = []
        spacing = grid_size/4
        size = spacing/2
        
        for row in range(5):
            offset = spacing/2 if row % 2 else 0
            for col in range(5):
                center = FreeCAD.Vector(
                    col * spacing + offset,
                    row * spacing,
                    0
                )
                patterns.extend(create_nested_triangle(center, size, 3))
        
        return Part.makeCompound(patterns)

    elif patternType == "InterlockingCircles":
        def create_circle_pattern(center, radius):
            shapes = []
            points = []
            steps = 32
            
            # Main circle
            for i in range(steps + 1):
                angle = 2 * math.pi * i / steps
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                )
                points.append(point)
            shapes.append(Part.makePolygon(points))
            
            # Intersecting arcs
            for i in range(6):
                arc_points = []
                angle_offset = i * math.pi/3
                arc_center = center + FreeCAD.Vector(
                    radius * math.cos(angle_offset),
                    radius * math.sin(angle_offset),
                    0
                )
                
                for j in range(steps//3 + 1):
                    angle = angle_offset + math.pi/2 + (math.pi * j / (steps//3))
                    point = arc_center + FreeCAD.Vector(
                        radius * math.cos(angle),
                        radius * math.sin(angle),
                        0
                    )
                    arc_points.append(point)
                shapes.append(Part.makePolygon(arc_points))
            
            return shapes
        
        patterns = []
        spacing = grid_size/3
        radius = spacing/2
        
        for row in range(4):
            offset = spacing/2 if row % 2 else 0
            for col in range(4):
                center = FreeCAD.Vector(
                    col * spacing + offset,
                    row * spacing,
                    0
                )
                patterns.extend(create_circle_pattern(center, radius))
        
        return Part.makeCompound(patterns)

    elif patternType == "RecursiveSquares":
        def create_recursive_square(center, size, depth):
            if depth == 0:
                return []
                
            shapes = []
            points = []
            
            # Create main square
            for i in range(5):
                angle = math.radians(90 * i + 45)
                point = center + FreeCAD.Vector(
                    size * math.cos(angle),
                    size * math.sin(angle),
                    0
                )
                points.append(point)
            shapes.append(Part.makePolygon(points))
            
            # Create four smaller squares
            new_size = size * 0.4
            for i in range(4):
                angle = math.radians(90 * i + 45)
                new_center = center + FreeCAD.Vector(
                    size * 0.7 * math.cos(angle),
                    size * 0.7 * math.sin(angle),
                    0
                )
                shapes.extend(create_recursive_square(new_center, new_size, depth-1))
            
            return shapes
        
        patterns = []
        spacing = grid_size/3
        size = spacing/2
        
        for row in range(3):
            for col in range(3):
                center = FreeCAD.Vector(
                    col * spacing + spacing/2,
                    row * spacing + spacing/2,
                    0
                )
                patterns.extend(create_recursive_square(center, size, 3))
        
        return Part.makeCompound(patterns)

    elif patternType == "FlowerOfLife":
        def create_flower_circle(center, radius, num_petals):
            shapes = []
            
            # Center circle
            points = []
            steps = 32
            for i in range(steps + 1):
                angle = 2 * math.pi * i / steps
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                )
                points.append(point)
            shapes.append(Part.makePolygon(points))
            
            # Surrounding circles
            for i in range(num_petals):
                petal_points = []
                angle = 2 * math.pi * i / num_petals
                petal_center = center + FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                )
                
                for j in range(steps + 1):
                    angle = 2 * math.pi * j / steps
                    point = petal_center + FreeCAD.Vector(
                        radius * math.cos(angle),
                        radius * math.sin(angle),
                        0
                    )
                    petal_points.append(point)
                shapes.append(Part.makePolygon(petal_points))
            
            return shapes
        
        patterns = []
        radius = grid_size/8
        levels = 3
        
        # Create multiple levels of the flower pattern
        for level in range(levels):
            center = FreeCAD.Vector(grid_size/2, grid_size/2, 0)
            patterns.extend(create_flower_circle(center, radius * (level + 1), 6 * (level + 1)))
        
        return Part.makeCompound(patterns)

    elif patternType == "VoronoiMesh":
        def create_cell(center, points):
            # Create Voronoi cell boundary
            cell_points = []
            num_segments = len(points)
            
            for i in range(num_segments):
                p1 = points[i]
                p2 = points[(i + 1) % num_segments]
                mid = (p1 + p2) * 0.5
                
                # Calculate perpendicular bisector
                diff = p2 - p1
                perp = FreeCAD.Vector(-diff.y, diff.x, 0).normalize()
                
                cell_points.append(mid + perp * grid_size/4)
                cell_points.append(mid - perp * grid_size/4)
            
            return Part.makePolygon(cell_points)
        
        cells = []
        num_points = 12
        points = []
        
        # Generate random points
        for _ in range(num_points):
            point = FreeCAD.Vector(
                random.uniform(grid_size/4, 3*grid_size/4),
                random.uniform(grid_size/4, 3*grid_size/4),
                0
            )
            points.append(point)
        
        # Create Voronoi cells
        for i, center in enumerate(points):
            nearby_points = sorted(points, 
                                key=lambda p: (p - center).Length)[1:6]
            cells.append(create_cell(center, nearby_points))
        
        return Part.makeCompound(cells)

    elif patternType == "OffsetChecker":
        comp = []
        tile_size = 2.5
        offset = False
        for y in range(0, grid_size*2, int(tile_size)):
            offset = not offset
            for x in range(-int(tile_size), grid_size*2, int(tile_size)):
                shift = tile_size/2 if offset else 0
                if (x//tile_size + y//tile_size) % 2 == 0:
                    square = makeRectangle(tile_size, tile_size)
                    square.translate(FreeCAD.Vector(x + shift, y - shift, 0))
                    comp.append(square)
        return Part.makeCompound(comp)

    elif patternType == "ZigZag":
        comp = []
        for y in range(0, grid_size+1, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(
                    FreeCAD.Vector(x,   y,   0),
                    FreeCAD.Vector(x+2, y+2, 0)
                )
                line2 = Part.makeLine(
                    FreeCAD.Vector(x+2, y+2, 0),
                    FreeCAD.Vector(x+4, y,   0)
                )
                comp.extend([line1, line2])
        return Part.makeCompound(comp)

    elif patternType == "HexagonalHoriz":
        hex_size = 2.0
        points = []
        for angle in range(30, 390, 60):
            rad = math.radians(angle)
            px = hex_size * math.cos(rad)
            py = hex_size * math.sin(rad)
            points.append(FreeCAD.Vector(px, py, 0))
        points.append(points[0])
        wire = Part.makePolygon(points)
        try:
            face = Part.Face(wire)
            return face
        except:
            return Part.makeCompound([])

    elif patternType == "HexagonalVerti":
        hex_size = 2.0
        points = []
        for angle in range(0, 360, 60):
            rad = math.radians(angle)
            px = hex_size * math.cos(rad)
            py = hex_size * math.sin(rad)
            points.append(FreeCAD.Vector(px, py, 0))
        points.append(points[0])
        wire = Part.makePolygon(points)
        try:
            face = Part.Face(wire)
            return face
        except:
            return Part.makeCompound([])

    elif patternType == "HexagonalPattern":
        hex_size = 2.0
        points = []
        for angle in range(30, 390, 60):
            rad = math.radians(angle)
            px = hex_size * math.cos(rad)
            py = hex_size * math.sin(rad)
            points.append(FreeCAD.Vector(px, py, 0))
        points.append(points[0])
        wire = Part.makePolygon(points)
        try:
            face = Part.Face(wire)
            return face
        except:
            return Part.makeCompound([])

    elif patternType == "TrianglesGrid":
        comp = []
        for y in range(0, grid_size+1, 3):
            for x in range(0, grid_size+1, 3):
                tri1 = Part.makePolygon([
                    FreeCAD.Vector(x,     y,   0),
                    FreeCAD.Vector(x+3,   y,   0),
                    FreeCAD.Vector(x+1.5, y+hex_y, 0),
                    FreeCAD.Vector(x,     y,   0)
                ])
                tri2 = Part.makePolygon([
                    FreeCAD.Vector(x+3,   y,   0),
                    FreeCAD.Vector(x+1.5, y+hex_y, 0),
                    FreeCAD.Vector(x+3,   y+hex_y*2, 0),
                    FreeCAD.Vector(x+3,   y,   0)
                ])
                comp.extend([tri1, tri2])
        return Part.makeCompound(comp)

    elif patternType == "MidEastMosaic":
        comp = []
        triangle_size = 3.0
        triangle_height = (math.sqrt(3)/2) * triangle_size
        x_step = triangle_size
        y_step = triangle_height
        y = 0.0
        row = 0
        while y <= grid_size:
            if row % 2 == 1:
                x_offset = triangle_size/2
            else:
                x_offset = 0.0
            x = x_offset
            while x <= grid_size:
                points_up = [
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x + (triangle_size / 2), y + triangle_height, 0),
                    FreeCAD.Vector(x - (triangle_size / 2), y + triangle_height, 0),
                    FreeCAD.Vector(x, y, 0)
                ]
                try:
                    wire_up = Part.makePolygon(points_up)
                    face_up = Part.Face(wire_up)
                    comp.append(face_up)
                except:
                    pass

                points_down = [
                    FreeCAD.Vector(x, y + triangle_height, 0),
                    FreeCAD.Vector(x + (triangle_size / 2), y, 0),
                    FreeCAD.Vector(x - (triangle_size / 2), y, 0),
                    FreeCAD.Vector(x, y + triangle_height, 0)
                ]
                try:
                    wire_down = Part.makePolygon(points_down)
                    face_down = Part.Face(wire_down)
                    comp.append(face_down)
                except:
                    pass

                x += x_step
            y += y_step
            row += 1
        return Part.makeCompound(comp)

    elif patternType == "StarGridPattern":
        comp = []
        star_size = 3.0
        spacing_x = 10.0
        spacing_y = 10.0
        rotation_angle = 0
        rows = int(grid_size / spacing_y) + 1
        cols = int(grid_size / spacing_x) + 1
        for row in range(rows):
            y = row * spacing_y
            for col in range(cols):
                x = col * spacing_x
                points = []
                for i in range(5):
                    angle_deg = rotation_angle + i * 72
                    angle_rad = math.radians(angle_deg)
                    outer_x = x + star_size * math.cos(angle_rad)
                    outer_y = y + star_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(outer_x, outer_y, 0))
                    angle_deg = rotation_angle + (i * 72) + 36
                    angle_rad = math.radians(angle_deg)
                    inner_size = star_size / 2
                    inner_x = x + inner_size * math.cos(angle_rad)
                    inner_y = y + inner_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(inner_x, inner_y, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    comp.append(face)
                except:
                    pass
        return Part.makeCompound(comp)

    elif patternType == "BasketWeave":
        comp = []
        for x in range(0, grid_size, 2):
            for y in range(0, grid_size, 4):
                line1 = Part.makeLine(
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x, y+2, 0)
                )
                line2 = Part.makeLine(
                    FreeCAD.Vector(x+2, y+2, 0),
                    FreeCAD.Vector(x+2, y+4, 0)
                )
                comp.extend([line1, line2])
        for y in range(0, grid_size, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x+2, y, 0)
                )
                line2 = Part.makeLine(
                    FreeCAD.Vector(x+2, y+2, 0),
                    FreeCAD.Vector(x+4, y+2, 0)
                )
                comp.extend([line1, line2])
        return Part.makeCompound(comp)

    elif patternType == "Honeycomb":
        comp = []
        hex_size = 1.5
        hex_height = math.sqrt(3) * hex_size
        x_step = 3 * hex_size
        y_step = hex_height
        y = 0.0
        while y <= grid_size:
            row_number = int(y / y_step)
            if row_number % 2 == 0:
                x_offset = 1.5 * hex_size
            else:
                x_offset = 0.0
            x = 0.0
            while x <= grid_size:
                center_x = x + x_offset
                center_y = y
                points = []
                for angle_deg in range(0, 360, 60):
                    angle_rad = math.radians(angle_deg)
                    px = center_x + hex_size * math.cos(angle_rad)
                    py = center_y + hex_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    comp.append(face)
                except:
                    pass
                x += x_step
            y += y_step
        return Part.makeCompound(comp)

    elif patternType == "SineWave":
        comp = []
        amp = 1.5
        freq = 2
        points = []
        for x in range(0, 100):
            x_val = x/10.0
            y_val = amp * math.sin(math.radians(x_val * freq * 36))
            points.append(FreeCAD.Vector(x_val, grid_size/2 + y_val, 0))
        comp.append(Part.makePolygon(points))
        return Part.makeCompound(comp)

    elif patternType == "SpaceFrame":
        comp = []
        node_dist = 2.5
        for x in range(0, grid_size, int(node_dist)):
            for y in range(0, grid_size, int(node_dist)):
                # Connect to all neighbors
                for dx in [-node_dist, 0, node_dist]:
                    for dy in [-node_dist, 0, node_dist]:
                        if dx == dy == 0: continue
                        if 0 <= x+dx <= grid_size and 0 <= y+dy <= grid_size:
                            comp.append(Part.makeLine(
                                FreeCAD.Vector(x,y,0),
                                FreeCAD.Vector(x+dx,y+dy,0)
                            ))
        return Part.makeCompound(comp)

    elif patternType == "HoneycombDual":
        comp = []  # List to store individual components of the pattern
        hex_size = 1.2  # Size of each hexagon
        hex_height = math.sqrt(3) * hex_size  # Height of the hexagon

        # Generate the Honeycomb Dual pattern by iterating over the grid
        for y in range(0, grid_size * 2, int(hex_height)):  # Iterate over rows
            x_offset = int(hex_size) if (y // int(hex_height)) % 2 else 0  # Offset for staggered rows
            for x in range(x_offset, grid_size * 2, int(hex_size * 2)):  # Iterate over columns
                # Outer hexagon
                points = []
                for angle in range(0, 360, 60):  # Generate 6 vertices of the hexagon
                    rad = math.radians(angle + 30)  # Rotate by 30 degrees for proper alignment
                    px = x + hex_size * math.cos(rad)
                    py = y + hex_size * math.sin(rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])  # Close the polygon by repeating the first point
                comp.append(Part.makePolygon(points))

                # Inner circle
                comp.append(Part.makeCircle(hex_size / 2, FreeCAD.Vector(x, y, 0)))

        # Combine all components into a single compound object and return it
        return Part.makeCompound(comp)

    elif patternType == "ArtDeco":
        comp = []
        unit = 3.0
        for x in range(0, grid_size, int(unit)):
            for y in range(0, grid_size, int(unit)):
                # Chevron pattern
                comp.append(Part.makePolygon([
                    FreeCAD.Vector(x,y,0),
                    FreeCAD.Vector(x+unit,y+unit,0),
                    FreeCAD.Vector(x,y+unit*2,0),
                    FreeCAD.Vector(x,y,0)
                ]))
                # Sun rays
                for a in range(0, 360, 45):
                    rad = math.radians(a)
                    comp.append(Part.makeLine(
                        FreeCAD.Vector(x+unit/2,y+unit/2,0),
                        FreeCAD.Vector(x+unit/2 + math.cos(rad)*unit/2,
                                    y+unit/2 + math.sin(rad)*unit/2,0)
                    ))
        return Part.makeCompound(comp)

    elif patternType == "StainedGlass":
        comp = []
        tile = 2.5
        for x in range(0, grid_size, int(tile)):
            for y in range(0, grid_size, int(tile)):
                # Leading lines
                comp.append(Part.makeLine(
                    FreeCAD.Vector(x+tile/2,y,0),
                    FreeCAD.Vector(x+tile/2,y+tile,0)
                ))
                comp.append(Part.makeLine(
                    FreeCAD.Vector(x,y+tile/2,0),
                    FreeCAD.Vector(x+tile,y+tile/2,0)
                ))
                # Circular elements
                comp.append(Part.makeCircle(tile/3, 
                    FreeCAD.Vector(x+tile/2,y+tile/2,0)))
        return Part.makeCompound(comp)

    elif patternType == "PenroseTriangle":
        comp = []
        size = 2.0
        for i in range(0, grid_size, int(size*2)):
            # Base triangle
            points = [
                FreeCAD.Vector(i,0,0),
                FreeCAD.Vector(i+size,0,0),
                FreeCAD.Vector(i+size/2,size*math.sqrt(3)/2,0)
            ]
            comp.append(Part.makePolygon(points + [points[0]]))
            # Inverse triangle
            points = [
                FreeCAD.Vector(i+size/2,size*math.sqrt(3)/2,0),
                FreeCAD.Vector(i+size,0,0),
                FreeCAD.Vector(i+size*1.5,size*math.sqrt(3)/2,0)
            ]
            comp.append(Part.makePolygon(points + [points[0]]))
        return Part.makeCompound(comp)

    elif patternType == "GreekKey":
        comp = []  # List to store individual components of the pattern
        unit_size = 2.0  # Base unit size for the Greek Key pattern

        # Generate the Greek Key pattern by iterating over the grid
        for x in range(0, grid_size, int(unit_size * 2)):
            for y in range(0, grid_size, int(unit_size * 2)):
                # Define the points for the Greek Key motif
                points = [
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x + unit_size, y, 0),
                    FreeCAD.Vector(x + unit_size, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 0.75, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 0.75, y + unit_size * 0.75, 0),
                    FreeCAD.Vector(x + unit_size * 1.25, y + unit_size * 0.75, 0),
                    FreeCAD.Vector(x + unit_size * 1.25, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 1.5, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 1.5, y, 0),
                    FreeCAD.Vector(x + unit_size * 2, y, 0)
                ]
                # Create a polygon from the points and add it to the component list
                comp.append(Part.makePolygon(points))

        # Combine all components into a single compound object and return it
        return Part.makeCompound(comp)

    elif patternType == "ChainLinks":
        comp = []  # List to store individual components of the pattern
        link_width = 1.5  # Width of each chain link

        # Generate the Chain Links pattern by iterating over the grid
        for y in range(0, grid_size, int(link_width * 2)):
            for x in range(0, grid_size, int(link_width * 3)):
                # Create a vertical link as a circle
                comp.append(
                    Part.makeCircle(
                        link_width / 2,
                        FreeCAD.Vector(x + link_width * 1.5, y + link_width, 0)
                    )
                )
                # Create a horizontal link as a circle
                comp.append(
                    Part.makeCircle(
                        link_width / 2,
                        FreeCAD.Vector(x + link_width * 3, y + link_width * 2, 0)
                    )
                )

        # Combine all components into a single compound object and return it
        return Part.makeCompound(comp)

    elif patternType == "TriangleForest":
        comp = []
        chevron_width = 3.0
        chevron_height = 3.0
        spacing_x = chevron_width * 2
        spacing_y = chevron_height
        rows = int(grid_size / spacing_y) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 0:
                x_offset = chevron_width
            else:
                x_offset = 0.0
            cols = int(grid_size / spacing_x) + 2
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = [
                    FreeCAD.Vector(x - chevron_width / 2, y, 0),
                    FreeCAD.Vector(x + chevron_width / 2, y, 0),
                    FreeCAD.Vector(x, y + chevron_height, 0),
                    FreeCAD.Vector(x - chevron_width / 2, y, 0)
                ]
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    comp.append(face)
                except:
                    pass
        return Part.makeCompound(comp)

    elif patternType == "CeramicTile":
        comp = []
        hex_size = 1.0
        hex_height = math.sqrt(3) * hex_size
        spacing_x = 3 * hex_size
        spacing_y = hex_height
        rows = int(grid_size / spacing_y) + 2
        cols = int(grid_size / spacing_x) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 1:
                x_offset = 1.5 * hex_size
            else:
                x_offset = 0.0
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = []
                for angle_deg in range(0, 360, 60):
                    angle_rad = math.radians(angle_deg)
                    px = x + hex_size * math.cos(angle_rad)
                    py = y + hex_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    comp.append(face)
                except:
                    pass
        return Part.makeCompound(comp)

    elif patternType == "CirclesGrid":
        comp = []
        for x in range(1, grid_size, 3):
            for y in range(1, grid_size, 3):
                comp.append(Part.makeCircle(1.0, FreeCAD.Vector(x,y,0)))
        return Part.makeCompound(comp)

    elif patternType == "PlusSigns":
        comp = []
        for x in range(1, grid_size, 3):
            for y in range(1, grid_size, 3):
                hline = Part.makeLine(
                    FreeCAD.Vector(x-0.5, y, 0),
                    FreeCAD.Vector(x+0.5, y, 0)
                )
                vline = Part.makeLine(
                    FreeCAD.Vector(x, y-0.5, 0),
                    FreeCAD.Vector(x, y+0.5, 0)
                )
                comp.extend([hline, vline])
        return Part.makeCompound(comp)

    elif patternType == "WavesPattern":
        comp = []
        amplitude = 1.0
        wavelength = 10.0
        frequency = 1.0
        wave_spacing = 5.0
        points_per_wave = 200
        num_waves = int(grid_size / wave_spacing) + 2
        for wave_num in range(num_waves):
            y_base = wave_num * wave_spacing
            points = []
            for i in range(points_per_wave + 1):
                x = i * (wavelength / points_per_wave)
                y = y_base + amplitude * math.sin(2 * math.pi * frequency * x / wavelength)
                points.append(FreeCAD.Vector(x, y, 0))
            try:
                wire = Part.makePolygon(points)
                comp.append(wire)
            except:
                pass
        return Part.makeCompound(comp)

    elif patternType == "GalaxyStarsPattern":
        comp = []
        star_size = 1.0
        inner_ratio = 0.5
        spacing_x = 3.0
        spacing_y = 3.0
        rotation_angle = 0
        inner_radius = star_size * inner_ratio
        rows = int(grid_size / spacing_y) + 2
        cols = int(grid_size / spacing_x) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 1:
                x_offset = spacing_x / 2
            else:
                x_offset = 0.0
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = []
                for i in range(10):
                    angle_deg = rotation_angle + i * 36
                    angle_rad = math.radians(angle_deg)
                    if i % 2 == 0:
                        px = x + star_size * math.cos(angle_rad)
                        py = y + star_size * math.sin(angle_rad)
                    else:
                        px = x + inner_radius * math.cos(angle_rad)
                        py = y + inner_radius * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    comp.append(face)
                except:
                    pass
        return Part.makeCompound(comp)

    elif patternType == "GridDots":
        comp = []
        for x in range(1, grid_size, 2):
            for y in range(1, grid_size, 2):
                comp.append(Part.makeCircle(0.2, FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(comp)
    
    elif patternType == "InterlockingCircles":
        comp = []
        diameter = 3.0
        spacing = diameter * 0.25
        for y in range(0, grid_size*2, int(diameter + spacing)):
            for x in range(0, grid_size*2, int(diameter + spacing)):
                center = FreeCAD.Vector(x + diameter/2, y + diameter/2, 0)
                comp.append(Part.makeCircle(diameter/2, center))
                comp.append(Part.makeCircle(diameter/4, center))
        return Part.makeCompound(comp)

    elif patternType == "HexDots":
        # Parameters for the hexagonal dot pattern
        comp = []  # Compound list to store all circles
        hex_size = 1.5  # Size of the hexagon (distance between adjacent dots)
        dot_radius = hex_size / 3  # Radius of each dot
        hex_height = math.sqrt(3) * hex_size  # Height of the hexagon

        # Generate dots in a hexagonal grid
        for row in range(int(math.ceil(grid_size / hex_height)) + 1):  # Rows based on hex height
            y = row * hex_height  # Vertical position of the row
            x_offset = hex_size if row % 2 == 1 else 0  # Stagger rows alternately

            for col in range(int(math.ceil(grid_size / (hex_size * 2))) + 1):  # Columns based on hex width
                x = col * hex_size * 2 + x_offset  # Horizontal position of the dot
                if x < grid_size and y < grid_size:  # Ensure dots stay within bounds
                    # Create a circle at the calculated position
                    comp.append(Part.makeCircle(dot_radius, FreeCAD.Vector(x, y, 0)))

        return Part.makeCompound(comp)

    elif patternType == "FractalTree":
        comp = []

        def draw_branch(start, direction, depth):
            if depth > 5:  # Increase depth limit for better visualization
                return
            end = start + direction
            comp.append(Part.makeLine(start, end))

            # Define angles for branching
            for ang in [-30, 0, 30]:  # Angles in degrees
                # Scale the branch length
                new_dir = direction * 0.7

                # Rotate the vector using a rotation matrix
                rot_angle = math.radians(ang)  # Convert angle to radians
                rotation_matrix = FreeCAD.Matrix()
                rotation_matrix.rotateZ(rot_angle)
                new_dir = rotation_matrix.multVec(new_dir)

                # Recursive call for the next branch
                draw_branch(end, new_dir, depth + 1)

        # Start the fractal tree from the center bottom of the grid
        start_point = FreeCAD.Vector(grid_size / 2, 0, 0)
        initial_direction = FreeCAD.Vector(0, 3, 0)  # Initial branch direction (upward)
        draw_branch(start_point, initial_direction, 0)

        return Part.makeCompound(comp)

    elif patternType == "Voronoi":
        comp = []

        # Generate random points within the grid bounds
        points = [
            FreeCAD.Vector(random.uniform(0, grid_size), random.uniform(0, grid_size), 0)
            for _ in range(15)
        ]

        # Compute Voronoi regions using Delaunay triangulation
        try:
            from scipy.spatial import Voronoi
            import numpy as np

            # Convert points to a NumPy array
            point_array = np.array([[pt.x, pt.y] for pt in points])

            # Compute Voronoi diagram
            vor = Voronoi(point_array)

            # Create polygons for each Voronoi region
            for region_index in vor.point_region:
                region = vor.regions[region_index]
                if -1 in region or len(region) == 0:
                    continue  # Skip infinite or empty regions

                # Extract vertices for the region
                vertices = [vor.vertices[vertex_index] for vertex_index in region]

                # Convert vertices to FreeCAD Vectors
                freeCAD_vertices = [FreeCAD.Vector(v[0], v[1], 0) for v in vertices]

                # Create a closed wire and face
                try:
                    wire = Part.makePolygon(freeCAD_vertices + [freeCAD_vertices[0]])  # Close the polygon
                    face = Part.Face(wire)
                    comp.append(face)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(f"Failed to create Voronoi face: {str(e)}\n")

        except ImportError:
            FreeCAD.Console.PrintError("scipy is required for Voronoi pattern generation.\n")
            return None

        return Part.makeCompound(comp)

    elif patternType == "FractalBranches":
        def create_branch(start, length, angle, depth):
            if depth == 0:
                return []
                
            end = start + FreeCAD.Vector(
                length * math.cos(angle),
                length * math.sin(angle),
                0
            )
            
            shapes = [Part.makeLine(start, end)]
            
            # Create sub-branches with golden ratio
            phi = (1 + math.sqrt(5)) / 2
            new_length = length / phi
            
            # Random angle variations
            angle_var = math.radians(random.uniform(-20, 20))
            
            # Create multiple branches with varying angles
            angles = [angle + math.pi/4 + angle_var, 
                    angle - math.pi/4 - angle_var,
                    angle + angle_var]
            
            for new_angle in angles:
                shapes.extend(create_branch(end, new_length, new_angle, depth-1))
                
            return shapes
        
        branches = []
        start_points = [
            FreeCAD.Vector(grid_size/2, 0, 0),
            FreeCAD.Vector(grid_size/4, 0, 0),
            FreeCAD.Vector(3*grid_size/4, 0, 0)
        ]
        
        for start in start_points:
            branches.extend(create_branch(start, grid_size/4, math.pi/2, 6))
        
        return Part.makeCompound(branches)

    elif patternType == "OrganicMaze":
        def create_maze_segment(start, end, complexity):
            points = [start]
            current = start
            target = end
            
            while (current - target).Length > grid_size/20:
                # Calculate direction to target
                direction = (target - current).normalize()
                
                # Add random perpendicular component
                perp = FreeCAD.Vector(-direction.y, direction.x, 0)
                random_offset = perp * random.uniform(-complexity, complexity)
                
                # Create next point
                step = direction * grid_size/20 + random_offset
                current = current + step
                points.append(current)
                
            points.append(target)
            return Part.makePolygon(points)
        
        segments = []
        num_segments = 15
        points = []
        
        # Generate random points for maze endpoints
        for _ in range(num_segments):
            point = FreeCAD.Vector(
                random.uniform(0, grid_size),
                random.uniform(0, grid_size),
                0
            )
            points.append(point)
        
        # Connect points with organic paths
        for i in range(len(points)):
            for j in range(i+1, len(points)):
                if random.random() < 0.3:  # Only connect some points
                    segments.append(create_maze_segment(points[i], points[j], grid_size/10))
        
        return Part.makeCompound(segments)

    elif patternType == "BiomorphicCells":
        def create_cell_membrane(center, size):
            points = []
            num_points = random.randint(12, 18)
            base_radius = size * (1 + random.uniform(-0.2, 0.2))
            
            # Create main cell shape
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                # Add organic variations
                radius = base_radius * (1 + 0.3 * math.sin(3 * angle) + 
                                    0.2 * math.sin(5 * angle) +
                                    random.uniform(-0.1, 0.1))
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                )
                points.append(point)
                
            return Part.makePolygon(points)
        
        cells = []
        num_cells = 12
        min_size = grid_size/8
        max_size = grid_size/4
        
        # Create primary cells
        for _ in range(num_cells):
            center = FreeCAD.Vector(
                random.uniform(grid_size/4, 3*grid_size/4),
                random.uniform(grid_size/4, 3*grid_size/4),
                0
            )
            size = random.uniform(min_size, max_size)
            cells.append(create_cell_membrane(center, size))
            
            # Add smaller organelle-like structures
            for _ in range(random.randint(2, 4)):
                offset = FreeCAD.Vector(
                    random.uniform(-size/2, size/2),
                    random.uniform(-size/2, size/2),
                    0
                )
                organelle_size = size * random.uniform(0.2, 0.4)
                cells.append(create_cell_membrane(center + offset, organelle_size))
        
        return Part.makeCompound(cells)

    elif patternType == "RadialSunburst":
        comp = []
        spokes = 16
        for i in range(spokes):
            angle = math.radians(i * (360/spokes))
            x = grid_size/2 + 4 * math.cos(angle)
            y = grid_size/2 + 4 * math.sin(angle)
            comp.append(Part.makeLine(FreeCAD.Vector(grid_size/2, grid_size/2,0), 
                FreeCAD.Vector(x,y,0)))
        return Part.makeCompound(comp)

    elif patternType == "Sunburst":
        comp = []
        spokes = 24
        for i in range(spokes):
            angle = math.radians(i * (360/spokes))
            inner = 2.0 if i%2 else 3.5
            outer = 4.0 if i%2 else 4.5
            x1 = grid_size/2 + inner * math.cos(angle)
            y1 = grid_size/2 + inner * math.sin(angle)
            x2 = grid_size/2 + outer * math.cos(angle)
            y2 = grid_size/2 + outer * math.sin(angle)
            comp.append(Part.makeLine(FreeCAD.Vector(x1,y1,0), FreeCAD.Vector(x2,y2,0)))
        return Part.makeCompound(comp)

    elif patternType == "Ziggurat":
        comp = []
        step_size = 1.5
        for x in range(0, grid_size, int(step_size*3)):
            for y in range(0, grid_size, int(step_size*3)):
                for i in range(3):
                    comp.append(Part.makePolygon([
                        FreeCAD.Vector(x + i*step_size, y + i*step_size, 0),
                        FreeCAD.Vector(x + (i+1)*step_size, y + i*step_size, 0),
                        FreeCAD.Vector(x + (i+1)*step_size, y + (i+1)*step_size, 0),
                        FreeCAD.Vector(x + i*step_size, y + (i+1)*step_size, 0),
                        FreeCAD.Vector(x + i*step_size, y + i*step_size, 0)
                    ]))
        return Part.makeCompound(comp)

    elif patternType == "SpiralPattern":
        def create_spiral(center, max_radius, turns, points_per_turn):
            points = []
            total_points = int(turns * points_per_turn)
            
            for i in range(total_points + 1):
                t = i / points_per_turn  # Current angle in turns
                radius = max_radius * t / turns
                angle = 2 * math.pi * t
                
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                )
                points.append(point)
            
            # Create lines between points
            edges = []
            for i in range(len(points) - 1):
                edges.append(Part.makeLine(points[i], points[i + 1]))
            
            return edges
        
        radius = min(grid_size/2.5, 2)
        center = FreeCAD.Vector(grid_size/2, grid_size/2, 0)
        
        # Create multiple interleaved spirals
        spirals = []
        for i in range(3):  # Number of interleaved spirals
            phase = 2 * math.pi * i / 3
            spiral_center = center + FreeCAD.Vector(
                0.2 * radius * math.cos(phase),
                0.2 * radius * math.sin(phase),
                0
            )
            spirals.extend(create_spiral(spiral_center, radius, 4, 50))
        
        return Part.makeCompound(spirals)

    elif patternType == "PentaflakeFractal":
        def pentaflake(center, size, depth):
            if depth == 0:
                points = []
                for i in range(5):
                    angle = math.radians(72 * i - 18)
                    points.append(center + FreeCAD.Vector(
                        size * math.cos(angle),
                        size * math.sin(angle),
                        0
                    ))
                points.append(points[0])  # Close the pentagon
                return [Part.makePolygon(points)]
                
            shapes = []
            # Central pentagon
            shapes.extend(pentaflake(center, size/3, depth-1))
            
            # Surrounding pentagons
            for i in range(5):
                angle = math.radians(72 * i - 18)
                new_center = center + FreeCAD.Vector(
                    size * 0.618 * math.cos(angle),
                    size * 0.618 * math.sin(angle),
                    0
                )
                shapes.extend(pentaflake(new_center, size/3, depth-1))
                
            return shapes
        
        size = min(grid_size/3, 2)
        center = FreeCAD.Vector(grid_size/2, grid_size/2, 0)
        
        return Part.makeCompound(pentaflake(center, size, 3))

    elif patternType == "HilbertCurve":
        def hilbert(x, y, xi, xj, yi, yj, n):
            if n <= 0:
                X = x + (xi + yi)/2
                Y = y + (xj + yj)/2
                return [FreeCAD.Vector(X, Y, 0)]
                
            points = []
            points.extend(hilbert(x,           y,           yi/2,  yj/2,  xi/2,  xj/2, n-1))
            points.extend(hilbert(x+xi/2,      y+xj/2,      xi/2,  xj/2,  yi/2,  yj/2, n-1))
            points.extend(hilbert(x+xi/2+yi/2, y+xj/2+yj/2, xi/2,  xj/2,  yi/2,  yj/2, n-1))
            points.extend(hilbert(x+xi+yi/2,   y+xj+yj/2,   -yi/2, -yj/2, -xi/2, -xj/2, n-1))
            return points
        
        size = min(grid_size*0.8, 4)
        offset = (grid_size - size)/2
        points = hilbert(offset, offset, size, 0, 0, size, 4)
        
        edges = []
        for i in range(len(points)-1):
            edges.append(Part.makeLine(points[i], points[i+1]))
        
        return Part.makeCompound(edges)

    elif patternType == "SierpinskiTriangle":
        def sierpinski(p1, p2, p3, depth):
            if depth == 0:
                return [Part.makePolygon([p1, p2, p3, p1])]
                
            # Calculate midpoints
            mid1 = (p1 + p2) * 0.5
            mid2 = (p2 + p3) * 0.5
            mid3 = (p3 + p1) * 0.5
            
            # Recursive calls for three smaller triangles
            return (sierpinski(p1, mid1, mid3, depth-1) +
                    sierpinski(mid1, p2, mid2, depth-1) +
                    sierpinski(mid3, mid2, p3, depth-1))
        
        size = min(grid_size/2, 3)
        center = FreeCAD.Vector(grid_size/2, grid_size/2, 0)
        height = size * math.sqrt(3)/2
        
        points = [
            center + FreeCAD.Vector(0, height, 0),
            center + FreeCAD.Vector(-size, -height, 0),
            center + FreeCAD.Vector(size, -height, 0)
        ]
        
        return Part.makeCompound(sierpinski(points[0], points[1], points[2], 4))

    elif patternType == "PenroseTiling":
        def create_rhombus(center, size, angle):
            points = []
            for i in [0, 72, 144, 216, 288]:
                a = math.radians(angle + i)
                points.append(center + FreeCAD.Vector(size*math.cos(a), size*math.sin(a),0))
            return Part.makePolygon(points[:4])
        
        comp = []
        phi = (1 + math.sqrt(5))/2  # Golden ratio
        sizes = [2.0, 2.0/phi]
        for i in range(10):
            center = FreeCAD.Vector(random.uniform(2,8), random.uniform(2,8),0)
            comp.append(create_rhombus(center, sizes[i%2], random.choice([0, 36])))
        return Part.makeCompound(comp)

    elif patternType == "EinsteinMonotile":
        def einstein_tile(center):
            angles = [0, 60, 120, 180, 240, 300]
            points = []
            for i, ang in enumerate(angles):
                r = 1.0 if i%2 else 1.732
                x = center.x + r*math.cos(math.radians(ang))
                y = center.y + r*math.sin(math.radians(ang))
                points.append(FreeCAD.Vector(x,y,0))
            points.append(points[0])
            return Part.makePolygon(points)
        
        comp = []
        for x in range(0, grid_size*2, 3):
            for y in range(0, grid_size*2, 3):
                comp.append(einstein_tile(FreeCAD.Vector(x,y,0)))
        return Part.makeCompound(comp)

    elif patternType == "LeafVeins":
        comp = []
        midrib = Part.makeLine(FreeCAD.Vector(5,1,0), FreeCAD.Vector(5,9,0))
        comp.append(midrib)
        for y in range(1, 10, 2):
            for side in [-1, 1]:
                angle = math.radians(45 * (1 - abs(y-5)/5))
                points = [
                    FreeCAD.Vector(5, y, 0),
                    FreeCAD.Vector(5 + side*3*math.cos(angle), y + 3*math.sin(angle), 0)
                ]
                comp.append(Part.makePolygon(points))
        return Part.makeCompound(comp)

    elif patternType == "WoodPlanks":
        comp = []
        for y in range(0, grid_size+1, 2):
            comp.append(Part.makeLine(
                FreeCAD.Vector(0, y, 0),
                FreeCAD.Vector(grid_size, y, 0)
            ))
            if y % 4 == 0:
                for x in [3, 7]:
                    vline = Part.makeLine(
                        FreeCAD.Vector(x, y, 0),
                        FreeCAD.Vector(x, y+1, 0)
                    )
                    comp.append(vline)
        return Part.makeCompound(comp)
    
    elif patternType == "ParquetHerringbone":
        comp = []  # List to store individual components of the pattern
        plank_w = 1.0  # Width of each plank
        plank_l = 4.0  # Length of each plank
        angle = 45  # Angle for herringbone orientation

        # Generate the herringbone pattern by iterating over the grid
        for i in range(-grid_size, grid_size * 2, int(plank_l / math.sqrt(2))):  # Adjust step size for diagonal alignment
            for j in range(-grid_size, grid_size * 2, int(plank_w)):
                # First plank (angled right)
                points1 = [
                    FreeCAD.Vector(j, i, 0),
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)),
                                i + plank_l * math.sin(math.radians(angle)), 0),
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)) - plank_w * math.sin(math.radians(angle)),
                                i + plank_l * math.sin(math.radians(angle)) + plank_w * math.cos(math.radians(angle)), 0),
                    FreeCAD.Vector(j - plank_w * math.sin(math.radians(angle)),
                                i + plank_w * math.cos(math.radians(angle)), 0)
                ]
                comp.append(Part.makePolygon(points1))

                # Second plank (angled left, offset)
                points2 = [
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)), i, 0),
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)) - plank_l * math.cos(math.radians(angle)),
                                i + plank_l * math.sin(math.radians(angle)), 0),
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)) - plank_l * math.cos(math.radians(angle)) - plank_w * math.sin(math.radians(angle)),
                                i + plank_l * math.sin(math.radians(angle)) + plank_w * math.cos(math.radians(angle)), 0),
                    FreeCAD.Vector(j + plank_l * math.cos(math.radians(angle)) - plank_w * math.sin(math.radians(angle)),
                                i + plank_w * math.cos(math.radians(angle)), 0)
                ]
                comp.append(Part.makePolygon(points2))

        return Part.makeCompound(comp)

    elif patternType == "WoodGrain":
        comp = []  # List to store individual components of the pattern
        num_grains = 50  # Number of wood grain lines
        max_deviation = 0.5  # Maximum deviation from a straight line

        for _ in range(num_grains):
            # Random starting position
            x_start = random.uniform(0, grid_size)
            y_start = random.uniform(0, grid_size)

            # Random amplitude and frequency for the wave
            amp = random.uniform(0.1, 0.5)  # Reduced amplitude for subtlety
            freq = random.uniform(0.1, 0.3)  # Lower frequency for smoother waves

            points = []
            for t in range(0, grid_size * 2, 2):  # Generate points along the grain
                x_val = x_start + t * 0.1
                y_val = y_start + amp * math.sin(freq * x_val) + random.uniform(-max_deviation, max_deviation)
                points.append(FreeCAD.Vector(x_val, y_val, 0))

            # Add the grain line to the compound
            comp.append(Part.makePolygon(points))

        return Part.makeCompound(comp)

    elif patternType == "DrywallOrangePeel":
        def create_texture_spot(center, size):
            points = []
            num_points = random.randint(6, 8)
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                # Create irregular circular shape
                radius = size * (1 + random.uniform(-0.3, 0.3))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        spots = []
        size = grid_size
        num_spots = 400
        spot_size = size/60
        
        for _ in range(num_spots):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            spots.append(create_texture_spot(center, spot_size))
        
        return Part.makeCompound(spots)

    elif patternType == "DrywallKnockdown":
        def create_knockdown_splatter(center, size):
            points = []
            num_points = random.randint(8, 12)
            
            # Create irregular elongated shape
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.4, 0.4))
                # Flatten the shape to simulate knockdown effect
                radius *= (1.5 if i % 2 == 0 else 0.7)
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        splatters = []
        size = grid_size
        num_splatters = 200
        splatter_size = size/40
        
        for _ in range(num_splatters):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            rotation = random.uniform(0, 2 * math.pi)
            splatters.append(create_knockdown_splatter(center, splatter_size))
        
        return Part.makeCompound(splatters)

    elif patternType == "StuccoSandFloat":
        def create_sand_grain(center, size):
            points = []
            num_points = random.randint(4, 6)
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.2, 0.2))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        grains = []
        size = grid_size
        num_grains = 1000
        grain_size = size/120
        
        for _ in range(num_grains):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            grains.append(create_sand_grain(center, grain_size))
        
        return Part.makeCompound(grains)

    elif patternType == "StuccoDash":
        def create_dash_splatter(center, size):
            points = []
            num_points = random.randint(7, 10)
            
            # Create irregular elongated shape
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.6, 0.6))
                # Create more elongated shape
                radius *= (2 if i % 2 == 0 else 0.5)
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        splatters = []
        size = grid_size
        num_splatters = 300
        splatter_size = size/50
        
        for _ in range(num_splatters):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            splatters.append(create_dash_splatter(center, splatter_size))
        
        return Part.makeCompound(splatters)

    elif patternType == "DrywallSkipTrowel":
        def create_skip_mark(center, size, angle):
            points = []
            length = size * random.uniform(0.8, 1.2)
            width = size * random.uniform(0.2, 0.4)
            
            # Create elongated oval shape
            steps = 20
            for i in range(steps + 1):
                t = i / steps
                r = width * math.sqrt(1 - (2*t - 1)**2)
                x = length * (t - 0.5)
                point = center.add(FreeCAD.Vector(
                    x * math.cos(angle) - r * math.sin(angle),
                    x * math.sin(angle) + r * math.cos(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        marks = []
        size = grid_size
        num_marks = 200
        mark_size = size/15
        
        for _ in range(num_marks):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            angle = random.uniform(0, math.pi)
            marks.append(create_skip_mark(center, mark_size, angle))
        
        return Part.makeCompound(marks)

    elif patternType == "Concrete":
        comp = []
        for _ in range(20):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.uniform(0, 360))
            x2 = x1 + random.uniform(1, 3)*math.cos(angle)
            y2 = y1 + random.uniform(1, 3)*math.sin(angle)
            comp.append(Part.makeLine(
                FreeCAD.Vector(x1, y1, 0),
                FreeCAD.Vector(x2, y2, 0)
            ))
        return Part.makeCompound(comp)

    elif patternType == "ConcreteStampedPattern":
        def create_cobblestone(center, size):
            num_points = random.randint(6, 8)
            points = []
            base_radius = size/2
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = base_radius * (1 + random.uniform(-0.2, 0.2))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
            
            return Part.makePolygon(points)
        
        stones = []
        size = grid_size
        stone_size = size/8
        offset = stone_size * 0.1
        
        for row in range(int(size/stone_size) + 1):
            row_offset = offset * (row % 2)
            for col in range(int(size/stone_size) + 1):
                center = FreeCAD.Vector(
                    col * stone_size + row_offset + random.uniform(-offset, offset),
                    row * stone_size + random.uniform(-offset, offset),
                    0
                )
                stones.append(create_cobblestone(center, stone_size))
        
        return Part.makeCompound(stones)

    elif patternType == "ConcreteSaltFinish":
        def create_speckle(center, size):
            num_points = random.randint(4, 6)
            points = []
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.3, 0.3))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        speckles = []
        size = grid_size
        num_speckles = 300
        speckle_size = size/100
        
        for _ in range(num_speckles):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            speckles.append(create_speckle(center, speckle_size))
        
        return Part.makeCompound(speckles)

    elif patternType == "ConcreteFormTiePattern":
        def create_tie_hole(center, radius):
            points = []
            num_points = 16
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        holes = []
        size = grid_size
        spacing_x = size/4
        spacing_y = size/3
        hole_radius = size/40
        
        for x in range(1, 4):
            for y in range(1, 3):
                center = FreeCAD.Vector(x * spacing_x, y * spacing_y, 0)
                holes.append(create_tie_hole(center, hole_radius))
        
        return Part.makeCompound(holes)

    elif patternType == "ConcreteSandblastPattern":
        def create_erosion_mark(center, size):
            points = []
            num_points = random.randint(8, 12)
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.5, 0.5))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
                
            return Part.makePolygon(points)
        
        marks = []
        size = grid_size
        num_marks = 200
        mark_size = size/60
        
        for _ in range(num_marks):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            marks.append(create_erosion_mark(center, mark_size))
        
        # Add some larger eroded areas
        for _ in range(20):
            center = FreeCAD.Vector(
                random.uniform(0, size),
                random.uniform(0, size),
                0
            )
            marks.append(create_erosion_mark(center, mark_size * 3))
        
        return Part.makeCompound(marks)

    elif patternType == "ConcreteControlJoint":
        def create_tooled_joint(start, end, width):
            direction = end.sub(start)
            perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
            length = direction.Length
            steps = int(length / (width * 2))
            points = []
            
            # Create sawtooth pattern for tooled joint
            for i in range(steps):
                t = i / steps
                mid = start.add(direction.multiply(t))
                points.append(mid.add(perp.multiply(width/2)))
                points.append(mid.add(direction.multiply(1/(2*steps))))
                points.append(mid.add(perp.multiply(-width/2)))
                points.append(mid.add(direction.multiply(1/steps)))
                
            return Part.makePolygon(points)
        
        joints = []
        size = grid_size
        joint_width = size/50
        
        # Create pattern of control joints
        for x in [size/3, 2*size/3]:
            start = FreeCAD.Vector(x, 0, 0)
            end = FreeCAD.Vector(x, size, 0)
            joints.append(create_tooled_joint(start, end, joint_width))
            
        for y in [size/3, 2*size/3]:
            start = FreeCAD.Vector(0, y, 0)
            end = FreeCAD.Vector(size, y, 0)
            joints.append(create_tooled_joint(start, end, joint_width))
        
        return Part.makeCompound(joints)

    elif patternType == "ConcreteGridPattern":
        def create_expansion_joint(start, end, width):
            direction = end.sub(start)
            perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
            
            points = [
                start.add(perp.multiply(width/2)),
                end.add(perp.multiply(width/2)),
                end.sub(perp.multiply(width/2)),
                start.sub(perp.multiply(width/2)),
                start.add(perp.multiply(width/2))
            ]
            return Part.makePolygon(points)
        
        joints = []
        size = grid_size
        spacing = size/4
        joint_width = size/50
        
        # Horizontal joints
        for y in range(1, 4):
            start = FreeCAD.Vector(0, y * spacing, 0)
            end = FreeCAD.Vector(size, y * spacing, 0)
            joints.append(create_expansion_joint(start, end, joint_width))
        
        # Vertical joints
        for x in range(1, 4):
            start = FreeCAD.Vector(x * spacing, 0, 0)
            end = FreeCAD.Vector(x * spacing, size, 0)
            joints.append(create_expansion_joint(start, end, joint_width))
        
        return Part.makeCompound(joints)

    elif patternType == "WoodKnotPattern":
        def create_knot(center, radius):
            shapes = []
            
            # Create concentric circles for knot
            for r in range(3, int(radius * 10), 2):
                r = r/10
                points = []
                steps = int(2 * math.pi * r * 10)
                
                for i in range(steps + 1):
                    angle = 2 * math.pi * i / steps
                    # Add some irregularity to the circles
                    curr_radius = r * (1 + random.uniform(-0.1, 0.1))
                    point = center.add(FreeCAD.Vector(
                        curr_radius * math.cos(angle),
                        curr_radius * math.sin(angle),
                        0
                    ))
                    points.append(point)
                
                shapes.append(Part.makePolygon(points))
            
            return shapes
        
        knots = []
        size = grid_size
        num_knots = 3
        
        # Generate random knots
        for _ in range(num_knots):
            center = FreeCAD.Vector(
                random.uniform(size/4, 3*size/4),
                random.uniform(size/4, 3*size/4),
                0
            )
            radius = random.uniform(size/15, size/10)
            knots.extend(create_knot(center, radius))
        
        return Part.makeCompound(knots)

    elif patternType == "ConcreteAggregatePattern":
        def create_aggregate(center, size):
            num_points = random.randint(5, 8)
            points = []
            
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = size * (1 + random.uniform(-0.3, 0.3))
                point = center.add(FreeCAD.Vector(
                    radius * math.cos(angle),
                    radius * math.sin(angle),
                    0
                ))
                points.append(point)
            
            return Part.makePolygon(points)
        
        aggregates = []
        grid = grid_size
        num_aggregates = 50
        
        for _ in range(num_aggregates):
            center = FreeCAD.Vector(
                random.uniform(0, grid),
                random.uniform(0, grid),
                0
            )
            size = random.uniform(grid/40, grid/20)
            aggregates.append(create_aggregate(center, size))
        
        return Part.makeCompound(aggregates)

    elif patternType == "BrushedConcrete":
        comp = []
        for _ in range(15):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.choice([30, 45, 60]))
            length = random.uniform(3, 6)
            x2 = x1 + length*math.cos(angle)
            y2 = y1 + length*math.sin(angle)
            comp.append(Part.makeLine(FreeCAD.Vector(x1,y1,0), FreeCAD.Vector(x2,y2,0)))
        return Part.makeCompound(comp)

    elif patternType == "PebbleConcrete":
        comp = []
        for _ in range(25):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            comp.append(Part.makeCircle(random.uniform(0.2,0.5), 
                FreeCAD.Vector(x,y,0)))
        return Part.makeCompound(comp)

    elif patternType == "CrackedConcrete":
        comp = []
        # Main cracks
        for _ in range(3):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.uniform(0, 360))
            length = random.uniform(4, 8)
            x2 = x1 + length*math.cos(angle)
            y2 = y1 + length*math.sin(angle)
            comp.append(Part.makeLine(FreeCAD.Vector(x1,y1,0), FreeCAD.Vector(x2,y2,0)))
            
            # Small branches
            for _ in range(3):
                branch_angle = angle + math.radians(random.uniform(-30,30))
                branch_length = random.uniform(1,2)
                x3 = x2 + branch_length*math.cos(branch_angle)
                y3 = y2 + branch_length*math.sin(branch_angle)
                comp.append(Part.makeLine(FreeCAD.Vector(x2,y2,0), FreeCAD.Vector(x3,y3,0)))
        return Part.makeCompound(comp)

    elif patternType == "AggregateConcrete":
        comp = []
        # Base cracks
        for _ in range(5):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            comp.append(Part.makeLine(
                FreeCAD.Vector(x1,y1,0),
                FreeCAD.Vector(x1 + random.uniform(-1,1), 
                            y1 + random.uniform(-1,1),0)
            ))
        # Aggregate stones
        for _ in range(20):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            comp.append(Part.makeCircle(random.uniform(0.2,0.5), 
                                    FreeCAD.Vector(x,y,0)))
        return Part.makeCompound(comp)

    elif patternType == "StampedConcrete":
        comp = []
        for x in range(0, grid_size, 3):
            for y in range(0, grid_size, 3):
                if (x//3 + y//3) % 2 == 0:
                    comp.append(Part.makeCircle(1.0, FreeCAD.Vector(x+1.5,y+1.5,0)))
                else:
                    comp.append(Part.makePolygon([
                        FreeCAD.Vector(x,y,0),
                        FreeCAD.Vector(x+3,y,0),
                        FreeCAD.Vector(x+1.5,y+3,0),
                        FreeCAD.Vector(x,y,0)
                    ]))
        return Part.makeCompound(comp)

    elif patternType == "Insulation":
        comp = []
        for y in range(0, grid_size+1, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(
                    FreeCAD.Vector(x,   y,   0),
                    FreeCAD.Vector(x+2, y+1, 0)
                )
                line2 = Part.makeLine(
                    FreeCAD.Vector(x+2, y+1, 0),
                    FreeCAD.Vector(x+4, y,   0)
                )
                comp.extend([line1, line2])
        return Part.makeCompound(comp)

    elif patternType == "Rebar":
        comp = []
        for x in range(0, grid_size+1, 2):
            for y in range(0, grid_size+1, 2):
                vline = Part.makeLine(FreeCAD.Vector(x, 0, 0), FreeCAD.Vector(x, grid_size, 0))
                hline = Part.makeLine(FreeCAD.Vector(0, y, 0), FreeCAD.Vector(grid_size, y, 0))
                circle= Part.makeCircle(0.3, FreeCAD.Vector(x, y, 0))
                comp.extend([vline, hline, circle])
        return Part.makeCompound(comp)

    elif patternType == "RoofTiles":
        comp = []
        for y in range(0, grid_size+1, 3):
            for x in range(0, grid_size+1, 3):
                arc = Part.ArcOfCircle(
                    Part.Circle(
                        FreeCAD.Vector(x+1.5, y+1.5, 0),
                        FreeCAD.Vector(0,0,1),
                        1.5
                    ),
                    0,
                    math.pi
                ).toShape()
                comp.append(arc)
        return Part.makeCompound(comp)

    else:
        FreeCAD.Console.PrintWarning(f"Unknown built-in pattern: {patternType}\n")
        return baseRect

# ================================
# Other Utilities 
# ================================
def getBaseShapeFromSketchOrFeature(obj):
    """
    If 'obj' is a Sketch, or a Draft object with MakeFace=False, 
    we try to build faces for any closed wires. Otherwise just return obj.Shape.
    """
    if not obj or not hasattr(obj, "Shape"):
        return None

    if obj.isDerivedFrom("Sketcher::SketchObject"):
        sketchShape = obj.Shape
        closed_faces = []
        for wire in sketchShape.Wires:
            if wire.isClosed():
                try:
                    face = Part.Face(wire)
                    closed_faces.append(face)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(f"Failed to create face from wire: {e}\n")
        if closed_faces:
            return Part.makeCompound(closed_faces)
        else:
            return sketchShape
    else:
        if hasattr(obj, "ViewObject") and hasattr(obj.ViewObject, "MakeFace"):
            if obj.ViewObject.MakeFace is False:
                shape = obj.Shape
                closed_faces = []
                for wire in shape.Wires:
                    if wire.isClosed():
                        try:
                            face = Part.Face(wire)
                            closed_faces.append(face)
                        except Exception as e:
                            FreeCAD.Console.PrintWarning(f"Failed to create face: {e}\n")
                if closed_faces:
                    return Part.makeCompound(closed_faces)
                else:
                    return shape
        return obj.Shape

def getClosedWiresAsFaces(obj):
    """
    Return a shape that has faces. If the object's shape already has faces
    (e.g., Draft object with MakeFace=True), just return it as is.
    Otherwise, attempt to build faces from closed wires.
    """
    if not obj or not hasattr(obj, "Shape"):
        return None

    shape = obj.Shape
    if shape.isNull():
        return None

    if shape.Faces:
        return shape

    faces = []
    for wire in shape.Wires:
        if wire.isClosed():
            try:
                f = Part.Face(wire)
                faces.append(f)
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Cannot make face from wire: {e}\n")

    if faces:
        return Part.makeCompound(faces)
    else:
        return shape

# ================================
# Parametric Hatch Feature
# ================================
class CustomHatchFeature:
    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "CustomHatchFP"
        self._is_recomputing = False

        # Base
        obj.addProperty("App::PropertyLink", "BaseObject", "Hatch", "Object with the base shape.")
        # === We also have "BaseObjects" here: multiple base shapes
        obj.addProperty("App::PropertyLinkList", "BaseObjects", "Hatch", 
                        "Optional list of multiple base objects.")

        # Pattern
        obj.addProperty("App::PropertyLink", "PatternObject", "Hatch", "Object with the pattern shape.")
        # === PatternObjects for multiple patterns
        obj.addProperty("App::PropertyLinkList", "PatternObjects", "Hatch", 
                        "Optional list of multiple pattern objects (fused).")

        # Base Tile
        obj.addProperty("App::PropertyLink", "BaseTileObject", "Hatch",
                        "Optional tile shape object (if set, used to define bounding box).")
        obj.addProperty("App::PropertyBool", "TileVisibility", "Hatch",
                        "Toggle tile shape visibility as a separate object.")
        obj.TileVisibility = True
        obj.addProperty("App::PropertyBool", "UpdateTileOnChange", "Hatch",
                        "Update tile shapes automatically when Base Tile Object changes.")
        obj.UpdateTileOnChange = True

        # === Subtractions
        obj.addProperty("App::PropertyLinkList", "Subtractions", "Hatch",
                        "List of objects to subtract from the hatch pattern.")

        # Distribution
        obj.addProperty("App::PropertyEnumeration", "DistributionMode", "Hatch",
                        "How to distribute the pattern.")
        obj.DistributionMode = [
            "CenteredTiling",
            "RelativeSpacing",
            "SeamlessTiling",
            "LinearGrid",
            "RadialDistribution",
            "ConcentricDistribution",
            "RandomDistribution",
            "AdaptiveDistribution"
        ]
        obj.DistributionMode = "SeamlessTiling"

        # Pattern Type
        obj.addProperty("App::PropertyEnumeration", "PatternType", "Hatch",
                        "Choose built-in patterns or custom PatternObject.")
        obj.PatternType = [
            "CustomObject",
            "SolidFill",
            "HorizontalLines",
            "VerticalLines",
            "Crosshatch",
            "Herringbone",
            "BrickPattern",
            "RandomDots",
            "OverlappingSquares",
            "Checkerboard",
            "CheckerboardCircles",
            "RotatingHexagons",
            "NestedTriangles",
            "InterlockingCircles",
            "RecursiveSquares",
            "FlowerOfLife",
            "VoronoiMesh",
            "OffsetChecker",
            "ZigZag",
            "HexagonalHoriz",
            "HexagonalVerti",
            "HexagonalPattern",
            "TrianglesGrid",
            "MidEastMosaic",
            "StarGridPattern",
            "BasketWeave",
            "Honeycomb",
            "SineWave",
            "SpaceFrame",
            "HoneycombDual",
            "ArtDeco",
            "StainedGlass",
            "PenroseTriangle",
            "GreekKey",
            "ChainLinks",
            "TriangleForest",
            "CeramicTile",
            "CirclesGrid",
            "PlusSigns",
            "WavesPattern",
            "GalaxyStarsPattern",
            "GridDots",
            "InterlockingCircles",
            "HexDots",
            "FractalTree",
            "Voronoi",
            "FractalBranches",
            "OrganicMaze",
            "BiomorphicCells",
            "RadialSunburst",
            "Sunburst",
            "Ziggurat",
            "SpiralPattern",
            "PentaflakeFractal",
            "HilbertCurve",
            "SierpinskiTriangle",
            "PenroseTiling",
            "EinsteinMonotile",
            "LeafVeins",
            "WoodPlanks",
            "ParquetHerringbone",
            "WoodGrain",
            "DrywallOrangePeel",
            "DrywallKnockdown",
            "StuccoSandFloat",
            "StuccoDash",
            "DrywallSkipTrowel",
            "Concrete",
            "ConcreteStampedPattern",
            "ConcreteSaltFinish",
            "ConcreteFormTiePattern",
            "ConcreteSandblastPattern",
            "ConcreteControlJoint",
            "ConcreteGridPattern",
            "WoodKnotPattern",
            "ConcreteAggregatePattern",
            "BrushedConcrete",
            "PebbleConcrete",
            "CrackedConcrete",
            "AggregateConcrete",
            "StampedConcrete",
            "Insulation",
            "Rebar",
            "RoofTiles",
        ]
        obj.PatternType = "CustomObject"

        # Scale / Rotation / Spacing
        obj.addProperty("App::PropertyBool", "AutoScaleToFitBase", "Scaling", 
                        "If true, automatically compute scale.")
        obj.AutoScaleToFitBase = False
        obj.addProperty("App::PropertyFloat", "PatternScale", "Scaling", "Manual scale factor.")
        obj.PatternScale = 1.0
        obj.addProperty("App::PropertyFloat", "RotationDeg", "Hatch", "Rotation around Z (degrees).")
        obj.RotationDeg = 0.0
        obj.addProperty("App::PropertyFloat", "BaseSpacing", "Hatch", "Base spacing (mm or %).")
        obj.BaseSpacing = 10.0
        # Unit system properties
        obj.addProperty("App::PropertyBool", "UseUnits", "Hatch", 
                        "If true, interpret BaseSpacing with a chosen unit system.")
        obj.UseUnits = False
        obj.addProperty("App::PropertyEnumeration", "SelectedUnitSystem", "Hatch",
                        "User-chosen unit system for BaseSpacing.")
        obj.SelectedUnitSystem = ["FreeCAD Default", "Metric (m)", "Imperial (ft)", "BIM Workbench Unit"]
        obj.SelectedUnitSystem = "FreeCAD Default"
        obj.addProperty("App::PropertyInteger", "RepetitionsX", "Hatch", "Number of repeats in X.")
        obj.RepetitionsX = 5
        obj.addProperty("App::PropertyInteger", "RepetitionsY", "Hatch", "Number of repeats in Y.")
        obj.RepetitionsY = 5

        # Pattern offset
        obj.addProperty("App::PropertyFloat", "PatternOffsetX", "Hatch", 
                        "Offset the pattern in X direction.")
        obj.PatternOffsetX = 0.0
        obj.addProperty("App::PropertyFloat", "PatternOffsetY", "Hatch", 
                        "Offset the pattern in Y direction.")
        obj.PatternOffsetY = 0.0

        # Scale Mode
        obj.addProperty("App::PropertyEnumeration", "ScaleMode", "Scaling", 
                        "How PatternScale is interpreted.")
        obj.ScaleMode = ["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"]
        obj.ScaleMode = "Absolute"

        # Random
        obj.addProperty("App::PropertyBool", "RandomizePlacement", "Random",
                        "Apply random transforms?")
        obj.RandomizePlacement = False
        obj.addProperty("App::PropertyFloat", "RandomOffsetRange", "Random", 
                        "Max random offset ± mm.")
        obj.RandomOffsetRange = 0.0
        obj.addProperty("App::PropertyFloat", "RandomRotationRange", "Random", 
                        "Max random rotation ± deg.")
        obj.RandomRotationRange = 0.0
        obj.addProperty("App::PropertyFloat", "RandomScaleMin", "Random",
                        "Min random scale factor.")
        obj.RandomScaleMin = 1.0
        obj.addProperty("App::PropertyFloat", "RandomScaleMax", "Random",
                        "Max random scale factor.")
        obj.RandomScaleMax = 1.0

        # Lock / Clamping
        obj.addProperty("App::PropertyBool", "LockToBase", "Placement",
                        "If True, user transform is re-clipped.")
        obj.LockToBase = False

        # Additional distribution parameters
        obj.addProperty("App::PropertyInteger", "RadialCount", "Radial",
                        "Number of copies around the circle.")
        obj.RadialCount = 8
        obj.addProperty("App::PropertyFloat", "RadialRadius", "Radial",
                        "Radius for radial distribution.")
        obj.RadialRadius = 50.0
        obj.addProperty("App::PropertyInteger", "ConcentricCount", "Concentric",
                        "Number of rings.")
        obj.ConcentricCount = 5
        obj.addProperty("App::PropertyFloat", "ConcentricSpacing", "Concentric",
                        "Spacing between rings.")
        obj.ConcentricSpacing = 10.0
        obj.addProperty("App::PropertyInteger", "RandomCount", "Random",
                        "Number of random placements.")
        obj.RandomCount = 30

        # Pattern Placement Mode
        obj.addProperty("App::PropertyEnumeration", "PatternPlacementMode", "Hatch",
                        "Positioning of the pattern within each tile.")
        obj.PatternPlacementMode = [
            "Origin", "Center", 
            "TopLeft", "TopRight", "BottomLeft", "BottomRight",
            "TopCenter", "BottomCenter", "LeftCenter", "RightCenter",
            "Custom"
        ]
        obj.PatternPlacementMode = "Origin"

        # === [MOVED TO MAIN TAB] was 'ShowFaces' in advanced tab
        obj.addProperty("App::PropertyBool", "ShowFaces", "Rendering",
                        "If True, tries to convert lines to faces.")
        obj.ShowFaces = False

        obj.addProperty("App::PropertyBool", "ApplyTo3DSurface", "Rendering",
                        "If True, map the pattern onto the selected 3D face.")
        obj.ApplyTo3DSurface = False

        obj.addProperty("App::PropertyEnumeration", "ClipMode", "Rendering",
                        "How to handle open wires vs. faces when clipping with the base.")
        obj.ClipMode = ["BooleanOnly", "PreserveLinesNoClip"]
        obj.ClipMode = "BooleanOnly"

        # ===== NEW: Surface projection properties =====
        obj.addProperty("App::PropertyBool", "UseSurfaceProjection", "Surface",
                        "If True, project pattern onto the surface of the base shape (works on walls, sloped roofs, etc.).")
        obj.UseSurfaceProjection = True
        obj.addProperty("App::PropertyBool", "ForceXYPlane", "Surface",
                        "If True, ignore surface projection and use XY plane only (old behavior).")
        obj.ForceXYPlane = False

        # Performance limit
        obj.addProperty("App::PropertyInteger", "MaxTilesAllowed", "Performance",
                        "Maximum number of tiles to generate.")
        obj.MaxTilesAllowed = 1000

        # Variation properties
        obj.addProperty("App::PropertyFloat", "DensityFactor", "Variation",
                        "Density factor for random distribution (0-1).")
        obj.DensityFactor = 1.0
        obj.addProperty("App::PropertyFloat", "RandomRotationMin", "Variation",
                        "Minimum random rotation (deg).")
        obj.RandomRotationMin = 0.0
        obj.addProperty("App::PropertyFloat", "RandomRotationMax", "Variation",
                        "Maximum random rotation (deg).")
        obj.RandomRotationMax = 0.0
        obj.addProperty("App::PropertyBool", "EnableColorVariation", "Variation",
                        "If true, random colors are assigned to each tile.")
        obj.EnableColorVariation = False
        obj.addProperty("App::PropertyFloat", "ColorVariationIntensity", "Variation",
                        "Intensity of color variation (0-1).")
        obj.ColorVariationIntensity = 0.5
        obj.addProperty("App::PropertyFloat", "SpacingVariation", "Variation",
                        "Random factor for spacing variation (0-1).")
        obj.SpacingVariation = 0.0
        obj.addProperty("App::PropertyBool", "EnableShapeDistortion", "Variation",
                        "If true, shapes might be distorted in random ways.")
        obj.EnableShapeDistortion = False

        # Statistics
        obj.addProperty("App::PropertyFloat", "GenerationTime", "Statistics", "Time taken to generate the hatch")
        obj.GenerationTime = 0.0
        obj.addProperty("App::PropertyInteger", "TileCount", "Statistics", "Number of tiles generated")
        obj.TileCount = 0

        if hasattr(obj, "ViewObject") and obj.ViewObject:
            safeSetDisplayMode(obj.ViewObject, "Flat Lines")

    def __getstate__(self):
        return self.Type  # Or return a dict if you need to preserve additional state

    def __setstate__(self, state):
        self.Type = state

    def onChanged(self, fp, prop):
        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            if prop == "BaseTileObject":
                if fp.BaseTileObject:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(fp.BaseTileObject)
            self.safe_delayed_execute(fp, 100)
        if prop == "PatternPlacementMode":
            self.safe_delayed_execute(fp, 100) 
        if prop == "DistributionMode":
            props = fp.PropertiesList
            def setModeIfExists(pn, mv):
                if pn in props:
                    fp.setEditorMode(pn, mv)
            setModeIfExists("BaseSpacing", 0)
            setModeIfExists("RepetitionsX", 0)
            setModeIfExists("RepetitionsY", 0)

        if prop == "LockToBase":
            if fp.LockToBase:
                fp.setEditorMode("Placement", 2)  # read-only
            else:
                fp.setEditorMode("Placement", 0)

        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            self.execute(fp)

    def safe_delayed_execute(self, fp, delay):
        """Safely schedule execute() with object existence check"""
        if not fp or not fp.Document:
            return
        
        # Capture object IDENTIFIERS instead of object references
        doc_name = fp.Document.Name
        obj_name = fp.Name
        
        def callback():
            # Check if document/object still exist
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return
            obj = doc.getObject(obj_name)
            if obj and obj.Proxy == self:
                self.execute(obj)
        
        QtCore.QTimer.singleShot(delay, callback)

    def clampPlacementInsideBaseBounding(self, fp):
        baseObj = fp.BaseObject
        if not baseObj:
            return
        bBB = baseObj.Shape.BoundBox
        shape = fp.Shape.copy()
        if shape.isNull():
            return
        newBB = shape.BoundBox
        if newBB.XMin < bBB.XMin:
            shift = bBB.XMin - newBB.XMin
            oldPlacement = fp.Placement
            p = oldPlacement.Base
            p.x += shift
            fp.Placement = FreeCAD.Placement(p, oldPlacement.Rotation)
        if newBB.XMax > bBB.XMax:
            shift = bBB.XMax - newBB.XMax
            oldPlacement = fp.Placement
            p = oldPlacement.Base
            p.x += shift
            fp.Placement = FreeCAD.Placement(p, oldPlacement.Rotation)

    def execute(self, fp):
        import datetime
        start_time = datetime.datetime.now()
        doc = fp.Document
        if not doc:
            fp.Shape = Part.makeCompound([])
            return

        baseShapes = []

        # Gather BaseObject
        # Force a best-effort conversion of closed wires to faces, 
        # but if that fails or it's partially open, keep edges.
        if fp.BaseObject:
            temp_shape = getClosedWiresAsFaces(fp.BaseObject)
            if temp_shape and not temp_shape.isNull():
                shape_conv = temp_shape
            else:
                shape_conv = fp.BaseObject.Shape.copy()

            if shape_conv and not shape_conv.isNull():
                baseShapes.append(shape_conv)

        # Gather BaseObjects (the list)
        if fp.BaseObjects:
            for bobj in (fp.BaseObjects or []):
                if bobj:
                    temp_face = getClosedWiresAsFaces(bobj)
                    if temp_face and not temp_face.isNull():
                        face_shape = temp_face
                    else:
                        face_shape = bobj.Shape.copy()
                    if face_shape and not face_shape.isNull():
                        baseShapes.append(face_shape)

        if not baseShapes:
            fp.Shape = Part.makeCompound([])
            return
        validBaseShapes = [sh for sh in baseShapes if sh and not sh.isNull()]
        if not validBaseShapes:
            fp.Shape = Part.makeCompound([])
            return

        distMode = fp.DistributionMode
        autoScale = fp.AutoScaleToFitBase
        scaleVal = fp.PatternScale
        rotVal = fp.RotationDeg
        spacingVal = fp.BaseSpacing
        if fp.UseUnits:
            spacingVal = convertBaseSpacingValue(spacingVal, fp.UseUnits, fp.SelectedUnitSystem)

        rx = fp.RepetitionsX
        ry = fp.RepetitionsY
        randPlace = fp.RandomizePlacement
        randOff = fp.RandomOffsetRange
        rotationMin = fp.RandomRotationMin
        rotationMax = fp.RandomRotationMax
        randSmin = fp.RandomScaleMin
        randSmax = fp.RandomScaleMax
        radialCount = fp.RadialCount
        radialRadius = fp.RadialRadius
        concentricCount = fp.ConcentricCount
        concentricSpace = fp.ConcentricSpacing
        randomCount = fp.RandomCount
        pOffX = fp.PatternOffsetX
        pOffY = fp.PatternOffsetY
        scaleMode = fp.ScaleMode
        tileObj = fp.BaseTileObject if hasattr(fp, "BaseTileObject") else None
        tileVisibility = fp.TileVisibility
        showFaces = fp.ShowFaces
        apply3D = fp.ApplyTo3DSurface
        maxTiles = fp.MaxTilesAllowed
        densityFactor = fp.DensityFactor
        enableColorVar = fp.EnableColorVariation
        colorVarInt = fp.ColorVariationIntensity
        spacingVariation = fp.SpacingVariation
        shapeDistortion = fp.EnableShapeDistortion
        
        # Surface projection properties
        useSurfaceProjection = getattr(fp, "UseSurfaceProjection", True)
        forceXYPlane = getattr(fp, "ForceXYPlane", False)

        # Subtractions
        subObjs = fp.Subtractions

        # Decide pattern shape
        if fp.PatternType == "SolidFill":
            combined = Part.makeCompound(validBaseShapes)
            fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
            fp.TileCount = len(validBaseShapes)
            tileObjName = fp.Name + "_TileRep"
            existingTileObj = doc.getObject(tileObjName)
            if existingTileObj:
                doc.removeObject(existingTileObj.Name)
        else:
            #  If PatternType == "CustomObject", gather from PatternObject + PatternObjects
            if fp.PatternType == "CustomObject":
                allPatternShapes = []
                if fp.PatternObject and hasattr(fp.PatternObject, "Shape"):
                    pat_shp = getClosedWiresAsFaces(fp.PatternObject) if showFaces else fp.PatternObject.Shape.copy()
                    if pat_shp and not pat_shp.isNull():
                        allPatternShapes.append(pat_shp)

                if fp.PatternObjects:
                    for pobj in fp.PatternObjects:
                        if pobj and hasattr(pobj, "Shape"):
                            pat2 = getClosedWiresAsFaces(pobj) if showFaces else pobj.Shape.copy()
                            if pat2 and not pat2.isNull():
                                allPatternShapes.append(pat2)

                if not allPatternShapes:
                    fp.Shape = Part.makeCompound([])
                    return
                if len(allPatternShapes) == 1:
                    patternShape = allPatternShapes[0]
                else:
                    patternShape = allPatternShapes[0]
                    for extraShape in allPatternShapes[1:]:
                        patternShape = patternShape.fuse(extraShape)
            else:
                patternShape = generateBuiltInPatternShape(fp.PatternType)

            if not patternShape or patternShape.isNull():
                fp.Shape = Part.makeCompound([])
                return

            total_tiles = 0
            finalParts = []
            
            for bShape in validBaseShapes:
                try:
                    # Check if we should use surface projection
                    use_projection = (useSurfaceProjection and not forceXYPlane and 
                                      bShape.Faces and len(bShape.Faces) > 0)
                    
                    if use_projection:
                        # Get the first face for transformation
                        target_face = bShape.Faces[0]
                        
                        # Build surface transforms
                        to_local, to_world = buildSurfaceTransforms(target_face)
                        
                        # Project base shape into local 2D coordinates
                        localBase = projectShapeToLocal(bShape, to_local)
                        
                        # Generate hatch in local 2D space
                        localHatch, tiles = buildHatchShape(
                            baseShape=localBase,
                            patternShape=patternShape,
                            distributionMode=distMode,
                            autoScaleToFitBase=autoScale,
                            patternScale=scaleVal,
                            rotationDeg=rotVal,
                            baseSpacing=spacingVal,
                            repX=rx,
                            repY=ry,
                            randRotMin=rotationMin,
                            randRotMax=rotationMax,
                            randomizePlacement=randPlace,
                            randomOffsetRange=randOff,
                            randomScaleMin=randSmin,
                            randomScaleMax=randSmax,
                            radialCount=radialCount,
                            radialRadius=radialRadius,
                            concentricCount=concentricCount,
                            concentricSpacing=concentricSpace,
                            randomCount=randomCount,
                            offsetX=pOffX,
                            offsetY=pOffY,
                            scaleMode=scaleMode,
                            tileShape=(tileObj.Shape if tileObj else None),
                            tileVisibility=tileVisibility,
                            showFaces=showFaces,
                            maxTiles=maxTiles,
                            densityFactor=densityFactor,
                            enableColorVar=enableColorVar,
                            colorVarInt=colorVarInt,
                            spacingVariation=spacingVariation,
                            shapeDistortion=shapeDistortion,
                            apply3D=apply3D,
                            placement_mode=fp.PatternPlacementMode,
                            clipMode=fp.ClipMode
                        )
                        
                        # Unproject back to world coordinates
                        shaped = unprojectShapeToWorld(localHatch, to_world)
                        total_tiles += tiles
                        finalParts.append(shaped)
                    else:
                        # Original XY-plane path
                        shaped, tiles = buildHatchShape(
                            baseShape=bShape,
                            patternShape=patternShape,
                            distributionMode=distMode,
                            autoScaleToFitBase=autoScale,
                            patternScale=scaleVal,
                            rotationDeg=rotVal,
                            baseSpacing=spacingVal,
                            repX=rx,
                            repY=ry,
                            randRotMin=rotationMin,
                            randRotMax=rotationMax,
                            randomizePlacement=randPlace,
                            randomOffsetRange=randOff,
                            randomScaleMin=randSmin,
                            randomScaleMax=randSmax,
                            radialCount=radialCount,
                            radialRadius=radialRadius,
                            concentricCount=concentricCount,
                            concentricSpacing=concentricSpace,
                            randomCount=randomCount,
                            offsetX=pOffX,
                            offsetY=pOffY,
                            scaleMode=scaleMode,
                            tileShape=(tileObj.Shape if tileObj else None),
                            tileVisibility=tileVisibility,
                            showFaces=showFaces,
                            maxTiles=maxTiles,
                            densityFactor=densityFactor,
                            enableColorVar=enableColorVar,
                            colorVarInt=colorVarInt,
                            spacingVariation=spacingVariation,
                            shapeDistortion=shapeDistortion,
                            apply3D=apply3D,
                            placement_mode=fp.PatternPlacementMode,
                            clipMode=fp.ClipMode
                        )
                        total_tiles += tiles
                        finalParts.append(shaped)
                except Exception as e:
                    FreeCAD.Console.PrintError(f"Error building hatch on base shape: {str(e)}\n")

            if not finalParts:
                combined = Part.makeCompound([])
            else:
                combined = finalParts[0]
                for c in finalParts[1:]:
                    combined = combined.fuse(c)

            fp.TileCount = total_tiles

            # Cleanup tile objects if any
            tileObjName = fp.Name + "_TileRep"
            existingTileObj = doc.getObject(tileObjName)

            if tileVisibility and tileObj and not tileObj.Shape.isNull():
                if not existingTileObj:
                    existingTileObj = doc.addObject("Part::Feature", tileObjName)
                doc.openTransaction("Update Tiles")
                try:
                    repeatedTile, _ = buildHatchShape(
                        baseShape=validBaseShapes[0],
                        patternShape=tileObj.Shape,
                        distributionMode=distMode,
                        autoScaleToFitBase=autoScale,
                        patternScale=scaleVal,
                        rotationDeg=rotVal,
                        baseSpacing=spacingVal,
                        repX=rx,
                        repY=ry,
                        randRotMin=rotationMin,
                        randRotMax=rotationMax,
                        randomizePlacement=randPlace,
                        randomOffsetRange=randOff,
                        randomScaleMin=randSmin,
                        randomScaleMax=randSmax,
                        offsetX=pOffX,
                        offsetY=pOffY,
                        scaleMode=scaleMode,
                        tileShape=None,
                        tileVisibility=False,
                        showFaces=False,
                        maxTiles=maxTiles,
                        densityFactor=densityFactor,
                        enableColorVar=False,
                        colorVarInt=0.0,
                        spacingVariation=spacingVariation,
                        shapeDistortion=False,
                        apply3D=apply3D,
                        placement_mode=fp.PatternPlacementMode
                    )
                    existingTileObj.Shape = repeatedTile
                    applyTileViewOverrides(existingTileObj, existingTileObj.ViewObject)
                finally:
                    doc.commitTransaction()
            else:
                if existingTileObj:
                    doc.removeObject(existingTileObj.Name)

        # === Apply subtractions (for either SolidFill or normal)
        if subObjs:
            for so in subObjs:
                if so and hasattr(so, "Shape") and not so.Shape.isNull():
                    try:
                        combined = combined.cut(so.Shape)
                    except Exception as e:
                        FreeCAD.Console.PrintError(f"Error subtracting {so.Name}: {str(e)}\n")

        fp.Shape = combined
        fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
        if fp.PatternType == "SolidFill":
            fp.TileCount = len(validBaseShapes)

    def onDocumentRestored(self, obj):
        """Handle document restoration"""
        obj.Proxy = self
        self._is_recomputing = False

# ------------------------------------------
# Updated buildHatchShape function
# ------------------------------------------
def buildHatchShape(baseShape,
                    patternShape,
                    distributionMode,
                    autoScaleToFitBase,
                    patternScale,
                    rotationDeg,
                    baseSpacing,
                    repX,
                    repY,
                    randRotMin,
                    randRotMax,
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
    if baseShape.isNull() or patternShape.isNull():
        return Part.makeCompound([]), 0

    normedPattern, pBB = normalizePatternShape(patternShape)
    patternW = pBB.XMax - pBB.XMin
    patternH = pBB.YMax - pBB.YMin

    if tileShape and not tileShape.isNull():
        normedTile, tileBB = normalizePatternShape(tileShape)
        if placement_mode != "Origin":
            tile_w = tileBB.XLength
            tile_h = tileBB.YLength
            if placement_mode == "Center":
                dx = -tile_w/2
                dy = -tile_h/2
            elif placement_mode == "TopLeft":
                dx = 0
                dy = -tile_h
            elif placement_mode == "TopRight":
                dx = -tile_w
                dy = -tile_h
            elif placement_mode == "BottomLeft":
                dx = 0
                dy = 0
            elif placement_mode == "BottomRight":
                dx = -tile_w
                dy = 0
            elif placement_mode == "TopCenter":
                dx = -tile_w/2
                dy = -tile_h
            elif placement_mode == "BottomCenter":
                dx = -tile_w/2
                dy = 0
            elif placement_mode == "LeftCenter":
                dx = 0
                dy = -tile_h/2
            elif placement_mode == "RightCenter":
                dx = -tile_w
                dy = -tile_h/2
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

    bBB = baseShape.BoundBox
    w   = bBB.XMax - bBB.XMin
    h   = bBB.YMax - bBB.YMin

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

    def getSpacing(base, iterationX=0, iterationY=0):
        if spacingVariation <= 0.0:
            return base
        rand_factor = random.uniform(-base*spacingVariation, base*spacingVariation)
        return base + rand_factor

    def fuseShapes(shapeList):
        if not shapeList:
            return Part.makeCompound([])
        fused = shapeList[0]
        for shp in shapeList[1:]:
            fused = fused.fuse(shp)
        if not fused.isValid():
            fused = fused.removeSplitter()
        return fused

    def tileAndClip(tileX, tileY, baseScale, baseRotRad, placement_mode):
        nonlocal tileCount
        halfW = (tileW * baseScale)*0.5
        halfH = (tileH * baseScale)*0.5
        minx = tileX - halfW
        maxx = tileX + halfW
        miny = tileY - halfH
        maxy = tileY + halfH

        if (maxx < bBB.XMin or minx > bBB.XMax or maxy < bBB.YMin or miny > bBB.YMax):
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

    if distributionMode == "RandomDistribution":
        randomCount = int(randomCount * densityFactor)
        if randomCount < 1:
            randomCount = 1

    if distributionMode == "CenteredTiling":
        startX = (bBB.XMin + bBB.XMax)*0.5
        startY = (bBB.YMin + bBB.YMax)*0.5
        for ix in range(repX):
            for iy in range(repY):
                if tileCount >= maxTiles:
                    break
                spacingX = getSpacing(baseSpacing, ix, iy)
                spacingY = getSpacing(baseSpacing, ix, iy)
                x = startX - ((repX-1)*spacingX*0.5) + ix*spacingX + offsetX
                y = startY - ((repY-1)*spacingY*0.5) + iy*spacingY + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "RelativeSpacing":
        minDim = min(w,h)
        spacing = (baseSpacing/100.0)*minDim
        for ix in range(repX):
            for iy in range(repY):
                if tileCount >= maxTiles:
                    break
                spacingX = getSpacing(spacing, ix, iy)
                x = (bBB.XMin + bBB.XMax)*0.5 - ((repX-1)*spacingX*0.5) + ix*spacingX + offsetX
                spacingY = getSpacing(spacing, ix, iy)
                y = (bBB.YMin + bBB.YMax)*0.5 - ((repY-1)*spacingY*0.5) + iy*spacingY + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "SeamlessTiling":
        if tileW < 1e-9 or tileH < 1e-9:
            return Part.makeCompound([]), 0
        neededX = max(1, int(math.ceil(w/(tileW*patternScale))))
        neededY = max(1, int(math.ceil(h/(tileH*patternScale))))
        for ix in range(neededX):
            for iy in range(neededY):
                if tileCount >= maxTiles:
                    break
                x = bBB.XMin + ix*(tileW*patternScale) + offsetX
                y = bBB.YMin + iy*(tileH*patternScale) + offsetY
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
                x = bBB.XMin + ix*dx + offsetX
                y = bBB.YMin + iy*dy + offsetY
                shp, color = tileAndClip(x, y, patternScale, rotRad, placement_mode)
                if shp and not shp.isNull():
                    patternList.append(shp)
            if tileCount >= maxTiles:
                break

    elif distributionMode == "RadialDistribution":
        cx = 0.5*(bBB.XMax + bBB.XMin) + offsetX
        cy = 0.5*(bBB.YMax + bBB.YMin) + offsetY
        for i in range(radialCount):
            if tileCount >= maxTiles:
                break
            angle = i*(2.0*math.pi/float(radialCount))
            x = cx + radialRadius*math.cos(angle)
            y = cy + radialRadius*math.sin(angle)
            shp, color = tileAndClip(x, y, patternScale, rotRad + angle, placement_mode)
            if shp and not shp.isNull():
                patternList.append(shp)

    elif distributionMode == "ConcentricDistribution":
        cx = 0.5*(bBB.XMax + bBB.XMin) + offsetX
        cy = 0.5*(bBB.YMax + bBB.YMin) + offsetY
        for ring in range(concentricCount):
            radius = (ring+1)*concentricSpacing
            countRing = ring + 3
            for i in range(countRing):
                if tileCount >= maxTiles:
                    break
                angle = i*(2.0*math.pi/float(countRing))
                x = cx + radius*math.cos(angle)
                y = cy + radius*math.sin(angle)
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
            keepCount = int(len(perimeterPoints)*densityFactor)
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

    finalShape = fuseShapes(patternList)
    return finalShape, tileCount

def mapPatternTo3DSurface(baseShape, patternShape):
    """Placeholder - Implement actual UV mapping here"""
    return patternShape

def normalizePatternShape(patternShape):
    bb = patternShape.BoundBox
    dx = -bb.XMin
    dy = -bb.YMin
    mat = FreeCAD.Matrix()
    mat.move(FreeCAD.Vector(dx, dy, 0))
    newShape = patternShape.copy()
    newShape.transformShape(mat)
    return newShape, newShape.BoundBox

def separateFacesAndEdges(s):
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

def clipShapeToBase(baseShape, repeatedShape, clipMode):
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

def shapeToEdges(shp):
    edges = shp.Edges
    if not edges:
        return Part.makeCompound([])
    return Part.makeCompound(edges)

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
    cpy = normedPattern.copy()
    bb = cpy.BoundBox
    w = bb.XLength
    h = bb.YLength

    finalScale = baseScale
    if randomize and randSmax >= randSmin and randSmin > 0:
        scaleFactor = random.uniform(max(0.0001, randSmin), randSmax)
        finalScale *= scaleFactor

    matScale = FreeCAD.Matrix()
    matScale.scale(finalScale, finalScale, finalScale)
    cpy.transformShape(matScale)

    finalRot = baseRotRad
    if randomize and randRotMax > randRotMin:
        finalRot += math.radians(random.uniform(randRotMin, randRotMax))

    matRot = FreeCAD.Matrix()
    matRot.rotateZ(finalRot)
    cpy.transformShape(matRot)

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
    else:
        dx, dy = 0, 0

    mat_placement = FreeCAD.Matrix()
    mat_placement.move(FreeCAD.Vector(dx, dy, 0))
    cpy.transformShape(mat_placement)

    if randomize and randOffRange > 0:
        jitter_x = random.uniform(-randOffRange, randOffRange)
        jitter_y = random.uniform(-randOffRange, randOffRange)
        matTrans = FreeCAD.Matrix()
        matTrans.move(FreeCAD.Vector(jitter_x, jitter_y, 0))
        cpy.transformShape(matTrans)

    matTrans = FreeCAD.Matrix()
    matTrans.move(FreeCAD.Vector(tileX, tileY, 0))
    cpy.transformShape(matTrans)

    if showFaces:
        try:
            # Attempt to create faces for each sub-shape if possible
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

    try:
        result = clipShapeToBase(baseShape, cpy, clipMode)
    except Exception as e:
        FreeCAD.Console.PrintError(f"Boolean operation error: {str(e)}\n")
        return None, None

    randomColor = None
    if enableColorVar:
        import colorsys
        hue = random.random()
        sat = 0.5 + 0.5 * colorVarInt
        val = 1.0
        randomColor = colorsys.hsv_to_rgb(hue, sat, val)

    return result, randomColor

# ================================
# ViewProvider
# ================================
class CustomHatchViewProvider:
    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def setupContextMenu(self, obj, menu):
        action_copy = menu.addAction("Copy Hatch")
        action_copy.triggered.connect(lambda: self.copyHatch(obj.Object))
        action_duplicate = menu.addAction("Duplicate Hatch")
        action_duplicate.triggered.connect(lambda: self.duplicateHatch(obj.Object))
        action_remove = menu.addAction("Remove Hatch")
        action_remove.triggered.connect(lambda: self.removeHatch(obj.Object))

    def copyHatch(self, doc_obj):
        try:
            new_name = "CopyOf_" + doc_obj.Name
            new_obj = makeCustomHatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Copy failed: {str(e)}\n")

    def duplicateHatch(self, doc_obj):
        try:
            new_name = FreeCAD.ActiveDocument.getUniqueObjectName("Hatch")
            new_obj = makeCustomHatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Duplicate failed: {str(e)}\n")

    def removeHatch(self, doc_obj):
        try:
            FreeCAD.ActiveDocument.removeObject(doc_obj.Name)
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Remove failed: {str(e)}\n")

    def doubleClicked(self, vobj):
        # FIXED: Use the correct command name
        FreeCADGui.runCommand('BIM_Hatch_Dialog')
        return True

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def getDisplayModes(self, obj):
        return ["Flat Lines", "Wireframe", "Shaded"]

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, vobj, prop):
        pass

    def getIcon(self):
        # FIXED: Use FreeCAD's built-in icon to avoid XPM parsing errors
        return ":/icons/Draft_Hatch.svg"

    def __getstate__(self):
        return {"Object": self.Object.Name}

    def __setstate__(self, state):
        if "Object" in state:
            self.Object = FreeCAD.ActiveDocument.getObject(state["Object"])
        return None

def makeCustomHatch(name="CustomHatchFP"):
    doc = FreeCAD.ActiveDocument
    obj = doc.addObject("Part::FeaturePython", name)
    CustomHatchFeature(obj)
    CustomHatchViewProvider(obj.ViewObject)
    return obj


# ============================================================================
# ======================== FIXED UI: TASK PANEL VERSION ======================
# ============================================================================

class HatchTaskPanel:
    """
    Task panel for the FreeCAD Hatch Generator.
    This class creates the UI widget and handles the creation logic.
    It replaces the old QDialog.
    """
    def __init__(self):
        FreeCAD.Console.PrintMessage("Initializing Hatch Generator Task Panel...\n")
        self.doc = FreeCAD.ActiveDocument
        if not self.doc:
            QtWidgets.QMessageBox.warning(None, "Error", "No active document found.")
            return

        self.current_temp_preview_name = None
        self.masterObjectList = self.getAllObjectsClassified()
        self.last_manual_scale = None
        
        # Build the UI Form (the widget displayed in the Task panel)
        self.form = self.createUI()
        
        # Initial setup
        self.initialPatternSetup()
        
    # ------------------------------------------------------------------------
    # UI Creation
    # ------------------------------------------------------------------------
    def createUI(self):
        # Main container widget
        main_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(main_widget)
        
        # Create a scroll area to contain the tabs
        # This is the crucial fix for the scroll issue
        scroll_area = QtWidgets.QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        scroll_area.setFrameShape(QtWidgets.QFrame.NoFrame)
        
        # Container for scrollable content
        scroll_content = QtWidgets.QWidget()
        scroll_layout = QtWidgets.QVBoxLayout(scroll_content)
        
        # Tabs
        self.tabs = QtWidgets.QTabWidget()
        # FIX: Add minimum height and size policy to prevent cut-off
        self.tabs.setMinimumHeight(450)
        self.tabs.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.mainTab = QtWidgets.QWidget()
        self.mainTabLayout = QtWidgets.QFormLayout(self.mainTab)
        self.advancedTab = QtWidgets.QWidget()
        self.advancedTabLayout = QtWidgets.QVBoxLayout(self.advancedTab)
        self.tabs.addTab(self.mainTab, "Main")
        self.tabs.addTab(self.advancedTab, "Advanced")
        scroll_layout.addWidget(self.tabs)

        # --- Instantiate UI Elements ---
        self.baseLabel = QtWidgets.QLabel("Base Shape:")
        self.baseCombo = QtWidgets.QComboBox()
        self.pickBaseBtn = QtWidgets.QPushButton("Pick Base Shape from Selection")
        self.pickBaseBtn.clicked.connect(self.pickBaseShape)
        
        self.baseSearchLabel = QtWidgets.QLabel("Search:")
        self.baseSearchField = QtWidgets.QLineEdit()
        self.baseTypeFilterLabel = QtWidgets.QLabel("Type Filter:")
        self.baseTypeFilterCombo = QtWidgets.QComboBox()
        for cat in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.baseTypeFilterCombo.addItem(cat)
            
        self.baseObjectsLabel = QtWidgets.QLabel("Additional Base Objects:")
        self.baseObjectsList = QtWidgets.QListWidget()
        self.baseObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

        self.patternSourceLabel = QtWidgets.QLabel("Pattern Source:")
        self.patternSourceCombo = QtWidgets.QComboBox()
        self.patternSourceCombo.addItems(["Built-in", "Custom"])
        self.patternSourceCombo.currentIndexChanged.connect(self.onPatternSourceChanged)

        self.builtinPatternLabel = QtWidgets.QLabel("Built-in Pattern:")
        self.builtinPatternCombo = QtWidgets.QComboBox()
        self.builtinPatternCombo.addItems([
            "SolidFill", "HorizontalLines", "VerticalLines", "Crosshatch", "Herringbone",
            "BrickPattern", "RandomDots", "OverlappingSquares", "Checkerboard",
            "CheckerboardCircles", "RotatingHexagons", "NestedTriangles", "InterlockingCircles",
            "RecursiveSquares", "FlowerOfLife", "VoronoiMesh", "OffsetChecker", "ZigZag",
            "HexagonalHoriz", "HexagonalVerti", "HexagonalPattern", "TrianglesGrid",
            "MidEastMosaic", "StarGridPattern", "BasketWeave", "Honeycomb", "SineWave",
            "SpaceFrame", "HoneycombDual", "ArtDeco", "StainedGlass", "PenroseTriangle",
            "GreekKey", "ChainLinks", "TriangleForest", "CeramicTile", "CirclesGrid",
            "PlusSigns", "WavesPattern", "GalaxyStarsPattern", "GridDots", "HexDots",
            "FractalTree", "Voronoi", "FractalBranches", "OrganicMaze", "BiomorphicCells",
            "RadialSunburst", "Sunburst", "Ziggurat", "SpiralPattern", "PentaflakeFractal",
            "HilbertCurve", "SierpinskiTriangle", "PenroseTiling", "EinsteinMonotile",
            "LeafVeins", "WoodPlanks", "ParquetHerringbone", "WoodGrain", "DrywallOrangePeel",
            "DrywallKnockdown", "StuccoSandFloat", "StuccoDash", "DrywallSkipTrowel",
            "Concrete", "ConcreteStampedPattern", "ConcreteSaltFinish", "ConcreteFormTiePattern",
            "ConcreteSandblastPattern", "ConcreteControlJoint", "ConcreteGridPattern",
            "WoodKnotPattern", "ConcreteAggregatePattern", "BrushedConcrete", "PebbleConcrete",
            "CrackedConcrete", "AggregateConcrete", "StampedConcrete", "Insulation", "Rebar", "RoofTiles"
        ])

        self.customPatternLabel = QtWidgets.QLabel("Custom Pattern:")
        self.customPatternCombo = QtWidgets.QComboBox()
        self.pickCustomBtn = QtWidgets.QPushButton("Pick Custom Pattern from Selection")
        self.pickCustomBtn.clicked.connect(self.pickCustomPattern)
        
        self.patternSearchLabel = QtWidgets.QLabel("Search:")
        self.patternSearchField = QtWidgets.QLineEdit()
        self.patternTypeFilterLabel = QtWidgets.QLabel("Type Filter:")
        self.patternTypeFilterCombo = QtWidgets.QComboBox()
        for cat in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.patternTypeFilterCombo.addItem(cat)

        self.patternObjectsLabel = QtWidgets.QLabel("Additional Pattern Objects:")
        self.patternObjectsList = QtWidgets.QListWidget()
        self.patternObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

        self.distLabel = QtWidgets.QLabel("Distribution Mode:")
        self.distCombo = QtWidgets.QComboBox()
        self.distCombo.addItems([
            "CenteredTiling", "RelativeSpacing", "SeamlessTiling",
            "LinearGrid", "RadialDistribution", "ConcentricDistribution",
            "RandomDistribution", "AdaptiveDistribution"
        ])
        self.distCombo.setCurrentText("SeamlessTiling")
        
        self.autoScaleCheck = QtWidgets.QCheckBox("Auto Scale to Fit Base?")
        
        self.scaleLabel = QtWidgets.QLabel("Pattern Scale:")
        self.scaleSpin = QtWidgets.QDoubleSpinBox()
        self.scaleSpin.setRange(0.0001, 1e5)
        self.scaleSpin.setValue(1.0)
        self.scaleSpin.valueChanged.connect(self.onScaleChanged)
        
        self.scaleResetBtn = QtWidgets.QPushButton("↺")
        self.scaleResetBtn.setToolTip("Reset to pattern default scale")
        self.scaleResetBtn.clicked.connect(lambda: self.setSmartDefaultScale(
            self.patternSourceCombo.currentIndex() == 0
        ))
        
        self.rotLabel = QtWidgets.QLabel("Rotation (deg):")
        self.rotSpin = QtWidgets.QDoubleSpinBox()
        self.rotSpin.setRange(-360, 360)
        
        self.spacingLabel = QtWidgets.QLabel("Base Spacing:")
        self.spacingInput = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.spacingInput.setProperty("unitCategory", "Length")
        self.spacingInput.setText("10 mm")
        
        self.repXLabel = QtWidgets.QLabel("Repetitions X:")
        self.repXSpin = QtWidgets.QSpinBox()
        self.repXSpin.setRange(1, 999)
        self.repXSpin.setValue(5)
        self.repYLabel = QtWidgets.QLabel("Repetitions Y:")
        self.repYSpin = QtWidgets.QSpinBox()
        self.repYSpin.setRange(1, 999)
        self.repYSpin.setValue(5)

        self.showFacesCheck = QtWidgets.QCheckBox("Show Faces?")
        self.showFacesCheck.setChecked(False)
        
        self.subtractionsLabel = QtWidgets.QLabel("Subtractions (cut these out):")
        self.subtractionsList = QtWidgets.QListWidget()
        self.subtractionsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)
        self.pickSubBtn = QtWidgets.QPushButton("Pick Subtractions from Selection")
        self.pickSubBtn.clicked.connect(self.pickSubtractions)

        # Advanced Tab Elements
        self.tileLabel = QtWidgets.QLabel("Base Tile (Optional):")
        self.tileCombo = QtWidgets.QComboBox()
        self.tileCombo.addItem("")
        self.pickTileBtn = QtWidgets.QPushButton("Pick Base Tile from Selection")
        self.pickTileBtn.clicked.connect(self.pickBaseTile)
        
        self.tileVisibilityCheck = QtWidgets.QCheckBox("Tile Visibility?")
        self.tileVisibilityCheck.setChecked(True)

        self.placementModeLabel = QtWidgets.QLabel("Pattern Placement Mode:")
        self.placementModeCombo = QtWidgets.QComboBox()
        self.placementModeCombo.addItems([
            "Origin", "Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight",
            "TopCenter", "BottomCenter", "LeftCenter", "RightCenter", "Custom"
        ])
        self.placementModeCombo.setCurrentText("Origin")

        self.lockCheck = QtWidgets.QCheckBox("Lock to Base?")
        
        self.offsetXLabel = QtWidgets.QLabel("Pattern Offset X:")
        self.offsetXSpin = QtWidgets.QDoubleSpinBox()
        self.offsetXSpin.setRange(-1e5, 1e5)
        self.offsetYLabel = QtWidgets.QLabel("Pattern Offset Y:")
        self.offsetYSpin = QtWidgets.QDoubleSpinBox()
        self.offsetYSpin.setRange(-1e5, 1e5)
        
        self.scaleModeLabel = QtWidgets.QLabel("Scale Mode:")
        self.scaleModeCombo = QtWidgets.QComboBox()
        self.scaleModeCombo.addItems(["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"])
        
        self.randomCheck = QtWidgets.QCheckBox("Randomize Placement?")
        self.offRangeLabel = QtWidgets.QLabel("Random Offset ±:")
        self.offRangeSpin = QtWidgets.QDoubleSpinBox()
        self.offRangeSpin.setRange(0.0, 1e4)
        self.rotRangeLabel = QtWidgets.QLabel("Random Rot ± (deg):")
        self.rotRangeSpin = QtWidgets.QDoubleSpinBox()
        self.rotRangeSpin.setRange(0.0, 360.0)
        self.scaleMinLabel = QtWidgets.QLabel("Random Scale Min:")
        self.scaleMinSpin = QtWidgets.QDoubleSpinBox()
        self.scaleMinSpin.setRange(0.01, 1e4)
        self.scaleMinSpin.setValue(1.0)
        self.scaleMaxLabel = QtWidgets.QLabel("Random Scale Max:")
        self.scaleMaxSpin = QtWidgets.QDoubleSpinBox()
        self.scaleMaxSpin.setRange(0.01, 1e4)
        self.scaleMaxSpin.setValue(1.0)
        
        self.apply3DCheck = QtWidgets.QCheckBox("Apply to 3D Surface?")
        self.maxTilesLabel = QtWidgets.QLabel("Max Tiles Allowed:")
        self.maxTilesSpin = QtWidgets.QSpinBox()
        self.maxTilesSpin.setRange(1, 999999)
        self.maxTilesSpin.setValue(1000)
        
        self.densityLabel = QtWidgets.QLabel("Density Factor (0-1):")
        self.densitySpin = QtWidgets.QDoubleSpinBox()
        self.densitySpin.setRange(0.0, 1.0)
        self.densitySpin.setValue(1.0)
        
        self.enableColorVarCheck = QtWidgets.QCheckBox("Enable Color Variation?")
        self.colorVarLabel = QtWidgets.QLabel("Color Variation Intensity (0-1):")
        self.colorVarSpin = QtWidgets.QDoubleSpinBox()
        self.colorVarSpin.setRange(0.0, 1.0)
        self.colorVarSpin.setValue(0.5)

        self.clipModeLabel = QtWidgets.QLabel("Clip Mode:")
        self.clipModeCombo = QtWidgets.QComboBox()
        self.clipModeCombo.addItems(["BooleanOnly", "PreserveLinesNoClip"])

        # Surface Projection Group
        self.surfaceProjectionGroup = QtWidgets.QGroupBox("Surface Projection")
        self.surfaceProjectionLayout = QtWidgets.QFormLayout(self.surfaceProjectionGroup)
        self.useSurfaceProjectionCheck = QtWidgets.QCheckBox("Use Surface Projection (works on walls, sloped roofs)")
        self.useSurfaceProjectionCheck.setChecked(True)
        self.forceXYPlaneCheck = QtWidgets.QCheckBox("Force XY Plane (old behavior)")
        self.forceXYPlaneCheck.setChecked(False)
        self.surfaceProjectionLayout.addRow(self.useSurfaceProjectionCheck)
        self.surfaceProjectionLayout.addRow(self.forceXYPlaneCheck)

        # Preview Button
        self.previewBtn = QtWidgets.QPushButton("Preview")
        self.previewBtn.clicked.connect(self.onPreview)

        # --- Populate Main Tab Layout ---
        baseRowLayout = QtWidgets.QHBoxLayout()
        baseRowLayout.addWidget(self.baseSearchLabel)
        baseRowLayout.addWidget(self.baseSearchField)
        baseRowLayout.addWidget(self.baseTypeFilterLabel)
        baseRowLayout.addWidget(self.baseTypeFilterCombo)

        self.mainTabLayout.addRow(self.baseLabel, self.baseCombo)
        self.mainTabLayout.addRow("", self.pickBaseBtn)
        self.mainTabLayout.addRow(baseRowLayout)
        self.mainTabLayout.addRow(self.baseObjectsLabel, self.baseObjectsList)
        self.mainTabLayout.addRow(self.patternSourceLabel, self.patternSourceCombo)
        self.mainTabLayout.addRow(self.builtinPatternLabel, self.builtinPatternCombo)
        
        patternRowLayout = QtWidgets.QHBoxLayout()
        patternRowLayout.addWidget(self.patternSearchLabel)
        patternRowLayout.addWidget(self.patternSearchField)
        patternRowLayout.addWidget(self.patternTypeFilterLabel)
        patternRowLayout.addWidget(self.patternTypeFilterCombo)
        self.mainTabLayout.addRow(self.customPatternLabel, self.customPatternCombo)
        self.mainTabLayout.addRow("", self.pickCustomBtn)
        self.mainTabLayout.addRow(patternRowLayout)
        self.mainTabLayout.addRow(self.patternObjectsLabel, self.patternObjectsList)
        
        self.mainTabLayout.addRow(self.distLabel, self.distCombo)
        self.mainTabLayout.addRow(self.autoScaleCheck)
        
        scaleRow = QtWidgets.QHBoxLayout()
        scaleRow.addWidget(self.scaleSpin)
        scaleRow.addWidget(self.scaleResetBtn)
        self.mainTabLayout.addRow(self.scaleLabel, scaleRow)
        
        self.mainTabLayout.addRow(self.rotLabel, self.rotSpin)
        self.mainTabLayout.addRow(self.spacingLabel, self.spacingInput)
        self.mainTabLayout.addRow(self.repXLabel, self.repXSpin)
        self.mainTabLayout.addRow(self.repYLabel, self.repYSpin)
        self.mainTabLayout.addRow(self.showFacesCheck)
        self.mainTabLayout.addRow(self.subtractionsLabel, self.subtractionsList)
        self.mainTabLayout.addRow("", self.pickSubBtn)

        # --- Populate Advanced Tab Layout ---
        advForm = QtWidgets.QFormLayout()
        self.advancedTabLayout.addLayout(advForm)
        
        tileGroup = QtWidgets.QGroupBox("Tile Settings")
        tileForm = QtWidgets.QFormLayout(tileGroup)
        tileForm.addRow(self.tileLabel, self.tileCombo)
        tileForm.addRow("", self.pickTileBtn)
        tileForm.addRow(self.tileVisibilityCheck)
        advForm.addRow(tileGroup)
        
        placementGroup = QtWidgets.QGroupBox("Placement")
        placementForm = QtWidgets.QFormLayout(placementGroup)
        placementForm.addRow(self.placementModeLabel, self.placementModeCombo)
        placementForm.addRow(self.lockCheck)
        placementForm.addRow(self.offsetXLabel, self.offsetXSpin)
        placementForm.addRow(self.offsetYLabel, self.offsetYSpin)
        placementForm.addRow(self.scaleModeLabel, self.scaleModeCombo)
        advForm.addRow(placementGroup)
        
        randGroup = QtWidgets.QGroupBox("Randomization")
        randForm = QtWidgets.QFormLayout(randGroup)
        randForm.addRow(self.randomCheck)
        randForm.addRow(self.offRangeLabel, self.offRangeSpin)
        randForm.addRow(self.rotRangeLabel, self.rotRangeSpin)
        randForm.addRow(self.scaleMinLabel, self.scaleMinSpin)
        randForm.addRow(self.scaleMaxLabel, self.scaleMaxSpin)
        advForm.addRow(randGroup)
        
        renderGroup = QtWidgets.QGroupBox("Rendering & Performance")
        renderForm = QtWidgets.QFormLayout(renderGroup)
        renderForm.addRow(self.clipModeLabel, self.clipModeCombo)
        renderForm.addRow(self.apply3DCheck)
        renderForm.addRow(self.maxTilesLabel, self.maxTilesSpin)
        advForm.addRow(renderGroup)
        
        variationGroup = QtWidgets.QGroupBox("Variation")
        variationForm = QtWidgets.QFormLayout(variationGroup)
        variationForm.addRow(self.densityLabel, self.densitySpin)
        variationForm.addRow(self.enableColorVarCheck)
        variationForm.addRow(self.colorVarLabel, self.colorVarSpin)
        advForm.addRow(variationGroup)
        
        advForm.addRow(self.surfaceProjectionGroup)
        advForm.addRow(self.previewBtn)

        # Set scroll area widget and add to main layout
        scroll_area.setWidget(scroll_content)
        main_layout.addWidget(scroll_area)

        # Connect signals
        self.baseCombo.currentIndexChanged.connect(self.onBaseComboIndexChanged)
        self.baseTypeFilterCombo.currentIndexChanged.connect(self.refreshBaseCombo)
        self.baseSearchField.textChanged.connect(lambda: QtCore.QTimer.singleShot(300, self.refreshBaseCombo))
        
        self.patternTypeFilterCombo.currentIndexChanged.connect(self.refreshCustomPatternCombo)
        self.patternSearchField.textChanged.connect(lambda: QtCore.QTimer.singleShot(300, self.refreshCustomPatternCombo))
        self.customPatternCombo.currentIndexChanged.connect(self.onCustomPatternSelected)
        self.tileCombo.currentIndexChanged.connect(self.onTileComboIndexChanged)
        
        # Initial population
        self.refreshBaseCombo()
        self.refreshCustomPatternCombo()
        self.populateMultiList(self.baseObjectsList)
        self.populateMultiList(self.patternObjectsList)
        self.populateMultiList(self.subtractionsList)
        self.populateTileCombo()
        
        return main_widget

    # ------------------------------------------------------------------------
    # Logic Methods (Adapted from original HatchGeneratorDialog)
    # ------------------------------------------------------------------------
    def getAllObjectsClassified(self):
        data = []
        if self.doc:
            for obj in self.doc.Objects:
                if not hasattr(obj, "Shape"):
                    continue
                cat = self.classifyObject(obj)
                data.append((obj.Name, cat, obj))
        return data

    def classifyObject(self, obj):
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return "Sketch"
        elif obj.isDerivedFrom("Part::Compound"):
            return "Compound"
        elif obj.isDerivedFrom("Part::Feature"):
            return "PartFeature"
        else:
            return "Other"

    def filterObjects(self, searchText, category):
        results = []
        lowSearch = searchText.lower().strip()
        for name, cat, obj in self.masterObjectList:
            if category != "All" and cat != category:
                continue
            if lowSearch and lowSearch not in name.lower():
                continue
            results.append((name, cat, obj))
        return results

    def refreshBaseCombo(self):
        sText = self.baseSearchField.text()
        cText = self.baseTypeFilterCombo.currentText()
        flist = self.filterObjects(sText, cText)
        self.baseCombo.clear()
        for (nm, cat, o) in flist:
            self.baseCombo.addItem(nm)

    def refreshCustomPatternCombo(self):
        sText = self.patternSearchField.text()
        cText = self.patternTypeFilterCombo.currentText()
        filtered = self.filterObjects(sText, cText)
        self.customPatternCombo.clear()
        self.customPatternCombo.addItem("")
        for (name, category, obj) in filtered:
            self.customPatternCombo.addItem(name)

    def populateMultiList(self, listWidget):
        listWidget.clear()
        for (nm, cat, obj) in self.masterObjectList:
            listWidget.addItem(nm)

    def populateTileCombo(self):
        self.tileCombo.clear()
        self.tileCombo.addItem("")
        for (nm, cat, obj) in self.masterObjectList:
            self.tileCombo.addItem(nm)

    def getSelectedObjectsFromList(self, listWidget):
        selectedItems = listWidget.selectedItems()
        selectedObjs = []
        for it in selectedItems:
            objName = it.text()
            fcObj = self.doc.getObject(objName)
            if fcObj:
                selectedObjs.append(fcObj)
        return selectedObjs

    def initialPatternSetup(self):
        is_builtin = self.patternSourceCombo.currentIndex() == 0
        self.setSmartDefaultScale(is_builtin)
        self.last_manual_scale = None
        self._update_pattern_controls_visibility(show_builtin=is_builtin)

    def onPatternSourceChanged(self, index):
        is_builtin = (index == 0)
        if self.last_manual_scale is None:
            self.setSmartDefaultScale(is_builtin)
        else:
            self.scaleSpin.setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; }")
        self._update_pattern_controls_visibility(show_builtin=is_builtin)

    def _update_pattern_controls_visibility(self, show_builtin=True):
        self.builtinPatternLabel.setVisible(show_builtin)
        self.builtinPatternCombo.setVisible(show_builtin)
        show_custom = not show_builtin
        self.customPatternLabel.setVisible(show_custom)
        self.customPatternCombo.setVisible(show_custom)
        self.patternSearchLabel.setVisible(show_custom)
        self.patternSearchField.setVisible(show_custom)
        self.patternTypeFilterLabel.setVisible(show_custom)
        self.patternTypeFilterCombo.setVisible(show_custom)
        self.pickCustomBtn.setVisible(show_custom)
        self.patternObjectsLabel.setVisible(show_custom)
        self.patternObjectsList.setVisible(show_custom)

    def setSmartDefaultScale(self, is_builtin):
        default_scale = 20.0 if is_builtin else 1.0
        self.scaleSpin.setValue(default_scale)
        self.last_manual_scale = None
        self.scaleSpin.setStyleSheet("")

    def onScaleChanged(self, value):
        self.last_manual_scale = value
        self.scaleSpin.setStyleSheet("")

    def onBaseComboIndexChanged(self, index):
        name = self.baseCombo.itemText(index)
        self.highlightObject(name)

    def onTileComboIndexChanged(self, index):
        name = self.tileCombo.itemText(index)
        if name:
            self.highlightObject(name)

    def onCustomPatternSelected(self):
        name = self.customPatternCombo.currentText()
        if name:
            self.highlightObject(name)

    def highlightObject(self, objName):
        if not objName:
            return
        FreeCADGui.Selection.clearSelection()
        obj = self.doc.getObject(objName)
        if obj:
            FreeCADGui.Selection.addSelection(obj)

    def pickBaseShape(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        idx = self.baseCombo.findText(label)
        if idx == -1:
            self.baseCombo.addItem(label)
            idx = self.baseCombo.findText(label)
        self.baseCombo.setCurrentIndex(idx)

    def pickCustomPattern(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        self.patternSourceCombo.setCurrentIndex(1)
        idx = self.customPatternCombo.findText(label)
        if idx == -1:
            self.customPatternCombo.addItem(label)
            idx = self.customPatternCombo.findText(label)
        self.customPatternCombo.setCurrentIndex(idx)

    def pickSubtractions(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select one or more objects.")
            return
        for obj in sel:
            label = obj.Label
            items = self.subtractionsList.findItems(label, QtCore.Qt.MatchExactly)
            if not items:
                self.subtractionsList.addItem(label)

    def pickBaseTile(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        idx = self.tileCombo.findText(label)
        if idx == -1:
            self.tileCombo.addItem(label)
            idx = self.tileCombo.findText(label)
        self.tileCombo.setCurrentIndex(idx)

    def getBaseSpacingInMM(self):
        q = self.spacingInput.property("quantity")
        return q.Value if q else 0.0

    def onPreview(self):
        FreeCAD.Console.PrintMessage("Generating preview...\n")
        # Simplified preview for Task Panel (can be expanded)
        QtWidgets.QMessageBox.information(self.form, "Preview", "Preview generation would run here.\n\n(Preview logic identical to original script)")

    # ------------------------------------------------------------------------
    # Task Panel Interface (Required for FreeCAD Task Panel)
    # ------------------------------------------------------------------------
    def accept(self):
        """Called when user presses OK in the Task panel."""
        FreeCAD.Console.PrintMessage("Creating Hatch...\n")
        try:
            FreeCAD.ActiveDocument.openTransaction("Create Hatch")
            hatch_objects = [obj for obj in self.doc.Objects if obj.Name.startswith("CustomHatch")]
            if not hatch_objects:
                hatch_name = "CustomHatch"
            else:
                max_num = 0
                for obj in hatch_objects:
                    suffix = obj.Name[len("CustomHatch"):]
                    if suffix.isdigit():
                        max_num = max(max_num, int(suffix))
                hatch_name = f"CustomHatch{max_num + 1:03d}"

            baseName = self.baseCombo.currentText()
            base_obj = self.doc.getObject(baseName)
            if not base_obj or not hasattr(base_obj, "Shape"):
                QtWidgets.QMessageBox.warning(self.form, "Error", "Invalid base object selection")
                FreeCAD.ActiveDocument.abortTransaction()
                FreeCADGui.Control.closeDialog()
                return True

            hatchObj = makeCustomHatch(name=hatch_name)

            if self.patternSourceCombo.currentIndex() == 0:
                pattern_type = self.builtinPatternCombo.currentText()
                hatchObj.PatternType = pattern_type
            else:
                hatchObj.PatternType = "CustomObject"
                custom_pattern_name = self.customPatternCombo.currentText()
                if custom_pattern_name:
                    custom_pattern = self.doc.getObject(custom_pattern_name)
                    if custom_pattern:
                        hatchObj.PatternObject = custom_pattern

            hatchObj.BaseObject = base_obj
            hatchObj.ClipMode = self.clipModeCombo.currentText()
            hatchObj.UseSurfaceProjection = self.useSurfaceProjectionCheck.isChecked()
            hatchObj.ForceXYPlane = self.forceXYPlaneCheck.isChecked()
            
            selectedBaseObjs = self.getSelectedObjectsFromList(self.baseObjectsList)
            hatchObj.BaseObjects = selectedBaseObjs
            selectedPatternObjs = self.getSelectedObjectsFromList(self.patternObjectsList)
            hatchObj.PatternObjects = selectedPatternObjs
            selectedSubs = self.getSelectedObjectsFromList(self.subtractionsList)
            hatchObj.Subtractions = selectedSubs

            hatchObj.DistributionMode = self.distCombo.currentText()
            hatchObj.AutoScaleToFitBase = self.autoScaleCheck.isChecked()
            hatchObj.PatternScale = float(self.scaleSpin.value())
            hatchObj.RotationDeg = float(self.rotSpin.value())
            hatchObj.BaseSpacing = float(self.getBaseSpacingInMM())
            hatchObj.RepetitionsX = int(self.repXSpin.value())
            hatchObj.RepetitionsY = int(self.repYSpin.value())
            hatchObj.RandomizePlacement = bool(self.randomCheck.isChecked())
            hatchObj.RandomOffsetRange = float(self.offRangeSpin.value())
            hatchObj.RandomRotationRange = float(self.rotRangeSpin.value())
            hatchObj.RandomScaleMin = float(self.scaleMinSpin.value())
            hatchObj.RandomScaleMax = float(self.scaleMaxSpin.value())
            hatchObj.LockToBase = bool(self.lockCheck.isChecked())
            hatchObj.PatternOffsetX = float(self.offsetXSpin.value())
            hatchObj.PatternOffsetY = float(self.offsetYSpin.value())
            hatchObj.ScaleMode = self.scaleModeCombo.currentText()
            hatchObj.ShowFaces = bool(self.showFacesCheck.isChecked())
            hatchObj.ApplyTo3DSurface = bool(self.apply3DCheck.isChecked())
            hatchObj.MaxTilesAllowed = int(self.maxTilesSpin.value())
            hatchObj.DensityFactor = float(self.densitySpin.value())
            hatchObj.EnableColorVariation = bool(self.enableColorVarCheck.isChecked())
            hatchObj.ColorVariationIntensity = float(self.colorVarSpin.value())
            hatchObj.PatternPlacementMode = self.placementModeCombo.currentText()

            tileObjName = self.tileCombo.currentText().strip()
            if tileObjName:
                tileObj = self.doc.getObject(tileObjName)
                if tileObj:
                    hatchObj.BaseTileObject = tileObj
            hatchObj.TileVisibility = bool(self.tileVisibilityCheck.isChecked())

            self.doc.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
            
            QtWidgets.QMessageBox.information(
                self.form,
                "Success",
                f"Hatch created successfully!\n"
                f"Time: {hatchObj.GenerationTime:.2f}s\n"
                f"Tiles: {hatchObj.TileCount}"
            )
            FreeCADGui.Control.closeDialog()
            
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            QtWidgets.QMessageBox.critical(self.form, "Error", f"Hatch creation failed:\n{str(e)}")
            FreeCADGui.Control.closeDialog()
        return True

    def reject(self):
        """Called when user presses Cancel or closes the Task panel."""
        FreeCAD.Console.PrintMessage("Hatch creation cancelled.\n")
        FreeCADGui.Control.closeDialog()
        return True


# ============================================================================
# FreeCAD Command Registration (Native Integration) - FIXED
# ============================================================================
class _CommandHatch:
    """FreeCAD Command for the Hatch Generator."""
    def GetResources(self):
        return {
            'Pixmap': 'Draft_Hatch',  # Use FreeCAD's built-in icon
            'MenuText': 'Create Hatch',
            'ToolTip': 'Generate parametric hatch patterns on surfaces'
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        # Create and display the Task Panel
        panel = HatchTaskPanel()
        FreeCADGui.Control.showDialog(panel)

# FIXED: Register the command globally with a unique name
if FreeCAD.GuiUp:
    # Use a unique name to avoid conflicts
    if 'BIM_Hatch_Dialog' not in FreeCADGui.listCommands():
        FreeCADGui.addCommand('BIM_Hatch_Dialog', _CommandHatch())
        FreeCAD.Console.PrintMessage("Hatch Generator command registered as 'BIM_Hatch_Dialog'\n")

# ============================================================================
# Standalone Macro Support (Fallback to Dialog with Scroll Area)
# ============================================================================
def runAsMacro():
    """Fallback to dialog mode if run as a macro. Includes scroll area fix."""
    from PySide import QtWidgets
    
    mw = FreeCADGui.getMainWindow()
    dlg = QtWidgets.QDialog(mw)
    dlg.setWindowTitle("Custom Hatch Generator (Macro Mode)")
    dlg.resize(450, 700)
    
    layout = QtWidgets.QVBoxLayout(dlg)
    
    # Create a scroll area for the content
    scroll = QtWidgets.QScrollArea()
    scroll.setWidgetResizable(True)
    scroll.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
    scroll.setFrameShape(QtWidgets.QFrame.NoFrame)
    
    # Create the task panel widget
    task_panel = HatchTaskPanel()
    widget = task_panel.form
    
    # Set the widget into the scroll area
    scroll.setWidget(widget)
    layout.addWidget(scroll)
    
    # Buttons
    button_box = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel)
    button_box.accepted.connect(lambda: (task_panel.accept(), dlg.accept()))
    button_box.rejected.connect(lambda: (task_panel.reject(), dlg.reject()))
    layout.addWidget(button_box)
    
    dlg.exec_()

def runHatchGeneratorDialog():
    """Legacy function to maintain compatibility with double-click handlers."""
    # FIXED: Use the registered command
    if FreeCAD.GuiUp:
        FreeCADGui.runCommand('BIM_Hatch_Dialog')

if __name__ == "__main__":
    if FreeCAD.GuiUp:
        runAsMacro()