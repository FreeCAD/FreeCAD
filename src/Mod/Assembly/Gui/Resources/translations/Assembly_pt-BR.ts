<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="pt-BR" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Criar Montagem</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation type="unfinished">Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="79"/>
      <source>Create a Fixed Joint</source>
      <translation>Criar uma junta fixa</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="86"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation type="unfinished">1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="92"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation type="unfinished">2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="114"/>
      <source>Create Revolute Joint</source>
      <translation>Criar junta revolucionária</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="121"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Crie uma Junta revolucionária: Permite a rotação em torno de um único eixo entre as partes selecionadas.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="142"/>
      <source>Create Cylindrical Joint</source>
      <translation>Criar uma junta cilíndrica</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="149"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Crie uma Junta cilindrica: Permite a rotação ao longo de um eixo, permitindo o movimento ao longo do mesmo eixo entre as peças montadas.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="168"/>
      <source>Create Slider Joint</source>
      <translation>Criar junta de controle deslizante</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="175"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Crie junta de controle deslizante: Permite um movimento linear ao longo de um único eixo, mas restringe a rotação entre as peças selecionadas.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="194"/>
      <source>Create Ball Joint</source>
      <translation>Criar junta esférica</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="201"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Criar junta esférica: conecta peças em um ponto, permitindo movimento irrestrito enquanto os pontos de conexão permanecerem em contato.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="220"/>
      <source>Create Distance Joint</source>
      <translation>Criar junta de Distância</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="227"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Criar junta de Distância: Restrinja a distância entre os objetos selecionados.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="233"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Crie uma de várias juntas diferentes baseado na seleção. ou exemplo, uma distância de 0 entre um plano e um cilindro cria uma articulação tangente. Uma distância de 0 entre os planos os fará co-planar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="499"/>
      <source>Toggle grounded</source>
      <translation>Ativar/Desativar fixagem</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>A fixação de uma peça bloqueia permanentemente a sua posição, impedindo qualquer movimento ou rotação. Você precisa de pelo menos uma peça fixa antes de começar a montar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Exportar Arquivo ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Exportar a montagem ativa atualmente como um arquivo ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>Inserir componente</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Insira um componente na montagem ativa. Isso criará links dinâmicos para peças, corpos, primitivos e montagens. Para inserir componentes externos, certifique-se de que o arquivo está &lt;b&gt;aberto na sessão atual&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Insira clicando nos itens com o botão esquerdo.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Remova clicando nos itens com o botão direito.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Pressione shift para adicionar várias instâncias do componente enquanto clica na exibição.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Resolver Montagem</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Resolva a montagem ativa atualmente.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Assemblagem</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly</source>
      <translation>Assemblagem</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation type="unfinished">Assembly Joints</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Montagem</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="46"/>
      <source>Fixed</source>
      <translation>Afixado</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="47"/>
      <source>Revolute</source>
      <translation>Revolução</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Cylindrical</source>
      <translation>Cilíndrico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Slider</source>
      <translation>Deslizante</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Ball</source>
      <translation>Bola</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="1533"/>
      <source>Distance</source>
      <translation>Distância</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Parallel</source>
      <translation>Paralelo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Perpendicular</source>
      <translation>Perpendicular</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <location filename="../../../JointObject.py" line="1535"/>
      <source>Angle</source>
      <translation>Ângulo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>RackPinion</source>
      <translation type="unfinished">RackPinion</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Screw</source>
      <translation>Parafuso</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Gears</source>
      <translation>Engrenagens</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Belt</source>
      <translation>Correia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1365"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation type="unfinished">You need to select 2 elements from 2 separate parts.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1537"/>
      <source>Radius 1</source>
      <translation type="unfinished">Radius 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1539"/>
      <source>Pitch radius</source>
      <translation type="unfinished">Pitch radius</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>Perguntar</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>Sempre</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>Nunca</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation type="unfinished">Index (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation type="unfinished">Name (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Descrição</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation type="unfinished">File Name (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation type="unfinished">Quantity (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Padrão</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation type="unfinished">Duplicate Name</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation type="unfinished">This name is already used. Please choose a different name.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="367"/>
      <source>Options:</source>
      <translation>Opções:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="374"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation type="unfinished">Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation type="unfinished">Parts children : If checked, Parts children will be added to the bill of materials.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation type="unfinished">Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="388"/>
      <source>Columns:</source>
      <translation>Colunas:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="395"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation type="unfinished">Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation type="unfinished">Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation type="unfinished">Any column (custom or not) can be deleted by pressing Del.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="192"/>
      <source>The type of the joint</source>
      <translation>O tipo da junta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="213"/>
      <location filename="../../../JointObject.py" line="449"/>
      <source>The first object of the joint</source>
      <translation>O primeiro objeto da junta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="221"/>
      <source>The first part of the joint</source>
      <translation>A primeira peça da junta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="232"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Este é o sistema de coordenadas locais no object1 que será usado para a junta.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="243"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Isso impede que o Posicionamento 1 seja recomputado, permitindo o posicionamento personalizado.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="252"/>
      <location filename="../../../JointObject.py" line="468"/>
      <source>The second object of the joint</source>
      <translation>O segundo objeto da junta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="260"/>
      <source>The second part of the joint</source>
      <translation>A segunda peça da junta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="271"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Este é o sistema de coordenadas locais no object2 que será usado para a junta.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="282"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Isso impede que o Placement2 seja recomputado, permitindo o posicionamento personalizado.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="294"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation type="unfinished">This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="305"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation type="unfinished">This is the second distance of the joint. It is used only by the gear joint to store the second radius.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="316"/>
      <source>This is the rotation of the joint.</source>
      <translation>Esta é a rotação da junta.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="327"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Este é o vetor de deslocamento da junta.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="338"/>
      <source>This indicates if the joint is active.</source>
      <translation>Isto indica se a junta está activa.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="350"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation type="unfinished">Enable the minimum length limit of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="362"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation type="unfinished">Enable the maximum length limit of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="374"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation type="unfinished">Enable the minimum angle limit of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="386"/>
      <source>Enable the minimum length of the joint.</source>
      <translation type="unfinished">Enable the minimum length of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="398"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation type="unfinished">This is the minimum limit for the length between both coordinate systems (along their Z axis).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="409"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation type="unfinished">This is the maximum limit for the length between both coordinate systems (along their Z axis).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="420"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation type="unfinished">This is the minimum limit for the angle between both coordinate systems (between their X axis).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="431"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation type="unfinished">This is the maximum limit for the angle between both coordinate systems (between their X axis).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1021"/>
      <source>The object to ground</source>
      <translation>Fixar objeto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1033"/>
      <source>This is where the part is grounded.</source>
      <translation>É aqui que a peça está fixada.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="235"/>
      <source>The object moved by the move</source>
      <translation>O objeto movido pelo movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="242"/>
      <source>The containing parts of objects moved by the move</source>
      <translation type="unfinished">The containing parts of objects moved by the move</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="252"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation type="unfinished">This is the movement of the move. The end placement is the result of the start placement * this placement.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>The type of the move</source>
      <translation>O tipo de movimento</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Criar junta</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>Distância</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Raio 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>Deslocamento</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>Rotação</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="141"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Inverta a direção da junta.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>Reverse</source>
      <translation>Inverter</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="155"/>
      <source>Limits</source>
      <translation>Limites</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="161"/>
      <source>Min length</source>
      <translation>Comprimento mínimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="184"/>
      <source>Max length</source>
      <translation>Comprimento Máximo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Min angle</source>
      <translation>Angulo mínimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max angle</source>
      <translation>Angulo máximo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="268"/>
      <source>Reverse rotation</source>
      <translation>Inverter a rotação</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>Inserir componente</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Procurar peças...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>Não encontra sua peça? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>Abrir arquivo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Se marcado, a lista irá mostrar apenas peças.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>Mostrar apenas peças</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Geral</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Permitir sair do modo de edição ao pressionar a tecla Esc</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation type="unfinished">Esc leaves edit mode</translation>
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
      <translation type="unfinished">Log dragging steps</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>Fixar primeira peça:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Ao inserir a primeira peça na montagem, você pode escolher fixar a peça automaticamente.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Delete associated joints</source>
      <translation>Excluir juntas associadas</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="162"/>
      <source>The object is associated to one or more joints.</source>
      <translation>O objeto está associado a uma ou mais juntas.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="164"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Você deseja mover o objeto e excluir juntas associadas?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="780"/>
      <source>Move part</source>
      <translation>Mover peça</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="334"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Criar junta de coroa e pinhão</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="341"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Criar junta de coroa e pinhão: Conecta uma peça com uma junta deslizante com uma peça com uma junção de revolução.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="346"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Selecione os mesmos sistemas de coordenadas que as juntas giratórias e deslizantes. O raio de ajuste define a proporção de movimento entre a coroa e o pinhão.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="365"/>
      <source>Create Screw Joint</source>
      <translation>Criar junta de Parafuso</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="372"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Criar junta de Parafuso: junte uma peça com uma junção deslizante com uma peça com uma junta de revolução.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="377"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Selecione os mesmos sistemas de coordenadas que as juntas revoltas e deslizantes. O raio de ajuste define a taxa de movimento entre o parafuso rotativo e a parte deslizante.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="408"/>
      <location filename="../../../CommandCreateJoint.py" line="439"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Selecione os mesmos sistemas de coordenadas que as juntas giratórias.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="396"/>
      <source>Create Gears Joint</source>
      <translation>Criar Junta de Engrenagens</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="403"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Crie uma Junta de Engrenagens: interligar duas engrenagens giratórias. Eles terão uma direção de rotação inversa.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="427"/>
      <source>Create Belt Joint</source>
      <translation>Criar Junta de Correia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="434"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Criar Junta de Correia: Junta dois objetos giratórios. Eles terão a mesma direção de rotação.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="459"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Criar junta de Engrenagem/Correia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="465"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Criar junta de Engrenagem/Correia: Junte duas engrenagens giratórias.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="470"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Selecione os mesmos sistemas de coordenadas que as juntas giratórias.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>Criar vista explodida</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>Cria uma visão explodida da montagem atual.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>Criar vista explodida</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>Se marcado, as peças serão selecionadas como um único sólido.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Peças como único sólido</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation type="unfinished">Align dragger</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation type="unfinished">Aligning dragger:
Select a feature.
Press ESC to cancel.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>Explodir radialmente</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation type="unfinished">Create Bill Of Materials</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation type="unfinished">If checked, Sub assemblies children will be added to the bill of materials.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation type="unfinished">Sub-assemblies children</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation type="unfinished">If checked, Parts children will be added to the bill of materials.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Parte filhos</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation type="unfinished">If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation type="unfinished">Only parts</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>Colunas</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation type="unfinished">Add column</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>Exportação</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>Ajuda</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="252"/>
      <source>Create Parallel Joint</source>
      <translation type="unfinished">Create Parallel Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="259"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation type="unfinished">Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="280"/>
      <source>Create Perpendicular Joint</source>
      <translation type="unfinished">Create Perpendicular Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="287"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation type="unfinished">Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="306"/>
      <source>Create Angle Joint</source>
      <translation type="unfinished">Create Angle Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="313"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation type="unfinished">Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation type="unfinished">Create Bill of Materials</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation type="unfinished">Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the bom. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation type="unfinished">The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the bom. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation type="unfinished">The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</translation>
    </message>
  </context>
</TS>
