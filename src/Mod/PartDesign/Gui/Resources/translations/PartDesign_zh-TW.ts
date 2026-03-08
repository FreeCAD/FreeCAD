<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-TW" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="80"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>螺旋線起點的中心點；從參考軸導出。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>螺旋線的方向；從參考軸導出。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>The reference axis of the helix.</source>
      <translation>螺旋線的參考軸。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>螺旋線輸入模式指定使用者來設定哪些屬性。
相關屬性之後將被計算。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="116"/>
      <source>The axial distance between two turns.</source>
      <translation>兩圈之間的軸向距離。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="123"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>螺旋路徑的高度，不考慮輪廓的範圍。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="133"/>
      <source>The number of turns in the helix.</source>
      <translation>螺旋線的圈數。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="141"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>圍繞螺旋線形成的錐體角度。
非零值會將螺旋線變為錐形螺旋。
正值使半徑增大，負值使半徑縮小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="154"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>每圈螺旋半徑的增長。
非零值將螺旋線變成錐形螺旋線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>將轉向方向設置為左手，
也就是說以逆時針方向沿著軸移動。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="176"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>確定螺旋線是否指向與軸相反的方向。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="186"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>如果設定，其結果將是輪廓和預先存在的主體的交集。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="196"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>如果為 false，該工具將根據輪廓邊界框建議間距的初始值，
從而避免自相交。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="208"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>螺旋的融合公差，如果螺旋形狀不能與零件很好地融合，則增加。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="106"/>
      <source>Number of gear teeth</source>
      <translation>輪齒數目</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="118"/>
      <source>Pressure angle of gear teeth</source>
      <translation>輪齒壓力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="112"/>
      <source>Module of the gear</source>
      <translation>齒輪模數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>設定 True=2 時，每條曲線有3個控制點；設定 False =1 時，每條曲線有4個控制點。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="135"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=外齒輪，False=內齒輪</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="144"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>齒輪的齒從節圓到齒尖的高度，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="153"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>齒輪的齒從節圓到齒根的高度，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="162"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>齒根處圓角的半徑，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="171"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>參考輪廓向外移動的距離，按模數進行標準化。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1661"/>
      <source>Additive Helix</source>
      <translation>添加螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1662"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a helix and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1561"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1562"/>
      <source>Additive Loft</source>
      <translation type="unfinished">Additive Loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1563"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation type="unfinished">Lofts the selected sketch or profile along a path and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1461"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1462"/>
      <source>Additive Pipe</source>
      <translation type="unfinished">Additive Pipe</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1463"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a path and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>New Body</source>
      <translation type="unfinished">New Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="92"/>
      <source>Creates a new body and activates it</source>
      <translation type="unfinished">Creates a new body and activates it</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2576"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2577"/>
      <source>Boolean Operation</source>
      <translation>布林運算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2578"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation type="unfinished">Applies boolean operations with the selected objects and the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Local Coordinate System</source>
      <translation>局部座標系統</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new local coordinate system</source>
      <translation type="unfinished">Creates a new local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1987"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1988"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1989"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation type="unfinished">Applies a chamfer to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="489"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="490"/>
      <source>Clone</source>
      <translation>複製</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="491"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation type="unfinished">Copies a solid object parametrically as the base feature of a new body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Draft</source>
      <translation>草稿</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2018"/>
      <source>Applies a draft to the selected faces</source>
      <translation type="unfinished">Applies a draft to the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>Duplicate &amp;Object</source>
      <translation type="unfinished">Duplicate &amp;Object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>複製已選物件添加至作業中主體</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1959"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1960"/>
      <source>Fillet</source>
      <translation>圓角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1961"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation type="unfinished">Applies a fillet to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1391"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1392"/>
      <source>Groove</source>
      <translation>挖槽</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1393"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation type="unfinished">Revolves the sketch or profile around a line or axis and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1285"/>
      <source>Hole</source>
      <translation>挖孔</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1287"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation type="unfinished">Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Line</source>
      <translation type="unfinished">Datum Line</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum line</source>
      <translation type="unfinished">Creates a new datum line</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2271"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2272"/>
      <source>Linear Pattern</source>
      <translation type="unfinished">Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2273"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation type="unfinished">Duplicates the selected features or the active body in a linear pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="385"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="386"/>
      <source>Migrate</source>
      <translation>遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="387"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation type="unfinished">Migrates the document to the modern Part Design workflow</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2214"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2215"/>
      <source>Mirror</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2216"/>
      <source>Mirrors the selected features or active body</source>
      <translation type="unfinished">Mirrors the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="821"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="822"/>
      <source>Move Object To…</source>
      <translation type="unfinished">Move Object To…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="823"/>
      <source>Moves the selected object to another body</source>
      <translation>移動被選物件至另一主體</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1016"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1017"/>
      <source>Move Feature After…</source>
      <translation type="unfinished">Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1018"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation type="unfinished">Moves the selected feature after another feature in the same body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Set Tip</source>
      <translation type="unfinished">Set Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation type="unfinished">Moves the tip of the body to the selected feature</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2445"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2446"/>
      <source>Multi-Transform</source>
      <translation type="unfinished">Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2447"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation type="unfinished">Applies multiple transformations to the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="573"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="574"/>
      <source>New Sketch</source>
      <translation type="unfinished">New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="575"/>
      <source>Creates a new sketch</source>
      <translation type="unfinished">Creates a new sketch</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1226"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>Pad</source>
      <translation>填充</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Datum Plane</source>
      <translation type="unfinished">Datum Plane</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Creates a new datum plane</source>
      <translation type="unfinished">Creates a new datum plane</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1256"/>
      <source>Pocket</source>
      <translation>凹陷</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1257"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="250"/>
      <source>Datum Point</source>
      <translation type="unfinished">Datum Point</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="251"/>
      <source>Creates a new datum point</source>
      <translation type="unfinished">Creates a new datum point</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2340"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2341"/>
      <source>Polar Pattern</source>
      <translation type="unfinished">Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2342"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation type="unfinished">Duplicates the selected features or the active body in a circular pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1329"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1330"/>
      <source>Revolve</source>
      <translation>旋轉</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1331"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation type="unfinished">Revolves the selected sketch or profile around a line or axis and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2403"/>
      <source>Scale</source>
      <translation>縮放</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2404"/>
      <source>Scales the selected features or the active body</source>
      <translation type="unfinished">Scales the selected features or the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="313"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="314"/>
      <source>Shape Binder</source>
      <translation type="unfinished">Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="315"/>
      <source>Creates a new shape binder</source>
      <translation type="unfinished">Creates a new shape binder</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="383"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="384"/>
      <source>Sub-Shape Binder</source>
      <translation>子形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="385"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation type="unfinished">Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1744"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>Subtractive Helix</source>
      <translation type="unfinished">Subtractive Helix</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a helix and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1611"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>Subtractive Loft</source>
      <translation type="unfinished">Subtractive Loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Lofts the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1511"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1512"/>
      <source>Subtractive Pipe</source>
      <translation type="unfinished">Subtractive Pipe</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1513"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2087"/>
      <source>Thickness</source>
      <translation>厚度</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2088"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation type="unfinished">Applies thickness and removes the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="74"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="75"/>
      <source>Additive Primitive</source>
      <translation type="unfinished">Additive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>Creates an additive primitive</source>
      <translation type="unfinished">Creates an additive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Box</source>
      <translation>添加立方體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Additive Cylinder</source>
      <translation>添加圓柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="231"/>
      <source>Additive Sphere</source>
      <translation>添加球體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="240"/>
      <source>Additive Cone</source>
      <translation>添加圓錐體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="246"/>
      <source>Additive Ellipsoid</source>
      <translation>添加橢圓體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="252"/>
      <source>Additive Torus</source>
      <translation>添加中空環型體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="258"/>
      <source>Additive Prism</source>
      <translation>添加角柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>Additive Wedge</source>
      <translation>添加楔形體</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="282"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="283"/>
      <source>Subtractive Primitive</source>
      <translation type="unfinished">Subtractive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>Creates a subtractive primitive</source>
      <translation type="unfinished">Creates a subtractive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="398"/>
      <source>Subtractive Box</source>
      <translation>立方體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="407"/>
      <source>Subtractive Cylinder</source>
      <translation>圓柱體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="416"/>
      <source>Subtractive Sphere</source>
      <translation>球體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="425"/>
      <source>Subtractive Cone</source>
      <translation>圓錐體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="431"/>
      <source>Subtractive Ellipsoid</source>
      <translation>橢圓體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="437"/>
      <source>Subtractive Torus</source>
      <translation>中空環型體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="443"/>
      <source>Subtractive Prism</source>
      <translation>角柱體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="449"/>
      <source>Subtractive Wedge</source>
      <translation>楔形體除料</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="335"/>
      <source>Edit Shape Binder</source>
      <translation type="unfinished">Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Create Shape Binder</source>
      <translation type="unfinished">Create Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="439"/>
      <source>Create Sub-Shape Binder</source>
      <translation type="unfinished">Create Sub-Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="508"/>
      <source>Create Clone</source>
      <translation>建立克隆</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1110"/>
      <source>Make Copy</source>
      <translation type="unfinished">Make Copy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2500"/>
      <source>Convert to Multi-Transform feature</source>
      <translation type="unfinished">Convert to Multi-Transform feature</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="253"/>
      <source>Sketch on Face</source>
      <translation type="unfinished">Sketch on Face</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="314"/>
      <source>Make copy</source>
      <translation>製作拷貝</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="516"/>
      <location filename="../../SketchWorkflow.cpp" line="772"/>
      <source>New Sketch</source>
      <translation type="unfinished">New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2597"/>
      <source>Create Boolean</source>
      <translation>建立布林運算</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="222"/>
      <location filename="../../DlgActiveBody.cpp" line="101"/>
      <source>Add a Body</source>
      <translation>添加一個主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation type="unfinished">Migrate legacy Part Design features to bodies</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="769"/>
      <source>Duplicate a Part Design object</source>
      <translation type="unfinished">Duplicate a Part Design object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1110"/>
      <source>Move a feature inside body</source>
      <translation type="unfinished">Move a feature inside body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="723"/>
      <source>Move tip to selected feature</source>
      <translation>移動尖點至被選的特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="926"/>
      <source>Move an object</source>
      <translation>移動物件</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="258"/>
      <source>Mirror</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="298"/>
      <source>Linear Pattern</source>
      <translation type="unfinished">Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="347"/>
      <source>Polar Pattern</source>
      <translation type="unfinished">Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
      <source>Scale</source>
      <translation>縮放</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Face Tools</source>
      <translation type="unfinished">Face Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Edge Tools</source>
      <translation type="unfinished">Edge Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Boolean Tools</source>
      <translation type="unfinished">Boolean Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Helper Tools</source>
      <translation type="unfinished">Helper Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Modeling Tools</source>
      <translation type="unfinished">Modeling Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Create Geometry</source>
      <translation>建立幾何</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation type="unfinished">Involute Parameter</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation type="unfinished">Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation type="unfinished">Module</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation type="unfinished">Pressure angle</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation type="unfinished">High precision</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>真(True)</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>偽(False)</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation type="unfinished">External gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation type="unfinished">Addendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation type="unfinished">Dedendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation type="unfinished">Root fillet coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation type="unfinished">Profile shift coefficient</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>需要作業中的主體</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation type="unfinished">To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation type="unfinished">Create New Body</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="52"/>
      <source>Please select</source>
      <translation>請選擇</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>基本幾何</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>第一方向之角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>第二方向之角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>間距</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>寬度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>高度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>旋轉角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>半徑 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>半徑 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation type="unfinished">U parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation type="unfinished">V parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>局部 z-方向之半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation type="unfinished">Radius in local X-direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation type="unfinished">Radius 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation type="unfinished">Radius in local Y-direction
