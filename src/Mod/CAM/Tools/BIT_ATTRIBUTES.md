# Bit types and attributes

Below is the list of bit types and their attributes recognized by CAM operations. 
This is independent from tool shapes - but tool shapes must honor those attributes in order to work properly.

## EndMill

| Name | Description |
| -------- | ------- |
| Diameter | Endmill diameter |
| Length | Overall length of the endmill |
| ShankDiameter | diameter of the shank |

## BallEndMill

| Name | Description |
| -------- | ------- |
| Diameter | Endmill diameter |
| Length | Overall length of the endmill |
| ShankDiameter | diameter of the shank |
| CuttingEdgeHeight | |


## BullNoseMill

Diameter; Endmill diameter
Length; Overall length of the endmill
ShankDiameter; diameter of the shank
FlatRadius;Radius of the bottom flat part.
CuttingEdgeHeight

## Drill
TipAngle; Full angle of the drill tip
Diameter; Drill bit diameter
Length; Overall length of the drillbit

## VBit

Diameter; Overall diameter of the V-bit
CuttingEdgeAngle;Full angle of the v-bit
Length; Overall  bit length
ShankDiameter
FlatHeight;Height of the flat extension of the v-bit
FlatRadius; Diameter of the flat end of the tip


## Engraver

Diameter
CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
CuttingEdgeHeight
LengthOffset


