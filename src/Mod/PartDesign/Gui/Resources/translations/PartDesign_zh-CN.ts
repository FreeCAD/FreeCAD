<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-CN" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="73"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>螺旋线开始的中心点; 源自参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="75"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>螺旋方向；派生自参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="77"/>
      <source>The reference axis of the helix.</source>
      <translation>螺旋参考轴。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="79"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>螺旋线输入模式指定了那些需要用户设置的属性。
然后计算依赖的属性。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="83"/>
      <source>The axial distance between two turns.</source>
      <translation>两圈之间的参考轴方向的距离。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="85"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>螺旋线路径的高度，不计入剖面的大小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="87"/>
      <source>The number of turns in the helix.</source>
      <translation>螺旋线的圈数</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>围绕螺旋线形成包络面的圆锥角度。
非零值会将螺旋线转换为锥状螺旋。
正值使半径增大，负值使半径缩小。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="95"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>螺旋线每圈的半径的增长。
非零值将螺旋线变成锥形螺旋。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="98"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>将转向方向设置为左行，
即沿其轴移动时逆时针。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="101"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>确定螺旋点是否位于轴的相反方向。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="103"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>如果设置，如果设定，结果将是轮廓与先存体的交集。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="105"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>如果是假的，该工具将基于轮廓边界框为螺距提出初始值，，从而避免自相交。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="108"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>螺旋的结合公差，如果螺旋形状不能与零件很好地结合，则增加。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="98"/>
      <source>Number of gear teeth</source>
      <translation>齿数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="104"/>
      <source>Pressure angle of gear teeth</source>
      <translation>压力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="101"/>
      <source>Module of the gear</source>
      <translation>齿轮模数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=2 曲线有3个控制点，False=1 曲线有4个控制点。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=外齿轮，False=内齿轮</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>从节圆到齿顶的高度，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>从节圆到齿根的高度，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>齿根圆角半径，以模数归一化。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>参考轮廓向外偏移的距离，以模数归一化。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1504"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1505"/>
      <source>Additive Helix</source>
      <translation>添加式螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1506"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>沿着螺旋线扫描选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1411"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1412"/>
      <source>Additive Loft</source>
      <translation>添加式放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1413"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>沿着路径放样选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1317"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1318"/>
      <source>Additive Pipe</source>
      <translation>添加式管道</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1319"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>沿着路径扫描选定的草图或轮廓，并将其添加到实体中</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="85"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="86"/>
      <source>New Body</source>
      <translation>新建实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>Creates a new body and activates it</source>
      <translation>创建一个新实体并激活它</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2315"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2316"/>
      <source>Boolean Operation</source>
      <translation>布尔运算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2317"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>对选定的对象和激活的实体应用布尔运算</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="245"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>Local Coordinate System</source>
      <translation>局部坐标系</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Creates a new local coordinate system</source>
      <translation>创建一个新的局部坐标系</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1784"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1785"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1786"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>对选定的边或面应用倒角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="427"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="428"/>
      <source>Clone</source>
      <translation>克隆</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>将一个实体对象参数化地复制为一个新实体的基本特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1813"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1814"/>
      <source>Draft</source>
      <translation>拔模</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1815"/>
      <source>Applies a draft to the selected faces</source>
      <translation>对选定的面应用拔模</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="619"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="620"/>
      <source>Duplicate &amp;Object</source>
      <translation>复制对象(&amp;O)</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="621"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>复制所选对象并将其添加到活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1756"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1757"/>
      <source>Fillet</source>
      <translation>圆角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1758"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>对选定的边或面应用圆角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1254"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>Groove</source>
      <translation>挖槽</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1256"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>围绕一条线或一个轴线旋转选定的草图或轮廓，并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1156"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1157"/>
      <source>Hole</source>
      <translation>孔</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1158"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>在选定草图或轮廓中圆或圆弧的中心点处，在激活的实体上创建孔</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Datum Line</source>
      <translation>基准线</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Creates a new datum line</source>
      <translation>创建一个新的基准线</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2049"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2050"/>
      <source>Linear Pattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2051"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>以线性阵列的方式，复制选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="318"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="319"/>
      <source>Migrate</source>
      <translation>迁移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="320"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>将文档迁移到现代零件设计工作流</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="1998"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1999"/>
      <source>Mirror</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2000"/>
      <source>Mirrors the selected features or active body</source>
      <translation>镜像选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="679"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="680"/>
      <source>Move Object To…</source>
      <translation>移动对象到…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="681"/>
      <source>Moves the selected object to another body</source>
      <translation>移动选定对象到另一个实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="846"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <source>Move Feature After…</source>
      <translation>向后移动特征…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="848"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>移动选中的特征到处于同一实体的其它特征后</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="540"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="541"/>
      <source>Set Tip</source>
      <translation>设置 Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="542"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>移动实体的标识到选定特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2200"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2201"/>
      <source>Multi-Transform</source>
      <translation>多变形</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2202"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>将多个转换应用到选定的特征或活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="509"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="510"/>
      <source>New Sketch</source>
      <translation>新建草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="511"/>
      <source>Creates a new sketch</source>
      <translation>创建一个新的草图</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1098"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1099"/>
      <source>Pad</source>
      <translation>凸台</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1100"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="162"/>
      <source>Datum Plane</source>
      <translation>基准面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>Creates a new datum plane</source>
      <translation>创建一个新的基准面</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1127"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1128"/>
      <source>Pocket</source>
      <translation>凹坑</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1129"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="217"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Datum Point</source>
      <translation>基准点</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>Creates a new datum point</source>
      <translation>创建一个新的基准点</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2103"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2104"/>
      <source>Polar Pattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2105"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>以环形阵列的方式，复制选定的特征或激活的实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1199"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1200"/>
      <source>Revolve</source>
      <translation>旋转</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1201"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation type="unfinished">Revolves the selected sketch or profile around a line or axis and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2158"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2159"/>
      <source>Scale</source>
      <translation>缩放</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2160"/>
      <source>Scales the selected features or the active body</source>
      <translation type="unfinished">Scales the selected features or the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="277"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>Shape Binder</source>
      <translation>形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>Creates a new shape binder</source>
      <translation>创建一个新的形状链接器</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="343"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="344"/>
      <source>Sub-Shape Binder</source>
      <translation>子形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation type="unfinished">Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1576"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1577"/>
      <source>Subtractive Helix</source>
      <translation>减料螺旋</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1578"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a helix and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1458"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1459"/>
      <source>Subtractive Loft</source>
      <translation>减料放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1460"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Lofts the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1364"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1365"/>
      <source>Subtractive Pipe</source>
      <translation>减料管道</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1366"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1881"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1882"/>
      <source>Thickness</source>
      <translation>抽壳</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1883"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation type="unfinished">Applies thickness and removes the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="66"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="67"/>
      <source>Additive Primitive</source>
      <translation>增料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>Creates an additive primitive</source>
      <translation>创建增料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="195"/>
      <source>Additive Box</source>
      <translation>增料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="199"/>
      <source>Additive Cylinder</source>
      <translation>增料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="203"/>
      <source>Additive Sphere</source>
      <translation>增料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="207"/>
      <source>Additive Cone</source>
      <translation>增料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="211"/>
      <source>Additive Ellipsoid</source>
      <translation>增料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Torus</source>
      <translation>增料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="219"/>
      <source>Additive Prism</source>
      <translation>增料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="223"/>
      <source>Additive Wedge</source>
      <translation>增料楔形体</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="239"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="240"/>
      <source>Subtractive Primitive</source>
      <translation>减料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="241"/>
      <source>Creates a subtractive primitive</source>
      <translation>创建一个减料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="347"/>
      <source>Subtractive Box</source>
      <translation>减料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="351"/>
      <source>Subtractive Cylinder</source>
      <translation>减料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="355"/>
      <source>Subtractive Sphere</source>
      <translation>减料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="359"/>
      <source>Subtractive Cone</source>
      <translation>减料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="363"/>
      <source>Subtractive Ellipsoid</source>
      <translation>减料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="367"/>
      <source>Subtractive Torus</source>
      <translation>减料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="371"/>
      <source>Subtractive Prism</source>
      <translation>减料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="375"/>
      <source>Subtractive Wedge</source>
      <translation>减料楔形体</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="298"/>
      <source>Edit Shape Binder</source>
      <translation>编辑形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Create Shape Binder</source>
      <translation>创建形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="390"/>
      <source>Create Sub-Shape Binder</source>
      <translation>创建子形状链接器</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="445"/>
      <source>Create Clone</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="999"/>
      <source>Make Copy</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2245"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>转换为多重变换特征</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="256"/>
      <source>Sketch on Face</source>
      <translation>在面上创建草图</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="315"/>
      <source>Make copy</source>
      <translation>制作副本</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="512"/>
      <location filename="../../SketchWorkflow.cpp" line="744"/>
      <source>New Sketch</source>
      <translation>新建草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2333"/>
      <source>Create Boolean</source>
      <translation>创建布尔变量</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="191"/>
      <location filename="../../DlgActiveBody.cpp" line="100"/>
      <source>Add a Body</source>
      <translation>添加实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="436"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation type="unfinished">Migrate legacy Part Design features to bodies</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="633"/>
      <source>Duplicate a Part Design object</source>
      <translation>复制零件设计对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="916"/>
      <source>Move a feature inside body</source>
      <translation>移动特征到实体中</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="588"/>
      <source>Move tip to selected feature</source>
      <translation>将结算位置移至所选特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="760"/>
      <source>Move an object</source>
      <translation>移动一个对象</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="262"/>
      <source>Mirror</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="299"/>
      <source>Linear Pattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="343"/>
      <source>Polar Pattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="380"/>
      <source>Scale</source>
      <translation>缩放</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Face Tools</source>
      <translation>面工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Edge Tools</source>
      <translation>边缘工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Boolean Tools</source>
      <translation>布尔工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Helper Tools</source>
      <translation>助手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Modeling Tools</source>
      <translation type="unfinished">Modeling Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Create Geometry</source>
      <translation>创建几何元素</translation>
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
      <translation>需要激活状态的实体</translation>
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
      <location filename="../../DlgActiveBody.cpp" line="53"/>
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
      <translation>本地z方向的半径</translation>
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
      <translation>多边形</translation>
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
      <translation>节距</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation type="unfinished">Coordinate system</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>增长</translation>
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
      <location filename="../../ReferenceSelection.cpp" line="272"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>选择操作会导致循环引用。</translation>
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
      <location filename="../../TaskBooleanParameters.cpp" line="52"/>
      <source>Boolean Parameters</source>
      <translation type="unfinished">Boolean Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="85"/>
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
      <location filename="../../TaskPrimitiveParameters.cpp" line="916"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="922"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="928"/>
      <source>Invalid wedge parameters</source>
      <translation>无效的请求参数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="917"/>
      <source>X min must not be equal to X max!</source>
      <translation>X最小值不能等于X最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="923"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y 最小值不能等于Y 最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="929"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z最小值不能等于Z最大值！</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="967"/>
      <source>Create primitive</source>
      <translation>创建图元</translation>
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
      <translation type="unfinished">Flips the direction</translation>
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
      <location filename="../../TaskChamferParameters.cpp" line="343"/>
      <source>Empty chamfer created!
