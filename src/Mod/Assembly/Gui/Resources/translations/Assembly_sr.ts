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
      <location filename="../../../CommandCreateJoint.py" line="76"/>
      <source>Create a Fixed Joint</source>
      <translation>Направи фиксни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="83"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ако је активан склоп: Направи спој који трајно закључа два дела заједно, спречавајуц́и било какво кретање или ротацију.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="89"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ако је активан део: Позиционира делове тако што ће направити подударним изабране координатне системе. Други изабрани део ће се померити.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="111"/>
      <source>Create Revolute Joint</source>
      <translation>Направи ротациони спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="118"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Направи ротациони спој (Кинематски пар V класе) између изабраних делова: Дозвољава ротацију око једне осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="139"/>
      <source>Create Cylindrical Joint</source>
      <translation>Направи цилиндрични спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="146"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Направи цилиндрични спој (Кинематски пар IV класе) измеђи изабраних делова: Дозвољава ротацију око једне осе и транслацију по истој оси.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="165"/>
      <source>Create Slider Joint</source>
      <translation>Направи транслаторни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="172"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Направи транслаторни спој (Кинематски пар V класе) између изабраних делова: Дозвољава једну транслацију.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="191"/>
      <source>Create Ball Joint</source>
      <translation>Направи кугласти спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="198"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Направи кугласти спој (Кинематски пар III класе) између изабраних делова: Дозвољава ротацију око све три осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="217"/>
      <source>Create Distance Joint</source>
      <translation>Направи равански спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="224"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Направи равански спој (кинематски пар III класе) између изабаних делова: Дозвољава ротацију око једне осе и транслацију по две осе.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="230"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>На основу избора направи један или неколико различитих спојева. На пример, растојање 0 између равни и цилиндра направиће их тангентним, растојање 0 између равни направиће их копланарним.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="416"/>
      <source>Toggle grounded</source>
      <translation>Учврсти</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="423"/>
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
      <location filename="../../../CommandExportASMT.py" line="52"/>
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
      <location filename="../../../InitGui.py" line="107"/>
      <source>Assembly</source>
      <translation>Скупштина</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly Joints</source>
      <translation>Спојеви у склопу</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="111"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Ассембли</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <location filename="../../../JointObject.py" line="61"/>
      <source>Fixed</source>
      <translation>Учвршћен</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <location filename="../../../JointObject.py" line="62"/>
      <source>Revolute</source>
      <translation>Ротациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <location filename="../../../JointObject.py" line="63"/>
      <source>Cylindrical</source>
      <translation>Цилиндрични</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="64"/>
      <source>Slider</source>
      <translation>Транслациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <location filename="../../../JointObject.py" line="65"/>
      <source>Ball</source>
      <translation>Кугласти</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <location filename="../../../JointObject.py" line="66"/>
      <source>Distance</source>
      <translation>Растојање</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>RackPinion</source>
      <translation>Зупчаста летва</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>Screw</source>
      <translation>Завртањ</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Gears</source>
      <translation>Зупчаник</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Belt</source>
      <translation>Ремен</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Питај</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Увек</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Никада</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="173"/>
      <source>The type of the joint</source>
      <translation>Врста споја</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="192"/>
      <source>The first object of the joint</source>
      <translation>Први објекат у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="200"/>
      <source>The first part of the joint</source>
      <translation>Први део у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="208"/>
      <source>The selected element of the first object</source>
      <translation>Изабрани елемент првог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>The selected vertex of the first object</source>
      <translation>Изабрано теме првог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="227"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар object1 који ће се користити у споју.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="238"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement1 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="247"/>
      <source>The second object of the joint</source>
      <translation>Други објекат у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="255"/>
      <source>The second part of the joint</source>
      <translation>Други део у споју</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="263"/>
      <source>The selected element of the second object</source>
      <translation>Изабрани елемент другог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="271"/>
      <source>The selected vertex of the second object</source>
      <translation>Изабрано теме другог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="282"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар object2 који ће се користити у споју.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="293"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement2 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="304"/>
      <source>This is the distance of the joint. It is used only by the distance joint and by RackPinion (pitch radius), Screw and Gears and Belt(radius1)</source>
      <translation type="unfinished">This is the distance of the joint. It is used only by the distance joint and by RackPinion (pitch radius), Screw and Gears and Belt(radius1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="315"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation type="unfinished">This is the second distance of the joint. It is used only by the gear joint to store the second radius.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="326"/>
      <source>This is the rotation of the joint.</source>
      <translation>Ротација споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="337"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Вектор одмака споја.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="348"/>
      <source>This indicates if the joint is active.</source>
      <translation>Ово показује да ли је спој активан.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="360"/>
      <source>Is this joint using limits.</source>
      <translation>Да ли овај спој користи ограничења.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="372"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Ово је минимално дужинско ограничење између координатних система (уздуж њихових З оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="383"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Ово је максимално дужинско ограничење између координатних система (уздуж њихових З оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="394"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Ово је минимално угаоно ограничење између координатних система (између њихових X оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="405"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Ово је максимално угаоно ограничење између координатних система (између њихових X оса).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="914"/>
      <source>The object to ground</source>
      <translation>Објекат који треба учврстити</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="926"/>
      <source>This is where the part is grounded.</source>
      <translation>Овде је место где је део учвршћен.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="230"/>
      <source>The object moved by the move</source>
      <translation type="unfinished">The object moved by the move</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="237"/>
      <source>The containing parts of objects moved by the move</source>
      <translation type="unfinished">The containing parts of objects moved by the move</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="247"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation type="unfinished">This is the movement of the move. The end placement is the result of the start placement * this placement.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Растојање</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Radius 2</source>
      <translation>Полупречник 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Offset</source>
      <translation>Одмак</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="100"/>
      <source>Rotation</source>
      <translation>Ротација</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="128"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Обрни смер спојаа.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="131"/>
      <source>Reverse</source>
      <translation>Обрнуто</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="142"/>
      <source>Limits</source>
      <translation>Ограничења</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="152"/>
      <source>Length min</source>
      <translation>Минимална дужина</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="172"/>
      <source>Length max</source>
      <translation>Максимална дужина</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="192"/>
      <source>Angle min</source>
      <translation>Минимални угао</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="212"/>
      <source>Angle max</source>
      <translation>Максимални угао</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="232"/>
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
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Не проналазиш део? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Отвори датотеку</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Ako je čekirano lista će pokazivati samo delove.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
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
      <source>Esc leave edit mode</source>
      <translation>Есц напусти режим уређивања</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Учврсти први део:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Да ли да први део који се умеће у склоп треба да буде учвршћен.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>Delete associated joints</source>
      <translation>Обриши придружене спојеве</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="160"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Објекту су придружени један или више спојева.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="162"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Да ли желиш померити објекат и обрисати придружене спојеве?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="762"/>
      <source>Move part</source>
      <translation>Помеи део</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="251"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Направи пренос са зупачастом летвом</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="258"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Направи пренос са зупачастом летвом: Комбинација транслаторног и ротационог споја.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="263"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation type="unfinished">Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Screw Joint</source>
      <translation>Направи навојни спој</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Направи навојни спој: Омогућава транслацију и ротацију једног дела унутар другог.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="294"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation type="unfinished">Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="325"/>
      <location filename="../../../CommandCreateJoint.py" line="356"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation type="unfinished">Select the same coordinate systems as the revolute joints.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="313"/>
      <source>Create Gears Joint</source>
      <translation>Направи зупчасти пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="320"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Направи зупчасти пренос: Спаја заједно два зупчаника који се ротирају. Зупчаници ће имати супротне смерове обртања.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="344"/>
      <source>Create Belt Joint</source>
      <translation>Направи ремени пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="351"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Направи ремени пренос: Спаја заједно две ременице које се ротирају. Ременице ће имати исте смерове обртања.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="376"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Направи зупчасти/ремени пренос</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="382"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation type="unfinished">Create a Gears/Belt Joint: Links two rotating gears together.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="387"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation type="unfinished">Select the same coordinate systems as the revolute joints.</translation>
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
  </context>
</TS>
