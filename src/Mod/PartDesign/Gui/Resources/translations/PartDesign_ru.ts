<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="77"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Центральная точка начала спирали; берется из опорной оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="79"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Направление спирали; берется из опорной оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="81"/>
      <source>The reference axis of the helix.</source>
      <translation>Опорная ось спирали.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="83"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Режим ввода спирали определяет, какие свойства задаются пользователем.
Затем рассчитываются зависимые свойства.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="87"/>
      <source>The axial distance between two turns.</source>
      <translation>Осевое расстояние между двумя оборотами.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="89"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Высота траектории спирали, не учитывающая протяженность профиля.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="91"/>
      <source>The number of turns in the helix.</source>
      <translation>Количество витков в спирали.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>Угол конуса, который образует оболочку вокруг спирали.
Ненулевые значения превращают спираль в коническую спираль.
При положительных значениях радиус увеличивается, а при отрицательных - уменьшается. </translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="99"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Увеличение радиуса спирали за один оборот.
Ненулевые значения превращают спираль в коническую спираль.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="102"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Устанавливает направление поворота на левостороннее,
то есть против часовой стрелки при движении вдоль своей оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="105"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Определяет, указывает ли спираль в направлении, противоположном оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="107"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Если установлено, результатом будет пересечение профиля и ранее существовавшего тела.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="109"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Если false, инструмент предложит начальное значение шага на основе ограничивающей рамки профиля,
чтобы избежать самопересечения.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="112"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Допуск на слияние для спирали, увеличьте, если спиральная форма не обеспечивает хорошего слияния с деталью.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="98"/>
      <source>Number of gear teeth</source>
      <translation>Число зубов передачи</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="104"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Угол давления зубьев передачи</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="101"/>
      <source>Module of the gear</source>
      <translation>Модуль шестерни</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=2 кривые с каждым 3-мя контрольными точками, False=1 кривая с четырьмя контрольными точками.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=внешняя шестерня, False=внутренняя шестерня</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>Высота зуба от делительной окружности до его вершины, нормированная по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>Высота зуба от делительной окружности до его корня, нормированная по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Радиус галтели у корня зуба, нормированный по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>Расстояние, на которое эталонный профиль сдвинут наружу, нормализуется модулем.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1498"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1499"/>
      <source>Additive Helix</source>
      <translation>Аддитивная спираль</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1500"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Перемещает выбранный эскиз или профиль по спирали и добавляет его к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1405"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1406"/>
      <source>Additive Loft</source>
      <translation>Аддитивный профиль по сечениям</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1407"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>Сращивает выбранные эскизы между собой и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1311"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1312"/>
      <source>Additive Pipe</source>
      <translation>Аддитивная труба</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1313"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Перемещает выбранный эскиз или профиль вдоль траектории и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="88"/>
      <source>New Body</source>
      <translation>Новое тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="89"/>
      <source>Creates a new body and activates it</source>
      <translation type="unfinished">Creates a new body and activates it</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2309"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2310"/>
      <source>Boolean Operation</source>
      <translation>Булева операция</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2311"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation type="unfinished">Applies boolean operations with the selected objects and the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Local Coordinate System</source>
      <translation>Локальная система координат </translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>Creates a new local coordinate system</source>
      <translation type="unfinished">Creates a new local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1778"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1779"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1780"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation type="unfinished">Applies a chamfer to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="430"/>
      <source>Clone</source>
      <translation>Клонировать</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="431"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation type="unfinished">Copies a solid object parametrically as the base feature of a new body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1807"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1808"/>
      <source>Draft</source>
      <translation>Уклон</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1809"/>
      <source>Applies a draft to the selected faces</source>
      <translation type="unfinished">Applies a draft to the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="614"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="615"/>
      <source>Duplicate &amp;Object</source>
      <translation type="unfinished">Duplicate &amp;Object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Дублирует выбранный объект и добавляет его в активное тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1750"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1751"/>
      <source>Fillet</source>
      <translation>Скругление</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1752"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation type="unfinished">Applies a fillet to the selected edges or faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1248"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1249"/>
      <source>Groove</source>
      <translation>Паз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1250"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation type="unfinished">Revolves the sketch or profile around a line or axis and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1150"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1151"/>
      <source>Hole</source>
      <translation>Отверстие</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1152"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation type="unfinished">Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>Datum Line</source>
      <translation type="unfinished">Datum Line</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Creates a new datum line</source>
      <translation type="unfinished">Creates a new datum line</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2044"/>
      <source>Linear Pattern</source>
      <translation type="unfinished">Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2045"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation type="unfinished">Duplicates the selected features or the active body in a linear pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="320"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="321"/>
      <source>Migrate</source>
      <translation>Миграция</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="322"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation type="unfinished">Migrates the document to the modern Part Design workflow</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="1992"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1993"/>
      <source>Mirror</source>
      <translation>Зеркально</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1994"/>
      <source>Mirrors the selected features or active body</source>
      <translation type="unfinished">Mirrors the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="674"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="675"/>
      <source>Move Object To…</source>
      <translation type="unfinished">Move Object To…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="676"/>
      <source>Moves the selected object to another body</source>
      <translation>Перемещает выделенный объект в другое тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="841"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="842"/>
      <source>Move Feature After…</source>
      <translation type="unfinished">Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="843"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation type="unfinished">Moves the selected feature after another feature in the same body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="535"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="536"/>
      <source>Set Tip</source>
      <translation type="unfinished">Set Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="537"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation type="unfinished">Moves the tip of the body to the selected feature</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2194"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2195"/>
      <source>Multi-Transform</source>
      <translation type="unfinished">Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2196"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation type="unfinished">Applies multiple transformations to the selected features or active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="503"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="504"/>
      <source>New Sketch</source>
      <translation type="unfinished">New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="505"/>
      <source>Creates a new sketch</source>
      <translation type="unfinished">Creates a new sketch</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1092"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1093"/>
      <source>Pad</source>
      <translation>Выдавливание</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1094"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>Datum Plane</source>
      <translation type="unfinished">Datum Plane</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="165"/>
      <source>Creates a new datum plane</source>
      <translation type="unfinished">Creates a new datum plane</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1121"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1122"/>
      <source>Pocket</source>
      <translation>Карман</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1123"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation type="unfinished">Extrudes the selected sketch or profile and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Point</source>
      <translation type="unfinished">Datum Point</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum point</source>
      <translation type="unfinished">Creates a new datum point</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2097"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2098"/>
      <source>Polar Pattern</source>
      <translation type="unfinished">Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2099"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation type="unfinished">Duplicates the selected features or the active body in a circular pattern</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1193"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1194"/>
      <source>Revolve</source>
      <translation>Тело вращения</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1195"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation type="unfinished">Revolves the selected sketch or profile around a line or axis and adds it to the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2152"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2153"/>
      <source>Scale</source>
      <translation>Масштаб</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2154"/>
      <source>Scales the selected features or the active body</source>
      <translation type="unfinished">Scales the selected features or the active body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Shape Binder</source>
      <translation type="unfinished">Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new shape binder</source>
      <translation type="unfinished">Creates a new shape binder</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Sub-Shape Binder</source>
      <translation>Связующее под-объектов</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="347"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation type="unfinished">Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1570"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1571"/>
      <source>Subtractive Helix</source>
      <translation type="unfinished">Subtractive Helix</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1572"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a helix and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1452"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1453"/>
      <source>Subtractive Loft</source>
      <translation type="unfinished">Subtractive Loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1454"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Lofts the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1358"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1359"/>
      <source>Subtractive Pipe</source>
      <translation type="unfinished">Subtractive Pipe</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1360"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation type="unfinished">Sweeps the selected sketch or profile along a path and removes it from the body</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1875"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1876"/>
      <source>Thickness</source>
      <translation>Оболочка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1877"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation type="unfinished">Applies thickness and removes the selected faces</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <source>Additive Primitive</source>
      <translation type="unfinished">Additive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Creates an additive primitive</source>
      <translation type="unfinished">Creates an additive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="197"/>
      <source>Additive Box</source>
      <translation>Аддитивный Параллелепипед</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="201"/>
      <source>Additive Cylinder</source>
      <translation>Аддитивный Цилиндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="205"/>
      <source>Additive Sphere</source>
      <translation>Аддитивная Сфера</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="209"/>
      <source>Additive Cone</source>
      <translation>Аддитивный Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Ellipsoid</source>
      <translation>Аддитивный Эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Additive Torus</source>
      <translation>Аддитивный Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="221"/>
      <source>Additive Prism</source>
      <translation>Аддитивная Призма</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="225"/>
      <source>Additive Wedge</source>
      <translation>Аддитивный Клин</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="241"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Subtractive Primitive</source>
      <translation type="unfinished">Subtractive Primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>Creates a subtractive primitive</source>
      <translation type="unfinished">Creates a subtractive primitive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="349"/>
      <source>Subtractive Box</source>
      <translation>Субтрактивный Куб (Параллелепипед)</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="353"/>
      <source>Subtractive Cylinder</source>
      <translation>Субтрактивный Цилиндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="357"/>
      <source>Subtractive Sphere</source>
      <translation>Субтрактивная Сфера</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="361"/>
      <source>Subtractive Cone</source>
      <translation>Субтрактивный Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="365"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Субтрактивный Эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="369"/>
      <source>Subtractive Torus</source>
      <translation>Субтрактивный Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="373"/>
      <source>Subtractive Prism</source>
      <translation>Субтрактивная Призма</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="377"/>
      <source>Subtractive Wedge</source>
      <translation>Субтрактивный Клин</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="300"/>
      <source>Edit Shape Binder</source>
      <translation type="unfinished">Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="309"/>
      <source>Create Shape Binder</source>
      <translation type="unfinished">Create Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="392"/>
      <source>Create Sub-Shape Binder</source>
      <translation type="unfinished">Create Sub-Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="447"/>
      <source>Create Clone</source>
      <translation>Клонировать</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="993"/>
      <source>Make Copy</source>
      <translation type="unfinished">Make Copy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2239"/>
      <source>Convert to Multi-Transform feature</source>
      <translation type="unfinished">Convert to Multi-Transform feature</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="258"/>
      <source>Sketch on Face</source>
      <translation type="unfinished">Sketch on Face</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="317"/>
      <source>Make copy</source>
      <translation>Сделать копию</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="514"/>
      <location filename="../../SketchWorkflow.cpp" line="742"/>
      <source>New Sketch</source>
      <translation type="unfinished">New Sketch</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2327"/>
      <source>Create Boolean</source>
      <translation>Булева операция</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="193"/>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <source>Add a Body</source>
      <translation>Добавить тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="438"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation type="unfinished">Migrate legacy Part Design features to bodies</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="628"/>
      <source>Duplicate a Part Design object</source>
      <translation type="unfinished">Duplicate a Part Design object</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="911"/>
      <source>Move a feature inside body</source>
      <translation type="unfinished">Move a feature inside body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="583"/>
      <source>Move tip to selected feature</source>
      <translation>Переместить подсказку к выбранному объекту</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>Move an object</source>
      <translation>Переместить объект</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="264"/>
      <source>Mirror</source>
      <translation>Зеркально</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="301"/>
      <source>Linear Pattern</source>
      <translation type="unfinished">Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="345"/>
      <source>Polar Pattern</source>
      <translation type="unfinished">Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="382"/>
      <source>Scale</source>
      <translation>Масштаб</translation>
    </message>
  </context>
  <context>
    <name>FeaturePickDialog</name>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="42"/>
      <source>Valid</source>
      <translation>Верно</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="43"/>
      <source>Invalid shape</source>
      <translation>Неверная фигура</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>Нет ломаных в эскизе</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>Эскиз уже используется другим элементом</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another body feature</source>
      <translation type="unfinished">Sketch belongs to another body feature</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the tip of the body</source>
      <translation type="unfinished">Feature is located after the tip of the body</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>Базовая плоскость</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation type="unfinished">Face Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation type="unfinished">Edge Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation type="unfinished">Boolean Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation type="unfinished">Helper Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation type="unfinished">Modeling Tools</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>Создать геометрию</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation type="unfinished">Involute Parameter</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation type="unfinished">Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation type="unfinished">Module</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation type="unfinished">Pressure angle</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation type="unfinished">High precision</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Да</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Нет</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation type="unfinished">External gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation type="unfinished">Addendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation type="unfinished">Dedendum coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation type="unfinished">Root fillet coefficient</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation type="unfinished">Profile shift coefficient</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Требуется активное тело</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation type="unfinished">To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation type="unfinished">Create New Body</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
      <source>Please select</source>
      <translation>Пожалуйста, выберите</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Геометрические примитивы</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Угол в первом направлении</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Угол во втором направлении</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>Длина</translation>
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
      <translation>Высота</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>Радиус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>Угол вращения</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>Радиус 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>Радиус 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>Угол</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation type="unfinished">U parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation type="unfinished">V parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Радиус в локальном Z-направлении</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation type="unfinished">Radius in local X-direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation type="unfinished">Radius 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation type="unfinished">Radius in local Y-direction
