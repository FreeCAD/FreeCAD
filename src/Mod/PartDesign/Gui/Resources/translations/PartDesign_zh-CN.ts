<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-CN" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="80"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>螺旋线开始的中心点; 源自参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>螺旋方向；派生自参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>The reference axis of the helix.</source>
      <translation>螺旋参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>螺旋线输入模式指定了那些需要用户设置的属性。
然后计算依赖的属性。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="116"/>
      <source>The axial distance between two turns.</source>
      <translation>两圈之间的参考轴方向的距离。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="123"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>螺旋线路径的高度，不计入剖面的大小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="133"/>
      <source>The number of turns in the helix.</source>
      <translation>螺旋线的圈数</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="141"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>围绕螺旋线形成包络面的圆锥角度。
非零值会将螺旋线转换为锥状螺旋。
正值使半径增大，负值使半径缩小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="154"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>螺旋线每圈的半径的增长。
非零值将螺旋线变成锥形螺旋。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>将转向方向设置为左行，
即沿其轴移动时逆时针。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="176"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>确定螺旋点是否位于轴的相反方向。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="186"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>如果设置，如果设定，结果将是轮廓与先存体的交集。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="196"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>如果是假的，该工具将基于轮廓边界框为螺距提出初始值，，从而避免自相交。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="208"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>螺旋的结合公差，如果螺旋形状不能与零件很好地结合，则增加。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="106"/>
      <source>Number of gear teeth</source>
      <translation>齿数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="118"/>
      <source>Pressure angle of gear teeth</source>
      <translation>压力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="112"/>
      <source>Module of the gear</source>
      <translation>齿轮模数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=2 曲线有3个控制点，False=1 曲线有4个控制点。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="135"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=外齿轮，False=内齿轮</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="144"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>从节圆到齿顶的高度，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="153"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>从节圆到齿根的高度，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="162"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>齿根圆角半径，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="171"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>参考轮廓向外偏移的距离，以模数归一化。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1661"/>
      <source>Additive Helix</source>
      <translation>添加式螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1662"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>沿着螺旋线扫描选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1561"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1562"/>
      <source>Additive Loft</source>
      <translation>添加式放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1563"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>沿着路径放样选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1461"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1462"/>
      <source>Additive Pipe</source>
      <translation>添加式管道</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1463"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>沿着路径扫描选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>New Body</source>
      <translation>新建实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="92"/>
      <source>Creates a new body and activates it</source>
      <translation>创建一个新实体并激活它</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2576"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2577"/>
      <source>Boolean Operation</source>
      <translation>布尔运算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2578"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>对选定的对象和激活的实体应用布尔运算</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Local Coordinate System</source>
      <translation>局部坐标系</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new local coordinate system</source>
      <translation>创建一个新的局部坐标系</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1987"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1988"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1989"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>对选定的边或面应用倒角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="489"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="490"/>
      <source>Clone</source>
      <translation>克隆</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="491"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>将一个实体对象参数化地复制为一个新实体的基本特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Draft</source>
      <translation>拔模</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2018"/>
      <source>Applies a draft to the selected faces</source>
      <translation>对选定的面应用拔模</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>Duplicate &amp;Object</source>
      <translation>复制对象(&amp;O)</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>复制所选对象并将其添加到活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1959"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1960"/>
      <source>Fillet</source>
      <translation>圆角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1961"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>对选定的边或面应用圆角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1391"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1392"/>
      <source>Groove</source>
      <translation>挖槽</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1393"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>围绕一条线或一个轴线旋转选定的草图或轮廓，并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1285"/>
      <source>Hole</source>
      <translation>孔</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1287"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>在选定草图或轮廓中圆或圆弧的中心点处，在激活的实体上创建孔</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Line</source>
      <translation>基准线</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum line</source>
      <translation>创建一个新的基准线</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2271"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2272"/>
      <source>Linear Pattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2273"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>以线性阵列的方式，复制选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="385"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="386"/>
      <source>Migrate</source>
      <translation>迁移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="387"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>将文档迁移到现代零件设计工作流</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2214"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2215"/>
      <source>Mirror</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2216"/>
      <source>Mirrors the selected features or active body</source>
      <translation>镜像选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="821"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="822"/>
      <source>Move Object To…</source>
      <translation>移动对象到…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="823"/>
      <source>Moves the selected object to another body</source>
      <translation>移动选定对象到另一个实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1016"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1017"/>
      <source>Move Feature After…</source>
      <translation>向后移动特征…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1018"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>移动选中的特征到处于同一实体的其它特征后</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Set Tip</source>
      <translation>设置 Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>移动实体的标识到选定特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2445"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2446"/>
      <source>Multi-Transform</source>
      <translation>多变形</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2447"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>将多个转换应用到选定的特征或活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="573"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="574"/>
      <source>New Sketch</source>
      <translation>新建草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="575"/>
      <source>Creates a new sketch</source>
      <translation>创建一个新的草图</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1226"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>Pad</source>
      <translation>凸台</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>拉伸选定的草图或轮廓并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Datum Plane</source>
      <translation>基准面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Creates a new datum plane</source>
      <translation>创建一个新的基准面</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1256"/>
      <source>Pocket</source>
      <translation>凹坑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1257"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>拉伸选定的草图或轮廓并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="250"/>
      <source>Datum Point</source>
      <translation>基准点</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="251"/>
      <source>Creates a new datum point</source>
      <translation>创建一个新的基准点</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2340"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2341"/>
      <source>Polar Pattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2342"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>以环形阵列的方式，复制选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1329"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1330"/>
      <source>Revolve</source>
      <translation>旋转</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1331"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>围绕一条线或一个轴旋转选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2403"/>
      <source>Scale</source>
      <translation>缩放</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2404"/>
      <source>Scales the selected features or the active body</source>
      <translation>缩放选定的特征或活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="313"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="314"/>
      <source>Shape Binder</source>
      <translation>形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="315"/>
      <source>Creates a new shape binder</source>
      <translation>创建一个新的形状链接器</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="383"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="384"/>
      <source>Sub-Shape Binder</source>
      <translation>子形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="385"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>创建一个或多个对象的几何引用，允许其在实体内外使用。它跟踪相对位置，支持多种几何类型（实体、面、边、顶点），并可与同一文档或外部文档中的对象协作。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1744"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>Subtractive Helix</source>
      <translation>减料螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>沿螺旋线扫描选定的草图或轮廓，并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1611"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>Subtractive Loft</source>
      <translation>减料放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>沿路径放样选定的草图或轮廓，并从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1511"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1512"/>
      <source>Subtractive Pipe</source>
      <translation>减料管道</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1513"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>沿路径扫描选定的草图或轮廓，并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2087"/>
      <source>Thickness</source>
      <translation>抽壳</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2088"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>应用厚度并移除选定的面</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="74"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="75"/>
      <source>Additive Primitive</source>
      <translation>增料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>Creates an additive primitive</source>
      <translation>创建增料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Box</source>
      <translation>增料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Additive Cylinder</source>
      <translation>增料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="231"/>
      <source>Additive Sphere</source>
      <translation>增料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="240"/>
      <source>Additive Cone</source>
      <translation>增料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="246"/>
      <source>Additive Ellipsoid</source>
      <translation>增料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="252"/>
      <source>Additive Torus</source>
      <translation>增料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="258"/>
      <source>Additive Prism</source>
      <translation>增料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>Additive Wedge</source>
      <translation>增料楔形体</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="282"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="283"/>
      <source>Subtractive Primitive</source>
      <translation>减料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>Creates a subtractive primitive</source>
      <translation>创建一个减料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="398"/>
      <source>Subtractive Box</source>
      <translation>减料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="407"/>
      <source>Subtractive Cylinder</source>
      <translation>减料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="416"/>
      <source>Subtractive Sphere</source>
      <translation>减料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="425"/>
      <source>Subtractive Cone</source>
      <translation>减料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="431"/>
      <source>Subtractive Ellipsoid</source>
      <translation>减料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="437"/>
      <source>Subtractive Torus</source>
      <translation>减料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="443"/>
      <source>Subtractive Prism</source>
      <translation>减料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="449"/>
      <source>Subtractive Wedge</source>
      <translation>减料楔形体</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="335"/>
      <source>Edit Shape Binder</source>
      <translation>编辑形状绑定器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Create Shape Binder</source>
      <translation>创建形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="439"/>
      <source>Create Sub-Shape Binder</source>
      <translation>创建子形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="508"/>
      <source>Create Clone</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1110"/>
      <source>Make Copy</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2500"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>转换为多重变换特征</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="253"/>
      <source>Sketch on Face</source>
      <translation>在面上创建草图</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="314"/>
      <source>Make copy</source>
      <translation>制作副本</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="516"/>
      <location filename="../../SketchWorkflow.cpp" line="772"/>
      <source>New Sketch</source>
      <translation>新建草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2597"/>
      <source>Create Boolean</source>
      <translation>创建布尔变量</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="222"/>
      <location filename="../../DlgActiveBody.cpp" line="101"/>
      <source>Add a Body</source>
      <translation>添加实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>迁移旧版零件设计特征到实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="769"/>
      <source>Duplicate a Part Design object</source>
      <translation>复制零件设计对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1110"/>
      <source>Move a feature inside body</source>
      <translation>移动特征到实体中</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="723"/>
      <source>Move tip to selected feature</source>
      <translation>将结算位置移至所选特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="926"/>
      <source>Move an object</source>
      <translation>移动一个对象</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="258"/>
      <source>Mirror</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="298"/>
      <source>Linear Pattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="347"/>
      <source>Polar Pattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
      <source>Scale</source>
      <translation>缩放</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Face Tools</source>
      <translation>面工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Edge Tools</source>
      <translation>边缘工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Boolean Tools</source>
      <translation>布尔工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Helper Tools</source>
      <translation>助手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Modeling Tools</source>
      <translation>建模工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Create Geometry</source>
      <translation>创建几何元素</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>渐开线参数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>齿数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>模数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>压力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>高精度</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>真</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>假</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>外齿轮</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>齿顶高系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>齿根高系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>齿根圆角系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>齿形变位系数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>需要激活状态的实体</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>要创建新的零件设计对象，文档中必须有一个激活的实体。
