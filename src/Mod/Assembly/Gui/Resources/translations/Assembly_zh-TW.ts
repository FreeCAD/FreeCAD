<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_TW" sourcelanguage="en_US">
    <extra-po-header-language>zh_TW</extra-po-header-language>
    <extra-po-header-language_team></extra-po-header-language_team>
    <extra-po-header-last_translator>DrBenson &lt;Benson.Dr@GMail.com&gt;</extra-po-header-last_translator>
    <extra-po-header-po_revision_date></extra-po-header-po_revision_date>
    <extra-po-header-pot_creation_date></extra-po-header-pot_creation_date>
    <extra-po-header-project_id_version>FreeCAD Assembly Language Translations v0.22_dev</extra-po-header-project_id_version>
    <extra-po-header-x_generator>Poedit 3.2.2</extra-po-header-x_generator>
    <extra-po-header_comment>#
# Translators:
# DrBenson &lt;Benson.Dr@GMail.com&gt;, 2023
#
    </extra-po-header_comment>
    <extra-po-headers>Project-Id-Version,POT-Creation-Date,PO-Revision-Date,Last-Translator,Language-Team,Language,MIME-Version,Content-Type,Content-Transfer-Encoding,Plural-Forms,X-Language,X-Source-Language,X-Qt-Contexts,X-Generator</extra-po-headers>
<context>
    <name>App::Property</name>
    <message>
        <location filename="../../../JointObject.py" line="64"/>
        <source>The type of the joint</source>
        <translation>接點類型</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="74"/>
        <source>The first object of the joint</source>
        <translation>接點的第一個物體</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="81"/>
        <source>The selected element of the first object</source>
        <translation>第一個物體的選定元素</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="88"/>
        <source>The selected vertex of the first object</source>
        <translation>第一個物體的選取頂點</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="95"/>
        <source>This is the local coordinate system within the object1 that will be used to joint.</source>
        <translation>這是物體1的局部座標系，將用於接點.</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="106"/>
        <source>The second object of the joint</source>
        <translation>接點的第二個物體</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="113"/>
        <source>The selected element of the second object</source>
        <translation>第二個物體的選定元素</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="120"/>
        <source>The selected vertex of the second object</source>
        <translation>第二個物體的選取頂點</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="127"/>
        <source>This is the local coordinate system within the object2 that will be used to joint.</source>
        <translation>這是物體2的局部座標系，將用於接點.</translation>
    </message>
