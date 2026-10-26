<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en-GB" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="84"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>The center point of the helix' start; derived from the reference axis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>The helix' direction; derived from the reference axis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="101"/>
      <source>The reference axis of the helix.</source>
      <translation>The reference axis of the helix.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="108"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="120"/>
      <source>The axial distance between two turns.</source>
      <translation>The axial distance between two turns.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="127"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>The height of the helix' path, not accounting for the extent of the profile.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="137"/>
      <source>The number of turns in the helix.</source>
      <translation>The number of turns in the helix.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="145"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="158"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="169"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="180"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Determines whether the helix points in the opposite direction of the axis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="210"/>
      <source>deprecated, do not use</source>
      <translation>deprecated, do not use</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="190"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="202"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>Number of gear teeth</source>
      <translation>Number of gear teeth</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="120"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Pressure angle of gear teeth</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="114"/>
      <source>Module of the gear</source>
      <translation>Module of the gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="129"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=2 curves with each 3 control points, False=1 curve with 4 control points.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="137"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=external Gear, False=internal Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="146"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>The height of the tooth from the pitch circle up to its tip, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="155"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>The height of the tooth from the pitch circle down to its root, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="164"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>The radius of the fillet at the root of the tooth, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="173"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>The distance by which the reference profile is shifted outwards, normalized by the module.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1715"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1716"/>
      <source>Additive Helix</source>
      <translation>Additive Helix</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1717"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Sweeps the selected sketch or profile along a helix and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1617"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1618"/>
      <source>Additive Loft</source>
      <translation>Additive Loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1619"/>
      <source>Lofts the selected sketch or profile through one or more sections and adds it to the body</source>
      <translation>Lofts the selected sketch or profile through one or more sections and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1517"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1518"/>
      <source>Additive Pipe</source>
      <translation>Additive Pipe</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1519"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Sweeps the selected sketch or profile along a path and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="109"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="110"/>
      <source>New Body</source>
      <translation>New Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="111"/>
      <source>Creates a new body and activates it</source>
      <translation>Creates a new body and activates it</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2870"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2871"/>
      <source>Boolean Operation</source>
      <translation>Boolean Operation</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2872"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>Applies boolean operations with the selected objects and the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="306"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Local Coordinate System</source>
      <translation>Local Coordinate System</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="308"/>
      <source>Creates a new local coordinate system</source>
      <translation>Creates a new local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2044"/>
      <source>Chamfer</source>
      <translation>Chamfer</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2045"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>Applies a chamfer to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="522"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="523"/>
      <source>Clone</source>
      <translation>Clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="524"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>Copies a solid object parametrically as the base feature of a new body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2160"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2161"/>
      <source>Draft</source>
      <translation>Draft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2162"/>
      <source>Applies a draft to the selected faces</source>
      <translation>Applies a draft to the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicate &amp;Object</source>
      <translation>Duplicate &amp;Object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="758"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Duplicates the selected object and adds it to the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="2015"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>Fillet</source>
      <translation>Fillet</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>Applies a fillet to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1447"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1448"/>
      <source>Groove</source>
      <translation>Groove</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1449"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>Revolves the sketch or profile around a line or axis and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1340"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1341"/>
      <source>Hole</source>
      <translation>Hole</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1343"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Datum Line</source>
      <translation>Datum Line</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Creates a new datum line</source>
      <translation>Creates a new datum line</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2415"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2416"/>
      <source>Linear Pattern</source>
      <translation>Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2417"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>Duplicates the selected features or the active body in a linear pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="391"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="392"/>
      <source>Migrate</source>
      <translation>Migrate</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="393"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>Migrates the document to the modern Part Design workflow</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2358"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2359"/>
      <source>Mirror</source>
      <translation>Mirror</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2360"/>
      <source>Mirrors the selected features or active body</source>
      <translation>Mirrors the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="824"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="825"/>
      <source>Move Object To…</source>
      <translation>Move Object To…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="826"/>
      <source>Moves the selected object to another body</source>
      <translation>Moves the selected object to another body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1021"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1022"/>
      <source>Move Feature After…</source>
      <translation>Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1023"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>Moves the selected feature after another feature in the same body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="657"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>Set Tip</source>
      <translation>Set Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>Moves the tip of the body to the selected feature</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2739"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2740"/>
      <source>Multi-Transform</source>
      <translation>Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2741"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>Applies multiple transformations to the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="612"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="613"/>
      <source>New Sketch</source>
      <translation>New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="614"/>
      <source>Creates a new sketch</source>
      <translation>Creates a new sketch</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1282"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1283"/>
      <source>Pad</source>
      <translation>Pad</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>Extrudes the selected sketch or profile and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="216"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="217"/>
      <source>Datum Plane</source>
      <translation>Datum Plane</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Creates a new datum plane</source>
      <translation>Creates a new datum plane</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1311"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1312"/>
      <source>Pocket</source>
      <translation>Pocket</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1313"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>Extrudes the selected sketch or profile and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="276"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="277"/>
      <source>Datum Point</source>
      <translation>Datum Point</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>Creates a new datum point</source>
      <translation>Creates a new datum point</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2634"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2635"/>
      <source>Polar Pattern</source>
      <translation>Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2636"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>Duplicates the selected features or the active body in a circular pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1385"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1386"/>
      <source>Revolve</source>
      <translation>Revolve</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1387"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>Revolves the selected sketch or profile around a line or axis and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2696"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2697"/>
      <source>Scale</source>
      <translation>Scale</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2698"/>
      <source>Scales the selected features or the active body</source>
      <translation>Scales the selected features or the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="340"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="341"/>
      <source>Shape Binder</source>
      <translation>Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="342"/>
      <source>Creates a new shape binder</source>
      <translation>Creates a new shape binder</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="411"/>
      <source>Sub-Shape Binder</source>
      <translation>Sub-Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="412"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1799"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1800"/>
      <source>Subtractive Helix</source>
      <translation>Subtractive Helix</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1801"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>Sweeps the selected sketch or profile along a helix and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1667"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1668"/>
      <source>Subtractive Loft</source>
      <translation>Subtractive Loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1670"/>
      <source>Lofts the selected sketch or profile through one or more sections and removes it from the body</source>
      <translation>Lofts the selected sketch or profile through one or more sections and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1567"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1568"/>
      <source>Subtractive Pipe</source>
      <translation>Subtractive Pipe</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1569"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>Sweeps the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2230"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2231"/>
      <source>Thickness</source>
      <translation>Thickness</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2232"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>Applies thickness and removes the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="77"/>
      <source>Additive Primitive</source>
      <translation>Additive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="78"/>
      <source>Creates an additive primitive</source>
      <translation>Creates an additive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Box</source>
      <translation>Additive Box</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Additive Cylinder</source>
      <translation>Additive Cylinder</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Additive Sphere</source>
      <translation>Additive Sphere</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Additive Cone</source>
      <translation>Additive Cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Additive Ellipsoid</source>
      <translation>Additive Ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Additive Torus</source>
      <translation>Additive Torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Additive Prism</source>
      <translation>Additive Prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Additive Wedge</source>
      <translation>Additive Wedge</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="285"/>
      <source>Subtractive Primitive</source>
      <translation>Subtractive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="286"/>
      <source>Creates a subtractive primitive</source>
      <translation>Creates a subtractive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Subtractive Box</source>
      <translation>Subtractive Box</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Subtractive Cylinder</source>
      <translation>Subtractive Cylinder</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Subtractive Sphere</source>
      <translation>Subtractive Sphere</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Subtractive Cone</source>
      <translation>Subtractive Cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Subtractive Ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Subtractive Torus</source>
      <translation>Subtractive Torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Subtractive Prism</source>
      <translation>Subtractive Prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Subtractive Wedge</source>
      <translation>Subtractive Wedge</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="362"/>
      <source>Edit Shape Binder</source>
      <translation>Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="373"/>
      <source>Create Shape Binder</source>
      <translation>Create Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="472"/>
      <source>Create Sub-Shape Binder</source>
      <translation>Create Sub-Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="542"/>
      <source>Create Clone</source>
      <translation>Create Clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1166"/>
      <source>Make Copy</source>
      <translation>Make Copy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2794"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>Convert to Multi-Transform feature</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="289"/>
      <source>Sketch on Face</source>
      <translation>Sketch on Face</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="351"/>
      <source>Make copy</source>
      <translation>Make copy</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="553"/>
      <location filename="../../SketchWorkflow.cpp" line="813"/>
      <source>New Sketch</source>
      <translation>New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2891"/>
      <source>Create Boolean</source>
      <translation>Create Boolean</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <location filename="../../CommandBody.cpp" line="225"/>
      <source>Add a Body</source>
      <translation>Add a Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="528"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>Migrate legacy Part Design features to bodies</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="770"/>
      <source>Duplicate a Part Design object</source>
      <translation>Duplicate a Part Design object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1115"/>
      <source>Move a feature inside body</source>
      <translation>Move a feature inside body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="724"/>
      <source>Move tip to selected feature</source>
      <translation>Move tip to selected feature</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="929"/>
      <source>Move an object</source>
      <translation>Move an object</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="306"/>
      <source>Mirror</source>
      <translation>Mirror</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="346"/>
      <source>Linear Pattern</source>
      <translation>Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="395"/>
      <source>Polar Pattern</source>
      <translation>Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="436"/>
      <source>Circular Pattern</source>
      <translation>Circular Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="475"/>
      <source>Path Pattern</source>
      <translation>Path Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="499"/>
      <source>Point Pattern</source>
      <translation>Point Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="521"/>
      <source>Scale</source>
      <translation>Scale</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation>Face Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation>Edge Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation>Boolean Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation>Helper Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation>Modeling Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>Create Geometry</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Start Part</source>
      <translation>Start Part</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>Involute Parameter</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>Module</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>Pressure angle</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>High precision</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>True</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>False</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>External gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>Addendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>Dedendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>Root fillet coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>Profile shift coefficient</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Active Body Required</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>Create New Body</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="53"/>
      <source>Select an active body</source>
      <translation>Select an active body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Geometric Primitives</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Angle in first direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Angle in second direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>Length</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>Width</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>Height</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>Radius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>Rotation angle</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>Radius 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>Radius 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>U parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>V parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Radius in local z-direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>Radius in local X-direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>Radius 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>Radius in local Y-direction