If zero, it is equal to Radius2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation type="unfinished">V parameter</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation type="unfinished">Radius in local XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation type="unfinished">Radius in local XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Многоугольник, Полигон</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation type="unfinished">Circumradius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation type="unfinished">X min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation type="unfinished">Y min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation type="unfinished">Z min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation type="unfinished">X2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation type="unfinished">Z2 min/max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Шаг</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation type="unfinished">Coordinate system</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Прирост</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation type="unfinished">Number of rotations</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation type="unfinished">Angle 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation type="unfinished">Angle 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation type="unfinished">From 3 Points</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation type="unfinished">Major radius</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation type="unfinished">Minor radius</translation>
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
      <translation>Для правшей</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Режим левши</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Начальная точка</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Конечная точка</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Ссылка</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Вы выбрали геометрию, которая не является частью активного тела. Определите, как поступить с выборкой. В противном случае отмените команду.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Создать независимую копию (рекомендуется)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Сделать зависимую копию</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Создать перекрестную ссылку</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="274"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Выбор этого вызовет циклическую зависимость.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation type="unfinished">Add Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation type="unfinished">Remove Body</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Объединение</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Обрезать</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Пересечение</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="54"/>
      <source>Boolean Parameters</source>
      <translation type="unfinished">Boolean Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="87"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="51"/>
      <source>Primitive Parameters</source>
      <translation type="unfinished">Primitive Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="920"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="926"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <source>Invalid wedge parameters</source>
      <translation>Неверные параметры клина</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="921"/>
      <source>X min must not be equal to X max!</source>
      <translation>X мин. не должен быть равен X макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="927"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y мин. не должен быть равен Y макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z мин. не должен быть равен Z макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="971"/>
      <source>Create primitive</source>
      <translation>Создать примитив</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- выберите элемент, чтобы выделить его