If zero, it is equal to Radius2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation type="unfinished">V parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation type="unfinished">Radius in local XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation type="unfinished">Radius in local XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>多邊形</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation type="unfinished">Circumradius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation type="unfinished">X min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation type="unfinished">Y min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation type="unfinished">Z min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation type="unfinished">X2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation type="unfinished">Z2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>螺距</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation type="unfinished">Coordinate system</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>增長</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation type="unfinished">Number of rotations</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation type="unfinished">Angle 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation type="unfinished">Angle 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation type="unfinished">From 3 Points</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation type="unfinished">Major radius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation type="unfinished">Minor radius</translation>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>右手系統</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>左手系統</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>起始點</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>最末點</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>參考</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>你所選之幾何物並非作業中主體之一部份，請定義如何處理這些選擇。如果你不想要這些參考的話請取消此指令。</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>製作獨立副本 (推薦)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>製作相依副本</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>建立交叉參照</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="285"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>挑選這個可能導致循環參照</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation type="unfinished">Add Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation type="unfinished">Remove Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>融合</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>切割</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>交集實體</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="51"/>
      <source>Boolean Parameters</source>
      <translation type="unfinished">Boolean Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="82"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="47"/>
      <source>Primitive Parameters</source>
      <translation type="unfinished">Primitive Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="940"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="948"/>
      <source>Invalid wedge parameters</source>
      <translation>無效的楔形體參數</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>X min must not be equal to X max!</source>
      <translation>X 最小值必須不等於 X 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="941"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y 最小值必須不等於 Y 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="949"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z 最小值必須不等於 Z 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="991"/>
      <source>Create primitive</source>
      <translation>建立幾何形體</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看倒角</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>同等距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>兩個距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>距離與角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation type="unfinished">Flips the direction</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation type="unfinished">Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>尺寸 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="344"/>
      <source>Empty chamfer created!