If zero, it is equal to Radius2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>V parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>Radius in local XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>Radius in local XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Polygon</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>Circumradius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Pitch</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>Coordinate system</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Growth</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>Number of rotations</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>Angle 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>Angle 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>From 3 Points</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>Major radius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>Minor radius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2354"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2365"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2370"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>Right-handed</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Left-handed</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Start point</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>End point</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Reference</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Make independent copy (recommended)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Make dependent copy</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Create cross-reference</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="336"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Selecting this will cause circular dependency.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>Add Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>Remove Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Fuse</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Cut</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="97"/>
      <source>Boolean Parameters</source>
      <translation>Boolean Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="128"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="100"/>
      <source>Subtractive Box Parameters</source>
      <translation>Subtractive Box Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="101"/>
      <source>Additive Box Parameters</source>
      <translation>Additive Box Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="103"/>
      <source>Subtractive Cylinder Parameters</source>
      <translation>Subtractive Cylinder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="104"/>
      <source>Additive Cylinder Parameters</source>
      <translation>Additive Cylinder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="106"/>
      <source>Subtractive Sphere Parameters</source>
      <translation>Subtractive Sphere Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="107"/>
      <source>Additive Sphere Parameters</source>
      <translation>Additive Sphere Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="109"/>
      <source>Subtractive Cone Parameters</source>
      <translation>Subtractive Cone Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="110"/>
      <source>Additive Cone Parameters</source>
      <translation>Additive Cone Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="112"/>
      <source>Subtractive Ellipsoid Parameters</source>
      <translation>Subtractive Ellipsoid Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="113"/>
      <source>Additive Ellipsoid Parameters</source>
      <translation>Additive Ellipsoid Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="115"/>
      <source>Subtractive Torus Parameters</source>
      <translation>Subtractive Torus Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="116"/>
      <source>Additive Torus Parameters</source>
      <translation>Additive Torus Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="118"/>
      <source>Subtractive Prism Parameters</source>
      <translation>Subtractive Prism Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="119"/>
      <source>Additive Prism Parameters</source>
      <translation>Additive Prism Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="121"/>
      <source>Subtractive Wedge Parameters</source>
      <translation>Subtractive Wedge Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="122"/>
      <source>Additive Wedge Parameters</source>
      <translation>Additive Wedge Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="125"/>
      <source>Subtractive Primitive Parameters</source>
      <translation>Subtractive Primitive Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="126"/>
      <source>Additive Primitive Parameters</source>
      <translation>Additive Primitive Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1045"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1053"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1061"/>
      <source>Invalid wedge parameters</source>
      <translation>Invalid wedge parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1046"/>
      <source>X min must not be equal to X max!</source>
      <translation>X min must not be equal to X max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1054"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y min must not be equal to Y max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1062"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z min must not be equal to Z max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1105"/>
      <source>Create primitive</source>
      <translation>Create primitive</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1150"/>
      <source>%1 fine dragging</source>
      <translation>%1 fine dragging</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1153"/>
      <source>%1 coarse dragging</source>
      <translation>%1 coarse dragging</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- select an item to highlight it
- double-click on an item to see the chamfers</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>Equal distance</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>Two distances</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>Distance and angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>Flips the direction</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>Size</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>Size 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="346"/>
      <source>Empty chamfer created!
</source>
      <translation>Empty chamfer created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="437"/>
      <source>The body list cannot be empty</source>
      <translation>The body list cannot be empty</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="437"/>
      <source>Empty Body List</source>
      <translation>Empty Body List</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="458"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Boolean: Accept: Input error</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="110"/>
      <source>Incompatible Reference Set</source>
      <translation>Incompatible Reference Set</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="112"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- select an item to highlight it
- double-click on an item to see the drafts</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Draft angle</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>Neutral Plane</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>Pull Direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Reverse pull direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="304"/>
      <source>Empty draft created!
</source>
      <translation>Empty draft created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="591"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="596"/>
      <source>Confirm Selection</source>
      <translation>Confirm Selection</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="609"/>
      <source>Add All Edges</source>
      <translation>Add All Edges</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="615"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>Adds all edges to the list box (only when in add selection mode)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="624"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1480"/>
      <source>No face selected</source>
      <translation>No face selected</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="184"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1243"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="81"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="401"/>
      <source>Preview</source>
      <translation>Preview</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="405"/>
      <source>Select Faces</source>
      <translation>Select Faces</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="780"/>
      <source>Select reference…</source>
      <translation>Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="667"/>
      <source>No shape selected</source>
      <translation>No shape selected</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="68"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="226"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1180"/>
      <source>No start reference selected</source>
      <translation>No start reference selected</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="69"/>
      <source>Offset from the profile or selected start reference</source>
      <translation>Offset from the profile or selected start reference</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="773"/>
      <source>Sketch normal</source>
      <translation>Sketch normal</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="776"/>
      <source>Face normal</source>
      <translation>Face normal</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="784"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="787"/>
      <source>Custom direction</source>
      <translation>Custom direction</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1174"/>
      <source>Cancel</source>
      <translation>Cancel</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1175"/>
      <source>Select face, plane...</source>
      <translation>Select face, plane...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1179"/>
      <source>Pick Reference</source>
      <translation>Pick Reference</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1190"/>
      <source>Click on a shape in the model</source>
      <translation>Click on a shape in the model</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1467"/>
      <source>One sided</source>
      <translation>One sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1468"/>
      <source>Two sided</source>
      <translation>Two sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1469"/>
      <source>Symmetric</source>
      <translation>Symmetric</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1475"/>
      <source>Face selection active</source>
      <translation>Face selection active</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Allow used features</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>Allow External Features</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>From other bodies of the same part</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>From different parts or free features</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Make independent copy (recommended)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Make dependent copy</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Create cross-reference</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Valid</source>
      <translation>Valid</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Invalid shape</source>
      <translation>Invalid shape</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>No wire in sketch</source>
      <translation>No wire in sketch</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="85"/>
      <source>Sketch already used by other feature</source>
      <translation>Sketch already used by other feature</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="87"/>
      <source>Belongs to another body</source>
      <translation>Belongs to another body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="89"/>
      <source>Belongs to another part</source>
      <translation>Belongs to another part</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="91"/>
      <source>Doesn't belong to any body</source>
      <translation>Doesn't belong to any body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="93"/>
      <source>Base plane</source>
      <translation>Base plane</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Feature is located after the tip of the body</source>
      <translation>Feature is located after the tip of the body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="107"/>
      <source>Select Attachment</source>
      <translation>Select Attachment</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- select an item to highlight it
