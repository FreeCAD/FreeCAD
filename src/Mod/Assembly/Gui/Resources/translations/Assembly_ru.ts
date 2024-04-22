<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Создание сборки</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Создайте объект сборки в текущем документе или в текущей активной сборке (если таковая имеется). Ограничение на одну корневую сборку на файл.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="68"/>
      <source>Create a Fixed Joint</source>
      <translation>Создайте фиксированное соединение</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="75"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1. Если сборка активна: создайте соединение, постоянно связывающее две детали вместе, предотвращающее любое движение или вращение.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2. Если деталь активна: расположите поддетали, сопоставив выбранные системы координат. Вторая выбранная часть будет двигаться.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="103"/>
      <source>Create Revolute Joint</source>
      <translation>Создать вращающееся соединение</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="110"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Создать вращающееся соединение: позволяет вращение вокруг одной оси между выбранными деталями.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="131"/>
      <source>Create Cylindrical Joint</source>
      <translation>Создать цилиндрическое соединение</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="138"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Создать цилиндрическое соединение. Обеспечивает вращение вдоль одной оси, одновременно допуская перемещение вдоль той же оси между собранными деталями.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="157"/>
      <source>Create Slider Joint</source>
      <translation>Создать скользящее соединение</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="164"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Создать скользящее соединение: позволяет линейное перемещение вдоль одной оси, но ограничивает вращение между выбранными частями.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="183"/>
      <source>Create Ball Joint</source>
      <translation>Создать шаровой шарнир</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="190"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Создать шаровой шарнир: соединяет детали в одной точке, обеспечивая неограниченное движение, пока точки соединения остаются в контакте.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="209"/>
      <source>Create Distance Joint</source>
      <translation>Создать дистанционный стык</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="216"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Создать дистанционное соединение: зафиксируйте расстояние между выбранными объектами.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Toggle grounded</source>
      <translation>Переключить заземление</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Заземление детали навсегда фиксирует ее положение в сборке, предотвращая любое движение или вращение. Прежде чем приступить к сборке, вам понадобится хотя бы одна заземленная часть.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Экспортировать файл ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Экспорт текущей активной сборки в файл ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="52"/>
      <source>Insert Link</source>
      <translation>Вставить ссылку</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="59"/>
      <source>Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Вставьте ссылку в текущую активную сборку. Это создаст динамические ссылки на детали/тела/примитивы/сборки. Чтобы вставить внешние объекты, убедитесь, что файл &lt;b&gt;открыт в текущем сеансе&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="61"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Вставьте, щелкнув левой кнопкой мыши элементы в списке.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="65"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Удалить, щелкнув правой кнопкой мыши элементы в списке.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="70"/>
      <source>Press shift to add several links while clicking on the view.</source>
      <translation>Нажмите Shift, чтобы добавить несколько ссылок, нажимая на представление.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Решить сборку</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Решите текущую активную сборку.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Сборка</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="98"/>
      <source>Assembly</source>
      <translation>Сборка</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="99"/>
      <source>Assembly Joints</source>
      <translation>Монтажные соединения</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="102"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Сборка</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Зафиксировано</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Вращение</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Цилиндрический</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Ползунок</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Шар</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Distance</source>
      <translation>Расстояние</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Спросить</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Всегда</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Никогда</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="116"/>
      <source>The type of the joint</source>
      <translation>Тип соединения</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="126"/>
      <source>The first object of the joint</source>
      <translation>Первый объект объединения</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="133"/>
      <source>The first part of the joint</source>
      <translation>Первая часть соединения</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="140"/>
      <source>The selected element of the first object</source>
      <translation>Выбранный элемент первого объекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="147"/>
      <source>The selected vertex of the first object</source>
      <translation>Выбранная вершина первого объекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="157"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Это локальная система координат внутри объекта 1, которая будет использоваться для соединения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="167"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Это предотвращает перерасчет Placement1, позволяя настраивать позиционирование места размещения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="175"/>
      <source>The second object of the joint</source>
      <translation>Второй объект соединения</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="182"/>
      <source>The second part of the joint</source>
      <translation>Вторая деталь соединения</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <source>The selected element of the second object</source>
      <translation>Выбранный элемент второго объекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="196"/>
      <source>The selected vertex of the second object</source>
      <translation>Выбранная вершина второго объекта</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Это локальная система координат внутри объекта 2, которая будет использоваться для соединения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Это предотвращает перерасчет Placement2, позволяя настраивать позиционирование места размещения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="226"/>
      <source>This is the distance of the joint. It is used only by the distance joint.</source>
      <translation>Это расстояние соединения. Используется только дистанционным соединением.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="236"/>
      <source>This is the rotation of the joint.</source>
      <translation>Это вращение соединения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="246"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Это вектор смещения соединения.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="256"/>
      <source>This indicates if the joint is active.</source>
      <translation>Это указывает на то, активно ли соединение.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="889"/>
      <source>The object to ground</source>
      <translation>Объект для закрепления</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="901"/>
      <source>This is where the part is grounded.</source>
      <translation>Это деталь для крепления.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Создать соединение</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Расстояние</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Offset</source>
      <translation>Смещение</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Rotation</source>
      <translation>Вращение</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="104"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Измените направление соединения.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="107"/>
      <source>Reverse</source>
      <translation>Развернуть</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Link</source>
      <translation>Вставить ссылку</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Поиск частей...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Не нашли свою деталь? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Открыть файл</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the selected object will be inserted inside a Part container, unless it is already a Part.</source>
      <translation>Если этот флажок установлен, выбранный объект будет вставлен в контейнер Детали, если он еще не является Деталью.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Insert as part</source>
      <translation>Вставить как деталь</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Основные</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Разрешить выход из режима редактирования при нажатии кнопки Esc</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leave edit mode</source>
      <translation>Esc выйти из режима редактирования</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Закрепить первую деталь:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>При вставке первой детали в сборку можно выбрать автоматическое заземление детали.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="136"/>
      <source>Delete associated joints</source>
      <translation>Удалить связанные соединения</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Объект связан с одним или несколькими соединениями.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Вы хотите переместить объект и удалить связанные соединения?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="651"/>
      <source>Move part</source>
      <translation>Переместить деталь</translation>
    </message>
  </context>
</TS>
