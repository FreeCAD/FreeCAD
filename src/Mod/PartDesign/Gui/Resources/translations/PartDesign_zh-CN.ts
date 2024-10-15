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
Positive values make the radius grow, nevatige shrink.</source>
      <translation>在螺旋线周围形成外壳的圆锥的角度。
非零值将螺旋变成锥形螺旋。
正值使半径增大，负值缩小。</translation>
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
      <translation type="unfinished">True=2 curves with each 3 control points, False=1 curve with 4 control points.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation type="unfinished">True=external Gear, False=internal Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation type="unfinished">The height of the tooth from the pitch circle up to its tip, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation type="unfinished">The height of the tooth from the pitch circle down to its root, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation type="unfinished">The radius of the fillet at the root of the tooth, normalized by the module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation type="unfinished">The distance by which the reference profile is shifted outwards, normalized by the module.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1535"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1536"/>
      <source>Additive helix</source>
      <translation>增料螺旋体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1537"/>
      <source>Sweep a selected sketch along a helix</source>
      <translation>沿螺旋扫描选中的草图</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1434"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1435"/>
      <source>Additive loft</source>
      <translation>增料放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1436"/>
      <source>Loft a selected profile through other profile sections</source>
      <translation>通过其他轮廓截面放样选定的轮廓</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1332"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1333"/>
      <source>Additive pipe</source>
      <translation>增料管状体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1334"/>
      <source>Sweep a selected sketch along a path or to other profiles</source>
      <translation>沿路径或轮廓扫掠所选草图</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="86"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>Create body</source>
      <translation>创建实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="88"/>
      <source>Create a new body and make it active</source>
      <translation>创建新的可编辑实体并激活</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2379"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2380"/>
      <source>Boolean operation</source>
      <translation>布尔运算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2381"/>
      <source>Boolean operation with two or more bodies</source>
      <translation>对两个或以上的实体进行布尔运算</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Create a local coordinate system</source>
      <translation>创建局部坐标系</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>Create a new local coordinate system</source>
      <translation>创建新的局部坐标系</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1829"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1830"/>
      <source>Chamfer</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1831"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>给所选形状的边缘倒角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="430"/>
      <source>Create a clone</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="431"/>
      <source>Create a new clone</source>
      <translation>创建新副本</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1858"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1859"/>
      <source>Draft</source>
      <translation>拔模</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1860"/>
      <source>Make a draft on a face</source>
      <translation>在面上创建拔模</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="610"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="611"/>
      <source>Duplicate selected object</source>
      <translation>复制所选对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="612"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>复制所选对象并将其添加到活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1801"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1802"/>
      <source>Fillet</source>
      <translation>圆角</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1803"/>
      <source>Make a fillet on an edge, face or body</source>
      <translation>给边、面或实体倒圆角</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1265"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1266"/>
      <source>Groove</source>
      <translation>挖槽</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1267"/>
      <source>Groove a selected sketch</source>
      <translation>以选定草图挖槽</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1159"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1160"/>
      <source>Hole</source>
      <translation>孔</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1161"/>
      <source>Create a hole with the selected sketch</source>
      <translation>基于选定草图创建孔</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>Create a datum line</source>
      <translation>创建基准线</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Create a new datum line</source>
      <translation>创建新基准线</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2098"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2099"/>
      <source>LinearPattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2100"/>
      <source>Create a linear pattern feature</source>
      <translation>创建线性阵列特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="316"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="317"/>
      <source>Migrate</source>
      <translation>迁移</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="318"/>
      <source>Migrate document to the modern PartDesign workflow</source>
      <translation>将文档迁移到当前零件设计工作流</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2044"/>
      <source>Mirrored</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2045"/>
      <source>Create a mirrored feature</source>
      <translation>创建镜像特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="666"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="667"/>
      <source>Move object to other body</source>
      <translation>将对象移动到其他实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="668"/>
      <source>Moves the selected object to another body</source>
      <translation>移动选定对象到另一个实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="833"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="834"/>
      <source>Move object after other object</source>
      <translation>将对象移至其它对象之后</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="835"/>
      <source>Moves the selected object and insert it after another object</source>
      <translation>插入所选对象至另一对象之后</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="531"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="532"/>
      <source>Set tip</source>
      <translation>设置结算位置</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="533"/>
      <source>Move the tip of the body</source>
      <translation>移动实体的结算位置</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2259"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2260"/>
      <source>Create MultiTransform</source>
      <translation>创建多重变换</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2261"/>
      <source>Create a multitransform feature</source>
      <translation>创建多重变换特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="503"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="504"/>
      <source>Create sketch</source>
      <translation>创建草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="505"/>
      <source>Create a new sketch</source>
      <translation>创建一个新草绘</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1095"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1096"/>
      <source>Pad</source>
      <translation>凸台</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1097"/>
      <source>Pad a selected sketch</source>
      <translation>基于选定草图创建凸台</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>Create a datum plane</source>
      <translation>创建基准面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="165"/>
      <source>Create a new datum plane</source>
      <translation>创建新基准面</translation>
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
      <source>Create a pocket with the selected sketch</source>
      <translation>基于选定草图创建凹坑</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Create a datum point</source>
      <translation>创建基准点</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Create a new datum point</source>
      <translation>创建新基准点</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2155"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2156"/>
      <source>PolarPattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2157"/>
      <source>Create a polar pattern feature</source>
      <translation>创建环形阵列特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1206"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1207"/>
      <source>Revolution</source>
      <translation>旋转体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1208"/>
      <source>Revolve a selected sketch</source>
      <translation>旋转选定的草绘</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2214"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2215"/>
      <source>Scaled</source>
      <translation>缩放</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2216"/>
      <source>Create a scaled feature</source>
      <translation>创建一个比例缩放特征</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Create a shape binder</source>
      <translation>创建形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Create a new shape binder</source>
      <translation>创建新的形状引用连接</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <location filename="../../Command.cpp" line="347"/>
      <source>Create a sub-object(s) shape binder</source>
      <translation>创建子对象形状引用连接</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Subtractive helix</source>
      <translation>减料螺旋体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1614"/>
      <source>Sweep a selected sketch along a helix and remove it from the body</source>
      <translation>沿螺旋线扫描选中的草图并将其从实体中移除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1485"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1486"/>
      <source>Subtractive loft</source>
      <translation>减料放样</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1487"/>
      <source>Loft a selected profile through other profile sections and remove it from the body</source>
      <translation>通过其他轮廓截面来放样所选轮廓, 并将其从实体中删除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1383"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1384"/>
      <source>Subtractive pipe</source>
      <translation>减料管状体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1385"/>
      <source>Sweep a selected sketch along a path or to other profiles and remove it from the body</source>
      <translation>以选定草图为截面沿路径或轮廓做减料扫掠</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1926"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1927"/>
      <source>Thickness</source>
      <translation>厚度</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1928"/>
      <source>Make a thick solid</source>
      <translation>抽壳</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Create an additive primitive</source>
      <translation>创建增料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="199"/>
      <source>Additive Box</source>
      <translation>增料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="203"/>
      <source>Additive Cylinder</source>
      <translation>增料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="207"/>
      <source>Additive Sphere</source>
      <translation>增料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="211"/>
      <source>Additive Cone</source>
      <translation>增料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Ellipsoid</source>
      <translation>增料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="219"/>
      <source>Additive Torus</source>
      <translation>增料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="223"/>
      <source>Additive Prism</source>
      <translation>增料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="227"/>
      <source>Additive Wedge</source>
      <translation>增料楔形体</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>PartDesign</source>
      <translation>零件设计</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <location filename="../../CommandPrimitive.cpp" line="245"/>
      <source>Create a subtractive primitive</source>
      <translation>创建减料图元</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="355"/>
      <source>Subtractive Box</source>
      <translation>减料立方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="359"/>
      <source>Subtractive Cylinder</source>
      <translation>减料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="363"/>
      <source>Subtractive Sphere</source>
      <translation>减料球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="367"/>
      <source>Subtractive Cone</source>
      <translation>减料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="371"/>
      <source>Subtractive Ellipsoid</source>
      <translation>减料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="375"/>
      <source>Subtractive Torus</source>
      <translation>减料圆环体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="379"/>
      <source>Subtractive Prism</source>
      <translation>减料棱柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="383"/>
      <source>Subtractive Wedge</source>
      <translation>减料楔形体</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="300"/>
      <source>Edit ShapeBinder</source>
      <translation>编辑形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="309"/>
      <source>Create ShapeBinder</source>
      <translation>创建形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="392"/>
      <source>Create SubShapeBinder</source>
      <translation>创建子形状引用连接</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="447"/>
      <source>Create Clone</source>
      <translation>创建副本</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="298"/>
      <location filename="../../Command.cpp" line="995"/>
      <source>Make copy</source>
      <translation>制作副本</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="248"/>
      <source>Create a Sketch on Face</source>
      <translation>在面上创建草图</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="488"/>
      <source>Create a new Sketch</source>
      <translation>创建新草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2309"/>
      <source>Convert to MultiTransform feature</source>
      <translation>转换为多重变换功能</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2397"/>
      <source>Create Boolean</source>
      <translation>创建布尔变量</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <location filename="../../CommandBody.cpp" line="190"/>
      <source>Add a Body</source>
      <translation>添加实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="434"/>
      <source>Migrate legacy Part Design features to Bodies</source>
      <translation type="unfinished">Migrate legacy Part Design features to Bodies</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="579"/>
      <source>Move tip to selected feature</source>
      <translation>将结算位置移至所选特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="624"/>
      <source>Duplicate a PartDesign object</source>
      <translation>复制零件设计对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="747"/>
      <source>Move an object</source>
      <translation>移动一个对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="898"/>
      <source>Move an object inside tree</source>
      <translation>在树中移动对象</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="267"/>
      <source>Mirrored</source>
      <translation>镜像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="304"/>
      <source>Make LinearPattern</source>
      <translation>线性阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="348"/>
      <source>PolarPattern</source>
      <translation>环形阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="385"/>
      <source>Scaled</source>
      <translation>缩放</translation>
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
      <translation>无效形状</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>草图中找不到线框</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>草图被其他特征使用</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another Body feature</source>
      <translation>草图属于另一个实体特征</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>基准平面</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the Tip feature</source>
      <translation>特征位于结算位置之后</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Face tools</source>
      <translation>面工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Edge tools</source>
      <translation>边缘工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Boolean tools</source>
      <translation>布尔工具</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Helper tools</source>
      <translation type="unfinished">Helper tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Modeling tools</source>
      <translation>建模工具</translation>
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
      <source>Involute parameter</source>
      <translation>渐开线参数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>齿数：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module:</source>
      <translation>模数：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle:</source>
      <translation>压力角:</translation>
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
      <translation>是</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>否</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear:</source>
      <translation>外齿轮：</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum Coefficient</source>
      <translation>齿顶高系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum Coefficient</source>
      <translation>齿根高系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root Fillet Coefficient</source>
      <translation>根部圆角系数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile Shift Coefficient</source>
      <translation>变位系数</translation>
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
      <source>To create a new PartDesign object, there must be an active Body object in the document.