- double-click on an item to see the fillets</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>Radius</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="204"/>
      <source>Empty fillet created!</source>
      <translation>Empty fillet created!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Valid</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="259"/>
      <source>Base X-axis</source>
      <translation>Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="260"/>
      <source>Base Y-axis</source>
      <translation>Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="261"/>
      <source>Base Z-axis</source>
      <translation>Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Horizontal sketch axis</source>
      <translation>Horizontal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="242"/>
      <source>Vertical sketch axis</source>
      <translation>Vertical sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Normal sketch axis</source>
      <translation>Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>Status</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>Axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="226"/>
      <source>Select reference…</source>
      <translation>Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Pitch-Height-Angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Pitch-Turns-Angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Height-Turns-Angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Height-Turns-Growth</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>Pitch</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>Height</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>Turns</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>Cone angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>Radial growth</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="274"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="285"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="290"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="307"/>
      <source>Recompute on change</source>
      <translation>Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Left handed</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Reversed</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="71"/>
      <source>Subtractive Helix Parameters</source>
      <translation>Subtractive Helix Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="72"/>
      <source>Additive Helix Parameters</source>
      <translation>Additive Helix Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="245"/>
      <source>Construction line %1</source>
      <translation>Construction line %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="313"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Warning: helix might be self intersecting</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="318"/>
      <source>Error: helix touches itself</source>
      <translation>Error: helix touches itself</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="364"/>
      <source>Error: unsupported mode</source>
      <translation>Error: unsupported mode</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Counterbore</source>
      <translation>Counterbore</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="60"/>
      <source>Countersink</source>
      <translation>Countersink</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Counterdrill</source>
      <translation>Counterdrill</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>Hole Parameters</source>
      <translation>Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>None</source>
      <translation>None</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ISO metric regular</source>
      <translation>ISO metric regular</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO metric fine</source>
      <translation>ISO metric fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>UTS coarse</source>
      <translation>UTS coarse</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>UTS fine</source>
      <translation>UTS fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>UTS extra fine</source>
      <translation>UTS extra fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="81"/>
      <source>ANSI pipes</source>
      <translation>ANSI pipes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="82"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP pipes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="83"/>
      <source>BSW whitworth</source>
      <translation>BSW whitworth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="84"/>
      <source>BSF whitworth fine</source>
      <translation>BSF whitworth fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="85"/>
      <source>ISO tyre valves</source>
      <translation>ISO tyre valves</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="203"/>
      <source>Offset from the profile or selected start reference</source>
      <translation>Offset from the profile or selected start reference</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="702"/>
      <source>Cancel</source>
      <translation>Cancel</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="703"/>
      <source>Select face, plane...</source>
      <translation>Select face, plane...</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="708"/>
      <source>Pick Reference</source>
      <translation>Pick Reference</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="709"/>
      <location filename="../../TaskHoleParameters.cpp" line="1130"/>
      <source>No start reference selected</source>
      <translation>No start reference selected</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="772"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Medium</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="776"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="780"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Coarse</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="786"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="790"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Close</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="794"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Loose</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="798"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="799"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Close</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="800"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Wide</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Ruled surface</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Closed</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Profile</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Object</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Add Section</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Remove Section</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="115"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="126"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="131"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="148"/>
      <source>Recompute on change</source>
      <translation>Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="65"/>
      <source>Subtractive Loft Parameters</source>
      <translation>Subtractive Loft Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="66"/>
      <source>Additive Loft Parameters</source>
      <translation>Additive Loft Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="96"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>Plane</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="186"/>
      <source>Error</source>
      <translation>Error</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>Transformations</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="74"/>
      <source>Edit</source>
      <translation>Edit</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Delete</source>
      <translation>Delete</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="80"/>
      <source>Add Mirror Transformation</source>
      <translation>Add Mirror Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="88"/>
      <source>Add Linear Pattern</source>
      <translation>Add Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="96"/>
      <source>Add Circular Pattern</source>
      <translation>Add Circular Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="104"/>
      <source>Add Path Pattern</source>
      <translation>Add Path Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="112"/>
      <source>Add Point Pattern</source>
      <translation>Add Point Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="120"/>
      <source>Add Polar Pattern</source>
      <translation>Add Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="128"/>
      <source>Add Scale Transformation</source>
      <translation>Add Scale Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="131"/>
      <source>Move Up</source>
      <translation>Move Up</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="134"/>
      <source>Move Down</source>
      <translation>Move Down</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="166"/>
      <source>Right-click to add a transformation</source>
      <translation>Right-click to add a transformation</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation>Pad Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>Offset the pad from the face at which the pad will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>Offset the pad from the face at which the pad will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>Reverses pad direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>To last</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>To first</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Up to face</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>Up to shape</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="150"/>
      <location filename="../../TaskPadPocketParameters.ui" line="385"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="158"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="166"/>
      <location filename="../../TaskPadPocketParameters.ui" line="395"/>
      <source>Length</source>
      <translation>Length</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="183"/>
      <location filename="../../TaskPadPocketParameters.ui" line="412"/>
      <source>Offset to face</source>
      <translation>Offset to face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="274"/>
      <location filename="../../TaskPadPocketParameters.ui" line="503"/>
      <source>Select all faces</source>
      <translation>Select all faces</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="299"/>
      <location filename="../../TaskPadPocketParameters.ui" line="528"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="337"/>
      <location filename="../../TaskPadPocketParameters.ui" line="566"/>
      <source>Select Face</source>
      <translation>Select Face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="363"/>
      <source>Side 2</source>
      <translation>Side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="104"/>
      <location filename="../../TaskPadPocketParameters.ui" line="594"/>
      <source>Direction</source>
      <translation>Richtung</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="623"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Set a direction or select an edge
from the model as reference</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="628"/>
      <source>Sketch normal</source>
      <translation>Sketch normal</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="638"/>
      <source>Custom direction</source>
      <translation>Custom direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="651"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="603"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>If unchecked, the length will be