- дважды щелкните по элементу, чтобы увидеть фаски</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>Тип</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>Равное расстояние</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>Два расстояния</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>Расстояние и угол</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation type="unfinished">Flips the direction</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation type="unfinished">Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>Размер 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>Угол</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="347"/>
      <source>Empty chamfer created!
</source>
      <translation type="unfinished">Empty chamfer created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="388"/>
      <source>Empty body list</source>
      <translation>Пустой список тел</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="389"/>
      <source>The body list cannot be empty</source>
      <translation>Список тел не может быть пустым</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="403"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Логическое значение: Принять: Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="101"/>
      <source>Incompatible Reference Set</source>
      <translation type="unfinished">Incompatible Reference Set</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="102"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Нет режимов присоединения, которому соответствует текущий набор ориентиров. Если Вы выберете "Продолжить", элемент останется там, где он сейчас находится, и не будет перенесён при изменении базового объекта. Продолжить?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="198"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="407"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- выберите элемент, чтобы выделить его
- дважды щелкните по элементу, чтобы увидеть черновики (прим. эскизы)</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Угол уклона</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation type="unfinished">Neutral Plane</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation type="unfinished">Pull Direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Развернуть направление уклона</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="289"/>
      <source>Empty draft created!
</source>
      <translation type="unfinished">Empty draft created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="271"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="276"/>
      <source>Confirm Selection</source>
      <translation type="unfinished">Confirm Selection</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="288"/>
      <source>Add All Edges</source>
      <translation type="unfinished">Add All Edges</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation type="unfinished">Adds all edges to the list box (only when in add selection mode)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="301"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>No face selected</source>
      <translation>Грань не выбрана</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="163"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1140"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="351"/>
      <source>Preview</source>
      <translation>Предварительный просмотр</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="355"/>
      <source>Select Faces</source>
      <translation type="unfinished">Select Faces</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="690"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="598"/>
      <source>No shape selected</source>
      <translation>Профиль не выбран</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="683"/>
      <source>Sketch normal</source>
      <translation>Нормаль эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="686"/>
      <source>Face normal</source>
      <translation>Нормаль грани</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="694"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>Произвольное направление</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1086"/>
      <source>Click on a shape in the model</source>
      <translation>Нажмите на форму в модели</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1347"/>
      <source>One sided</source>
      <translation type="unfinished">One sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1348"/>
      <source>Two sided</source>
      <translation type="unfinished">Two sided</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1349"/>
      <source>Symmetric</source>
      <translation>Симметрично</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1355"/>
      <source>Click on a face in the model</source>
      <translation>Выберите грань внутри модели</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Разрешить используемые элементы</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation type="unfinished">Allow External Features</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>От других тел этой же детали</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>Из разных деталей или свободных элементов</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Создать независимую копию (рекомендуется)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Сделать зависимую копию</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Создать перекрестную ссылку</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Valid</source>
      <translation>Действительный</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Invalid shape</source>
      <translation>Неверная фигура</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>No wire in sketch</source>
      <translation>Нет ломаных в эскизе</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Sketch already used by other feature</source>
      <translation>Эскиз уже используется другим элементом</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another body</source>
      <translation>Принадлежит другому телу</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Belongs to another part</source>
      <translation>Принадлежит другой детали</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Doesn't belong to any body</source>
      <translation>Не принадлежит ни какому телу</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Base plane</source>
      <translation>Базовая плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="85"/>
      <source>Feature is located after the tip of the body</source>
      <translation type="unfinished">Feature is located after the tip of the body</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Select attachment</source>
      <translation>Выберите место прикрепления</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- выберите элемент, чтобы выделить его
- дважды щелкните по элементу, чтобы увидеть скругления</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>Радиус</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation type="unfinished">Use all edges</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="205"/>
      <source>Empty fillet created!</source>
      <translation>Пустое скругление создано!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Верно</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="244"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="245"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="227"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="226"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Normal sketch axis</source>
      <translation>Нормаль оси Эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>Статус</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>Ось</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="210"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Шаг-Высота-Угол</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Шаг-Поворот-Угол</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Высота-Поворот-Угол</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Высота-Поворот-Приращение</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>Шаг</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>Высота</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation type="unfinished">Turns</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation type="unfinished">Cone angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation type="unfinished">Radial growth</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Левосторонняя спираль</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>В обратную сторону</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>Удалить внешний профиль</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="60"/>
      <source>Helix Parameters</source>
      <translation type="unfinished">Helix Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="229"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="295"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Предупреждение: спираль может быть самопересекающейся</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="300"/>
      <source>Error: helix touches itself</source>
      <translation>Ошибка: спираль касается сама себя</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="349"/>
      <source>Error: unsupported mode</source>
      <translation>Ошибка: неподдерживаемый режим</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Counterbore</source>
      <translation>Цековка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="54"/>
      <source>Countersink</source>
      <translation>Зенковка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterdrill</source>
      <translation>Зенковка с цилиндрическим хвостовиком</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Hole Parameters</source>
      <translation type="unfinished">Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="69"/>
      <source>None</source>
      <translation>Ничего</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>ISO metric regular</source>
      <translation>ISO metric regular</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric fine</source>
      <translation>Метрическая прецизионная резьба ISO </translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>UTS coarse</source>
      <translation>UTS грубый</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS fine</source>
      <translation>UTS fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS extra fine</source>
      <translation>UTS extra fine</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>ANSI pipes</source>
      <translation>ANSI трубы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ISO/BSP pipes</source>
      <translation>Пункты ISO/BSP</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>BSW whitworth</source>
      <translation>Британский стандарт Уитворта по резьбе</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSF whitworth fine</source>
      <translation>BSF британская стандартная резьба Уитворта (BSW) с более мелким шагом</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>ISO tyre valves</source>
      <translation type="unfinished">ISO tyre valves</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="673"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Средний</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="674"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Точно</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="675"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Грубо</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Обычный (либо Нормаль)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="679"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Закрыть</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="680"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Loose (макс. допуск)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="683"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Основная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="684"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Закрыть</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="685"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Wide (макс. допуск)</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Линованная поверхность</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Закрыто</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Профиль</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Объект</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Добавить сечение</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Удалить сечение</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>Список может быть переупорядочен перетаскиванием</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="51"/>
      <source>Loft Parameters</source>
      <translation type="unfinished">Loft Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>Плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="175"/>
      <source>Error</source>
      <translation>Ошибки</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>Преобразования</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="70"/>
      <source>Edit</source>
      <translation>Редактировать</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="76"/>
      <source>Delete</source>
      <translation>Удалить</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="82"/>
      <source>Add Mirror Transformation</source>
      <translation type="unfinished">Add Mirror Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="88"/>
      <source>Add Linear Pattern</source>
      <translation type="unfinished">Add Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="94"/>
      <source>Add Polar Pattern</source>
      <translation type="unfinished">Add Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="100"/>
      <source>Add Scale Transformation</source>
      <translation type="unfinished">Add Scale Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="106"/>
      <source>Move Up</source>
      <translation type="unfinished">Move Up</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="109"/>
      <source>Move Down</source>
      <translation type="unfinished">Move Down</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="143"/>
      <source>Right-click to add a transformation</source>
      <translation type="unfinished">Right-click to add a transformation</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation type="unfinished">Pad Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation type="unfinished">Offset the pad from the face at which the pad will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation type="unfinished">Offset the pad from the face at which the pad will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>Противоположное направление выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>К последнему</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Поднять до грани</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>До формы</translation>
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
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>Длина</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>Отступ от поверхности</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>Выбрать все грани</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation type="unfinished">Select Face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation type="unfinished">Side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>Направление</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="527"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Задайте направление или выберите ребро модели</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="532"/>
      <source>Sketch normal</source>
      <translation>Нормаль эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="542"/>
      <source>Custom direction</source>
      <translation>Произвольное направление</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="552"/>
      <source>Show direction</source>
      <translation>Показать направление</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="562"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Укажите пользовательский вектор для направления кармана,