Please select a body from below, or create a new body.</source>
      <translation>要创建新的零件设计对象，文档中必须有一个活动的主体对象。

请从下方选择一个主体，或创建一个新主体。</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="36"/>
      <source>Create new body</source>
      <translation>创建新实体</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length:</source>
      <translation>长度:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width:</source>
      <translation>宽度:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height:</source>
      <translation>高度:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius:</source>
      <translation>半径:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <source>Angle in first direction:</source>
      <translation>第一方向的角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>第一方向的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <source>Angle in second direction:</source>
      <translation>第二方向的角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>第二方向的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle:</source>
      <translation>旋转角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1:</source>
      <translation>半径 1:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2:</source>
      <translation>半径 2:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle:</source>
      <translation>角度:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <source>U parameter:</source>
      <translation>绕指定轴角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters:</source>
      <translation>与指定轴的夹角：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>本地z方向的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local x-direction</source>
      <translation>本地X方向的半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3:</source>
      <translation>半径 3:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local y-direction
If zero, it is equal to Radius2</source>
      <translation>本地方向的半径
如果零，则等于半径2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter:</source>
      <translation>与指定轴的夹角：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local xy-plane</source>
      <translation>本地Xy-平面半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local xz-plane</source>
      <translation>本地xz-平面半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U Parameter:</source>
      <translation>绕指定轴角度：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon:</source>
      <translation>多边形：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius:</source>
      <translation>外接圆半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max:</source>
      <translation>X 最小值/最大值:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max:</source>
      <translation>Y 最小值/最大值:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max:</source>
      <translation>Z 最小值/最大值:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max:</source>
      <translation>X2 最小值/最大值:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max:</source>
      <translation>Z2 最小值/最大值:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch:</source>
      <translation>节距：</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system:</source>
      <translation>坐标系:</translation>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth:</source>
      <translation>节距</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations:</source>
      <translation>圈数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1:</source>
      <translation>角度 1:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2:</source>
      <translation>角度 2:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From three points</source>
      <translation>以三点构建</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius:</source>
      <translation>主半径:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius:</source>
      <translation>次半径:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X:</source>
      <translation>X:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y:</source>
      <translation>Y:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z:</source>
      <translation>Z:</translation>
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
      <translation>选择的几何体不是激活状态实体的一部分。请定义如何处理这些选择。如果放弃编辑请取消指令。</translation>
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
      <location filename="../../ReferenceSelection.cpp" line="270"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>选择操作会导致循环引用。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add body</source>
      <translation>添加实体</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove body</source>
      <translation>删除实体</translation>
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
      <location filename="../../TaskBooleanParameters.cpp" line="53"/>
      <source>Boolean parameters</source>
      <translation>布尔参数</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="86"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="50"/>
      <source>Primitive parameters</source>
      <translation>图元参数</translation>
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
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>点击按钮进入选择模式，
再次点击结束选择</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="24"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 选择一个项目以高亮显示
- 双击一个项目以查看其倒角</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="49"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="57"/>
      <source>Equal distance</source>
      <translation>等距：</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="62"/>
      <source>Two distances</source>
      <translation>两倍距离</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="67"/>
      <source>Distance and angle</source>
      <translation>距离和角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="80"/>
      <source>Flip direction</source>
      <translation>翻转方向</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="101"/>
      <source>Size</source>
      <translation>大小</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="120"/>
      <source>Use All Edges</source>
      <translation>使用所有边</translation>
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
      <translation type="unfinished">Empty chamfer created !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="392"/>
      <source>Empty body list</source>
      <translation>空的实体列表</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="393"/>
      <source>The body list cannot be empty</source>
      <translation>实体列表不能空</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="407"/>
      <source>Boolean: Accept: Input error</source>
      <translation>布尔值： 接受： 输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="101"/>
      <source>Incompatible reference set</source>
      <translation>不兼容的引用集</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="102"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>没有适合当前参考集的附着模式。如果您选择继续，特征将保持现有状态，且将被定义为参照更改而不被移动。要继续吗？</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="139"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="412"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>点击按钮进入选择模式，
再次点击结束选择</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="24"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看草稿</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="47"/>
      <source>Draft angle</source>
      <translation>拔模角度</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="80"/>
      <source>Neutral plane</source>
      <translation>中性面</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="97"/>
      <source>Pull direction</source>
      <translation>拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="112"/>
      <source>Reverse pull direction</source>
      <translation>反转拔模方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="292"/>
      <source>Empty draft created !