measured along the specified direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="607"/>
      <source>Length along sketch normal</source>
      <translation>Length along sketch normal</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="296"/>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Start</source>
      <translation>Start</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Profile plane</source>
      <translation>Profile plane</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="35"/>
      <location filename="../../TaskPadPocketParameters.ui" line="87"/>
      <source>Offset</source>
      <translation>Offset</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="40"/>
      <location filename="../../TaskPadPocketParameters.ui" line="48"/>
      <source>Reference</source>
      <translation>Reference</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="77"/>
      <source>Pick Reference</source>
      <translation>Pick Reference</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="587"/>
      <source>Reversed</source>
      <translation>Reversed</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="616"/>
      <source>Direction/edge</source>
      <translation>Direction/edge</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="633"/>
      <source>Select reference…</source>
      <translation>Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="664"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="671"/>
      <source>X-component of direction vector</source>
      <translation>X-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="693"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="700"/>
      <source>Y-component of direction vector</source>
      <translation>Y-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="722"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="729"/>
      <source>Z-component of direction vector</source>
      <translation>Z-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="200"/>
      <location filename="../../TaskPadPocketParameters.ui" line="429"/>
      <source>Angle to taper the extrusion</source>
      <translation>Angle to taper the extrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="128"/>
      <source>Side 1</source>
      <translation>Side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="203"/>
      <location filename="../../TaskPadPocketParameters.ui" line="432"/>
      <source>Taper angle</source>
      <translation>Taper angle</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="256"/>
      <location filename="../../TaskPadPocketParameters.ui" line="485"/>
      <source>Select Shape</source>
      <translation>Select Shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="271"/>
      <location filename="../../TaskPadPocketParameters.ui" line="500"/>
      <source>Selects all faces of the shape</source>
      <translation>Selects all faces of the shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="762"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="773"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="778"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="788"/>
      <source>Recompute on change</source>
      <translation>Recompute on change</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Orientation mode</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Fixed</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Auxiliary</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Binormal</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Curvilinear equivalence</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Profile</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Object</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Add Edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Remove Edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Set the constant binormal vector used to calculate the profiles orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="190"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="197"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="204"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="83"/>
      <source>Subtractive Pipe Section Orientation</source>
      <translation>Subtractive Pipe Section Orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="84"/>
      <source>Additive Pipe Section Orientation</source>
      <translation>Additive Pipe Section Orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="666"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Profile</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Object</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>Corner transition</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>Right corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>Round corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>Path to Sweep Along</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Add edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>Remove edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="158"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="169"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="174"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Transformed</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="77"/>
      <source>Subtractive Pipe Parameters</source>
      <translation>Subtractive Pipe Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="78"/>
      <source>Additive Pipe Parameters</source>
      <translation>Additive Pipe Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="113"/>
      <source>Select All</source>
      <translation>Select All</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="131"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="502"/>
      <location filename="../../TaskPipeParameters.cpp" line="624"/>
      <source>Input Error</source>
      <translation>Input Error</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="502"/>
      <source>No active body</source>
      <translation>No active body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Transform mode</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Constant</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Multisection</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Add Section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Remove Section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="89"/>
      <source>Subtractive Pipe Section Transformation</source>
      <translation>Subtractive Pipe Section Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="90"/>
      <source>Additive Pipe Section Transformation</source>
      <translation>Additive Pipe Section Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="956"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation>Pocket Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>Offset from the selected face at which the pocket will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>Offset from the selected face at which the pocket will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="48"/>
      <source>Reverses pocket direction</source>
      <translation>Reverses pocket direction</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Through all</source>
      <translation>Through all</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>To first</source>
      <translation>To first</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="80"/>
      <source>Up to face</source>
      <translation>Up to face</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="81"/>
      <source>Up to shape</source>
      <translation>Up to shape</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="194"/>
      <location filename="../../TaskRevolutionParameters.ui" line="296"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="131"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="313"/>
      <source>Base X-axis</source>
      <translation>Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Start</source>
      <translation>Start</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <source>Profile plane</source>
      <translation>Profile plane</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="35"/>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Offset</source>
      <translation>Offset</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="40"/>
      <location filename="../../TaskRevolutionParameters.ui" line="48"/>
      <source>Reference</source>
      <translation>Reference</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="77"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="497"/>
      <source>Pick Reference</source>
      <translation>Pick Reference</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="113"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="136"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="314"/>
      <source>Base Y-axis</source>
      <translation>Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="141"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="315"/>
      <source>Base Z-axis</source>
      <translation>Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="146"/>
      <source>Horizontal sketch axis</source>
      <translation>Horizontal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="151"/>
      <source>Vertical sketch axis</source>
      <translation>Vertical sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="178"/>
      <source>Side 1</source>
      <translation>Side 1</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="247"/>
      <location filename="../../TaskRevolutionParameters.ui" line="343"/>
      <source>Select Face</source>
      <translation>Select Face</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="280"/>
      <source>Side 2</source>
      <translation>Side 2</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="367"/>
      <source>Symmetric to plane</source>
      <translation>Symmetric to plane</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="374"/>
      <source>Projects the selected axis onto the profile plane if the profile is planar. When unchecked, uses the axis in its original position and direction.</source>
      <translation>Projects the selected axis onto the profile plane if the profile is planar. When unchecked, uses the axis in its original position and direction.</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="377"/>
      <source>Project selected axis to profile plane</source>
      <translation>Project selected axis to profile plane</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="384"/>
      <source>Reversed</source>
      <translation>Reversed</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="393"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="404"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="409"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="123"/>
      <source>Axis</source>
      <translation>Axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="156"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="323"/>
      <source>Select reference…</source>
      <translation>Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="210"/>
      <location filename="../../TaskRevolutionParameters.ui" line="306"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="258"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="235"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="630"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="426"/>
      <source>Recompute on change</source>
      <translation>Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="146"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="498"/>
      <source>No start reference selected</source>
      <translation>No start reference selected</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="147"/>
      <source>Angular offset from the profile or selected start reference</source>
      <translation>Angular offset from the profile or selected start reference</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="260"/>
      <source>To last</source>
      <translation>To last</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="263"/>
      <source>Through all</source>
      <translation>Through all</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="265"/>
      <source>To first</source>
      <translation>To first</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="266"/>
      <source>Up to face</source>
      <translation>Up to face</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="273"/>
      <source>One sided</source>
      <translation>One sided</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="274"/>
      <source>Two sided</source>
      <translation>Two sided</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="275"/>
      <source>Symmetric</source>
      <translation>Symmetric</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="506"/>
      <source>Cancel</source>
      <translation>Cancel</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="507"/>
      <source>Select face, plane...</source>
      <translation>Select face, plane...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="651"/>
      <source>Face selection active</source>
      <translation>Face selection active</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="246"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="656"/>
      <source>No face selected</source>
      <translation>No face selected</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>Factor</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Object</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Add Geometry</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Remove Geometry</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation>Shape Binder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="139"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="127"/>
      <location filename="../../TaskSketchBasedParameters.cpp" line="242"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- select an item to highlight it
- double-click on an item to see the features</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>Thickness</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>Skin</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Pipe</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>Recto verso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>Join type</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>Make thickness inwards</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="216"/>
      <source>Recto verso applies the thickness equally to both sides</source>
      <translation>Recto verso applies the thickness equally to both sides</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="219"/>
      <source>Total wall thickness; half is applied to each side</source>
      <translation>Total wall thickness; half is applied to each side</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="293"/>
      <source>Empty thickness created!
</source>
      <translation>Empty thickness created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>Remove</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="398"/>
      <source>Normal sketch axis</source>
      <translation>Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="397"/>
      <source>Vertical sketch axis</source>
      <translation>Vertical sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="396"/>
      <source>Horizontal sketch axis</source>
      <translation>Horizontal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="400"/>
      <location filename="../../TaskTransformedParameters.cpp" line="436"/>
      <source>Construction line %1</source>
      <translation>Construction line %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="414"/>
      <source>Base X-axis</source>
      <translation>Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="415"/>
      <source>Base Y-axis</source>
      <translation>Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="416"/>
      <source>Base Z-axis</source>
      <translation>Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="450"/>
      <source>Base XY-plane</source>
      <translation>Base XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="451"/>
      <source>Base YZ-plane</source>
      <translation>Base YZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="452"/>
      <source>Base XZ-plane</source>
      <translation>Base XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="424"/>
      <location filename="../../TaskTransformedParameters.cpp" line="460"/>
      <source>Select reference…</source>
      <translation>Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>Transform body</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>Transform tool shapes</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>Add Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>Remove Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="116"/>
      <source>Recompute on change</source>
      <translation>Recompute on change</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="911"/>
      <source>Select Body</source>
      <translation>Select Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="912"/>
      <source>Select a body from the list</source>
      <translation>Select a body from the list</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1100"/>
      <source>Move Feature After…</source>
      <translation>Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1101"/>
      <source>Select a feature from the list</source>
      <translation>Select a feature from the list</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1188"/>
      <source>Move Tip</source>
      <translation>Move Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1194"/>
      <source>Set tip to last feature?</source>
      <translation>Set tip to last feature?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>The moved feature appears after the currently set tip.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="174"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>There are no attachment modes that fit selected objects. Select something else.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="187"/>
      <location filename="../../Command.cpp" line="195"/>
      <location filename="../../Command.cpp" line="202"/>
      <source>Error</source>
      <translation>Error</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="851"/>
      <source>Several sub-elements selected</source>
      <translation>Several sub-elements selected</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="852"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>Select a single face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="859"/>
      <source>Select a face as support for a sketch!</source>
      <translation>Select a face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="866"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>Need a planar face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="873"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>Create a plane first or select a face to sketch on</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="858"/>
      <source>No support face selected</source>
      <translation>No support face selected</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="865"/>
      <source>No planar support</source>
      <translation>No planar support</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="872"/>
      <source>No valid planes in this document</source>
      <translation>No valid planes in this document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1194"/>
      <location filename="../../ViewProviderDatum.cpp" line="259"/>
      <location filename="../../ViewProvider.cpp" line="164"/>
      <location filename="../../SketchWorkflow.cpp" line="778"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="98"/>
      <source>A dialog is already open in the task panel</source>
      <translation>A dialog is already open in the task panel</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1048"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Cannot use this command as there is no solid to subtract from.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Ensure that the body contains a feature before attempting a subtractive command.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1075"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Cannot use selected object. Selected object must belong to the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="188"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>There is no active body. Please activate a body before inserting a datum entity.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="173"/>
      <source>Invalid Selection</source>
      <translation>Invalid Selection</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="500"/>
      <source>Sub-shape binder</source>
      <translation>Sub-shape binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1107"/>
      <source>No sketch to work on</source>
      <translation>No sketch to work on</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1108"/>
      <source>No sketch is available in the document</source>
      <translation>No sketch is available in the document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1195"/>
      <location filename="../../ViewProviderDatum.cpp" line="260"/>
      <location filename="../../ViewProvider.cpp" line="165"/>
      <location filename="../../SketchWorkflow.cpp" line="779"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="99"/>
      <source>Close this dialog?</source>
      <translation>Close this dialog?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1876"/>
      <location filename="../../Command.cpp" line="1911"/>
      <source>Wrong selection</source>
      <translation>Wrong selection</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1877"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Select an edge, face, or body from a single body.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1884"/>
      <location filename="../../Command.cpp" line="2335"/>
      <source>Selection is not in the active body</source>
      <translation>Selection is not in the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1912"/>
      <source>Shape of the selected part is empty</source>
      <translation>Shape of the selected part is empty</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2085"/>
      <location filename="../../Command.cpp" line="2113"/>
      <source>Wrong Selection</source>
      <translation>Wrong Selection</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>Select faces from a single body</source>
      <translation>Select faces from a single body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2093"/>
      <source>Selection Outside Active Body</source>
      <translation>Selection Outside Active Body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2094"/>
      <source>Select faces from the active body</source>
      <translation>Select faces from the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2102"/>
      <source>Wrong Object Type</source>
      <translation>Wrong Object Type</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2103"/>
      <location filename="../../Command.cpp" line="2114"/>
      <source>Defeaturing works only on faces</source>
      <translation>Defeaturing works only on faces</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2336"/>
      <source>Select only one feature in an active body.</source>
      <translation>Select only one feature in an active body.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1885"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Select an edge, face, or body from an active body.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1078"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>Consider using a shape binder or a base feature to reference external geometry in a body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1898"/>
      <source>Wrong object type</source>
      <translation>Wrong object type</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1899"/>
      <source>%1 works only on parts.</source>
      <translation>%1 works only on parts.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>Part creation failed</source>
      <translation>Part creation failed</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>Failed to create a part object.</source>
      <translation>Failed to create a part object.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="139"/>
      <location filename="../../CommandBody.cpp" line="147"/>
      <location filename="../../CommandBody.cpp" line="166"/>
      <location filename="../../CommandBody.cpp" line="218"/>
      <source>Bad base feature</source>
      <translation>Bad base feature</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="140"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>A body cannot be based on a Part Design feature.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="148"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 already belongs to a body and cannot be used as a base feature for another body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="185"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>The selected shape consists of multiple solids.
