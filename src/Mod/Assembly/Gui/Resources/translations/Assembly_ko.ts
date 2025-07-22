<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ko" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>조립품 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>현재 문서 또는 활성화 되어있는 조립품에서 새로운 조립 객체를 만듭니다. 파일당 하나의 상위 조립품만 가능합니다.</translation>
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
      <translation>1 - 만약 조립품이 활성화된 경우 : 두 부품을 영구적으로 고정하여 움직임이나 회전을 잠그는 관절을 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="90"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - 만약 부품이 활성화된 경우 : 선택한 좌표계를 일치시켜 하위 부품과 맞춥니다. 두번째로 선택한 부품이 이동합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="112"/>
      <source>Create Revolute Joint</source>
      <translation>체인 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="119"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>회전 관절 생성: 선택한 부품들이 하나의 축을 기준으로 회전합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="140"/>
      <source>Create Cylindrical Joint</source>
      <translation>원통 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="147"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>원통 관절 생성: 조립된 부품들이 동일한 축을 따라 회전합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="166"/>
      <source>Create Slider Joint</source>
      <translation>슬라이더 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="173"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>슬라이더 관절 생성: 선택한 부품간 선형 이동은 가능하지만, 부품간 회전은 불가능합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="192"/>
      <source>Create Ball Joint</source>
      <translation>원형 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="199"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>원형 관절 생성: 부품들을 한 점에서 연결하여, 연결되어 있는 한 제한 없이 움직일 수 있습니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="218"/>
      <source>Create Distance Joint</source>
      <translation>거리 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="225"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>거리 관절 생성: 선택한 부품들 사이의 거리를 고정합니다.</translation>
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
      <translation>고정 상태 전환</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="509"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>부품을 영구적으로 조립품에 고정하여 움직임 및 회전을 막습니다. 조립하기 전 최소 한 개 이상의 고정된 부품이 필요합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="46"/>
      <source>Export ASMT File</source>
      <translation>ASMT 파일 내보내기</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="50"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>현재 작업중인 조립품을 ASMT 파일로 내보냅니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="89"/>
      <source>Insert Component</source>
      <translation>부품 삽입</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="51"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>부품을 활성화 된 조립품에 삽입합니다. 삽입된 부품은 부품, 몸통, 기본 도형 및 조립품을 동적으로 연결합니다. 외부 부품을 삽입하려면, 해당 파일이 &lt;b&gt;open in the current session&lt;/b&gt; 인지 확인하세요.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>마우스 왼쪽 클릭으로 리스트의 아이템들을 삽입합니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="55"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>마우스 오른쪽 클릭으로 리스트의 아이템들을 삭제합니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>뷰에서 Shift 키를 누르고 여러 부품 개체들을 동시에 선택합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="50"/>
      <source>Solve Assembly</source>
      <translation>조립품 솔브</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="57"/>
      <source>Solve the currently active assembly.</source>
      <translation>현재 활성화 되어 있는 조립품들을 솔브합니다.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation type="unfinished">Assembly</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="127"/>
      <source>Active object</source>
      <translation>객체 활성화</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="127"/>
      <source>Turn flexible</source>
      <translation>유연하게 전환</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="128"/>
      <source>Your sub-assembly is currently rigid. This will make it flexible instead.</source>
      <translation>조립품이 고정되어 있다면 상태를 유연하게 전환합니다.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="132"/>
      <source>Turn rigid</source>
      <translation>고정 상태로 전환</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="133"/>
      <source>Your sub-assembly is currently flexible. This will make it rigid instead.</source>
      <translation>조립품이 유연한 상태일 경우 고정 상태로 전환합니다.</translation>
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
      <translation>조립체</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="110"/>
      <source>Assembly Joints</source>
      <translation>조립품 관절</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="113"/>
      <source>&amp;Assembly</source>
      <translation>&amp;조립품</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Fixed</source>
      <translation>고정됨</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Revolute</source>
      <translation>회전</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Cylindrical</source>
      <translation>원통형</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Slider</source>
      <translation>슬라이더</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Ball</source>
      <translation>구형</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <location filename="../../../JointObject.py" line="1516"/>
      <source>Distance</source>
      <translation type="unfinished">Distance</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Parallel</source>
      <translation>평행</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Perpendicular</source>
      <translation>직교</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <location filename="../../../JointObject.py" line="1518"/>
      <source>Angle</source>
      <translation>각</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>RackPinion</source>
      <translation>랙 피니언</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Screw</source>
      <translation>나사</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="61"/>
      <source>Gears</source>
      <translation>기어</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="62"/>
      <source>Belt</source>
      <translation>체인</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="624"/>
      <source>Broken link in: </source>
      <translation type="unfinished">Broken link in: </translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1360"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>2개의 부품에서 2개의 요소를 선택해주세요.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1520"/>
      <source>Radius 1</source>
      <translation>반지름 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1522"/>
      <source>Pitch radius</source>
      <translation>피치 반지름</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>요청</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>항상</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>거절</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>번호 (자동)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>이름 (자동)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>설명</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>파일 이름 (자동)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>수량 (자동)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>기본값</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>중복된 이름</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>이 이름은 이미 사용중입니다. 다른 이름을 사용해 주세요.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>옵션:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>하위 조립품: 선택 시 조립된 하위 조립품들이 재료 목록에 추가됩니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>하위 부품: 선택 시 하위 부품들이 재료 목록에 추가됩니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>단일 부품 : 선택 시 단일 그룹과 하위 조립품들은 재료 목록에 추가됩니다. 몸통 부품, 조립 부품, 기본 작업대 부품과 같은 솔리드는 무시됩니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>열:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>자동 정렬 : (번호, 수량, 이름...)이 정렬됩니다. 수정한 내용은 모두 덮어 씌워집니다. 열의 이름은 변경할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation type="unfinished">Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Del 키를 눌러 열(사용자 정의 여부에 상관 없이)을 삭제할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="415"/>
      <source>Export:</source>
      <translation type="unfinished">Export:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="422"/>
      <source>The exported file format can be customized in the Spreadsheet workbench preferences.</source>
      <translation type="unfinished">The exported file format can be customized in the Spreadsheet workbench preferences.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="84"/>
      <source>Part name</source>
      <translation type="unfinished">Part name</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="89"/>
      <source>Part</source>
      <translation>부품</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="94"/>
      <source>Create part in new file</source>
      <translation type="unfinished">Create part in new file</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="101"/>
      <source>Joint new part origin</source>
      <translation type="unfinished">Joint new part origin</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="135"/>
      <source>Save Document</source>
      <translation>문서 저장</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="137"/>
      <source>Save</source>
      <translation>저장</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="140"/>
      <source>Don't link</source>
      <translation type="unfinished">Don't link</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="474"/>
      <source>Enter your formula...</source>
      <translation type="unfinished">Enter your formula...</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="527"/>
      <source>In capital are variables that you need to replace with actual values. More details about each example in it's tooltip.</source>
      <translation type="unfinished">In capital are variables that you need to replace with actual values. More details about each example in it's tooltip.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="530"/>
      <source> - Linear: C + VEL*time</source>
      <translation type="unfinished"> - Linear: C + VEL*time</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="532"/>
      <source> - Quadratic: C + VEL*time + ACC*time^2</source>
      <translation type="unfinished"> - Quadratic: C + VEL*time + ACC*time^2</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="535"/>
      <source> - Harmonic: C + AMP*sin(VEL*time - PHASE)</source>
      <translation type="unfinished"> - Harmonic: C + AMP*sin(VEL*time - PHASE)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="538"/>
      <source> - Exponential: C*exp(time/TIMEC)</source>
      <translation type="unfinished"> - Exponential: C*exp(time/TIMEC)</translation>
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
      <translation type="unfinished">C is a constant offset.
