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
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>Create a Fixed Joint</source>
      <translation>固定ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="88"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - アセンブリがアクティブな場合: ジョイントを作成し、2つのパーツを合わせて永続的に固定し、運動や回転を防止します。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="94"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - パーツがアクティブな場合: 選択した座標系に一致するサブパーツを配置。選択した2つ目のパーツは移動します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="116"/>
      <source>Create Revolute Joint</source>
      <translation>回転ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="123"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>回転ジョイントを作成: 選択した部品間の単一軸を中心に回転できます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="144"/>
      <source>Create Cylindrical Joint</source>
      <translation>円筒ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="151"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>円筒ジョイントの作成: 組み立て部品間で同じ軸に沿って移動を許可しながら、1つの軸に沿って回転できます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="170"/>
      <source>Create Slider Joint</source>
      <translation>スライダージョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="177"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>スライダージョイントを作成: 1つの軸に沿って線形移動を可能にしますが、選択した部品間の回転は制限されます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="196"/>
      <source>Create Ball Joint</source>
      <translation>ボールジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="203"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>ボールジョイントを作成: 複数パーツを1点で接続し、接点が接触している限りは自由な運動を許容します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="222"/>
      <source>Create Distance Joint</source>
      <translation>距離ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="229"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>距離ジョイントを作成: 選択したオブジェクト間の距離を固定します。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="235"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>選択に基づいて複数の異なるジョイントのいずれかを作成します。例えば平面と円筒の間の距離を0とするには正接ジョイントを作成します。平面の間の距離を0とするとそれらを同一平面上します。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Toggle grounded</source>
      <translation>接地状態の切り替え</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="513"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>パーツを接地するとアセンブリ内の位置が恒久的に固定され、任意の動きや回転を防止します。アセンブルを開始する前に少なくとも1つの接地パーツが必要です。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>ASMTファイルをエクスポート</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="51"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>現在アクティブなアセンブリをASMTファイルとしてエクスポート。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>コンポーネントを挿入</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>アクティブなアセンブリにコンポーネントを挿入。これによってパーツ、ボディー、プリミティブとアセンブリの間に動的リンクが作成されます。外部コンポーネントを挿入する場合はファイルが&lt;b&gt;現在のセッションで開かれている&lt;/b&gt;ことを確認してください。</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>リスト内のアイテムを左クリックで挿入</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>リスト内のアイテムを右クリックで削除</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>コンポーネントのインスタンスを追加するにはビュー上でクリックしながら Shiftキーを押してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>アセンブリを求解</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
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
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly</source>
      <translation>アセンブリ</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation>アセンブリ ジョイント</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assembly</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="46"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="47"/>
      <source>Revolute</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Cylindrical</source>
      <translation>円筒</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Slider</source>
      <translation>スライダー</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Ball</source>
      <translation>ボール</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="1582"/>
      <source>Distance</source>
      <translation>距離</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Parallel</source>
      <translation>平行</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Perpendicular</source>
      <translation>直角</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <location filename="../../../JointObject.py" line="1584"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>RackPinion</source>
      <translation>ラックピニオン</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Screw</source>
      <translation>スクリュー</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Gears</source>
      <translation>ギア</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Belt</source>
      <translation>ベルト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1433"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>2つの別々の部品から2つの要素を選択する必要があります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1586"/>
      <source>Radius 1</source>
      <translation>半径 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1588"/>
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
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>カスタム列: 「列を追加」をクリックして追加する「説明」やその他のカスタム列はデータが上書きされません。ダブルクリックまたはF2キーを押すとこれらの列の名前を変更できます (列の名前を変更すると現在のデータは失われます)。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Delキーを押すと、任意の列 (カスタムか、非カスタム) を削除できます。</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="188"/>
      <source>The type of the joint</source>
      <translation>ジョイントのタイプ</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="212"/>
      <location filename="../../../JointObject.py" line="462"/>
      <source>The first reference of the joint</source>
      <translation>ジョイントの1つ目の参照</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="223"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>ジョイントに使用される Reference1 のオブジェクト内のローカル座標系です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="245"/>
      <location filename="../../../JointObject.py" line="521"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>ジョイントの1番目のコネクターのアタッチメント・オフセットです。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="254"/>
      <location filename="../../../JointObject.py" line="487"/>
      <source>The second reference of the joint</source>
      <translation>ジョイントの2つ目の参照</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="265"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>ジョイントに使用される Reference2 のオブジェクト内のローカル座標系です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="287"/>
      <location filename="../../../JointObject.py" line="531"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>ジョイントの2番目のコネクターのアタッチメント・オフセットです。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="432"/>
      <source>The first object of the joint</source>
      <translation>ジョイントの1つ目のオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="234"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Placement1 の再計算はされなくなり、カスタムでの位置設定が可能になります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="451"/>
      <source>The second object of the joint</source>
      <translation>ジョイントの2つ目のオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Placement2 の再計算はされなくなり、カスタムでの位置設定が可能になります。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="299"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>ジョイントの距離。距離ジョイント、ラックピニオン (ピッチ半径)、スクリューとギアとベルト (半径1) でだけ使用されます。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="310"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>ジョイントの2つ目の距離。ギアジョイントでだけ2つ目の半径を格納するために使用されます。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="321"/>
      <source>This indicates if the joint is active.</source>
      <translation>ジョイントがアクティブかどうか</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="333"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>ジョイントの最小長さ制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="345"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>ジョイントの最大長さ制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="357"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>ジョイントの最小角度制限を有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="369"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>ジョイントの最小長さを有効にします。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="381"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>両座標系の間の (Z軸に方向の) 長さの下限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="392"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>両座標系の間の (Z軸に方向の) 長さの上限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="403"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>両座標系の間の (X軸間の) 角度の下限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>両座標系の間の (X軸間の) 角度の上限です。</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1088"/>
      <source>The object to ground</source>
      <translation>接地オブジェクト</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1100"/>
      <source>This is where the part is grounded.</source>
      <translation>これはパーツが接地される場所です。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="248"/>
      <location filename="../../../CommandCreateView.py" line="282"/>
      <source>The objects moved by the move</source>
      <translation>運動によって移動するオブジェクト</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>これは運動の動きです。終了位置は開始位置 * この位置の結果となります。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="267"/>
      <source>The type of the move</source>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="141"/>
      <source>Reverse the direction of the joint.</source>
      <translation>ジョイントの方向を反転。</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>Reverse</source>
      <translation>反転</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="155"/>
      <source>Limits</source>
      <translation>制限</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="161"/>
      <source>Min length</source>
      <translation>最小長さ</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="184"/>
      <source>Max length</source>
      <translation>最大長さ</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Min angle</source>
      <translation>最小角度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max angle</source>
      <translation>最大角度</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="268"/>
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
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Esc キーの押下で編集モードを終了</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Esc で編集モードを終了</translation>
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
      <location filename="../../ViewProviderAssembly.cpp" line="180"/>
      <source>The object is associated to one or more joints.</source>
      <translation>オブジェクトは1つ以上のジョイントに関連付けられています。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="182"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>オブジェクトを移動して関連付けられているジョイントを削除しますか？</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="871"/>
      <source>Move part</source>
      <translation>パーツを移動</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="336"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>ラックとピニオンのジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>ラックとピニオンのジョイントを作成: スライドジョイントを設定されたパーツと回転ジョイントを設定されたパーツをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="348"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>回転ジョイントとスライドジョイントと同じ座標系を選択します。ピッチ半径によってラックとピニオンの間の移動比が定義されます。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="367"/>
      <source>Create Screw Joint</source>
      <translation>スクリュージョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>スクリュージョイントを作成: スライドジョイントを設定されたパーツと回転ジョイントを設定されたパーツをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="379"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>回転ジョイントとスライドジョイントと同じ座標系を選択します。ピッチ半径によって回転スクリューとスライドするパーツの間の移動比が定義されます。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="410"/>
      <location filename="../../../CommandCreateJoint.py" line="441"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>回転ジョイントと同じ座標系を選択してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="398"/>
      <source>Create Gears Joint</source>
      <translation>ギアジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>ギアジョイントを作成: 2つの回転ギアをリンクします。ギアは互いに逆の回転方向になります。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="429"/>
      <source>Create Belt Joint</source>
      <translation>ベルトジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>ベルトジョイントを作成: 2つの回転オブジェクトをリンクします。オブジェクトは同じ回転方向になります。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="461"/>
      <source>Create Gear/Belt Joint</source>
      <translation>ギア/ベルトジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>ギア/ベルトジョイントの作成: 2つの回転ギアをリンクします。</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="472"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>回転ジョイントと同じ座標系を選択してください。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>分解ビューを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
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
      <location filename="../../../CommandCreateJoint.py" line="254"/>
      <source>Create Parallel Joint</source>
      <translation>平行ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="261"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>平行ジョイントを作成: 選択した座標系のZ軸を平行にします。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Perpendicular Joint</source>
      <translation>直角ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>直角ジョイントを作成: 選択した座標系のZ軸を直角にします。</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="308"/>
      <source>Create Angle Joint</source>
      <translation>角度ジョイントを作成</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="315"/>
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
</TS>
