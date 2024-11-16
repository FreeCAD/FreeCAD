<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-TW" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="73"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>螺旋線起點的中心點；從參考軸導出。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="75"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>螺旋線的方向；從參考軸導出。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="77"/>
      <source>The reference axis of the helix.</source>
      <translation>螺旋線的參考軸。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="79"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>螺旋線輸入模式指定使用者來設定哪些屬性。
相關屬性之後將被計算。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="83"/>
      <source>The axial distance between two turns.</source>
      <translation>兩圈之間的軸向距離。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="85"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>螺旋路徑的高度，不考慮輪廓的範圍。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="87"/>
      <source>The number of turns in the helix.</source>
      <translation>螺旋線的圈數。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, nevatige shrink.</source>
      <translation>圍繞螺旋線形成外殼的圓錐的角度。
非零值將螺旋線變成錐形螺旋線。
正值使半徑增大，負值縮小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="95"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>每圈螺旋半徑的增長。
非零值將螺旋線變成錐形螺旋線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="98"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>將轉向方向設置為左手，
也就是說以逆時針方向沿著軸移動。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="101"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>確定螺旋線是否指向與軸相反的方向。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="103"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>如果設定，其結果將是輪廓和預先存在的主體的交集。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="105"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>如果為 false，該工具將根據輪廓邊界框建議間距的初始值，
從而避免自相交。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="98"/>
      <source>Number of gear teeth</source>
      <translation>輪齒數目</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="104"/>
      <source>Pressure angle of gear teeth</source>
      <translation>輪齒壓力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="101"/>
      <source>Module of the gear</source>
      <translation>齒輪模數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>設定 True=2 時，每條曲線有3個控制點；設定 False =1 時，每條曲線有4個控制點。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=外齒輪，False=內齒輪</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>齒輪的齒從節圓到齒尖的高度，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>齒輪的齒從節圓到齒根的高度，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>齒根處圓角的半徑，按模數進行標準化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>參考輪廓向外移動的距離，按模數進行標準化。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1508"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1509"/>
      <source>Additive helix</source>
      <translation>添加螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1510"/>
      <source>Sweep a selected sketch along a helix</source>
      <translation>沿著一個螺旋掃掠一個被選的草圖</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1413"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1414"/>
      <source>Additive loft</source>
      <translation>添加拉伸成形</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1415"/>
      <source>Loft a selected profile through other profile sections</source>
      <translation>以選定的輪廓圖拉伸成形並使其穿過另一個輪廓圖</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1317"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1318"/>
      <source>Additive pipe</source>
      <translation>添加管件</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1319"/>
      <source>Sweep a selected sketch along a path or to other profiles</source>
      <translation>沿著一條路徑或對其它輪廊掃掠已選草圖</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="88"/>
      <source>Create body</source>
      <translation>建立實體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="89"/>
      <source>Create a new body and make it active</source>
      <translation>新增一個實體並使激活它</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2321"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2322"/>
      <source>Boolean operation</source>
      <translation>布林運算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2323"/>
      <source>Boolean operation with two or more bodies</source>
      <translation>含兩個以上之物體的布林運算</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Create a local coordinate system</source>
      <translation>建立局部座標系統</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>Create a new local coordinate system</source>
      <translation>建立一個新的局部座標系統</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1791"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1792"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1793"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>所選造型邊進行倒角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="430"/>
      <source>Create a clone</source>
      <translation>建立一個克隆</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="431"/>
      <source>Create a new clone</source>
      <translation>建立一個新克隆</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1820"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <source>Draft</source>
      <translation>草稿</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Make a draft on a face</source>
      <translation>於面上建立草稿</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="608"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="609"/>
      <source>Duplicate selected object</source>
      <translation>複製選定物件</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="610"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>複製已選物件添加至作業中主體</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1763"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1764"/>
      <source>Fillet</source>
      <translation>圓角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1765"/>
      <source>Make a fillet on an edge, face or body</source>
      <translation>於邊、面或實體產生圓角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1253"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1254"/>
      <source>Groove</source>
      <translation>挖槽</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>Groove a selected sketch</source>
      <translation>於選定草圖上挖槽</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1153"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1154"/>
      <source>Hole</source>
      <translation>挖孔</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1155"/>
      <source>Create a hole with the selected sketch</source>
      <translation>以選定的草圖產生孔</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>Create a datum line</source>
      <translation>建立基準線</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Create a new datum line</source>
      <translation>建立新基準線</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2056"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2057"/>
      <source>LinearPattern</source>
      <translation>線性複製特徵</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2058"/>
      <source>Create a linear pattern feature</source>
      <translation>建立線性複製特徵</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="314"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="315"/>
      <source>Migrate</source>
      <translation>遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="316"/>
      <source>Migrate document to the modern PartDesign workflow</source>
      <translation>移動文件至PartDesign工作區</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2005"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2006"/>
      <source>Mirrored</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2007"/>
      <source>Create a mirrored feature</source>
      <translation>建立一個鏡像特徵</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="668"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="669"/>
      <source>Move object to other body</source>
      <translation>移動物件至其他主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="670"/>
      <source>Moves the selected object to another body</source>
      <translation>移動被選物件至另一主體</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="835"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="836"/>
      <source>Move object after other object</source>
      <translation>移動物件至其它物件後</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="837"/>
      <source>Moves the selected object and insert it after another object</source>
      <translation>移動選定的物件，並將其插入到另一個物件之後</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="529"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="530"/>
      <source>Set tip</source>
      <translation>設定尖點</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="531"/>
      <source>Move the tip of the body</source>
      <translation>移動物體尖點</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2206"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2207"/>
      <source>Create MultiTransform</source>
      <translation>建立多重轉換</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2208"/>
      <source>Create a multitransform feature</source>
      <translation>建立多重轉換特徵</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="503"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="504"/>
      <source>Create sketch</source>
      <translation>建立草圖</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="505"/>
      <source>Create a new sketch</source>
      <translation>建立一個新的sketch</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1095"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1096"/>
      <source>Pad</source>
      <translation>填充</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1097"/>
      <source>Pad a selected sketch</source>
      <translation>填充所選草圖</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>Create a datum plane</source>
      <translation>建立基準面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="165"/>
      <source>Create a new datum plane</source>
      <translation>建立新基準面</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1124"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1125"/>
      <source>Pocket</source>
      <translation>凹陷</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1126"/>
      <source>Create a pocket with the selected sketch</source>
      <translation>以選定草圖產生凹陷</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Create a datum point</source>
      <translation>建立基準點</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Create a new datum point</source>
      <translation>建立新基準點</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2109"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2110"/>
      <source>PolarPattern</source>
      <translation>環狀複製模式</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2111"/>
      <source>Create a polar pattern feature</source>
      <translation>建立一個環狀複製特徵</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1197"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1198"/>
      <source>Revolution</source>
      <translation>旋轉</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1199"/>
      <source>Revolve a selected sketch</source>
      <translation>旋轉選定之草圖</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2164"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2165"/>
      <source>Scaled</source>
      <translation>縮放</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2166"/>
      <source>Create a scaled feature</source>
      <translation>建立一個等比特徵</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Create a shape binder</source>
      <translation>建立路徑面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Create a new shape binder</source>
      <translation>建立新路徑面</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <location filename="../../Command.cpp" line="347"/>
      <source>Create a sub-object(s) shape binder</source>
      <translation>建立子物件形狀粘合劑</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1582"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1583"/>
      <source>Subtractive helix</source>
      <translation>螺旋狀除料</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1584"/>
      <source>Sweep a selected sketch along a helix and remove it from the body</source>
      <translation>沿著一個螺旋掃掠一個被選的草圖並將其自主體中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1461"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1462"/>
      <source>Subtractive loft</source>
      <translation>拉伸成形除料</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1463"/>
      <source>Loft a selected profile through other profile sections and remove it from the body</source>
      <translation>以多個輪廓圖產生拉伸外型，並在主體中挖空此形狀</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1365"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1366"/>
      <source>Subtractive pipe</source>
      <translation>管狀除料</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1367"/>
      <source>Sweep a selected sketch along a path or to other profiles and remove it from the body</source>
      <translation>由輪廓圖沿路徑掃出一外型，並在主體中挖空此形狀</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1888"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1889"/>
      <source>Thickness</source>
      <translation>厚度</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1890"/>
      <source>Make a thick solid</source>
      <translation>建立薄殼件</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Create an additive primitive</source>
      <translation>建立一個附加的基本物件</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="197"/>
      <source>Additive Box</source>
      <translation>添加立方體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="201"/>
      <source>Additive Cylinder</source>
      <translation>添加圓柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="205"/>
      <source>Additive Sphere</source>
      <translation>添加球體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="209"/>
      <source>Additive Cone</source>
      <translation>添加圓錐體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Ellipsoid</source>
      <translation>添加橢圓體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Additive Torus</source>
      <translation>添加中空環型體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="221"/>
      <source>Additive Prism</source>
      <translation>添加角柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="225"/>
      <source>Additive Wedge</source>
      <translation>添加楔形體</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="241"/>
      <source>PartDesign</source>
      <translation>零件設計</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>Create a subtractive primitive</source>
      <translation>建立一個除料幾何形體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="349"/>
      <source>Subtractive Box</source>
      <translation>立方體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="353"/>
      <source>Subtractive Cylinder</source>
      <translation>圓柱體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="357"/>
      <source>Subtractive Sphere</source>
      <translation>球體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="361"/>
      <source>Subtractive Cone</source>
      <translation>圓錐體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="365"/>
      <source>Subtractive Ellipsoid</source>
      <translation>橢圓體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="369"/>
      <source>Subtractive Torus</source>
      <translation>中空環型體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="373"/>
      <source>Subtractive Prism</source>
      <translation>角柱體除料</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="377"/>
      <source>Subtractive Wedge</source>
      <translation>楔形體除料</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="300"/>
      <source>Edit ShapeBinder</source>
      <translation>編輯形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="309"/>
      <source>Create ShapeBinder</source>
      <translation>建立形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="392"/>
      <source>Create SubShapeBinder</source>
      <translation>編輯子形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="447"/>
      <source>Create Clone</source>
      <translation>建立克隆</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="298"/>
      <location filename="../../Command.cpp" line="995"/>
      <source>Make copy</source>
      <translation>製作拷貝</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="248"/>
      <source>Create a Sketch on Face</source>
      <translation>在平面上建立草圖</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="488"/>
      <location filename="../../SketchWorkflow.cpp" line="623"/>
      <source>Create a new Sketch</source>
      <translation>建立新草圖</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2251"/>
      <source>Convert to MultiTransform feature</source>
      <translation>轉換至多重轉換特徵</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2339"/>
      <source>Create Boolean</source>
      <translation>建立布林運算</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <location filename="../../CommandBody.cpp" line="188"/>
      <source>Add a Body</source>
      <translation>添加一個主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="432"/>
      <source>Migrate legacy Part Design features to Bodies</source>
      <translation>將舊版零件設計功能遷移到主體中</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="577"/>
      <source>Move tip to selected feature</source>
      <translation>移動尖點至被選的特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="622"/>
      <source>Duplicate a PartDesign object</source>
      <translation>複製一個 PartDesign 物件</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="749"/>
      <source>Move an object</source>
      <translation>移動物件</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="900"/>
      <source>Move an object inside tree</source>
      <translation>將一個物件移至樹中</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="267"/>
      <source>Mirrored</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="304"/>
      <source>Make LinearPattern</source>
      <translation>建立線形樣式</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="348"/>
      <source>PolarPattern</source>
      <translation>環狀複製模式</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="385"/>
      <source>Scaled</source>
      <translation>縮放</translation>
    </message>
  </context>
  <context>
    <name>FeaturePickDialog</name>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="42"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="43"/>
      <source>Invalid shape</source>
      <translation>非有效形狀</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>草圖找不到線段</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>草圖已被其他特徵使用</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another Body feature</source>
      <translation>草圖屬於其他物件特徵</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>基準面</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the Tip feature</source>
      <translation>特徵位於尖點特徵之後</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Face tools</source>
      <translation>面編輯工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Edge tools</source>
      <translation>邊緣工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Boolean tools</source>
      <translation>布林工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Helper tools</source>
      <translation>輔助工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Modeling tools</source>
      <translation>建模工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Create Geometry</source>
      <translation>建立幾何</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute parameter</source>
      <translation>漸開線參數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>齒數：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module:</source>
      <translation>模組：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle:</source>
      <translation>壓力角：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision:</source>
      <translation>高精度：</translation>
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
      <source>External gear:</source>
      <translation>外齒輪：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum Coefficient</source>
      <translation>齒冠係數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum Coefficient</source>
      <translation>齒根係數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root Fillet Coefficient</source>
      <translation>齒根圓角係數</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile Shift Coefficient</source>
      <translation>齒廓移位係數</translation>
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
      <source>To create a new PartDesign object, there must be an active Body object in the document.