VEL is a velocity or slope or gradient of the straight line.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="576"/>
      <source>C is a constant offset.
VEL is the velocity or slope or gradient of the straight line.
ACC is the acceleration or coefficient of the second order. The function is a parabola.</source>
      <translation type="unfinished">C is a constant offset.
VEL is the velocity or slope or gradient of the straight line.
ACC is the acceleration or coefficient of the second order. The function is a parabola.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="585"/>
      <source>C is a constant offset.
AMP is the amplitude of the sine wave.
VEL is the angular velocity in radians per second.
PHASE is the phase of the sine wave.</source>
      <translation type="unfinished">C is a constant offset.
AMP is the amplitude of the sine wave.
VEL is the angular velocity in radians per second.
PHASE is the phase of the sine wave.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="592"/>
      <source>C is a constant.
TIMEC is the time constant of the exponential function.</source>
      <translation type="unfinished">C is a constant.
TIMEC is the time constant of the exponential function.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="600"/>
      <source>L1 is step level before time = T0.
L2 is step level after time = T0.
SLOPE defines the steepness of the transition between L1 and L2 about time = T0. Higher values gives sharper cornered steps. SLOPE = 1000 or greater are suitable.</source>
      <translation type="unfinished">L1 is step level before time = T0.
L2 is step level after time = T0.
SLOPE defines the steepness of the transition between L1 and L2 about time = T0. Higher values gives sharper cornered steps. SLOPE = 1000 or greater are suitable.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="609"/>
      <source>H is the height of the impulse.