This may lead to unexpected results.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="191"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>The selected shape consists of multiple shells.
This may lead to unexpected results.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="197"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>The selected shape consists of only a shell.
This may lead to unexpected results.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="203"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="210"/>
      <source>Base feature</source>
      <translation>Base feature</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="219"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Body may be based on no more than one feature.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="234"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="427"/>
      <source>Nothing to migrate</source>
      <translation>Nothing to migrate</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>Select exactly one Part Design feature or a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="699"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>Could not determine a body for the selected feature '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="880"/>
      <source>Only features of a single source body can be moved</source>
      <translation>Only features of a single source body can be moved</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="615"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Sketch plane cannot be migrated</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="167"/>
      <source>Base feature (%1) has an empty shape.</source>
      <translation>Base feature (%1) has an empty shape.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="428"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>No Part Design features without body found Nothing to migrate.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="690"/>
      <location filename="../../CommandBody.cpp" line="698"/>
      <location filename="../../CommandBody.cpp" line="712"/>
      <location filename="../../CommandBody.cpp" line="1066"/>
      <location filename="../../CommandBody.cpp" line="1076"/>
      <source>Selection error</source>
      <translation>Selection error</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="713"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Only a solid feature can be the tip of a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="849"/>
      <location filename="../../CommandBody.cpp" line="879"/>
      <location filename="../../CommandBody.cpp" line="897"/>
      <source>Features cannot be moved</source>
      <translation>Features cannot be moved</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="850"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Some of the selected features have dependencies in the source body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="898"/>
      <source>There are no other bodies to move to</source>
      <translation>There are no other bodies to move to</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1067"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Impossible to move the base feature of a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1077"/>
      <source>Select one or more features from the same body.</source>
      <translation>Select one or more features from the same body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1092"/>
      <source>Beginning of the body</source>
      <translation>Beginning of the body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1173"/>
      <source>Dependency violation</source>
      <translation>Dependency violation</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1174"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Early feature must not depend on later feature.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="309"/>
      <source>No previous feature found</source>
      <translation>No previous feature found</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="310"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>It is not possible to create a subtractive feature without a base feature available</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="433"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="299"/>
      <source>Vertical sketch axis</source>
      <translation>Vertical sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="434"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="300"/>
      <source>Horizontal sketch axis</source>
      <translation>Horizontal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="302"/>
      <source>Construction line %1</source>
      <translation>Construction line %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="97"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="203"/>
      <source>Active Body Required</source>
      <translation>Active Body Required</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="145"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="204"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="266"/>
      <source>Feature is not in a body</source>
      <translation>Feature is not in a body</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="267"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>In order to use this feature it needs to belong to a body object in the document.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="313"/>
      <source>Feature is not in a part</source>
      <translation>Feature is not in a part</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="314"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>In order to use this feature it needs to belong to a part object in the document.</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="121"/>
      <location filename="../../ViewProviderDressUp.cpp" line="71"/>
      <location filename="../../ViewProviderTransformed.cpp" line="68"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="228"/>
      <source>Edit %1</source>
      <translation>Edit %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="134"/>
      <source>Set Face Colors</source>
      <translation>Set Face Colors</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="214"/>
      <source>Plane</source>
      <translation>Plane</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="209"/>
      <source>Line</source>
      <translation>Line</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="219"/>
      <source>Point</source>
      <translation>Point</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>Coordinate System</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="236"/>
      <source>Edit Datum</source>
      <translation>Edit Datum</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="100"/>
      <source>Feature error</source>
      <translation>Feature error</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="101"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1 misses a base feature.
This feature is broken and cannot be edited.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="223"/>
      <source>Edit Shape Binder</source>
      <translation>Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Synchronize</source>
      <translation>Synchronize</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="354"/>
      <source>Select Bound Object</source>
      <translation>Select Bound Object</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="156"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>The document "%1" you are editing was designed with an old version of Part Design workbench.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="163"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>Migrate in order to use modern Part Design features?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="168"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="175"/>
      <source>Make the migration automatically?</source>
      <translation>Make the migration automatically?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="178"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="191"/>
      <source>Migrate Manually</source>
      <translation>Migrate Manually</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="351"/>
      <source>Edit Boolean</source>
      <translation>Edit Boolean</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="42"/>
      <source>Edit Chamfer</source>
      <translation>Edit Chamfer</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="43"/>
      <source>Edit Draft</source>
      <translation>Edit Draft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="42"/>
      <source>Edit Fillet</source>
      <translation>Edit Fillet</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="45"/>
      <source>Edit Groove</source>
      <translation>Edit Groove</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="50"/>
      <source>Edit Helix</source>
      <translation>Edit Helix</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="130"/>
      <source>Edit Hole</source>
      <translation>Edit Hole</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="40"/>
      <source>Edit Linear Pattern</source>
      <translation>Edit Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="67"/>
      <source>Edit Loft</source>
      <translation>Edit Loft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="40"/>
      <source>Edit Mirror</source>
      <translation>Edit Mirror</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="49"/>
      <source>Edit Multi-Transform</source>
      <translation>Edit Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="45"/>
      <source>Edit Pad</source>
      <translation>Edit Pad</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="77"/>
      <source>Edit Pipe</source>
      <translation>Edit Pipe</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="47"/>
      <source>Edit Pocket</source>
      <translation>Edit Pocket</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="40"/>
      <source>Edit Polar Pattern</source>
      <translation>Edit Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="52"/>
      <source>Edit Primitive</source>
      <translation>Edit Primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="45"/>
      <source>Edit Revolution</source>
      <translation>Edit Revolution</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="40"/>
      <source>Edit Scale</source>
      <translation>Edit Scale</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="42"/>
      <source>Edit Thickness</source>
      <translation>Edit Thickness</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDefeaturing.cpp" line="39"/>
      <source>Edit Defeaturing</source>
      <translation>Edit Defeaturing</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>Sprocket Parameters</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>Sprocket reference</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="66"/>
      <source>ANSI 25</source>
      <translation>ANSI 25</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="71"/>
      <source>ANSI 35</source>
      <translation>ANSI 35</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="76"/>
      <source>ANSI 41</source>
      <translation>ANSI 41</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="81"/>
      <source>ANSI 40</source>
      <translation>ANSI 40</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="86"/>
      <source>ANSI 50</source>
      <translation>ANSI 50</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="91"/>
      <source>ANSI 60</source>
      <translation>ANSI 60</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="96"/>
      <source>ANSI 80</source>
      <translation>ANSI 80</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="101"/>
      <source>ANSI 100</source>
      <translation>ANSI 100</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="106"/>
      <source>ANSI 120</source>
      <translation>ANSI 120</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="111"/>
      <source>ANSI 140</source>
      <translation>ANSI 140</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="116"/>
      <source>ANSI 160</source>
      <translation>ANSI 160</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="121"/>
      <source>ANSI 180</source>
      <translation>ANSI 180</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="126"/>
      <source>ANSI 200</source>
      <translation>ANSI 200</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="131"/>
      <source>ANSI 240</source>
      <translation>ANSI 240</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="136"/>
      <source>Bicycle with derailleur</source>
      <translation>Bicycle with derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>Bicycle without derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>Chain pitch</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>Chain roller diameter</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>Tooth width</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="146"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="151"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 08B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="156"/>
      <source>ISO 606 10B</source>
      <translation>ISO 606 10B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="161"/>
      <source>ISO 606 12B</source>
      <translation>ISO 606 12B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="166"/>
      <source>ISO 606 16B</source>
      <translation>ISO 606 16B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="171"/>
      <source>ISO 606 20B</source>
      <translation>ISO 606 20B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="176"/>
      <source>ISO 606 24B</source>
      <translation>ISO 606 24B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="181"/>
      <source>Motorcycle 420</source>
      <translation>Motorcycle 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>Motorcycle 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>Motorcycle 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>Motorcycle 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>Motorcycle 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>Motorcycle 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>Motorcycle 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 in</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="919"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Live update of changes to the thread