Please select a body from below, or create a new body.</source>
      <translation>要建立新的零件設計物件，文件中必須有一個作業中主體物件。
請由下選擇一個主體，或建立一個新的主體。</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="36"/>
      <source>Create new body</source>
      <translation>建立新主體</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length:</source>
      <translation>長度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width:</source>
      <translation>寬度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height:</source>
      <translation>高度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius:</source>
      <translation>半徑：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <source>Angle in first direction:</source>
      <translation>第一方向之角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>第一方向之角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <source>Angle in second direction:</source>
      <translation>第二方向之角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>第二方向之角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle:</source>
      <translation>旋轉角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1:</source>
      <translation>半徑 1：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2:</source>
      <translation>半徑 2：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle:</source>
      <translation>角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <source>U parameter:</source>
      <translation>繞指定軸角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters:</source>
      <translation>與指定軸之夾角：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>局部 z-方向之半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local x-direction</source>
      <translation>局部 x-方向之半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3:</source>
      <translation>半徑 3：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local y-direction
If zero, it is equal to Radius2</source>
      <translation>局部 y-方向之半徑
若為 0 的話則等同於半徑2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter:</source>
      <translation>與指定軸之夾角：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local xy-plane</source>
      <translation>局部 xy-平面之半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local xz-plane</source>
      <translation>局部 xz-平面之半徑</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U Parameter:</source>
      <translation>繞指定軸角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon:</source>
      <translation>多邊形：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius:</source>
      <translation>外接圓</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max:</source>
      <translation>X 最小/最大：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max:</source>
      <translation>Y 最小/最大：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max:</source>
      <translation>Z 最小/最大：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max:</source>
      <translation>X2 最小/最大：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max:</source>
      <translation>Z2 最小/最大：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch:</source>
      <translation>螺距:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system:</source>
      <translation>座標系統：</translation>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth:</source>
      <translation>增長：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations:</source>
      <translation>迴旋數量：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1:</source>
      <translation>角度 1：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2:</source>
      <translation>角度 2：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From three points</source>
      <translation>由3點組成</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius:</source>
      <translation>長軸半徑：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius:</source>
      <translation>短軸半徑：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X:</source>
      <translation>X：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y:</source>
      <translation>Y：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z:</source>
      <translation>Z：</translation>
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
      <location filename="../../ReferenceSelection.cpp" line="270"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>挑選這個可能導致循環參照</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add body</source>
      <translation>加入主體</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove body</source>
      <translation>刪除主體</translation>
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
      <location filename="../../TaskBooleanParameters.cpp" line="53"/>
      <source>Boolean parameters</source>
      <translation>布林運算參數</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="86"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="50"/>
      <source>Primitive parameters</source>
      <translation>幾何形體參數</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="916"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="922"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="928"/>
      <source>Invalid wedge parameters</source>
      <translation>無效的楔形體參數</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="917"/>
      <source>X min must not be equal to X max!</source>
      <translation>X 最小值必須不等於 X 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="923"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y 最小值必須不等於 Y 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="929"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z 最小值必須不等於 Z 最大值!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="967"/>
      <source>Create primitive</source>
      <translation>建立幾何形體</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>點擊按鍵以進入選擇模式，再點擊以結束選擇。</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="24"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看倒角</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="49"/>
      <source>Type</source>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="57"/>
      <source>Equal distance</source>
      <translation>同等距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="62"/>
      <source>Two distances</source>
      <translation>兩個距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="67"/>
      <source>Distance and angle</source>
      <translation>距離與角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="80"/>
      <source>Flip direction</source>
      <translation>翻轉方向</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="101"/>
      <source>Size</source>
      <translation>尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="120"/>
      <source>Use All Edges</source>
      <translation>使用所有邊線</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="147"/>
      <source>Size 2</source>
      <translation>尺寸 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="180"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="332"/>
      <source>Empty chamfer created !
