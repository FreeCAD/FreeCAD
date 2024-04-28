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
      <location filename="../../../CommandCreateJoint.py" line="68"/>
      <source>Create a Fixed Joint</source>
      <translation>Направи фиксни зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="75"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ако је активан склоп: Направи зглоб који заједно спаја два дела, спречавајуцћи било какво кретање или ротацију.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ако је активан део: Позиционира делове тако што ће направити подударним изабране координатне системе. Други изабрани део ће се померити.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="103"/>
      <source>Create Revolute Joint</source>
      <translation>Направи ротациони зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="110"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Направи ротациони зглоб (Кинематски пар V класе) између изабраних делова: Дозвољава ротацију око једне осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="131"/>
      <source>Create Cylindrical Joint</source>
      <translation>Направи цилиндрични зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="138"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Направи цилиндрични зглоб (Кинематски пар IV класе) измеђи изабраних делова: Дозвољава ротацију око једне осе и транслацију по истој оси.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="157"/>
      <source>Create Slider Joint</source>
      <translation>Направи транслаторни зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="164"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Направи транслаторни зглоб (Кинематски пар V класе) између изабраних делова: Дозвољава једну транслацију.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="183"/>
      <source>Create Ball Joint</source>
      <translation>Направи кугласти зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="190"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Направи кугласти зглоб (Кинематски пар III класе) између изабраних делова: Дозвољава ротацију око све три осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="209"/>
      <source>Create Distance Joint</source>
      <translation>Направи равански зглоб</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="216"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Направи равански зглоб (кинематски пар III класе) између изабаних делова: Дозвољава ротацију око једне осе и транслацију по две осе.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Toggle grounded</source>
      <translation>Укљ/Искљ учвршћеност</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
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
      <location filename="../../../CommandInsertLink.py" line="52"/>
      <source>Insert Link</source>
      <translation>Уметни везу</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="59"/>
      <source>Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Уметните везу у тренутно активни склоп. Ово ће створити динамичке везе ка деловима/телима/примитивима/склоповима. Да бисте уметнули спољне објекте, уверите се да је датотека &lt;b&gt;тренутно отворена&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="61"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Уметнути левим кликом на ставке у листи.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="65"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Уклонити десним кликом на ставке у листи.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="70"/>
      <source>Press shift to add several links while clicking on the view.</source>
      <translation type="unfinished">Press shift to add several links while clicking on the view.</translation>
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
      <location filename="../../../InitGui.py" line="98"/>
      <source>Assembly</source>
      <translation>Скупштина</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="99"/>
      <source>Assembly Joints</source>
      <translation>Зглобови склопа</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="102"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Ассембли</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Учвршћен</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Ротациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Цилиндрични</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Транслациони</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Кугласти</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Distance</source>
      <translation>Растојање</translation>
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
      <location filename="../../../JointObject.py" line="116"/>
      <source>The type of the joint</source>
      <translation>Врста зглоба</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="126"/>
      <source>The first object of the joint</source>
      <translation>Први објекат зглоба</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="133"/>
      <source>The first part of the joint</source>
      <translation>Први део зглоба</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="140"/>
      <source>The selected element of the first object</source>
      <translation>Изабрани елемент првог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="147"/>
      <source>The selected vertex of the first object</source>
      <translation>Изабрано теме првог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="157"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар object1 који ће се користити у зглобу.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="167"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement1 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="175"/>
      <source>The second object of the joint</source>
      <translation>Други објекат зглоба</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="182"/>
      <source>The second part of the joint</source>
      <translation>Други део зглоба</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <source>The selected element of the second object</source>
      <translation>Изабрани елемент другог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="196"/>
      <source>The selected vertex of the second object</source>
      <translation>Изабрано теме другог објекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Ово је локални координатни систем унутар object2 који ће се користити у зглобу.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ово спречава да се поново израчунава Placement2 омогућавајући сопствено позиционирање.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="226"/>
      <source>This is the distance of the joint. It is used only by the distance joint.</source>
      <translation>Растојање између изабраних равни делова који чине кинематски пар.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="236"/>
      <source>This is the rotation of the joint.</source>
      <translation>Ротација зглоба.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="246"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Вектор одмака зглоба.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="256"/>
      <source>This indicates if the joint is active.</source>
      <translation>Ово показује да ли је зглоб активан.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="889"/>
      <source>The object to ground</source>
      <translation>Објекат који треба учврстити</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="901"/>
      <source>This is where the part is grounded.</source>
      <translation>Овде је место где је део учвршћен.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Направи зглоб</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Растојање</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Offset</source>
      <translation>Одмак</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Rotation</source>
      <translation>Ротација</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="104"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Обрни смер злоба.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="107"/>
      <source>Reverse</source>
      <translation>Обрнуто</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Link</source>
      <translation>Уметни везу</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Претрага делова...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation type="unfinished">Don't find your part? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Отвори датотеку</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the selected object will be inserted inside a Part container, unless it is already a Part.</source>
      <translation>Ако је означено, изабрани објекат ће бити уметнут унутар контејнера Парт, осим ако већ није Парт.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Insert as part</source>
      <translation>Уметни као део</translation>
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
      <location filename="../../ViewProviderAssembly.cpp" line="136"/>
      <source>Delete associated joints</source>
      <translation>Обриши придружене зглобове</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Објекту су придружени један или више зглобова.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Да ли желиш померити објекат и обрисати придружене зглобове?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="651"/>
      <source>Move part</source>
      <translation>Помеи део</translation>
    </message>
  </context>
</TS>