</source>
      <translation>没有倒角被创建！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="386"/>
      <source>Empty body list</source>
      <translation>空的实体列表</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="387"/>
      <source>The body list cannot be empty</source>
      <translation>实体列表不能空</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="401"/>
      <source>Boolean: Accept: Input error</source>
      <translation>布尔值： 接受： 输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="99"/>
      <source>Incompatible Reference Set</source>
      <translation>不兼容的引用集</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="100"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>没有适合当前参考集的附着模式。如果您选择继续，特征将保持现有状态，且将被定义为参照更改而不被移动。要继续吗？</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="196"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="405"/>
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
      <translation>反转拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="287"/>
      <source>Empty draft created!
</source>
      <translation type="unfinished">Empty draft created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="269"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="274"/>
      <source>Confirm Selection</source>
      <translation type="unfinished">Confirm Selection</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="286"/>
      <source>Add All Edges</source>
      <translation type="unfinished">Add All Edges</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="291"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation type="unfinished">Adds all edges to the list box (only when in add selection mode)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="299"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1355"/>
      <source>No face selected</source>
      <translation>未选择任何面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="161"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1135"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="73"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="349"/>
      <source>Preview</source>
      <translation>预览</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="353"/>
      <source>Select Faces</source>
      <translation type="unfinished">Select Faces</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="688"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="596"/>
      <source>No shape selected</source>
      <translation>无选定的形状</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="681"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="684"/>
      <source>Face normal</source>
      <translation>面法线</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="692"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="697"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1081"/>
      <source>Click on a shape in the model</source>
      <translation>点击模型中的形状</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1342"/>
      <source>One sided</source>
      <translation type="unfinished">One sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1343"/>
      <source>Two sided</source>
      <translation type="unfinished">Two sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1344"/>
      <source>Symmetric</source>
      <translation>对称</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1350"/>
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
      <translation type="unfinished">Allow External Features</translation>
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
      <translation type="unfinished">Feature is located after the tip of the body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="93"/>
      <source>Select attachment</source>
      <translation>选择附件</translation>
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
      <location filename="../../TaskHelixParameters.cpp" line="242"/>
      <source>Base X-axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="244"/>
      <source>Base Z-axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="226"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="224"/>
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
      <location filename="../../TaskHelixParameters.cpp" line="209"/>
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
      <translation type="unfinished">Recompute on change</translation>
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
      <location filename="../../TaskHelixParameters.cpp" line="59"/>
      <source>Helix Parameters</source>
      <translation type="unfinished">Helix Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="228"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="294"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告：螺旋可能是自交的</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="299"/>
      <source>Error: helix touches itself</source>
      <translation>错误: 螺旋自相交</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="348"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="52"/>
      <source>Counterbore</source>
      <translation>沉孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Countersink</source>
      <translation>埋头孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="54"/>
      <source>Counterdrill</source>
      <translation>沉头钻</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="58"/>
      <source>Hole Parameters</source>
      <translation type="unfinished">Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>None</source>
      <translation>无</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>ISO metric regular</source>
      <translation>ISO 米制普通螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="69"/>
      <source>ISO metric fine</source>
      <translation>ISO 米制细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>UTS coarse</source>
      <translation>UTS 统一粗牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>UTS fine</source>
      <translation>UTS 统一细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>UTS extra fine</source>
      <translation>UTS 统一超细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>ANSI pipes</source>
      <translation>ANSI 美标管螺纹标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP 国际/英标管螺纹标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>BSW whitworth</source>
      <translation>BSW 惠氏螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>BSF whitworth fine</source>
      <translation>BSF 惠氏细牙螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO tyre valves</source>
      <translation type="unfinished">ISO tyre valves</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="671"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>中</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="672"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>精细</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="673"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>粗糙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="676"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>法向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="677"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>宽松</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="681"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>法向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="682"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="683"/>
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
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="49"/>
      <source>Loft Parameters</source>
      <translation type="unfinished">Loft Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="73"/>
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
      <location filename="../../TaskMultiTransformParameters.cpp" line="68"/>
      <source>Edit</source>
      <translation>编辑</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="74"/>
      <source>Delete</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="80"/>
      <source>Add Mirror Transformation</source>
      <translation type="unfinished">Add Mirror Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="86"/>
      <source>Add Linear Pattern</source>
      <translation type="unfinished">Add Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="92"/>
      <source>Add Polar Pattern</source>
      <translation type="unfinished">Add Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="98"/>
      <source>Add Scale Transformation</source>
      <translation type="unfinished">Add Scale Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="104"/>
      <source>Move Up</source>
      <translation type="unfinished">Move Up</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="107"/>
      <source>Move Down</source>
      <translation type="unfinished">Move Down</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="141"/>
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
      <location filename="../../TaskPadPocketParameters.ui" line="527"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>设置一个方向或从模型中选择边