</source>
      <translation>已建立空倒角 !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="392"/>
      <source>Empty body list</source>
      <translation>清空主體清單</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="393"/>
      <source>The body list cannot be empty</source>
      <translation>主體列表不能空白</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="407"/>
      <source>Boolean: Accept: Input error</source>
      <translation>布林值: 接受: 輸出錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="101"/>
      <source>Incompatible reference set</source>
      <translation>不相容的參照集</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="102"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>沒有適合當前參考集的附件模式。如果您選擇繼續，該特徵將保留在它現在的位置，並且不會隨著參考的變化而移動。是否繼續 ?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="146"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="412"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>點擊按鍵以進入選擇模式，再點擊以結束選擇。</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="24"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看草稿</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="47"/>
      <source>Draft angle</source>
      <translation>拔模角度</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="80"/>
      <source>Neutral plane</source>
      <translation>中立面</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="97"/>
      <source>Pull direction</source>
      <translation>拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="112"/>
      <source>Reverse pull direction</source>
      <translation>反轉拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="292"/>
      <source>Empty draft created !
</source>
      <translation>已建立空草稿!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="84"/>
      <source>Preview</source>
      <translation>預覽</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="90"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Add all edges</source>
      <translation>添加所有邊線</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="300"/>
      <source>Adds all edges to the list box (active only when in add selection mode).</source>
      <translation>將所有邊添加到列表框（僅在添加選擇模式下有效）。</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="308"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1218"/>
      <source>No face selected</source>
      <translation>無選定之面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="160"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1053"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="176"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="268"/>
      <source>Preview</source>
      <translation>預覽</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="272"/>
      <source>Select faces</source>
      <translation>選取面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="496"/>
      <source>No shape selected</source>
      <translation>無選取物件</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="604"/>
      <source>Sketch normal</source>
      <translation>草圖法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="607"/>
      <source>Face normal</source>
      <translation>面法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="611"/>
      <source>Select reference...</source>
      <translation>選取參考...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="615"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="620"/>
      <source>Custom direction</source>
      <translation>自訂方向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1005"/>
      <source>Click on a shape in the model</source>
      <translation>點擊模型中的形狀</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1213"/>
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
      <source>Allow external features</source>
      <translation>允許外部特徵</translation>
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
      <location filename="../../TaskFeaturePick.cpp" line="63"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="65"/>
      <source>Invalid shape</source>
      <translation>非有效形狀</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>No wire in sketch</source>
      <translation>草圖找不到線段</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Sketch already used by other feature</source>
      <translation>草圖已被其他特徵使用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Belongs to another body</source>
      <translation>屬於另一個主體</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Belongs to another part</source>
      <translation>屬於另一個零件</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Doesn't belong to any body</source>
      <translation>不屬於任何主體</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Base plane</source>
      <translation>基準面</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Feature is located after the tip feature</source>
      <translation>特徵位於尖點特徵之後</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="89"/>
      <source>Select attachment</source>
      <translation>選擇附件</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>點擊按鍵以進入選擇模式，再點擊以結束選擇。</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="24"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看其圓角</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="47"/>
      <source>Radius:</source>
      <translation>半徑：</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="63"/>
      <source>Use All Edges</source>
      <translation>使用所有邊線</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="198"/>
      <source>Empty fillet created!</source>
      <translation>已建立空倒圓角!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status:</source>
      <translation>狀態:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis:</source>
      <translation>軸：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="237"/>
      <source>Base X axis</source>
      <translation>基本 X 軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="238"/>
      <source>Base Y axis</source>
      <translation>物體原點的Y軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="239"/>
      <source>Base Z axis</source>
      <translation>Z 軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="220"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="219"/>
      <source>Normal sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="204"/>
      <source>Select reference...</source>
      <translation>選取參考...</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode:</source>
      <translation>模式：</translation>
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
      <source>Pitch:</source>
      <translation>螺距:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height:</source>
      <translation>高度：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns:</source>
      <translation>圈數：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle:</source>
      <translation>錐角：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth:</source>
      <translation>徑向增長：</translation>
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
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Update view</source>
      <translation>更新視圖</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="56"/>
      <source>Helix parameters</source>
      <translation>螺旋參數</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="289"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告：螺旋可能自相交</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="294"/>
      <source>Error: helix touches itself</source>
      <translation>錯誤：螺旋碰觸到自己</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="343"/>
      <source>Error: unsupported mode</source>
      <translation>錯誤：不支援的模式</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="47"/>
      <source>Counterbore</source>
      <translation>沉頭孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="48"/>
      <source>Countersink</source>
      <translation>埋頭孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="49"/>
      <source>Counterdrill</source>
      <translation>連柱坑鑽頭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Hole parameters</source>
      <translation>圓孔參數</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="63"/>
      <source>None</source>
      <translation>無</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="64"/>
      <source>ISO metric regular profile</source>
      <translation>ISO 公制常規輪廓</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>ISO metric fine profile</source>
      <translation>ISO公制細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="66"/>
      <source>UTS coarse profile</source>
      <translation>UTS統一英制粗牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>UTS fine profile</source>
      <translation>UTS統一英制細牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>UTS extra fine profile</source>
      <translation>UTS統一英制特細牙</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLinearPatternParameters</name>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="34"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="46"/>
      <source>Reverse direction</source>
      <translation>反轉方向</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="55"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="63"/>
      <source>Overall Length</source>
      <translation>總長度</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="68"/>
      <location filename="../../TaskLinearPatternParameters.ui" line="131"/>
      <source>Offset</source>
      <translation>偏移</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="93"/>
      <source>Length</source>
      <translation>間距</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="156"/>
      <source>Occurrences</source>
      <translation>產生次數</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="330"/>
      <source>Error</source>
      <translation>錯誤</translation>
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
      <source>Update view</source>
      <translation>更新視圖</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft parameters</source>
      <translation>拉伸成形參數</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="74"/>
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
      <location filename="../../TaskMirroredParameters.cpp" line="180"/>
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
      <location filename="../../TaskMultiTransformParameters.cpp" line="71"/>
      <source>Edit</source>
      <translation>編輯</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Delete</source>
      <translation>刪除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add mirrored transformation</source>
      <translation>加入鏡射效果</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="89"/>
      <source>Add linear pattern</source>
      <translation>加入線狀排列效果</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="95"/>
      <source>Add polar pattern</source>
      <translation>加入環狀排列效果</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="101"/>
      <source>Add scaled transformation</source>
      <translation>加入縮放效果</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="107"/>
      <source>Move up</source>
      <translation>上移</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="110"/>
      <source>Move down</source>
      <translation>下移</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="144"/>
      <source>Right-click to add</source>
      <translation>按滑鼠右鍵加入</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad parameters</source>
      <translation>填充參數</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset from face at which pad will end</source>
      <translation>偏移自面在其中填充將會結束</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Reverses pad direction</source>
      <translation>相反填充方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="70"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>To last</source>
      <translation>到最後位置</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Two dimensions</source>
      <translation>雙向填充</translation>
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
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Type</source>
      <translation>類型</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="38"/>
      <source>Length</source>
      <translation>間距</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="75"/>
      <source>Offset to face</source>
      <translation>偏移至面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="130"/>
      <source>Select shape</source>
      <translation>選擇形狀</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="148"/>
      <source>Select all faces</source>
      <translation>選擇所有面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="170"/>
      <source>Click button to enter selection mode,
         click again to end selection</source>
      <translation>點擊按鍵以進入選擇模式，