Note that the calculation can take some time</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1114"/>
      <source>Thread Depth</source>
      <translation>Thread Depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1167"/>
      <source>Customize thread clearance</source>
      <translation>Customize thread clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="794"/>
      <source>Clearance</source>
      <translation>Clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="219"/>
      <source>Head type</source>
      <translation>Head type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="248"/>
      <source>Depth type</source>
      <translation>Depth type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="340"/>
      <source>Head diameter</source>
      <translation>Head diameter</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="388"/>
      <source>Head depth</source>
      <translation>Head depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="763"/>
      <source>Clearance / Passthrough</source>
      <translation>Clearance / Passthrough</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="781"/>
      <source>Hole type</source>
      <translation>Hole type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="923"/>
      <source>Update thread view</source>
      <translation>Update thread view</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1173"/>
      <source>Custom Clearance</source>
      <translation>Custom Clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1186"/>
      <source>Custom Thread clearance value</source>
      <translation>Custom Thread clearance value</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="193"/>
      <source>Size</source>
      <translation>Size</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="807"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Hole clearance
Only available for holes without thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="812"/>
      <source>Standard</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="768"/>
      <source>Tap drill</source>
      <translation>Tap drill</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="773"/>
      <source>Threaded</source>
      <translation>Threaded</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Close</source>
      <translation>Close</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="822"/>
      <source>Wide</source>
      <translation>Wide</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="900"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>Whether the hole gets a modelled thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="903"/>
      <source>Model Thread</source>
      <translation>Model Thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="956"/>
      <source>Class</source>
      <translation>Class</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="943"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Tolerance class for threaded holes according to hole profile</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="647"/>
      <source>Diameter</source>
      <translation>Diameter</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="669"/>
      <source>Hole diameter</source>
      <translation>Hole diameter</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="602"/>
      <source>Depth</source>
      <translation>Depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="38"/>
      <source>Profile and Placement</source>
      <translation>Profile and Placement</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="52"/>
      <source>Base profile types</source>
      <translation>Base profile types</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="60"/>
      <source>Circles and arcs</source>
      <translation>Circles and arcs</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Points, circles and arcs</source>
      <translation>Points, circles and arcs</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="70"/>
      <source>Points</source>
      <translation>Points</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="78"/>
      <source>Start</source>
      <translation>Start</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="86"/>
      <source>Profile plane</source>
      <translation>Profile plane</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="91"/>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Offset</source>
      <translation>Offset</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="96"/>
      <location filename="../../TaskHoleParameters.ui" line="104"/>
      <source>Reference</source>
      <translation>Reference</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="121"/>
      <source>Pick Reference</source>
      <translation>Pick Reference</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Hole</source>
      <translation>Hole</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="265"/>
      <location filename="../../TaskHoleParameters.ui" line="1084"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="270"/>
      <source>Through all</source>
      <translation>Through all</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="292"/>
      <source>Custom head values</source>
      <translation>Custom head values</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Drill angle</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="498"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Include in depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="705"/>
      <source>Switch direction</source>
      <translation>Switch direction</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1210"/>
      <source>Operation</source>
      <translation>Operation</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1221"/>
      <source>Subtraction</source>
      <translation>Subtraction</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1226"/>
      <source>Common</source>
      <translation>Common</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Thread</source>
      <translation>Thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1000"/>
      <source>&amp;Right hand</source>
      <translation>&amp;Right hand</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1016"/>
      <source>&amp;Left hand</source>
      <translation>&amp;Left hand</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Thread Depth Type</source>
      <translation>Thread Depth Type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1079"/>
      <source>Hole depth</source>
      <translation>Hole depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1089"/>
      <source>Tapped (DIN76)</source>
      <translation>Tapped (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="232"/>
      <source>Cut type for screw heads</source>
      <translation>Cut type for screw heads</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="286"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Check to override the values predefined by the 'Type'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="401"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>For countersinks this is the depth of
the screw's top below the surface</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="557"/>
      <source>Countersink angle</source>
      <translation>Countersink angle</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="494"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>The size of the drill point will be taken into
account for the depth of blind holes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="721"/>
      <source>Tapered</source>
      <translation>Tapered</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="734"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="702"/>
      <source>Reverses the hole direction</source>
      <translation>Reverses the hole direction</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>No message</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Sketch</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Part Design</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>Datums</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation>Additive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation>Subtractive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation>Dress-Up Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation>Transformation Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation>Sprocket…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation>Involute Gear</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation>Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Measure</source>
      <translation>Measure</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Refresh</source>
      <translation>Refresh</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Toggle 3D</source>
      <translation>Toggle 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Helper</source>
      <translation>Part Design Helper</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="67"/>
      <source>Part Design Modeling</source>
      <translation>Part Design Modeling</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Length [mm]</source>
      <translation>Length [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Diameter [mm]</source>
      <translation>Diameter [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Inner diameter [mm]</source>
      <translation>Inner diameter [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Constraint type</source>
      <translation>Constraint type</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>Start edge type</source>
      <translation>Start edge type</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>Start edge size</source>
      <translation>Start edge size</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="54"/>
      <source>End edge type</source>
      <translation>End edge type</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>End edge size</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="69"/>
      <source>Shaft Wizard</source>
      <translation>Shaft Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="77"/>
      <source>Section 1</source>
      <translation>Section 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="78"/>
      <source>Section 2</source>
      <translation>Section 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="82"/>
      <source>Add column</source>
      <translation>Add column</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="130"/>
      <source>Section %s</source>
      <translation>Section %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="178"/>
      <source>None</source>
      <translation>None</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Fixed</source>
      <translation>Fixed</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Force</source>
      <translation>Force</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Bearing</source>
      <translation>Bearing</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="163"/>
      <source>Gear</source>
      <translation>Gear</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="164"/>
      <source>Pulley</source>
      <translation>Pulley</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="181"/>
      <source>Chamfer</source>
      <translation>Chamfer</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="182"/>
      <source>Fillet</source>
      <translation>Fillet</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="60"/>
      <source>All</source>
      <translation>All</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="120"/>
      <source>Missing Module</source>
      <translation>Missing Module</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="126"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>The Plot add-on is not installed. Install it to enable this feature.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="253"/>
      <source>Shaft design wizard...</source>
      <translation>Shaft design wizard...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="256"/>
      <source>Start the shaft design wizard</source>
      <translation>Start the shaft design wizard</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="426"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>Linked object is not a PartDesign feature</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="435"/>
      <source>Tip shape is empty</source>
      <translation>Tip shape is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="80"/>
      <source>BaseFeature link is not set</source>
      <translation>BaseFeature link is not set</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="100"/>
      <location filename="../../../App/FeatureBase.cpp" line="125"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature has an empty shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="172"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Cannot do boolean cut without BaseFeature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="194"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Cannot do boolean with anything but Part::Feature and its derivatives</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="211"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Cannot do boolean operation with invalid base shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="266"/>
      <location filename="../../../App/FeaturePipe.cpp" line="514"/>
      <location filename="../../../App/FeaturePipe.cpp" line="539"/>
      <location filename="../../../App/FeaturePipe.cpp" line="557"/>
      <location filename="../../../App/FeatureFillet.cpp" line="142"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="196"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="535"/>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="435"/>
      <location filename="../../../App/FeatureHelix.cpp" line="453"/>
      <location filename="../../../App/FeatureDraft.cpp" line="335"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="836"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="852"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="865"/>
      <location filename="../../../App/FeatureHole.cpp" line="2144"/>
      <location filename="../../../App/FeatureLoft.cpp" line="387"/>
      <location filename="../../../App/FeatureLoft.cpp" line="422"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>Result has multiple solids: enable 'Allow Compound' in the active body.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="221"/>
      <source>Tool shape is null</source>
      <translation>Tool shape is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="248"/>
      <source>Unsupported boolean operation</source>
      <translation>Unsupported boolean operation</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="402"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>Cannot create a pad with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="407"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>Cannot create a pocket with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="770"/>
      <source>No extrusion geometry was generated.</source>
      <translation>No extrusion geometry was generated.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="794"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>Resulting fused extrusion is null.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="549"/>
      <location filename="../../../App/FeatureHelix.cpp" line="445"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="132"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="827"/>
      <location filename="../../../App/FeatureLoft.cpp" line="415"/>
      <source>Resulting shape is not a solid</source>
      <translation>Resulting shape is not a solid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Failed to create chamfer</source>
      <translation>Failed to create chamfer</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="122"/>
      <location filename="../../../App/FeatureDraft.cpp" line="330"/>
      <source>Resulting shape is null</source>
      <translation>Resulting shape is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="144"/>
      <source>No edges specified</source>
      <translation>No edges specified</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="211"/>
      <source>Chamfer failed: OCC kernel error in chamfer computation</source>
      <translation>Chamfer failed: OCC kernel error in chamfer computation</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="302"/>
      <source>Size must be greater than zero</source>
      <translation>Size must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="313"/>
      <source>Size2 must be greater than zero</source>
      <translation>Size2 must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="320"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>Angle must be greater than 0 and less than 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="97"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>Fillet not possible on selected shapes</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="105"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Fillet radius must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="159"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1785"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="451"/>
      <source>Failed to obtain profile shape</source>
      <translation>Failed to obtain profile shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="505"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Creation failed because direction is orthogonal to sketch's normal vector</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="298"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="528"/>
      <source>Creating a face from sketch failed</source>
      <translation>Creating a face from sketch failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="333"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>Revolve axis intersects the sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="494"/>
      <source>No revolution geometry was generated</source>
      <translation>No revolution geometry was generated</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="520"/>
      <source>Could not revolve the sketch!</source>
      <translation>Could not revolve the sketch!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="849"/>
      <source>Cannot project the axis because the profile is not planar</source>
      <translation>Cannot project the axis because the profile is not planar</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="860"/>
      <source>Cannot project an axis perpendicular to the profile plane</source>
      <translation>Cannot project an axis perpendicular to the profile plane</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="208"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="236"/>
      <source>Error: Pitch too small!</source>
      <translation>Error: Pitch too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="241"/>
      <location filename="../../../App/FeatureHelix.cpp" line="264"/>
      <source>Error: height too small!</source>
      <translation>Error: height too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="250"/>
      <source>Error: pitch too small!</source>
      <translation>Error: pitch too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="255"/>
      <location filename="../../../App/FeatureHelix.cpp" line="269"/>
      <location filename="../../../App/FeatureHelix.cpp" line="278"/>
      <source>Error: turns too small!</source>
      <translation>Error: turns too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="284"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Error: either height or growth must not be zero!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="302"/>
      <source>Error: unsupported mode</source>
      <translation>Error: unsupported mode</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="316"/>
      <source>Error: No valid sketch or face</source>
      <translation>Error: No valid sketch or face</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="329"/>
      <source>Error: Face must be planar</source>
      <translation>Error: Face must be planar</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2507"/>
      <source>Error: Result is not a solid</source>
      <translation>Error: Result is not a solid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="414"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Error: There is nothing to subtract</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Error: Could not create face from sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1270"/>
      <source>Thread type is invalid</source>
      <translation>Thread type is invalid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1798"/>
      <source>Hole error: Diameter too small</source>
      <translation>Hole error: Diameter too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1849"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Hole error: Unsupported length specification</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1855"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Hole error: Invalid hole depth</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1881"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Hole error: Invalid taper angle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1905"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Hole error: Hole cut diameter too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1910"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Hole error: Hole cut depth must be less than hole depth</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1917"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Hole error: Hole cut depth must be greater or equal to zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1947"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Hole error: Invalid countersink</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Hole error: Invalid drill point angle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2000"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Hole error: Invalid drill point</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2037"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Hole error: Could not revolve sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2044"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Hole error: Resulting shape is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2067"/>
      <source>Hole error: Finding axis failed</source>
      <translation>Hole error: Finding axis failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2116"/>
      <location filename="../../../App/FeatureHole.cpp" line="2124"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Boolean operation failed on profile Edge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2131"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>Boolean operation produced non-solid on profile Edge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="258"/>
      <source>Boolean operation failed</source>
      <translation>Boolean operation failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2157"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2330"/>
      <source>Thread type out of range</source>
      <translation>Thread type out of range</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2333"/>
      <source>Thread size out of range</source>
      <translation>Thread size out of range</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2481"/>
      <source>Error: Thread could not be built</source>
      <translation>Error: Thread could not be built</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="246"/>
      <source>Loft: At least one section is needed</source>
      <translation>Loft: At least one section is needed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="439"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Loft: A fatal error occurred when making the loft</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="293"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Loft: Creating a face from sketch failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="477"/>
      <location filename="../../../App/FeatureLoft.cpp" line="357"/>
      <source>Loft: Failed to create shell</source>
      <translation>Loft: Failed to create shell</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="880"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="187"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Pipe: Could not obtain profile shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="194"/>
      <source>No spine linked</source>
      <translation>No spine linked</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="209"/>
      <source>No auxiliary spine linked.</source>
      <translation>No auxiliary spine linked.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="231"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="240"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Pipe: At least one section is needed when using a single point for profile</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="258"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>Pipe: All sections need to be Part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="267"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Pipe: Could not obtain section shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Pipe: Only the profile and last section can be vertices</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="288"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Multisections need to have the same amount of inner wires as the base section</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="321"/>
      <source>Path must not be a null shape</source>
      <translation>Path must not be a null shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="361"/>
      <source>Pipe could not be built</source>
      <translation>Pipe could not be built</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="509"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Pipe: There is nothing to subtract from</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="571"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>A fatal error occurred when making the pipe</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Invalid element in spine.</source>
      <translation>Invalid element in spine.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="703"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Element in spine is neither an edge nor a wire.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="716"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Spine is neither an edge nor a wire.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="721"/>
      <source>Invalid spine.</source>
      <translation>Invalid spine.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="103"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Cannot subtract primitive feature without base feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="124"/>
      <location filename="../../../App/FeatureLoft.cpp" line="408"/>
      <source>Failed to perform boolean operation</source>
      <translation>Failed to perform boolean operation</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="208"/>
      <source>Length of box too small</source>
      <translation>Length of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="213"/>
      <source>Width of box too small</source>
      <translation>Width of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="218"/>
      <source>Height of box too small</source>
      <translation>Height of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="266"/>
      <source>Radius of cylinder too small</source>
      <translation>Radius of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="271"/>
      <source>Height of cylinder too small</source>
      <translation>Height of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="276"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Rotation angle of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="333"/>
      <source>Radius of sphere too small</source>
      <translation>Radius of sphere too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="385"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="390"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Radius of cone cannot be negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="395"/>
      <source>Height of cone too small</source>
      <translation>Height of cone too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="475"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="480"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Radius of ellipsoid too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="574"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="579"/>
      <source>Radius of torus too small</source>
      <translation>Radius of torus too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="664"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Polygon of prism is invalid, must have 3 or more sides</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="669"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Circumradius of the polygon, of the prism, is too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="674"/>
      <source>Height of prism is too small</source>
      <translation>Height of prism is too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="761"/>
      <source>delta x of wedge too small</source>
      <translation>delta x of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="767"/>
      <source>delta y of wedge too small</source>
      <translation>delta y of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="773"/>
      <source>delta z of wedge too small</source>
      <translation>delta z of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="779"/>
      <source>delta z2 of wedge is negative</source>
      <translation>delta z2 of wedge is negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="785"/>
      <source>delta x2 of wedge is negative</source>
      <translation>delta x2 of wedge is negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="246"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="261"/>
      <source>Angle of revolution too large</source>
      <translation>Angle of revolution too large</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="254"/>
      <source>Angle of revolution too small</source>
      <translation>Angle of revolution too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="292"/>
      <source>Reference axis is invalid</source>
      <translation>Reference axis is invalid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="819"/>
      <source>Fusion with base feature failed</source>
      <translation>Fusion with base feature failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Transformation feature Linked object is not a Part object</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="120"/>
      <source>No features selected to be mirrored.</source>
      <translation>No features selected to be mirrored.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="127"/>
      <source>No features selected to be patterned.</source>
      <translation>No features selected to be patterned.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="130"/>
      <source>No features selected to be transformed.</source>
      <translation>No features selected to be transformed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="447"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Cannot transform invalid support shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="561"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>Shape of additive/subtractive feature is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="552"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Only additive and subtractive features can be transformed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="181"/>
      <source>Invalid face reference</source>
      <translation>Invalid face reference</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDefeaturing.cpp" line="75"/>
      <source>Defeaturing failed: result is null</source>
      <translation>Defeaturing failed: result is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDefeaturing.cpp" line="84"/>
      <source>Defeaturing did not produce a single solid</source>
      <translation>Defeaturing did not produce a single solid</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Involute Gear</source>
      <translation>Involute Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="66"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>Creates or edits the involute gear definition</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="65"/>
      <source>Sprocket</source>
      <translation>Sprocket</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="69"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>Creates or edits the sprocket definition.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>Show final result</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>Show preview overlay</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="52"/>
      <source>Preview</source>
      <translation>Preview</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="225"/>
      <source>Shaft Design Wizard</source>
      <translation>Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="228"/>
      <source>Starts the shaft design wizard</source>
      <translation>Starts the shaft design wizard</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="141"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>Failure while computing removed volume preview: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="180"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2938"/>
      <source>Create Datum</source>
      <translation>Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2939"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2973"/>
      <source>Create Datum</source>
      <translation>Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2974"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>Creates an additive box by its width, height, and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>Creates an additive cylinder by its radius, height, and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="235"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>Creates an additive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <source>Creates an additive cone</source>
      <translation>Creates an additive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="250"/>
      <source>Creates an additive ellipsoid</source>
      <translation>Creates an additive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="256"/>
      <source>Creates an additive torus</source>
      <translation>Creates an additive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="262"/>
      <source>Creates an additive prism</source>
      <translation>Creates an additive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="268"/>
      <source>Creates an additive wedge</source>
      <translation>Creates an additive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="402"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>Creates a subtractive box by its width, height and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="411"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>Creates a subtractive cylinder by its radius, height and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="420"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>Creates a subtractive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="429"/>
      <source>Creates a subtractive cone</source>
      <translation>Creates a subtractive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="435"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>Creates a subtractive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="441"/>
      <source>Creates a subtractive torus</source>
      <translation>Creates a subtractive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="447"/>
      <source>Creates a subtractive prism</source>
      <translation>Creates a subtractive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="453"/>
      <source>Creates a subtractive wedge</source>
      <translation>Creates a subtractive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1202"/>
      <source>Attachment</source>
      <translation>Attachment</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="1116"/>
      <source>Revolution Parameters</source>
      <translation>Revolution Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="1126"/>
      <source>Groove Parameters</source>
      <translation>Groove Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="39"/>
      <source>Transformed Feature Messages</source>
      <translation>Transformed Feature Messages</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="220"/>
      <source>Active Body</source>
      <translation>Active Body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="44"/>
      <source>Chamfer Parameters</source>
      <translation>Chamfer Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation>Datum Plane Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation>Datum Line Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation>Datum Point Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation>Local Coordinate System Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="45"/>
      <source>Draft Parameters</source>
      <translation>Draft Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="44"/>
      <source>Fillet Parameters</source>
      <translation>Fillet Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="41"/>
      <source>Linear Pattern Parameters</source>
      <translation>Linear Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="41"/>
      <source>Mirror Parameters</source>
      <translation>Mirror Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="41"/>
      <source>Multi-Transform Parameters</source>
      <translation>Multi-Transform Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="41"/>
      <source>Polar Pattern Parameters</source>
      <translation>Polar Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="41"/>
      <source>Scale Parameters</source>
      <translation>Scale Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="44"/>
      <source>Thickness Parameters</source>
      <translation>Thickness Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="425"/>
      <source>Select a sketch or shape containing the pattern points</source>
      <translation>Select a sketch or shape containing the pattern points</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="431"/>
      <source>Select a sketch, Sub-Shape Binder, or path edge</source>
      <translation>Select a sketch, Sub-Shape Binder, or path edge</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="440"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>Select a direction reference (edge, face, datum line)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="504"/>
      <source>Invalid selection. Select a sketch or shape containing points.</source>
      <translation>Invalid selection. Select a sketch or shape containing points.</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="507"/>
      <source>Invalid selection. Select a sketch, Sub-Shape Binder, or path edge.</source>
      <translation>Invalid selection. Select a sketch, Sub-Shape Binder, or path edge.</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="509"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>Invalid selection. Select an edge, planar face, or datum line.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDefeaturingParameters</name>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="20"/>
      <source>Toggles selection mode</source>
      <translation>Toggles selection mode</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Select</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="33"/>
      <source>Select an item in the 3D view to highlight it</source>
      <translation>Select an item in the 3D view to highlight it</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.cpp" line="118"/>
      <source>Empty defeaturing created</source>
      <translation>Empty defeaturing created</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDefeaturing</name>
    <message>
      <location filename="../../Command.cpp" line="2132"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2133"/>
      <source>Defeaturing</source>
      <translation>Defeaturing</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2134"/>
      <source>Removes selected faces from a solid</source>
      <translation>Removes selected faces from a solid</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCircularPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2489"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2490"/>
      <source>Circular Pattern</source>
      <translation>Circular Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2491"/>
      <source>Duplicates the selected features or the active body in concentric circular patterns</source>
      <translation>Duplicates the selected features or the active body in concentric circular patterns</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPathPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2556"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2557"/>
      <source>Path Pattern</source>
      <translation>Path Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2558"/>
      <source>Duplicates the selected features or the active body along a path</source>
      <translation>Duplicates the selected features or the active body along a path</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPointPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2594"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2595"/>
      <source>Point Pattern</source>
      <translation>Point Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2596"/>
      <source>Duplicates the selected features or the active body at points from a shape</source>
      <translation>Duplicates the selected features or the active body at points from a shape</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDefeaturing</name>
    <message>
      <location filename="../../ViewProviderDefeaturing.h" line="40"/>
      <source>Defeaturing Parameters</source>
      <translation>Defeaturing Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPathPattern</name>
    <message>
      <location filename="../../ViewProviderPathPattern.h" line="18"/>
      <source>Path Pattern Parameters</source>
      <translation>Path Pattern Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPathPattern.cpp" line="11"/>
      <source>Edit Path Pattern</source>
      <translation>Edit Path Pattern</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="138"/>
      <source>%1 fine dragging</source>
      <translation>%1 fine dragging</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="141"/>
      <source>%1 coarse dragging</source>
      <translation>%1 coarse dragging</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="264"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Adjust the parameters and try again.</source>
      <translation>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Adjust the parameters and try again.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="443"/>
      <source>Input Error</source>
      <translation>Input Error</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderCircularPattern</name>
    <message>
      <location filename="../../ViewProviderCircularPattern.h" line="18"/>
      <source>Circular Pattern Parameters</source>
      <translation>Circular Pattern Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderCircularPattern.cpp" line="11"/>
      <source>Edit Circular Pattern</source>
      <translation>Edit Circular Pattern</translation>
    </message>
  </context>
  <context>
    <name>BooleanToolSelection</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="83"/>
      <source>The result Body cannot be used as a Boolean tool.</source>
      <translation>The result Body cannot be used as a Boolean tool.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPointPattern</name>
    <message>
      <location filename="../../ViewProviderPointPattern.h" line="18"/>
      <source>Point Pattern Parameters</source>
      <translation>Point Pattern Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPointPattern.cpp" line="11"/>
      <source>Edit Point Pattern</source>
      <translation>Edit Point Pattern</translation>
    </message>
  </context>
</TS>