в противном случае будет использована нормаль эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="671"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Если флажок не установлен, длина будет измеряться вдоль заданного направления</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="675"/>
      <source>Length along sketch normal</source>
      <translation>Длина вдоль нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>Реверсивный</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="520"/>
      <source>Direction/edge</source>
      <translation type="unfinished">Direction/edge</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="537"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="575"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X-component of direction vector</source>
      <translation type="unfinished">X-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="604"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y-component of direction vector</source>
      <translation type="unfinished">Y-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="633"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z-component of direction vector</source>
      <translation type="unfinished">Z-component of direction vector</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>Угол уклона выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation type="unfinished">Side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>Угол сужения</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation type="unfinished">Select Shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation type="unfinished">Selects all faces of the shape</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="685"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Режим ориентации</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Стандарт</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Исправлено</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Френе</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Вспомогательный</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Бинормали</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Кривилинейная эквивалентность</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Профиль</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Объект</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Добавить ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Удалить ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Установить постоянный вектор бинормали, используемый для вычисления ориентации профилей</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="580"/>
      <source>Section Orientation</source>
      <translation type="unfinished">Section Orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="608"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Профиль</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Объект</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation type="unfinished">Corner transition</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation type="unfinished">Right corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation type="unfinished">Round corner</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation type="unfinished">Path to Sweep Along</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Добавить ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation type="unfinished">Remove edge</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Преобразованный</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe Parameters</source>
      <translation type="unfinished">Pipe Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="89"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="448"/>
      <location filename="../../TaskPipeParameters.cpp" line="560"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="448"/>
      <source>No active body</source>
      <translation>Нет активного Тела</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Режим трансформации</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Постоянная</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Много сечений</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Добавить сечение</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Удалить сечение</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>Список может быть переупорядочен перетаскиванием</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="879"/>
      <source>Section Transformation</source>
      <translation type="unfinished">Section Transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="896"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation type="unfinished">Pocket Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="42"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation type="unfinished">Offset from the selected face at which the pocket will end on side 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation>Переворачивает направление кармана</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Through all</source>
      <translation>Насквозь</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>До грани</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>До формы</translation>
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
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="193"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="242"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>Симметрично плоскости</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>Реверсивный</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>Второй угол</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>Ось</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="249"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Angle</source>
      <translation>Угол</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="157"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="475"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="195"/>
      <source>To last</source>
      <translation>К последнему</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="198"/>
      <source>Through all</source>
      <translation>Насквозь</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="200"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="201"/>
      <source>Up to face</source>
      <translation>До грани</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Two dimensions</source>
      <translation>Два размера</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="462"/>
      <source>No face selected</source>
      <translation>Грань не выбрана</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>Фактор</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>События</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Объект</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Добавить геометрию</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Удалить геометрию</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation type="unfinished">Shape Binder Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="131"/>
      <source>Remove</source>
      <translation>Удалить</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="189"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation type="unfinished">Toggles between selection and preview mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- выберите элемент, чтобы выделить его
- дважды щелкните по элементу, чтобы увидеть характеристики (features)</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>Толщина</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>Режим</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>Тема оформления</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Труба</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation type="unfinished">Recto verso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>Тип соединения</translation>
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
      <translation>Пересечение</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>Наращивать стены внутрь тела</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="269"/>
      <source>Empty thickness created!