再點擊以結束選擇</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="244"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="252"/>
      <source>Direction/edge:</source>
      <translation>方向/邊：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="259"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>設定方向或是由模型選擇一個邊作為參考</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="264"/>
      <source>Sketch normal</source>
      <translation>草圖法線</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="269"/>
      <source>Select reference...</source>
      <translation>選取參考...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="274"/>
      <source>Custom direction</source>
      <translation>自訂方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="284"/>
      <source>Show direction</source>
      <translation>顯示方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="294"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>使用自定向量作為填充方向，否則使用草圖平面的法線來長厚度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="307"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="314"/>
      <source>x-component of direction vector</source>
      <translation>方向向量的 x 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="336"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="343"/>
      <source>y-component of direction vector</source>
      <translation>方向向量的 y 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="365"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="372"/>
      <source>z-component of direction vector</source>
      <translation>方向向量的 z 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>若未選擇的話，其長度將沿著指定方向測量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="407"/>
      <source>Length along sketch normal</source>
      <translation>沿著草圖法線的長度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="145"/>
      <location filename="../../TaskPadPocketParameters.ui" line="227"/>
      <source>Applies length symmetrically to sketch plane</source>
      <translation>將長度對稱地應用於草圖平面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="230"/>
      <source>Symmetric to plane</source>
      <translation>依平面對稱</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="237"/>
      <source>Reversed</source>
      <translation>反轉</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="419"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Angle to taper the extrusion</source>
      <translation>擠出錐度的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="422"/>
      <source>Taper angle</source>
      <translation>斜角</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="58"/>
      <source>2nd length</source>
      <translation>第二長度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>2nd taper angle</source>
      <translation>第2斜角</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="212"/>
      <source>Select face</source>
      <translation>選擇面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="472"/>
      <source>Update view</source>
      <translation>更新視圖</translation>
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
      <source>Curvelinear equivalence</source>
      <translation>相同曲線</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="583"/>
      <source>Section orientation</source>
      <translation>輪廓圖方向</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="611"/>
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
      <source>Corner Transition</source>
      <translation>轉角轉換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>變換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right Corner</source>
      <translation>右角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round Corner</source>
      <translation>圓角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to sweep along</source>
      <translation>要掃掠之路徑</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add Edge</source>
      <translation>新增邊界</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove Edge</source>
      <translation>移除邊界</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="68"/>
      <source>Pipe parameters</source>
      <translation>管件參數</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="88"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="451"/>
      <location filename="../../TaskPipeParameters.cpp" line="563"/>
      <source>Input error</source>
      <translation>輸入錯誤</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="451"/>
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
      <location filename="../../TaskPipeParameters.cpp" line="887"/>
      <source>Section transformation</source>
      <translation>輪廓圖轉換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="904"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket parameters</source>
      <translation>凹陷參數</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from face at which pocket will end</source>
      <translation>偏移至面在其中凹陷將會結束</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation>相反凹陷方向</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="72"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Two dimensions</source>
      <translation>雙向填充</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to shape</source>
      <translation>上升到形狀</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPolarPatternParameters</name>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="34"/>
      <source>Axis</source>
      <translation>軸</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="46"/>
      <source>Reverse direction</source>
      <translation>反轉方向</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="55"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="63"/>
      <source>Overall Angle</source>
      <translation>總角度</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="68"/>
      <source>Offset Angle</source>
      <translation>偏移角度</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="93"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="137"/>
      <source>Offset</source>
      <translation>偏移</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="168"/>
      <source>Occurrences</source>
      <translation>產生次數</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="329"/>
      <source>Error</source>
      <translation>錯誤</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="981"/>
      <source>Attachment</source>
      <translation>附件</translation>
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
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="174"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis:</source>
      <translation>軸：</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="219"/>
      <source>Base X axis</source>
      <translation>基本 X 軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="220"/>
      <source>Base Y axis</source>
      <translation>物體原點的Y軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="221"/>
      <source>Base Z axis</source>
      <translation>Z 軸</translation>
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
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="228"/>
      <source>Select reference...</source>
      <translation>選取參考...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Angle:</source>
      <translation>角度：</translation>
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
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="138"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="447"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Update view</source>
      <translation>更新視圖</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.h" line="52"/>
      <source>Revolution parameters</source>
      <translation>旋轉成形參數</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="176"/>
      <source>To last</source>
      <translation>到最後位置</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="179"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="181"/>
      <source>To first</source>
      <translation>到起始面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <source>Up to face</source>
      <translation>向上至面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="183"/>
      <source>Two dimensions</source>
      <translation>雙向填充</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="433"/>
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
      <location filename="../../TaskShapeBinder.cpp" line="60"/>
      <source>Datum shape parameters</source>
      <translation>基準形狀參數</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="130"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="174"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>點擊按鍵以進入選擇模式，再點擊以結束選擇。</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="24"/>
      <source>Select</source>
      <translation>選擇</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- 選擇一個項目以突出顯示它
- 雙擊一個項目以查看其特徵</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="47"/>
      <source>Thickness</source>
      <translation>厚度</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="76"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="84"/>
      <source>Skin</source>
      <translation>單面</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="89"/>
      <source>Pipe</source>
      <translation>管件</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="94"/>
      <source>Recto Verso</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="102"/>
      <source>Join Type</source>
      <translation>接合方式</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="110"/>
      <source>Arc</source>
      <translation>圓弧</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="115"/>
      <location filename="../../TaskThicknessParameters.ui" line="125"/>
      <source>Intersection</source>
      <translation>直角</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="132"/>
      <source>Make thickness inwards</source>
      <translation>壁厚朝內</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="265"/>
      <source>Empty thickness created !