</source>
      <translation type="unfinished">Empty draft created !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="84"/>
      <source>Preview</source>
      <translation>预览</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="90"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Add all edges</source>
      <translation>添加所有边</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="300"/>
      <source>Adds all edges to the list box (active only when in add selection mode).</source>
      <translation>将所有边添加到列表框中（仅当处于添加选择模式时才处于活动状态）。</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="308"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1216"/>
      <source>No face selected</source>
      <translation>未选择任何面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="160"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1051"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="176"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="268"/>
      <source>Preview</source>
      <translation>预览</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="272"/>
      <source>Select faces</source>
      <translation>选取面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="494"/>
      <source>No shape selected</source>
      <translation>无选定的形状</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="602"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="605"/>
      <source>Face normal</source>
      <translation>面法线</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="609"/>
      <source>Select reference...</source>
      <translation type="unfinished">Select reference...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="613"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="618"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1003"/>
      <source>Click on a shape in the model</source>
      <translation type="unfinished">Click on a shape in the model</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1211"/>
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
      <source>Allow external features</source>
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
      <translation>创建关联副本</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>创建交叉引用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="63"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="65"/>
      <source>Invalid shape</source>
      <translation>无效形状</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>No wire in sketch</source>
      <translation>草图中找不到线框</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Sketch already used by other feature</source>
      <translation>草图被其他特征使用</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Belongs to another body</source>
      <translation>属于另一个实体</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Belongs to another part</source>
      <translation>属于另一个零件</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Doesn't belong to any body</source>
      <translation>不属于任何实体</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Base plane</source>
      <translation>基准平面</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Feature is located after the tip feature</source>
      <translation>特征位于结算位置之后</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="89"/>
      <source>Select attachment</source>
      <translation type="unfinished">Select attachment</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>点击按钮进入选择模式，
再次点击结束选择</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="24"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看这些文件</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="47"/>
      <source>Radius:</source>
      <translation>半径:</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="63"/>
      <source>Use All Edges</source>
      <translation>使用所有边</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="198"/>
      <source>Empty fillet created!</source>
      <translation>空圆角已创建！</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status:</source>
      <translation>状态:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>有效</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis:</source>
      <translation>轴:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="237"/>
      <source>Base X axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="238"/>
      <source>Base Y axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="239"/>
      <source>Base Z axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="220"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="219"/>
      <source>Normal sketch axis</source>
      <translation type="unfinished">Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="204"/>
      <source>Select reference...</source>
      <translation>选择参考...</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode:</source>
      <translation>模式：</translation>
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
      <translation>高转弯角角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>高转弯角增长</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch:</source>
      <translation>节距：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height:</source>
      <translation>高度:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns:</source>
      <translation>圈数</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle:</source>
      <translation>圆锥角：</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth:</source>
      <translation>径向增长</translation>
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
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Update view</source>
      <translation>更新视图</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="56"/>
      <source>Helix parameters</source>
      <translation>螺旋参数</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="289"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告：螺旋可能是自交错的</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="294"/>
      <source>Error: helix touches itself</source>
      <translation>错误: 螺旋自相交</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="343"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="47"/>
      <source>Counterbore</source>
      <translation>沉孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="48"/>
      <source>Countersink</source>
      <translation>埋头孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="49"/>
      <source>Counterdrill</source>
      <translation>沉头钻</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Hole parameters</source>
      <translation>孔参数</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="63"/>
      <source>None</source>
      <translation>无</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="64"/>
      <source>ISO metric regular profile</source>
      <translation>ISO 公制常规配置</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>ISO metric fine profile</source>
      <translation>ISO 公制细牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="66"/>
      <source>UTS coarse profile</source>
      <translation>UTS 美制粗牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>UTS fine profile</source>
      <translation>UTS 美制细牙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>UTS extra fine profile</source>
      <translation>UTS 美制超细牙</translation>
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
      <translation>反转方向</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="55"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="63"/>
      <source>Overall Length</source>
      <translation type="unfinished">Overall Length</translation>
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
      <translation>长度</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="156"/>
      <source>Occurrences</source>
      <translation>出现次数</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="330"/>
      <source>Error</source>
      <translation>错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>直纹曲面</translation>
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
      <source>Update view</source>
      <translation>更新视图</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft parameters</source>
      <translation>放样参数</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="74"/>
      <source>Remove</source>
      <translation>删除</translation>
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
      <location filename="../../TaskMultiTransformParameters.cpp" line="71"/>
      <source>Edit</source>
      <translation>编辑</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Delete</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add mirrored transformation</source>
      <translation>添加镜像变换</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="89"/>
      <source>Add linear pattern</source>
      <translation>添加线性阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="95"/>
      <source>Add polar pattern</source>
      <translation>添加环形阵列</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="101"/>
      <source>Add scaled transformation</source>
      <translation>添加缩放变换</translation>
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
      <translation>右键添加</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad parameters</source>
      <translation>凸台参数</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset from face at which pad will end</source>
      <translation>偏移，从凸台结束的面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Reverses pad direction</source>
      <translation>反转凸台方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="70"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>To last</source>
      <translation>直到最后</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Up to face</source>
      <translation>直到表面</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Two dimensions</source>
      <translation>双向尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to shape</source>
      <translation type="unfinished">Up to shape</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Type</source>
      <translation>类型</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="38"/>
      <source>Length</source>
      <translation>长度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="75"/>
      <source>Offset to face</source>
      <translation>相对于面偏移</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="130"/>
      <source>Select shape</source>
      <translation>选择形状</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="148"/>
      <source>Select all faces</source>
      <translation>选取所有面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="170"/>
      <source>Click button to enter selection mode,
         click again to end selection</source>
      <translation type="unfinished">Click button to enter selection mode,
         click again to end selection</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="244"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="252"/>
      <source>Direction/edge:</source>
      <translation>方向/边缘：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="259"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>设置一个方向或从模型中选择边
作为参考值</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="264"/>
      <source>Sketch normal</source>
      <translation>草图法向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="269"/>
      <source>Select reference...</source>
      <translation type="unfinished">Select reference...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="274"/>
      <source>Custom direction</source>
      <translation>自定义方向：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="284"/>
      <source>Show direction</source>
      <translation>显示方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="294"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>否则，请将自定义向量用于凸台方向