作为参考值</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="532"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="542"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="552"/>
      <source>Show direction</source>
      <translation>显示方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="562"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>否则，请将自定义向量用于凸台方向
将使用草图平面的法向量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="671"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>如果不选中，长度将按照指定的方向进行测量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="675"/>
      <source>Length along sketch normal</source>
      <translation>沿草图法线长度：</translation>
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
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="520"/>
      <source>Direction/edge</source>
      <translation type="unfinished">Direction/edge</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="537"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="575"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X-component of direction vector</source>
      <translation type="unfinished">X-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="604"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y-component of direction vector</source>
      <translation type="unfinished">Y-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="633"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z-component of direction vector</source>
      <translation type="unfinished">Z-component of direction vector</translation>
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
      <translation type="unfinished">Side 1</translation>
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
      <translation type="unfinished">Select Shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation type="unfinished">Selects all faces of the shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="685"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="578"/>
      <source>Section Orientation</source>
      <translation type="unfinished">Section Orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="606"/>
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
      <translation>添加边缘</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation type="unfinished">Remove edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>变换</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="67"/>
      <source>Pipe Parameters</source>
      <translation type="unfinished">Pipe Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="87"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="446"/>
      <location filename="../../TaskPipeParameters.cpp" line="558"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="446"/>
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
      <location filename="../../TaskPipeParameters.cpp" line="877"/>
      <source>Section Transformation</source>
      <translation type="unfinished">Section Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="894"/>
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
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="41"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="42"/>
      <source>Reverses pocket direction</source>
      <translation>反转凹槽方向</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="72"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>直到面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="238"/>
      <source>Base X-axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="239"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="247"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="191"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="155"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="473"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="193"/>
      <source>To last</source>
      <translation>直到最后</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="196"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="198"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="199"/>
      <source>Up to face</source>
      <translation>直到面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="200"/>
      <source>Two angles</source>
      <translation type="unfinished">Two angles</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="460"/>
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
      <translation type="unfinished">Shape Binder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="129"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="187"/>
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
      <translation type="unfinished">Recto verso</translation>
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
      <translation type="unfinished">Empty thickness created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="103"/>
      <source>Remove</source>
      <translation>移除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Normal sketch axis</source>
      <translation>法向草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="401"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="400"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草图轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <location filename="../../TaskTransformedParameters.cpp" line="441"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="418"/>
      <source>Base X-axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="419"/>
      <source>Base Y-axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base Z-axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="455"/>
      <source>Base XY-plane</source>
      <translation type="unfinished">Base XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base YZ-plane</source>
      <translation type="unfinished">Base YZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base XZ-plane</source>
      <translation type="unfinished">Base XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="428"/>
      <location filename="../../TaskTransformedParameters.cpp" line="465"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
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
      <translation>列表可以通过拖动重新排序</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="749"/>
      <source>Select Body</source>
      <translation type="unfinished">Select Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="750"/>
      <source>Select a body from the list</source>
      <translation>从列表中选择实体</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="903"/>
      <source>Move Feature After…</source>
      <translation>向后移动特征…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="904"/>
      <source>Select a feature from the list</source>
      <translation>从列表中选择特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="978"/>
      <source>Move Tip</source>
      <translation type="unfinished">Move Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="980"/>
      <source>Set tip to last feature?</source>
      <translation type="unfinished">Set tip to last feature?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="979"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>被移动特征出现在当前设置的结算位置之后。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>Invalid selection</source>
      <translation>无效选择</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>没有适合选定对象的附着模式。请选择其他的东西。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <location filename="../../Command.cpp" line="147"/>
      <location filename="../../Command.cpp" line="149"/>
      <source>Error</source>
      <translation>错误</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="777"/>
      <source>Several sub-elements selected</source>
      <translation>若干子元素被选择</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="778"/>
      <source>Select a single face as support for a sketch!</source>
      <translation type="unfinished">Select a single face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="782"/>
      <source>Select a face as support for a sketch!</source>
      <translation type="unfinished">Select a face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="786"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation type="unfinished">Need a planar face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="790"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation type="unfinished">Create a plane first or select a face to sketch on</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="781"/>
      <source>No support face selected</source>
      <translation>未选中支持面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="785"/>
      <source>No planar support</source>
      <translation>无支持平面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="789"/>
      <source>No valid planes in this document</source>
      <translation>文档中无有效平面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1020"/>
      <location filename="../../SketchWorkflow.cpp" line="704"/>
      <location filename="../../ViewProvider.cpp" line="134"/>
      <location filename="../../ViewProviderDatum.cpp" line="248"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="94"/>
      <source>A dialog is already open in the task panel</source>
      <translation>一个对话框已在任务面板打开</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="900"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>无法使用此命令，因为没有可以减去的实体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="901"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>在尝试减料命令之前确保实体包含特征。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="922"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>无法使用所选对象。所选对象必须属于活动实体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation type="unfinished">There is no active body. Please activate a body before inserting a datum entity.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="408"/>
      <source>Sub-shape binder</source>
      <translation type="unfinished">Sub-shape binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="945"/>
      <source>No sketch to work on</source>
      <translation>没有可工作的草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="946"/>
      <source>No sketch is available in the document</source>
      <translation>文档无可用草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1021"/>
      <location filename="../../SketchWorkflow.cpp" line="705"/>
      <location filename="../../ViewProvider.cpp" line="135"/>
      <location filename="../../ViewProviderDatum.cpp" line="249"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <source>Close this dialog?</source>
      <translation type="unfinished">Close this dialog?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1639"/>
      <location filename="../../Command.cpp" line="1665"/>
      <source>Wrong selection</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1640"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>从一单一实体中选择一边，面或体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1644"/>
      <location filename="../../Command.cpp" line="1976"/>
      <source>Selection is not in the active body</source>
      <translation type="unfinished">Selection is not in the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1666"/>
      <source>Shape of the selected part is empty</source>
      <translation type="unfinished">Shape of the selected part is empty</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1645"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>从活动实体中选择边、面或体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="923"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation type="unfinished">Consider using a shape binder or a base feature to reference external geometry in a body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1655"/>
      <source>Wrong object type</source>
      <translation>错误的对象类型</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1656"/>
      <source>%1 works only on parts.</source>
      <translation>%1 仅能运作于零件上。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1977"/>
      <source>Please select only one feature in an active body.</source>
      <translation>请在一个活动的实体中仅选择一个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="67"/>
      <source>Part creation failed</source>
      <translation>零件创建失败</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="68"/>
      <source>Failed to create a part object.</source>
      <translation>创建零件对象失败。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="117"/>
      <location filename="../../CommandBody.cpp" line="122"/>
      <location filename="../../CommandBody.cpp" line="135"/>
      <location filename="../../CommandBody.cpp" line="184"/>
      <source>Bad base feature</source>
      <translation>不正确的基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="118"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation type="unfinished">A body cannot be based on a Part Design feature.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="123"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation type="unfinished">%1 already belongs to a body and cannot be used as a base feature for another body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="136"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>基础特征 (%1) 录属于其他部件。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="160"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="164"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="168"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>所选形状仅由一个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="172"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体或壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="177"/>
      <source>Base feature</source>
      <translation>基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="185"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>实体基于的特征不能超过一个。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="199"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="349"/>
      <source>Nothing to migrate</source>
      <translation>没有可迁移的对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="569"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation type="unfinished">Select exactly one Part Design feature or a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="573"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation type="unfinished">Could not determine a body for the selected feature '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="723"/>
      <source>Only features of a single source body can be moved</source>
      <translation type="unfinished">Only features of a single source body can be moved</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="505"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>草图平面不能被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="350"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation type="unfinished">No Part Design features without body found Nothing to migrate.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="506"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>请编辑 '%1'并使用基面或基准平面作为草绘平面来重新定义它。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="568"/>
      <location filename="../../CommandBody.cpp" line="572"/>
      <location filename="../../CommandBody.cpp" line="577"/>
      <location filename="../../CommandBody.cpp" line="874"/>
      <location filename="../../CommandBody.cpp" line="881"/>
      <source>Selection error</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="578"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>只有实体特征才能成为实体的结算特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="700"/>
      <location filename="../../CommandBody.cpp" line="722"/>
      <location filename="../../CommandBody.cpp" line="737"/>
      <source>Features cannot be moved</source>
      <translation>特征无法被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="701"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>一些选定的特征依赖于源实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="738"/>
      <source>There are no other bodies to move to</source>
      <translation>没有其他实体可以移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="875"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>无法移动实体的基础特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="882"/>
      <source>Select one or more features from the same body.</source>
      <translation>从同一实体上选择一个或多个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>Beginning of the body</source>
      <translation>实体的起始</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="964"/>
      <source>Dependency violation</source>
      <translation>依赖冲突</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="965"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>较早的特征不能依赖于较后的特征。

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="261"/>
      <source>No previous feature found</source>
      <translation>未找到之前的特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="262"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>如果没有可用的基础特征, 就不可能创建减料特征</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="224"/>
      <location filename="../../TaskTransformedParameters.cpp" line="438"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="225"/>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="227"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="78"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="185"/>
      <source>Active Body Required</source>
      <translation>需要激活状态的实体</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="138"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation type="unfinished">To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="186"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation type="unfinished">To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="231"/>
      <source>Feature is not in a body</source>
      <translation>特征不在实体内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="232"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的实体对象。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="264"/>
      <source>Feature is not in a part</source>
      <translation>特征不在零件内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="265"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的零件对象。</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="91"/>
      <location filename="../../ViewProviderDressUp.cpp" line="63"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="208"/>
      <location filename="../../ViewProviderTransformed.cpp" line="64"/>
      <source>Edit %1</source>
      <translation>编辑 %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="104"/>
      <source>Set Face Colors</source>
      <translation type="unfinished">Set Face Colors</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="112"/>
      <location filename="../../ViewProviderDatum.cpp" line="206"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="117"/>
      <location filename="../../ViewProviderDatum.cpp" line="202"/>
      <source>Line</source>
      <translation>线</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="122"/>
      <location filename="../../ViewProviderDatum.cpp" line="210"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="127"/>
      <source>Coordinate System</source>
      <translation>坐标系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="227"/>
      <source>Edit Datum</source>
      <translation type="unfinished">Edit Datum</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="87"/>
      <source>Feature error</source>
      <translation>特征错误</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="88"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation type="unfinished">%1 misses a base feature.