</source>
      <translation>空厚度被建立！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed feature messages</source>
      <translation>特徵效果訊息</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="104"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="408"/>
      <source>Normal sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="409"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="410"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="412"/>
      <location filename="../../TaskTransformedParameters.cpp" line="448"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="426"/>
      <source>Base X axis</source>
      <translation>基本 X 軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="427"/>
      <source>Base Y axis</source>
      <translation>物體原點的Y軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="428"/>
      <source>Base Z axis</source>
      <translation>Z 軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="436"/>
      <location filename="../../TaskTransformedParameters.cpp" line="472"/>
      <source>Select reference...</source>
      <translation>選取參考...</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="462"/>
      <source>Base XY plane</source>
      <translation>XY 平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="463"/>
      <source>Base YZ plane</source>
      <translation>YZ 平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="464"/>
      <source>Base XZ plane</source>
      <translation>XZ 平面</translation>
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
      <source>Add feature</source>
      <translation>加入特徵</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove feature</source>
      <translation>移除特徵</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>清單可以通過拖曳來重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Update view</source>
      <translation>更新視圖</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer parameters</source>
      <translation>倒角參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane parameters</source>
      <translation>基準平面參數</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line parameters</source>
      <translation>基準線參數</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point parameters</source>
      <translation>基準點參數</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System parameters</source>
      <translation>局部座標系參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft parameters</source>
      <translation>拔模參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet parameters</source>
      <translation>圓角參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>LinearPattern parameters</source>
      <translation>線性樣式參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>MultiTransform parameters</source>
      <translation>多重轉換參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>PolarPattern parameters</source>
      <translation>環狀複製樣式參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scaled parameters</source>
      <translation>縮放參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness parameters</source>
      <translation>厚度參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirrored parameters</source>
      <translation>鏡像參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="198"/>
      <source>Create an additive box by its width, height, and length</source>
      <translation>依長、寬、高建立立方體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="202"/>
      <source>Create an additive cylinder by its radius, height, and angle</source>
      <translation>依半徑、高、角度建立圓柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="206"/>
      <source>Create an additive sphere by its radius and various angles</source>
      <translation>依半徑、多個角度建立球狀體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="210"/>
      <source>Create an additive cone</source>
      <translation>建立圓錐體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="214"/>
      <source>Create an additive ellipsoid</source>
      <translation>建立橢圓體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="218"/>
      <source>Create an additive torus</source>
      <translation>建立中空環狀體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Create an additive prism</source>
      <translation>建立角柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Create an additive wedge</source>
      <translation>建立楔形體</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="350"/>
      <source>Create a subtractive box by its width, height and length</source>
      <translation>依長寬高建立除料立方體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="354"/>
      <source>Create a subtractive cylinder by its radius, height and angle</source>
      <translation>依半徑、高、角度建立除料圓柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="358"/>
      <source>Create a subtractive sphere by its radius and various angles</source>
      <translation>依半徑與角度建立除料球體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="362"/>
      <source>Create a subtractive cone</source>
      <translation>建立除料圓錐體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="366"/>
      <source>Create a subtractive ellipsoid</source>
      <translation>建立除料橢圓體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="370"/>
      <source>Create a subtractive torus</source>
      <translation>建立除料中空環體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="374"/>
      <source>Create a subtractive prism</source>
      <translation>建立除料角柱體</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="378"/>
      <source>Create a subtractive wedge</source>
      <translation>建立除料楔形體</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="738"/>
      <source>Select body</source>
      <translation>選擇主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="739"/>
      <source>Select a body from the list</source>
      <translation>從清單中選擇主體</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="891"/>
      <source>Select feature</source>
      <translation>選擇特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="892"/>
      <source>Select a feature from the list</source>
      <translation>從清單中選擇特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="962"/>
      <source>Move tip</source>
      <translation>移動尖點</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="963"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>移動特徵出現在當前設置的尖點之後。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="964"/>
      <source>Do you want the last feature to be the new tip?</source>
      <translation>您希望最後一個特徵為新的尖點嗎？</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>Invalid selection</source>
      <translation>無效選擇</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>沒有適合選定物件的附件模式。選擇其它東西。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <location filename="../../Command.cpp" line="149"/>
      <location filename="../../Command.cpp" line="151"/>
      <source>Error</source>
      <translation>錯誤</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>There is no active body. Please make a body active before inserting a datum entity.</source>
      <translation>目前沒有作業中主體，請在插入基準個體前設定一個主體為作業中。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>Sub-Shape Binder</source>
      <translation>子形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="656"/>
      <source>Several sub-elements selected</source>
      <translation>多個次元素被選取</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="657"/>
      <source>You have to select a single face as support for a sketch!</source>
      <translation>您需要選擇單一面作為草圖之基準面!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="660"/>
      <source>No support face selected</source>
      <translation>未選取基礎面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="661"/>
      <source>You have to select a face as support for a sketch!</source>
      <translation>您需要選擇一個面作為草圖之基準面!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="664"/>
      <source>No planar support</source>
      <translation>無平面之基礎面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="665"/>
      <source>You need a planar face as support for a sketch!</source>
      <translation>您需要選取平面作為草圖之基準面!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="668"/>
      <source>No valid planes in this document</source>
      <translation>在本文件的非法平面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="669"/>
      <source>Please create a plane first or select a face to sketch on</source>
      <translation>請先建立一個平面或選擇要在其上繪製草圖的面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="77"/>
      <location filename="../../ViewProviderDatum.cpp" line="250"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="68"/>
      <location filename="../../SketchWorkflow.cpp" line="592"/>
      <location filename="../../ViewProvider.cpp" line="96"/>
      <location filename="../../ViewProviderBoolean.cpp" line="78"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <location filename="../../Command.cpp" line="1016"/>
      <source>A dialog is already open in the task panel</source>
      <translation>於工作面板已開啟對話窗</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="78"/>
      <location filename="../../ViewProviderDatum.cpp" line="251"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="69"/>
      <location filename="../../SketchWorkflow.cpp" line="593"/>
      <location filename="../../ViewProvider.cpp" line="97"/>
      <location filename="../../ViewProviderBoolean.cpp" line="79"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <location filename="../../Command.cpp" line="1017"/>
      <source>Do you want to close this dialog?</source>
      <translation>您確定要關閉此對話窗嗎?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="896"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>無法使用此命令，因為沒有可減去之固體。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="897"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>在嘗試除料命令之前，請確保主體包含特徵。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="918"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>無法使用選定的物件。選定物件必須屬於作業中主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="919"/>
      <source>Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</source>
      <translation>考慮使用 ShapeBinder 或 BaseFeature 來引用主體中的外部幾何體。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="941"/>
      <source>No sketch to work on</source>
      <translation>沒有可工作之草圖</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="942"/>
      <source>No sketch is available in the document</source>
      <translation>文檔中沒有可用之草圖</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1646"/>
      <location filename="../../Command.cpp" line="1672"/>
      <source>Wrong selection</source>
      <translation>錯誤的選擇</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1647"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>自單一主體中選擇一個邊、面或是主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1651"/>
      <location filename="../../Command.cpp" line="1983"/>
      <source>Selection is not in Active Body</source>
      <translation>選擇不在一個作業中主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1652"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>自一個作業中主體選擇一個邊、面或是主體</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1662"/>
      <source>Wrong object type</source>
      <translation>物件種類錯誤</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1663"/>
      <source>%1 works only on parts.</source>
      <translation>%1 僅能用於零件上</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1673"/>
      <source>Shape of the selected Part is empty</source>
      <translation>所選零件的形狀為空白</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1984"/>
      <source>Please select only one feature in an active body.</source>
      <translation>請在一個作業中主體只選擇一個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="69"/>
      <source>Part creation failed</source>
      <translation>新增零件失敗</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="70"/>
      <source>Failed to create a part object.</source>
      <translation>建立一個零件物件失敗</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="114"/>
      <location filename="../../CommandBody.cpp" line="119"/>
      <location filename="../../CommandBody.cpp" line="132"/>
      <location filename="../../CommandBody.cpp" line="181"/>
      <source>Bad base feature</source>
      <translation>壞掉的基礎特性</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="115"/>
      <source>Body can't be based on a PartDesign feature.</source>
      <translation>主體不能基於零件設計特徵上</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="120"/>
      <source>%1 already belongs to a body, can't use it as base feature for another body.</source>
      <translation>%1 已經屬於某個主體，不能用在另一個主體的基礎特徵上</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="133"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>基礎特徵 (%1) 屬於其他部分。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="157"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個實體組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="161"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="165"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>選定的形狀只由一個殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="169"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>選定的形狀由多個實體或殼組成。
這可能導致無法預測的結果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="174"/>
      <source>Base feature</source>
      <translation>基礎特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="182"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>主體可能基於不超過一個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="196"/>
      <source>Body</source>
      <translation>主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="345"/>
      <source>Nothing to migrate</source>
      <translation>沒有什麼東西可以遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="346"/>
      <source>No PartDesign features found that don't belong to a body. Nothing to migrate.</source>
      <translation>未找到不屬於主體的 PartDesign 特徵。沒有什麼東西可以遷移。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="494"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>草圖平面無法被遷移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="495"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>請編輯 '%1' 並重新定義它以使用 Base 或 Datum 平面作為草圖平面。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="557"/>
      <location filename="../../CommandBody.cpp" line="561"/>
      <location filename="../../CommandBody.cpp" line="566"/>
      <location filename="../../CommandBody.cpp" line="863"/>
      <location filename="../../CommandBody.cpp" line="870"/>
      <source>Selection error</source>
      <translation>選取錯誤</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="558"/>
      <source>Select exactly one PartDesign feature or a body.</source>
      <translation>選擇正好一個 PartDesign 特徵或是一個主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="562"/>
      <source>Couldn't determine a body for the selected feature '%s'.</source>
      <translation>無法決定所選特徵之主體</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="567"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>只有實心特徵能成為主體的尖點。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="689"/>
      <location filename="../../CommandBody.cpp" line="711"/>
      <location filename="../../CommandBody.cpp" line="726"/>
      <source>Features cannot be moved</source>
      <translation>特徵無法被移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="690"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>某些被選的特徵與來源實體有相依性</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="712"/>
      <source>Only features of a single source Body can be moved</source>
      <translation>只有單一來源主體的特徵可以被移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="727"/>
      <source>There are no other bodies to move to</source>
      <translation>沒有其它主體可以搬移過去</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="864"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>不可能移動主體的基礎特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="871"/>
      <source>Select one or more features from the same body.</source>
      <translation>選擇相同主體的一個或更多個特徵</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="884"/>
      <source>Beginning of the body</source>
      <translation>主體的起點</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="948"/>
      <source>Dependency violation</source>
      <translation>相依性衝突</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="949"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>較早的特徵不可以相依在較晚的特徵。</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="263"/>
      <source>No previous feature found</source>
      <translation>找不到先前特徵</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>如果沒有可用的基礎特徵，則無法建立除料特徵</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="445"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="205"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="446"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="206"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草圖軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="208"/>
      <source>Construction line %1</source>
      <translation>作圖線 %1：</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="80"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="138"/>
      <source>In order to use PartDesign you need an active Body object in the document. Please make one active (double click) or create one.

If you have a legacy document with PartDesign objects without Body, use the migrate function in PartDesign to put them into a Body.</source>
      <translation>要使用零件設計物件，文件中必須有一個作業中主體物件。請雙擊讓一個主體為作業中，或建立一個新的主體。
如果您使用舊版文件其零件設計物件沒有主體的話，使用零件設計中之遷移功能將他們搬到一個主體中。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="186"/>
      <source>Active Body Required</source>
      <translation>需要作業中主體</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="187"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document. Please make one active (double click) or create a new Body.</source>
      <translation>要建立新的零件設計物件，文件中必須有一個作業中主體物件。