从下方选择一个实体，或创建一个新实体。</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>创建新实体</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="52"/>
      <source>Please select</source>
      <translation>请选择</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>几何图元</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>第一方向的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>第二方向的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>长度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>宽度</translation>
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
      <translation>半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>旋转角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>半径 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>半径 2</translation>
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
      <translation>U 参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>V 参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>本地z方向的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>局部 X 方向的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>半径 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>局部 Y 方向的半径
如果为零，则等于半径 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>V 参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>局部 XY 平面的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>局部 XZ 平面的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>多边形</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>外接圆半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X 最小/最大</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y 最小/最大</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z 最小/最大</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 最小/最大</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 最小/最大</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>节距</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>坐标系</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>增长</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>旋转次数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>角度 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>角度 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>通过 3 点定义</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>主半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>次半径</translation>
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
      <translation>右手</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>左手</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>起点</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>终点</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>参考</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>选择的几何体不是活动的实体的一部分。请明确如何处理这些选择。如果放弃编辑，请取消指令。</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>创建独立副本 (推荐)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>创建关联副本</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>创建交叉引用</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="285"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>选择操作会导致循环引用。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>添加实体</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>移除实体</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>结合</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>剪切</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>交集</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="51"/>
      <source>Boolean Parameters</source>
      <translation>布尔参数</translation>
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
      <translation>图元参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="940"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="948"/>
      <source>Invalid wedge parameters</source>
      <translation>无效的请求参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>X min must not be equal to X max!</source>
      <translation>X最小值不能等于X最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="941"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y 最小值不能等于Y 最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="949"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z最小值不能等于Z最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="991"/>
      <source>Create primitive</source>
      <translation>创建图元</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>在选择和预览模式之间切换</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 选择一个项目以高亮显示