</context>
<context>
    <name>Assembly</name>
    <message>
        <location filename="../../../CommandCreateAssembly.py" line="45"/>
        <source>Create Assembly</source>
        <translation>建立零件組合</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="55"/>
        <source>Create Fixed Joint</source>
        <translation>建立固定接點</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="63"/>
        <source>Loading Assembly workbench...</source>
        <translation>載入零件組合工作台...</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="63"/>
        <source>Initializing Assembly workbench...</source>
        <translation>初始化零件組合工作台...</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="106"/>
        <source>Assembly workbench loaded</source>
        <translation>零件組合工作台已載入</translation>
    </message>
    <message>
        <location filename="../../../AssemblyImport.py" line="32"/>
        <source>Inserting file: </source>
        <translation>插入檔案: </translation>
    </message>
    <message>
        <location filename="../../../AssemblyImport.py" line="32"/>
        <source> into document: </source>
        <translation> 到文件: </translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="49"/>
        <source>&lt;p&gt;Insert a Link into the assembly. This will create dynamic links to parts/bodies/primitives/assemblies.To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;&lt;/p&gt;&lt;p&gt;Press shift to add several links while clicking on the view.</source>
        <translation>&lt;p&gt;將連結插入到程式集中. 這將建立到零件/實體/基元/零件組合件的動態連結. 若要插入外部物體，請確保檔案&lt;b&gt;在目前作業階段中開啟&lt;/b&gt;&lt;/p&gt;&lt;p&gt;按Shift 鍵新增多個連結點擊檢視內容.</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="51"/>
        <source>Insert Link</source>
        <translation>插入連結</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="162"/>
        <source>Select FreeCAD documents to import parts from</source>
        <translation>選擇要從中匯入零件的 FreeCAD 文件</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="164"/>
        <source>Supported Formats (*.FCStd *.fcstd);;All files (*)</source>
        <translation>支援的格式 (*.FCStd *.fcstd);;所有類型 (*)</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="191"/>
        <source>The current document must be saved before inserting an external part</source>
        <translation>插入外部零件之前必須儲存目前文件</translation>
    </message>
    <message>
        <location filename="../../../UtilsAssembly.py" line="32"/>
        <source>Assembly utilitary functions</source>
        <translation>零件組合實用函數</translation>
    </message>
    <message>
        <location filename="../../../UtilsAssembly.py" line="68"/>
        <source>getObject() in UtilsAssembly.py the object name is too short, at minimum it should be something like &apos;Assembly.Box.edge16&apos;. It shouldn&apos;t be shorter</source>
        <translation>UtilsAssembly.py 中的 getObject() 物件名稱太短，至少應該類似於&quot;Assembly.Box.edge16&quot;. 不應該更短</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="44"/>
        <source>Fixed</source>
        <translation>固定</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="45"/>
        <source>Revolute</source>
        <translation>旋轉</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="46"/>
        <source>Cylindrical</source>
        <translation>圓柱</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="47"/>
        <source>Slider</source>
        <translation>滑動</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="48"/>
        <source>Ball</source>
        <translation>球體</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="49"/>
        <source>Planar</source>
        <translation>平面</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="50"/>
        <source>Parallel</source>
        <translation>平行</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="51"/>
        <source>Tangent</source>
        <translation>切線</translation>
    </message>
</context>
<context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
        <location filename="../preferences/Assembly.ui" line="14"/>
        <source>General</source>
        <translation>一般</translation>
    </message>
</context>
<context>
    <name>AssemblyJoint</name>
    <message>
        <location filename="../../../JointObject.py" line="44"/>
        <source>Fixed</source>
        <translation>固定</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="45"/>
        <source>Revolute</source>
        <translation>旋轉</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="46"/>
        <source>Cylindrical</source>
        <translation>圓柱</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="47"/>
        <source>Slider</source>
        <translation>滑動</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="48"/>
        <source>Ball</source>
        <translation>球體</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="49"/>
        <source>Planar</source>
        <translation>平面</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="50"/>
        <source>Parallel</source>
        <translation>平行</translation>
    </message>
    <message>
        <location filename="../../../JointObject.py" line="51"/>
        <source>Tangent</source>
        <translation>切線</translation>
    </message>
</context>
<context>
    <name>Assembly_CommandCreateJoint</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="55"/>
        <source>Create Fixed Joint</source>
        <translation>建立固定接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="60"/>
        <source>&lt;p&gt;Create a Fixed Joint: Permanently locks two parts together, preventing any movement or rotation.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個固定接點: 將兩個零件永久鎖在一起，防止任何移動或旋轉.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="86"/>
        <source>Create Revolute Joint</source>
        <translation>建立旋轉接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="91"/>
        <source>&lt;p&gt;Create a Revolute Joint: Allows rotation around a single axis between selected parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個旋轉接點: 允許在選定零件之間繞單軸旋轉.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="118"/>
        <source>Create Cylindrical Joint</source>
        <translation>建立圓柱體接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="123"/>
        <source>&lt;p&gt;Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個圓柱體接點: 允許沿一個軸旋轉，同時允許在零件組合之間沿同一軸移動.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="149"/>
        <source>Create Slider Joint</source>
        <translation>建立滑動接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="154"/>
        <source>&lt;p&gt;Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個滑動接點: 允許沿單一軸線性移動，但限制選定零件之間的旋轉.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="180"/>
        <source>Create Ball Joint</source>
        <translation>建立球體接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="185"/>
        <source>&lt;p&gt;Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立球體接點: 在一個點連接零件，只要連接點保持接觸，就可以不受限制地移動.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="211"/>
        <source>Create Planar Joint</source>
        <translation>建立平面接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="216"/>
        <source>&lt;p&gt;Create a Planar Joint: Ensures two selected features are in the same plane, restricting movement to that plane.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個平面接點: 確保兩個選定的特徵位於同一平面，限制向該平面的移動.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="242"/>
        <source>Create Parallel Joint</source>
        <translation>建立平行接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="247"/>
        <source>&lt;p&gt;Create a Parallel Joint: Aligns two features to be parallel, constraining relative movement to parallel translations.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個平行接點: 將兩個特徵平行對齊，將相對運動限制為平行平移.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="273"/>
        <source>Create Tangent Joint</source>
        <translation>建立相切接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="278"/>
        <source>&lt;p&gt;Create a Tangent Joint: Forces two features to be tangent, restricting movement to smooth transitions along their contact surface.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立相切接點強制兩個特徵相切，限制移動以沿其接觸面平滑過渡.&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="360"/>
        <source>You need to select 2 elements from 2 separate parts.</source>
        <translation>您需要從 2 個單獨的零件中選擇 2 個元素.</translation>
    </message>
