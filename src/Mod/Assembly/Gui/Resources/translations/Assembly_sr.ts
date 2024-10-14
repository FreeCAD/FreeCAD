<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="sr" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Направи склоп</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Направи главни склоп у корену стабла документа, или подсклоп у тренутно активном склопу. Може постојати само један главни склоп.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>Create a Fixed Joint</source>
      <translation>Направи фиксни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="88"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ако је активан склоп: Направи спој који трајно закључа два дела заједно, спречавајуц́и било какво кретање или ротацију.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="94"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ако је активан део: Позиционира делове тако што ће направити подударним изабране координатне системе. Други изабрани део ће се померити.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="116"/>
      <source>Create Revolute Joint</source>
      <translation>Направи ротациони спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="123"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Направи ротациони спој (Кинематски пар V класе) између изабраних делова: Дозвољава ротацију око једне осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="144"/>
      <source>Create Cylindrical Joint</source>
      <translation>Направи цилиндрични спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="151"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Направи цилиндрични спој (Кинематски пар IV класе) измеђи изабраних делова: Дозвољава ротацију око једне осе и транслацију по истој оси.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="170"/>
      <source>Create Slider Joint</source>
      <translation>Направи транслациони спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="177"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Направи транслациони спој (Кинематски пар V класе) између изабраних делова: Дозвољава једну транслацију.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="196"/>
      <source>Create Ball Joint</source>
      <translation>Направи кугласти спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="203"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Направи кугласти спој (Кинематски пар III класе) између изабраних делова: Дозвољава ротацију око све три осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="222"/>
      <source>Create Distance Joint</source>
      <translation>Направи равански спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="229"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Направи равански спој (кинематски пар III класе) између изабаних делова: Дозвољава ротацију око једне осе и транслацију по две осе.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="235"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>На основу избора направи један или неколико различитих спојева. На пример, растојање 0 између равни и цилиндра направиће их тангентним, растојање 0 између равни направиће их копланарним.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Toggle grounded</source>
      <translation>Учврсти</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="513"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Учвршћавање трајно фиксира позицију дела у склопу, спречавајући било какву транслацију или ротацију. Потребан је најмање један учвршћен део пре него што се почне формирати склоп.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Извези АСМТ датотеку</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="51"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Извези тренутно активан склоп као АСМТ датотеку.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>Уметни компоненту</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Уметни компоненту у активни склоп. Ово ц́е створити динамичке везе ка деловима, телима, примитивима и склоповима. Да би уметнуо спољне компоненте, прво се увери да је датотека &lt;b&gt;отворена у тренутној сесији&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Уметнути левим кликом на ставке у листи.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Уклонити десним кликом на ставке у листи.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Да би изабрао више компоненти одједанпут држи притиснуту типку shift.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Реши склоп</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Реши тренутно активни склоп.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Скупштина</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly</source>
      <translation>Склопови</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation>Спојеви у склопу</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Склопови</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="46"/>
      <source>Fixed</source>
      <translation>Фиксни</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="47"/>
      <source>Revolute</source>
      <translation>Ротациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Cylindrical</source>
      <translation>Цилиндрични</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Slider</source>
      <translation>Транслациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Ball</source>
      <translation>Кугласти</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="1582"/>
      <source>Distance</source>
      <translation>Растојање</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Parallel</source>
      <translation>Паралелни</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Perpendicular</source>
      <translation>Управни</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <location filename="../../../JointObject.py" line="1584"/>
      <source>Angle</source>
      <translation>Угаони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>RackPinion</source>
      <translation>Зупчаста летва</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Screw</source>
      <translation>Навојни</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Gears</source>
      <translation>Зупчасти</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Belt</source>
      <translation>Ремени</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1433"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>Потребно је изабрати 2 елемента са 2 различита дела.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1586"/>
      <source>Radius 1</source>
      <translation>Полупречник 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1588"/>
      <source>Pitch radius</source>
      <translation>Подеони полупречник</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>Питај</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>Увек</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>Никада</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>Индекс (аутоматски)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>Име (аутоматски)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Опис</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>Име Датотеке (аутоматски)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>Количина (аутоматски)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Подразумевано</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>Дуплирано име</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>Ово име је већ у употреби. Изабери друго име.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>Opcije:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Компоненте унутар подсклопова: Ако је означено, у саставници ће се појавити и компоненте подсклопова.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>Компоненте унутар контејнера Део: Ако је означено, у саставници ће се појавити контејнери Део и компоненте унутар њих.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Само контејнери Део: Ако је означено, у саставници ће се појавити само контејнери Део и подсклопови. Компоненте као што су контејнери Тело, компоненте из гвожђаре или објекти из окружења Делови биће игнорисани.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>Колоне:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>Аутоматске колоне: (Индекс, Количина, Назив...) се попуњавају аутоматски. Свака промена коју направиш биће поништена. Ове колоне се не могу преименовати.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>Сопствене колоне: 'Опис' и остале колоне које су додате помоћу опције 'Додај колону' неће бити преписане. Ове колоне се могу преименовати двоструким кликом или притиском на типку Ф2 (преименовање колоне ће обрисати њихове податке).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Било која колона може бити обрисана притиском на типку Дел.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="188"/>
      <source>The type of the joint</source>
      <translation>Врста споја</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="212"/>
      <location filename="../../../JointObject.py" line="462"/>
      <source>The first reference of the joint</source>
      <translation>Прва референца у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="223"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар референтног објекта 1 који ће се користити за спој.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="245"/>
      <location filename="../../../JointObject.py" line="521"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation type="unfinished">This is the attachment offset of the first connector of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="254"/>
      <location filename="../../../JointObject.py" line="487"/>
      <source>The second reference of the joint</source>
      <translation>Друга референца у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="265"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар референтног објекта 2 који ће се користити за спој.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="287"/>
      <location filename="../../../JointObject.py" line="531"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation type="unfinished">This is the attachment offset of the second connector of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="432"/>
      <source>The first object of the joint</source>
      <translation>Први објекат у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="234"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement1 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="451"/>
      <source>The second object of the joint</source>
      <translation>Други објекат у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement2 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="299"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>Растојање споја. Користи се код раванског и навојног споја, а такође и код зупчастог, ременог (полупречник) и преносног споја са зупчастом летвом (полупречник корака)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="310"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Ово је друго растојање споја. Користи се само код зупчастог споја да одреди други полупречник.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="321"/>
      <source>This indicates if the joint is active.</source>
      <translation>Ово показује да ли је спој активан.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="333"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Омогући минимално дужинско ограничење споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="345"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Омогући максимално дужинско ограничење споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="357"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Омогући минимално угаоно ограничење споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="369"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Омогући минимално растојање споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="381"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Ово је минимално дужинско ограничење између координатних система (уздуж њихових З оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="392"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Ово је максимално дужинско ограничење између координатних система (уздуж њихових З оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="403"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Ово је минимално угаоно ограничење између координатних система (између њихових X оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Ово је максимално угаоно ограничење између координатних система (између њихових X оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1088"/>
      <source>The object to ground</source>
      <translation>Објекат који треба учврстити</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1100"/>
      <source>This is where the part is grounded.</source>
      <translation>Овде је место где је део учвршћен.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="248"/>
      <location filename="../../../CommandCreateView.py" line="282"/>
      <source>The objects moved by the move</source>
      <translation>Објекти који се померају приликом померања</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Померање током померања. Крајњи положај је резултат почетног положаја * овог положаја.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="267"/>
      <source>The type of the move</source>
      <translation>Врста померања</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Направи спој</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>Равански</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Полупречник 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>Одмак</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>Окретање</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="141"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Обрни смер спојаа.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>Reverse</source>
      <translation>Обрнуто</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="155"/>
      <source>Limits</source>
      <translation>Ограничења</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="161"/>
      <source>Min length</source>
      <translation>Минимална дужина</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="184"/>
      <source>Max length</source>
      <translation>Максимална дужина</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Min angle</source>
      <translation>Минимални угао</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max angle</source>
      <translation>Минимални угао</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="268"/>
      <source>Reverse rotation</source>
      <translation>Обрни ротацију</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>Уметни компоненту</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Претрага делова...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>Не проналазиш део? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>Отвори датотеку</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Ako je čekirano lista će pokazivati samo delove.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>Прикажи само делове</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Опште</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Напушти режим уређивања притиском на типку Есц</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Esc да напустиш режим уређивања</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>Забележи кораке солвера. Ово је корисно ако желиш да пријавиш грешку.
Датотеке се зову „runPreDrag.asmt“ и „dragging.log“ и налазе се у подразумеваној фасцикли std::ofstream (у Windows-у је то радна површина)</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>Забележи кораке</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>Учврсти први део:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Да ли да први део који се умеће у склоп треба да буде учвршћен.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="180"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Објекту су придружени један или више спојева.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="182"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Да ли желиш померити објекат и обрисати придружене спојеве?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="871"/>
      <source>Move part</source>
      <translation>Помеи део</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="336"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Направи пренос са зупачастом летвом</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Направи пренос са зупачастом летвом: Комбинација транслаторног и ротационог споја.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="348"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Изабери исте координатне системе као код ротационих или транслаторних спојева. Полупречник корака одређује однос кретања између зупчаника и летве.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="367"/>
      <source>Create Screw Joint</source>
      <translation>Направи навојни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Направи навојни спој: Омогућава транслацију и ротацију једног дела унутар другог.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="379"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Изабери исте координатне системе као код ротационих или транслаторних спојева. Полупречник корака одређује однос кретања између завртња који се ротира и дела који клизи.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="410"/>
      <location filename="../../../CommandCreateJoint.py" line="441"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Изабери исте координатне системе као код ротационих спојева.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="398"/>
      <source>Create Gears Joint</source>
      <translation>Направи зупчасти пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Направи зупчасти пренос: Спаја заједно два зупчаника који се ротирају. Зупчаници ће имати супротне смерове обртања.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="429"/>
      <source>Create Belt Joint</source>
      <translation>Направи ремени пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Направи ремени пренос: Спаја заједно две ременице које се ротирају. Ременице ће имати исте смерове обртања.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="461"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Направи зупчасти/ремени пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Направи зупачасти/ремени пренос: Споји заједно два зупчаника/ременице.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="472"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Изабери исте координатне системе као код ротационих спојева.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>Направи растављени поглед</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>Направи растављени поглед склопа.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>Направи растављени поглед</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>Ако је чекирано, делови ће бити изабрани као једно тело.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Део као пуно тело</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>Поравнај вучни триедар</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>Вучни триедар за поравнавање:
Изабери.
Притисни ESC да откажеш.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>Растави радијално</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>Направи саставницу</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Ако је означено, у саставници ће се појавити и компоненте подсклопова.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>Компоненте унутар подсклопова</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>Ако је означено, у саставници ће се појавити контејнери Део и компоненте унутар њих.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Компоненте унутар контејнера Део</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Ако је означено, у саставници ће се појавити само контејнери Део и подсклопови. Компоненте као што су контејнери Тело, компоненте из гвожђаре или објекти из окружења Делови биће игнорисани.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>Само контејнери Део</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>Колоне</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>Додај колону</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>Извези</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>Помоћ</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="254"/>
      <source>Create Parallel Joint</source>
      <translation>Направи паралелни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="261"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Направи паралелни спој: Направи З осе координатних система изабраних делова паралелним.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Perpendicular Joint</source>
      <translation>Направи управни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Направи управни спој: Направи З осе координатних система изабраних делова управним.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="308"/>
      <source>Create Angle Joint</source>
      <translation>Направи угаони спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="315"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>Направи угаони спој: Фиксира задани угао између З оса координатних система изабраних делова.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>Направи саставницу</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>Направи саставницу склопа. Ако је неки склоп активан направиће се његова саставница. У супротном, направиће се саставница целог документа.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>Објекат Саставница је документ који чува подешавања саставнице. То је објекат окружења Табеле и може се прегледати. Ако није потребно да објекат Саставница буде сачуван као документ, једноставно извези и откажи задатак.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>Колоне 'Индекс', 'Име', 'Име датотеке' и 'Количина' се аутоматски генеришу при поновном израчунавању. Колоне „Опис“ и сопствене колоне се не преписују.</translation>
    </message>
  </context>
</TS>