- 双击一个项目以查看其倒角</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>等距：</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>两个距离</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>距离和角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>翻转方向</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>使用所有边</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>大小</translation>
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
      <translation>没有倒角被创建！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>Empty body list</source>
      <translation>空的实体列表</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>The body list cannot be empty</source>
      <translation>实体列表不能空</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="399"/>
      <source>Boolean: Accept: Input error</source>
      <translation>布尔值： 接受： 输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>Incompatible Reference Set</source>
      <translation>不兼容的引用集</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>没有适合当前参考集的附着模式。如果您选择继续，特征将保持现有状态，且将被定义为参照更改而不被移动。要继续吗？</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="228"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</source>
      <translation>无法使用给定的参数创建该特征。
几何图形可能无效或参数可能不兼容。
请调整参数并重试。</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="235"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="440"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>在选区和预览模式中切换</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看草稿</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>拔模角度</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>中性面</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>反转拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="288"/>
      <source>Empty draft created!
</source>
      <translation>空拔模已创建！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="298"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <source>Confirm Selection</source>
      <translation>确认选择</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="316"/>
      <source>Add All Edges</source>
      <translation>添加所有边</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="322"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>将所有边添加到列表框（仅在添加选择模式下）</translation>
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
      <translation>未选择任何面</translation>
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
      <translation>预览</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="356"/>
      <source>Select Faces</source>
      <translation>选择面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="692"/>
      <source>Select reference…</source>
      <translation>选择参考…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="602"/>
      <source>No shape selected</source>
      <translation>无选定的形状</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="685"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="688"/>
      <source>Face normal</source>
      <translation>面法线</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="696"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1088"/>
      <source>Click on a shape in the model</source>
      <translation>点击模型中的形状</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1359"/>
      <source>One sided</source>
      <translation>单侧</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>Two sided</source>
      <translation>双侧</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>Symmetric</source>
      <translation>对称</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1367"/>
      <source>Click on a face in the model</source>
      <translation>点击模型中的一个面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>允许已被使用的特征</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>允许外部特征</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>从相同零件的其他实体</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>来自不同的零件或自由特征</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>创建独立副本 (推荐)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>创建依赖副本</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>创建交叉引用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Invalid shape</source>
      <translation>无效形状</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>No wire in sketch</source>
      <translation>草图中找不到线框</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Sketch already used by other feature</source>
      <translation>草图被其他特征使用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Belongs to another body</source>
      <translation>属于另一个实体</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another part</source>
      <translation>属于另一个零件</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Doesn't belong to any body</source>
      <translation>不属于任何实体</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Base plane</source>
      <translation>基准平面</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Feature is located after the tip of the body</source>
      <translation>特征位于实体的末端之后</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Select attachment</source>
      <translation>选择附件</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>在选择和预览模式之间切换</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看这些文件</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>使用所有边</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="203"/>
      <source>Empty fillet created!</source>
      <translation>空圆角已创建！</translation>
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
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="240"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base Z-axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="222"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
      <source>Normal sketch axis</source>
      <translation type="unfinished">Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>状态</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>轴线</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="206"/>
      <source>Select reference…</source>
      <translation>选择参考…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>节距-高度-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>节距-圈数-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>高-圈数-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>高-圈数-增长</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>节距</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>高度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>圈数</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>圆锥角</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>径向增长</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>更改时重新计算</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>左旋</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>删除配置之外的文件</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="55"/>
      <source>Helix Parameters</source>
      <translation>螺旋参数</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="293"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告：螺旋可能是自交的</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="298"/>
      <source>Error: helix touches itself</source>
      <translation>错误: 螺旋自相交</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="347"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterbore</source>
      <translation>沉孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="56"/>
      <source>Countersink</source>
      <translation>埋头孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterdrill</source>
      <translation>沉头钻</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Hole Parameters</source>
      <translation>孔参数</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>None</source>
      <translation>无</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric regular</source>
      <translation>ISO 米制普通螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>ISO metric fine</source>
      <translation>ISO 米制细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS coarse</source>
      <translation>UTS 统一粗牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS fine</source>
      <translation>UTS 统一细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS extra fine</source>
      <translation>UTS 统一超细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ANSI pipes</source>
      <translation>ANSI 美标管螺纹标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP 国际/英标管螺纹标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSW whitworth</source>
      <translation>BSW 惠氏螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>BSF whitworth fine</source>
      <translation>BSF 惠氏细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>ISO tyre valves</source>
      <translation>ISO 轮胎气门嘴</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>中</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="682"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>精细</translation>
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
      <translation>法向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="696"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="700"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>宽松</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="704"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>法向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="705"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="706"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>宽度</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>直纹面</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>轮廓</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>对象</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>添加截面</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>删除截面</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>列表可以通过拖动重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation>更改时重新计算</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="48"/>
      <source>Loft Parameters</source>
      <translation>放样参数</translation>
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
      <translation>错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>变换</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>确定</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="69"/>
      <source>Edit</source>
      <translation>编辑</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="72"/>
      <source>Delete</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="75"/>
      <source>Add Mirror Transformation</source>
      <translation>添加镜像变换</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add Linear Pattern</source>
      <translation>添加线性阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="91"/>
      <source>Add Polar Pattern</source>
      <translation>添加极轴阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="99"/>
      <source>Add Scale Transformation</source>
      <translation>添加缩放变换</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Move Up</source>
      <translation>上移</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="105"/>
      <source>Move Down</source>
      <translation>下移</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="137"/>
      <source>Right-click to add a transformation</source>
      <translation>右键单击以添加变换</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="38"/>
      <source>Pad Parameters</source>
      <translation>凸台参数</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>从垫片将在侧面 1 结束的面偏移垫片</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="41"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>从垫片将在侧面 2 结束的面偏移垫片</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Reverses pad direction</source>
      <translation>反转凸台方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To last</source>
      <translation>直到最后</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>直到表面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to shape</source>
      <translation>上至形状</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>长度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>相对于面偏移</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>选取所有面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation>选择面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>侧面 2</translation>
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
      <translation>设置一个方向或从模型中选择边