</source>
      <translation type="unfinished">Empty chamfer created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>Empty body list</source>
      <translation>清空主體清單</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>The body list cannot be empty</source>
      <translation>主體列表不能空白</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="399"/>
      <source>Boolean: Accept: Input error</source>
      <translation>布林值: 接受: 輸出錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>Incompatible Reference Set</source>
      <translation type="unfinished">Incompatible Reference Set</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>沒有適合當前參考集的附件模式。如果您選擇繼續，該特徵將保留在它現在的位置，並且不會隨著參考的變化而移動。是否繼續 ?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="228"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</source>
      <translation type="unfinished">The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="235"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="440"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看草稿</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>拔模角度</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation type="unfinished">Neutral Plane</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation type="unfinished">Pull Direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>反轉拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="288"/>
      <source>Empty draft created!
</source>
      <translation type="unfinished">Empty draft created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="298"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <source>Confirm Selection</source>
      <translation type="unfinished">Confirm Selection</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="316"/>
      <source>Add All Edges</source>
      <translation type="unfinished">Add All Edges</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="322"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation type="unfinished">Adds all edges to the list box (only when in add selection mode)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="331"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1372"/>
      <source>No face selected</source>
      <translation>無選定之面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="171"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1141"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="352"/>
      <source>Preview</source>
      <translation>預覽</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="356"/>
      <source>Select Faces</source>
      <translation type="unfinished">Select Faces</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="692"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="602"/>
      <source>No shape selected</source>
      <translation>無選取物件</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="685"/>
      <source>Sketch normal</source>
      <translation>草圖法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="688"/>
      <source>Face normal</source>
      <translation>面法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="696"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>自訂方向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1088"/>
      <source>Click on a shape in the model</source>
      <translation>點擊模型中的形狀</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1359"/>
      <source>One sided</source>
      <translation type="unfinished">One sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>Two sided</source>
      <translation type="unfinished">Two sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>Symmetric</source>
      <translation>對稱</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1367"/>
      <source>Click on a face in the model</source>
      <translation>點擊模型中的一個面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>允許已使用過的特徵</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation type="unfinished">Allow External Features</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>從同一零件中的另一實體</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>來自不同的零件或自由的特徵</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>製作獨立副本 (推薦)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>製作相依副本</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>建立交叉參照</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Invalid shape</source>
      <translation>非有效形狀</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>No wire in sketch</source>
      <translation>草圖找不到線段</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Sketch already used by other feature</source>
      <translation>草圖已被其他特徵使用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Belongs to another body</source>
      <translation>屬於另一個主體</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another part</source>
      <translation>屬於另一個零件</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Doesn't belong to any body</source>
      <translation>不屬於任何主體</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Base plane</source>
      <translation>基準面</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Feature is located after the tip of the body</source>
      <translation type="unfinished">Feature is located after the tip of the body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Select attachment</source>
      <translation>選擇附件</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看其圓角</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>半徑</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation type="unfinished">Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="203"/>
      <source>Empty fillet created!</source>
      <translation>已建立空倒圓角!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="239"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="240"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="222"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
      <source>Normal sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>狀態</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="206"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>俯仰高度角</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>俯仰轉角</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>高度轉角</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>高度-轉圈數-增長</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>螺距</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>高度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation type="unfinished">Turns</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation type="unfinished">Cone angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation type="unfinished">Radial growth</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>左撇子</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>反轉</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>刪除輪廓外部</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="55"/>
      <source>Helix Parameters</source>
      <translation type="unfinished">Helix Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="293"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告：螺旋可能自相交</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="298"/>
      <source>Error: helix touches itself</source>
      <translation>錯誤：螺旋碰觸到自己</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="347"/>
      <source>Error: unsupported mode</source>
      <translation>錯誤：不支援的模式</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterbore</source>
      <translation>沉頭孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="56"/>
      <source>Countersink</source>
      <translation>埋頭孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterdrill</source>
      <translation>連柱坑鑽頭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Hole Parameters</source>
      <translation type="unfinished">Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>None</source>
      <translation>無</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric regular</source>
      <translation>ISO公制標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>ISO metric fine</source>
      <translation>ISO公制細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS coarse</source>
      <translation type="unfinished">UTS coarse</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS fine</source>
      <translation>UTS細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS extra fine</source>
      <translation>UTS超細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ANSI pipes</source>
      <translation>ANSI 管件</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP 管件</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSW whitworth</source>
      <translation>BSW惠氏螺紋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>BSF whitworth fine</source>
      <translation>BSF惠氏細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>ISO tyre valves</source>
      <translation type="unfinished">ISO tyre valves</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>中等</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="682"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>細緻</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="686"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>粗糙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="692"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>垂直</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="696"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="700"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>寬鬆</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="704"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>垂直</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="705"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="706"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>寬</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>直紋曲面</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>輪廓</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>物件</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>加入輪廓圖</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>刪除輪廓圖</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>清單可以通過拖曳來重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="48"/>
      <source>Loft Parameters</source>
      <translation type="unfinished">Loft Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="184"/>
      <source>Error</source>
      <translation>錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>排列形式</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>確定</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="69"/>
      <source>Edit</source>
      <translation>編輯</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="72"/>
      <source>Delete</source>
      <translation>刪除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="75"/>
      <source>Add Mirror Transformation</source>
      <translation type="unfinished">Add Mirror Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add Linear Pattern</source>
      <translation type="unfinished">Add Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="91"/>
      <source>Add Polar Pattern</source>
      <translation type="unfinished">Add Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="99"/>
      <source>Add Scale Transformation</source>
      <translation type="unfinished">Add Scale Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Move Up</source>
      <translation type="unfinished">Move Up</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="105"/>
      <source>Move Down</source>
      <translation type="unfinished">Move Down</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="137"/>
      <source>Right-click to add a transformation</source>
      <translation type="unfinished">Right-click to add a transformation</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="38"/>
      <source>Pad Parameters</source>
      <translation type="unfinished">Pad Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation type="unfinished">Offset the pad from the face at which the pad will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="41"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation type="unfinished">Offset the pad from the face at which the pad will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Reverses pad direction</source>
      <translation>相反填充方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To last</source>
      <translation>到最後位置</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to shape</source>
      <translation>上升到形狀</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>間距</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>偏移至面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>選擇所有面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation type="unfinished">Select Face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation type="unfinished">Side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="541"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>設定方向或是由模型選擇一個邊作為參考</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>草圖法線</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>自訂方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>使用自定向量作為填充方向，否則使用草圖平面的法線來長厚度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>若未選擇的話，其長度將沿著指定方向測量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>沿著草圖法線的長度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>反轉</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation type="unfinished">Direction/edge</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation type="unfinished">X-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation type="unfinished">Y-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation type="unfinished">Z-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>擠出錐度的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation type="unfinished">Side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>斜角</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation type="unfinished">Select Shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation type="unfinished">Selects all faces of the shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>定向模式</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>弗勒內公式(Frenet)</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>輔助</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>副法向量</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>曲線等價</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>輪廓</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>物件</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>新增邊界</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>移除邊界</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>設定用於計算輪廓方向的恆定副法向量</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="575"/>
      <source>Section Orientation</source>
      <translation type="unfinished">Section Orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="603"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>輪廓</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>物件</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation type="unfinished">Corner transition</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation type="unfinished">Right corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation type="unfinished">Round corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation type="unfinished">Path to Sweep Along</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>加上邊</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation type="unfinished">Remove edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>變換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="67"/>
      <source>Pipe Parameters</source>
      <translation type="unfinished">Pipe Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="86"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <location filename="../../TaskPipeParameters.cpp" line="561"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <source>No active body</source>
      <translation>沒有作業中的主體</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>變換模式</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>常數</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>多重輪廓圖</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>加入輪廓圖</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>刪除輪廓圖</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>清單可以通過拖曳來重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="870"/>
      <source>Section Transformation</source>
      <translation type="unfinished">Section Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="889"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="38"/>
      <source>Pocket Parameters</source>
      <translation type="unfinished">Pocket Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="41"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Reverses pocket direction</source>
      <translation>相反凹陷方向</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>Up to shape</source>
      <translation>上升到形狀</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="254"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="255"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="256"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>依平面對稱</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>反轉</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>第 2 角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="264"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="197"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="160"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="491"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="199"/>
      <source>To last</source>
      <translation>到最後位置</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="204"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="215"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="216"/>
      <source>Two angles</source>
      <translation type="unfinished">Two angles</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="479"/>
      <source>No face selected</source>
      <translation>無選定之面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>因數</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>產生次數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>物件</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>新增幾何外型</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>刪除幾何外型</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Shape Binder Parameters</source>
      <translation type="unfinished">Shape Binder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="137"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="193"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看其特徵</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>厚度</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>單面</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>管件</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation type="unfinished">Recto verso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>連接型式</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>圓弧</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>直角</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>壁厚朝內</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="267"/>
      <source>Empty thickness created!