将使用草图平面的法向量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="307"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="314"/>
      <source>x-component of direction vector</source>
      <translation>方向向量的x分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="336"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="343"/>
      <source>y-component of direction vector</source>
      <translation>方向向量的y分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="365"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="372"/>
      <source>z-component of direction vector</source>
      <translation>方向向量的z分量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>如果不选中，长度将按照指定的方向进行测量</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="407"/>
      <source>Length along sketch normal</source>
      <translation>沿草图法线长度：</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="145"/>
      <location filename="../../TaskPadPocketParameters.ui" line="227"/>
      <source>Applies length symmetrically to sketch plane</source>
      <translation>将长度对称地应用于草图平面中</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="230"/>
      <source>Symmetric to plane</source>
      <translation>相当平面对称</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="237"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="419"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Angle to taper the extrusion</source>
      <translation>倾斜拉伸的角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="422"/>
      <source>Taper angle</source>
      <translation>锥度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="58"/>
      <source>2nd length</source>
      <translation>第二长度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>2nd taper angle</source>
      <translation>第二锥角</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="212"/>
      <source>Select face</source>
      <translation>选取面</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="472"/>
      <source>Update view</source>
      <translation>更新视图</translation>
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
      <source>Curvelinear equivalence</source>
      <translation>曲线等量</translation>
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
      <translation>设置用于计算轮廓方向的常量副向量</translation>
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
      <translation>截面方向</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="611"/>
      <source>Remove</source>
      <translation>删除</translation>
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
      <source>Corner Transition</source>
      <translation>角过渡</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>变换</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right Corner</source>
      <translation>倒角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round Corner</source>
      <translation>圆角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to sweep along</source>
      <translation>扫描路径</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add Edge</source>
      <translation>添加边</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove Edge</source>
      <translation>删除边</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="68"/>
      <source>Pipe parameters</source>
      <translation>管参数</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="88"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="451"/>
      <location filename="../../TaskPipeParameters.cpp" line="563"/>
      <source>Input error</source>
      <translation>输入错误</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="451"/>
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
      <location filename="../../TaskPipeParameters.cpp" line="887"/>
      <source>Section transformation</source>
      <translation>截面变换</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="904"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket parameters</source>
      <translation>凹槽参数</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from face at which pocket will end</source>
      <translation>偏移，从凹坑结束的面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation>反转凹坑方向</translation>
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
      <translation>直到表面</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Two dimensions</source>
      <translation>双向尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to shape</source>
      <translation type="unfinished">Up to shape</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPolarPatternParameters</name>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="34"/>
      <source>Axis</source>
      <translation>轴线</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="46"/>
      <source>Reverse direction</source>
      <translation>反转方向</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="55"/>
      <source>Mode</source>
      <translation>模式</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="63"/>
      <source>Overall Angle</source>
      <translation type="unfinished">Overall Angle</translation>
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
      <translation>出现次数</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="329"/>
      <source>Error</source>
      <translation>错误</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="981"/>
      <source>Attachment</source>
      <translation>附着</translation>
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
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="173"/>
      <source>Dimension</source>
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis:</source>
      <translation>轴:</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="218"/>
      <source>Base X axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="219"/>
      <source>Base Y axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="220"/>
      <source>Base Z axis</source>
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
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="227"/>
      <source>Select reference...</source>
      <translation type="unfinished">Select reference...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Angle:</source>
      <translation>角度:</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="121"/>
      <source>Symmetric to plane</source>
      <translation>相当平面对称</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="128"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="137"/>
      <source>2nd angle</source>
      <translation type="unfinished">2nd angle</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="137"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="444"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Update view</source>
      <translation>更新视图</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="50"/>
      <source>Revolution parameters</source>
      <translation>旋转体参数</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="175"/>
      <source>To last</source>
      <translation>直到最后</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="178"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="180"/>
      <source>To first</source>
      <translation>到起始位置</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="181"/>
      <source>Up to face</source>
      <translation>直到表面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <source>Two dimensions</source>
      <translation>双向尺寸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="430"/>
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
      <location filename="../../TaskShapeBinder.cpp" line="60"/>
      <source>Datum shape parameters</source>
      <translation>基准图形参数</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="130"/>
      <source>Remove</source>
      <translation>删除</translation>
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
      <translation>点击按钮进入选择模式，
再次点击结束选择</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="24"/>
      <source>Select</source>
      <translation>选择</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- 选择一个项目来突出显示
- 双击一个项目来查看这些特征</translation>
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
      <translation>表皮</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="89"/>
      <source>Pipe</source>
      <translation>管状</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="94"/>
      <source>Recto Verso</source>
      <translation>双面</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="102"/>
      <source>Join Type</source>
      <translation>接合类型</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="110"/>
      <source>Arc</source>
      <translation>圆弧</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="115"/>
      <location filename="../../TaskThicknessParameters.ui" line="125"/>
      <source>Intersection</source>
      <translation>交集</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="132"/>
      <source>Make thickness inwards</source>
      <translation>厚度方向向里</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="265"/>
      <source>Empty thickness created !
</source>
      <translation type="unfinished">Empty thickness created !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed feature messages</source>
      <translation>变换特征消息</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="104"/>
      <source>Remove</source>
      <translation>删除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="408"/>
      <source>Normal sketch axis</source>
      <translation type="unfinished">Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="409"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="410"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="412"/>
      <location filename="../../TaskTransformedParameters.cpp" line="448"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="426"/>
      <source>Base X axis</source>
      <translation>X 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="427"/>
      <source>Base Y axis</source>
      <translation>Y 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="428"/>
      <source>Base Z axis</source>
      <translation>Z 轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="436"/>
      <location filename="../../TaskTransformedParameters.cpp" line="472"/>
      <source>Select reference...</source>
      <translation>选择引用...</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="462"/>
      <source>Base XY plane</source>
      <translation>XY 基准平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="463"/>
      <source>Base YZ plane</source>
      <translation>YZ 基准平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="464"/>
      <source>Base XZ plane</source>
      <translation>XZ 基准平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation type="unfinished">Transform body</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation type="unfinished">Transform tool shapes</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add feature</source>
      <translation>添加特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove feature</source>
      <translation>删除特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>列表可以通过拖动重新排序</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Update view</source>
      <translation>更新视图</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer parameters</source>
      <translation>倒角参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane parameters</source>
      <translation>基准面参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line parameters</source>
      <translation>基准线参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point parameters</source>
      <translation>基准点参数</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System parameters</source>
      <translation>局部坐标系参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft parameters</source>
      <translation>拔模参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet parameters</source>
      <translation>圆角参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>LinearPattern parameters</source>
      <translation>线性样式参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>MultiTransform parameters</source>
      <translation>多重变换参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>PolarPattern parameters</source>
      <translation>环形阵列样式参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scaled parameters</source>
      <translation>缩放参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness parameters</source>
      <translation>厚度参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirrored parameters</source>
      <translation>镜像参数</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="200"/>
      <source>Create an additive box by its width, height, and length</source>
      <translation>创建宽、高、长定义的增料长方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="204"/>
      <source>Create an additive cylinder by its radius, height, and angle</source>
      <translation>创建半径、高、角度定义的增料圆柱体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="208"/>
      <source>Create an additive sphere by its radius and various angles</source>
      <translation>创建半径、角度驱动的增料球体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="212"/>
      <source>Create an additive cone</source>
      <translation>创建增料圆锥体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="216"/>
      <source>Create an additive ellipsoid</source>
      <translation>创建增料椭球体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="220"/>
      <source>Create an additive torus</source>
      <translation>创建增料旋转体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Create an additive prism</source>
      <translation>创建增料棱柱</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="228"/>
      <source>Create an additive wedge</source>
      <translation>创建增料楔形体</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="356"/>
      <source>Create a subtractive box by its width, height and length</source>
      <translation>创建边长驱动的减料长方体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="360"/>
      <source>Create a subtractive cylinder by its radius, height and angle</source>
      <translation>创建半径、高、角度驱动的减料圆柱体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="364"/>
      <source>Create a subtractive sphere by its radius and various angles</source>
      <translation>创建半径、角度驱动的减料球体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="368"/>
      <source>Create a subtractive cone</source>
      <translation>创建减料圆锥体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="372"/>
      <source>Create a subtractive ellipsoid</source>
      <translation>创建减料椭球体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="376"/>
      <source>Create a subtractive torus</source>
      <translation>创建减料旋转体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="380"/>
      <source>Create a subtractive prism</source>
      <translation>创建减料棱柱体特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="384"/>
      <source>Create a subtractive wedge</source>
      <translation>创建减料楔形体特征</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="736"/>
      <source>Select body</source>
      <translation>选择实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="737"/>
      <source>Select a body from the list</source>
      <translation>从列表中选择实体</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="889"/>
      <source>Select feature</source>
      <translation>选择特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="890"/>
      <source>Select a feature from the list</source>
      <translation>从列表中选择特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="960"/>
      <source>Move tip</source>
      <translation>移动结算位置</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="961"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>被移动特征出现在当前设置的结算位置之后。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="962"/>
      <source>Do you want the last feature to be the new tip?</source>
      <translation>您想要最后一个特征成为新的结算位置？</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>Invalid selection</source>
      <translation>无效选择</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>没有适合选定对象的附着模式。请选择其他的东西。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <location filename="../../Command.cpp" line="149"/>
      <location filename="../../Command.cpp" line="151"/>
      <source>Error</source>
      <translation>错误</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>There is no active body. Please make a body active before inserting a datum entity.</source>
      <translation>无激活状态的实体。请在插入基准实体前激活实体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>Sub-Shape Binder</source>
      <translation>子形状引用连接</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="651"/>
      <source>Several sub-elements selected</source>
      <translation>若干子元素被选择</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="652"/>
      <source>You have to select a single face as support for a sketch!</source>
      <translation>您必须选择一个支持面以绘制草图!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="655"/>
      <source>No support face selected</source>
      <translation>未选中支持面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="656"/>
      <source>You have to select a face as support for a sketch!</source>
      <translation>您必须选择一个支持面以绘制草图!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="659"/>
      <source>No planar support</source>
      <translation>无支持平面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="660"/>
      <source>You need a planar face as support for a sketch!</source>
      <translation>您需要一个支持平面以绘制草图!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="663"/>
      <source>No valid planes in this document</source>
      <translation>文档中无有效平面</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="664"/>
      <source>Please create a plane first or select a face to sketch on</source>
      <translation>请先创建一个平面或选择一个平面用于画草图</translation>
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
      <translation>一个对话框已在任务面板打开</translation>
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
      <translation>您要关闭此对话框吗?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="896"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>无法使用此命令，因为没有可以减去的实体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="897"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>在尝试减料命令之前确保实体包含特征。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="918"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>无法使用所选对象。所选对象必须属于活动实体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="919"/>
      <source>Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</source>
      <translation>考虑使用引用连接或基础特征来参考物体的外部几何形状。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="941"/>
      <source>No sketch to work on</source>
      <translation>没有可工作的草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="942"/>
      <source>No sketch is available in the document</source>
      <translation>文档无可用草图</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1684"/>
      <location filename="../../Command.cpp" line="1710"/>
      <source>Wrong selection</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1685"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>从一单一实体中选择一边，面或体</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1689"/>
      <location filename="../../Command.cpp" line="2021"/>
      <source>Selection is not in Active Body</source>
      <translation>未在激活状态的实体中进行选择</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1690"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>从活动实体中选择边、面或体。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1700"/>
      <source>Wrong object type</source>
      <translation>错误的对象类型</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1701"/>
      <source>%1 works only on parts.</source>
      <translation>%1 仅能运作于零件上。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1711"/>
      <source>Shape of the selected Part is empty</source>
      <translation>所选零件的形状为空</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2022"/>
      <source>Please select only one feature in an active body.</source>
      <translation>请在一个活动的实体中仅选择一个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="68"/>
      <source>Part creation failed</source>
      <translation>零件创建失败</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="69"/>
      <source>Failed to create a part object.</source>
      <translation>创建零件对象失败。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="116"/>
      <location filename="../../CommandBody.cpp" line="121"/>
      <location filename="../../CommandBody.cpp" line="134"/>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>Bad base feature</source>
      <translation>不正确的基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="117"/>
      <source>Body can't be based on a PartDesign feature.</source>
      <translation>实体不能基于零件设计工作台特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="122"/>
      <source>%1 already belongs to a body, can't use it as base feature for another body.</source>
      <translation>%1 已经属于一个实体, 不能用它作为另一个实体的基础特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="135"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>基础特征 (%1) 录属于其他部件。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="159"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="163"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="167"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>所选形状仅由一个壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="171"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>所选形状由多个实体或壳体组成。