請雙擊讓一個主體為作業中，或建立一個新的主體。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="222"/>
      <source>Feature is not in a body</source>
      <translation>特徵不在主體中</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="223"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>為了使用此特徵，它需要屬於文件中的主體物件。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="255"/>
      <source>Feature is not in a part</source>
      <translation>特徵不在零件中</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="256"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>為了使用此特徵，它需要屬於文件中的一個零件物件。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="50"/>
      <location filename="../../ViewProviderTransformed.cpp" line="76"/>
      <location filename="../../ViewProvider.cpp" line="63"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="209"/>
      <source>Edit %1</source>
      <translation>編輯 %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="76"/>
      <source>Set colors...</source>
      <translation>設定顏色...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="62"/>
      <source>Edit boolean</source>
      <translation>編輯布林</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="208"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="204"/>
      <source>Line</source>
      <translation>線</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Point</source>
      <translation>點</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>座標系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="229"/>
      <source>Edit datum</source>
      <translation>編輯基準</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="74"/>
      <source>Feature error</source>
      <translation>特徵錯誤</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="75"/>
      <source>%1 misses a base feature.
This feature is broken and can't be edited.</source>
      <translation>%1 缺少基礎特徵。
此特徵已損毀而無法編輯。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="46"/>
      <source>Edit groove</source>
      <translation>編輯挖槽</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="61"/>
      <source>Edit hole</source>
      <translation>編輯洞</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="66"/>
      <source>Edit loft</source>
      <translation>編輯拉伸成形</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="46"/>
      <source>Edit pad</source>
      <translation>編輯填充</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="74"/>
      <source>Edit pipe</source>
      <translation>編輯管件</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="48"/>
      <source>Edit pocket</source>
      <translation>編輯凹陷</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="54"/>
      <source>Edit primitive</source>
      <translation>編輯幾何形體</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="46"/>
      <source>Edit revolution</source>
      <translation>編輯旋轉成形</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="204"/>
      <source>Edit shape binder</source>
      <translation>編輯形狀粘合劑</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="316"/>
      <source>Synchronize</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="318"/>
      <source>Select bound object</source>
      <translation>選擇綁定物件</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="179"/>
      <source>One transformed shape does not intersect the support</source>
      <translation>一個變換後的形狀與支撐體不相交</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="181"/>
      <source>%1 transformed shapes do not intersect the support</source>
      <translation>%1 個變換後的形狀與支撐體不相交</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="191"/>
      <source>Transformation succeeded</source>
      <translation>效果成功產生</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="140"/>
      <source>The document "%1" you are editing was designed with an old version of PartDesign workbench.</source>
      <translation>您所編輯的文件 "%1" 是由舊版的 PartDesign 工作台所設計。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="143"/>
      <source>Do you want to migrate in order to use modern PartDesign features?</source>
      <translation>您要遷移以使用現代 PartDesign 功能嗎？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="146"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy PartDesign or have a slightly broken structure.</source>
      <translation>文檔 "%1" 似乎正處於從舊 PartDesign 遷移過程的中間，或者具有輕微損壞的結構。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Do you want to make the migration automatically?</source>
      <translation>您需要使遷移自動進行嗎?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="152"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>注意：如果您選擇遷移，您將無法使用舊版 FreeCAD 編輯文件。
如果您拒絕遷移，您將無法使用新的 PartDesign 功能，例如實體和零件。因此，您也將無法在 assembly 工作台中使用您的零件。
儘管您稍後可以使用“零件設計 -&gt; 遷移”進行遷移。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate manually</source>
      <translation>手動遷移</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="51"/>
      <source>Edit helix</source>
      <translation>編輯螺旋</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="41"/>
      <source>Edit chamfer</source>
      <translation>編輯倒角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="42"/>
      <source>Edit draft</source>
      <translation>編輯草稿</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="41"/>
      <source>Edit fillet</source>
      <translation>編輯圓角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="39"/>
      <source>Edit linear pattern</source>
      <translation>編輯線性樣式</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="39"/>
      <source>Edit mirrored</source>
      <translation>編輯鏡像</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="48"/>
      <source>Edit multi-transform</source>
      <translation>編輯多重轉換</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit polar pattern</source>
      <translation>編輯環形樣式</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="39"/>
      <source>Edit scaled</source>
      <translation>編輯縮放</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="41"/>
      <source>Edit thickness</source>
      <translation>編輯厚度</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket parameters</source>
      <translation>鏈輪參數</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth:</source>
      <translation>齒數：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket Reference</source>
      <translation>鏈輪參考</translation>
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
      <source>Bicycle with Derailleur</source>
      <translation>變速腳踏車</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without Derailleur</source>
      <translation>單速腳踏車</translation>
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
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain Pitch:</source>
      <translation>鏈節距：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 吋</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain Roller Diameter:</source>
      <translation>鏈滾子直徑：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth Width:</source>
      <translation>齒寬：</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Task Hole Parameters</source>
      <translation>工作孔參數</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>輪廓</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="290"/>
      <source>Whether the hole gets a thread</source>
      <translation>孔是否有螺紋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Threaded</source>
      <translation>螺紋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="447"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>孔是否有建模螺紋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="450"/>
      <source>Model Thread</source>
      <translation>建模螺紋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="466"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>螺紋修改的即時更新
請注意計算會需要一些時間</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="483"/>
      <source>Customize thread clearance</source>
      <translation>自訂螺紋間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="90"/>
      <source>Clearance</source>
      <translation>間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="470"/>
      <source>Update thread view</source>
      <translation>更新螺紋視圖</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="486"/>
      <source>Custom Clearance</source>
      <translation>自訂間隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="499"/>
      <source>Custom Thread clearance value</source>
      <translation>自訂螺紋間隙值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="394"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="431"/>
      <source>Right hand</source>
      <translation>右手</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="410"/>
      <source>Left hand</source>
      <translation>左手</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="63"/>
      <source>Size</source>
      <translation>尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="103"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>孔洞間隙