作为参考值</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>否则，请将自定义向量用于凸台方向
将使用草图平面的法向量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>如果不选中，长度将按照指定的方向进行测量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>沿草图法线长度：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>在选择和预览模式之间切换</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation>方向/边缘</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation>选择参考…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation>方向向量的 X 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation>方向向量的 Y 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation>方向向量的 Z 分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>倾斜拉伸的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation>侧面 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>锥度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>选择形状</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>选择形状的所有面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation>更改时重新计算</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>方向模式</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>标准</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>辅助</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>副法线</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>曲线等效性</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>轮廓</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>对象</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>添加边</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>删除边</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>设置用于计算轮廓方向的常量副法向量</translation>
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
      <translation>截面方向</translation>
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
      <translation>轮廓</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>对象</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>拐角过渡</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>右拐角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>圆角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>扫掠路径</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>添加边缘</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>移除边缘</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>变换</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="67"/>
      <source>Pipe Parameters</source>
      <translation>管道参数</translation>
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
      <translation>输入错误</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <source>No active body</source>
      <translation>没有活动的实体</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>变换模式</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>常量</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>多截面</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>添加截面</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>删除截面</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>列表可以通过拖动重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="870"/>
      <source>Section Transformation</source>
      <translation>截面变换</translation>
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
      <translation>凹槽参数</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="41"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>从选定的面偏移，口袋将在侧面 1 结束</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>从选定的面偏移，口袋将在侧面 2 结束</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Reverses pocket direction</source>
      <translation>反转凹槽方向</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Up to face</source>
      <translation>直到面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>Up to shape</source>
      <translation>上至形状</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="254"/>
      <source>Base X-axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="255"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="256"/>
      <source>Base Z-axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>相当平面对称</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>第二角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>轴线</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="264"/>
      <source>Select reference…</source>
      <translation>选择参考…</translation>
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
      <translation>更改时重新计算</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="199"/>
      <source>To last</source>
      <translation>直到最后</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="204"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="215"/>
      <source>Up to face</source>
      <translation>直到面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="216"/>
      <source>Two angles</source>
      <translation>两个角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="479"/>
      <source>No face selected</source>
      <translation>未选择任何面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>缩放因子</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>出现次数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>对象</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>添加几何图形</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>移除几何图形</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Shape Binder Parameters</source>
      <translation>形状绑定器参数</translation>
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
      <translation>在选择和预览模式之间切换</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看这些特征</translation>
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
      <translation>表皮</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>管道</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>正反面</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>连接类型</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>圆弧</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>交集</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>厚度方向向里</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="267"/>
      <source>Empty thickness created!
</source>
      <translation>已创建空厚度！
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
      <translation>法向草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="442"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base XY-plane</source>
      <translation>基准 XY 平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base YZ-plane</source>
      <translation>基准 YZ 平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base XZ-plane</source>
      <translation>基准 XZ 平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="466"/>
      <source>Select reference…</source>
      <translation>选择参考…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>变换实体</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>变换工具图样形状</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>添加特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>移除特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation>更改时重新计算</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>列表可以通过拖动重新排序</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="908"/>
      <source>Select Body</source>
      <translation>选择实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="909"/>
      <source>Select a body from the list</source>
      <translation>从列表中选择实体</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1095"/>
      <source>Move Feature After…</source>
      <translation>向后移动特征…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1096"/>
      <source>Select a feature from the list</source>
      <translation>从列表中选择特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1183"/>
      <source>Move Tip</source>
      <translation>移动尖端</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>Set tip to last feature?</source>
      <translation>将尖端设置为最后一个特征？</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1184"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>被移动特征出现在当前设置的结算位置之后。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>Invalid selection</source>
      <translation>无效选择</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="147"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>没有适合选定对象的附着模式。请选择其他的东西。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="160"/>
      <location filename="../../Command.cpp" line="168"/>
      <location filename="../../Command.cpp" line="175"/>
      <source>Error</source>
      <translation>错误</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="809"/>
      <source>Several sub-elements selected</source>
      <translation>若干子元素被选择</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="810"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>选择单个面作为草图的支撑！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="817"/>
      <source>Select a face as support for a sketch!</source>
      <translation>选择一个面作为草图的支撑！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="824"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>需要一个平面作为草图的支撑！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="831"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>首先创建一个平面或选择一个面进行草图绘制</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="816"/>
      <source>No support face selected</source>
      <translation>未选中支持面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="823"/>
      <source>No planar support</source>
      <translation>无支持平面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="830"/>
      <source>No valid planes in this document</source>
      <translation>文档中无有效平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="257"/>
      <location filename="../../ViewProvider.cpp" line="135"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <location filename="../../Command.cpp" line="1138"/>
      <location filename="../../SketchWorkflow.cpp" line="728"/>
      <source>A dialog is already open in the task panel</source>
      <translation>一个对话框已在任务面板打开</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>无法使用此命令，因为没有可以减去的实体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="995"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>在尝试减料命令之前确保实体包含特征。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1019"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>无法使用所选对象。所选对象必须属于活动实体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>当前没有激活的实体。请在插入基准实体之前激活一个实体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="467"/>
      <source>Sub-shape binder</source>
      <translation>子形状绑定器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>No sketch to work on</source>
      <translation>没有可工作的草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1052"/>
      <source>No sketch is available in the document</source>
      <translation>文档无可用草图</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="258"/>
      <location filename="../../ViewProvider.cpp" line="136"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <location filename="../../Command.cpp" line="1139"/>
      <location filename="../../SketchWorkflow.cpp" line="729"/>
      <source>Close this dialog?</source>
      <translation>关闭此对话框？</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <location filename="../../Command.cpp" line="1856"/>
      <source>Wrong selection</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>从一单一实体中选择一边，面或体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1829"/>
      <location filename="../../Command.cpp" line="2191"/>
      <source>Selection is not in the active body</source>
      <translation>选择不在活动实体中</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1857"/>
      <source>Shape of the selected part is empty</source>
      <translation>所选零件的形状为空</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1830"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>从活动实体中选择边、面或体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1022"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>考虑使用形状绑定器或基准特征在实体中引用外部几何体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1843"/>
      <source>Wrong object type</source>
      <translation>错误的对象类型</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1844"/>
      <source>%1 works only on parts.</source>
      <translation>%1 仅能运作于零件上。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2192"/>
      <source>Please select only one feature in an active body.</source>
      <translation>请在一个活动的实体中仅选择一个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="71"/>
      <source>Part creation failed</source>
      <translation>零件创建失败</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="72"/>
      <source>Failed to create a part object.</source>
      <translation>创建零件对象失败。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="149"/>
      <location filename="../../CommandBody.cpp" line="215"/>
      <source>Bad base feature</source>
      <translation>不正确的基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="126"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>实体不能基于零件设计特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 已经属于一个实体，不能用作另一个实体的基准特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="150"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>基础特征 (%1) 录属于其他部件。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="177"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>所选形状仅由一个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="195"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体或壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="204"/>
      <source>Base feature</source>
      <translation>基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="216"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>实体基于的特征不能超过一个。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="230"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="421"/>
      <source>Nothing to migrate</source>
      <translation>没有可迁移的对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="692"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>请准确选择一个零件设计特征或一个实体。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="700"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>无法确定所选特征 '%s' 的实体。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Only features of a single source body can be moved</source>
      <translation>只能移动单一源实体的特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>草图平面不能被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="422"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>未找到没有实体的零件设计特征，无需迁移。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="617"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>请编辑 '%1'并使用基面或基准平面作为草绘平面来重新定义它。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <location filename="../../CommandBody.cpp" line="699"/>
      <location filename="../../CommandBody.cpp" line="711"/>
      <location filename="../../CommandBody.cpp" line="1061"/>
      <location filename="../../CommandBody.cpp" line="1071"/>
      <source>Selection error</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="712"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>只有实体特征才能成为实体的结算特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="846"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <location filename="../../CommandBody.cpp" line="894"/>
      <source>Features cannot be moved</source>
      <translation>特征无法被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>一些选定的特征依赖于源实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>There are no other bodies to move to</source>
      <translation>没有其他实体可以移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1062"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>无法移动实体的基础特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1072"/>
      <source>Select one or more features from the same body.</source>
      <translation>从同一实体上选择一个或多个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1087"/>
      <source>Beginning of the body</source>
      <translation>实体的起始</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1168"/>
      <source>Dependency violation</source>
      <translation>依赖冲突</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1169"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>较早的特征不能依赖于较后的特征。

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="307"/>
      <source>No previous feature found</source>
      <translation>未找到之前的特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="308"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>如果没有可用的基础特征, 就不可能创建减料特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="243"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="85"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>Active Body Required</source>
      <translation>需要激活状态的实体</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="148"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>要使用零件设计，文档中需要有一个激活的实体。激活一个实体（双击）或创建一个新实体。