</source>
      <translation type="unfinished">Empty thickness created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="442"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base XY-plane</source>
      <translation type="unfinished">Base XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base YZ-plane</source>
      <translation type="unfinished">Base YZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base XZ-plane</source>
      <translation type="unfinished">Base XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="466"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>變換主體</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>變換工具形狀</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation type="unfinished">Add Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation type="unfinished">Remove Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>清單可以通過拖曳來重新排序</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="908"/>
      <source>Select Body</source>
      <translation type="unfinished">Select Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="909"/>
      <source>Select a body from the list</source>
      <translation>從清單中選擇主體</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1095"/>
      <source>Move Feature After…</source>
      <translation type="unfinished">Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1096"/>
      <source>Select a feature from the list</source>
      <translation>從清單中選擇特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1183"/>
      <source>Move Tip</source>
      <translation type="unfinished">Move Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>Set tip to last feature?</source>
      <translation type="unfinished">Set tip to last feature?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1184"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>移動特徵出現在當前設置的尖點之後。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>Invalid selection</source>
      <translation>無效選擇</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="147"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>沒有適合選定物件的附件模式。選擇其它東西。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="160"/>
      <location filename="../../Command.cpp" line="168"/>
      <location filename="../../Command.cpp" line="175"/>
      <source>Error</source>
      <translation>錯誤</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="809"/>
      <source>Several sub-elements selected</source>
      <translation>多個次元素被選取</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="810"/>
      <source>Select a single face as support for a sketch!</source>
      <translation type="unfinished">Select a single face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="817"/>
      <source>Select a face as support for a sketch!</source>
      <translation type="unfinished">Select a face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="824"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation type="unfinished">Need a planar face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="831"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation type="unfinished">Create a plane first or select a face to sketch on</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="816"/>
      <source>No support face selected</source>
      <translation>未選取基礎面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="823"/>
      <source>No planar support</source>
      <translation>無平面之基礎面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="830"/>
      <source>No valid planes in this document</source>
      <translation>在本文件的非法平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="257"/>
      <location filename="../../ViewProvider.cpp" line="135"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <location filename="../../Command.cpp" line="1138"/>
      <location filename="../../SketchWorkflow.cpp" line="728"/>
      <source>A dialog is already open in the task panel</source>
      <translation>於工作面板已開啟對話窗</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>無法使用此命令，因為沒有可減去之實體。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="995"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>在嘗試除料命令之前，請確保主體包含特徵。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1019"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>無法使用選定的物件。選定物件必須屬於作業中主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation type="unfinished">There is no active body. Please activate a body before inserting a datum entity.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="467"/>
      <source>Sub-shape binder</source>
      <translation type="unfinished">Sub-shape binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>No sketch to work on</source>
      <translation>沒有可工作之草圖</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1052"/>
      <source>No sketch is available in the document</source>
      <translation>文檔中沒有可用之草圖</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="258"/>
      <location filename="../../ViewProvider.cpp" line="136"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <location filename="../../Command.cpp" line="1139"/>
      <location filename="../../SketchWorkflow.cpp" line="729"/>
      <source>Close this dialog?</source>
      <translation type="unfinished">Close this dialog?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <location filename="../../Command.cpp" line="1856"/>
      <source>Wrong selection</source>
      <translation>錯誤的選擇</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>自單一主體中選擇一個邊、面或是主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1829"/>
      <location filename="../../Command.cpp" line="2191"/>
      <source>Selection is not in the active body</source>
      <translation type="unfinished">Selection is not in the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1857"/>
      <source>Shape of the selected part is empty</source>
      <translation type="unfinished">Shape of the selected part is empty</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1830"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>自一個作業中主體選擇一個邊、面或是主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1022"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation type="unfinished">Consider using a shape binder or a base feature to reference external geometry in a body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1843"/>
      <source>Wrong object type</source>
      <translation>物件種類錯誤</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1844"/>
      <source>%1 works only on parts.</source>
      <translation>%1 僅能用於零件上</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2192"/>
      <source>Please select only one feature in an active body.</source>
      <translation>請在一個作業中主體只選擇一個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="71"/>
      <source>Part creation failed</source>
      <translation>新增零件失敗</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="72"/>
      <source>Failed to create a part object.</source>
      <translation>建立一個零件物件失敗</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="149"/>
      <location filename="../../CommandBody.cpp" line="215"/>
      <source>Bad base feature</source>
      <translation>壞掉的基礎特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="126"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation type="unfinished">A body cannot be based on a Part Design feature.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation type="unfinished">%1 already belongs to a body and cannot be used as a base feature for another body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="150"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>基礎特徵 (%1) 屬於其他部分。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="177"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個實體組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>選定的形狀只由一個殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="195"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個實體或殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="204"/>
      <source>Base feature</source>
      <translation>基礎特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="216"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>主體可能基於不超過一個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="230"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="421"/>
      <source>Nothing to migrate</source>
      <translation>沒有什麼東西可以遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="692"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation type="unfinished">Select exactly one Part Design feature or a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="700"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation type="unfinished">Could not determine a body for the selected feature '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Only features of a single source body can be moved</source>
      <translation type="unfinished">Only features of a single source body can be moved</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>草圖平面無法被遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="422"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation type="unfinished">No Part Design features without body found Nothing to migrate.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="617"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>請編輯 '%1' 並重新定義它以使用 Base 或 Datum 平面作為草圖平面。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <location filename="../../CommandBody.cpp" line="699"/>
      <location filename="../../CommandBody.cpp" line="711"/>
      <location filename="../../CommandBody.cpp" line="1061"/>
      <location filename="../../CommandBody.cpp" line="1071"/>
      <source>Selection error</source>
      <translation>選取錯誤</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="712"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>只有實心特徵能成為主體的尖點。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="846"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <location filename="../../CommandBody.cpp" line="894"/>
      <source>Features cannot be moved</source>
      <translation>特徵無法被移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>某些被選的特徵與來源實體有相依性</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>There are no other bodies to move to</source>
      <translation>沒有其它主體可以搬移過去</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1062"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>不可能移動主體的基礎特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1072"/>
      <source>Select one or more features from the same body.</source>
      <translation>選擇相同主體的一個或更多個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1087"/>
      <source>Beginning of the body</source>
      <translation>主體的起點</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1168"/>
      <source>Dependency violation</source>
      <translation>相依性衝突</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1169"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>較早的特徵不可以相依在較晚的特徵。</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="307"/>
      <source>No previous feature found</source>
      <translation>找不到先前特徵</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="308"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>如果沒有可用的基礎特徵，則無法建立除料特徵</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="243"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="85"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>Active Body Required</source>
      <translation>需要作業中主體</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="148"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation type="unfinished">To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="207"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation type="unfinished">To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="273"/>
      <source>Feature is not in a body</source>
      <translation>特徵不在主體中</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="274"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>為了使用此特徵，它需要屬於文件中的主體物件。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="320"/>
      <source>Feature is not in a part</source>
      <translation>特徵不在零件中</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="321"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>為了使用此特徵，它需要屬於文件中的一個零件物件。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="62"/>
      <location filename="../../ViewProviderTransformed.cpp" line="63"/>
      <location filename="../../ViewProvider.cpp" line="92"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="225"/>
      <source>Edit %1</source>
      <translation>編輯 %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="105"/>
      <source>Set Face Colors</source>
      <translation type="unfinished">Set Face Colors</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="112"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="117"/>
      <location filename="../../ViewProviderDatum.cpp" line="207"/>
      <source>Line</source>
      <translation>線</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="122"/>
      <location filename="../../ViewProviderDatum.cpp" line="217"/>
      <source>Point</source>
      <translation>點</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="127"/>
      <source>Coordinate System</source>
      <translation>座標系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="234"/>
      <source>Edit Datum</source>
      <translation type="unfinished">Edit Datum</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="91"/>
      <source>Feature error</source>
      <translation>特徵錯誤</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="92"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation type="unfinished">%1 misses a base feature.