僅適用於無螺紋孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="108"/>
      <location filename="../../TaskHoleParameters.cpp" line="660"/>
      <source>Standard</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="113"/>
      <location filename="../../TaskHoleParameters.cpp" line="663"/>
      <location filename="../../TaskHoleParameters.cpp" line="680"/>
      <source>Close</source>
      <translation>關閉</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <location filename="../../TaskHoleParameters.cpp" line="666"/>
      <source>Wide</source>
      <translation>寬</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>Class</source>
      <translation>類別 (Class)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="321"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>根據孔輪廓的螺紋孔公差等級</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="692"/>
      <source>Drill Point</source>
      <translation>鑽孔點</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="134"/>
      <location filename="../../TaskHoleParameters.ui" line="556"/>
      <source>Diameter</source>
      <translation>直徑</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="147"/>
      <source>Hole diameter</source>
      <translation>孔洞直徑</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="171"/>
      <location filename="../../TaskHoleParameters.ui" line="336"/>
      <location filename="../../TaskHoleParameters.ui" line="594"/>
      <source>Depth</source>
      <translation>深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="185"/>
      <location filename="../../TaskHoleParameters.ui" line="355"/>
      <source>Dimension</source>
      <translation>標註尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="190"/>
      <source>Through all</source>
      <translation>完全貫穿</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="350"/>
      <source>Hole depth</source>
      <translation>孔深</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="360"/>
      <source>Tapped (DIN76)</source>
      <translation>攻牙(DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="543"/>
      <source>Cut type for screw heads</source>
      <translation>螺絲頭的切割類型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="665"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>選取覆蓋「類型」預定義的值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="668"/>
      <source>Custom values</source>
      <translation>自訂值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>對於埋頭孔來說，這是螺絲頭部分位於表面以下的深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="530"/>
      <source>Hole Cut Type</source>
      <translation>孔切割類型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="633"/>
      <source>Countersink angle</source>
      <translation>埋頭角</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="705"/>
      <source>Flat</source>
      <translation>平</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="721"/>
      <source>Angled</source>
      <translation>有角度的</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="758"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>鑽尖尺寸將不會用來考慮作盲孔深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="762"/>
      <source>Take into account for depth</source>
      <translation>考慮深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="240"/>
      <source>Tapered</source>
      <translation>錐</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="255"/>
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
      <location filename="../../TaskHoleParameters.ui" line="224"/>
      <source>Reverses the hole direction</source>
      <translation>反轉孔之方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="227"/>
      <source>Reversed</source>
      <translation>反轉</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="677"/>
      <source>Normal</source>
      <translation>垂直</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="683"/>
      <source>Loose</source>
      <translation>寬鬆</translation>
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
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;草圖</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;零件設計</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Create a datum</source>
      <translation>建立基準</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Create an additive feature</source>
      <translation>建立可添加特徵</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Create a subtractive feature</source>
      <translation>建立除料特徵</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Apply a pattern</source>
      <translation>套用一個樣式</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Apply a dress-up feature</source>
      <translation>套用 dress-up 功能</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket...</source>
      <translation>鏈輪...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute gear...</source>
      <translation>漸開線齒輪...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Shaft design wizard</source>
      <translation>軸設計精靈</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Measure</source>
      <translation>測量</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Refresh</source>
      <translation>重新運算</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Toggle 3D</source>
      <translation>切換 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Helper</source>
      <translation>零件設計幫手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Modeling</source>
      <translation>零件設計建模</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute gear...</source>
      <translation>漸開線齒輪...</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edit the involute gear definition.</source>
      <translation>建立或編輯漸開線齒輪定義。</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket...</source>
      <translation>鏈輪...</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edit the sprocket definition.</source>
      <translation>建立或編輯鏈輪定義。</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Length [mm]</source>
      <translation>長度[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Diameter [mm]</source>
      <translation>外徑[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Inner diameter [mm]</source>
      <translation>內徑[mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Constraint type</source>
      <translation>拘束類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Start edge type</source>
      <translation>開始邊類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge size</source>
      <translation>開始邊尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>End edge type</source>
      <translation>結束邊類型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>結束邊尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="64"/>
      <source>Shaft wizard</source>
      <translation>軸精靈</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 1</source>
      <translation>輪廊圖 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 2</source>
      <translation>輪廊圖 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Add column</source>
      <translation>新增欄位</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation>輪廊圖 %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="150"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="165"/>
      <source>None</source>
      <translation>無</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="151"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="152"/>
      <source>Force</source>
      <translation>力</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="153"/>
      <source>Bearing</source>
      <translation>軸承</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="154"/>
      <source>Gear</source>
      <translation>齒輪</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="155"/>
      <source>Pulley</source>
      <translation>滑輪</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="166"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="167"/>
      <source>Fillet</source>
      <translation>圓角</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="57"/>
      <source>All</source>
      <translation>所有</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="104"/>
      <source>Missing module</source>
      <translation>缺少模組</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>You may have to install the Plot add-on</source>
      <translation>您可能需要安裝繪圖 (Plot) 附加元件</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft design wizard...</source>
      <translation>軸設計精靈...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Start the shaft design wizard</source>
      <translation>啟動軸設計精靈</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="214"/>
      <source>Shaft design wizard...</source>
      <translation>軸設計精靈...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="215"/>
      <source>Start the shaft design wizard</source>
      <translation>啟動軸設計精靈</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="401"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>被鏈接物件不是零件設計特徵</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="408"/>
      <source>Tip shape is empty</source>
      <translation>尖端形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="64"/>
      <source>BaseFeature link is not set</source>
      <translation>BaseFeature 連結未設置</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="69"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>BaseFeature 必須是一個 Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="78"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature 有一個空形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="82"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>無法在沒有 BaseFeature 的情況下進行布林切割</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="96"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="125"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>無法對除 Part::Feature 及其衍生物之外的任何物件進行布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="103"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>無法對無效的基礎形狀進行布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="109"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation>無法對不在主體中的特徵進行布林操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="135"/>
      <source>Base shape is null</source>
      <translation>基礎形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="116"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="138"/>
      <source>Tool shape is null</source>
      <translation>工具形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="154"/>
      <source>Unsupported boolean operation</source>
      <translation>不支援的布林運算</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="782"/>
      <location filename="../../../App/FeaturePipe.cpp" line="397"/>
      <location filename="../../../App/FeaturePipe.cpp" line="417"/>
      <location filename="../../../App/FeatureLoft.cpp" line="258"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="127"/>
      <source>Resulting shape is not a solid</source>
      <translation>產成形狀不是固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="786"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="797"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="804"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="171"/>
      <location filename="../../../App/FeaturePipe.cpp" line="401"/>
      <location filename="../../../App/FeaturePipe.cpp" line="421"/>
      <location filename="../../../App/FeatureDraft.cpp" line="322"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="167"/>
      <location filename="../../../App/FeatureFillet.cpp" line="117"/>
      <location filename="../../../App/FeatureLoft.cpp" line="263"/>
      <location filename="../../../App/FeatureHole.cpp" line="1956"/>
      <location filename="../../../App/FeatureGroove.cpp" line="193"/>
      <source>Result has multiple solids: that is not currently supported.</source>
      <translation>產生形狀有多重(非相連)固體：目前尚未支援。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="153"/>
      <source>Failed to create chamfer</source>
      <translation>建立倒角失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="319"/>
      <location filename="../../../App/FeatureFillet.cpp" line="98"/>
      <source>Resulting shape is null</source>
      <translation>產成形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Resulting shape is invalid</source>
      <translation>產生形狀無效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="124"/>
      <source>No edges specified</source>
      <translation>未指定邊緣</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="240"/>
      <source>Size must be greater than zero</source>
      <translation>尺寸必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="249"/>
      <source>Size2 must be greater than zero</source>
      <translation>尺寸2必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="254"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>角度必須大於 0 而且小於 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="315"/>
      <source>Failed to create draft</source>
      <translation>建立草稿失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="81"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>在被選的形狀上無法進行圓角處理</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="88"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>倒圓角半徑必須大於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="87"/>
      <source>Angle of groove too large</source>
      <translation>挖槽的角度太大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="91"/>
      <source>Angle of groove too small</source>
      <translation>挖槽的角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1673"/>
      <location filename="../../../App/FeatureGroove.cpp" line="110"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>要求之特徵無法建立，可能的原因有：
 - 作業中主體不包含一個基礎形狀，所以沒有可移除之材料；
 - 所選擇的草圖不屬於作業中主體。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="477"/>
      <source>Length too small</source>
      <translation>長度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="484"/>
      <source>Second length too small</source>
      <translation>第二長度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="518"/>
      <source>Failed to obtain profile shape</source>
      <translation>無法獲取輪廓形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="570"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>建立失敗，因為方向與草圖的法向量正交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="628"/>
      <source>Extrude: Can only offset one face</source>
      <translation>拉伸：只能偏移一個面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="139"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="589"/>
      <location filename="../../../App/FeatureGroove.cpp" line="127"/>
      <source>Creating a face from sketch failed</source>
      <translation>由草圖建立面失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="668"/>
      <source>Up to face: Could not get SubShape!</source>
      <translation>“到面”操作：無法獲取子形狀！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="696"/>
      <source>Unable to reach the selected shape, please select faces</source>
      <translation>無法到達所選形狀，請選擇面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="723"/>
      <source>Magnitude of taper angle matches or exceeds 90 degrees</source>
      <translation>錐度角的大小等於或超過 90 度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="734"/>
      <source>Padding with draft angle failed</source>
      <translation>帶有斜角的填充操作失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="157"/>
      <location filename="../../../App/FeatureGroove.cpp" line="149"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>旋轉軸與草圖相交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="227"/>
      <location filename="../../../App/FeatureGroove.cpp" line="157"/>
      <source>Could not revolve the sketch!</source>
      <translation>無法旋轉草圖！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="239"/>
      <location filename="../../../App/FeatureGroove.cpp" line="201"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>無法從草圖建立面。
草圖中不允許有相交的草圖實體。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="128"/>
      <source>Error: Pitch too small!</source>
      <translation>錯誤：螺距太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="130"/>
      <location filename="../../../App/FeatureHelix.cpp" line="144"/>
      <source>Error: height too small!</source>
      <translation>錯誤：高度太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="136"/>
      <source>Error: pitch too small!</source>
      <translation>錯誤：螺距太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="138"/>
      <location filename="../../../App/FeatureHelix.cpp" line="146"/>
      <location filename="../../../App/FeatureHelix.cpp" line="152"/>
      <source>Error: turns too small!</source>
      <translation>錯誤：圈數太小!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="156"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>錯誤：高度或增長必須不為零！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="170"/>
      <source>Error: unsupported mode</source>
      <translation>錯誤：不支援的模式</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="182"/>
      <source>Error: No valid sketch or face</source>
      <translation>錯誤：沒有有效的草圖或面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="191"/>
      <source>Error: Face must be planar</source>
      <translation>錯誤：面必須為平面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2196"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="298"/>
      <source>Error: Result is not a solid</source>
      <translation>錯誤：產生形狀不是固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="246"/>
      <source>Error: There is nothing to subtract</source>
      <translation>錯誤：沒有東西可以減</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="250"/>
      <location filename="../../../App/FeatureHelix.cpp" line="272"/>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: Result has multiple solids</source>
      <translation>錯誤：產生形狀有多重(非相連)固體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="261"/>
      <source>Error: Adding the helix failed</source>
      <translation>錯誤：添加螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="285"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>錯誤：交叉螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="292"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>錯誤：減去螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="313"/>
      <source>Error: Could not create face from sketch</source>
      <translation>錯誤：無法從草圖建立面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1691"/>
      <source>Hole error: Creating a face from sketch failed</source>
      <translation>挖孔錯誤：由草圖建立面失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1716"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>挖孔錯誤：不支援的長度規格</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>挖孔錯誤：無效的孔深</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1742"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>挖孔錯誤：無效斜角</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1763"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>挖孔錯誤：挖孔直徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1767"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>挖孔錯誤：孔的切割深度必須小於孔的深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1771"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>挖孔錯誤：孔的切割深度必須大於或等於零</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1793"/>
      <source>Hole error: Invalid countersink</source>
      <translation>挖孔錯誤：無效的埋頭孔</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1826"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>挖孔錯誤：無效的鑽尖角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1836"/>
      <source>Hole error: Invalid drill point</source>
      <translation>挖孔錯誤：無效的鑽尖</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1870"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>挖孔錯誤：無法旋轉草圖</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1874"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>挖孔錯誤：結果形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1884"/>
      <source>Error: Adding the thread failed</source>
      <translation>錯誤：添加螺旋失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1935"/>
      <location filename="../../../App/FeatureHole.cpp" line="1940"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>布林運算在輪廓邊緣失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1946"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>布林運算在輪廓邊緣產生了非實體</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="160"/>
      <source>Boolean operation failed</source>
      <translation>布林運算失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1966"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>無法從草圖建立面。草圖實體的交叉或草圖中存在多個面不允許製作一個沖孔至一個面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2076"/>
      <source>Thread type out of range</source>
      <translation>螺紋類型超出範圍</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2079"/>
      <source>Thread size out of range</source>
      <translation>螺紋尺寸超出範圍</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2171"/>
      <source>Error: Thread could not be built</source>
      <translation>錯誤：無法建立螺紋</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="141"/>
      <source>Loft: At least one section is needed</source>
      <translation>拉伸成形：至少需要一個輪廊</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="275"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>拉伸成形：在建立拉伸成形時發生致命錯誤</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="178"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Loft 操作：從草圖建立面失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="211"/>
      <source>Loft: Failed to create shell</source>
      <translation>Loft 操作：建立外殼失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="816"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>無法從草圖立建面。
草圖中不允許有相交的實體或多個面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="176"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>管件：無法獲得輪廊形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="181"/>
      <source>No spine linked</source>
      <translation>沒有連結到 spline 曲線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="194"/>
      <source>No auxiliary spine linked.</source>
      <translation>沒有連結到輔助 spline 曲線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="215"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>管件：在使用具有孤立點作為剖面的草圖時，只需要一個孤立點。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="221"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>管件：當使用一個點給輪廊時，至少需要一個剖面</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="235"/>
      <source>Pipe: All sections need to be part features</source>
      <translation>管件：所有剖面必須為零件特徵</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="241"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>管件：無法獲得剖面形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="250"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>管件：只有輪廊及最後一個斷面可以為頂點</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="259"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>多重剖面需要有與基本剖面相同數量的內部線</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="286"/>
      <source>Path must not be a null shape</source>
      <translation>路徑不能為空形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="321"/>
      <source>Pipe could not be built</source>
      <translation>無法建立管件</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="366"/>
      <source>Result is not a solid</source>
      <translation>產生形狀不是實心物體</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="381"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>管件：沒有東西可以減</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="392"/>
      <source>Adding the pipe failed</source>
      <translation>添加管件失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="412"/>
      <source>Subtracting the pipe failed</source>
      <translation>管件減去操作失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>在建立管件時發生致命錯誤</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="559"/>
      <source>Invalid element in spine.</source>
      <translation>Spline 曲線中有無效元件。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="562"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Spline 曲線中的元件既不是一個邊也不是一條線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="575"/>
      <source>Spine is not connected.</source>
      <translation>Spline 曲線未連接</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="579"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Spline 曲線不是一個邊也不是一條線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="583"/>
      <source>Invalid spine.</source>
      <translation>無效的 spline 曲線。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="95"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>無法在沒有基礎特徵的情況下進行原始特徵的減法操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="247"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="113"/>
      <source>Unknown operation type</source>
      <translation>未知的運算類型</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="253"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="120"/>
      <source>Failed to perform boolean operation</source>
      <translation>執行布林運算失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="197"/>
      <source>Length of box too small</source>
      <translation>方塊長度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="199"/>
      <source>Width of box too small</source>
      <translation>方塊寬度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="201"/>
      <source>Height of box too small</source>
      <translation>方塊高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="247"/>
      <source>Radius of cylinder too small</source>
      <translation>圓柱的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="249"/>
      <source>Height of cylinder too small</source>
      <translation>圓柱的高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="251"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>圓柱的旋轉角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="304"/>
      <source>Radius of sphere too small</source>
      <translation>球體半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="353"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="355"/>
      <source>Radius of cone cannot be negative</source>
      <translation>錐體半徑不能為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="357"/>
      <source>Height of cone too small</source>
      <translation>錐體高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="420"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="422"/>
      <source>Radius of ellipsoid too small</source>
      <translation>橢球體的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="504"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="506"/>
      <source>Radius of torus too small</source>
      <translation>環形圓(甜甜圈狀)的半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="569"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>角柱體的多邊形為無效的，必須具有3個或更多個邊</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="571"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>多邊形或角柱體的外接圓半徑太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="573"/>
      <source>Height of prism is too small</source>
      <translation>角柱體的高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="654"/>
      <source>delta x of wedge too small</source>
      <translation>楔形體的 x 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="657"/>
      <source>delta y of wedge too small</source>
      <translation>楔形體的 y 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="660"/>
      <source>delta z of wedge too small</source>
      <translation>楔形體的 z 差量太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="663"/>
      <source>delta z2 of wedge is negative</source>
      <translation>楔形體的 z2 差量為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="666"/>
      <source>delta x2 of wedge is negative</source>
      <translation>楔形體的 x2 差量為負值</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="88"/>
      <source>Angle of revolution too large</source>
      <translation>旋轉角度太大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="94"/>
      <source>Angle of revolution too small</source>
      <translation>旋轉角度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="132"/>
      <source>Reference axis is invalid</source>
      <translation>參考軸是無效的</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="775"/>
      <source>Fusion with base feature failed</source>
      <translation>與基本特徵進行聯集失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>轉換特徵連結物件不是零件物件</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="117"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>沒有與變換特徵相關聯的原始特徵。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="277"/>
      <source>Cannot transform invalid support shape</source>
      <translation>無法轉換無效支援形狀</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="323"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>添加/除料特徵的形狀為空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="315"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>只有添加與除料特徵可以被轉換</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="101"/>
      <source>Invalid face reference</source>
      <translation>無效的面參考</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="137"/>
      <source>Active body</source>
      <translation>作業中主體</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2384"/>
      <source>Create datum</source>
      <translation>建立基準</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2385"/>
      <source>Create a datum object or local coordinate system</source>
      <translation>建立一個基準物件或是局部座標系統</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2419"/>
      <source>Create datum</source>
      <translation>建立基準</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2420"/>
      <source>Create a datum object or local coordinate system</source>
      <translation>建立一個基準物件或是局部座標系統</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="701"/>
      <source>Revolution parameters</source>
      <translation>旋轉成形參數</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="708"/>
      <source>Groove parameters</source>
      <translation type="unfinished">Groove parameters</translation>
    </message>
  </context>
</TS>