对于缺少实体的零件设计对象的旧版文档，请使用零件设计中的迁移功能将它们放入实体中。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="207"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>要创建新的零件设计对象，文档中需要有一个激活的实体。激活一个现有实体（双击）或创建一个新实体。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="273"/>
      <source>Feature is not in a body</source>
      <translation>特征不在实体内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="274"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的实体对象。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="320"/>
      <source>Feature is not in a part</source>
      <translation>特征不在零件内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="321"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的零件对象。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="62"/>
      <location filename="../../ViewProviderTransformed.cpp" line="63"/>
      <location filename="../../ViewProvider.cpp" line="92"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="225"/>
      <source>Edit %1</source>
      <translation>编辑 %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="105"/>
      <source>Set Face Colors</source>
      <translation>设置面颜色</translation>
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
      <translation>线</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="122"/>
      <location filename="../../ViewProviderDatum.cpp" line="217"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="127"/>
      <source>Coordinate System</source>
      <translation>坐标系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="234"/>
      <source>Edit Datum</source>
      <translation>编辑基准</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="91"/>
      <source>Feature error</source>
      <translation>特征错误</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="92"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1 缺少基准特征。
此特征已损坏，无法编辑。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="220"/>
      <source>Edit Shape Binder</source>
      <translation>编辑形状绑定器</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="350"/>
      <source>Synchronize</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Select Bound Object</source>
      <translation>选择绑定对象</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="154"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>您正在编辑的文档 "%1" 是使用旧版本的零件设计工作台设计的。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>迁移以便使用现代零件设计功能？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="166"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>文档 "%1" 似乎要么处于从旧版零件设计迁移过程的中间，要么具有稍微损坏的结构。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="173"/>
      <source>Make the migration automatically?</source>
      <translation>自动进行迁移吗？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="176"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>注意：如果您选择迁移，您将无法使用旧版本的 FreeCAD 编辑该文件。
如果您拒绝迁移，您将无法使用新的零件设计功能，如实体和零部件。因此，您也将无法在装配工作台中使用您的零部件。
尽管您可以稍后随时使用“零件设计 -&gt; 迁移”进行迁移。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="189"/>
      <source>Migrate Manually</source>
      <translation>手动迁移</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="67"/>
      <source>Edit Boolean</source>
      <translation>编辑布尔运算</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="40"/>
      <source>Edit Chamfer</source>
      <translation>编辑倒角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="41"/>
      <source>Edit Draft</source>
      <translation>编辑放样</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="40"/>
      <source>Edit Fillet</source>
      <translation>编辑圆角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="43"/>
      <source>Edit Groove</source>
      <translation>编辑凹槽</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="48"/>
      <source>Edit Helix</source>
      <translation>编辑螺旋</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit Hole</source>
      <translation>编辑孔</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="38"/>
      <source>Edit Linear Pattern</source>
      <translation>编辑线性阵列</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="65"/>
      <source>Edit Loft</source>
      <translation>编辑放样</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="38"/>
      <source>Edit Mirror</source>
      <translation>编辑镜像</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="47"/>
      <source>Edit Multi-Transform</source>
      <translation>编辑多重变换</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="43"/>
      <source>Edit Pad</source>
      <translation>编辑凸台</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="75"/>
      <source>Edit Pipe</source>
      <translation>编辑管道</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="45"/>
      <source>Edit Pocket</source>
      <translation>编辑口袋</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation>编辑极轴阵列</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="49"/>
      <source>Edit Primitive</source>
      <translation>编辑基元</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="43"/>
      <source>Edit Revolution</source>
      <translation>编辑旋转体</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="38"/>
      <source>Edit Scale</source>
      <translation>编辑缩放</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="40"/>
      <source>Edit Thickness</source>
      <translation>编辑厚度</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>链轮参数</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>齿数</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>链轮参考</translation>
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
      <translation>带变速器的自行车</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>不带变速器的自行车</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>链距</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>链轮直径</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>齿宽</translation>
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
      <translation>摩托车 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>摩托车 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>摩托车 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>摩托车 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>摩托车 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>摩托车 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>摩托车 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 到</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>实时更新对线程的更改
