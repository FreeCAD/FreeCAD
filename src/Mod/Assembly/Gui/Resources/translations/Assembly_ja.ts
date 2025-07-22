<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>アセンブリを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>現在のドキュメントまたは (存在する場合) 現在のアクティブなアセンブリでアセンブリオブジェクトを作成します。ルートアセンブリはファイルに1つだけ作成できます。</translation>
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
      <translation>1 - アセンブリがアクティブな場合: ジョイントを作成し、2つのパーツを合わせて永続的に固定し、運動や回転を防止します。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="90"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - パーツがアクティブな場合: 選択した座標系に一致するサブパーツを配置。選択した2つ目のパーツは移動します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="112"/>
      <source>Create Revolute Joint</source>
      <translation>回転ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="119"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>回転ジョイントを作成: 選択した部品間の単一軸を中心に回転できます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="140"/>
      <source>Create Cylindrical Joint</source>
      <translation>円筒ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="147"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>円筒ジョイントの作成: 組み立て部品間で同じ軸に沿って移動を許可しながら、1つの軸に沿って回転できます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="166"/>
      <source>Create Slider Joint</source>
      <translation>スライダージョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="173"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>スライダージョイントを作成: 1つの軸に沿って線形移動を可能にしますが、選択した部品間の回転は制限されます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="192"/>
      <source>Create Ball Joint</source>
      <translation>ボールジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="199"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>ボールジョイントを作成: 複数パーツを1点で接続し、接点が接触している限りは自由な運動を許容します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="218"/>
      <source>Create Distance Joint</source>
      <translation>距離ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="225"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>距離ジョイントを作成: 選択したオブジェクト間の距離を固定します。</translation>
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
      <translation>接地状態の切り替え</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="509"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>パーツを接地するとアセンブリ内の位置が恒久的に固定され、任意の動きや回転を防止します。アセンブルを開始する前に少なくとも1つの接地パーツが必要です。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="46"/>
      <source>Export ASMT File</source>
      <translation>ASMTファイルをエクスポート</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="50"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>現在アクティブなアセンブリをASMTファイルとしてエクスポート。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="89"/>
      <source>Insert Component</source>
      <translation>コンポーネントを挿入</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="51"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>アクティブなアセンブリにコンポーネントを挿入。これによってパーツ、ボディー、プリミティブとアセンブリの間に動的リンクが作成されます。外部コンポーネントを挿入する場合はファイルが&lt;b&gt;現在のセッションで開かれている&lt;/b&gt;ことを確認してください。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>リスト内のアイテムを左クリックで挿入</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="55"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>リスト内のアイテムを右クリックで削除</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>コンポーネントのインスタンスを追加するにはビュー上でクリックしながら Shiftキーを押してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="50"/>
      <source>Solve Assembly</source>
      <translation>アセンブリを求解</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="57"/>
      <source>Solve the currently active assembly.</source>
      <translation>現在アクティブなアセンブリを求解。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>アセンブリ</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="127"/>
      <source>Active object</source>
      <translation>アクティブなオブジェクト</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="127"/>
      <source>Turn flexible</source>
      <translation>フレキシブルに切り替え</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="128"/>
      <source>Your sub-assembly is currently rigid. This will make it flexible instead.</source>
      <translation>サブアセンブリは現在リジッドです。これによって代わりにフレキシブルになります。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="132"/>
      <source>Turn rigid</source>
      <translation>リジッドに切り替え</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="133"/>
      <source>Your sub-assembly is currently flexible. This will make it rigid instead.</source>
      <translation>サブアセンブリは現在フレキシブルです。これによって代わりにリジッドになります。</translation>
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
      <translation>アセンブリ</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="110"/>
      <source>Assembly Joints</source>
      <translation>アセンブリ ジョイント</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="113"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assembly</translation>
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
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Cylindrical</source>
      <translation>円筒</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Slider</source>
      <translation>スライダー</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Ball</source>
      <translation>ボール</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <location filename="../../../JointObject.py" line="1516"/>
      <source>Distance</source>
      <translation>距離</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Parallel</source>
      <translation>平行</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Perpendicular</source>
      <translation>直角</translation>
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
      <translation>ラックピニオン</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Screw</source>
      <translation>スクリュー</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="61"/>
      <source>Gears</source>
      <translation>ギア</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="62"/>
      <source>Belt</source>
      <translation>ベルト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="624"/>
      <source>Broken link in: </source>
      <translation type="unfinished">Broken link in: </translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1360"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>2つの別々の部品から2つの要素を選択する必要があります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1520"/>
      <source>Radius 1</source>
      <translation>半径 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1522"/>
      <source>Pitch radius</source>
      <translation>ピッチ半径</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>尋ねる</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>常に</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>なし</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>インデックス (自動)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>名前 (自動)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>説明</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>ファイル名 (自動)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>数量 (自動)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>デフォルト</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>名前が重複しています。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>その名前はすでに使用されています。別の名前を選んでください。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>オプション:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>サブアセンブリの子要素: チェックされている場合、サブアセンブリの子要素が部品表に追加されます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>パーツの子要素: チェックされている場合、パーツの子要素が部品表に追加されます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>パーツのみ: チェックされている場合、パーツコンテナとサブアセンブリのみが部品表に追加されます。 PartDesignのボディー、留め具、Partワークベンチのプリミティブといったソリッドは無視されます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>列：</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>自動列: (インデックス、数量、名前...) が自動的に入力されます。変更はすべて上書きされます。これらの列の名前は変更できません。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation type="unfinished">Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Delキーを押すと、任意の列 (カスタムか、非カスタム) を削除できます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="415"/>
      <source>Export:</source>
      <translation>エクスポート:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="422"/>
      <source>The exported file format can be customized in the Spreadsheet workbench preferences.</source>
      <translation>エクスポートするファイル形式はスプレッドシートワークベンチの設定でカスタマイズできます。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="84"/>
      <source>Part name</source>
      <translation>パーツ名</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="89"/>
      <source>Part</source>
      <translation>パーツ</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="94"/>
      <source>Create part in new file</source>
      <translation>新しいファイルにパーツを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="101"/>
      <source>Joint new part origin</source>
      <translation>ジョイントの新しいパーツの原点</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="135"/>
      <source>Save Document</source>
      <translation>ドキュメントを保存</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="137"/>
      <source>Save</source>
      <translation>保存</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="140"/>
      <source>Don't link</source>
      <translation>リンクしない</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="474"/>
      <source>Enter your formula...</source>
      <translation>数式を入力してください...</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="527"/>
      <source>In capital are variables that you need to replace with actual values. More details about each example in it's tooltip.</source>
      <translation>大文字は実際の値に置き換える必要がある変数です。ツールチップ内に各例の詳しい説明があります。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="530"/>
      <source> - Linear: C + VEL*time</source>
      <translation> - 1次方程式: C + VEL*time</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="532"/>
      <source> - Quadratic: C + VEL*time + ACC*time^2</source>
      <translation> - 2次方程式: C + VEL*time + ACC*time^2</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="535"/>
      <source> - Harmonic: C + AMP*sin(VEL*time - PHASE)</source>
      <translation> - 調波: C + AMP*sin(VEL*time - PHASE)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="538"/>
      <source> - Exponential: C*exp(time/TIMEC)</source>
      <translation> - 指数: C*exp(time/TIMEC)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="544"/>
      <source> - Smooth Step: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(time - T0)))</source>
      <translation> - 平滑ステップ: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(time - T0)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="551"/>
      <source> - Smooth Square Impulse: (H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</source>
      <translation> - 平滑方形インパルス：(H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="558"/>
      <source> - Smooth Ramp Top Impulse: ((1/pi)*(arctan(1000*(time - T1)) - arctan(1000*(time - T2))))*(((H2 - H1)/(T2 - T1))*(time - T1) + H1)</source>
      <translation> - 平滑ランプトップインパルス: ((1/pi)*(arctan(1000*(time - T1)) - arctan(1000*(time - T2))))*(((H2 - H1)/(T2 - T1))*(time - T1) + H1)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="568"/>
      <source>C is a constant offset.
VEL is a velocity or slope or gradient of the straight line.</source>
      <translation>Cは定数オフセットです。
VELは速度、または直線の傾き、勾配です。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="576"/>
      <source>C is a constant offset.
VEL is the velocity or slope or gradient of the straight line.
ACC is the acceleration or coefficient of the second order. The function is a parabola.</source>
      <translation>Cは定数オフセットです。
VELは速度、または直線の傾き、勾配です。
ACCは加速度、または2次の係数です。この関数は放物線です。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="585"/>
      <source>C is a constant offset.
AMP is the amplitude of the sine wave.
VEL is the angular velocity in radians per second.
PHASE is the phase of the sine wave.</source>
      <translation>Cは定数オフセットです。
AMPは正弦波の振幅です。
VELはラジアン/秒単位の角速度です。
PHASEは正弦波の位相です。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="592"/>
      <source>C is a constant.
TIMEC is the time constant of the exponential function.</source>
      <translation>Cは定数です。
TIMECは指数関数の時定数です。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="600"/>
      <source>L1 is step level before time = T0.
L2 is step level after time = T0.
SLOPE defines the steepness of the transition between L1 and L2 about time = T0. Higher values gives sharper cornered steps. SLOPE = 1000 or greater are suitable.</source>
      <translation>L1は時間 = T0より前のステップ値です。
L2は時間 = T0より後のステップ値です。
SLOPE は時間 = T0付近でのL1とL2間の遷移の勾配を定義します。値が大きいほど角の鋭いステップになります。SLOPE = 1000以上が適しています。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="609"/>
      <source>H is the height of the impulse.
T1 is the start of the impulse.
T2 is the end of the impulse.
SLOPE defines the steepness of the transition between 0 and H about time = T1 and T2. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>Hはインパルスの高さです。
T1はインパルスの開始時間です。
T2はインパルスの終了時間です。
SLOPEは時間 = T1とT2付近での0とHの間の遷移の勾配を定義します。値が大きいほど角の鋭いインパルスになります。SLOPE = 1000以上が適しています。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="620"/>
      <source>This is similar to the square impulse but the top has a sloping ramp. It is good for building a smooth piecewise linear function by adding a series of these.
T1 is the start of the impulse.
T2 is the end of the impulse.
H1 is the height at T1 at the beginning of the ramp.
H2 is the height at T2 at the end of the ramp.
SLOPE defines the steepness of the transition between 0 and H1 and H2 to 0 about time = T1 and T2 respectively. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>方形インパルスに似ていますが、上部に傾斜ランプがあります。一連のランプを追加することで滑らかな区分線形関数の構築に適しています。
T1はインパルスの開始時間です。
T2はインパルスの終了時間です。
H1はランプの開始時のT1の高さです。
H2はランプの終了時のT2の高さです。
SLOPEはそれぞれ時間 = T1とT2付近での、0とH1の間、またH2から0への遷移の勾配を定義します。値が大きいほど角の鋭いインパルスになります。SLOPE = 1000以上が適しています。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="658"/>
      <location filename="../../../CommandCreateSimulation.py" line="675"/>
      <source>Help</source>
      <translation>ヘルプ</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="673"/>
      <source>Hide help</source>
      <translation>ヘルプを非表示</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="181"/>
      <source>The type of the joint</source>
      <translation>ジョイントのタイプ</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>The first reference of the joint</source>
      <translation>ジョイントの1つ目の参照</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="218"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>ジョイントに使用される Reference1 のオブジェクト内のローカル座標系です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <location filename="../../../JointObject.py" line="526"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>ジョイントの1番目のコネクターのアタッチメント・オフセットです。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="252"/>
      <source>The second reference of the joint</source>
      <translation>ジョイントの2つ目の参照</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>ジョイントに使用される Reference2 のオブジェクト内のローカル座標系です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="288"/>
      <location filename="../../../JointObject.py" line="537"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>ジョイントの2番目のコネクターのアタッチメント・オフセットです。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="445"/>
      <source>The first object of the joint</source>
      <translation>ジョイントの1つ目のオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="230"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Placement1 の再計算はされなくなり、カスタムでの位置設定が可能になります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="465"/>
      <source>The second object of the joint</source>
      <translation>ジョイントの2つ目のオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Placement2 の再計算はされなくなり、カスタムでの位置設定が可能になります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="301"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>ジョイントの距離。距離ジョイント、ラックピニオン (ピッチ半径)、スクリューとギアとベルト (半径1) でだけ使用されます。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="313"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>ジョイントの2つ目の距離。ギアジョイントでだけ2つ目の半径を格納するために使用されます。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="325"/>
      <source>This indicates if the joint is active.</source>
      <translation>ジョイントがアクティブかどうか</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="338"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>ジョイントの最小長さ制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="351"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>ジョイントの最大長さ制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="364"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>ジョイントの最小角度制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="377"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>ジョイントの最小長さを有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="390"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>両座標系の間の (Z軸に方向の) 長さの下限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="402"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>両座標系の間の (Z軸に方向の) 長さの上限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>両座標系の間の (X軸間の) 角度の下限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="426"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>両座標系の間の (X軸間の) 角度の上限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="479"/>
      <source>The {order} reference of the joint</source>
      <translation type="unfinished">The {order} reference of the joint</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="993"/>
      <source>The object to ground</source>
      <translation>接地オブジェクト</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
      <location filename="../../../CommandCreateView.py" line="291"/>
      <source>The objects moved by the move</source>
      <translation>運動によって移動するオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="266"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>これは運動の動きです。終了位置は開始位置 * この位置の結果となります。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="275"/>
      <source>The type of the move</source>
      <translation>運動のタイプ</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="107"/>
      <source>Simulation start time.</source>
      <translation>シミュレーションの開始時間</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="119"/>
      <source>Simulation end time.</source>
      <translation>シミュレーションの終了時間</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="131"/>
      <source>Simulation time step for output.</source>
      <translation>出力のシミュレーション時間刻み</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="143"/>
      <source>Integration global error tolerance.</source>
      <translation>グローバルな誤差許容値の積分値</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="155"/>
      <source>Frames Per Second.</source>
      <translation>1秒あたりのフレーム数</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="207"/>
      <source>The number of decimals to use for calculated texts</source>
      <translation>計算結果テキストで使用する小数点以下桁数</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="304"/>
      <source>The joint that is moved by the motion</source>
      <translation>運動によって動作するジョイント</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="316"/>
      <source>This is the formula of the motion. For example '1.0*time'.</source>
      <translation>これが運動式です。例えば「1.0*time」となります。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="325"/>
      <source>The type of the motion</source>
      <translation>運動のタイプ</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>距離</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>半径 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>オフセット</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="137"/>
      <source>Offset1</source>
      <translation>オフセット1</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="158"/>
      <source>Offset2</source>
      <translation>オフセット2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>By clicking this button, you can set the attachment offset of the first marker (coordinate system) of the joint.</source>
      <translation>このボタンをクリックすると、ジョイントの1つ目のマーカー (座標系) のアタッチメントオフセットを設定できます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="165"/>
      <source>By clicking this button, you can set the attachment offset of the second marker (coordinate system) of the joint.</source>
      <translation>このボタンをクリックすると、ジョイントの2つ目のマーカー (座標系) のアタッチメントオフセットを設定できます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="177"/>
      <source>Show advanced offsets</source>
      <translation>高度なオフセットを表示</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="193"/>
      <source>Reverse the direction of the joint.</source>
      <translation>ジョイントの方向を反転。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="196"/>
      <source>Reverse</source>
      <translation>反転</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Limits</source>
      <translation>制限</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Min length</source>
      <translation>最小長さ</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max length</source>
      <translation>最大長さ</translation>
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
      <translation>回転を反転</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>コンポーネントを挿入</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>パーツを検索...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>パーツが見つかりませんか？ </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>ファイルを開く</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>チェックすると、パーツだけがリストに表示されます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>パーツのみ表示</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="74"/>
      <source>Sets whether the inserted sub-assemblies will be rigid or flexible.
Rigid means that the added sub-assembly will be considered as a solid unit within the parent assembly.
Flexible means that the added sub-assembly will allow movement of its individual components' joints within the parent assembly.
You can change this behavior at any time by either right-clicking the sub-assembly on the document tree and toggling the
Turn rigid/Turn flexible command there, or by editing its Rigid property in the Property Editor.</source>
      <translation>挿入されたサブアセンブリがリジッドかフレキシブルかを設定します。
リジッドとは、追加されたサブアセンブリが親アセンブリ内でソリッド単位として扱われることを意味します。
フレキシブルとは、追加されたサブアセンブリが親アセンブリ内で個々のコンポーネントのジョイントを移動できることを意味します。
この動作は、ドキュメント ツリーでサブアセンブリを右クリックして「リジッドに切り替え」「フレキシブルに切り替え」のコマンド切り替えを行うか、プロパティエディターでRigidプロパティを編集することでいつでも変更できます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="81"/>
      <source>Rigid sub-assemblies</source>
      <translation>リジッドなサブアセンブリ</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allows leaving edit mode when pressing Esc button</source>
      <translation type="unfinished">Allows leaving edit mode when pressing Esc button</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Escで編集モードを終了</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>ソルバーのドラッグ中のステップを記録します。バグを報告したい場合に便利です。
ファイル名は「runPreDrag.asmt」「dragging.log」で、デフォルトでは std::ofstream のディレクトリ (Windowsではデスクトップ) に保存されます。</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>ドラッグ中のステップを記録</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>最初のパーツを接地:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>最初のパーツをアセンブリに挿入する際に、パーツを自動的に接地するかを選択できます。</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="196"/>
      <source>The object is associated to one or more joints.</source>
      <translation>オブジェクトは1つ以上のジョイントに関連付けられています。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="198"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>オブジェクトを移動して関連付けられているジョイントを削除しますか？</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="891"/>
      <source>Move part</source>
      <translation>パーツを移動</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="332"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>ラックとピニオンのジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="339"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>ラックとピニオンのジョイントを作成: スライドジョイントを設定されたパーツと回転ジョイントを設定されたパーツをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="344"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>回転ジョイントとスライドジョイントと同じ座標系を選択します。ピッチ半径によってラックとピニオンの間の移動比が定義されます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="363"/>
      <source>Create Screw Joint</source>
      <translation>スクリュージョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="370"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>スクリュージョイントを作成: スライドジョイントを設定されたパーツと回転ジョイントを設定されたパーツをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="375"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>回転ジョイントとスライドジョイントと同じ座標系を選択します。ピッチ半径によって回転スクリューとスライドするパーツの間の移動比が定義されます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="406"/>
      <location filename="../../../CommandCreateJoint.py" line="437"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>回転ジョイントと同じ座標系を選択してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="394"/>
      <source>Create Gears Joint</source>
      <translation>ギアジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="401"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>ギアジョイントを作成: 2つの回転ギアをリンクします。ギアは互いに逆の回転方向になります。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="425"/>
      <source>Create Belt Joint</source>
      <translation>ベルトジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="432"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>ベルトジョイントを作成: 2つの回転オブジェクトをリンクします。オブジェクトは同じ回転方向になります。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="457"/>
      <source>Create Gear/Belt Joint</source>
      <translation>ギア/ベルトジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="463"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>ギア/ベルトジョイントの作成: 2つの回転ギアをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="468"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>回転ジョイントと同じ座標系を選択してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="54"/>
      <source>Create Exploded View</source>
      <translation>分解ビューを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="61"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>現在のアセンブリの分解ビューを作成します。</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>分解ビューを作成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>チェックされている場合、パーツは単一のソリッドとして選択されます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>パーツを単一のソリッドとして扱う</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>ドラッグ配置</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>ドラッグ配置:
フィーチャーを選択します。
キャンセルするにはESCを押します。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>放射状に分解</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>部品表を作成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>チェックされている場合、サブアセンブリの子要素が部品表に追加されます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>サブアセンブリの子要素</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>チェックされている場合、パーツの子要素が部品表に追加されます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>パーツの子要素</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>チェックされている場合、パーツコンテナとサブアセンブリのみが部品表に追加されます。 PartDesignのボディー、留め具、Partワークベンチのプリミティブといったソリッドは無視されます。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>パーツのみ</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>列</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>列を追加</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>エクスポート</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>ヘルプ</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Create Parallel Joint</source>
      <translation>平行ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>平行ジョイントを作成: 選択した座標系のZ軸を平行にします。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="278"/>
      <source>Create Perpendicular Joint</source>
      <translation>直角ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="285"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>直角ジョイントを作成: 選択した座標系のZ軸を直角にします。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="304"/>
      <source>Create Angle Joint</source>
      <translation>角度ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="311"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>角度ジョイントを作成: 選択した座標系のZ軸間の角度を固定します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>部品表を作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>現在のアセンブリの部品表を作成します。 アセンブリがアクティブな場合は、このアセンブリの部品表になります。そうでない場合はドキュメント全体の部品表になります。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>BOMオブジェクトは、BOMの設定を保存するドキュメントオブジェクトです。 スプレッドシートオブジェクトでもあるのでBOMを簡単に可視化できます。BOMオブジェクトをドキュメントオブジェクトとして保存する必要がない場合は、たんにエクスポートしてこのタスクをキャンセルすることもできます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>「インデックス」、「名前」、「ファイル名」、「数量」の列は、再計算時に自動生成されます。「説明」列とカスタム列は上書きされません。</translation>
    </message>
  </context>
  <context>
    <name>Assembly::AssemblyLink</name>
    <message>
      <location filename="../../../App/AssemblyLink.cpp" line="492"/>
      <source>Joints</source>
      <translation>ジョイント</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="139"/>
      <source>Toggle Rigid</source>
      <translation>リジッドの切り替え</translation>
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
      <translation>アクティブなアセンブリに新しいパーツを挿入します。新しいパーツの原点をアセンブリ内に配置できます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateSimulation</name>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="67"/>
      <source>Create Simulation</source>
      <translation>シミュレーションを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="74"/>
      <source>Create a simulation of the current assembly.</source>
      <translation>現在のアセンブリのシミュレーションを作成します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_Insert</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="73"/>
      <source>Insert</source>
      <translation>挿入</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateSimulation</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="14"/>
      <source>Create Simulation</source>
      <translation>シミュレーションを作成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="20"/>
      <source>Motions</source>
      <translation>運動</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="50"/>
      <source>Add a prescribed motion</source>
      <translation>所定の運動を追加</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="70"/>
      <source>Delete selected motions</source>
      <translation>選択した運動を削除</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="89"/>
      <source>Simulation settings</source>
      <translation>シミュレーション設定</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="95"/>
      <source>Start</source>
      <translation>開始</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="98"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="105"/>
      <source>Start time of the simulation</source>
      <translation>シミュレーションの開始時間</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="112"/>
      <source>End</source>
      <translation>終了</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="115"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="122"/>
      <source>End time of the simulation</source>
      <translation>シミュレーションの終了時間</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="129"/>
      <source>Step</source>
      <translation>ステップ</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="132"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="139"/>
      <source>Time Step</source>
      <translation>時間刻み</translation>
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
      <translation>グローバルな誤差許容値</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="166"/>
      <source>Generate</source>
      <translation>生成</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="173"/>
      <source>Animation player</source>
      <translation>アニメーション再生</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="181"/>
      <source>Frame</source>
      <translation>フレーム</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="201"/>
      <source>0.00 s</source>
      <translation>0.00 秒</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="212"/>
      <source>Frames Per Second</source>
      <translation>1秒あたりのフレーム数</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="232"/>
      <source>Step backward</source>
      <translation>1ステップ戻る</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="252"/>
      <source>Play backward</source>
      <translation>逆再生</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="272"/>
      <source>Stop</source>
      <translation>停止</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="292"/>
      <source>Play forward</source>
      <translation>通常再生</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="312"/>
      <source>Step forward</source>
      <translation>1ステップ進む</translation>
    </message>
  </context>
</TS>