</context>
<context>
    <name>Assembly_CommandImport</name>
    <message>
        <location filename="../../../AssemblyImport.py" line="36"/>
        <source>Inserting file: </source>
        <translation>插入檔案: </translation>
    </message>
    <message>
        <location filename="../../../AssemblyImport.py" line="36"/>
        <source> into document: </source>
        <translation> 到文件: </translation>
    </message>
</context>
<context>
    <name>Assembly_CreateAssembly</name>
    <message>
        <location filename="../../../CommandCreateAssembly.py" line="45"/>
        <source>Create Assembly</source>
        <translation>建立零件組合</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateAssembly.py" line="47"/>
        <source>Create an assembly object in the current document.</source>
        <translation>在目前文件中建立零件組合物體.</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointBall</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="174"/>
        <source>Create Ball Joint</source>
        <translation>建立球體接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="176"/>
        <source>&lt;p&gt;Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立球體接點: 在一個點連接零件，只要連接點保持接觸，就可以不受限制地移動.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="112"/>
        <source>Create Cylindrical Joint</source>
        <translation>建立圓柱體接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="116"/>
        <source>&lt;p&gt;Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個圓柱體接點: 允許沿一個軸旋轉，同時允許在零件組合之間沿同一軸移動.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointFixed</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="52"/>
        <source>Create Fixed Joint</source>
        <translation>建立固定接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="54"/>
        <source>&lt;p&gt;Create a Fixed Joint: Permanently locks two parts together, preventing any movement or rotation.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個固定接點: 將兩個零件永久鎖在一起，防止任何移動或旋轉.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointParallel</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="234"/>
        <source>Create Parallel Joint</source>
        <translation>建立平行接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="236"/>
        <source>&lt;p&gt;Create a Parallel Joint: Aligns two features to be parallel, constraining relative movement to parallel translations.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個平行接點: 將兩個特徵平行對齊，將相對運動限制為平行平移.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointPlanar</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="204"/>
        <source>Create Planar Joint</source>
        <translation>建立平面接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="206"/>
        <source>&lt;p&gt;Create a Planar Joint: Ensures two selected features are in the same plane, restricting movement to that plane.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個平面接點: 確保兩個選定的特徵位於同一平面，限制向該平面的移動.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="82"/>
        <source>Create Revolute Joint</source>
        <translation>建立旋轉接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="84"/>
        <source>&lt;p&gt;Create a Revolute Joint: Allows rotation around a single axis between selected parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個旋轉接點: 允許在選定零件之間繞單軸旋轉.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointSlider</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="144"/>
        <source>Create Slider Joint</source>
        <translation>建立滑動接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="146"/>
        <source>&lt;p&gt;Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立一個滑動接點: 允許沿單一軸線性移動，但限制選定零件之間的旋轉.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_CreateJointTangent</name>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="264"/>
        <source>Create Tangent Joint</source>
        <translation>建立相切接點</translation>
    </message>
    <message>
        <location filename="../../../CommandCreateJoint.py" line="266"/>
        <source>&lt;p&gt;Create a Tangent Joint: Forces two features to be tangent, restricting movement to smooth transitions along their contact surface.&lt;/p&gt;</source>
        <translation>&lt;p&gt;建立相切接點強制兩個特徵相切，限制移動以沿其接觸面平滑過渡.&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>Assembly_InsertLink</name>
    <message>
        <location filename="../../../CommandInsertLink.py" line="55"/>
        <source>Insert Link</source>
        <translation>插入連結</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="57"/>
        <source>&lt;p&gt;Insert a Link into the assembly. This will create dynamic links to parts/bodies/primitives/assemblies.To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;&lt;/p&gt;&lt;p&gt;Press shift to add several links while clicking on the view.</source>
        <translation>&lt;p&gt;將連結插入到程式集中. 這將建立到零件/實體/基元/零件組合件的動態連結. 若要插入外部物體，請確保檔案&lt;b&gt;在目前作業階段中開啟&lt;/b&gt;&lt;/p&gt;&lt;p&gt;按Shift 鍵新增多個連結點擊檢視內容.</translation>
    </message>