这可能会导致意外的结果。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="176"/>
      <source>Base feature</source>
      <translation>基础特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="184"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>实体基于的特征不能超过一个。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="198"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="347"/>
      <source>Nothing to migrate</source>
      <translation>没有可迁移的对象</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="348"/>
      <source>No PartDesign features found that don't belong to a body. Nothing to migrate.</source>
      <translation>没有找到不属于实体的零件设计工作台特征。没有可迁移对象。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="496"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>草图平面不能被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="497"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>请编辑 '%1'并使用基面或基准平面作为草绘平面来重新定义它。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="559"/>
      <location filename="../../CommandBody.cpp" line="563"/>
      <location filename="../../CommandBody.cpp" line="568"/>
      <location filename="../../CommandBody.cpp" line="861"/>
      <location filename="../../CommandBody.cpp" line="868"/>
      <source>Selection error</source>
      <translation>选择错误</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="560"/>
      <source>Select exactly one PartDesign feature or a body.</source>
      <translation>仅选择一个 PartDesign 特征或一个实体。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="564"/>
      <source>Couldn't determine a body for the selected feature '%s'.</source>
      <translation>无法确定所选特征 "%s" 的实体。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="569"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>只有实体特征才能成为实体的结算特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="687"/>
      <location filename="../../CommandBody.cpp" line="709"/>
      <location filename="../../CommandBody.cpp" line="724"/>
      <source>Features cannot be moved</source>
      <translation>特征无法被移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="688"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>一些选定的特征依赖于源实体</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="710"/>
      <source>Only features of a single source Body can be moved</source>
      <translation>只能移动单个源实体的特征</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="725"/>
      <source>There are no other bodies to move to</source>
      <translation>没有其他实体可以移动</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="862"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>无法移动实体的基础特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="869"/>
      <source>Select one or more features from the same body.</source>
      <translation>从同一实体上选择一个或多个特征。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="882"/>
      <source>Beginning of the body</source>
      <translation>实体的起始</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="946"/>
      <source>Dependency violation</source>
      <translation>依赖冲突</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="947"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>较早的特征不能依赖于较后的特征。

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="269"/>
      <source>No previous feature found</source>
      <translation>未找到之前的特征</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="270"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>如果没有可用的基础特征, 就不可能创建减料特征</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="445"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="204"/>
      <source>Vertical sketch axis</source>
      <translation>垂直草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="446"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="205"/>
      <source>Horizontal sketch axis</source>
      <translation>水平草绘轴</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="207"/>
      <source>Construction line %1</source>
      <translation>辅助线 %1</translation>
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
      <translation>想使用零件设计工作台, 您需要一个激活状态的实体对象。请 (双击) 激活或创建一个新的实体对象。

如果您的旧版文档中存在没有实体的零件设计工作台对象, 请使用零件设计工作台中的迁移功能将它们放入一个实体中。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="186"/>
      <source>Active Body Required</source>
      <translation>需要激活状态的实体</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="187"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document. Please make one active (double click) or create a new Body.</source>
      <translation>要创建新的 PartDesign 对象, 文档中必须有一个激活状态的实体。请 (双击) 激活或创建一个新的实体。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="222"/>
      <source>Feature is not in a body</source>
      <translation>特征不在实体内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="223"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的实体对象。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="255"/>
      <source>Feature is not in a part</source>
      <translation>特征不在零件内</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="256"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>要使用此特征, 它需隶属于文档中的零件对象。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="50"/>
      <location filename="../../ViewProviderTransformed.cpp" line="76"/>
      <location filename="../../ViewProvider.cpp" line="63"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="209"/>
      <source>Edit %1</source>
      <translation>编辑 %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="76"/>
      <source>Set colors...</source>
      <translation>设置颜色...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="62"/>
      <source>Edit boolean</source>
      <translation>编辑布尔运算</translation>
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
      <translation>线</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>坐标系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="229"/>
      <source>Edit datum</source>
      <translation>编辑基准</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="74"/>
      <source>Feature error</source>
      <translation>特征错误</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="75"/>
      <source>%1 misses a base feature.
This feature is broken and can't be edited.</source>
      <translation>%1 基本特征缺失。