This feature is broken and cannot be edited.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="220"/>
      <source>Edit Shape Binder</source>
      <translation type="unfinished">Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="350"/>
      <source>Synchronize</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Select Bound Object</source>
      <translation type="unfinished">Select Bound Object</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="154"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation type="unfinished">The document "%1" you are editing was designed with an old version of Part Design workbench.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation type="unfinished">Migrate in order to use modern Part Design features?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="166"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation type="unfinished">The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="173"/>
      <source>Make the migration automatically?</source>
      <translation type="unfinished">Make the migration automatically?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="176"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>注意：如果您選擇遷移，您將無法使用舊版 FreeCAD 編輯文件。
如果您拒絕遷移，您將無法使用新的 PartDesign 功能，例如實體和零件。因此，您也將無法在 assembly 工作台中使用您的零件。
儘管您稍後可以使用“零件設計 -&gt; 遷移”進行遷移。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="189"/>
      <source>Migrate Manually</source>
      <translation type="unfinished">Migrate Manually</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="67"/>
      <source>Edit Boolean</source>
      <translation type="unfinished">Edit Boolean</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="40"/>
      <source>Edit Chamfer</source>
      <translation type="unfinished">Edit Chamfer</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="41"/>
      <source>Edit Draft</source>
      <translation type="unfinished">Edit Draft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="40"/>
      <source>Edit Fillet</source>
      <translation type="unfinished">Edit Fillet</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="43"/>
      <source>Edit Groove</source>
      <translation type="unfinished">Edit Groove</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="48"/>
      <source>Edit Helix</source>
      <translation type="unfinished">Edit Helix</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit Hole</source>
      <translation type="unfinished">Edit Hole</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="38"/>
      <source>Edit Linear Pattern</source>
      <translation type="unfinished">Edit Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="65"/>
      <source>Edit Loft</source>
      <translation type="unfinished">Edit Loft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="38"/>
      <source>Edit Mirror</source>
      <translation>編輯鏡像</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="47"/>
      <source>Edit Multi-Transform</source>
      <translation type="unfinished">Edit Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="43"/>
      <source>Edit Pad</source>
      <translation type="unfinished">Edit Pad</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="75"/>
      <source>Edit Pipe</source>
      <translation type="unfinished">Edit Pipe</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="45"/>
      <source>Edit Pocket</source>
      <translation type="unfinished">Edit Pocket</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation type="unfinished">Edit Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="49"/>
      <source>Edit Primitive</source>
      <translation type="unfinished">Edit Primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="43"/>
      <source>Edit Revolution</source>
      <translation type="unfinished">Edit Revolution</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="38"/>
      <source>Edit Scale</source>
      <translation type="unfinished">Edit Scale</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="40"/>
      <source>Edit Thickness</source>
      <translation type="unfinished">Edit Thickness</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation type="unfinished">Sprocket Parameters</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation type="unfinished">Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation type="unfinished">Sprocket reference</translation>
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
      <translation type="unfinished">Bicycle with derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation type="unfinished">Bicycle without derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation type="unfinished">Chain pitch</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation type="unfinished">Chain roller diameter</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation type="unfinished">Tooth width</translation>
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
      <translation>0 吋</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>螺紋修改的即時更新
