<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="82"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Центральна точка початку спіралі; береться із опорної осі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="92"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Напрямок спіралі; обраховується від опорної осі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="99"/>
      <source>The reference axis of the helix.</source>
      <translation>Опорна вісь спіралі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="106"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Режим введення спіралі визначає, які властивості задаються користувачем.
Потім розраховуються залежні властивості.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="118"/>
      <source>The axial distance between two turns.</source>
      <translation>Осьова відстань між двома витками.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="125"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Висота траєкторії спіралі, що не враховує довжину профілю.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="135"/>
      <source>The number of turns in the helix.</source>
      <translation>Кількість витків у спіралі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="143"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>Кут конуса, що утворює обводи довкола спіралі.
Ненульові значення перетворюють спіраль на конічну спіраль.
При позитивних значеннях радіус збільшується, при негативних зменшується.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="156"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Приріст радіусу спіралі за один виток.
Ненульові значення перетворюють спіраль на конічну спіраль.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="167"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Встановлює напрямок обертання на лівостороннє,
тобто. проти годинникової стрілки при переміщенні вздовж осі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="178"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Визначає, чи спрямована спіраль у протилежний бік від напрямку осі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="188"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Якщо встановлено, то результатом буде перетин профілю та наявного тіла.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="198"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Якщо не встановлено, інструмент запропонує початкове значення кроку на основі 
обмежувальної границі профілю, щоб уникнути самоперетину.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="210"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Терпимість злиття для гвинтової лінії, збільште, якщо спіральна форма не зливається коректно з частиною.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>Number of gear teeth</source>
      <translation>Кількість зубів шестерні</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="120"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Кут тиску зубів передачі</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="114"/>
      <source>Module of the gear</source>
      <translation>Модуль шестерні</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="129"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True = 2 криві з кожними 3 контрольними точками, False = 1 крива з 4 контрольними точками.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="137"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=зовнішня шестерня, False=внутрішня шестерня</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="146"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>Висота зуба від ділильного кола до його вершини, нормована за модулем.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="155"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>Висота зуба від ділильного кола до його кореня, нормована за модулем.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="164"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Радіус галтелі біля кореня зуба, нормований за модулем.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="173"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>Відстань, на яку зміщується еталонний профіль назовні, нормована за модулем.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1674"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1675"/>
      <source>Additive Helix</source>
      <translation>Адитивна спіраль</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1676"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Проводить вибраний ескіз або профіль вздовж спіралі та додає його до тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1575"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1576"/>
      <source>Additive Loft</source>
      <translation>Адитивний лофт</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1577"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>Проводить лофт вибраного ескізу вздовж шляху та додає до тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1475"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1476"/>
      <source>Additive Pipe</source>
      <translation>Адитивна труба</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1477"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Проводить вибраний ескіз або профіль вздовж шляху та додає до тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="93"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="94"/>
      <source>New Body</source>
      <translation>Нове тіло</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="95"/>
      <source>Creates a new body and activates it</source>
      <translation>Створює нове тіло та активує його</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2590"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2591"/>
      <source>Boolean Operation</source>
      <translation>Булева операція</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2592"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>Застосовує булеві операції між вибраними об'єктами та активним тілом</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="283"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="284"/>
      <source>Local Coordinate System</source>
      <translation>Локальна система координат</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="285"/>
      <source>Creates a new local coordinate system</source>
      <translation>Створює нову локальну систему координат</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="2001"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2002"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2003"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>Застосовує фаску до вибраних ребер або граней</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="493"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="494"/>
      <source>Clone</source>
      <translation>Клонувати</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="495"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>Параметрично копіює суцільний об'єкт як базовий елемент нового тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2030"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2031"/>
      <source>Draft</source>
      <translation>Креслення</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2032"/>
      <source>Applies a draft to the selected faces</source>
      <translation>Застосовує ухил до вибраних граней</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="754"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>Duplicate &amp;Object</source>
      <translation>Дублювати &amp;об'єкт</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Дублює виділений обʼєкт і додає його до активного тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1973"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1974"/>
      <source>Fillet</source>
      <translation>Заокруглення</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1975"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>Застосовує заокруглення до вибраних ребер або граней</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1405"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1406"/>
      <source>Groove</source>
      <translation>Проточка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1407"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>Обертає ескіз або профіль навколо лінії/осі та видаляє з тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1298"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1299"/>
      <source>Hole</source>
      <translation>Отвір</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1301"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>Створює отвори в активному тілі в центрах кіл або дуг вибраного ескізу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="223"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="224"/>
      <source>Datum Line</source>
      <translation>Опорна лінія</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="225"/>
      <source>Creates a new datum line</source>
      <translation>Створює нову опорну лінію</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2285"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2286"/>
      <source>Linear Pattern</source>
      <translation>Лінійний масив</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2287"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>Дублює вибрані елементи або активне тіло в лінійному масиві</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="389"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="390"/>
      <source>Migrate</source>
      <translation>Мігрувати</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="391"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>Мігрує документ до сучасного робочого процесу Part Design</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2228"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2229"/>
      <source>Mirror</source>
      <translation>Дзеркало</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2230"/>
      <source>Mirrors the selected features or active body</source>
      <translation>Дзеркально відображає вибрані елементи або активне тіло</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="822"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="823"/>
      <source>Move Object To…</source>
      <translation>Перемістити об'єкт до…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="824"/>
      <source>Moves the selected object to another body</source>
      <translation>Переміщує виділений обʼєкт до іншого тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1019"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1020"/>
      <source>Move Feature After…</source>
      <translation>Перемістити елемент після…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1021"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>Переміщує вибраний елемент після іншого елемента в тому самому тілі</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="655"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="656"/>
      <source>Set Tip</source>
      <translation>Встановити вершину</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="657"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>Переміщує вершину тіла до вибраного елемента</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2459"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2460"/>
      <source>Multi-Transform</source>
      <translation>Множинна трансформація</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2461"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>Застосовує кілька трансформацій до вибраних елементів або активного тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="573"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="574"/>
      <source>New Sketch</source>
      <translation>Новий ескіз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="575"/>
      <source>Creates a new sketch</source>
      <translation>Створює новий ескіз</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1240"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1241"/>
      <source>Pad</source>
      <translation>Видавлювання</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1242"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>Видавлює вибраний ескіз або профіль та додає до тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="194"/>
      <source>Datum Plane</source>
      <translation>Опорна площина</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="195"/>
      <source>Creates a new datum plane</source>
      <translation>Створює нову опорну площину</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1269"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1270"/>
      <source>Pocket</source>
      <translation>Виріз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1271"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>Видавлює вибраний ескіз або профіль та видаляє його з тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="253"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="254"/>
      <source>Datum Point</source>
      <translation>Опорна точка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="255"/>
      <source>Creates a new datum point</source>
      <translation>Створює нову опорну точку</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2354"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2355"/>
      <source>Polar Pattern</source>
      <translation>Круговий масив</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2356"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>Дублює вибрані елементи або активне тіло у круговому масиві</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1343"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1344"/>
      <source>Revolve</source>
      <translation>Обертатися</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1345"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>Обертає вибраний ескіз або профіль навколо лінії/осі та додає до тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2416"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2417"/>
      <source>Scale</source>
      <translation>Масштабування</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2418"/>
      <source>Scales the selected features or the active body</source>
      <translation>Масштабує вибрані елементи або активне тіло</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="317"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="318"/>
      <source>Shape Binder</source>
      <translation>Прив'язка форми</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="319"/>
      <source>Creates a new shape binder</source>
      <translation>Створює нову прив'язку форми</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="387"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="388"/>
      <source>Sub-Shape Binder</source>
      <translation>Сполучна Форма для Підобʼєкта</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="389"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>Створює посилання на геометрію з одного або кількох об’єктів, що дозволяє йому використовуватися всередині або поза тілом. Він відслідковує відносне місцезнаходження, підтримує декілька типів геометрії (тіла, грані, ребра, вершини) і може працювати з об'єктами в тому ж самому або зовнішньому документах.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1758"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1759"/>
      <source>Subtractive Helix</source>
      <translation>Субтрактивна спіраль</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1760"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>Проводить вибраний ескіз або профіль вздовж спіралі та видаляє з тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1625"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1626"/>
      <source>Subtractive Loft</source>
      <translation>Субтрактивний лофт</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1627"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>Проводить лофт вибраного ескізу вздовж шляху та видаляє з тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1525"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1526"/>
      <source>Subtractive Pipe</source>
      <translation>Субтрактивна труба</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1527"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>Проводить вибраний ескіз або профіль вздовж шляху та видаляє з тіла</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2100"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2101"/>
      <source>Thickness</source>
      <translation>Товщина</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2102"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>Застосовує товщину та видаляє вибрані грані</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="77"/>
      <source>Additive Primitive</source>
      <translation>Адитивний примітив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="78"/>
      <source>Creates an additive primitive</source>
      <translation>Створює адитивний примітив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Box</source>
      <translation>Аддитивний Паралелепіпед</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Additive Cylinder</source>
      <translation>Аддитивний Циліндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Additive Sphere</source>
      <translation>Аддитивна Сфера</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Additive Cone</source>
      <translation>Аддитивний Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Additive Ellipsoid</source>
      <translation>Аддитивний Еліпсоїд</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Additive Torus</source>
      <translation>Аддитивний Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Additive Prism</source>
      <translation>Аддитивна Призма</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Additive Wedge</source>
      <translation>Аддитивний Клин</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>PartDesign</source>
      <translation>Проєктування деталі</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="285"/>
      <source>Subtractive Primitive</source>
      <translation>Субтрактивний примітив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="286"/>
      <source>Creates a subtractive primitive</source>
      <translation>Створює субтрактивний примітив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Subtractive Box</source>
      <translation>Субтрактивний Паралелепіпед</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Subtractive Cylinder</source>
      <translation>Субтрактивний Циліндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Subtractive Sphere</source>
      <translation>Субтрактивна Сфера</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Subtractive Cone</source>
      <translation>Субтрактивний Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Субтрактивний Еліпсоїд</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Subtractive Torus</source>
      <translation>Субтрактивний Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Subtractive Prism</source>
      <translation>Субтрактивна Призма</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Subtractive Wedge</source>
      <translation>Субтрактивний Клин</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="339"/>
      <source>Edit Shape Binder</source>
      <translation>Редагувати прив'язку форми</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="350"/>
      <source>Create Shape Binder</source>
      <translation>Створити прив'язку форми</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="443"/>
      <source>Create Sub-Shape Binder</source>
      <translation>Створити прив'язку підформи</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="513"/>
      <source>Create Clone</source>
      <translation>Створити Клон</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1124"/>
      <source>Make Copy</source>
      <translation>Зробити копію</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2514"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>Перетворити на елемент множинної трансформації</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="266"/>
      <source>Sketch on Face</source>
      <translation>Ескіз на грані</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="327"/>
      <source>Make copy</source>
      <translation>Зробити копію</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="529"/>
      <location filename="../../SketchWorkflow.cpp" line="805"/>
      <source>New Sketch</source>
      <translation>Новий ескіз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2611"/>
      <source>Create Boolean</source>
      <translation>Створити Булеву операцію</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <location filename="../../CommandBody.cpp" line="221"/>
      <source>Add a Body</source>
      <translation>Додати Тіло</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="526"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>Мігрувати застарілі елементи Part Design до тіл</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="768"/>
      <source>Duplicate a Part Design object</source>
      <translation>Дублювати об'єкт Part Design</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1113"/>
      <source>Move a feature inside body</source>
      <translation>Перемістити елемент всередині тіла</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="722"/>
      <source>Move tip to selected feature</source>
      <translation>Перемістити активний елемент до вибраного елемента</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="927"/>
      <source>Move an object</source>
      <translation>Перемістити об’єкт</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="261"/>
      <source>Mirror</source>
      <translation>Дзеркало</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="301"/>
      <source>Linear Pattern</source>
      <translation>Лінійний масив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="350"/>
      <source>Polar Pattern</source>
      <translation>Круговий масив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="389"/>
      <source>Scale</source>
      <translation>Масштабування</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation>Інструменти граней</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation>Інструменти ребер</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation>Булеві інструменти</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation>Допоміжні інструменти</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation>Інструменти моделювання</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>Створити геометрію</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>Параметр евольвенти</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>Кількість зубів</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>Модуль</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>Кут тиску</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>Висока точність</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Так</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Ні</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>Зовнішнє зубчасте колесо</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>Коефіцієнт голівки зуба</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>Коефіцієнт ніжки зуба</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>Коефіцієнт заокруглення ніжки</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>Коефіцієнт зміщення профілю</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Необхідне Активне Тіло</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>Щоб створити новий Pard Design об’єкт, у документі має бути активне Тіло.