此特征已损坏, 无法编辑。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="46"/>
      <source>Edit groove</source>
      <translation>编辑挖槽</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="61"/>
      <source>Edit hole</source>
      <translation>编辑孔</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="66"/>
      <source>Edit loft</source>
      <translation>编辑放样</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="46"/>
      <source>Edit pad</source>
      <translation>编辑凸台</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="74"/>
      <source>Edit pipe</source>
      <translation>编辑管</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="48"/>
      <source>Edit pocket</source>
      <translation>编辑凹槽</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="54"/>
      <source>Edit primitive</source>
      <translation>编辑图元</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="46"/>
      <source>Edit revolution</source>
      <translation>编辑旋转体</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="204"/>
      <source>Edit shape binder</source>
      <translation>编辑形状引用连接</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="316"/>
      <source>Synchronize</source>
      <translation>同步</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="318"/>
      <source>Select bound object</source>
      <translation>选择绑定对象</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="179"/>
      <source>One transformed shape does not intersect the support</source>
      <translation type="unfinished">One transformed shape does not intersect the support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="181"/>
      <source>%1 transformed shapes do not intersect the support</source>
      <translation type="unfinished">%1 transformed shapes do not intersect the support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="191"/>
      <source>Transformation succeeded</source>
      <translation>变换成功</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="140"/>
      <source>The document "%1" you are editing was designed with an old version of PartDesign workbench.</source>
      <translation>您正在编辑的文档 "%1" 是用旧版本的零件设计工作台设计的。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="143"/>
      <source>Do you want to migrate in order to use modern PartDesign features?</source>
      <translation>是否要迁移以使用时下的零件设计功能？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="146"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy PartDesign or have a slightly broken structure.</source>
      <translation>文档 "%1" 似乎正处于从旧版零件设计工作台迁移的过程中, 或有一个轻微的破碎结构。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Do you want to make the migration automatically?</source>
      <translation>想要让移动自动进行吗？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="152"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>注意: 如果您选择迁移, 您将无法使用旧的 FreeCAD 版本编辑该文件。如果你拒绝迁移, 你将无法使用新的零件设计工作台功能, 如实体和零部件。因此, 您也无法在装配工作台中使用您的零部件。但是以后您仍然可以用 "零件设计-迁移..."来完成迁移。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate manually</source>
      <translation>手动迁移</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="51"/>
      <source>Edit helix</source>
      <translation>编辑螺旋线</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="41"/>
      <source>Edit chamfer</source>
      <translation>编辑倒角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="42"/>
      <source>Edit draft</source>
      <translation type="unfinished">Edit draft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="41"/>
      <source>Edit fillet</source>
      <translation>编辑圆角</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="39"/>
      <source>Edit linear pattern</source>
      <translation>编辑线性阵列</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="39"/>
      <source>Edit mirrored</source>
      <translation>编辑镜像</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="48"/>
      <source>Edit multi-transform</source>
      <translation>编辑多重变换</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit polar pattern</source>
      <translation>编辑环形阵列</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="39"/>
      <source>Edit scaled</source>
      <translation>编辑缩放</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="41"/>
      <source>Edit thickness</source>
      <translation>编辑厚度</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket parameters</source>
      <translation>链轮参数</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth:</source>
      <translation>齿数：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket Reference</source>
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
      <source>Bicycle with Derailleur</source>
      <translation>有变速器的单车</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without Derailleur</source>
      <translation>无变速器的单车</translation>
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
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain Pitch:</source>
      <translation>链条节距：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 到</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain Roller Diameter:</source>
      <translation>链条滚刀直径：</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth Width:</source>
      <translation>齿宽:</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Task Hole Parameters</source>
      <translation>任务孔参数</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>轮廓</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="290"/>
      <source>Whether the hole gets a thread</source>
      <translation>孔是否具有螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Threaded</source>
      <translation>螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="447"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>孔是否具有模型化螺纹</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="450"/>
      <source>Model Thread</source>
      <translation>模型线程</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="466"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>实时更新对线程的更改
注意计算可能需要一些时间</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="483"/>
      <source>Customize thread clearance</source>
      <translation>自定义螺纹间隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="90"/>
      <source>Clearance</source>
      <translation>间隙</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="470"/>
      <source>Update thread view</source>
      <translation type="unfinished">Update thread view</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="486"/>
      <source>Custom Clearance</source>
      <translation type="unfinished">Custom Clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="499"/>
      <source>Custom Thread clearance value</source>
      <translation>自定义螺纹间隙大小</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="394"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="431"/>
      <source>Right hand</source>
      <translation>右旋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="410"/>
      <source>Left hand</source>
      <translation>左旋</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="63"/>
      <source>Size</source>
      <translation>大小</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="103"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>孔位