This feature is broken and cannot be edited.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="203"/>
      <source>Edit Shape Binder</source>
      <translation>编辑形状链接器</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="315"/>
      <source>Synchronize</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="317"/>
      <source>Select Bound Object</source>
      <translation type="unfinished">Select Bound Object</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="138"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation type="unfinished">The document "%1" you are editing was designed with an old version of Part Design workbench.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="141"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation type="unfinished">Migrate in order to use modern Part Design features?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="144"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation type="unfinished">The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="148"/>
      <source>Make the migration automatically?</source>
      <translation type="unfinished">Make the migration automatically?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>注意: 如果您选择迁移, 您将无法使用旧的 FreeCAD 版本编辑该文件。如果你拒绝迁移, 你将无法使用新的零件设计工作台功能, 如实体和零部件。因此, 您也无法在装配工作台中使用您的零部件。但是以后您仍然可以用 "零件设计-迁移..."来完成迁移。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="159"/>
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
      <location filename="../../ViewProviderGroove.cpp" line="44"/>
      <source>Edit Groove</source>
      <translation type="unfinished">Edit Groove</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="49"/>
      <source>Edit Helix</source>
      <translation type="unfinished">Edit Helix</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="64"/>
      <source>Edit Hole</source>
      <translation type="unfinished">Edit Hole</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="38"/>
      <source>Edit Linear Pattern</source>
      <translation type="unfinished">Edit Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="64"/>
      <source>Edit Loft</source>
      <translation type="unfinished">Edit Loft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="38"/>
      <source>Edit Mirror</source>
      <translation>编辑镜像</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="47"/>
      <source>Edit Multi-Transform</source>
      <translation type="unfinished">Edit Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="44"/>
      <source>Edit Pad</source>
      <translation type="unfinished">Edit Pad</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="72"/>
      <source>Edit Pipe</source>
      <translation type="unfinished">Edit Pipe</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="46"/>
      <source>Edit Pocket</source>
      <translation type="unfinished">Edit Pocket</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="37"/>
      <source>Edit Polar Pattern</source>
      <translation type="unfinished">Edit Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="49"/>
      <source>Edit Primitive</source>
      <translation type="unfinished">Edit Primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="44"/>
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
      <translation type="unfinished">Hole Parameters</translation>
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
      <translation type="unfinished">&lt;b&gt;Threading&lt;/b&gt;</translation>
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
      <location filename="../../Workbench.cpp" line="42"/>
      <source>&amp;Sketch</source>
      <translation>草图(&amp;S)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>&amp;Part Design</source>
      <translation>零件设计(&amp;P)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Datums</source>
      <translation>基准</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Additive Features</source>
      <translation type="unfinished">Additive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Subtractive Features</source>
      <translation type="unfinished">Subtractive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Dress-Up Features</source>
      <translation type="unfinished">Dress-Up Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Transformation Features</source>
      <translation type="unfinished">Transformation Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Sprocket…</source>
      <translation type="unfinished">Sprocket…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Measure</source>
      <translation>测量</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Refresh</source>
      <translation>刷新</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Toggle 3D</source>
      <translation>切换3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Helper</source>
      <translation>零件设计助手</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Modeling</source>
      <translation>零件设计建模</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Length [mm]</source>
      <translation>长度 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Diameter [mm]</source>
      <translation>直径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Inner diameter [mm]</source>
      <translation>内直径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Constraint type</source>
      <translation>约束类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Start edge type</source>
      <translation>起始边缘类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge size</source>
      <translation>起始边缘尺寸</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>End edge type</source>
      <translation>结束边缘类型</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>结束边缘大小</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="64"/>
      <source>Shaft Wizard</source>
      <translation type="unfinished">Shaft Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 1</source>
      <translation>截面1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 2</source>
      <translation>截面2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Add column</source>
      <translation>添加列</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation>截面 %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="150"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="165"/>
      <source>None</source>
      <translation>无</translation>
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
      <translation>轴承</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="154"/>
      <source>Gear</source>
      <translation>齿轮</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="155"/>
      <source>Pulley</source>
      <translation>滑轮</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="166"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="167"/>
      <source>Fillet</source>
      <translation>圆角</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="57"/>
      <source>All</source>
      <translation>全部</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="104"/>
      <source>Missing Module</source>
      <translation type="unfinished">Missing Module</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation type="unfinished">The Plot add-on is not installed. Install it to enable this feature.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="214"/>
      <source>Shaft design wizard...</source>
      <translation>轴设计向导...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="215"/>
      <source>Start the shaft design wizard</source>
      <translation>启动轴设计向导</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="395"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>链接对象不是 PartDesign 功能</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="402"/>
      <source>Tip shape is empty</source>
      <translation>提示形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="62"/>
      <source>BaseFeature link is not set</source>
      <translation>基础特征链接未设置</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="67"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>基础特征必须是 Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="73"/>
      <source>BaseFeature has an empty shape</source>
      <translation>基础特征有空形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="76"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>无基础特征时无法进行布尔剪切</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="90"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="119"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>除 Part::Feature 及其衍生外，无法进行布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="97"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>无法对无效的基础形状进行布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="103"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation>无法对不在实体内的特征进行布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="129"/>
      <source>Base shape is null</source>
      <translation>基础形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="161"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="171"/>
      <location filename="../../../App/FeatureDraft.cpp" line="293"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="690"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="703"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="713"/>
      <location filename="../../../App/FeatureFillet.cpp" line="116"/>
      <location filename="../../../App/FeatureGroove.cpp" line="231"/>
      <location filename="../../../App/FeatureHole.cpp" line="2194"/>
      <location filename="../../../App/FeatureLoft.cpp" line="272"/>
      <location filename="../../../App/FeatureLoft.cpp" line="307"/>
      <location filename="../../../App/FeaturePipe.cpp" line="400"/>
      <location filename="../../../App/FeaturePipe.cpp" line="439"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="234"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation type="unfinished">Result has multiple solids: enable 'Allow Compound' in the active body.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="110"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="132"/>
      <source>Tool shape is null</source>
      <translation>工具形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="148"/>
      <source>Unsupported boolean operation</source>
      <translation>不支持的布尔操作</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="326"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pad with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="331"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pocket with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="628"/>
      <source>No extrusion geometry was generated.</source>
      <translation type="unfinished">No extrusion geometry was generated.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="648"/>
      <source>Resulting fused extrusion is null.</source>
      <translation type="unfinished">Resulting fused extrusion is null.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="682"/>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePipe.cpp" line="433"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="127"/>
      <source>Resulting shape is not a solid</source>
      <translation>结果形状不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="154"/>
      <source>Failed to create chamfer</source>
      <translation>创建倒角失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="289"/>
      <location filename="../../../App/FeatureFillet.cpp" line="99"/>
      <source>Resulting shape is null</source>
      <translation>结果形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="125"/>
      <source>No edges specified</source>
      <translation>未指定边</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="238"/>
      <source>Size must be greater than zero</source>
      <translation>尺寸必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="247"/>
      <source>Size2 must be greater than zero</source>
      <translation>尺寸2 必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="252"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>角度必须大于 0 且小于 180 度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="82"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>选择的形状上无法进行圆角处理</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="89"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>圆角半径必须大于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="89"/>
      <source>Angle of groove too large</source>
      <translation>沟槽角度过大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="95"/>
      <source>Angle of groove too small</source>
      <translation>沟槽角度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1910"/>
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
      <location filename="../../../App/FeatureExtrude.cpp" line="372"/>
      <source>Failed to obtain profile shape</source>
      <translation>无法获取轮廓形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="424"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>创建失败，方向与草图的法线矢量正交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="446"/>
      <location filename="../../../App/FeatureGroove.cpp" line="138"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="141"/>
      <source>Creating a face from sketch failed</source>
      <translation>从草图创建面失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>Angles of groove nullify each other</source>
      <translation type="unfinished">Angles of groove nullify each other</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="154"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="157"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>旋转轴与草图相交</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="238"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="241"/>
      <source>Could not revolve the sketch!</source>
      <translation>无法旋转草图！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="250"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="253"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>无法从草图中创建面。