</context>
<context>
    <name>CommandInsertLink</name>
    <message>
        <location filename="../../../CommandInsertLink.py" line="168"/>
        <source>Select FreeCAD documents to import parts from</source>
        <translation>選擇要從中匯入零件的 FreeCAD 文件</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="170"/>
        <source>Supported Formats (*.FCStd *.fcstd);;All files (*)</source>
        <translation>支援的格式 (*.FCStd *.fcstd);;All files (*)</translation>
    </message>
    <message>
        <location filename="../../../CommandInsertLink.py" line="197"/>
        <source>The current document must be saved before inserting an external part</source>
        <translation>插入外部零件之前必須儲存目前文件</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../../InitGui.py" line="76"/>
        <source>Assembly</source>
        <translation>零件組合</translation>
    </message>
</context>
<context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
        <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
        <source>Create Joint</source>
        <translation>建立接點</translation>
    </message>
</context>
<context>
    <name>TaskAssemblyInsertLink</name>
    <message>
        <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
        <source>Insert Link</source>
        <translation>插入連結</translation>
    </message>
    <message>
        <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
        <source>Search parts...</source>
        <translation>搜尋零件...</translation>
    </message>
    <message>
        <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
        <source>Don&apos;t find your part? </source>
        <translation>找不到您的零件? </translation>
    </message>
    <message>
        <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
        <source>Open file</source>
        <translation>開啟檔案</translation>
    </message>
</context>
<context>
    <name>UtilsAssembly</name>
    <message>
        <location filename="../../../UtilsAssembly.py" line="72"/>
        <source>getObject() in UtilsAssembly.py the object name is too short, at minimum it should be something like &apos;Assembly.Box.edge16&apos;. It shouldn&apos;t be shorter</source>
        <translation>UtilsAssembly.py 中的 getObject() 物件名稱太短，至少應該類似於&quot;Assembly.Box.edge16&quot;. 不應該更短</translation>
    </message>
</context>
<context>
    <name>Workbench</name>
    <message>
        <location filename="../../../InitGui.py" line="91"/>
        <source>Assembly</source>
        <translation>零件組合</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="70"/>
        <source>Assembly workbench</source>
        <translation>零件組合工作台</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="71"/>
        <source>Initializing Assembly workbench...</source>
        <translation>初始化零件組合工作台...</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="92"/>
        <source>Assembly Joints</source>
        <translation>零件組合接點</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="95"/>
        <source>&amp;Assembly</source>
        <translation>零件組合(&amp;A)</translation>
    </message>
    <message>
        <location filename="../../../InitGui.py" line="110"/>
        <source>Assembly workbench loaded</source>
        <translation>零件組合工作台已載入</translation>
    </message>
</context>
</TS>