T1 is the start of the impulse.
T2 is the end of the impulse.
SLOPE defines the steepness of the transition between 0 and H about time = T1 and T2. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation type="unfinished">H is the height of the impulse.
T1 is the start of the impulse.
T2 is the end of the impulse.
SLOPE defines the steepness of the transition between 0 and H about time = T1 and T2. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="620"/>
      <source>This is similar to the square impulse but the top has a sloping ramp. It is good for building a smooth piecewise linear function by adding a series of these.
T1 is the start of the impulse.
T2 is the end of the impulse.
H1 is the height at T1 at the beginning of the ramp.
H2 is the height at T2 at the end of the ramp.
SLOPE defines the steepness of the transition between 0 and H1 and H2 to 0 about time = T1 and T2 respectively. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation type="unfinished">This is similar to the square impulse but the top has a sloping ramp. It is good for building a smooth piecewise linear function by adding a series of these.
T1 is the start of the impulse.
T2 is the end of the impulse.
H1 is the height at T1 at the beginning of the ramp.
H2 is the height at T2 at the end of the ramp.
SLOPE defines the steepness of the transition between 0 and H1 and H2 to 0 about time = T1 and T2 respectively. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="658"/>
      <location filename="../../../CommandCreateSimulation.py" line="675"/>
      <source>Help</source>
      <translation>도움말</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="673"/>
      <source>Hide help</source>
      <translation>도움말 숨기기</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="181"/>
      <source>The type of the joint</source>
      <translation>관절 종류</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>The first reference of the joint</source>
      <translation>관절의 첫 번째 기준 위치</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="218"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>이 좌표계은 참조1의 객체의 관절에 사용될 지역 좌표계 입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <location filename="../../../JointObject.py" line="526"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>이 편차는 관절의 첫 그룹의 부착 편차입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="252"/>
      <source>The second reference of the joint</source>
      <translation>관절의 두 번째 기준 위치</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>이 좌표계는 참조2의 객체의 관절에 사용될 지역 좌표계 입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="288"/>
      <location filename="../../../JointObject.py" line="537"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>이 편차는 관절의 두 번째 그룹의 부착 편차입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="445"/>
      <source>The first object of the joint</source>
      <translation>관절의 첫 번째 객체</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="230"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>이것은 배치1이 다시 계산되지 않아, 자유롭게 재배치 할 수 있게 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="465"/>
      <source>The second object of the joint</source>
      <translation>관절의 두 번째 객체</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>이것은 배치2가 다시 계산되지 않아, 자유롭게 재배치 할 수 있게 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="301"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>이것은 관절 사이의 거리 입니다. 이것은 거리 관절, 랙 및 피니언(피치 반지름), 나사, 기어 및 체인(반지름1)에만 사용할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="313"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>이것은 두 번째 관절 사이의 거리 입니다. 이것은 기어 관절의 두 번째 반지름의 길이를 저장하는 데에만 사용할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="325"/>
      <source>This indicates if the joint is active.</source>
      <translation>이것은 관절이 활성화 되어 있다면 표시됩니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="338"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>관절의 최소 길이 제한을 활성화 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="351"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>관절의 최대 길이 제한을 활성화 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="364"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>관절의 최소 각도 제한을 활성화 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="377"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>관절의 최소 길이를 활성화 합니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="390"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>이것은 (Z축을 기준으로) 두 좌표계 사이의 최소 길이 제한입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="402"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>이것은 (Z축을 기준으로) 두 좌표계 사이의 최대 길이 제한입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>이것은 (X축을 기준으로) 두 좌표계 사이의 최소 각도 제한입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="426"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>이것은 (X축을 기준으로) 두 좌표계 사이의 최대 각도 제한입니다.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="479"/>
      <source>The {order} reference of the joint</source>
      <translation type="unfinished">The {order} reference of the joint</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="993"/>
      <source>The object to ground</source>
      <translation>고정할 객체</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
      <location filename="../../../CommandCreateView.py" line="291"/>
      <source>The objects moved by the move</source>
      <translation>이 오브젝트는 이동 동작에 의해 배치되었습니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="266"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>이동 동작에 의한 배치입니다. 최종 배치는 시작 위치 * 현재 위치 입니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="275"/>
      <source>The type of the move</source>
      <translation>이동 방식</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="107"/>
      <source>Simulation start time.</source>
      <translation type="unfinished">Simulation start time.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="119"/>
      <source>Simulation end time.</source>
      <translation type="unfinished">Simulation end time.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="131"/>
      <source>Simulation time step for output.</source>
      <translation type="unfinished">Simulation time step for output.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="143"/>
      <source>Integration global error tolerance.</source>
      <translation type="unfinished">Integration global error tolerance.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="155"/>
      <source>Frames Per Second.</source>
      <translation type="unfinished">Frames Per Second.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="207"/>
      <source>The number of decimals to use for calculated texts</source>
      <translation>계산된 텍스트에 사용할 소수점 수</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="304"/>
      <source>The joint that is moved by the motion</source>
      <translation type="unfinished">The joint that is moved by the motion</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="316"/>
      <source>This is the formula of the motion. For example '1.0*time'.</source>
      <translation type="unfinished">This is the formula of the motion. For example '1.0*time'.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="325"/>
      <source>The type of the motion</source>
      <translation type="unfinished">The type of the motion</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>관절 생성</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>거리</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>반지름 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>편차</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>회전</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="137"/>
      <source>Offset1</source>
      <translation>편차1</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="158"/>
      <source>Offset2</source>
      <translation>편차2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>By clicking this button, you can set the attachment offset of the first marker (coordinate system) of the joint.</source>
      <translation type="unfinished">By clicking this button, you can set the attachment offset of the first marker (coordinate system) of the joint.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="165"/>
      <source>By clicking this button, you can set the attachment offset of the second marker (coordinate system) of the joint.</source>
      <translation type="unfinished">By clicking this button, you can set the attachment offset of the second marker (coordinate system) of the joint.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="177"/>
      <source>Show advanced offsets</source>
      <translation type="unfinished">Show advanced offsets</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="193"/>
      <source>Reverse the direction of the joint.</source>
      <translation>관절의 방향 뒤집기</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="196"/>
      <source>Reverse</source>
      <translation>뒤집기</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Limits</source>
      <translation>제한</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Min length</source>
      <translation>최소 길이</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max length</source>
      <translation>최대 길이</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="259"/>
      <source>Min angle</source>
      <translation>최소 각</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="288"/>
      <source>Max angle</source>
      <translation>최대 각</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="320"/>
      <source>Reverse rotation</source>
      <translation>역 회전</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>부품 삽입</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>부품 찾기...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>찾는 부품이 없나요? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>파일 열기</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>체크 시, 리스트에 부품만 보여집니다.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>부품만 보기</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="74"/>
      <source>Sets whether the inserted sub-assemblies will be rigid or flexible.