Виберіть тіло нижче або створіть нове тіло.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>Створити нове тіло</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="53"/>
      <source>Select an active body</source>
      <translation>Вибрати активне тіло</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Геометричні примітиви</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Кут у першому напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Кут у другому напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>Довжина</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>Ширина</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>Висота</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>Радіус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>Кут обертання</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>Радіус 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>Радіус 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>Кут</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>Параметр U</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>Параметри V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Радіус у напрямку локальної осі Z</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>Радіус у локальному напрямку X</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>Радіус 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>Радіус у локальному напрямку Y
Якщо нуль, рівний Радіусу2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>Параметр V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>Радіус у локальній площині XY</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>Радіус у локальній площині XZ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Багатокутник</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>Описаний радіус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X мін/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y мін/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z мін/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 мін/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 мін/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Крок</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>Система координат</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Приріст</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>Кількість обертів</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>Кут 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>Кут 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>Через 3 точки</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>Великий радіус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>Малий радіус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>Обертання праворуч</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Обертання ліворуч</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Початкова точка</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Кінцева точка</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Посилання</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Ви вибрали геометрії, які не є частиною активного тіла. Будь ласка, визначте, як обробляти цей вибір. Якщо вам не потрібні ці посилання, скасуйте команду.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Створити незалежну копію (рекомендовано)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Створити повʼязану копію</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Створити перехресне посилання</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="287"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Вибір цього призведе до циклічної залежності.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>Додати тіло</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>Видалити тіло</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Обʼєднання</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Вирізати</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Перетин</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="53"/>
      <source>Boolean Parameters</source>
      <translation>Булеві параметри</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="84"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="53"/>
      <source>Primitive Parameters</source>
      <translation>Параметри примітива</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="947"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="955"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="963"/>
      <source>Invalid wedge parameters</source>
      <translation>Невірні параметри клина</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="948"/>
      <source>X min must not be equal to X max!</source>
      <translation>X min не повинен дорівнювати Х max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="956"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y min не повинен дорівнювати Y max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="964"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z min не повинен дорівнювати Z max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1006"/>
      <source>Create primitive</source>
      <translation>Створити примітив</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1051"/>
      <source>%1 fine dragging</source>
      <translation>%1 точне перетягування</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1054"/>
      <source>%1 coarse dragging</source>
      <translation>%1 грубе перетягування</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Перемикає між режимом вибору та попереднього перегляду</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- виділіть елемент, щоб виділити його
- двічі клацніть на елемент, щоб побачити його фаски</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>Тип</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>Рівна відстань</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>Дві відстані</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>Відстань і кут</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>Інвертує напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>Використовувати всі ребра</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>Розмір</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>Розмір 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>Кут</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="346"/>
      <source>Empty chamfer created!
</source>
      <translation>Створено порожню фаску!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="386"/>
      <source>The body list cannot be empty</source>
      <translation>Список тіл не може бути пустим</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="386"/>
      <source>Empty Body List</source>
      <translation>Очистити Список Тіл</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="407"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Логічне значення: Прийняти: Помилка введення</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>Incompatible Reference Set</source>
      <translation>Несумісний набір посилань</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="109"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Відсутній режим приєднання, який би відповідав поточному набору орієнтирів. Якщо ви вирішите продовжити, елемент залишиться на поточному місці та не буде переміщений при зміні орієнтиру. Продовжити?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Перемикає між режимом вибору та попереднього перегляду</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- виберіть елемент, щоб виділити його
- двічі клацніть на елемент, щоб побачити його ухили</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Кут нахилу</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>Нейтральна площина</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>Напрямок витягування</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Зворотний напрямок витягування</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="304"/>
      <source>Empty draft created!