不允许在草图中交叉实体。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="133"/>
      <source>Error: Pitch too small!</source>
      <translation>错误：节距太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="135"/>
      <location filename="../../../App/FeatureHelix.cpp" line="149"/>
      <source>Error: height too small!</source>
      <translation>错误：高度太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="141"/>
      <source>Error: pitch too small!</source>
      <translation>错误：节距太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="143"/>
      <location filename="../../../App/FeatureHelix.cpp" line="151"/>
      <location filename="../../../App/FeatureHelix.cpp" line="157"/>
      <source>Error: turns too small!</source>
      <translation>错误：圈数太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="161"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>错误：高度和增长率不能为零！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="175"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="187"/>
      <source>Error: No valid sketch or face</source>
      <translation>错误：没有有效的草图或面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="196"/>
      <source>Error: Face must be planar</source>
      <translation>错误：面必须是平面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="295"/>
      <location filename="../../../App/FeatureHelix.cpp" line="327"/>
      <location filename="../../../App/FeatureHole.cpp" line="2538"/>
      <source>Error: Result is not a solid</source>
      <translation>错误：结果不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="271"/>
      <source>Error: There is nothing to subtract</source>
      <translation>错误: 没有可减少的内容</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="275"/>
      <location filename="../../../App/FeatureHelix.cpp" line="299"/>
      <location filename="../../../App/FeatureHelix.cpp" line="330"/>
      <source>Error: Result has multiple solids</source>
      <translation>错误：结果有多个实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="288"/>
      <source>Error: Adding the helix failed</source>
      <translation>错误：添加螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="314"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>错误：交叉螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="321"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>错误：减去螺旋失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="344"/>
      <source>Error: Could not create face from sketch</source>
      <translation>错误：无法从草图创建面</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1416"/>
      <source>Thread type is invalid</source>
      <translation>螺纹类型无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1950"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>孔错误：不支持的长度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1953"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>孔错误：无效的孔深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1976"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>孔错误：无效斜角</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1997"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>孔错误：挖孔直径太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2001"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>孔错误：孔切割深度必须小于孔深度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2005"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>孔错误：孔切割深度必须大于等于 0</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2027"/>
      <source>Hole error: Invalid countersink</source>
      <translation>孔错误：无效的埋头孔</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2060"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>孔错误：无效的钻尖角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2070"/>
      <source>Hole error: Invalid drill point</source>
      <translation>孔错误：钻点无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2104"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>孔错误：无法旋转草图</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2108"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>孔错误：结果形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2118"/>
      <source>Error: Adding the thread failed</source>
      <translation>错误：添加螺纹失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2126"/>
      <source>Hole error: Finding axis failed</source>
      <translation type="unfinished">Hole error: Finding axis failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2172"/>
      <location filename="../../../App/FeatureHole.cpp" line="2177"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>轮廓边缘布尔运算失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2183"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>在轮廓边缘的布尔运算产生了非实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="154"/>
      <source>Boolean operation failed</source>
      <translation>布尔操作失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2204"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>无法从草图创建面。不允许使用相交的草图实体、或草图中的多个面来制作凹槽。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2354"/>
      <source>Thread type out of range</source>
      <translation>螺线类型超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2357"/>
      <source>Thread size out of range</source>
      <translation>螺线大小超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2513"/>
      <source>Error: Thread could not be built</source>
      <translation>错误：无法构建螺纹</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="165"/>
      <source>Loft: At least one section is needed</source>
      <translation>拉伸：至少需要一个轮廊</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="320"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>拉伸：在拉伸时发生致命错误</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="202"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>拉伸：从草图创建面失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="251"/>
      <location filename="../../../App/FeaturePipe.cpp" line="375"/>
      <source>Loft: Failed to create shell</source>
      <translation>拉伸：创建外壳失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="725"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>无法从草图立建面。
