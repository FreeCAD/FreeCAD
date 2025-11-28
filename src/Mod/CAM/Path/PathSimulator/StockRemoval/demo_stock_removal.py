#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (c) 2024 FreeCAD Multi-Axis Development Team

"""
Demo script for 5-axis stock removal system.
"""

import time
from StockRemoval5Axis import (
    StockRemoval5Axis,
    ToolGeometry,
    ToolType
)

def main():
    print("="*70)
    print("5-AXIS STOCK REMOVAL DEMONSTRATION")
    print("="*70)
    
    # Create stock removal system
    print("\n1. Initializing stock removal system...")
    start_time = time.time()
    
    system = StockRemoval5Axis(
        stock_bounds=(-50, 50, -50, 50, 0, 50),
        resolution=1.0,
        use_cuda=False
    )
    
    init_time = time.time() - start_time
    print(f"   Initialization time: {init_time:.3f}s")
    
    # Create tool
    print("\n2. Creating tool geometry...")
    tool = ToolGeometry(
        tool_type=ToolType.ENDMILL,
        diameter=6.0,
        length=60.0,
        flute_length=30.0
    )
    print(f"   Tool: {tool.tool_type.value}, Ø{tool.diameter}mm")
    
    # Perform machining operations
    print("\n3. Performing machining operations...")
    operations = [
        ((-40, 0, 10), (-30, 0, 10)),
        ((-30, 0, 10), (-20, 0, 10)),
        ((-20, 0, 10), (-10, 0, 10)),
        ((-10, 0, 10), (0, 0, 10)),
        ((0, 0, 10), (10, 0, 10)),
        ((10, 0, 10), (20, 0, 10)),
        ((20, 0, 10), (30, 0, 10)),
        ((30, 0, 10), (40, 0, 10)),
    ]
    
    start_time = time.time()
    for i, (start, end) in enumerate(operations, 1):
        result = system.remove_material(tool, start, end, check_collision=False)
        print(f"   Operation {i}/{len(operations)}: {start} → {end}")
    
    operation_time = time.time() - start_time
    print(f"   Total operation time: {operation_time:.3f}s")
    print(f"   Average per operation: {operation_time/len(operations):.3f}s")
    
    # Get statistics
    print("\n4. Statistics:")
    stats = system.get_statistics()
    print(f"   Initial volume: {stats['initial_volume']:.2f} mm³")
    print(f"   Current volume: {stats['current_volume']:.2f} mm³")
    print(f"   Removed volume: {stats['removed_volume']:.2f} mm³")
    print(f"   Removal percentage: {stats['removal_percentage']:.2f}%")
    print(f"   Operations: {stats['operations']}")
    print(f"   Collisions: {stats['collisions']}")
    print(f"   Voxels removed: {stats['voxels_removed']:,}")
    
    # Export result
    print("\n5. Exporting stock mesh...")
    output_file = "obj_preview_5axis_stock.obj"
    start_time = time.time()
    system.export_stock(output_file)
    export_time = time.time() - start_time
    print(f"   Export time: {export_time:.3f}s")
    print(f"   Output file: {output_file}")
    
    print("\n" + "="*70)
    print("DEMONSTRATION COMPLETE")
    print("="*70)

if __name__ == '__main__':
    main()
