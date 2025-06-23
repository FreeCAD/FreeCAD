<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-CN" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>创建装配体</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>在当前文档或当前的活跃的装配体（如果有）中创建装配对象。每个文件限定一个根装配体。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="77"/>
      <source>Create Fixed Joint</source>
      <translation type="unfinished">Create Fixed Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="84"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - 如果一个装配体是活动的：创建一个永久锁定两个零件的连接，以阻止任何的移动或旋转。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="90"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - 如果一个零件是活动的：通过匹配所选的坐标系来定位子零件。第二个选中的零件将移动。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="112"/>
      <source>Create Revolute Joint</source>
      <translation>创建旋转配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="119"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>创建一个旋转连接：允许在所选的零件之间围绕单轴旋转。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="140"/>
      <source>Create Cylindrical Joint</source>
      <translation>创建同轴配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="147"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>创建同轴配合：允许沿一个轴旋转，同时允许在装配的零件之间沿同一个轴移动。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="166"/>
      <source>Create Slider Joint</source>
      <translation>创建滑块配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="173"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>创建一个滑块配合：允许沿单轴线移动，但限制所选零件之间的旋转。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="192"/>
      <source>Create Ball Joint</source>
      <translation>创建球窝配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="199"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>创建一个球窝配合：把零件连接在一个点，允许围绕重合的连接点不受限制地转动。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="218"/>
      <source>Create Distance Joint</source>
      <translation>创建距离配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="225"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>创建一个距离配合：固定所选对象之间的距离。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="231"/>
      <source>Create one of several different joints based on the selection. For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation type="unfinished">Create one of several different joints based on the selection. For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="502"/>
      <source>Toggle grounded</source>
      <translation>固定切换</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="509"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>锁定零件在装配体中的位置，阻止任何移动或旋转。在开始装配前，你需要固定至少一个零件。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="46"/>
      <source>Export ASMT File</source>
      <translation>导出 ASMT 文件</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="50"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>将当前激活的装配体导出为 ASMT 文件。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="89"/>
      <source>Insert Component</source>
      <translation>插入零件</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="51"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>插入一个部件到激活的装配体中。这将创建一个到零件、实体、图元或装配体的动态链接。若要插入外部部件，需要确保&lt;b&gt;相应的文件已在当前会话中打开&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>左键单击列表中的项目以插入。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="55"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>右键单击列表中的项目以删除。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>在单击视图的同时按 Shift 键添加组件的多个实例。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="50"/>
      <source>Solve Assembly</source>
      <translation>解算装配体</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="57"/>
      <source>Solve the currently active assembly.</source>
      <translation>解算当前激活的装配体。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>装配</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="127"/>
      <source>Active object</source>
      <translation>活动对象</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="127"/>
      <source>Turn flexible</source>
      <translation>转为可动的</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="128"/>
      <source>Your sub-assembly is currently rigid. This will make it flexible instead.</source>
      <translation>你的子装配体目前是刚性的。这将使其变为可动的。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="132"/>
      <source>Turn rigid</source>
      <translation>转为刚体</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="133"/>
      <source>Your sub-assembly is currently flexible. This will make it rigid instead.</source>
      <translation>你的子装配体目前是可动的。这将使其变成刚体。</translation>
    </message>
    <message>
      <location filename="../../../App/BomObject.cpp" line="272"/>
      <source>N/A</source>
      <translation type="unfinished">N/A</translation>
    </message>
    <message>
      <location filename="../../../App/BomObject.cpp" line="296"/>
      <source>Not supported</source>
      <translation type="unfinished">Not supported</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly</source>
      <translation>装配</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="110"/>
      <source>Assembly Joints</source>
      <translation>装配连接</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="113"/>
      <source>&amp;Assembly</source>
      <translation>装配 (&amp;A)</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Revolute</source>
      <translation>旋转体</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Cylindrical</source>
      <translation>柱面</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Slider</source>
      <translation>滑块</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Ball</source>
      <translation>球窝</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <location filename="../../../JointObject.py" line="1516"/>
      <source>Distance</source>
      <translation>距离</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Parallel</source>
      <translation>平行</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Perpendicular</source>
      <translation>垂直</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <location filename="../../../JointObject.py" line="1518"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>RackPinion</source>
      <translation>齿轮齿条</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Screw</source>
      <translation>螺丝</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="61"/>
      <source>Gears</source>
      <translation>齿轮</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="62"/>
      <source>Belt</source>
      <translation>皮带</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="624"/>
      <source>Broken link in: </source>
      <translation type="unfinished">Broken link in: </translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1360"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>您需要从2个不同的零件中选择2个元素。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1520"/>
      <source>Radius 1</source>
      <translation>半径 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1522"/>
      <source>Pitch radius</source>
      <translation>节距半径</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>询问</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>总是</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>从不</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>索引 (自动)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>名称 (自动)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>描述</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>文件名（自动）</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>数量 (自动)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>默认</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>名称重复</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>此名称已被使用。请选择一个不同的名称。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>选项:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>子装配体零件：如果选中，子装配体的零件将会被添加到物料清单中。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>子零件：如果选中，子零件将会被添加到物料清单中。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>仅零件：若勾选，则仅零件容器和子组件将被添加到物料清单中。零件设计主体、紧固件或零件工作台原形等实体将被忽略。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>列：</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>自动列：（Index、Quantity、Name 等）由系统自动填充，您所做的任何修改都会被覆盖，这些列无法重命名。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation type="unfinished">Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>任何栏目（不论是否自定义）都可通过按下 Del 来删除。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="415"/>
      <source>Export:</source>
      <translation>导出：</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="422"/>
      <source>The exported file format can be customized in the Spreadsheet workbench preferences.</source>
      <translation>导出的文件格式可以在工作台首选项中自定义。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="84"/>
      <source>Part name</source>
      <translation>零件名称</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="89"/>
      <source>Part</source>
      <translation>零件</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="94"/>
      <source>Create part in new file</source>
      <translation>在新文件中创建零件</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="101"/>
      <source>Joint new part origin</source>
      <translation>连接新零件原点</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="135"/>
      <source>Save Document</source>
      <translation>保存文档</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="137"/>
      <source>Save</source>
      <translation>保存</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="140"/>
      <source>Don't link</source>
      <translation>不关联</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="474"/>
      <source>Enter your formula...</source>
      <translation>输入您的公式...</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="527"/>
      <source>In capital are variables that you need to replace with actual values. More details about each example in it's tooltip.</source>
      <translation>大写字母表示的是需要您替换为实际数值的变量，每个示例的更多详细信息请参阅其工具提示。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="530"/>
      <source> - Linear: C + VEL*time</source>
      <translation> - 线性：C + VEL*time</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="532"/>
      <source> - Quadratic: C + VEL*time + ACC*time^2</source>
      <translation> - 二次：C + VEL*time + ACC*time^2</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="535"/>
      <source> - Harmonic: C + AMP*sin(VEL*time - PHASE)</source>
      <translation> - 谐波：C + AMP*sin(VEL*time - PHASE)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="538"/>
      <source> - Exponential: C*exp(time/TIMEC)</source>
      <translation> - 指数：C*exp(time/TIMEC)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="544"/>
      <source> - Smooth Step: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(time - T0)))</source>
      <translation type="unfinished"> - Smooth Step: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(time - T0)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="551"/>
      <source> - Smooth Square Impulse: (H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</source>
      <translation type="unfinished"> - Smooth Square Impulse: (H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="558"/>
      <source> - Smooth Ramp Top Impulse: ((1/pi)*(arctan(1000*(time - T1)) - arctan(1000*(time - T2))))*(((H2 - H1)/(T2 - T1))*(time - T1) + H1)</source>
      <translation type="unfinished"> - Smooth Ramp Top Impulse: ((1/pi)*(arctan(1000*(time - T1)) - arctan(1000*(time - T2))))*(((H2 - H1)/(T2 - T1))*(time - T1) + H1)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="568"/>
      <source>C is a constant offset.
VEL is a velocity or slope or gradient of the straight line.</source>
      <translation>C 是固定偏移。
VEL 是直线的速度、斜率或梯度。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="576"/>
      <source>C is a constant offset.
VEL is the velocity or slope or gradient of the straight line.
ACC is the acceleration or coefficient of the second order. The function is a parabola.</source>
      <translation>C 是一个常数偏移量。
VEL 是直线的速度、斜率或梯度。
ACC 是加速度或二次项系数。该函数是一条抛物线。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="585"/>
      <source>C is a constant offset.
AMP is the amplitude of the sine wave.
VEL is the angular velocity in radians per second.
PHASE is the phase of the sine wave.</source>
      <translation>C 是一个恒定偏移量。
AMP 是正弦波的振幅。
VEL 是以弧度为单位的角速度。
PHASE 是正弦波的相位。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="592"/>
      <source>C is a constant.
TIMEC is the time constant of the exponential function.</source>
      <translation>C 是一个常量。
TIMEC 是指数函数的时间常量。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="600"/>
      <source>L1 is step level before time = T0.
L2 is step level after time = T0.
SLOPE defines the steepness of the transition between L1 and L2 about time = T0. Higher values gives sharper cornered steps. SLOPE = 1000 or greater are suitable.</source>
      <translation>L1 是时间= T0之前的一步级别。
L2 是时间= T0之后的步级别。
SLOPE 定义了L1 和 L2 之间关于时间= T0的过渡速度。 更高的数值给出清晰的相关步骤。SLOPE = 1000或更高的步骤是合适的。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="609"/>
      <source>H is the height of the impulse.
T1 is the start of the impulse.
T2 is the end of the impulse.
SLOPE defines the steepness of the transition between 0 and H about time = T1 and T2. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>H 是脉冲的高度。
T1 是脉冲的起始点。
T2 是脉冲的结束点。
SLOPE（斜率）定义了在时间等于 T1 和 T2 时，从 0 到 H 的过渡的陡峭程度。数值越高，脉冲的拐角越尖锐。SLOPE 等于 1000 或更大为宜。 </translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="620"/>
      <source>This is similar to the square impulse but the top has a sloping ramp. It is good for building a smooth piecewise linear function by adding a series of these.
T1 is the start of the impulse.
T2 is the end of the impulse.
H1 is the height at T1 at the beginning of the ramp.
H2 is the height at T2 at the end of the ramp.
SLOPE defines the steepness of the transition between 0 and H1 and H2 to 0 about time = T1 and T2 respectively. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>这类似于方波脉冲，但其顶部为倾斜的斜坡。通过添加一系列此类脉冲，可用于构建平滑的分段线性函数。
T1 是脉冲的起始时间。
T2 是脉冲的结束时间。
H1 是斜坡起始点（T1 处）的高度。
H2 是斜坡结束点（T2 处）的高度。
SLOPE 定义了在 time = T1 和 T2 附近，从 0 到 H1、从 H2 到 0 之间过渡的陡峭程度。SLOPE 值越大，脉冲的边角越尖锐。将 SLOPE 设为 1000 或更大比较合适。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="658"/>
      <location filename="../../../CommandCreateSimulation.py" line="675"/>
      <source>Help</source>
      <translation>帮助</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="673"/>
      <source>Hide help</source>
      <translation>隐藏帮助</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="181"/>
      <source>The type of the joint</source>
      <translation>接头类型</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>The first reference of the joint</source>
      <translation>配合的第一参考</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="218"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>这是 参考1 对象内将被用于配合的局部坐标系。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <location filename="../../../JointObject.py" line="526"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>这是配合的第一个连接器的附着偏移。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="252"/>
      <source>The second reference of the joint</source>
      <translation>配合的第二参考</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>这是 参考2 对象内将被用于配合的局部坐标系。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="288"/>
      <location filename="../../../JointObject.py" line="537"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>这是配合的第二个连接器的附着偏移。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="445"/>
      <source>The first object of the joint</source>
      <translation>配合的第一个对象</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="230"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>这将阻止 Placement1 重新计算，从而允许对其位置进行自定义设置。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="465"/>
      <source>The second object of the joint</source>
      <translation>配合的第二个对象</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>这将阻止 Placement2 重新计算，从而允许对其位置进行自定义设置。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="301"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>这是配合的距离。仅在距离配合、齿轮条配合（节距半径）、螺纹配合、齿轮和皮带配合（半径1）中使用</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="313"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>这是配合的第二个距离。仅在齿轮配合中作为第二半径。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="325"/>
      <source>This indicates if the joint is active.</source>
      <translation>这表示此配合是否已激活。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="338"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>启用此配合的最小长度限制。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="351"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>启用此配合的最大长度限制。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="364"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>启用此配合的最小角度限制。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="377"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>启用此配合的最小长度。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="390"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>这是两个坐标系统之间长度的最小限制（沿其Z轴）。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="402"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>这是两个坐标系统之间长度的最大限制（沿其Z轴）。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>这是两个坐标系统之间角度的最小限制（沿其X轴）。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="426"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>这是两个坐标系统之间角度的最大限制（沿其X轴）。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="479"/>
      <source>The {order} reference of the joint</source>
      <translation type="unfinished">The {order} reference of the joint</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="993"/>
      <source>The object to ground</source>
      <translation>要固定的对象</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
      <location filename="../../../CommandCreateView.py" line="291"/>
      <source>The objects moved by the move</source>
      <translation>对象已被移动操作移位</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="266"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>这是移动操作的位移部分，结束位置由起始位置与此位置相乘得到。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="275"/>
      <source>The type of the move</source>
      <translation>移动类型</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="107"/>
      <source>Simulation start time.</source>
      <translation>模拟开始时间。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="119"/>
      <source>Simulation end time.</source>
      <translation>模拟结束时间。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="131"/>
      <source>Simulation time step for output.</source>
      <translation>输出模拟时间步长。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="143"/>
      <source>Integration global error tolerance.</source>
      <translation>整合全局公差。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="155"/>
      <source>Frames Per Second.</source>
      <translation>帧率。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="207"/>
      <source>The number of decimals to use for calculated texts</source>
      <translation>用于计算文字的小数位数</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="304"/>
      <source>The joint that is moved by the motion</source>
      <translation>被运动移位的关节</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="316"/>
      <source>This is the formula of the motion. For example '1.0*time'.</source>
      <translation>这是运动的公式。例如“1.0*time”。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="325"/>
      <source>The type of the motion</source>
      <translation>运动类型</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>创建配合</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>距离</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>半径 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>偏移</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>旋转</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="137"/>
      <source>Offset1</source>
      <translation>偏移1</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="158"/>
      <source>Offset2</source>
      <translation>偏移2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>By clicking this button, you can set the attachment offset of the first marker (coordinate system) of the joint.</source>
      <translation>单击此按钮，您可以设置配合的第一个标记（坐标系统）的附着偏移量。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="165"/>
      <source>By clicking this button, you can set the attachment offset of the second marker (coordinate system) of the joint.</source>
      <translation>单击此按钮，您可以设置配合的第二个标记（坐标系统）的附着偏移量。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="177"/>
      <source>Show advanced offsets</source>
      <translation>显示高级偏移量</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="193"/>
      <source>Reverse the direction of the joint.</source>
      <translation>反转配合方向。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="196"/>
      <source>Reverse</source>
      <translation>反转</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Limits</source>
      <translation>限位</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Min length</source>
      <translation>最小长度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max length</source>
      <translation>最大长度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="259"/>
      <source>Min angle</source>
      <translation>最小角度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="288"/>
      <source>Max angle</source>
      <translation>最大角度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="320"/>
      <source>Reverse rotation</source>
      <translation>反向旋转</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>插入零件</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>搜索零件...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>找不到您的零件？ </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>打开文件</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>如果选中，列表将只显示零件。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>只显示零件</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="74"/>
      <source>Sets whether the inserted sub-assemblies will be rigid or flexible.
Rigid means that the added sub-assembly will be considered as a solid unit within the parent assembly.
Flexible means that the added sub-assembly will allow movement of its individual components' joints within the parent assembly.
You can change this behavior at any time by either right-clicking the sub-assembly on the document tree and toggling the
Turn rigid/Turn flexible command there, or by editing its Rigid property in the Property Editor.</source>
      <translation>设置子装配体将是刚性的还是可动的。
刚性的意味着子装配体将被视为不可动的刚体。
可动的意味着允许子装配体内部组件依据其配合关系移动。
你可以通过右键单击文档树中的子装配体并点击 转为刚体/转为可动的 选项或编辑它的刚性属性随时修改它。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="81"/>
      <source>Rigid sub-assemblies</source>
      <translation>刚性子装配体</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>常规</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allows leaving edit mode when pressing Esc button</source>
      <translation type="unfinished">Allows leaving edit mode when pressing Esc button</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Esc 离开编辑模式</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation type="unfinished">Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>记录拖拽步骤</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>固定第一个零件：</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>当你向装配体中插入第一个零件时，你可以选择自动固定这个零件。</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="196"/>
      <source>The object is associated to one or more joints.</source>
      <translation>该对象与一个或多个配合有关联。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="198"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>您想要移动对象并删除关联的配合吗？</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="891"/>
      <source>Move part</source>
      <translation>移动零件</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="332"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>创建齿轮条配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="339"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>创建齿轮条配合：连接带有滑块配合的零件和带有旋转配合的零件。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="344"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>选择与旋转和滑块配合相同的坐标系。节距半径定义了齿条与齿轮之间的运动比率。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="363"/>
      <source>Create Screw Joint</source>
      <translation>创建螺纹配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="370"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>创建螺纹配合：將一個帶有滑動接頭的零件與一個帶有旋轉接頭的零件連結起來。将一个带有滑块配合的零件与一个带有旋转配合的零件连接起来。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="375"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>选择与旋转配合和滑块配合相同的坐标系。节距半径定义了旋转螺纹零件与滑动零件之间的运动比率。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="406"/>
      <location filename="../../../CommandCreateJoint.py" line="437"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>选择与旋转配合相同的坐标系。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="394"/>
      <source>Create Gears Joint</source>
      <translation>创建齿轮配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="401"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>创建齿轮配合：将两个旋转器件连接在一起。它们将具有相反的旋转方向。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="425"/>
      <source>Create Belt Joint</source>
      <translation>创建皮带配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="432"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>创建皮带配合：将两个旋转器件连接在一起。它们将具有相同的旋转方向。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="457"/>
      <source>Create Gear/Belt Joint</source>
      <translation>创建齿轮/皮带配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="463"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>创建一个齿轮/皮带配合：将两个旋转器件连接在一起。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="468"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>选择与旋转配合相同的坐标系统。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="54"/>
      <source>Create Exploded View</source>
      <translation>创建分解视图</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="61"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>创建当前装配体的分解视图。</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>创建分解视图</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>如果勾选，零件将作为单个实体被选取。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>零件作为单个实体</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>对齐拖动点</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>对齐拖动点：
选择一个特征。
按下 ESC 取消。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>径向分解</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>创建物料清单</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>如果选中，子装配的零件将被添加到物料清单中。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>包含子装配零件</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>如果选中，实体的子级将被添加到物料清单中。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>包含实体子级</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>若勾选，则仅零件容器和子组件将被添加到物料清单中。零件设计主体、紧固件或零件工作台原形等实体将被忽略。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>只有零件</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>列</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>添加一列</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>导出</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>帮助</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Create Parallel Joint</source>
      <translation>创建平行配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>创建平行配合：使选定坐标系的 Z 轴平行。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="278"/>
      <source>Create Perpendicular Joint</source>
      <translation>创建垂直配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="285"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>创建一个垂直配合：使选定坐标系的 Z 轴之间互相垂直。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="304"/>
      <source>Create Angle Joint</source>
      <translation>创建角度配合</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="311"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>创建一个角度配合：固定选定坐标系的 Z 轴之间的角度。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>创建物料清单</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>创建当前装配体的物料清单。如果有装配体为已激活的，这将是该装配体的物料清单。否则它将是整个文档的物料清单。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation type="unfinished">The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation type="unfinished">The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</translation>
    </message>
  </context>
  <context>
    <name>Assembly::AssemblyLink</name>
    <message>
      <location filename="../../../App/AssemblyLink.cpp" line="492"/>
      <source>Joints</source>
      <translation>关节</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="139"/>
      <source>Toggle Rigid</source>
      <translation>切换刚性</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertNewPart</name>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="54"/>
      <source>Insert New Part</source>
      <translation type="unfinished">Insert New Part</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="61"/>
      <source>Insert a new part into the active assembly. The new part's origin can be positioned in the assembly.</source>
      <translation>将一个新零件插入到当前激活的装配体中。新零件的原点可以在装配体中定位。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateSimulation</name>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="67"/>
      <source>Create Simulation</source>
      <translation>创建模拟</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="74"/>
      <source>Create a simulation of the current assembly.</source>
      <translation>创建当前装配体的模拟。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_Insert</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="73"/>
      <source>Insert</source>
      <translation>插入</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateSimulation</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="14"/>
      <source>Create Simulation</source>
      <translation>创建模拟</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="20"/>
      <source>Motions</source>
      <translation>运动</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="50"/>
      <source>Add a prescribed motion</source>
      <translation>添加指定的运动</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="70"/>
      <source>Delete selected motions</source>
      <translation>删除选定的运动</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="89"/>
      <source>Simulation settings</source>
      <translation>模拟设置</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="95"/>
      <source>Start</source>
      <translation>开始</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="98"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="105"/>
      <source>Start time of the simulation</source>
      <translation>模拟开始时间</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="112"/>
      <source>End</source>
      <translation>结束</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="115"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="122"/>
      <source>End time of the simulation</source>
      <translation>模拟结束时间</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="129"/>
      <source>Step</source>
      <translation>步</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="132"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="139"/>
      <source>Time Step</source>
      <translation>时间步长</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="146"/>
      <source>Tolerance</source>
      <translation>公差</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="149"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="156"/>
      <source>Global Error Tolerance</source>
      <translation>全局容错</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="166"/>
      <source>Generate</source>
      <translation>生成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="173"/>
      <source>Animation player</source>
      <translation>动画播放器</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="181"/>
      <source>Frame</source>
      <translation>框架</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="201"/>
      <source>0.00 s</source>
      <translation>0.00 秒</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="212"/>
      <source>Frames Per Second</source>
      <translation>帧率</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="232"/>
      <source>Step backward</source>
      <translation>向后一步</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="252"/>
      <source>Play backward</source>
      <translation>向后播放</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="272"/>
      <source>Stop</source>
      <translation>停止</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="292"/>
      <source>Play forward</source>
      <translation>向前播放</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="312"/>
      <source>Step forward</source>
      <translation>向前一步</translation>
    </message>
  </context>
</TS>