請注意計算會需要一些時間</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>螺紋深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>自訂螺紋間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation type="unfinished">Head type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation type="unfinished">Depth type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation type="unfinished">Head diameter</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation type="unfinished">Head depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation type="unfinished">Clearance / Passthrough</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation type="unfinished">Tap drill (to be threaded)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation type="unfinished">Modeled thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation type="unfinished">Hole type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>更新螺紋視圖</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>自訂間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Thread clearance value</source>
      <translation>自訂螺紋間隙值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>孔洞間隙
僅適用於無螺紋孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Standard</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="732"/>
      <source>Close</source>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="737"/>
      <source>Wide</source>
      <translation>寬</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Class</source>
      <translation>類別 (Class)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="835"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>根據孔輪廓的螺紋孔公差等級</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>直徑</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>孔洞直徑</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation type="unfinished">Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>基礎輪廓類型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>圓跟弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>點，圓跟弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>點</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>自訂頭部值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>鑽孔角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>包含深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>切換方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="662"/>
      <source>&lt;b&gt;Threading&lt;/b&gt;</source>
      <translation type="unfinished">&lt;b&gt;Threading&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="783"/>
      <source>Thread</source>
      <translation>Thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="892"/>
      <source>&amp;Right hand</source>
      <translation>右手 (&amp;R)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="908"/>
      <source>&amp;Left hand</source>
      <translation>左手 (&amp;L)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="957"/>
      <source>Thread Depth Type</source>
      <translation>螺紋深度類型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="971"/>
      <source>Hole depth</source>
      <translation>孔深</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="981"/>
      <source>Tapped (DIN76)</source>
      <translation>攻牙(DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>螺絲頭的切割類型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>選取覆蓋「類型」預定義的值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>對於埋頭孔來說，這是螺絲頭部分位於表面以下的深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>埋頭角</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>鑽尖尺寸將不會用來考慮作盲孔深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>錐</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>挖孔斜角
90度：直孔
低於 90：底部的孔半徑較小
超過 90：底部孔半徑較大</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>反轉孔之方向</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>無訊息</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="41"/>
      <source>&amp;Sketch</source>
      <translation>&amp;草圖</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Part Design</source>
      <translation>&amp;零件設計</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>Datums</source>
      <translation>基準面</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Additive Features</source>
      <translation type="unfinished">Additive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Subtractive Features</source>
      <translation type="unfinished">Subtractive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Dress-Up Features</source>
      <translation type="unfinished">Dress-Up Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Transformation Features</source>
      <translation type="unfinished">Transformation Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Sprocket…</source>
      <translation type="unfinished">Sprocket…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Measure</source>
      <translation>測量</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Refresh</source>
      <translation>重新運算</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Toggle 3D</source>
      <translation>切換 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Part Design Helper</source>
      <translation>零件設計幫手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Modeling</source>
      <translation>零件設計建模</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Length [mm]</source>
      <translation>長度[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Diameter [mm]</source>
      <translation>外徑[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Inner diameter [mm]</source>
      <translation>內徑[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Constraint type</source>
      <translation>拘束類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge type</source>
      <translation>開始邊類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Start edge size</source>
      <translation>開始邊尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>End edge type</source>
      <translation>結束邊類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>結束邊尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="67"/>
      <source>Shaft Wizard</source>
      <translation type="unfinished">Shaft Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="75"/>
      <source>Section 1</source>
      <translation>輪廊圖 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Section 2</source>
      <translation>輪廊圖 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="80"/>
      <source>Add column</source>
      <translation>新增欄位</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="128"/>
      <source>Section %s</source>
      <translation>輪廊圖 %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="157"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="176"/>
      <source>None</source>
      <translation>無</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="158"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <source>Force</source>
      <translation>力</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Bearing</source>
      <translation>軸承</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Gear</source>
      <translation>齒輪</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Pulley</source>
      <translation>滑輪</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="179"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="180"/>
      <source>Fillet</source>
      <translation>圓角</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="58"/>
      <source>All</source>
      <translation>所有</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="118"/>
      <source>Missing Module</source>
      <translation type="unfinished">Missing Module</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="124"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation type="unfinished">The Plot add-on is not installed. Install it to enable this feature.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="251"/>
      <source>Shaft design wizard...</source>
      <translation>軸設計精靈...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="254"/>
      <source>Start the shaft design wizard</source>
      <translation>啟動軸設計精靈</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>被鏈接物件不是零件設計特徵</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="412"/>
      <source>Tip shape is empty</source>
      <translation>尖端形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="66"/>
      <source>BaseFeature link is not set</source>
      <translation>BaseFeature 連結未設置</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="72"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>BaseFeature 必須是一個 Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="82"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature 有一個空形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="75"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>無法在沒有 BaseFeature 的情況下進行布林切割</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>無法對除 Part::Feature 及其衍生物之外的任何物件進行布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="104"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>無法對無效的基礎形狀進行布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="192"/>
      <location filename="../../../App/FeatureDraft.cpp" line="332"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="159"/>
      <location filename="../../../App/FeatureFillet.cpp" line="140"/>
      <location filename="../../../App/FeatureHole.cpp" line="2067"/>
      <location filename="../../../App/FeatureLoft.cpp" line="331"/>
      <location filename="../../../App/FeatureLoft.cpp" line="375"/>
      <location filename="../../../App/FeaturePipe.cpp" line="480"/>
      <location filename="../../../App/FeaturePipe.cpp" line="529"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="284"/>
      <location filename="../../../App/FeatureGroove.cpp" line="253"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="773"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="789"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="802"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation type="unfinished">Result has multiple solids: enable 'Allow Compound' in the active body.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="114"/>
      <source>Tool shape is null</source>
      <translation>工具形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="141"/>
      <source>Unsupported boolean operation</source>
      <translation>不支援的布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="351"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pad with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="356"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pocket with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="704"/>
      <source>No extrusion geometry was generated.</source>
      <translation type="unfinished">No extrusion geometry was generated.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="728"/>
      <source>Resulting fused extrusion is null.</source>
      <translation type="unfinished">Resulting fused extrusion is null.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="368"/>
      <location filename="../../../App/FeaturePipe.cpp" line="521"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="139"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="764"/>
      <source>Resulting shape is not a solid</source>
      <translation>產成形狀不是固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="172"/>
      <source>Failed to create chamfer</source>
      <translation>建立倒角失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="327"/>
      <location filename="../../../App/FeatureFillet.cpp" line="120"/>
      <source>Resulting shape is null</source>
      <translation>產成形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="141"/>
      <source>No edges specified</source>
      <translation>未指定邊緣</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="293"/>
      <source>Size must be greater than zero</source>
      <translation>尺寸必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="304"/>
      <source>Size2 must be greater than zero</source>
      <translation>尺寸2必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="311"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>角度必須大於 0 而且小於 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="95"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>在被選的形狀上無法進行圓角處理</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="103"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>倒圓角半徑必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="157"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation type="unfinished">Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>Angle of groove too large</source>
      <translation>挖槽的角度太大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="108"/>
      <source>Angle of groove too small</source>
      <translation>挖槽的角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>要求之特徵無法建立，可能的原因有：
 - 作業中主體不包含一個基礎形狀，所以沒有可移除之材料；
 - 所選擇的草圖不屬於作業中主體。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="400"/>
      <source>Failed to obtain profile shape</source>
      <translation>無法獲取輪廓形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="454"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>建立失敗，因為方向與草圖的法向量正交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="176"/>
      <location filename="../../../App/FeatureGroove.cpp" line="154"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="477"/>
      <source>Creating a face from sketch failed</source>
      <translation>由草圖建立面失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="115"/>
      <source>Angles of groove nullify each other</source>
      <translation type="unfinished">Angles of groove nullify each other</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="193"/>
      <location filename="../../../App/FeatureGroove.cpp" line="171"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>旋轉軸與草圖相交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="294"/>
      <location filename="../../../App/FeatureGroove.cpp" line="263"/>
      <source>Could not revolve the sketch!</source>
      <translation>無法旋轉草圖！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="306"/>
      <location filename="../../../App/FeatureGroove.cpp" line="275"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>無法從草圖建立面。
草圖中不允許有相交的草圖實體。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="235"/>
      <source>Error: Pitch too small!</source>
      <translation>錯誤：螺距太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="240"/>
      <location filename="../../../App/FeatureHelix.cpp" line="263"/>
      <source>Error: height too small!</source>
      <translation>錯誤：高度太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="249"/>
      <source>Error: pitch too small!</source>
      <translation>錯誤：螺距太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="254"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="277"/>
      <source>Error: turns too small!</source>
      <translation>錯誤：圈數太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="283"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>錯誤：高度或增長必須不為零！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: unsupported mode</source>
      <translation>錯誤：不支援的模式</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="315"/>
      <source>Error: No valid sketch or face</source>
      <translation>錯誤：沒有有效的草圖或面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="328"/>
      <source>Error: Face must be planar</source>
      <translation>錯誤：面必須為平面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2422"/>
      <location filename="../../../App/FeatureHelix.cpp" line="443"/>
      <location filename="../../../App/FeatureHelix.cpp" line="484"/>
      <source>Error: Result is not a solid</source>
      <translation>錯誤：產生形狀不是固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="413"/>
      <source>Error: There is nothing to subtract</source>
      <translation>錯誤：沒有東西可以減</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="449"/>
      <location filename="../../../App/FeatureHelix.cpp" line="490"/>
      <source>Error: Result has multiple solids</source>
      <translation>錯誤：產生形狀有多重(非相連)固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="434"/>
      <source>Error: Adding the helix failed</source>
      <translation>錯誤：添加螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>錯誤：交叉螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="475"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>錯誤：減去螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="506"/>
      <source>Error: Could not create face from sketch</source>
      <translation>錯誤：無法從草圖建立面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1224"/>
      <source>Thread type is invalid</source>
      <translation>螺紋類型無效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1764"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>挖孔錯誤：不支援的長度規格</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1770"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>挖孔錯誤：無效的孔深</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1796"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>挖孔錯誤：無效斜角</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1820"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>挖孔錯誤：挖孔直徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1825"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>挖孔錯誤：孔的切割深度必須小於孔的深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1832"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>挖孔錯誤：孔的切割深度必須大於或等於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1862"/>
      <source>Hole error: Invalid countersink</source>
      <translation>挖孔錯誤：無效的埋頭孔</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1898"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>挖孔錯誤：無效的鑽尖角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1915"/>
      <source>Hole error: Invalid drill point</source>
      <translation>挖孔錯誤：無效的鑽尖</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1952"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>挖孔錯誤：無法旋轉草圖</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1959"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>挖孔錯誤：結果形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1972"/>
      <source>Error: Adding the thread failed</source>
      <translation>錯誤：添加螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Finding axis failed</source>
      <translation type="unfinished">Hole error: Finding axis failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2039"/>
      <location filename="../../../App/FeatureHole.cpp" line="2047"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>布林運算在輪廓邊緣失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>布林運算在輪廓邊緣產生了非實體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="151"/>
      <source>Boolean operation failed</source>
      <translation>布林運算失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2080"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>無法從草圖建立面。草圖實體的交叉或草圖中存在多個面不允許製作一個沖孔至一個面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2245"/>
      <source>Thread type out of range</source>
      <translation>螺紋類型超出範圍</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2248"/>
      <source>Thread size out of range</source>
      <translation>螺紋尺寸超出範圍</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2396"/>
      <source>Error: Thread could not be built</source>
      <translation>錯誤：無法建立螺紋</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="191"/>
      <source>Loft: At least one section is needed</source>
      <translation>拉伸成形：至少需要一個輪廊</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="392"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>拉伸成形：在建立拉伸成形時發生致命錯誤</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="238"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Loft 操作：從草圖建立面失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePipe.cpp" line="444"/>
      <source>Loft: Failed to create shell</source>
      <translation>Loft 操作：建立外殼失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="817"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>無法從草圖立建面。
草圖中不允許有相交的實體或多個面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="203"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>管件：無法獲得輪廊形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="210"/>
      <source>No spine linked</source>
      <translation>沒有連結到 spline 曲線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="225"/>
      <source>No auxiliary spine linked.</source>
      <translation>沒有連結到輔助 spline 曲線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="248"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>管件：在使用具有孤立點作為剖面的草圖時，只需要一個孤立點。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="257"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>管件：當使用一個點給輪廊時，至少需要一個剖面</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation type="unfinished">Pipe: All sections need to be Part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="283"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>管件：無法獲得剖面形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="293"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>管件：只有輪廊及最後一個斷面可以為頂點</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="306"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>多重剖面需要有與基本剖面相同數量的內部線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="339"/>
      <source>Path must not be a null shape</source>
      <translation>路徑不能為空形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="379"/>
      <source>Pipe could not be built</source>
      <translation>無法建立管件</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>Result is not a solid</source>
      <translation>產生形狀不是實心物體</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="475"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>管件：沒有東西可以減</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="543"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>在建立管件時發生致命錯誤</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="672"/>
      <source>Invalid element in spine.</source>
      <translation>Spline 曲線中有無效元件。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="677"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Spline 曲線中的元件既不是一個邊也不是一條線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Spine is not connected.</source>
      <translation>Spline 曲線未連接</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="704"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Spline 曲線不是一個邊也不是一條線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="709"/>
      <source>Invalid spine.</source>
      <translation>無效的 spline 曲線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="101"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>無法在沒有基礎特徵的情況下進行原始特徵的減法操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="353"/>
      <location filename="../../../App/FeaturePipe.cpp" line="505"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Unknown operation type</source>
      <translation>未知的運算類型</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="361"/>
      <location filename="../../../App/FeaturePipe.cpp" line="513"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="131"/>
      <source>Failed to perform boolean operation</source>
      <translation>執行布林運算失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="215"/>
      <source>Length of box too small</source>
      <translation>方塊長度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="220"/>
      <source>Width of box too small</source>
      <translation>方塊寬度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="225"/>
      <source>Height of box too small</source>
      <translation>方塊高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="273"/>
      <source>Radius of cylinder too small</source>
      <translation>圓柱的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="278"/>
      <source>Height of cylinder too small</source>
      <translation>圓柱的高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="283"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>圓柱的旋轉角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="340"/>
      <source>Radius of sphere too small</source>
      <translation>球體半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="392"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="397"/>
      <source>Radius of cone cannot be negative</source>
      <translation>錐體半徑不能為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="402"/>
      <source>Height of cone too small</source>
      <translation>錐體高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="482"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="487"/>
      <source>Radius of ellipsoid too small</source>
      <translation>橢球體的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="581"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="586"/>
      <source>Radius of torus too small</source>
      <translation>環形圓(甜甜圈狀)的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="671"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>角柱體的多邊形為無效的，必須具有3個或更多個邊</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="676"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>多邊形或角柱體的外接圓半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="681"/>
      <source>Height of prism is too small</source>
      <translation>角柱體的高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="768"/>
      <source>delta x of wedge too small</source>
      <translation>楔形體的 x 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="774"/>
      <source>delta y of wedge too small</source>
      <translation>楔形體的 y 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="780"/>
      <source>delta z of wedge too small</source>
      <translation>楔形體的 z 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="786"/>
      <source>delta z2 of wedge is negative</source>
      <translation>楔形體的 z2 差量為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="792"/>
      <source>delta x2 of wedge is negative</source>
      <translation>楔形體的 x2 差量為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="123"/>
      <source>Angle of revolution too large</source>
      <translation>旋轉角度太大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Angle of revolution too small</source>
      <translation>旋轉角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Angles of revolution nullify each other</source>
      <translation type="unfinished">Angles of revolution nullify each other</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="168"/>
      <location filename="../../../App/FeatureGroove.cpp" line="146"/>
      <source>Reference axis is invalid</source>
      <translation>參考軸是無效的</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="756"/>
      <source>Fusion with base feature failed</source>
      <translation>與基本特徵進行聯集失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="99"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>轉換特徵連結物件不是零件物件</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="106"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>沒有與變換特徵相關聯的原始特徵。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="346"/>
      <source>Cannot transform invalid support shape</source>
      <translation>無法轉換無效支援形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="397"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>添加/除料特徵的形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="388"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>只有添加與除料特徵可以被轉換</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="107"/>
      <source>Invalid face reference</source>
      <translation>無效的面參考</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="60"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="64"/>
      <source>Creates or edits the involute gear definition</source>
      <translation type="unfinished">Creates or edits the involute gear definition</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="63"/>
      <source>Sprocket</source>
      <translation type="unfinished">Sprocket</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="67"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation type="unfinished">Creates or edits the sprocket definition.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation type="unfinished">Show final result</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation type="unfinished">Show preview overlay</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="48"/>
      <source>Preview</source>
      <translation>預覽</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="223"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="226"/>
      <source>Starts the shaft design wizard</source>
      <translation type="unfinished">Starts the shaft design wizard</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation type="unfinished">Failure while computing removed volume preview: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="105"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation type="unfinished">Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2644"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2645"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2679"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2680"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation type="unfinished">Creates an additive box by its width, height, and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation type="unfinished">Creates an additive cylinder by its radius, height, and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates an additive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Creates an additive cone</source>
      <translation type="unfinished">Creates an additive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Creates an additive ellipsoid</source>
      <translation type="unfinished">Creates an additive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Creates an additive torus</source>
      <translation type="unfinished">Creates an additive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Creates an additive prism</source>
      <translation type="unfinished">Creates an additive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Creates an additive wedge</source>
      <translation type="unfinished">Creates an additive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation type="unfinished">Creates a subtractive box by its width, height and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation type="unfinished">Creates a subtractive cylinder by its radius, height and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates a subtractive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Creates a subtractive cone</source>
      <translation type="unfinished">Creates a subtractive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation type="unfinished">Creates a subtractive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Creates a subtractive torus</source>
      <translation type="unfinished">Creates a subtractive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Creates a subtractive prism</source>
      <translation type="unfinished">Creates a subtractive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Creates a subtractive wedge</source>
      <translation type="unfinished">Creates a subtractive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1007"/>
      <source>Attachment</source>
      <translation>附件</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="835"/>
      <source>Revolution Parameters</source>
      <translation type="unfinished">Revolution Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="845"/>
      <source>Groove Parameters</source>
      <translation type="unfinished">Groove Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation type="unfinished">Transformed Feature Messages</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="122"/>
      <source>Active Body</source>
      <translation type="unfinished">Active Body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="43"/>
      <source>Chamfer Parameters</source>
      <translation>倒角參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="113"/>
      <source>Datum Plane Parameters</source>
      <translation type="unfinished">Datum Plane Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="118"/>
      <source>Datum Line Parameters</source>
      <translation type="unfinished">Datum Line Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="123"/>
      <source>Datum Point Parameters</source>
      <translation type="unfinished">Datum Point Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="128"/>
      <source>Local Coordinate System Parameters</source>
      <translation type="unfinished">Local Coordinate System Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="44"/>
      <source>Draft Parameters</source>
      <translation type="unfinished">Draft Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="43"/>
      <source>Fillet Parameters</source>
      <translation type="unfinished">Fillet Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="40"/>
      <source>Linear Pattern Parameters</source>
      <translation type="unfinished">Linear Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="40"/>
      <source>Mirror Parameters</source>
      <translation type="unfinished">Mirror Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="40"/>
      <source>Multi-Transform Parameters</source>
      <translation type="unfinished">Multi-Transform Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="40"/>
      <source>Polar Pattern Parameters</source>
      <translation type="unfinished">Polar Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="40"/>
      <source>Scale Parameters</source>
      <translation type="unfinished">Scale Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="43"/>
      <source>Thickness Parameters</source>
      <translation type="unfinished">Thickness Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="130"/>
      <source>Direction 2</source>
      <translation type="unfinished">Direction 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="246"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation type="unfinished">Select a direction reference (edge, face, datum line)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="332"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation type="unfinished">Invalid selection. Select an edge, planar face, or datum line.</translation>
    </message>
  </context>
</TS>