Rigid means that the added sub-assembly will be considered as a solid unit within the parent assembly.
Flexible means that the added sub-assembly will allow movement of its individual components' joints within the parent assembly.
You can change this behavior at any time by either right-clicking the sub-assembly on the document tree and toggling the
Turn rigid/Turn flexible command there, or by editing its Rigid property in the Property Editor.</source>
      <translation type="unfinished">Sets whether the inserted sub-assemblies will be rigid or flexible.
Rigid means that the added sub-assembly will be considered as a solid unit within the parent assembly.
Flexible means that the added sub-assembly will allow movement of its individual components' joints within the parent assembly.
You can change this behavior at any time by either right-clicking the sub-assembly on the document tree and toggling the
Turn rigid/Turn flexible command there, or by editing its Rigid property in the Property Editor.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="81"/>
      <source>Rigid sub-assemblies</source>
      <translation>하위 조립품 고정</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>일반</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allows leaving edit mode when pressing Esc button</source>
      <translation type="unfinished">Allows leaving edit mode when pressing Esc button</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>편집 모드를 종료하려면 Esc 를 누르세요</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>솔버의 드래그 내역을 기록합니다. 버그를 리포트 할 경우 유용합니다.
(데스크탑의 Windows에서는)기본 폴더인 std::ofstream 폴더의 "runPreDrag.asmt" 및 "dragging.log" 파일에 기록됩니다.</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>드래그 내역 기록</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>고정된 첫 부품:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>조립품의 첫 부품을 삽입할 경우, 자동적으로 부품이 고정되도록 선택할 수 있습니다.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="196"/>
      <source>The object is associated to one or more joints.</source>
      <translation>이 오브젝트는 하나 이상의 관절로 연결되어 있습니다.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="198"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>관절 연결을 삭제하고 이 오브젝트를 이동시키겠습니까?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="891"/>
      <source>Move part</source>
      <translation>부품 이동</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="332"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>랙 및 피니언 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="339"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>랙 및 피니언 관절 생성: 슬라이드 관절이 있는 부품과 회전 관절이 있는 부품을 연결합니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="344"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>회전 관절 및 슬라이드 관절과 동일한 좌표계를 선택합니다. 피치 반지름은 랙 및 피니언 사이의 이동에 대한 비율료 정의됩니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="363"/>
      <source>Create Screw Joint</source>
      <translation>나사 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="370"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>나사 관절 생성: 슬라이드 관절이 있는 부품과 회전 관절이 있는 부품을 연결합니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="375"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>회전 관절 및 슬라이드 관절과 동일한 좌표계를 선택합니다. 피치 반지름은 나사 관절과 슬라이드 관절 사이의 회전에 대한 비율료 정의됩니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="406"/>
      <location filename="../../../CommandCreateJoint.py" line="437"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>회전 관절과 동일한 좌표계를 선택합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="394"/>
      <source>Create Gears Joint</source>
      <translation>기어 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="401"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>기어 관절 생성: 두 개의 기어 관절을 연결합니다. 두 개의 기어 관절은 서로 다른 방향으로 회전합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="425"/>
      <source>Create Belt Joint</source>
      <translation>체인 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="432"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>체인 관절 생성: 두 개의 회전하는 오브젝트를 서로 연결합니다. 두 개의 회전하는 오브젝트는 같은 방향으로 회전합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="457"/>
      <source>Create Gear/Belt Joint</source>
      <translation>기어/체인 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="463"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>기어/체인 관절 생성: 두 개의 회전 기어를 함께 연결</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="468"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>회전 관절은 동일한 좌표계의 관절을 선택해야 합니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="54"/>
      <source>Create Exploded View</source>
      <translation>부품 분해도 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="61"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>현재 조힙품의 부품 분해도를 생성합니다.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>부품 분해도 생성</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>선택 시, 부품은 단일 솔리드로 선택될 것입니다.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>단일 솔리드 부품</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>드래그 정렬</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>드래그 정렬:
기능을 선택합니다.
취소하려면 ESC 키를 누르십시오.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>방사형으로 분해</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>재료 목록 생성</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>선택 시, 하위 조립품들은 재료 목록에 추가됩니다.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>하위 조립품</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>선택 시, 하위 부품들은 재료 목록에 추가됩니다.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>하위 부품</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>선택 시, 부품 그룹과 자식 조립품들은 재료 목록에 추가됩니다. 몸통 부품, 조립 부품, 기본 작업대 부품과 같은 솔리드는 무시됩니다.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>단일 부품</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>열</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>열 추가</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>내보내기</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>도움말</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Create Parallel Joint</source>
      <translation>평행 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>평행 관절 생성: 선택한 좌표계들의 Z축을 평행하게 만듭니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="278"/>
      <source>Create Perpendicular Joint</source>
      <translation>직교 관절 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="285"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>직교 관절 생성: 선택한 좌표계들의 Z축이 직교하도록 만듭니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="304"/>
      <source>Create Angle Joint</source>
      <translation>각 관절</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="311"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>각 관절 생성: 선택한 좌표계들의 Z축 사이의 각도를 고정시킵니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>재료 목록 생성</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>현재 조립품의 재료 목록을 생성합니다. 만약 조립품이 활성화 되어있는 상태일 경우, 해당 조립품의 재료 목록이 됩니다. 비활성화 되어있는 상태일 경우 전체 문서의 재료 목록이 됩니다.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>재료 목록 오브젝트는 재료 목록의 설정을 저장하는 문서입니다. 또한 쉽게 볼 수 있는 표 형태의 오브젝트 입니다. 재료 목록 오브젝트가 필요하지 않다면, 이 작업을 취소하십시오.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>'번호', '이름', '파일 이름' 그리고 '수량' 열은 재계산 시 자동 생성됩니다. '설명' 및 '사용자 정의 열'은 덮어 씌워지지 않습니다.</translation>
    </message>
  </context>
  <context>
    <name>Assembly::AssemblyLink</name>
    <message>
      <location filename="../../../App/AssemblyLink.cpp" line="492"/>
      <source>Joints</source>
      <translation>관절들</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="139"/>
      <source>Toggle Rigid</source>
      <translation>고정 상태 전환</translation>
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
      <translation type="unfinished">Insert a new part into the active assembly. The new part's origin can be positioned in the assembly.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateSimulation</name>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="67"/>
      <source>Create Simulation</source>
      <translation type="unfinished">Create Simulation</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="74"/>
      <source>Create a simulation of the current assembly.</source>
      <translation type="unfinished">Create a simulation of the current assembly.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_Insert</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="73"/>
      <source>Insert</source>
      <translation>삽입</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateSimulation</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="14"/>
      <source>Create Simulation</source>
      <translation type="unfinished">Create Simulation</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="20"/>
      <source>Motions</source>
      <translation type="unfinished">Motions</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="50"/>
      <source>Add a prescribed motion</source>
      <translation type="unfinished">Add a prescribed motion</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="70"/>
      <source>Delete selected motions</source>
      <translation type="unfinished">Delete selected motions</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="89"/>
      <source>Simulation settings</source>
      <translation type="unfinished">Simulation settings</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="95"/>
      <source>Start</source>
      <translation>시작</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="98"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="105"/>
      <source>Start time of the simulation</source>
      <translation type="unfinished">Start time of the simulation</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="112"/>
      <source>End</source>
      <translation type="unfinished">End</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="115"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="122"/>
      <source>End time of the simulation</source>
      <translation type="unfinished">End time of the simulation</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="129"/>
      <source>Step</source>
      <translation type="unfinished">Step</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="132"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="139"/>
      <source>Time Step</source>
      <translation type="unfinished">Time Step</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="146"/>
      <source>Tolerance</source>
      <translation>공차</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="149"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="156"/>
      <source>Global Error Tolerance</source>
      <translation type="unfinished">Global Error Tolerance</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="166"/>
      <source>Generate</source>
      <translation type="unfinished">Generate</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="173"/>
      <source>Animation player</source>
      <translation type="unfinished">Animation player</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="181"/>
      <source>Frame</source>
      <translation>프레임</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="201"/>
      <source>0.00 s</source>
      <translation type="unfinished">0.00 s</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="212"/>
      <source>Frames Per Second</source>
      <translation type="unfinished">Frames Per Second</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="232"/>
      <source>Step backward</source>
      <translation type="unfinished">Step backward</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="252"/>
      <source>Play backward</source>
      <translation type="unfinished">Play backward</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="272"/>
      <source>Stop</source>
      <translation>중지</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="292"/>
      <source>Play forward</source>
      <translation type="unfinished">Play forward</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="312"/>
      <source>Step forward</source>
      <translation type="unfinished">Step forward</translation>
    </message>
  </context>
</TS>