</source>
      <translation type="unfinished">Empty thickness created!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="105"/>
      <source>Remove</source>
      <translation>Удалить</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>Нормаль оси Эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="443"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation type="unfinished">Base X-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation type="unfinished">Base Y-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation type="unfinished">Base Z-axis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base XY-plane</source>
      <translation type="unfinished">Base XY-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base YZ-plane</source>
      <translation type="unfinished">Base YZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="459"/>
      <source>Base XZ-plane</source>
      <translation type="unfinished">Base XZ-plane</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="467"/>
      <source>Select reference…</source>
      <translation type="unfinished">Select reference…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>Преобразование тела</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>Инструмент преобразования фигуры</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation type="unfinished">Add Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation type="unfinished">Remove Feature</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation type="unfinished">Recompute on change</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>Список может быть переупорядочен перетаскиванием</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="744"/>
      <source>Select Body</source>
      <translation type="unfinished">Select Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="745"/>
      <source>Select a body from the list</source>
      <translation>Выберите тело из списка</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="898"/>
      <source>Move Feature After…</source>
      <translation type="unfinished">Move Feature After…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="899"/>
      <source>Select a feature from the list</source>
      <translation>Выбрать черту из списка</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="973"/>
      <source>Move Tip</source>
      <translation type="unfinished">Move Tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="975"/>
      <source>Set tip to last feature?</source>
      <translation type="unfinished">Set tip to last feature?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="974"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>Перемещенная характеристика появляется после текущей подсказки.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>Invalid selection</source>
      <translation>Неверный выбор</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Нет режимов присоединения, которые соответствуют выбранным объектам. Выберите что-нибудь другое.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <location filename="../../Command.cpp" line="149"/>
      <location filename="../../Command.cpp" line="151"/>
      <source>Error</source>
      <translation>Ошибка</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="775"/>
      <source>Several sub-elements selected</source>
      <translation>Неправильное выделение</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="776"/>
      <source>Select a single face as support for a sketch!</source>
      <translation type="unfinished">Select a single face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="780"/>
      <source>Select a face as support for a sketch!</source>
      <translation type="unfinished">Select a face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="784"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation type="unfinished">Need a planar face as support for a sketch!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="788"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation type="unfinished">Create a plane first or select a face to sketch on</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="779"/>
      <source>No support face selected</source>
      <translation>Не выбрана грань</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="783"/>
      <source>No planar support</source>
      <translation>Неплоская грань</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="787"/>
      <source>No valid planes in this document</source>
      <translation>В документе нет корректных плоскостей</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1014"/>
      <location filename="../../SketchWorkflow.cpp" line="702"/>
      <location filename="../../ViewProvider.cpp" line="139"/>
      <location filename="../../ViewProviderDatum.cpp" line="250"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Диалог уже открыт в панели задач</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="894"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Невозможно применить данную команду, в связи с отсутствием твердого для вычитания формы.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="895"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Убедитесь в том что тело содержит хоть какую-нибудь форму, перед попыткой применения субтрактивной команды.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="916"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Невозможно использовать выбранный объект. Выбранный объект должен принадлежать активному телу</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation type="unfinished">There is no active body. Please activate a body before inserting a datum entity.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>Sub-shape binder</source>
      <translation type="unfinished">Sub-shape binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="939"/>
      <source>No sketch to work on</source>
      <translation>Не найден эскиз для работы с ним</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="940"/>
      <source>No sketch is available in the document</source>
      <translation>В документе отсутствуют эскизы для применения данного действия</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1015"/>
      <location filename="../../SketchWorkflow.cpp" line="703"/>
      <location filename="../../ViewProvider.cpp" line="140"/>
      <location filename="../../ViewProviderDatum.cpp" line="251"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="97"/>
      <source>Close this dialog?</source>
      <translation type="unfinished">Close this dialog?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1633"/>
      <location filename="../../Command.cpp" line="1659"/>
      <source>Wrong selection</source>
      <translation>Неправильный выбор</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1634"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Выберите ребро, грань или тело от одного тела.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1638"/>
      <location filename="../../Command.cpp" line="1970"/>
      <source>Selection is not in the active body</source>
      <translation type="unfinished">Selection is not in the active body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>Shape of the selected part is empty</source>
      <translation type="unfinished">Shape of the selected part is empty</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1639"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Выберите ребро, грань или тело от активного тела.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="917"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation type="unfinished">Consider using a shape binder or a base feature to reference external geometry in a body</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1649"/>
      <source>Wrong object type</source>
      <translation>Неверный тип объекта</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1650"/>
      <source>%1 works only on parts.</source>
      <translation>%1 работает только с деталями.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1971"/>
      <source>Please select only one feature in an active body.</source>
      <translation>Пожалуйста, выберите только один элемент в активном теле.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="69"/>
      <source>Part creation failed</source>
      <translation>Ошибка создания детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="70"/>
      <source>Failed to create a part object.</source>
      <translation>Не удалось создать объект Деталь.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="119"/>
      <location filename="../../CommandBody.cpp" line="124"/>
      <location filename="../../CommandBody.cpp" line="137"/>
      <location filename="../../CommandBody.cpp" line="186"/>
      <source>Bad base feature</source>
      <translation>Испорченный базовый элемент</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="120"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation type="unfinished">A body cannot be based on a Part Design feature.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation type="unfinished">%1 already belongs to a body and cannot be used as a base feature for another body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="138"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>Базовый элемент (%1) принадлежит другой детали.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="162"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит из нескольких твердых тел.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="166"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит из нескольких оболочек.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="170"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит только из оболочки.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="174"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>Выбранная форма состоит из нескольких твердых тел или оболочек.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="179"/>
      <source>Base feature</source>
      <translation>Базовый элемент</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="187"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Тело может быть основано не более, чем на одном элементе.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="201"/>
      <source>Body</source>
      <translation>Тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="351"/>
      <source>Nothing to migrate</source>
      <translation>Нечему мигрировать</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="564"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation type="unfinished">Select exactly one Part Design feature or a body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="568"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation type="unfinished">Could not determine a body for the selected feature '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="718"/>
      <source>Only features of a single source body can be moved</source>
      <translation type="unfinished">Only features of a single source body can be moved</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="500"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Плоскость эскиза не может быть перенесена</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="352"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation type="unfinished">No Part Design features without body found Nothing to migrate.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="501"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Пожалуйста, отредактируйте '%1' и переопределите его, чтобы использовать базовую или опорную плоскость в качестве плоскости эскиза.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="563"/>
      <location filename="../../CommandBody.cpp" line="567"/>
      <location filename="../../CommandBody.cpp" line="572"/>
      <location filename="../../CommandBody.cpp" line="869"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <source>Selection error</source>
      <translation>Ошибка выбора</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="573"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Только твердый элемент может быть кончиком тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="695"/>
      <location filename="../../CommandBody.cpp" line="717"/>
      <location filename="../../CommandBody.cpp" line="732"/>
      <source>Features cannot be moved</source>
      <translation>Элементы не могут быть перемещены</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="696"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Некоторые из выбранных элементов имеют зависимости в исходном теле</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="733"/>
      <source>There are no other bodies to move to</source>
      <translation>Других тел нет</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="870"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Невозможно переместить базовые элементы тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Select one or more features from the same body.</source>
      <translation>Выберите один или несколько элементов одного тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="890"/>
      <source>Beginning of the body</source>
      <translation>Начало тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="959"/>
      <source>Dependency violation</source>
      <translation>Нарушение зависимостей</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="960"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Ранний элемент не может зависеть от более позднего.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="263"/>
      <source>No previous feature found</source>
      <translation>Предыдущий элемент не найден</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Невозможно создать субтрактивный элемент без базового элемента</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="226"/>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="227"/>
      <location filename="../../TaskTransformedParameters.cpp" line="441"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="229"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="80"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="187"/>
      <source>Active Body Required</source>
      <translation>Требуется активное тело</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="140"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation type="unfinished">To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="188"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation type="unfinished">To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="222"/>
      <source>Feature is not in a body</source>
      <translation>Элемент вне тела</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="223"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Чтобы использовать этот элемент, он должен принадлежать объекту тело в документе.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="255"/>
      <source>Feature is not in a part</source>
      <translation>Элемент вне детали</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="256"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Для того, чтобы использовать этот элемент, он должен являться объектом типа деталь в документе.</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="96"/>
      <location filename="../../ViewProviderDressUp.cpp" line="65"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="210"/>
      <location filename="../../ViewProviderTransformed.cpp" line="66"/>
      <source>Edit %1</source>
      <translation>Редактировать %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="109"/>
      <source>Set Face Colors</source>
      <translation type="unfinished">Set Face Colors</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="208"/>
      <source>Plane</source>
      <translation>Плоскость</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="204"/>
      <source>Line</source>
      <translation>Линия</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Point</source>
      <translation>Точка</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>Система Координат</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="229"/>
      <source>Edit Datum</source>
      <translation type="unfinished">Edit Datum</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="89"/>
      <source>Feature error</source>
      <translation>Ошибка элемента</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="90"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation type="unfinished">%1 misses a base feature.
This feature is broken and cannot be edited.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="205"/>
      <source>Edit Shape Binder</source>
      <translation type="unfinished">Edit Shape Binder</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="317"/>
      <source>Synchronize</source>
      <translation>Синхронизировать</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="319"/>
      <source>Select Bound Object</source>
      <translation type="unfinished">Select Bound Object</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="140"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation type="unfinished">The document "%1" you are editing was designed with an old version of Part Design workbench.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="143"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation type="unfinished">Migrate in order to use modern Part Design features?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="146"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation type="unfinished">The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Make the migration automatically?</source>
      <translation type="unfinished">Make the migration automatically?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="152"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Примечание. Если вы решите выполнить миграцию, вы не сможете редактировать файл в устаревшей версии FreeCAD. Если вы откажетесь мигрировать, вы не сможете использовать новые функции рабочего окружения "разработка детали", такие как Тело и Деталь. В результате вы также не сможете использовать свои детали в рабочем окружении Сборка. В любом случае вы сможете мигрировать позже, выполнив 'разработка детали-&gt;Миграция'.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate Manually</source>
      <translation type="unfinished">Migrate Manually</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="72"/>
      <source>Edit Boolean</source>
      <translation type="unfinished">Edit Boolean</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="41"/>
      <source>Edit Chamfer</source>
      <translation type="unfinished">Edit Chamfer</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="42"/>
      <source>Edit Draft</source>
      <translation type="unfinished">Edit Draft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="41"/>
      <source>Edit Fillet</source>
      <translation type="unfinished">Edit Fillet</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="46"/>
      <source>Edit Groove</source>
      <translation type="unfinished">Edit Groove</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="51"/>
      <source>Edit Helix</source>
      <translation type="unfinished">Edit Helix</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="66"/>
      <source>Edit Hole</source>
      <translation type="unfinished">Edit Hole</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="39"/>
      <source>Edit Linear Pattern</source>
      <translation type="unfinished">Edit Linear Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="66"/>
      <source>Edit Loft</source>
      <translation type="unfinished">Edit Loft</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="39"/>
      <source>Edit Mirror</source>
      <translation>Изменить зеркалирование</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="48"/>
      <source>Edit Multi-Transform</source>
      <translation type="unfinished">Edit Multi-Transform</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="46"/>
      <source>Edit Pad</source>
      <translation type="unfinished">Edit Pad</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="74"/>
      <source>Edit Pipe</source>
      <translation type="unfinished">Edit Pipe</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="48"/>
      <source>Edit Pocket</source>
      <translation type="unfinished">Edit Pocket</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation type="unfinished">Edit Polar Pattern</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="51"/>
      <source>Edit Primitive</source>
      <translation type="unfinished">Edit Primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="46"/>
      <source>Edit Revolution</source>
      <translation type="unfinished">Edit Revolution</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="39"/>
      <source>Edit Scale</source>
      <translation type="unfinished">Edit Scale</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="41"/>
      <source>Edit Thickness</source>
      <translation type="unfinished">Edit Thickness</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation type="unfinished">Sprocket Parameters</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation type="unfinished">Number of teeth</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation type="unfinished">Sprocket reference</translation>
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
      <translation type="unfinished">Bicycle with derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation type="unfinished">Bicycle without derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation type="unfinished">Chain pitch</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation type="unfinished">Chain roller diameter</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation type="unfinished">Tooth width</translation>
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
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Текущее обновление изменений в теме (thread)
Обратите внимание, что расчет может занять некоторое время</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>Глубина резьбы </translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>Указать внутреннее отклонение диаметра резьбы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>Отклонение</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation type="unfinished">Head type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation type="unfinished">Depth type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation type="unfinished">Head diameter</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation type="unfinished">Head depth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation type="unfinished">Clearance / Passthrough</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation type="unfinished">Tap drill (to be threaded)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation type="unfinished">Modeled thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation type="unfinished">Hole type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>Обновления вида резьбы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>Настраиваемые отклонения</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Thread clearance value</source>
      <translation>Значение внутреннего отклонения диаметра резьбы в мм</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Direction</source>
      <translation>Направление</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Отверстие под резьбу