</source>
      <translation>Створено порожній ухил!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="302"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="307"/>
      <source>Confirm Selection</source>
      <translation>Підтвердити вибір</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="320"/>
      <source>Add All Edges</source>
      <translation>Додати всі ребра</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="326"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>Додає всі ребра до списку (лише в режимі додавання)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="335"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1374"/>
      <source>No face selected</source>
      <translation>Грань не виділена</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="173"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1143"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="77"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="354"/>
      <source>Preview</source>
      <translation>Попередній перегляд</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="358"/>
      <source>Select Faces</source>
      <translation>Вибрати грані</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="694"/>
      <source>Select reference…</source>
      <translation>Вибрати посилання…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="604"/>
      <source>No shape selected</source>
      <translation>Немає обраної форми</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="687"/>
      <source>Sketch normal</source>
      <translation>Нормаль Ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="690"/>
      <source>Face normal</source>
      <translation>Нормаль Грані</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="698"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="701"/>
      <source>Custom direction</source>
      <translation>Довільний напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1090"/>
      <source>Click on a shape in the model</source>
      <translation>Натисніть на фігуру в моделі</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>One sided</source>
      <translation>Однобічний</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1362"/>
      <source>Two sided</source>
      <translation>Двобічний</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1363"/>
      <source>Symmetric</source>
      <translation>Симетрично</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1369"/>
      <source>Click on a face in the model</source>
      <translation>Натисніть на грань в моделі</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Дозволити використані елементи</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>Дозволити зовнішні елементи</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>Із інших тіл цієї ж деталі</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>З різних деталей або вільних елементів</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Створити незалежну копію (рекомендовано)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Створити повʼязану копію</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Створити перехресне посилання</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Valid</source>
      <translation>Вірна</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Invalid shape</source>
      <translation>Неприпустима фігура</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>No wire in sketch</source>
      <translation>Немає ламаної лінії в ескізі</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Sketch already used by other feature</source>
      <translation>Ескіз вже використовується іншим елементом</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another body</source>
      <translation>Належить до іншого тіла</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Belongs to another part</source>
      <translation>Належить до іншої деталі</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Doesn't belong to any body</source>
      <translation>Не належить жодному тілу</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Base plane</source>
      <translation>Базова поверхня</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="85"/>
      <source>Feature is located after the tip of the body</source>
      <translation>Елемент розміщений після вершини тіла</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="97"/>
      <source>Select Attachment</source>
      <translation>Вибрати прикріплення</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Перемикає між режимом вибору та попереднього перегляду</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- виберіть елемент, щоб виділити його
- двічі клацніть на елемент, щоб побачити його заокруглення</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>Радіус</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>Використовувати всі ребра</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="205"/>
      <source>Empty fillet created!</source>
      <translation>Створено порожнє скруглення!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Вірна</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base X-axis</source>
      <translation>Базова вісь X</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="242"/>
      <source>Base Y-axis</source>
      <translation>Базова вісь Y</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Base Z-axis</source>
      <translation>Базова вісь Z</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="224"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Normal sketch axis</source>
      <translation>Нормаль до осі ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>Статус</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>Вісь</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="208"/>
      <source>Select reference…</source>
      <translation>Вибрати посилання…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Нахил-Висота-Кут</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Крок-Поворот-Кут</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Висота-Поворот-Кут</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Висота-Повороти-Зростання</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>Тангаж</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>Висота</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>Витки</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>Кут конуса</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>Радіальний приріст</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>Перерахунок при зміні</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Лівостороння спіраль</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Зворотній</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>Видалити зовнішній профіль</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="57"/>
      <source>Helix Parameters</source>
      <translation>Параметри спіралі</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="227"/>
      <source>Construction line %1</source>
      <translation>Допоміжна лінія %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="295"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Попередження: спіраль може самоперетинатися</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="300"/>
      <source>Error: helix touches itself</source>
      <translation>Помилка: спіраль торкається самої себе</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="352"/>
      <source>Error: unsupported mode</source>
      <translation>Помилка: не підтримуваний режим</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterbore</source>
      <translation>Цековка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="58"/>
      <source>Countersink</source>
      <translation>Конічна зенківка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Counterdrill</source>
      <translation>Торцева та конічна зенківка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="63"/>
      <source>Hole Parameters</source>
      <translation>Параметри отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>None</source>
      <translation>Нічого</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>ISO metric regular</source>
      <translation>ISO метрична стандартна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>ISO metric fine</source>
      <translation>ISO метрична дрібна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS coarse</source>
      <translation>UTS груба</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>UTS fine</source>
      <translation>UTS дрібна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>UTS extra fine</source>
      <translation>UTS особливо дрібна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>ANSI pipes</source>
      <translation>Трубна ANSI</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>ISO/BSP pipes</source>
      <translation>Труби ISO/BSP</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>BSW whitworth</source>
      <translation>BSW Вітворта</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="81"/>
      <source>BSF whitworth fine</source>
      <translation>BSF Вітворта дрібна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="82"/>
      <source>ISO tyre valves</source>
      <translation>Клапани шин ISO</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="712"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Середній</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="716"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Добре</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="720"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Шорсткий</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="726"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Звичайна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="730"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Закрити</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="734"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Loose (макс. допуск)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="738"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Звичайна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="739"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Закрити</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="740"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Wide (макс. відхилення)</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Лінійчата поверхня</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Закрито</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Профіль</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Обʼєкт</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Додати переріз</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Видалити переріз</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>Список можна змінити шляхом перетягування</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation>Перерахунок при зміні</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft Parameters</source>
      <translation>Параметри лофту</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="74"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>Площини</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="186"/>
      <source>Error</source>
      <translation>Помилка</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>Перетворення</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>Гаразд</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="71"/>
      <source>Edit</source>
      <translation>Редагувати</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="74"/>
      <source>Delete</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Add Mirror Transformation</source>
      <translation>Додати дзеркальну трансформацію</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="85"/>
      <source>Add Linear Pattern</source>
      <translation>Додати лінійний масив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="93"/>
      <source>Add Polar Pattern</source>
      <translation>Додати круговий масив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="101"/>
      <source>Add Scale Transformation</source>
      <translation>Додати трансформацію масштабування </translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="104"/>
      <source>Move Up</source>
      <translation>Перемістити вгору</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="107"/>
      <source>Move Down</source>
      <translation>Перемістити вниз</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="139"/>
      <source>Right-click to add a transformation</source>
      <translation>Клацніть правою кнопкою для додавання трансформації</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation>Параметри видавлювання</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>Зміщення видавлювання від грані, на якій воно закінчиться з боку 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>Зміщення видавлювання від грані, на якій воно закінчиться з боку 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>Змінити напрямок видавлювання</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Розмірність</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>До останнього</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>До першої</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>До лиця</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>До форми</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>Тип</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>Розмірність</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>Довжина</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>Зсув до грані</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>Обрати всі поверхні</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation>Вибрати грань</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>Сторона 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>Напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="541"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Встановіть напрямок або виберіть ребро з моделі як орієнтир</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>Нормаль Ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>Довільний напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Використовуйте власний вектор напрямку видавлювання,