仅适用于无螺纹孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="108"/>
      <location filename="../../TaskHoleParameters.cpp" line="660"/>
      <source>Standard</source>
      <translation>标准</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="113"/>
      <location filename="../../TaskHoleParameters.cpp" line="663"/>
      <location filename="../../TaskHoleParameters.cpp" line="680"/>
      <source>Close</source>
      <translation>关闭</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <location filename="../../TaskHoleParameters.cpp" line="666"/>
      <source>Wide</source>
      <translation>宽度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>Class</source>
      <translation>种类</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="321"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>根据孔配置方案螺纹孔的公差等级</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="692"/>
      <source>Drill Point</source>
      <translation type="unfinished">Drill Point</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="134"/>
      <location filename="../../TaskHoleParameters.ui" line="556"/>
      <source>Diameter</source>
      <translation>直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="147"/>
      <source>Hole diameter</source>
      <translation>孔直径</translation>
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
      <translation>尺寸标注</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="190"/>
      <source>Through all</source>
      <translation>通过所有</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="350"/>
      <source>Hole depth</source>
      <translation>孔深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="360"/>
      <source>Tapped (DIN76)</source>
      <translation>螺纹 (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="543"/>
      <source>Cut type for screw heads</source>
      <translation>螺丝头的切割类型</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="665"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>检查以覆盖“类型”预定义的值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="668"/>
      <source>Custom values</source>
      <translation>自定义值</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation type="unfinished">For countersinks this is the depth of
the screw's top below the surface</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="530"/>
      <source>Hole Cut Type</source>
      <translation type="unfinished">Hole Cut Type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="633"/>
      <source>Countersink angle</source>
      <translation>埋头孔角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="705"/>
      <source>Flat</source>
      <translation>平头孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="721"/>
      <source>Angled</source>
      <translation>斜钻孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="758"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>钻孔点的大小将被计入
盲孔的深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="762"/>
      <source>Take into account for depth</source>
      <translation>考虑深度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="240"/>
      <source>Tapered</source>
      <translation>锥孔</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="255"/>
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
      <location filename="../../TaskHoleParameters.ui" line="224"/>
      <source>Reverses the hole direction</source>
      <translation>反转孔方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="227"/>
      <source>Reversed</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="677"/>
      <source>Normal</source>
      <translation>法向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="683"/>
      <source>Loose</source>
      <translation>宽松</translation>
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
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;草图</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;零件设计</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Create a datum</source>
      <translation>创建基准</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Create an additive feature</source>
      <translation>创建增料特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Create a subtractive feature</source>
      <translation>创建减料特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Apply a pattern</source>
      <translation>应用图样</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Apply a dress-up feature</source>
      <translation>应用修饰特征</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket...</source>
      <translation>链轮...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute gear...</source>
      <translation>渐开线齿轮...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Shaft design wizard</source>
      <translation>轴设计向导</translation>
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
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute gear...</source>
      <translation>渐开线齿轮...</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edit the involute gear definition.</source>
      <translation>创建或编辑渐开线齿轮定义。</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket...</source>
      <translation>链轮...</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edit the sprocket definition.</source>
      <translation>创建或编辑链轮定义。</translation>
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
      <source>Shaft wizard</source>
      <translation>轴向导</translation>
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
      <translation>添加一列</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation type="unfinished">Section %s</translation>
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
      <source>Missing module</source>
      <translation type="unfinished">Missing module</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>You may have to install the Plot add-on</source>
      <translation>您可能需要安装Plot组件</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft design wizard...</source>
      <translation>轴设计向导...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Start the shaft design wizard</source>
      <translation>启动轴设计向导</translation>
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
      <location filename="../../../App/Body.cpp" line="401"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation type="unfinished">Linked object is not a PartDesign feature</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="408"/>
      <source>Tip shape is empty</source>
      <translation type="unfinished">Tip shape is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="64"/>
      <source>BaseFeature link is not set</source>
      <translation type="unfinished">BaseFeature link is not set</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="69"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation type="unfinished">BaseFeature must be a Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="78"/>
      <source>BaseFeature has an empty shape</source>
      <translation type="unfinished">BaseFeature has an empty shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="82"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation type="unfinished">Cannot do boolean cut without BaseFeature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="96"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="125"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation type="unfinished">Cannot do boolean with anything but Part::Feature and its derivatives</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="103"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation type="unfinished">Cannot do boolean operation with invalid base shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="109"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation type="unfinished">Cannot do boolean on feature which is not in a body</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="135"/>
      <source>Base shape is null</source>
      <translation type="unfinished">Base shape is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="116"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="138"/>
      <source>Tool shape is null</source>
      <translation type="unfinished">Tool shape is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="154"/>
      <source>Unsupported boolean operation</source>
      <translation type="unfinished">Unsupported boolean operation</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="787"/>
      <location filename="../../../App/FeaturePipe.cpp" line="397"/>
      <location filename="../../../App/FeaturePipe.cpp" line="417"/>
      <location filename="../../../App/FeatureLoft.cpp" line="252"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="127"/>
      <source>Resulting shape is not a solid</source>
      <translation type="unfinished">Resulting shape is not a solid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="791"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="802"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="809"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="171"/>
      <location filename="../../../App/FeaturePipe.cpp" line="401"/>
      <location filename="../../../App/FeaturePipe.cpp" line="421"/>
      <location filename="../../../App/FeatureDraft.cpp" line="322"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="167"/>
      <location filename="../../../App/FeatureFillet.cpp" line="117"/>
      <location filename="../../../App/FeatureLoft.cpp" line="257"/>
      <location filename="../../../App/FeatureHole.cpp" line="1956"/>
      <location filename="../../../App/FeatureGroove.cpp" line="193"/>
      <source>Result has multiple solids: that is not currently supported.</source>
      <translation type="unfinished">Result has multiple solids: that is not currently supported.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="153"/>
      <source>Failed to create chamfer</source>
      <translation type="unfinished">Failed to create chamfer</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="319"/>
      <location filename="../../../App/FeatureFillet.cpp" line="98"/>
      <source>Resulting shape is null</source>
      <translation type="unfinished">Resulting shape is null</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Resulting shape is invalid</source>
      <translation type="unfinished">Resulting shape is invalid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="124"/>
      <source>No edges specified</source>
      <translation type="unfinished">No edges specified</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="240"/>
      <source>Size must be greater than zero</source>
      <translation type="unfinished">Size must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="249"/>
      <source>Size2 must be greater than zero</source>
      <translation type="unfinished">Size2 must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="254"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation type="unfinished">Angle must be greater than 0 and less than 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="315"/>
      <source>Failed to create draft</source>
      <translation type="unfinished">Failed to create draft</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="81"/>
      <source>Fillet not possible on selected shapes</source>
      <translation type="unfinished">Fillet not possible on selected shapes</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="88"/>
      <source>Fillet radius must be greater than zero</source>
      <translation type="unfinished">Fillet radius must be greater than zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="87"/>
      <source>Angle of groove too large</source>
      <translation type="unfinished">Angle of groove too large</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="91"/>
      <source>Angle of groove too small</source>
      <translation type="unfinished">Angle of groove too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1673"/>
      <location filename="../../../App/FeatureGroove.cpp" line="110"/>
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
      <location filename="../../../App/FeatureExtrude.cpp" line="482"/>
      <source>Length too small</source>
      <translation type="unfinished">Length too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="489"/>
      <source>Second length too small</source>
      <translation type="unfinished">Second length too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="523"/>
      <source>Failed to obtain profile shape</source>
      <translation type="unfinished">Failed to obtain profile shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="575"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation type="unfinished">Creation failed because direction is orthogonal to sketch's normal vector</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="633"/>
      <source>Extrude: Can only offset one face</source>
      <translation type="unfinished">Extrude: Can only offset one face</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="140"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="594"/>
      <location filename="../../../App/FeatureGroove.cpp" line="127"/>
      <source>Creating a face from sketch failed</source>
      <translation type="unfinished">Creating a face from sketch failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="673"/>
      <source>Up to face: Could not get SubShape!</source>
      <translation type="unfinished">Up to face: Could not get SubShape!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="701"/>
      <source>Unable to reach the selected shape, please select faces</source>
      <translation type="unfinished">Unable to reach the selected shape, please select faces</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="728"/>
      <source>Magnitude of taper angle matches or exceeds 90 degrees</source>
      <translation type="unfinished">Magnitude of taper angle matches or exceeds 90 degrees</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="739"/>
      <source>Padding with draft angle failed</source>
      <translation type="unfinished">Padding with draft angle failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="158"/>
      <location filename="../../../App/FeatureGroove.cpp" line="149"/>
      <source>Revolve axis intersects the sketch</source>
      <translation type="unfinished">Revolve axis intersects the sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="228"/>
      <location filename="../../../App/FeatureGroove.cpp" line="157"/>
      <source>Could not revolve the sketch!</source>
      <translation type="unfinished">Could not revolve the sketch!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="240"/>
      <location filename="../../../App/FeatureGroove.cpp" line="201"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation type="unfinished">Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="128"/>
      <source>Error: Pitch too small!</source>
      <translation type="unfinished">Error: Pitch too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="130"/>
      <location filename="../../../App/FeatureHelix.cpp" line="144"/>
      <source>Error: height too small!</source>
      <translation>错误：高度太小！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="136"/>
      <source>Error: pitch too small!</source>
      <translation type="unfinished">Error: pitch too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="138"/>
      <location filename="../../../App/FeatureHelix.cpp" line="146"/>
      <location filename="../../../App/FeatureHelix.cpp" line="152"/>
      <source>Error: turns too small!</source>
      <translation type="unfinished">Error: turns too small!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="156"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation type="unfinished">Error: either height or growth must not be zero!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="170"/>
      <source>Error: unsupported mode</source>
      <translation>错误：不支持的模式</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="182"/>
      <source>Error: No valid sketch or face</source>
      <translation type="unfinished">Error: No valid sketch or face</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="191"/>
      <source>Error: Face must be planar</source>
      <translation type="unfinished">Error: Face must be planar</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2196"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="298"/>
      <source>Error: Result is not a solid</source>
      <translation type="unfinished">Error: Result is not a solid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="246"/>
      <source>Error: There is nothing to subtract</source>
      <translation type="unfinished">Error: There is nothing to subtract</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="250"/>
      <location filename="../../../App/FeatureHelix.cpp" line="272"/>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: Result has multiple solids</source>
      <translation type="unfinished">Error: Result has multiple solids</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="261"/>
      <source>Error: Adding the helix failed</source>
      <translation type="unfinished">Error: Adding the helix failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="285"/>
      <source>Error: Intersecting the helix failed</source>
      <translation type="unfinished">Error: Intersecting the helix failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="292"/>
      <source>Error: Subtracting the helix failed</source>
      <translation type="unfinished">Error: Subtracting the helix failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="313"/>
      <source>Error: Could not create face from sketch</source>
      <translation type="unfinished">Error: Could not create face from sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1691"/>
      <source>Hole error: Creating a face from sketch failed</source>
      <translation type="unfinished">Hole error: Creating a face from sketch failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1716"/>
      <source>Hole error: Unsupported length specification</source>
      <translation type="unfinished">Hole error: Unsupported length specification</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>Hole error: Invalid hole depth</source>
      <translation type="unfinished">Hole error: Invalid hole depth</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1742"/>
      <source>Hole error: Invalid taper angle</source>
      <translation type="unfinished">Hole error: Invalid taper angle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1763"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation type="unfinished">Hole error: Hole cut diameter too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1767"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation type="unfinished">Hole error: Hole cut depth must be less than hole depth</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1771"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation type="unfinished">Hole error: Hole cut depth must be greater or equal to zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1793"/>
      <source>Hole error: Invalid countersink</source>
      <translation type="unfinished">Hole error: Invalid countersink</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1826"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation type="unfinished">Hole error: Invalid drill point angle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1836"/>
      <source>Hole error: Invalid drill point</source>
      <translation type="unfinished">Hole error: Invalid drill point</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1870"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation type="unfinished">Hole error: Could not revolve sketch</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1874"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation type="unfinished">Hole error: Resulting shape is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1884"/>
      <source>Error: Adding the thread failed</source>
      <translation type="unfinished">Error: Adding the thread failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1935"/>
      <location filename="../../../App/FeatureHole.cpp" line="1940"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation type="unfinished">Boolean operation failed on profile Edge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1946"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation type="unfinished">Boolean operation produced non-solid on profile Edge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="160"/>
      <source>Boolean operation failed</source>
      <translation>布尔操作失败</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1966"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation type="unfinished">Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2076"/>
      <source>Thread type out of range</source>
      <translation>螺线类型超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2079"/>
      <source>Thread size out of range</source>
      <translation>螺线大小超出范围</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2171"/>
      <source>Error: Thread could not be built</source>
      <translation type="unfinished">Error: Thread could not be built</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="141"/>
      <source>Loft: At least one section is needed</source>
      <translation type="unfinished">Loft: At least one section is needed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="269"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation type="unfinished">Loft: A fatal error occurred when making the loft</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="172"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation type="unfinished">Loft: Creating a face from sketch failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="205"/>
      <source>Loft: Failed to create shell</source>
      <translation type="unfinished">Loft: Failed to create shell</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="821"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation type="unfinished">Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="176"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation type="unfinished">Pipe: Could not obtain profile shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="181"/>
      <source>No spine linked</source>
      <translation type="unfinished">No spine linked</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="194"/>
      <source>No auxiliary spine linked.</source>
      <translation type="unfinished">No auxiliary spine linked.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="215"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation type="unfinished">Pipe: Only one isolated point is needed if using a sketch with isolated points for section</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="221"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation type="unfinished">Pipe: At least one section is needed when using a single point for profile</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="235"/>
      <source>Pipe: All sections need to be part features</source>
      <translation type="unfinished">Pipe: All sections need to be part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="241"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation type="unfinished">Pipe: Could not obtain section shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="250"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation type="unfinished">Pipe: Only the profile and last section can be vertices</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="259"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation type="unfinished">Multisections need to have the same amount of inner wires as the base section</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="286"/>
      <source>Path must not be a null shape</source>
      <translation type="unfinished">Path must not be a null shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="321"/>
      <source>Pipe could not be built</source>
      <translation type="unfinished">Pipe could not be built</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="366"/>
      <source>Result is not a solid</source>
      <translation type="unfinished">Result is not a solid</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="381"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation type="unfinished">Pipe: There is nothing to subtract from</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="392"/>
      <source>Adding the pipe failed</source>
      <translation type="unfinished">Adding the pipe failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="412"/>
      <source>Subtracting the pipe failed</source>
      <translation type="unfinished">Subtracting the pipe failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation type="unfinished">A fatal error occurred when making the pipe</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="559"/>
      <source>Invalid element in spine.</source>
      <translation type="unfinished">Invalid element in spine.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="562"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation type="unfinished">Element in spine is neither an edge nor a wire.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="575"/>
      <source>Spine is not connected.</source>
      <translation type="unfinished">Spine is not connected.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="579"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation type="unfinished">Spine is neither an edge nor a wire.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="583"/>
      <source>Invalid spine.</source>
      <translation type="unfinished">Invalid spine.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="95"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation type="unfinished">Cannot subtract primitive feature without base feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="241"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="113"/>
      <source>Unknown operation type</source>
      <translation type="unfinished">Unknown operation type</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="247"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="120"/>
      <source>Failed to perform boolean operation</source>
      <translation type="unfinished">Failed to perform boolean operation</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="197"/>
      <source>Length of box too small</source>
      <translation type="unfinished">Length of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="199"/>
      <source>Width of box too small</source>
      <translation type="unfinished">Width of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="201"/>
      <source>Height of box too small</source>
      <translation type="unfinished">Height of box too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="247"/>
      <source>Radius of cylinder too small</source>
      <translation type="unfinished">Radius of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="249"/>
      <source>Height of cylinder too small</source>
      <translation type="unfinished">Height of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="251"/>
      <source>Rotation angle of cylinder too small</source>
      <translation type="unfinished">Rotation angle of cylinder too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="304"/>
      <source>Radius of sphere too small</source>
      <translation type="unfinished">Radius of sphere too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="353"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="355"/>
      <source>Radius of cone cannot be negative</source>
      <translation type="unfinished">Radius of cone cannot be negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="357"/>
      <source>Height of cone too small</source>
      <translation type="unfinished">Height of cone too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="420"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="422"/>
      <source>Radius of ellipsoid too small</source>
      <translation type="unfinished">Radius of ellipsoid too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="504"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="506"/>
      <source>Radius of torus too small</source>
      <translation type="unfinished">Radius of torus too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="569"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation type="unfinished">Polygon of prism is invalid, must have 3 or more sides</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="571"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation type="unfinished">Circumradius of the polygon, of the prism, is too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="573"/>
      <source>Height of prism is too small</source>
      <translation type="unfinished">Height of prism is too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="654"/>
      <source>delta x of wedge too small</source>
      <translation type="unfinished">delta x of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="657"/>
      <source>delta y of wedge too small</source>
      <translation type="unfinished">delta y of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="660"/>
      <source>delta z of wedge too small</source>
      <translation type="unfinished">delta z of wedge too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="663"/>
      <source>delta z2 of wedge is negative</source>
      <translation type="unfinished">delta z2 of wedge is negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="666"/>
      <source>delta x2 of wedge is negative</source>
      <translation type="unfinished">delta x2 of wedge is negative</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="89"/>
      <source>Angle of revolution too large</source>
      <translation type="unfinished">Angle of revolution too large</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="95"/>
      <source>Angle of revolution too small</source>
      <translation type="unfinished">Angle of revolution too small</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="133"/>
      <source>Reference axis is invalid</source>
      <translation type="unfinished">Reference axis is invalid</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="780"/>
      <source>Fusion with base feature failed</source>
      <translation type="unfinished">Fusion with base feature failed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation type="unfinished">Transformation feature Linked object is not a Part object</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="117"/>
      <source>No originals linked to the transformed feature.</source>
      <translation type="unfinished">No originals linked to the transformed feature.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="277"/>
      <source>Cannot transform invalid support shape</source>
      <translation type="unfinished">Cannot transform invalid support shape</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="323"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation type="unfinished">Shape of additive/subtractive feature is empty</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="315"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation type="unfinished">Only additive and subtractive features can be transformed</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="101"/>
      <source>Invalid face reference</source>
      <translation type="unfinished">Invalid face reference</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="137"/>
      <source>Active body</source>
      <translation>活动实体</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2442"/>
      <source>Create datum</source>
      <translation>创建基准</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2443"/>
      <source>Create a datum object or local coordinate system</source>
      <translation>创建基准对象或局部坐标系统</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2477"/>
      <source>Create datum</source>
      <translation>创建基准</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2478"/>
      <source>Create a datum object or local coordinate system</source>
      <translation>创建基准对象或局部坐标系统</translation>
    </message>
  </context>
</TS>