注意计算可能需要一些时间</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>螺纹深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>自定义螺纹间隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>间隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>头部类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>深度类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>头部直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>头部深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation>间隙 / 通过孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation>攻丝钻（待攻丝）</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation>建模线程</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation>孔类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>更新螺纹视图</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>自定义间隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Thread clearance value</source>
      <translation>自定义螺纹间隙大小</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>大小</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>孔位
仅适用于无螺纹孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Standard</source>
      <translation>标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="732"/>
      <source>Close</source>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="737"/>
      <source>Wide</source>
      <translation>宽度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Class</source>
      <translation>类</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="835"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>根据孔配置方案螺纹孔的公差等级</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>孔直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>孔参数</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>基本轮廓类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>圆和圆弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>点、圆和圆弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>自定义头部值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>钻孔角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>包含在深度中</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>切换方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="662"/>
      <source>&lt;b&gt;Threading&lt;/b&gt;</source>
      <translation>&lt;b&gt;攻丝&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="783"/>
      <source>Thread</source>
      <translation>螺纹铣</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="892"/>
      <source>&amp;Right hand</source>
      <translation>右手(&amp;R)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="908"/>
      <source>&amp;Left hand</source>
      <translation>左手(&amp;L)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="957"/>
      <source>Thread Depth Type</source>
      <translation>螺纹深度类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="971"/>
      <source>Hole depth</source>
      <translation>孔深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="981"/>
      <source>Tapped (DIN76)</source>
      <translation>螺纹 (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>螺丝头的切割类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>检查以覆盖“类型”预定义的值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>对于沉头孔，这是螺钉顶部到沉头表面的深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>埋头孔角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>钻孔点的大小将被计入
盲孔的深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>锥孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>孔的锥度：
90度：直孔
小于90：底部的较小孔半径
大于90：底部的较大孔半径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>反转孔方向</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>无消息</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="41"/>
      <source>&amp;Sketch</source>
      <translation>草图(&amp;S)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Part Design</source>
      <translation>零件设计(&amp;P)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>Datums</source>
      <translation>基准</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Additive Features</source>
      <translation>增料特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Subtractive Features</source>
      <translation>减料特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Dress-Up Features</source>
      <translation>修整特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Transformation Features</source>
      <translation>变换特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Sprocket…</source>
      <translation>链轮…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Involute Gear</source>
      <translation>渐开线齿轮</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Shaft Design Wizard</source>
      <translation>轴设计向导</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Measure</source>
      <translation>测量</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Refresh</source>
      <translation>刷新</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Toggle 3D</source>
      <translation>切换3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Part Design Helper</source>
      <translation>零件设计助手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Modeling</source>
      <translation>零件设计建模</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Length [mm]</source>
      <translation>长度 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Diameter [mm]</source>
      <translation>直径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Inner diameter [mm]</source>
      <translation>内直径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Constraint type</source>
      <translation>约束类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge type</source>
      <translation>起始边缘类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Start edge size</source>
      <translation>起始边缘尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>End edge type</source>
      <translation>结束边缘类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>结束边缘大小</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="67"/>
      <source>Shaft Wizard</source>
      <translation>轴向导向</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="75"/>
      <source>Section 1</source>
      <translation>截面1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Section 2</source>
      <translation>截面2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="80"/>
      <source>Add column</source>
      <translation>添加列</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="128"/>
      <source>Section %s</source>
      <translation>截面 %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="157"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="176"/>
      <source>None</source>
      <translation>无</translation>
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
      <translation>轴承</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Gear</source>
      <translation>齿轮</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Pulley</source>
      <translation>滑轮</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="179"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="180"/>
      <source>Fillet</source>
      <translation>圆角</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="58"/>
      <source>All</source>
      <translation>全部</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="118"/>
      <source>Missing Module</source>
      <translation>缺少模块</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="124"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>未安装绘图附加组件。请安装它以启用此功能。</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="251"/>
      <source>Shaft design wizard...</source>
      <translation>轴设计向导...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="254"/>
      <source>Start the shaft design wizard</source>
      <translation>启动轴设计向导</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>链接对象不是 PartDesign 功能</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="412"/>
      <source>Tip shape is empty</source>
      <translation>提示形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="66"/>
      <source>BaseFeature link is not set</source>
      <translation>基础特征链接未设置</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="72"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>基础特征必须是 Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="82"/>
      <source>BaseFeature has an empty shape</source>
      <translation>基础特征有空形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="75"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>无基础特征时无法进行布尔剪切</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>除 Part::Feature 及其衍生外，无法进行布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="104"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>无法对无效的基础形状进行布尔操作</translation>
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
      <translation>结果有多个实体：在活动实体中启用“允许复合”</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="114"/>
      <source>Tool shape is null</source>
      <translation>工具形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="141"/>
      <source>Unsupported boolean operation</source>
      <translation>不支持的布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="351"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>无法创建总长度为零的垫片。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="356"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>无法创建总长度为零的口袋。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="704"/>
      <source>No extrusion geometry was generated.</source>
      <translation>未生成任何拉伸几何体。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="728"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>结果融合的拉伸为 null。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="368"/>
      <location filename="../../../App/FeaturePipe.cpp" line="521"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="139"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="764"/>
      <source>Resulting shape is not a solid</source>
      <translation>结果形状不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="172"/>
      <source>Failed to create chamfer</source>
      <translation>创建倒角失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="327"/>
      <location filename="../../../App/FeatureFillet.cpp" line="120"/>
      <source>Resulting shape is null</source>
      <translation>结果形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="141"/>
      <source>No edges specified</source>
      <translation>未指定边</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="293"/>
      <source>Size must be greater than zero</source>
      <translation>尺寸必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="304"/>
      <source>Size2 must be greater than zero</source>
      <translation>尺寸2 必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="311"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>角度必须大于 0 且小于 180 度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="95"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>选择的形状上无法进行圆角处理</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="103"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>圆角半径必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="157"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>圆角操作失败。选定的边可能包含无法同时进行圆角处理的几何图形。请尝试单独对边进行圆角处理，或使用更小的圆角半径。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>Angle of groove too large</source>
      <translation>沟槽角度过大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="108"/>
      <source>Angle of groove too small</source>
      <translation>沟槽角度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>无法创建请求的功能。 原因可能是：
  - 活动实体不包含基础形状， 因此没有
  材料可被删除；
  - 选中的草图不属于活动实体。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="400"/>
      <source>Failed to obtain profile shape</source>
      <translation>无法获取轮廓形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="454"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>创建失败，方向与草图的法线矢量正交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="176"/>
      <location filename="../../../App/FeatureGroove.cpp" line="154"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="477"/>
      <source>Creating a face from sketch failed</source>
      <translation>从草图创建面失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="115"/>
      <source>Angles of groove nullify each other</source>
      <translation>凹槽的角度相互抵消</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="193"/>
      <location filename="../../../App/FeatureGroove.cpp" line="171"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>旋转轴与草图相交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="294"/>
      <location filename="../../../App/FeatureGroove.cpp" line="263"/>
      <source>Could not revolve the sketch!</source>
      <translation>无法旋转草图！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="306"/>
      <location filename="../../../App/FeatureGroove.cpp" line="275"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>无法从草图中创建面。
不允许在草图中交叉实体。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="235"/>
      <source>Error: Pitch too small!</source>
      <translation>错误：节距太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="240"/>
      <location filename="../../../App/FeatureHelix.cpp" line="263"/>
      <source>Error: height too small!</source>
      <translation>错误：高度太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="249"/>
      <source>Error: pitch too small!</source>
      <translation>错误：节距太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="254"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="277"/>
      <source>Error: turns too small!</source>
      <translation>错误：圈数太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="283"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>错误：高度和增长率不能为零！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="315"/>
      <source>Error: No valid sketch or face</source>
      <translation>错误：没有有效的草图或面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="328"/>
      <source>Error: Face must be planar</source>
      <translation>错误：面必须是平面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2422"/>
      <location filename="../../../App/FeatureHelix.cpp" line="443"/>
      <location filename="../../../App/FeatureHelix.cpp" line="484"/>
      <source>Error: Result is not a solid</source>
      <translation>错误：结果不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="413"/>
      <source>Error: There is nothing to subtract</source>
      <translation>错误: 没有可减少的内容</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="449"/>
      <location filename="../../../App/FeatureHelix.cpp" line="490"/>
      <source>Error: Result has multiple solids</source>
      <translation>错误：结果有多个实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="434"/>
      <source>Error: Adding the helix failed</source>
      <translation>错误：添加螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>错误：交叉螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="475"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>错误：减去螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="506"/>
      <source>Error: Could not create face from sketch</source>
      <translation>错误：无法从草图创建面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1224"/>
      <source>Thread type is invalid</source>
      <translation>螺纹类型无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1764"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>孔错误：不支持的长度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1770"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>孔错误：无效的孔深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1796"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>孔错误：无效斜角</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1820"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>孔错误：挖孔直径太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1825"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>孔错误：孔切割深度必须小于孔深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1832"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>孔错误：孔切割深度必须大于等于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1862"/>
      <source>Hole error: Invalid countersink</source>
      <translation>孔错误：无效的埋头孔</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1898"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>孔错误：无效的钻尖角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1915"/>
      <source>Hole error: Invalid drill point</source>
      <translation>孔错误：钻点无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1952"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>孔错误：无法旋转草图</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1959"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>孔错误：结果形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1972"/>
      <source>Error: Adding the thread failed</source>
      <translation>错误：添加螺纹失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Finding axis failed</source>
      <translation>孔错误：查找轴失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2039"/>
      <location filename="../../../App/FeatureHole.cpp" line="2047"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>轮廓边缘布尔运算失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>在轮廓边缘的布尔运算产生了非实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="151"/>
      <source>Boolean operation failed</source>
      <translation>布尔操作失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2080"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>无法从草图创建面。不允许使用相交的草图实体、或草图中的多个面来制作凹槽。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2245"/>
      <source>Thread type out of range</source>
      <translation>螺线类型超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2248"/>
      <source>Thread size out of range</source>
      <translation>螺线大小超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2396"/>
      <source>Error: Thread could not be built</source>
      <translation>错误：无法构建螺纹</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="191"/>
      <source>Loft: At least one section is needed</source>
      <translation>拉伸：至少需要一个轮廊</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="392"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>拉伸：在拉伸时发生致命错误</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="238"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>拉伸：从草图创建面失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePipe.cpp" line="444"/>
      <source>Loft: Failed to create shell</source>
      <translation>拉伸：创建外壳失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="817"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>无法从草图立建面。
草图中不允许有相交的实体或多个面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="203"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>管道：无法获取轮廓形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="210"/>
      <source>No spine linked</source>
      <translation>没链接到骨架</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="225"/>
      <source>No auxiliary spine linked.</source>
      <translation>没链接到辅助骨架。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="248"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>管道：在使用带有孤立点的草图作为截面时，只需要一个孤立点。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="257"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>管道：当使用单点轮廓时，至少需要一个截面</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>管道：所有截面都需要是零件特征</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="283"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>管道：无法获取截面形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="293"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>管道：只有轮廓和最后一个截面可以作为顶点</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="306"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>多重截面需要有与基本截面相同数量的内部线</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="339"/>
      <source>Path must not be a null shape</source>
      <translation>路径不能是空形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="379"/>
      <source>Pipe could not be built</source>
      <translation>无法构建管道</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>Result is not a solid</source>
      <translation>结果不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="475"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>错误: 没有可减少的内容</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="543"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>制作管道时发生致命错误</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="672"/>
      <source>Invalid element in spine.</source>
      <translation>骨架中有无效元素。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="677"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>骨架中的元素既不是边线也不是连线。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Spine is not connected.</source>
      <translation>骨架未连接</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="704"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>骨架既不是边线也不是连线。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="709"/>
      <source>Invalid spine.</source>
      <translation>无效骨架。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="101"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>没有基础特征时无法减去原始特征</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="353"/>
      <location filename="../../../App/FeaturePipe.cpp" line="505"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Unknown operation type</source>
      <translation>未知操作类型</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="361"/>
      <location filename="../../../App/FeaturePipe.cpp" line="513"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="131"/>
      <source>Failed to perform boolean operation</source>
      <translation>执行布尔操作失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="215"/>
      <source>Length of box too small</source>
      <translation>方块长度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="220"/>
      <source>Width of box too small</source>
      <translation>方块宽度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="225"/>
      <source>Height of box too small</source>
      <translation>方块高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="273"/>
      <source>Radius of cylinder too small</source>
      <translation>圆柱半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="278"/>
      <source>Height of cylinder too small</source>
      <translation>圆柱高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="283"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>圆柱旋转角度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="340"/>
      <source>Radius of sphere too small</source>
      <translation>球体半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="392"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="397"/>
      <source>Radius of cone cannot be negative</source>
      <translation>锥体半径不能为负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="402"/>
      <source>Height of cone too small</source>
      <translation>锥体高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="482"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="487"/>
      <source>Radius of ellipsoid too small</source>
      <translation>椭球半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="581"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="586"/>
      <source>Radius of torus too small</source>
      <translation>环面半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="671"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>棱柱的多边形无效，必须至少有 3 条或以上的边</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="676"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>棱柱多边形的外接圆半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="681"/>
      <source>Height of prism is too small</source>
      <translation>棱柱高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="768"/>
      <source>delta x of wedge too small</source>
      <translation>楔形的 X 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="774"/>
      <source>delta y of wedge too small</source>
      <translation>楔形的 Y 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="780"/>
      <source>delta z of wedge too small</source>
      <translation>楔形的 Z 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="786"/>
      <source>delta z2 of wedge is negative</source>
      <translation>楔形的 Z2 差是负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="792"/>
      <source>delta x2 of wedge is negative</source>
      <translation>楔形的 X2 差是负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="123"/>
      <source>Angle of revolution too large</source>
      <translation>旋转角过大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Angle of revolution too small</source>
      <translation>旋转角过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Angles of revolution nullify each other</source>
      <translation>旋转的角度相互抵消</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="168"/>
      <location filename="../../../App/FeatureGroove.cpp" line="146"/>
      <source>Reference axis is invalid</source>
      <translation>参考坐标轴无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="756"/>
      <source>Fusion with base feature failed</source>
      <translation>与基本特征联合失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="99"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>转换功能链接的不是零件对象</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="106"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>没有与变换特征链接的原始特征。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="346"/>
      <source>Cannot transform invalid support shape</source>
      <translation>无法变换无效的支持形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="397"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>添加/减料的特性形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="388"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>只能变换增料和减料特征</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="107"/>
      <source>Invalid face reference</source>
      <translation>无效的面参考</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="60"/>
      <source>Involute Gear</source>
      <translation>渐开线齿轮</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="64"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>创建或编辑渐开线齿轮定义</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="63"/>
      <source>Sprocket</source>
      <translation>链轮</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="67"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>创建或编辑链轮定义。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>显示最终结果</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>显示预览覆盖</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="48"/>
      <source>Preview</source>
      <translation>预览</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="223"/>
      <source>Shaft Design Wizard</source>
      <translation>轴设计向导</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="226"/>
      <source>Starts the shaft design wizard</source>
      <translation>启动轴设计向导</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>计算移除体积预览时出错：%1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="105"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>结果形状为空。这可能表示不会移除任何材料或模型存在问题。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2644"/>
      <source>Create Datum</source>
      <translation>创建基准</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2645"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>创建基准对象或局部坐标系</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2679"/>
      <source>Create Datum</source>
      <translation>创建基准</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2680"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>创建基准对象或局部坐标系</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>通过宽度、高度和长度创建加法盒</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>通过半径、高度和角度创建加法圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>通过半径和各种角度创建加法球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Creates an additive cone</source>
      <translation>创建加法锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Creates an additive ellipsoid</source>
      <translation>创建加法椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Creates an additive torus</source>
      <translation>创建加法环面</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Creates an additive prism</source>
      <translation>创建加法棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Creates an additive wedge</source>
      <translation>创建加法楔形体</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>通过宽度、高度和长度创建减法盒</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>通过半径、高度和角度创建减法圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>通过半径和各种角度创建减法球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Creates a subtractive cone</source>
      <translation>创建减法锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>创建减法椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Creates a subtractive torus</source>
      <translation>创建减法环面</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Creates a subtractive prism</source>
      <translation>创建减法棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Creates a subtractive wedge</source>
      <translation>创建减法楔形体</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1007"/>
      <source>Attachment</source>
      <translation>附着</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="835"/>
      <source>Revolution Parameters</source>
      <translation>旋转参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="845"/>
      <source>Groove Parameters</source>
      <translation>槽参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation>变换特征消息</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="122"/>
      <source>Active Body</source>
      <translation>活动实体</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="43"/>
      <source>Chamfer Parameters</source>
      <translation>倒角参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="113"/>
      <source>Datum Plane Parameters</source>
      <translation>基准面参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="118"/>
      <source>Datum Line Parameters</source>
      <translation>基准线参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="123"/>
      <source>Datum Point Parameters</source>
      <translation>基准点参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="128"/>
      <source>Local Coordinate System Parameters</source>
      <translation>局部坐标系参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="44"/>
      <source>Draft Parameters</source>
      <translation>放样参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="43"/>
      <source>Fillet Parameters</source>
      <translation>圆角参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="40"/>
      <source>Linear Pattern Parameters</source>
      <translation>线性阵列参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="40"/>
      <source>Mirror Parameters</source>
      <translation>镜像参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="40"/>
      <source>Multi-Transform Parameters</source>
      <translation>多重变换参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="40"/>
      <source>Polar Pattern Parameters</source>
      <translation>极轴阵列参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="40"/>
      <source>Scale Parameters</source>
      <translation>缩放参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="43"/>
      <source>Thickness Parameters</source>
      <translation>厚度参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="130"/>
      <source>Direction 2</source>
      <translation>方向 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="246"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>选择一个方向参考（边、面、基准线）</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="332"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>无效选择。选择一条边、一个平面或一条基准线。</translation>
    </message>
  </context>
</TS>