інакше буде використаний вектор нормалі площини ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Якщо прапорець не встановлений, довжина 
вимірюватиметься вздовж зазначеного напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>Довжина вздовж нормалі ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Перемикає між режимом вибору та попереднього перегляду</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>Зворотній</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation>Напрямок/ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation>Вибрати посилання…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation>Компонент X вектора напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation>Компонент Y вектора напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation>Компонент Z вектора напрямку</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>Кут конусності екструзії</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation>Сторона 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>Кут конусності</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>Вибрати форму</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>Вибирає всі грані форми</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation>Перерахунок при зміні</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Режим орієнтації</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Стандартний</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Зафіксовано</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Френе</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Допоміжний</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Бінормальний</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Криволінійна еквівалентність</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Профіль</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Обʼєкт</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Додати ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Видалити ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Встановіть постійний бінарний вектор для визначення орієнтації профілів</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="190"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="197"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="204"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="598"/>
      <source>Section Orientation</source>
      <translation>Орієнтація перерізу</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="626"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Профіль</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Обʼєкт</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>Перехід в кутах</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>Прямий кут</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>Заокруглений кут</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>Шлях для розгортки</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Додати ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>Видалити ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Перетворений</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe Parameters</source>
      <translation>Параметри труби</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="80"/>
      <source>Select All</source>
      <translation>Вибрати все</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="98"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="463"/>
      <location filename="../../TaskPipeParameters.cpp" line="584"/>
      <source>Input Error</source>
      <translation>Помилка введення</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="463"/>
      <source>No active body</source>
      <translation>Немає активного тіла</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Режим перетворення</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Постійний</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Багатоперерізний</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Додати переріз</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Видалити переріз</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>Список можна змінити шляхом перетягування</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="897"/>
      <source>Section Transformation</source>
      <translation>Трансформація перерізу</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="916"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation>Параметри вирізу</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>Зміщення від вибраної грані, на якій виріз закінчиться з боку 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>Зміщення від вибраної грані, на якій виріз закінчиться з боку 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="48"/>
      <source>Reverses pocket direction</source>
      <translation>Змінює напрямок вирізу</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Dimension</source>
      <translation>Розмірність</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Through all</source>
      <translation>Наскрізь</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>To first</source>
      <translation>До першої</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="80"/>
      <source>Up to face</source>
      <translation>До лиця</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="81"/>
      <source>Up to shape</source>
      <translation>До форми</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>Тип</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="222"/>
      <source>Base X-axis</source>
      <translation>Базова вісь X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="223"/>
      <source>Base Y-axis</source>
      <translation>Базова вісь Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="224"/>
      <source>Base Z-axis</source>
      <translation>Базова вісь Z</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>Симетрично до площини</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>Зворотній</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>Другий кут</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>Вісь</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="232"/>
      <source>Select reference…</source>
      <translation>Вибрати посилання…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="175"/>
      <source>Angle</source>
      <translation>Кут</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="149"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="459"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation>Перерахунок при зміні</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="177"/>
      <source>To last</source>
      <translation>До останнього</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="180"/>
      <source>Through all</source>
      <translation>Наскрізь</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <source>To first</source>
      <translation>До першої</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="183"/>
      <source>Up to face</source>
      <translation>До лиця</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="184"/>
      <source>Two angles</source>
      <translation>Два кути</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="447"/>
      <source>No face selected</source>
      <translation>Грань не виділена</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>Коефіцієнт</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>Кількість Входження</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Обʼєкт</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Додати геометрію</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Видалити геометрію</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation>Параметри прив'язки форми</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="139"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="210"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Перемикає між режимом вибору та попереднього перегляду</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Вибрати</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- виберіть елемент, щоб виділити його
- двічі клацніть на елемент, щоб побачити його властивості</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>Товщина</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>Тема оформлення</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Труба</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>Двосторонній</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>Тип зʼєднання</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>Дуга</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>Перетин</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>Створити товщину всередину</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="269"/>
      <source>Empty thickness created!
</source>
      <translation>Створено порожню товщину!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="398"/>
      <source>Normal sketch axis</source>
      <translation>Нормаль до осі ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="397"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="396"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="400"/>
      <location filename="../../TaskTransformedParameters.cpp" line="436"/>
      <source>Construction line %1</source>
      <translation>Допоміжна лінія %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="414"/>
      <source>Base X-axis</source>
      <translation>Базова вісь X</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="415"/>
      <source>Base Y-axis</source>
      <translation>Базова вісь Y</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="416"/>
      <source>Base Z-axis</source>
      <translation>Базова вісь Z</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="450"/>
      <source>Base XY-plane</source>
      <translation>Базова площина XY</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="451"/>
      <source>Base YZ-plane</source>
      <translation>Базова площина YZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="452"/>
      <source>Base XZ-plane</source>
      <translation>Базова площина XZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="424"/>
      <location filename="../../TaskTransformedParameters.cpp" line="460"/>
      <source>Select reference…</source>
      <translation>Вибрати посилання…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>Перетворення тіла</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>Інструмент перетворення форми</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>Додати елемент</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>Видалити елемент</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="116"/>
      <source>Recompute on change</source>
      <translation>Перерахунок при зміні</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="909"/>
      <source>Select Body</source>
      <translation>Вибрати тіло</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="910"/>
      <source>Select a body from the list</source>
      <translation>Виділіть тіло зі списку</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1098"/>
      <source>Move Feature After…</source>
      <translation>Перемістити елемент після…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1099"/>
      <source>Select a feature from the list</source>
      <translation>Оберіть елемент зі списку</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1186"/>
      <source>Move Tip</source>
      <translation>Перемістити вершину</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1192"/>
      <source>Set tip to last feature?</source>
      <translation>Встановити вершину на останній елемент?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1187"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>Переміщений елемент з'являється після поточного активного елемента.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="151"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Немає режимів приєднання, які відповідають вибраним обʼєктам. Оберіть щось інше.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <location filename="../../Command.cpp" line="172"/>
      <location filename="../../Command.cpp" line="179"/>
      <source>Error</source>
      <translation>Помилка</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="842"/>
      <source>Several sub-elements selected</source>
      <translation>Обрано декілька під-елементів</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="843"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>Виберіть одну грань як підкладку для ескізу!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="850"/>
      <source>Select a face as support for a sketch!</source>
      <translation>Виберіть грань як підкладку для ескізу!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="857"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>Потрібна плоска грань як підкладка для ескізу!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="864"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>Спочатку створіть площину або виберіть грань для ескізу</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="849"/>
      <source>No support face selected</source>
      <translation>Не обрано базової грані</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="856"/>
      <source>No planar support</source>
      <translation>Площина підтримки відсутня</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="863"/>
      <source>No valid planes in this document</source>
      <translation>В цьому документі відсутні коректні площини</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="761"/>
      <location filename="../../Command.cpp" line="1152"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="97"/>
      <location filename="../../ViewProviderDatum.cpp" line="259"/>
      <location filename="../../ViewProvider.cpp" line="137"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Діалогове вікно вже відкрито в панелі задач</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1006"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Неможливо використати команду, тому що відсутнє суцільне тіло для віднімання.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1009"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Перш ніж виконувати команду віднімання, переконайтеся, що тіло має таку можливість.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1033"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Неможливо використати виділений об’єкт. Виділений обʼєкт повинен належати до активного тіла</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="165"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>Немає активного тіла. Активуйте тіло перед вставкою опорного об'єкта.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="150"/>
      <source>Invalid Selection</source>
      <translation>Некоректний вибір</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="471"/>
      <source>Sub-shape binder</source>
      <translation>Прив'язка підформи</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1065"/>
      <source>No sketch to work on</source>
      <translation>Немає ескізу для роботи</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1066"/>
      <source>No sketch is available in the document</source>
      <translation>В документі відсутній ескіз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2206"/>
      <source>Select only one feature in an active body.</source>
      <translation>Виберіть лише одну операцію в активному тілі.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="762"/>
      <location filename="../../Command.cpp" line="1153"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="98"/>
      <location filename="../../ViewProviderDatum.cpp" line="260"/>
      <location filename="../../ViewProvider.cpp" line="138"/>
      <source>Close this dialog?</source>
      <translation>Закрити цей діалог?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1835"/>
      <location filename="../../Command.cpp" line="1870"/>
      <source>Wrong selection</source>
      <translation>Невірний вибір</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1836"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Оберіть ребро, грань або тіло з одного тіла.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1843"/>
      <location filename="../../Command.cpp" line="2205"/>
      <source>Selection is not in the active body</source>
      <translation>Вибране не належить активному тілу</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1871"/>
      <source>Shape of the selected part is empty</source>
      <translation>Форма вибраної деталі порожня</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1844"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Оберіть ребро, грань або тіло в активному тілі.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1036"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>Розгляньте використання прив'язки форми або базового елемента для посилання на зовнішню геометрію в тілі</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1857"/>
      <source>Wrong object type</source>
      <translation>Невірний тип обʼєкта</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1858"/>
      <source>%1 works only on parts.</source>
      <translation>%1 працює тільки з деталями.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="74"/>
      <source>Part creation failed</source>
      <translation>Не вдалося створити деталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="75"/>
      <source>Failed to create a part object.</source>
      <translation>Не вдалося створити обʼєкт Деталь.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="124"/>
      <location filename="../../CommandBody.cpp" line="132"/>
      <location filename="../../CommandBody.cpp" line="148"/>
      <location filename="../../CommandBody.cpp" line="214"/>
      <source>Bad base feature</source>
      <translation>Зіпсований базовий елемент</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>Тіло не може базуватися на елементі Part Design.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="133"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 вже належить до тіла і не може використовуватися як базовий елемент іншого тіла.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="149"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>Базова функція (%1) належить до іншої деталі.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="176"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>Вибрана форма складається з кількох суцільних тіл.
