#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (c) 2024 FreeCAD Multi-Axis Development Team

"""
StockRemoval - 5-Axis Collision-Aware Stock Removal System

This module provides advanced stock removal simulation for multi-axis machining
with CUDA acceleration and real-time collision detection.
"""

from .StockRemoval5Axis import StockRemoval5Axis, VolumetricStock, ToolGeometry, CollisionDetector

__all__ = ["StockRemoval5Axis", "VolumetricStock", "ToolGeometry", "CollisionDetector"]

__version__ = "1.0.0"