草图中不允许有相交的实体或多个面。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="178"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>管道：无法获取轮廓形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="183"/>
      <source>No spine linked</source>
      <translation>没链接到骨架</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="196"/>
      <source>No auxiliary spine linked.</source>
      <translation>没链接到辅助骨架。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="217"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>管道：在使用带有孤立点的草图作为截面时，只需要一个孤立点。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="223"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>管道：当使用单点轮廓时，至少需要一个截面</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="237"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation type="unfinished">Pipe: All sections need to be Part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="243"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>管道：无法获取截面形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="252"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>管道：只有轮廓和最后一个截面可以作为顶点</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="261"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>多重截面需要有与基本截面相同数量的内部线</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="288"/>
      <source>Path must not be a null shape</source>
      <translation>路径不能是空形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="323"/>
      <source>Pipe could not be built</source>
      <translation>无法构建管道</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="370"/>
      <source>Result is not a solid</source>
      <translation>结果不是实体</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="397"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>错误: 没有可减少的内容</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="450"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>制作管道时发生致命错误</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="573"/>
      <source>Invalid element in spine.</source>
      <translation>骨架中有无效元素。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="576"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>骨架中的元素既不是边线也不是连线。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="589"/>
      <source>Spine is not connected.</source>
      <translation>骨架未连接</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="593"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>骨架既不是边线也不是连线。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="597"/>
      <source>Invalid spine.</source>
      <translation>无效骨架。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="95"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>没有基础特征时无法减去原始特征</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="290"/>
      <location filename="../../../App/FeaturePipe.cpp" line="421"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="113"/>
      <source>Unknown operation type</source>
      <translation>未知操作类型</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="296"/>
      <location filename="../../../App/FeaturePipe.cpp" line="427"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="120"/>
      <source>Failed to perform boolean operation</source>
      <translation>执行布尔操作失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="201"/>
      <source>Length of box too small</source>
      <translation>方块长度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="203"/>
      <source>Width of box too small</source>
      <translation>方块宽度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="205"/>
      <source>Height of box too small</source>
      <translation>方块高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="251"/>
      <source>Radius of cylinder too small</source>
      <translation>圆柱半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="253"/>
      <source>Height of cylinder too small</source>
      <translation>圆柱高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="255"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>圆柱旋转角度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="308"/>
      <source>Radius of sphere too small</source>
      <translation>球体半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="357"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="359"/>
      <source>Radius of cone cannot be negative</source>
      <translation>锥体半径不能为负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="361"/>
      <source>Height of cone too small</source>
      <translation>锥体高度太小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="424"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="426"/>
      <source>Radius of ellipsoid too small</source>
      <translation>椭球半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="508"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="510"/>
      <source>Radius of torus too small</source>
      <translation>环面半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="573"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>棱柱的多边形无效，必须至少有 3 条或以上的边</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="575"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>棱柱多边形的外接圆半径过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="577"/>
      <source>Height of prism is too small</source>
      <translation>棱柱高度过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="658"/>
      <source>delta x of wedge too small</source>
      <translation>楔形的 X 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="661"/>
      <source>delta y of wedge too small</source>
      <translation>楔形的 Y 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="664"/>
      <source>delta z of wedge too small</source>
      <translation>楔形的 Z 差过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="667"/>
      <source>delta z2 of wedge is negative</source>
      <translation>楔形的 Z2 差是负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="670"/>
      <source>delta x2 of wedge is negative</source>
      <translation>楔形的 X2 差是负数</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="92"/>
      <source>Angle of revolution too large</source>
      <translation>旋转角过大</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="98"/>
      <source>Angle of revolution too small</source>
      <translation>旋转角过小</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="104"/>
      <source>Angles of revolution nullify each other</source>
      <translation type="unfinished">Angles of revolution nullify each other</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="131"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="134"/>
      <source>Reference axis is invalid</source>
      <translation>参考坐标轴无效</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="675"/>
      <source>Fusion with base feature failed</source>
      <translation>与基本特征联合失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="101"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>转换功能链接的不是零件对象</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="106"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>没有与变换特征链接的原始特征。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="324"/>
      <source>Cannot transform invalid support shape</source>
      <translation>无法变换无效的支持形状</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="373"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>添加/减料的特性形状为空</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="365"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>只能变换增料和减料特征</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="101"/>
      <source>Invalid face reference</source>
      <translation>无效的面参考</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edits the involute gear definition</source>
      <translation type="unfinished">Creates or edits the involute gear definition</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket</source>
      <translation type="unfinished">Sprocket</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
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
      <translation>预览</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Starts the shaft design wizard</source>
      <translation type="unfinished">Starts the shaft design wizard</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="82"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation type="unfinished">Failure while computing removed volume preview: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="101"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation type="unfinished">Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2378"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2379"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2413"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2414"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="196"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation type="unfinished">Creates an additive box by its width, height, and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="200"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation type="unfinished">Creates an additive cylinder by its radius, height, and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="204"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates an additive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="208"/>
      <source>Creates an additive cone</source>
      <translation type="unfinished">Creates an additive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="212"/>
      <source>Creates an additive ellipsoid</source>
      <translation type="unfinished">Creates an additive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="216"/>
      <source>Creates an additive torus</source>
      <translation type="unfinished">Creates an additive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="220"/>
      <source>Creates an additive prism</source>
      <translation type="unfinished">Creates an additive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Creates an additive wedge</source>
      <translation type="unfinished">Creates an additive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="348"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation type="unfinished">Creates a subtractive box by its width, height and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="352"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation type="unfinished">Creates a subtractive cylinder by its radius, height and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="356"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates a subtractive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="360"/>
      <source>Creates a subtractive cone</source>
      <translation type="unfinished">Creates a subtractive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="364"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation type="unfinished">Creates a subtractive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="368"/>
      <source>Creates a subtractive torus</source>
      <translation type="unfinished">Creates a subtractive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="372"/>
      <source>Creates a subtractive prism</source>
      <translation type="unfinished">Creates a subtractive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="376"/>
      <source>Creates a subtractive wedge</source>
      <translation type="unfinished">Creates a subtractive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="982"/>
      <source>Attachment</source>
      <translation>附着</translation>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="843"/>
      <source>Groove Parameters</source>
      <translation type="unfinished">Groove Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="36"/>
      <source>Transformed Feature Messages</source>
      <translation type="unfinished">Transformed Feature Messages</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="119"/>
      <source>Active Body</source>
      <translation type="unfinished">Active Body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer Parameters</source>
      <translation type="unfinished">Chamfer Parameters</translation>
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
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft Parameters</source>
      <translation type="unfinished">Draft Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet Parameters</source>
      <translation type="unfinished">Fillet Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>Linear Pattern Parameters</source>
      <translation type="unfinished">Linear Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirror Parameters</source>
      <translation type="unfinished">Mirror Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>Multi-Transform Parameters</source>
      <translation type="unfinished">Multi-Transform Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>Polar Pattern Parameters</source>
      <translation type="unfinished">Polar Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scale Parameters</source>
      <translation type="unfinished">Scale Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness Parameters</source>
      <translation type="unfinished">Thickness Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="112"/>
      <source>Direction 2</source>
      <translation type="unfinished">Direction 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="215"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation type="unfinished">Select a direction reference (edge, face, datum line)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="295"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation type="unfinished">Invalid selection. Select an edge, planar face, or datum line.</translation>
    </message>
  </context>
</TS>