Це може призвести до несподіваних результатів.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="182"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>Вибрана форма складається з кількох оболонок.
Це може призвести до несподіваних результатів.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="188"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>Вибрана фігура складається лише з оболонки.
Це може призвести до несподіваних результатів.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="194"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>Вибрана форма складається з кількох суцільних тіл або оболонок.
Це може призвести до несподіваних результатів.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="203"/>
      <source>Base feature</source>
      <translation>Базовий елемент</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="215"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Тіло може бути засноване не більше ніж на одній функції.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="230"/>
      <source>Body</source>
      <translation>Тіло</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="425"/>
      <source>Nothing to migrate</source>
      <translation>Немає що мігрувати</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="614"/>
      <source>Edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Будь ласка, відредагуйте і перевизначте '%1' аби використати Базову або Опорну площину як площину ескізу.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="689"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>Виберіть рівно один елемент Part Design або тіло.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="697"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>Не вдалося визначити тіло для вибраного елемента '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="878"/>
      <source>Only features of a single source body can be moved</source>
      <translation>Можна переміщати лише елементи одного вихідного тіла</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="613"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Площину ескізу не можна мігрувати</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="426"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>Елементів Part Design без тіла не знайдено. Нічого мігрувати.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="688"/>
      <location filename="../../CommandBody.cpp" line="696"/>
      <location filename="../../CommandBody.cpp" line="710"/>
      <location filename="../../CommandBody.cpp" line="1064"/>
      <location filename="../../CommandBody.cpp" line="1074"/>
      <source>Selection error</source>
      <translation>Помилка виділення</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="711"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Лише елемент суцільного тіла може бути активним елементом тіла.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <location filename="../../CommandBody.cpp" line="877"/>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>Features cannot be moved</source>
      <translation>Функції не можуть бути переміщені</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="848"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Деякі з виділених елементів мають залежності в початковому тілі</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="896"/>
      <source>There are no other bodies to move to</source>
      <translation>Немає інших тіл для переміщення до</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1065"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Неможливо перемістити базові функції тіла.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1075"/>
      <source>Select one or more features from the same body.</source>
      <translation>Виділіть один чи декілька елементів одного тіла.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1090"/>
      <source>Beginning of the body</source>
      <translation>Початок тіла</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1171"/>
      <source>Dependency violation</source>
      <translation>Порушення залежностей</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1172"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Рання функція не має залежати від пізнішої функції.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="309"/>
      <source>No previous feature found</source>
      <translation>Не знайдено попереднього елементу</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="310"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Неможливо створити субтрактивний елемент без базового елементу</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="208"/>
      <location filename="../../TaskTransformedParameters.cpp" line="433"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="209"/>
      <location filename="../../TaskTransformedParameters.cpp" line="434"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальна вісь ескізу</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="211"/>
      <source>Construction line %1</source>
      <translation>Допоміжна лінія %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="96"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="209"/>
      <source>Active Body Required</source>
      <translation>Необхідне Активне Тіло</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="151"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>Для використання Part Design необхідно активне тіло документа. Активуйте тіло (подвійний натискання) або створіть новий.

Для застарілих документів з об'єктами Part Design, які не мають тіла, використайте функцію міграції в Part Design, щоб помістити їх у тіло.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="210"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>Для створення нового об'єкта Part Design потрібне активне тіло. Активуйте наявне (подвійне клацання) або створіть нове.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="272"/>
      <source>Feature is not in a body</source>
      <translation>Елемент за межами тіла</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="273"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Щоб використовувати цей елемент, він повинен належати обʼєкту Тіло в документі.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="319"/>
      <source>Feature is not in a part</source>
      <translation>Елемент за межами деталі</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="320"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Щоб використовувати цю функцію, вона повина належати обʼєкту Деталь в документі.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="227"/>
      <location filename="../../ViewProvider.cpp" line="94"/>
      <location filename="../../ViewProviderTransformed.cpp" line="67"/>
      <location filename="../../ViewProviderDressUp.cpp" line="64"/>
      <source>Edit %1</source>
      <translation>Редагувати %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="107"/>
      <source>Set Face Colors</source>
      <translation>Встановити кольори граней</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="214"/>
      <source>Plane</source>
      <translation>Площини</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="209"/>
      <source>Line</source>
      <translation>Лінія</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="219"/>
      <source>Point</source>
      <translation>Точка</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>Система координат</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="236"/>
      <source>Edit Datum</source>
      <translation>Редагувати опорний об'єкт</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="93"/>
      <source>Feature error</source>
      <translation>Помилка елементу</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="94"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1 не має базового елемента.
Цей елемент пошкоджений і не може бути відредагований.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="222"/>
      <source>Edit Shape Binder</source>
      <translation>Редагувати прив'язку форми</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Synchronize</source>
      <translation>Синхронізувати</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="354"/>
      <source>Select Bound Object</source>
      <translation>Вибрати прив'язаний об'єкт</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="156"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>Документ "%1", який ви редагуєте, створено зі старою версією верстату Part Design.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="163"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>Мігрувати для використання сучасних функцій Part Design?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="168"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>Схоже, що документ "%1" або знаходиться в середині процесу міграції зі застарілого в PartDesign, або має порушену структуру.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="175"/>
      <source>Make the migration automatically?</source>
      <translation>Виконати міграцію автоматично?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="178"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Нотатка: Якщо ви оберете міграцію, ви не зможете редагувати файл у старій версії FreeCAD.
Якщо ви відмовитеся від міграції, ви не зможете використовувати нові функції середовища "Проєктування деталі", такі як Тіла та Деталі. В результаті ви також не зможете використовувати свої деталі у складальному робочому середовищі.
Хоча ви зможете мігрувати будь-якої миті пізніше за допомогою 'Проєктування деталі -&gt; Мігрувати'.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="191"/>
      <source>Migrate Manually</source>
      <translation>Мігрувати вручну</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="70"/>
      <source>Edit Boolean</source>
      <translation>Редагувати булеву операцію</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="42"/>
      <source>Edit Chamfer</source>
      <translation>Редагувати фаску</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="43"/>
      <source>Edit Draft</source>
      <translation>Редагувати ухил</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="42"/>
      <source>Edit Fillet</source>
      <translation>Редагувати заокруглення</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="45"/>
      <source>Edit Groove</source>
      <translation>Редагувати проточку</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="50"/>
      <source>Edit Helix</source>
      <translation>Редагувати спіраль</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="130"/>
      <source>Edit Hole</source>
      <translation>Редагувати отвір</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="40"/>
      <source>Edit Linear Pattern</source>
      <translation>Редагувати лінійний масив</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="67"/>
      <source>Edit Loft</source>
      <translation>Редагувати лофт</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="40"/>
      <source>Edit Mirror</source>
      <translation>Редагувати дзеркальне зображення</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="49"/>
      <source>Edit Multi-Transform</source>
      <translation>Редагувати множинну трансформацію</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="45"/>
      <source>Edit Pad</source>
      <translation>Редагувати видавлювання</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="77"/>
      <source>Edit Pipe</source>
      <translation>Редагувати трубу</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="47"/>
      <source>Edit Pocket</source>
      <translation>Редагувати виріз</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="40"/>
      <source>Edit Polar Pattern</source>
      <translation>Редагувати круговий масив</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="52"/>
      <source>Edit Primitive</source>
      <translation>Редагувати примітив</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="45"/>
      <source>Edit Revolution</source>
      <translation>Редагувати обертання</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="40"/>
      <source>Edit Scale</source>
      <translation>Редагувати масштаб</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="42"/>
      <source>Edit Thickness</source>
      <translation>Редагувати товщину</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>Параметри зірочки</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>Кількість зубів</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>Еталон зірочки</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="66"/>
      <source>ANSI 25</source>
      <translation>ANSI 25</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="71"/>
      <source>ANSI 35</source>
      <translation>ANSI 35</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="76"/>
      <source>ANSI 41</source>
      <translation>ANSI 41</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="81"/>
      <source>ANSI 40</source>
      <translation>ANSI 40</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="86"/>
      <source>ANSI 50</source>
      <translation>ANSI 50</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="91"/>
      <source>ANSI 60</source>
      <translation>ANSI 60</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="96"/>
      <source>ANSI 80</source>
      <translation>ANSI 80</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="101"/>
      <source>ANSI 100</source>
      <translation>ANSI 100</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="106"/>
      <source>ANSI 120</source>
      <translation>ANSI 120</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="111"/>
      <source>ANSI 140</source>
      <translation>ANSI 140</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="116"/>
      <source>ANSI 160</source>
      <translation>ANSI 160</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="121"/>
      <source>ANSI 180</source>
      <translation>ANSI 180</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="126"/>
      <source>ANSI 200</source>
      <translation>ANSI 200</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="131"/>
      <source>ANSI 240</source>
      <translation>ANSI 240</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="136"/>
      <source>Bicycle with derailleur</source>
      <translation>Велосипед з перемикачем</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>Велосипед без перемикача</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>Крок ланцюга</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>Діаметр ролика ланцюга</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>Ширина зуба</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="146"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="151"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 08B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="156"/>
      <source>ISO 606 10B</source>
      <translation>ISO 606 10B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="161"/>
      <source>ISO 606 12B</source>
      <translation>ISO 606 12B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="166"/>
      <source>ISO 606 16B</source>
      <translation>ISO 606 16B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="171"/>
      <source>ISO 606 20B</source>
      <translation>ISO 606 20B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="176"/>
      <source>ISO 606 24B</source>
      <translation>ISO 606 24B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="181"/>
      <source>Motorcycle 420</source>
      <translation>Для мотоцикла 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>Для мотоцикла 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>Для мотоцикла 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>Для мотоцикла 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>Для мотоцикла 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>Для мотоцикла 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>Для мотоцикла 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 в</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="824"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Оновлення змін в потоці