Доступно только для отверстий без резьбы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Standard</source>
      <translation>Стандартно</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="732"/>
      <source>Close</source>
      <translation>Закрыть</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="737"/>
      <source>Wide</source>
      <translation>Wide (макс. допуск)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Class</source>
      <translation>Поле допуска</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="835"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Класс допуска для резьбовых отверстий в соответствии профилю отверстия</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>Диаметр</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>Диаметр отверстия</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>Глубина</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation type="unfinished">Hole Parameters</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>Базовые типы профилей</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>Круги и дуги</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>Точки, круги и дуги</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>Точки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>Насквозь</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>Пользовательские значения головки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Угол сверления</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Включить в глубину</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>Изменить направление</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="662"/>
      <source>&lt;b&gt;Threading&lt;/b&gt;</source>
      <translation type="unfinished">&lt;b&gt;Threading&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="783"/>
      <source>Thread</source>
      <translation>Резьба</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="892"/>
      <source>&amp;Right hand</source>
      <translation>&amp;Правая</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="908"/>
      <source>&amp;Left hand</source>
      <translation>&amp;Левая</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="957"/>
      <source>Thread Depth Type</source>
      <translation>Тип глубины резьбы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="971"/>
      <source>Hole depth</source>
      <translation>На глубину дна</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="981"/>
      <source>Tapped (DIN76)</source>
      <translation>С резьбой (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>Вид гнезда под шляпку</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Установите флажок, чтобы переопределить значения, определенные 'Типом'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>Для зенкеров это глубина
верхняя часть винта ниже поверхности</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>Угол зенковки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>Размер точки сверла будет учитываться
для глубины слепых отверстий</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>Сужение</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Угол уклона отверстия
90 градусов: прямое отверстие
меньше 90: радиус отверстия уменьшается
больше 90: радиус отверстия увеличивается</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>Развернуть направление отверстия</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Сообщений нет</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Эскиз</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Проектная Деталь</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>С величиной</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation type="unfinished">Additive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation type="unfinished">Subtractive Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation type="unfinished">Dress-Up Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation type="unfinished">Transformation Features</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation type="unfinished">Sprocket…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Measure</source>
      <translation>Измерения</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Refresh</source>
      <translation>Обновить</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Toggle 3D</source>
      <translation>Переключить 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Helper</source>
      <translation>Помощник по разработке детали</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Modeling</source>
      <translation>Моделирование при разработке детали</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Length [mm]</source>
      <translation>Длина [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Diameter [mm]</source>
      <translation>Диаметр [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Inner diameter [mm]</source>
      <translation>Внутренний диаметр [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Constraint type</source>
      <translation>Тип ограничения</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Start edge type</source>
      <translation>Тип начального ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge size</source>
      <translation>Размер начального ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>End edge type</source>
      <translation>Тип конечного ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>Размер конечного ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="64"/>
      <source>Shaft Wizard</source>
      <translation type="unfinished">Shaft Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 1</source>
      <translation>Раздел 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 2</source>
      <translation>Раздел 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Add column</source>
      <translation>Добавить столбец</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation>Раздел %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="150"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="165"/>
      <source>None</source>
      <translation>Ничего</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="151"/>
      <source>Fixed</source>
      <translation>Исправлено</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="152"/>
      <source>Force</source>
      <translation>Сила</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="153"/>
      <source>Bearing</source>
      <translation>Несущий</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="154"/>
      <source>Gear</source>
      <translation>Механизм</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="155"/>
      <source>Pulley</source>
      <translation>Шкив</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="166"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="167"/>
      <source>Fillet</source>
      <translation>Скругление</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="57"/>
      <source>All</source>
      <translation>Все</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="104"/>
      <source>Missing Module</source>
      <translation type="unfinished">Missing Module</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation type="unfinished">The Plot add-on is not installed. Install it to enable this feature.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="214"/>
      <source>Shaft design wizard...</source>
      <translation>Мастер проектирования вала...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="215"/>
      <source>Start the shaft design wizard</source>
      <translation>Запускает мастер проектирования вала</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="396"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>Связанный объект не является функцией создания деталей(PartDesign)</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Tip shape is empty</source>
      <translation>Форма подсказки пуста</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="64"/>
      <source>BaseFeature link is not set</source>
      <translation>Ссылка на базовый элемент не установлена</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="69"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>Базовый элемент должен быть Part::Feature (Деталь:Элемент)</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="75"/>
      <source>BaseFeature has an empty shape</source>
      <translation>Базовый элемент имеет пустую форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="78"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Невозможно выполнить логический разрез без базового элемента</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="121"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Не может делать логическую операцию ни с чем, кроме Part::Feature(элемент детали) и его производных</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="99"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Невозможно выполнить логическую операцию с недопустимой базовой формой</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="105"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation>Невозможно сделать логическую операцию для функции, которая не является телом</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="131"/>
      <source>Base shape is null</source>
      <translation>Базовая форма пустая</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="163"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="174"/>
      <location filename="../../../App/FeatureDraft.cpp" line="291"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="576"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="589"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="599"/>
      <location filename="../../../App/FeatureFillet.cpp" line="119"/>
      <location filename="../../../App/FeatureGroove.cpp" line="196"/>
      <location filename="../../../App/FeatureHole.cpp" line="2185"/>
      <location filename="../../../App/FeatureLoft.cpp" line="277"/>
      <location filename="../../../App/FeatureLoft.cpp" line="312"/>
      <location filename="../../../App/FeaturePipe.cpp" line="404"/>
      <location filename="../../../App/FeaturePipe.cpp" line="425"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="233"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation type="unfinished">Result has multiple solids: enable 'Allow Compound' in the active body.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="112"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="134"/>
      <source>Tool shape is null</source>
      <translation>Форма инструмента пустая</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="150"/>
      <source>Unsupported boolean operation</source>
      <translation>Неподдерживаемая булева операция</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="327"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pad with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="332"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation type="unfinished">Cannot create a pocket with a total length of zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="514"/>
      <source>No extrusion geometry was generated.</source>
      <translation type="unfinished">No extrusion geometry was generated.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="534"/>
      <source>Resulting fused extrusion is null.</source>
      <translation type="unfinished">Resulting fused extrusion is null.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="568"/>
      <location filename="../../../App/FeatureLoft.cpp" line="306"/>
      <location filename="../../../App/FeaturePipe.cpp" line="401"/>
      <location filename="../../../App/FeaturePipe.cpp" line="422"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="130"/>
      <source>Resulting shape is not a solid</source>
      <translation>Результат не является твердотельным</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="157"/>
      <source>Failed to create chamfer</source>
      <translation>Не удалось создать фаску</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="287"/>
      <location filename="../../../App/FeatureFillet.cpp" line="102"/>
      <source>Resulting shape is null</source>
      <translation>Результирующая форма нулевая</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="128"/>
      <source>No edges specified</source>
      <translation>Ребра не указаны</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="241"/>
      <source>Size must be greater than zero</source>
      <translation>Размер должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="250"/>
      <source>Size2 must be greater than zero</source>
      <translation>Размер2 должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="255"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>Угол должен быть больше 0.0 и меньше 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="85"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>Скругление невозможно на выбранных фигурах</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="92"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Радиус скругления должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="89"/>
      <source>Angle of groove too large</source>
      <translation>Слишком большой угол канавки</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="93"/>
      <source>Angle of groove too small</source>
      <translation>Угол канавки слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="112"/>
      <location filename="../../../App/FeatureHole.cpp" line="1904"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>Запрошенный элемент не может быть создан. Причина может состоять в том, что:
  - активное тело не содержит базовой формы, поэтому нет
  материала для удаления;
  - выбранный эскиз не принадлежит активному телу.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="373"/>
      <source>Failed to obtain profile shape</source>
      <translation>Не удалось получить форму профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="425"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Создание не удалось, поскольку направление ортогонально вектору нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="447"/>
      <location filename="../../../App/FeatureGroove.cpp" line="129"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Creating a face from sketch failed</source>
      <translation>Не удалось создать грань из эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="151"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="155"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>Ось вращения пересекает эскиз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="159"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="240"/>
      <source>Could not revolve the sketch!</source>
      <translation>Не удалось повернуть эскиз!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="205"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="252"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Не удалось создать грань из эскиза.
Пересекающиеся объекты эскиза в эскизе не допускаются.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="137"/>
      <source>Error: Pitch too small!</source>
      <translation>Ошибка: Шаг слишком мал!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="139"/>
      <location filename="../../../App/FeatureHelix.cpp" line="153"/>
      <source>Error: height too small!</source>
      <translation>Ошибка: высота слишком мала!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="145"/>
      <source>Error: pitch too small!</source>
      <translation>Ошибка: шаг слишком мал!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="147"/>
      <location filename="../../../App/FeatureHelix.cpp" line="155"/>
      <location filename="../../../App/FeatureHelix.cpp" line="161"/>
      <source>Error: turns too small!</source>
      <translation>Ошибка: число оборотов мало!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Ошибка: ни высота, ни нарастание не должны быть равны нулю!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="179"/>
      <source>Error: unsupported mode</source>
      <translation>Ошибка: неподдерживаемый режим</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="191"/>
      <source>Error: No valid sketch or face</source>
      <translation>Ошибка: нет допустимого эскиза или грани</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="200"/>
      <source>Error: Face must be planar</source>
      <translation>Ошибка: грань должна быть плоской</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="299"/>
      <location filename="../../../App/FeatureHelix.cpp" line="331"/>
      <location filename="../../../App/FeatureHole.cpp" line="2529"/>
      <source>Error: Result is not a solid</source>
      <translation>Ошибка: Результат не твердотельный</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="275"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Ошибка: Нечего вычитать</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="279"/>
      <location filename="../../../App/FeatureHelix.cpp" line="303"/>
      <location filename="../../../App/FeatureHelix.cpp" line="334"/>
      <source>Error: Result has multiple solids</source>
      <translation>Ошибка: Результат имеет несколько твердых частей</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="292"/>
      <source>Error: Adding the helix failed</source>
      <translation>Ошибка: добавление спирали не удалось</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="318"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>Ошибка: пересечение спирали не удалось</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="325"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>Ошибка: не удалось вычесть спираль</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="348"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Ошибка: Не удалось создать грань из эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1419"/>
      <source>Thread type is invalid</source>
      <translation>Недопустимый тип резьбы</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1944"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Ошибка отверстия: неподдерживаемая длина</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1947"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Ошибка отверстия: неверная глубина отверстия</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1970"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Ошибка при создании отверстия: недопустимый угол зенкования</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1991"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Ошибка при создании отверстия: димаметр цековки слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1995"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Ошибка при создании отверстия: глубина цековки должна быть меньше глубины отверстия</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1999"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Ошибка при создании отверстия: глубина цековки должна быть больше либо равна нулю</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2021"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Ошибка при создании отверстия: невозможная зенковка</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Ошибка при создании отверстия: невозможный угол сверла</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2064"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Ошибка при создании отверстия: недопустимая режущая часть сверла</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2098"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Ошибка при создании отверстия: не найден эскиз профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2102"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Ошибка при создании отверстия: результирующая форма пуста</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2112"/>
      <source>Error: Adding the thread failed</source>
      <translation>Ошибка: не удалось добавить резьбу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2163"/>
      <location filename="../../../App/FeatureHole.cpp" line="2168"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Булева операция не удалась на краю профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2174"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>Булева операция, произведенная не на профильной грани</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="156"/>
      <source>Boolean operation failed</source>
      <translation>Не удалось выполнить булеву операцию</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2195"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Не удалось создать грань из эскиза.
Создание выреза из эскиза с пересекающимися элементами или множественными контуры в эскизе невозможно.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2345"/>
      <source>Thread type out of range</source>
      <translation>Тип резьбы выходит за пределы допустимого диапазона</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2348"/>
      <source>Thread size out of range</source>
      <translation>Размер резьбы выходит за пределы допустимого диапазона</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2504"/>
      <source>Error: Thread could not be built</source>
      <translation>Ошибка: резьба не может быть построена</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="170"/>
      <source>Loft: At least one section is needed</source>
      <translation>Операция по сечениям: Требуется хотя бы одно сечение</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="325"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Операция по сечениям: Создание привело к фатальной ошибке</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="207"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Лофт: не удалось создать грань по эскизу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="256"/>
      <source>Loft: Failed to create shell</source>
      <translation>Лофт: не удалось создать оболочку</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="611"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Не удалось создать грань из эскиза.
Эскизы с пересекающимися элементами или множественными контурами не допускаются.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="178"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Профиль по траектории: Не удалось получить форму профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="183"/>
      <source>No spine linked</source>
      <translation>Нет привязки к направляющей</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="196"/>
      <source>No auxiliary spine linked.</source>
      <translation>Нет привязки к вспомогательным направляющим.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="217"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Профиль по траектории: при использовании точки в качестве сечения эскиз должен содержать только одну точку</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="223"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Профиль по траектории: при использовании точки в качестве профиля требуется как минимум одна секция</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="237"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation type="unfinished">Pipe: All sections need to be Part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="243"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Профиль по траектории: не удалось получить форму сечения</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="252"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Профиль по траектории: вершинами могут быть только профиль и последняя секция</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="261"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Мультисекции должны иметь такое же количество внутренних проводов каркаса, как и базовая секция</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="288"/>
      <source>Path must not be a null shape</source>
      <translation>Путь обработки не должен иметь нулевую форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="323"/>
      <source>Pipe could not be built</source>
      <translation>Труба не может быть построена</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="368"/>
      <source>Result is not a solid</source>
      <translation>Результат не является твердым телом</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="383"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Профиль по траектории: тут нечего вычитать</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="396"/>
      <source>Adding the pipe failed</source>
      <translation>Не удалось добавить трубу</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="417"/>
      <source>Subtracting the pipe failed</source>
      <translation>Вычитание трубы не удалось</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="442"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>Произошла фатальная ошибка при создании трубы</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="565"/>
      <source>Invalid element in spine.</source>
      <translation>Недопустимый элемент в каркасе.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="568"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Элемент в каркасе не является ни ребром, ни проволокой.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="581"/>
      <source>Spine is not connected.</source>
      <translation>Каркас не подключен.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="585"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Каркас не является ни гранью, ни проволокой.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="589"/>
      <source>Invalid spine.</source>
      <translation>Неверный каркас.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="98"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Невозможно вычесть примитивный элемент без базового элемента</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="295"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="116"/>
      <source>Unknown operation type</source>
      <translation>Неизвестный тип операции</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Failed to perform boolean operation</source>
      <translation>Не удалось выполнить логическую операцию</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="204"/>
      <source>Length of box too small</source>
      <translation>Размер габаритов слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="206"/>
      <source>Width of box too small</source>
      <translation>Ширина габаритов слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="208"/>
      <source>Height of box too small</source>
      <translation>Высота габаритов слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="254"/>
      <source>Radius of cylinder too small</source>
      <translation>Радиус цилиндра мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="256"/>
      <source>Height of cylinder too small</source>
      <translation>Высота цилиндра мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="258"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Угол поворота цилиндра слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="311"/>
      <source>Radius of sphere too small</source>
      <translation>Радиус сферы слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="360"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="362"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Радиус конуса не может быть отрицательным</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="364"/>
      <source>Height of cone too small</source>
      <translation>Высота конуса слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="427"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="429"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Радиус эллипсоида слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="511"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="513"/>
      <source>Radius of torus too small</source>
      <translation>Радиус торса слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="576"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Многоугольник призмы неправилен, должен иметь 3 или более сторон</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="578"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Радиус окружности многоугольника призмы слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="580"/>
      <source>Height of prism is too small</source>
      <translation>Высота призмы слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="661"/>
      <source>delta x of wedge too small</source>
      <translation>дельта x клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="664"/>
      <source>delta y of wedge too small</source>
      <translation>дельта Y клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="667"/>
      <source>delta z of wedge too small</source>
      <translation>дельта z клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="670"/>
      <source>delta z2 of wedge is negative</source>
      <translation>дельта z2 клина отрицательно</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="673"/>
      <source>delta x2 of wedge is negative</source>
      <translation>дельта x2 клина отрицательно</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="92"/>
      <source>Angle of revolution too large</source>
      <translation>Слишком большой угол поворота</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="98"/>
      <source>Angle of revolution too small</source>
      <translation>Угол поворота слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Reference axis is invalid</source>
      <translation>Базовая ось недействительна</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="561"/>
      <source>Fusion with base feature failed</source>
      <translation>Сбой слияния с базовой функцией</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="103"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Функция преобразования Связанный объект не является объектом детали</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="108"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>Нет оригиналов, связанных с преобразованным объектом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="326"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Невозможно преобразовать недопустимую форму фигуры</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="375"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>Не выбрана часть для вычитания/слияния</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="367"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Только добавляемые и вычитаемые признаки могут быть преобразованы</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="103"/>
      <source>Invalid face reference</source>
      <translation>Неверная ссылка на сторону</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute Gear</source>
      <translation type="unfinished">Involute Gear</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edits the involute gear definition</source>
      <translation type="unfinished">Creates or edits the involute gear definition</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket</source>
      <translation type="unfinished">Sprocket</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation type="unfinished">Creates or edits the sprocket definition.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation type="unfinished">Show final result</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation type="unfinished">Show preview overlay</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="50"/>
      <source>Preview</source>
      <translation>Предварительный просмотр</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft Design Wizard</source>
      <translation type="unfinished">Shaft Design Wizard</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Starts the shaft design wizard</source>
      <translation type="unfinished">Starts the shaft design wizard</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation type="unfinished">Failure while computing removed volume preview: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="103"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation type="unfinished">Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2372"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2373"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2407"/>
      <source>Create Datum</source>
      <translation type="unfinished">Create Datum</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2408"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation type="unfinished">Creates a datum object or local coordinate system</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="198"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation type="unfinished">Creates an additive box by its width, height, and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="202"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation type="unfinished">Creates an additive cylinder by its radius, height, and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="206"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates an additive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="210"/>
      <source>Creates an additive cone</source>
      <translation type="unfinished">Creates an additive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="214"/>
      <source>Creates an additive ellipsoid</source>
      <translation type="unfinished">Creates an additive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="218"/>
      <source>Creates an additive torus</source>
      <translation type="unfinished">Creates an additive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Creates an additive prism</source>
      <translation type="unfinished">Creates an additive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive wedge</source>
      <translation type="unfinished">Creates an additive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="350"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation type="unfinished">Creates a subtractive box by its width, height and length</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="354"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation type="unfinished">Creates a subtractive cylinder by its radius, height and angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="358"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation type="unfinished">Creates a subtractive sphere by its radius and various angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="362"/>
      <source>Creates a subtractive cone</source>
      <translation type="unfinished">Creates a subtractive cone</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="366"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation type="unfinished">Creates a subtractive ellipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="370"/>
      <source>Creates a subtractive torus</source>
      <translation type="unfinished">Creates a subtractive torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="374"/>
      <source>Creates a subtractive prism</source>
      <translation type="unfinished">Creates a subtractive prism</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="378"/>
      <source>Creates a subtractive wedge</source>
      <translation type="unfinished">Creates a subtractive wedge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="986"/>
      <source>Attachment</source>
      <translation>Вложение</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="828"/>
      <source>Revolution Parameters</source>
      <translation type="unfinished">Revolution Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="836"/>
      <source>Groove Parameters</source>
      <translation type="unfinished">Groove Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation type="unfinished">Transformed Feature Messages</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="121"/>
      <source>Active Body</source>
      <translation type="unfinished">Active Body</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer Parameters</source>
      <translation>Параметры Фаски</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation type="unfinished">Datum Plane Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation type="unfinished">Datum Line Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation type="unfinished">Datum Point Parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation type="unfinished">Local Coordinate System Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft Parameters</source>
      <translation type="unfinished">Draft Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet Parameters</source>
      <translation type="unfinished">Fillet Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>Linear Pattern Parameters</source>
      <translation type="unfinished">Linear Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirror Parameters</source>
      <translation type="unfinished">Mirror Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>Multi-Transform Parameters</source>
      <translation type="unfinished">Multi-Transform Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>Polar Pattern Parameters</source>
      <translation type="unfinished">Polar Pattern Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scale Parameters</source>
      <translation type="unfinished">Scale Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness Parameters</source>
      <translation type="unfinished">Thickness Parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="116"/>
      <source>Direction 2</source>
      <translation type="unfinished">Direction 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="219"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation type="unfinished">Select a direction reference (edge, face, datum line)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="299"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation type="unfinished">Invalid selection. Select an edge, planar face, or datum line.</translation>
    </message>
  </context>
</TS>