Зауважте, що обчислення може зайняти деякий час</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1019"/>
      <source>Thread Depth</source>
      <translation>Глибина різьби</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1072"/>
      <source>Customize thread clearance</source>
      <translation>Вказати внутрішнє відхилення діаметра різьблення</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="699"/>
      <source>Clearance</source>
      <translation>Відхилення</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>Тип голівки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>Тип глибини</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>Діаметр голівки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>Глибина голівки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="668"/>
      <source>Clearance / Passthrough</source>
      <translation>Зазор / Наскрізний</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="686"/>
      <source>Hole type</source>
      <translation>Тип отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="828"/>
      <source>Update thread view</source>
      <translation>Оновити загальний вигляд</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Clearance</source>
      <translation>Користувацький допуск</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1091"/>
      <source>Custom Thread clearance value</source>
      <translation>Значення внутрішнього відхилення діаметра різьби в мм</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="881"/>
      <source>Direction</source>
      <translation>Напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>Розмір</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="712"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Допуск отвору
Доступно лише для отворів без різьби</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="717"/>
      <source>Standard</source>
      <translation>Стандартний</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="673"/>
      <source>Tap drill</source>
      <translation>Свердло під різьбу</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Threaded</source>
      <translation>З різьбою</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Close</source>
      <translation>Закрити</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Wide</source>
      <translation>Wide (макс. відхилення)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="805"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>Чи має отвір змодельовану різьбу</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="808"/>
      <source>Model Thread</source>
      <translation>Модель різьби</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="861"/>
      <source>Class</source>
      <translation>Клас</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Клас допуску для різьбових отворів відповідно до профілю отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>Діаметр</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>Діаметр отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>На глибину</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>Параметри отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>Типи базового профілю</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>Кола та дуги</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>Точки, кола та дуги</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>Точки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="989"/>
      <source>Dimension</source>
      <translation>Розмірність</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>Наскрізь</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>Користувацькі значення голівки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Кут свердла</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Включити в глибину</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>Змінити напрямок</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="773"/>
      <source>Thread</source>
      <translation>Різьба</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="905"/>
      <source>&amp;Right hand</source>
      <translation>&amp;Права різьба</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="921"/>
      <source>&amp;Left hand</source>
      <translation>&amp;Ліва різьба</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="970"/>
      <source>Thread Depth Type</source>
      <translation>Тип глибини різьби</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="984"/>
      <source>Hole depth</source>
      <translation>Глибина отвору</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="994"/>
      <source>Tapped (DIN76)</source>
      <translation>Різьбові (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>Тип вирізу для головки гвинта</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Встановіть прапорець, щоб скасувати значення, визначені 'Тип'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>Для зенкерів це глибина
верхня частина гвинта нижче поверхні</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>Кут зенківки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>Врахувує розмір точки свердління
для глибини глухих отворів</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>Звуження</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Кут конусності для отвору
90 градусів: прямий отвір
менше ніж 90: менший радіус отвору внизу
понад 90: більший радіус отвору внизу</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>Змінює напрямок отвору</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Повідомлень немає</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Ескіз</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;ДизайнДеталі</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>Величини</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation>Адитивні елементи</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation>Субтрактивні елементи</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation>Оздоблювальні елементи</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation>Елементи трансформації</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation>Зірочка…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation>Евольвентна шестерня</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation>Майстер проектування вала</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Measure</source>
      <translation>Вимірювання</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Refresh</source>
      <translation>Оновити</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Toggle 3D</source>
      <translation>Перемкнути 3D виміри</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Helper</source>
      <translation>Помічник з розробки деталі</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Modeling</source>
      <translation>Моделювання розробки деталі</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Length [mm]</source>
      <translation>Довжина [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Diameter [mm]</source>
      <translation>Діаметр [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Inner diameter [mm]</source>
      <translation>Внутрішній діаметр [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Constraint type</source>
      <translation>Тип обмеження</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>Start edge type</source>
      <translation>Тип початкового ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>Start edge size</source>
      <translation>Розмір початкового ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="54"/>
      <source>End edge type</source>
      <translation>Тип кінцевого ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>Розмір кінцевого ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="69"/>
      <source>Shaft Wizard</source>
      <translation>Майстер вала</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="77"/>
      <source>Section 1</source>
      <translation>Переріз 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="78"/>
      <source>Section 2</source>
      <translation>Переріз 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="82"/>
      <source>Add column</source>
      <translation>Додати стовпчик</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="130"/>
      <source>Section %s</source>
      <translation>Переріз %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="178"/>
      <source>None</source>
      <translation>Немає</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Fixed</source>
      <translation>Зафіксовано</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Force</source>
      <translation>Сила</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Bearing</source>
      <translation>Підшипник</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="163"/>
      <source>Gear</source>
      <translation>Шестерня</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="164"/>
      <source>Pulley</source>
      <translation>Шків</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="181"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="182"/>
      <source>Fillet</source>
      <translation>Заокруглення</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="60"/>
      <source>All</source>
      <translation>Всі</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="120"/>
      <source>Missing Module</source>
      <translation>Відсутній модуль</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="126"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>Доповнення Plot не встановлено. Встановіть його для активації цієї функції.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="253"/>
      <source>Shaft design wizard...</source>
      <translation>Майстер конструювання валів...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="256"/>
      <source>Start the shaft design wizard</source>
      <translation>Запустити майстер конструювання валу</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="406"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>Зв'язаний об'єкт не є елементом "Проєктування деталі"</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="415"/>
      <source>Tip shape is empty</source>
      <translation>Форма активного елемента порожня</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="68"/>
      <source>BaseFeature link is not set</source>
      <translation>Посилання на базовий елемент не встановлено</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="74"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>Базовий елемент повинен бути Деталь::Елемент</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="84"/>
      <source>BaseFeature has an empty shape</source>
      <translation>Базовий елемент має порожню форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="77"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Неможливо виконати булеве вирізання без базового елемента</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="94"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Не можна робити булеві операції ні з чим, крім Деталь::Елемент та його похідних</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="106"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Неможливо виконати булеву операцію з неправильною базовою формою</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2091"/>
      <location filename="../../../App/FeatureDraft.cpp" line="335"/>
      <location filename="../../../App/FeatureFillet.cpp" line="142"/>
      <location filename="../../../App/FeatureHelix.cpp" line="420"/>
      <location filename="../../../App/FeatureHelix.cpp" line="442"/>
      <location filename="../../../App/FeatureHelix.cpp" line="488"/>
      <location filename="../../../App/FeatureLoft.cpp" line="334"/>
      <location filename="../../../App/FeatureLoft.cpp" line="378"/>
      <location filename="../../../App/FeaturePipe.cpp" line="537"/>
      <location filename="../../../App/FeaturePipe.cpp" line="571"/>
      <location filename="../../../App/FeaturePipe.cpp" line="600"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="161"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="775"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="791"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="804"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="196"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="217"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>Результат містить кілька суцільних тіл: увімкніть 'Дозволити складений об'єкт' в активному тілі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="116"/>
      <source>Tool shape is null</source>
      <translation>Форма інструменту порожня</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="143"/>
      <source>Unsupported boolean operation</source>
      <translation>Непідтримувана логічна операція</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="353"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>Неможливо створити видавлювання із загальною довжиною нуль.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="358"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>Неможливо створити виріз із загальною довжиною нуль.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="706"/>
      <source>No extrusion geometry was generated.</source>
      <translation>Геометрію видавлювання не згенеровано.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="730"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>Результуюче злите видавлювання порожнє.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="371"/>
      <location filename="../../../App/FeaturePipe.cpp" line="592"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="141"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="766"/>
      <source>Resulting shape is not a solid</source>
      <translation>Отримана форма не є твердотільною</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Failed to create chamfer</source>
      <translation>Не вдалося створити фаску</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="330"/>
      <location filename="../../../App/FeatureFillet.cpp" line="122"/>
      <source>Resulting shape is null</source>
      <translation>Результуюча форма є нульовою</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="144"/>
      <source>No edges specified</source>
      <translation>Ребра не вказані</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="211"/>
      <source>Chamfer failed: OCC kernel error in chamfer computation</source>
      <translation>Помилка фаски: помилка ядра OCC при обчисленні фаски</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="302"/>
      <source>Size must be greater than zero</source>
      <translation>Розмір повинен бути більше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="313"/>
      <source>Size2 must be greater than zero</source>
      <translation>Розмір 2 повинен бути більше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="320"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>Кут повинен бути більше 0 і менше 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="97"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>Округлення неможливе для вибраних форм</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="105"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Радіус заокруглення повинен бути більшим за нуль</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="159"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>Операція скруглення не вдалася. Обрані краї можуть містити геометрію, яка не може бути скруглена разом. Спробуйте скруглити кожний край окремо або з меншим радіусом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1739"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>Неможливо створити запитану функцію. Причиною може бути те, що:
  - активне тіло не містить базової форми, тому немає матеріалу 
для видалення;
  - виділений ескіз не належить до активного тіла.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="402"/>
      <source>Failed to obtain profile shape</source>
      <translation>Не вдалося отримати форму профілю</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="456"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Створення не вдалося, оскільки напрямок ортогональний до вектора нормалі ескізу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="479"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="132"/>
      <source>Creating a face from sketch failed</source>
      <translation>Створення поверхні з ескізу не вдалося</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="152"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>Вісь обертання перетинає ескіз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="202"/>
      <source>Could not revolve the sketch!</source>
      <translation>Не вдалося обернути ескіз!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="69"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Не вдалося створити грань з ескізу.
Пересічні об'єкти ескізу в ескізі не допускаються.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="237"/>
      <source>Error: Pitch too small!</source>
      <translation>Помилка: Занадто малий крок!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="242"/>
      <location filename="../../../App/FeatureHelix.cpp" line="265"/>
      <source>Error: height too small!</source>
      <translation>Помилка: Висота занадто мала!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="251"/>
      <source>Error: pitch too small!</source>
      <translation>Помилка: Занадто малий крок!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="256"/>
      <location filename="../../../App/FeatureHelix.cpp" line="270"/>
      <location filename="../../../App/FeatureHelix.cpp" line="279"/>
      <source>Error: turns too small!</source>
      <translation>Помилка: Число обертів замале!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="285"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Помилка: Висота та приріст не повинні бути нульовими!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="303"/>
      <source>Error: unsupported mode</source>
      <translation>Помилка: не підтримуваний режим</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="317"/>
      <source>Error: No valid sketch or face</source>
      <translation>Помилка: Немає допустимого ескізу або грані</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="330"/>
      <source>Error: Face must be planar</source>
      <translation>Помилка: Грань повинна бути плоскою</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2454"/>
      <location filename="../../../App/FeatureHelix.cpp" line="454"/>
      <location filename="../../../App/FeatureHelix.cpp" line="499"/>
      <source>Error: Result is not a solid</source>
      <translation>Помилка: Результат не твердотільний</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="415"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Помилка: Немає чого віднімати</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="437"/>
      <source>Error: Adding the helix failed</source>
      <translation>Помилка: Не вдалося додати спіраль</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="472"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>Помилка: Не вдалося перетнути спіральну лінію</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="481"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>Помилка: Віднімання спіралі не вдалося</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="515"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Помилка: Неможливо створити поверхню з ескізу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1233"/>
      <source>Thread type is invalid</source>
      <translation>Тип різьби недійсний</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1752"/>
      <source>Hole error: Diameter too small</source>
      <translation>Помилка створення отвору: Занадто малий діаметр</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1789"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Помилка отвору: Непідтримувана специфікація довжини</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1795"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Помилка отвору: Неправильна глибина отвору</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1821"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Помилка отвору: Неприпустимий кут зенкування</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1845"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Помилка отвору: Занадто малий діаметр отвору</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1850"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Помилка отвору: Глибина цековки повинна бути меншою за глибину отвору</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1857"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Помилка отвору: Глибина цековки повинна бути більшою або дорівнювати нулю</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1887"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Помилка отвору: Неправильне зенкування</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1923"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Помилка отвору: Неправильний кут свердла</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1940"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Помилка отвору: Неправильна точка свердління</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1977"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Помилка отвору: Не вдалося обернути ескіз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1984"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Помилка отвору: Результуюча форма порожня</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2007"/>
      <source>Hole error: Finding axis failed</source>
      <translation>Помилка отвору: не вдалося знайти вісь</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2063"/>
      <location filename="../../../App/FeatureHole.cpp" line="2071"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Помилка логічної операції на профілі ребра</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2078"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>Логічна операція призвела до появлення несуцільних елементів на профілі ребра</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="153"/>
      <source>Boolean operation failed</source>
      <translation>Не вдалося виконати логічну операцію</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2104"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Неможливо створити грані з ескізу.
Пересічні об'єкти ескізу або декілька граней в ескізі не дозволяють створити кишеню на грані.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2277"/>
      <source>Thread type out of range</source>
      <translation>Тип різьби знаходиться поза діапазоном</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2280"/>
      <source>Thread size out of range</source>
      <translation>Розмір різьби поза діапазоном</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2428"/>
      <source>Error: Thread could not be built</source>
      <translation>Помилка: Не вдалося створити різьбу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="193"/>
      <source>Loft: At least one section is needed</source>
      <translation>Лофт: Потрібен хоча б один переріз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="395"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Лофт: Виникла фатальна помилка при створенні</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="240"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Лофт: Не вдалося створити грань з ескізу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="304"/>
      <location filename="../../../App/FeaturePipe.cpp" line="500"/>
      <source>Loft: Failed to create shell</source>
      <translation>Лофт: Не вдалося створити оболонку</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="819"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Не вдалося створити грань з ескізу.
Ескізи з пересічними елементами або множинними контурами не допускаються.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="211"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Профіль по траєкторії: Не вдалося отримати форму профілю</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="218"/>
      <source>No spine linked</source>
      <translation>Немає прив'язки до напрямної</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="233"/>
      <source>No auxiliary spine linked.</source>
      <translation>Немає прив'язки до допоміжних напрямних.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="255"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Профіль по траєкторії: У разі використання точки як перерізу ескіз повинен містити тільки одну точку</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="264"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Профіль по траєкторії: У разі використання точки як профілю потрібна щонайменше один переріз</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="282"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>Труба: всі перерізи мають бути елементами Part</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="290"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Профіль по траєкторії: Не вдалося отримати форму перерізу</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="298"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Профіль по траєкторії: вершинами можуть бути тільки профіль і останній переріз</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="311"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Різні перерізи повинні мати таку саму кількість внутрішніх поліліній, як і базовий переріз</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="344"/>
      <source>Path must not be a null shape</source>
      <translation>Шлях не може бути пустою формою</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="384"/>
      <source>Pipe could not be built</source>
      <translation>Трубу не вдалося побудувати</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="532"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Профіль по траєкторії: Тут нічого віднімати</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="584"/>
      <source>Pipe: Invalid Boolean Type</source>
      <translation>Pipe: Неправильний логічний тип</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="614"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>При створенні труби сталася фатальна помилка</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="741"/>
      <source>Invalid element in spine.</source>
      <translation>Недопустимий елемент у каркасі.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="746"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Елемент траєкторії не є ні ребром, ні дротом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="759"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Каркас не є ні ребром, ні дротом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="764"/>
      <source>Invalid spine.</source>
      <translation>Неприпустимий каркас.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="103"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Неможливо відняти примітивний елемент без базового елемента</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="356"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="125"/>
      <source>Unknown operation type</source>
      <translation>Невідомий тип операції</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="364"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="133"/>
      <source>Failed to perform boolean operation</source>
      <translation>Не вдалося виконати логічну операцію</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="217"/>
      <source>Length of box too small</source>
      <translation>Занадто мала довжина коробки</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="222"/>
      <source>Width of box too small</source>
      <translation>Занадто мала ширина коробки</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="227"/>
      <source>Height of box too small</source>
      <translation>Занадто мала висота коробки</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="275"/>
      <source>Radius of cylinder too small</source>
      <translation>Занадто малий радіус циліндра</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="280"/>
      <source>Height of cylinder too small</source>
      <translation>Занадто мала висота циліндра</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="285"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Кут повороту циліндра занадто малий</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="342"/>
      <source>Radius of sphere too small</source>
      <translation>Занадто малий радіус сфери</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="394"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="399"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Радіус конуса не може бути від'ємним</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="404"/>
      <source>Height of cone too small</source>
      <translation>Занадто мала висота конуса</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="484"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="489"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Радіус еліпсоїда занадто малий</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="583"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="588"/>
      <source>Radius of torus too small</source>
      <translation>Занадто малий радіус тора</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="673"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Багатокутник призми є неправильним, він повинен мати 3 або більше сторін</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="678"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Радіус багатокутника, призми, занадто малий</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="683"/>
      <source>Height of prism is too small</source>
      <translation>Висота призми занадто мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="770"/>
      <source>delta x of wedge too small</source>
      <translation>дельта X клина занадто мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="776"/>
      <source>delta y of wedge too small</source>
      <translation>дельта Y клина занадто мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="782"/>
      <source>delta z of wedge too small</source>
      <translation>дельта Z клина занадто мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="788"/>
      <source>delta z2 of wedge is negative</source>
      <translation>дельта Z2 клину від'ємна</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="794"/>
      <source>delta x2 of wedge is negative</source>
      <translation>дельта Х2 клину від'ємна</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="96"/>
      <source>Angle of revolution too large</source>
      <translation>Кут повороту занадто великий</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="103"/>
      <source>Angle of revolution too small</source>
      <translation>Кут повороту занадто малий</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="110"/>
      <source>Angles of revolution nullify each other</source>
      <translation>Кути обертання взаємно анулюються</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="126"/>
      <source>Reference axis is invalid</source>
      <translation>Базова вісь недійсна</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="758"/>
      <source>Fusion with base feature failed</source>
      <translation>Злиття з базовою функцією не вдалося</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="101"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Особливість перетворення Зв'язаний об'єкт не є об'єктом Деталі</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="109"/>
      <source>No features selected to be mirrored.</source>
      <translation>Не вибрано елементів для дзеркального відображення.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>No features selected to be patterned.</source>
      <translation>Не вибрано елементів для масиву.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="115"/>
      <source>No features selected to be transformed.</source>
      <translation>Не вибрано елементів для трансформації.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="379"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Неможливо перетворити неправильну форму фігури</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="430"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>Не вибрано частину для віднімання/злиття</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="421"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Тільки ознаки, що додаються і віднімаються, можуть бути перетворені</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="109"/>
      <source>Invalid face reference</source>
      <translation>Неправильне посилання на грань</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Involute Gear</source>
      <translation>Евольвентна шестерня</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="66"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>Створює або редагує визначення евольвентної шестерні</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="65"/>
      <source>Sprocket</source>
      <translation>Зірочка</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="69"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>Створює або редагує визначення зірочки.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>Показати кінцевий результат</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>Показати накладений попередній перегляд</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="52"/>
      <source>Preview</source>
      <translation>Попередній перегляд</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="225"/>
      <source>Shaft Design Wizard</source>
      <translation>Майстер проектування вала</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="228"/>
      <source>Starts the shaft design wizard</source>
      <translation>Запускає майстер проектування вала</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="87"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>Помилка при обчисленні попереднього перегляду видаленого об'єму: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="125"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>Результуюча форма порожня. Це може означати, що матеріал не буде видалено або є проблема з моделлю.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2658"/>
      <source>Create Datum</source>
      <translation>Створити опорний об'єкт</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2659"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Створює опорний об'єкт або локальну систему координат</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2693"/>
      <source>Create Datum</source>
      <translation>Створити опорний об'єкт</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2694"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Створює опорний об'єкт або локальну систему координат</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>Створює адитивний паралелепіпед за шириною, висотою та довжиною</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>Створює адитивний циліндр за радіусом, висотою та кутом</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="235"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>Створює адитивну сферу за радіусом та різними кутами</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <source>Creates an additive cone</source>
      <translation>Створює адитивний конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="250"/>
      <source>Creates an additive ellipsoid</source>
      <translation>Створює адитивний еліпсоїд</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="256"/>
      <source>Creates an additive torus</source>
      <translation>Створює адитивний тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="262"/>
      <source>Creates an additive prism</source>
      <translation>Створює адитивну призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="268"/>
      <source>Creates an additive wedge</source>
      <translation>Створює адитивний клин</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="402"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>Створює субтрактивний паралелепіпед за шириною, висотою та довжиною</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="411"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>Створює субтрактивний циліндр за радіусом, висотою та кутом</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="420"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>Створює субтрактивну сферу за радіусом та різними кутами</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="429"/>
      <source>Creates a subtractive cone</source>
      <translation>Створює субтрактивний конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="435"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>Створює субтрактивний еліпсоїд</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="441"/>
      <source>Creates a subtractive torus</source>
      <translation>Створює субтрактивний тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="447"/>
      <source>Creates a subtractive prism</source>
      <translation>Створює субтрактивну призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="453"/>
      <source>Creates a subtractive wedge</source>
      <translation>Створює субтрактивний клин</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1103"/>
      <source>Attachment</source>
      <translation>Приєднання</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="803"/>
      <source>Revolution Parameters</source>
      <translation>Параметри обертання</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="813"/>
      <source>Groove Parameters</source>
      <translation>Параметри проточки</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="39"/>
      <source>Transformed Feature Messages</source>
      <translation>Повідомлення трансформованого елемента</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="199"/>
      <source>Active Body</source>
      <translation>Активне тіло</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="44"/>
      <source>Chamfer Parameters</source>
      <translation>Параметри фаски</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation>Параметри опорної площини</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation>Параметри опорної лінії</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation>Параметри опорної точки</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation>Параметри локальної системи координат</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="45"/>
      <source>Draft Parameters</source>
      <translation>Параметри ухилу</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="44"/>
      <source>Fillet Parameters</source>
      <translation>Параметри заокруглення</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="41"/>
      <source>Linear Pattern Parameters</source>
      <translation>Параметри лінійного масиву</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="41"/>
      <source>Mirror Parameters</source>
      <translation>Параметри дзеркала</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="41"/>
      <source>Multi-Transform Parameters</source>
      <translation>Параметри множинної трансформації</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="41"/>
      <source>Polar Pattern Parameters</source>
      <translation>Параметри кругового масиву</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="41"/>
      <source>Scale Parameters</source>
      <translation>Параметри масштабу</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="44"/>
      <source>Thickness Parameters</source>
      <translation>Параметри товщини</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="148"/>
      <source>Direction 2</source>
      <translation>Напрямок 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="267"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>Виберіть посилання напрямку (ребро, грань, опорна лінія)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="355"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>Недійсний вибір. Виберіть ребро, плоску грань або опорну лінію.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="443"/>
      <source>Input Error</source>
      <translation>Помилка введення</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="138"/>
      <source>%1 fine dragging</source>
      <translation>%1 точне перетягування</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="141"/>
      <source>%1 coarse dragging</source>
      <translation>%1 грубе перетягування</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="264"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Adjust the parameters and try again.</source>
      <translation>Функція не може бути створена за допомогою заданих параметрів.
Геометрія може бути некоректною або параметри можуть бути несумісні.
Налаштуйте параметри і повторіть спробу.</translation>
    </message>
  </context>
</TS>
