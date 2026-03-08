<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="80"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Центральная точка начала спирали; берется из опорной оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Направление спирали; берется из опорной оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>The reference axis of the helix.</source>
      <translation>Опорная ось спирали.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Режим ввода спирали определяет, какие свойства задаются пользователем.
Затем рассчитываются зависимые свойства.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="116"/>
      <source>The axial distance between two turns.</source>
      <translation>Осевое расстояние между двумя оборотами.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="123"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Высота траектории спирали, не учитывающая протяженность профиля.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="133"/>
      <source>The number of turns in the helix.</source>
      <translation>Количество витков в спирали.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="141"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>Угол конуса, который образует оболочку вокруг спирали.
Ненулевые значения превращают спираль в коническую спираль.
При положительных значениях радиус увеличивается, а при отрицательных - уменьшается. </translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="154"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Увеличение радиуса спирали за один оборот.
Ненулевые значения превращают спираль в коническую спираль.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Устанавливает направление поворота на левостороннее,
то есть против часовой стрелки при движении вдоль своей оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="176"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Определяет, указывает ли спираль в направлении, противоположном оси.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="186"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Если установлено, результатом будет пересечение профиля и ранее существовавшего тела.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="196"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Если false, инструмент предложит начальное значение шага на основе ограничивающей рамки профиля,
чтобы избежать самопересечения.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="208"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Допуск на слияние для спирали, увеличьте, если спиральная форма не обеспечивает хорошего слияния с деталью.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="106"/>
      <source>Number of gear teeth</source>
      <translation>Число зубов передачи</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="118"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Угол давления зубьев передачи</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="112"/>
      <source>Module of the gear</source>
      <translation>Модуль шестерни</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=2 кривые с каждым 3-мя контрольными точками, False=1 кривая с четырьмя контрольными точками.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="135"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=внешняя шестерня, False=внутренняя шестерня</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="144"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>Высота зуба от делительной окружности до его вершины, нормированная по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="153"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>Высота зуба от делительной окружности до его корня, нормированная по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="162"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Радиус галтели у корня зуба, нормированный по модулю.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="171"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>Расстояние, на которое эталонный профиль сдвинут наружу, нормализуется модулем.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1661"/>
      <source>Additive Helix</source>
      <translation>Аддитивная спираль</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1662"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Перемещает выбранный эскиз или профиль по винтовой линии и добавляет его к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1561"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1562"/>
      <source>Additive Loft</source>
      <translation>Выдавить по сечениям</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1563"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>Сращивает выбранные эскизы между собой и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1461"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1462"/>
      <source>Additive Pipe</source>
      <translation>Выдавить по траектории</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1463"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Перемещает выбранный эскиз или профиль вдоль траектории и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>New Body</source>
      <translation>Новое тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="92"/>
      <source>Creates a new body and activates it</source>
      <translation>Создаёт новое тело и делает его активным</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2576"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2577"/>
      <source>Boolean Operation</source>
      <translation>Булева операция</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2578"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>Применяет логические (булевы) операции с выбранными объектами и активным телом</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Local Coordinate System</source>
      <translation>Локальная система координат </translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new local coordinate system</source>
      <translation>Создаёт новую локальную систему координат</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1987"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1988"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1989"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>Формирует фаски на выбранных рёбрах или гранях</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="489"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="490"/>
      <source>Clone</source>
      <translation>Клонировать</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="491"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>Параметрически копирует твёрдотельный объект в качестве базового элемента нового тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Draft</source>
      <translation>Уклон</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2018"/>
      <source>Applies a draft to the selected faces</source>
      <translation>Создаёт уклон на выбранных гранях</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>Duplicate &amp;Object</source>
      <translation>Дублировать объект</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Дублирует выбранный объект и добавляет его в активное тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1959"/>
      <source>PartDesign</source>
      <translation>Деталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1960"/>
      <source>Fillet</source>
      <translation>Скругление</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1961"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>Скругляет выбранные рёбра или грани</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1391"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1392"/>
      <source>Groove</source>
      <translation>Проточка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1393"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>Поворачивает эскиз или профиль вокруг линии или оси и вырезает результат из тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1285"/>
      <source>Hole</source>
      <translation>Отверстие</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1287"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>Создаёт отверстия в активном теле в центральных точках окружностей или дуг выбранного эскиза или профиля</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Line</source>
      <translation>Опорная линия</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum line</source>
      <translation>Создаёт новую опорную линию</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2271"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2272"/>
      <source>Linear Pattern</source>
      <translation>Линейный шаблон</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2273"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>Дублирует выбранные объекты или активное тело по линейному шаблону</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="385"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="386"/>
      <source>Migrate</source>
      <translation>Миграция</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="387"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>Переносит документ в современный рабочий процесс Проектирования Детали</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2214"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2215"/>
      <source>Mirror</source>
      <translation>Зеркально</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2216"/>
      <source>Mirrors the selected features or active body</source>
      <translation>Отражает зеркально выбранные операции с телом или само активное тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="821"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="822"/>
      <source>Move Object To…</source>
      <translation>Переместить объект в…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="823"/>
      <source>Moves the selected object to another body</source>
      <translation>Перемещает выделенный объект в другое тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1016"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1017"/>
      <source>Move Feature After…</source>
      <translation>Переместить операцию после…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1018"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>Перемещает выбранную операцию после другой операцию в том же теле</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Set Tip</source>
      <translation>Установить точку завершения расчёта тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>Перемещает точку завершения расчёта тела на выбранную операцию</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2445"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2446"/>
      <source>Multi-Transform</source>
      <translation>Множественное преобразование</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2447"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>Одновременно применяет несколько преобразований к выбранным операциям с телом или самим активным телом</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="573"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="574"/>
      <source>New Sketch</source>
      <translation>Создать эскиз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="575"/>
      <source>Creates a new sketch</source>
      <translation>Создаёт новый эскиз</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1226"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>Pad</source>
      <translation>Выдавливание</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>Выдавливает выбранный эскиз или профиль и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Datum Plane</source>
      <translation>Опорная плоскость</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Creates a new datum plane</source>
      <translation>Создаёт новую опорную плоскость</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1256"/>
      <source>Pocket</source>
      <translation>Вырезание</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1257"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>Выдавливает выбранный эскиз или профиль и вырезает результат из тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="250"/>
      <source>Datum Point</source>
      <translation>Опорная точка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="251"/>
      <source>Creates a new datum point</source>
      <translation>Создаёт новую опорную точку</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2340"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2341"/>
      <source>Polar Pattern</source>
      <translation>Круговой шаблон</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2342"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>Дублирует выбранные объекты или активное тело по круговому шаблону</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1329"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1330"/>
      <source>Revolve</source>
      <translation>Вращение</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1331"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>Проворачивает выбранный эскиз или профиль вокруг линии или оси и добавляет результат к телу</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2403"/>
      <source>Scale</source>
      <translation>Масштабировать</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2404"/>
      <source>Scales the selected features or the active body</source>
      <translation>Масштабирует выбранные операции с телом или само активное тело</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="313"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="314"/>
      <source>Shape Binder</source>
      <translation>Связующая форма</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="315"/>
      <source>Creates a new shape binder</source>
      <translation>Создаёт новую связующую форму</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="383"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="384"/>
      <source>Sub-Shape Binder</source>
      <translation>Подобъектная связующая форма</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="385"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>Создаёт геометрически привязанную форму к одному или нескольким объектам, позволяя использовать эту форму внутри или вне тела. Отслеживает относительное расположение, поддерживает несколько типов геометрии (тела, грани, рёбра, вершины) и может работать с объектами в том же или внешних документах.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1744"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>Subtractive Helix</source>
      <translation>Вырезать спираль</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>Перемещает выбранный эскиз или профиль по спирали и вырезает результат из тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1611"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>Subtractive Loft</source>
      <translation>Вырезать по сечениям</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>Сращивает выбранные эскизы между собой и вырезает результат из тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1511"/>
      <source>PartDesign</source>
      <translation>Проектирование детали</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1512"/>
      <source>Subtractive Pipe</source>
      <translation>Вырезать по траектории</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1513"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>Перемещает выбранный эскиз или профиль вдоль траектории и вырезает результат из тела</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2087"/>
      <source>Thickness</source>
      <translation>Оболочка</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2088"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>Придаёт толщину стенкам и удаляет выбранные грани</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="74"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="75"/>
      <source>Additive Primitive</source>
      <translation>Аддитивный Примитив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>Creates an additive primitive</source>
      <translation>Создать аддитивный примитив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Box</source>
      <translation>Выдавить Блок</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Additive Cylinder</source>
      <translation>Выдавить Цилиндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="231"/>
      <source>Additive Sphere</source>
      <translation>Выдавить Сферу</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="240"/>
      <source>Additive Cone</source>
      <translation>Выдавить Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="246"/>
      <source>Additive Ellipsoid</source>
      <translation>Выдавить Эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="252"/>
      <source>Additive Torus</source>
      <translation>Выдавить Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="258"/>
      <source>Additive Prism</source>
      <translation>Выдавить Призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>Additive Wedge</source>
      <translation>Выдавить Клин</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="282"/>
      <source>PartDesign</source>
      <translation>ПроектнаяДеталь</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="283"/>
      <source>Subtractive Primitive</source>
      <translation>Субтрактивный Примитив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>Creates a subtractive primitive</source>
      <translation>Создать субтрактивный примитив</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="398"/>
      <source>Subtractive Box</source>
      <translation>Вырезать Блок</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="407"/>
      <source>Subtractive Cylinder</source>
      <translation>Вырезать Цилиндр</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="416"/>
      <source>Subtractive Sphere</source>
      <translation>Вырезать Сферу</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="425"/>
      <source>Subtractive Cone</source>
      <translation>Вырезать Конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="431"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Вырезать Эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="437"/>
      <source>Subtractive Torus</source>
      <translation>Вырезать Тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="443"/>
      <source>Subtractive Prism</source>
      <translation>Вырезать Призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="449"/>
      <source>Subtractive Wedge</source>
      <translation>Вырезать Клин</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="335"/>
      <source>Edit Shape Binder</source>
      <translation>Редактировать связующую форму</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Create Shape Binder</source>
      <translation>Создать связующую форму</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="439"/>
      <source>Create Sub-Shape Binder</source>
      <translation>Создать под-объектную связующую форму</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="508"/>
      <source>Create Clone</source>
      <translation>Клонировать</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1110"/>
      <source>Make Copy</source>
      <translation>Сделать копию</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2500"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>Преобразовать в элемент множественного преобразования</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="253"/>
      <source>Sketch on Face</source>
      <translation>Эскиз на грани</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="314"/>
      <source>Make copy</source>
      <translation>Сделать копию</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="516"/>
      <location filename="../../SketchWorkflow.cpp" line="772"/>
      <source>New Sketch</source>
      <translation>Создать эскиз</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2597"/>
      <source>Create Boolean</source>
      <translation>Булева операция</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="222"/>
      <location filename="../../DlgActiveBody.cpp" line="101"/>
      <source>Add a Body</source>
      <translation>Добавить тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>Перенести устаревшие элементы Проектной Детали в тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="769"/>
      <source>Duplicate a Part Design object</source>
      <translation>Дублировать объект ПроектнойДетали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1110"/>
      <source>Move a feature inside body</source>
      <translation>Переместить операцию внутрь тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="723"/>
      <source>Move tip to selected feature</source>
      <translation>Переместить точку окончания расчёта тела к выбранной операции</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="926"/>
      <source>Move an object</source>
      <translation>Переместить объект</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="258"/>
      <source>Mirror</source>
      <translation>Зеркально</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="298"/>
      <source>Linear Pattern</source>
      <translation>Линейный массив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="347"/>
      <source>Polar Pattern</source>
      <translation>Круговой массив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
      <source>Scale</source>
      <translation>Масштабировать</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Face Tools</source>
      <translation>Инструменты работы с гранями</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Edge Tools</source>
      <translation>Инструменты работы с рёбрами</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Boolean Tools</source>
      <translation>Булевы инструменты</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Helper Tools</source>
      <translation>Вспомогательные инструменты</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Modeling Tools</source>
      <translation>Инструменты моделирования</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Create Geometry</source>
      <translation>Создать геометрию</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>Параметры эвольвенты</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>Число зубьев</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>Модуль</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>Угол давления</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>Высокая точность</translation>
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
      <translation>Внешнее зацепление</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>Коэффициент Аддендум (головки зуба)</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>Коэффициент Дедендум (ножки зуба)</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>Коэффициент корневого скругления</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>Коэффициент сдвига профиля</translation>
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
      <translation>Для создания нового объекта Проектной Детали в документе должно быть активное тело.
Выберите тело из списка ниже или создайте новое тело.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>Создать новое тело</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="52"/>
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
      <translation>Параметр U</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>Параметры V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Радиус в локальном Z-направлении</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>Радиус в локальном X-направлении</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>Радиус 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>Радиус в локальном Y-направлении
Если ноль, то он равен Радиусу 2</translation>
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
      <translation>Радиус в локальной XY-плоскости</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>Радиус в локальной XZ-плоскости</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Многоугольник</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>Описанный радиус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X мин/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y мин/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z мин/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 мин/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 мин/макс</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Шаг</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>Система координат</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Прирост</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>Количество оборотов</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>Угол 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>Угол 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>По 3 точкам</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>Большой радиус</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>Малый радиус</translation>
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
      <translation>Правое</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Левое</translation>
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
      <translation>Ориентир</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Вы выбрали геометрию, которая не является частью активного тела. Укажите, как следует поступить с этими выделенными элементами. Если вы не хотите использовать эти опорные элементы, отмените команду.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Сделать независимую копию (рекомендуется)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Сделать зависимую копию</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Создать перекрёстную ссылку</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="285"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Выбор этого вызовет циклическую зависимость.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>Добавить тело</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>Удалить тело</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Объединить</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Обрезать</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Пересечь</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="51"/>
      <source>Boolean Parameters</source>
      <translation>Параметры булевой операции</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="82"/>
      <source>Remove</source>
      <translation>Удалить</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="47"/>
      <source>Primitive Parameters</source>
      <translation>Параметры примитива</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="940"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="948"/>
      <source>Invalid wedge parameters</source>
      <translation>Неверные параметры клина</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>X min must not be equal to X max!</source>
      <translation>X мин. не должен быть равен X макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="941"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y мин. не должен быть равен Y макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="949"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z мин. не должен быть равен Z макс.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="991"/>
      <source>Create primitive</source>
      <translation>Создать примитив</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Переключает между режимами выбора и предварительного просмотра</translation>
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
- дважды щёлкните по элементу, чтобы увидеть фаски</translation>
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
      <translation>Сменить направление</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>Применить для всех рёбер</translation>
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
      <location filename="../../TaskChamferParameters.cpp" line="344"/>
      <source>Empty chamfer created!
</source>
      <translation>Создана пустая фаска!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>Empty body list</source>
      <translation>Пустой список тел</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>The body list cannot be empty</source>
      <translation>Список тел не может быть пустым</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="399"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Логическое значение: Принять: Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>Incompatible Reference Set</source>
      <translation>Несовместимый набор ориентиров</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Нет режимов присоединения, которому соответствует текущий набор ориентиров. Если Вы выберете "Продолжить", элемент останется там, где он сейчас находится, и не будет перенесён при изменении базового объекта. Продолжить?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="228"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</source>
      <translation>Операция не может быть выполнена с заданными параметрами.
Геометрия может быть некорректной, или параметры могут 
быть несовместимыми.
Пожалуйста настройте параметры и повторите попытку.</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="235"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="440"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Переключает между режимами выбора и предварительного просмотра</translation>
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
- дважды щелкните по элементу, чтобы увидеть наброски</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Угол уклона</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>Нейтральная плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>Направление уклона</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Развернуть направление уклона</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="288"/>
      <source>Empty draft created!
</source>
      <translation>Создан пустой уклон!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="298"/>
      <source>Select</source>
      <translation>Выбрать</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <source>Confirm Selection</source>
      <translation>Подтвердить выбор</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="316"/>
      <source>Add All Edges</source>
      <translation>Применить для всех рёбер</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="322"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>Добавляет все рёбра в список (только в режиме добавления выделения)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="331"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1372"/>
      <source>No face selected</source>
      <translation>Нет выбранной грани</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="171"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1141"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="352"/>
      <source>Preview</source>
      <translation>Предварительный просмотр</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="356"/>
      <source>Select Faces</source>
      <translation>Выбрать грани</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="692"/>
      <source>Select reference…</source>
      <translation>Выберите ориентир…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="602"/>
      <source>No shape selected</source>
      <translation>Форма не выбрана</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="685"/>
      <source>Sketch normal</source>
      <translation>Вдоль нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="688"/>
      <source>Face normal</source>
      <translation>Нормально грани</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="696"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>Задать направление</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1088"/>
      <source>Click on a shape in the model</source>
      <translation>Нажмите на форму в модели</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1359"/>
      <source>One sided</source>
      <translation>Одно направление</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>Two sided</source>
      <translation>Два направления</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>Symmetric</source>
      <translation>Симметрично</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1367"/>
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
      <translation>Разрешить внешние элементы</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>От других тел этой же детали</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>От разных деталей или независимых операций</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Сделать независимую копию (рекомендуется)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Сделать зависимую копию</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Создать перекрёстную ссылку</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Valid</source>
      <translation>Верно</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Invalid shape</source>
      <translation>Неверная фигура</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>No wire in sketch</source>
      <translation>Нет кривых в эскизе</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Sketch already used by other feature</source>
      <translation>Эскиз уже используется другой операцией</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Belongs to another body</source>
      <translation>Принадлежит другому телу</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another part</source>
      <translation>Принадлежит другой детали</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Doesn't belong to any body</source>
      <translation>Не принадлежит ни какому телу</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Base plane</source>
      <translation>Базовая плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Feature is located after the tip of the body</source>
      <translation>Операция расположена после точки окончания расчёта тела</translation>
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
      <translation>Переключает между режимами выбора и предварительного просмотра</translation>
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
      <translation>Применить для всех рёбер</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="203"/>
      <source>Empty fillet created!</source>
      <translation>Пустое скругление создано!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Действительный</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="239"/>
      <source>Base X-axis</source>
      <translation>Базовая ось X</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="240"/>
      <source>Base Y-axis</source>
      <translation>Базовая ось Y</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base Z-axis</source>
      <translation>Базовая ось Z</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="222"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
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
      <location filename="../../TaskHelixParameters.cpp" line="206"/>
      <source>Select reference…</source>
      <translation>Выберите ориентир…</translation>
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
      <translation>Шаг-Витки-Угол</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Высота-Витки-Угол</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Высота-Витки-Приращение</translation>
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
      <translation>Витков</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>Угол конусности</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>Радиальное приращение</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>Пересчёт при изменении</translation>
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
      <location filename="../../TaskHelixParameters.cpp" line="55"/>
      <source>Helix Parameters</source>
      <translation>Параметры спирали</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="293"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Предупреждение: спираль может быть самопересекающейся</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="298"/>
      <source>Error: helix touches itself</source>
      <translation>Ошибка: спираль касается сама себя</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="347"/>
      <source>Error: unsupported mode</source>
      <translation>Ошибка: неподдерживаемый режим</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterbore</source>
      <translation>Цековка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="56"/>
      <source>Countersink</source>
      <translation>Зенковка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterdrill</source>
      <translation>Утопленная Зенковка</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Hole Parameters</source>
      <translation>Параметры отверстия</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>None</source>
      <translation>Ничего</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric regular</source>
      <translation>Метрическая стандартная ISO</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>ISO metric fine</source>
      <translation>Метрическая прецизионная ISO</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS coarse</source>
      <translation>UTS грубая</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS fine</source>
      <translation>UTS точная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS extra fine</source>
      <translation>UTS прецизионная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ANSI pipes</source>
      <translation>ANSI трубная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP трубная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSW whitworth</source>
      <translation>BSW Уитворта</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>BSF whitworth fine</source>
      <translation>BSF Уитворта с мелким шагом</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>ISO tyre valves</source>
      <translation>ISO шинные клапаны</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Среднее</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="682"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Точно</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="686"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Грубо</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="692"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Нормальное</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="696"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Закрыть</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="700"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Свободный</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="704"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Основная</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="705"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Закрыть</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="706"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Свободный/широкий</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Линейчатая поверхность</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Замкнуть поверхность</translation>
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
      <translation>Пересчёт при изменении</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="48"/>
      <source>Loft Parameters</source>
      <translation>Параметры протяжки по сечениям</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
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
      <location filename="../../TaskMirroredParameters.cpp" line="184"/>
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
      <location filename="../../TaskMultiTransformParameters.cpp" line="69"/>
      <source>Edit</source>
      <translation>Редактировать</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="72"/>
      <source>Delete</source>
      <translation>Удалить</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="75"/>
      <source>Add Mirror Transformation</source>
      <translation>Добавить зеркальное преобразование</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add Linear Pattern</source>
      <translation>Добавить линейный массив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="91"/>
      <source>Add Polar Pattern</source>
      <translation>Добавить круговой массив</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="99"/>
      <source>Add Scale Transformation</source>
      <translation>Добавить масштабирование</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Move Up</source>
      <translation>Переместить вверх</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="105"/>
      <source>Move Down</source>
      <translation>Переместить вниз</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="137"/>
      <source>Right-click to add a transformation</source>
      <translation>Щёлкните правой кнопкой мыши для добавления преобразования</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="38"/>
      <source>Pad Parameters</source>
      <translation>Параметры выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>Отступ выдавливания от поверхности, на которой оно заканчивается в направлении 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="41"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>Отступ выдавливания от поверхности, на которой оно заканчивается в направлении 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Reverses pad direction</source>
      <translation>Противоположное направление выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To last</source>
      <translation>К последнему</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>Поднять до грани</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to shape</source>
      <translation>Поднять до формы</translation>
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
      <translation>Выбрать грань</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>Сторона 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>Направление</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="541"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Задайте направление или выберите ребро модели в качестве ориентира</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>Вдоль нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>Задать направление</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Использовать произвольный вектор для направления выдавливания, в противном случае будет использоваться вектор нормали плоскости эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Если флажок не установлен, длина будет измеряться вдоль заданного направления</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>Длина вдоль нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Переключает между режимами выбора и предварительного просмотра</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>Реверсивный</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation>Направление/ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation>Выберите ориентир…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation>X-компонента вектора направления</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation>Y-компонента вектора направления</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation>Z-компонента вектора направления</translation>
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
      <translation>Сторона 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>Угол конусности</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>Выберите форму</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>Выбирает все грани формы</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation>Пересчёт при изменении</translation>
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
      <translation>Поддержка</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Бинормали</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Криволинейная эквивалентность</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="575"/>
      <source>Section Orientation</source>
      <translation>Ориентация сечения</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="603"/>
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
      <translation>Переход в углу</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>Правильный угол</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>Скруглённый угол</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>Траектория выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Добавить ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>Удалить ребро</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Преобразованный</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="67"/>
      <source>Pipe Parameters</source>
      <translation>Параметры выдавливания</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="86"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <location filename="../../TaskPipeParameters.cpp" line="561"/>
      <source>Input error</source>
      <translation>Ошибка ввода</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <source>No active body</source>
      <translation>Нет активного тела</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="870"/>
      <source>Section Transformation</source>
      <translation>Преобразование сечений</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="889"/>
      <source>Remove</source>
      <translation>Убрать</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="38"/>
      <source>Pocket Parameters</source>
      <translation>Параметры выреза</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="41"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>Отступ выдавливания от поверхности, на которой оно заканчивается в направлении 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>Отступ выдавливания от поверхности, на которой оно заканчивается в направлении 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Reverses pocket direction</source>
      <translation>Разворачивает направление выреза на противоположное</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Dimension</source>
      <translation>Размер</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Through all</source>
      <translation>Насквозь</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Up to face</source>
      <translation>Вырезать до грани</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>Up to shape</source>
      <translation>Вырезать до формы</translation>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="254"/>
      <source>Base X-axis</source>
      <translation>Базовая ось X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="255"/>
      <source>Base Y-axis</source>
      <translation>Базовая ось Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="256"/>
      <source>Base Z-axis</source>
      <translation>Базовая ось Z</translation>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="264"/>
      <source>Select reference…</source>
      <translation>Выберите ориентир…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="197"/>
      <source>Angle</source>
      <translation>Угол</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="160"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="491"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation>Пересчёт при изменении</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="199"/>
      <source>To last</source>
      <translation>К последнему</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Through all</source>
      <translation>Насквозь</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="204"/>
      <source>To first</source>
      <translation>К первому</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="215"/>
      <source>Up to face</source>
      <translation>Поднять до грани</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="216"/>
      <source>Two angles</source>
      <translation>Два угла</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="479"/>
      <source>No face selected</source>
      <translation>Нет выбранной грани</translation>
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
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Shape Binder Parameters</source>
      <translation>Параметры Связующей формы</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="137"/>
      <source>Remove</source>
      <translation>Удалить</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="193"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Переключает между режимами выбора и предварительного просмотра</translation>
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
- дважды щёлкните по элементу, чтобы редактировать выбор</translation>
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
      <translation>Оболочка</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Труба</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>Две стороны</translation>
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
      <translation>Наращивать толщину внутрь</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="267"/>
      <source>Empty thickness created!
</source>
      <translation>Создана пустая толщина!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>Удалить</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>Ось Нормали Эскиза</translation>
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
      <location filename="../../TaskTransformedParameters.cpp" line="442"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation>Базовая ось X</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation>Базовая ось Y</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation>Базовая ось Z</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base XY-plane</source>
      <translation>Базовая XY-плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base YZ-plane</source>
      <translation>Базовая YZ-плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base XZ-plane</source>
      <translation>Базовая XZ-плоскость</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="466"/>
      <source>Select reference…</source>
      <translation>Выберите ориентир…</translation>
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
      <translation>Добавить операцию</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>Удалить операцию</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation>Пересчёт при изменении</translation>
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
      <location filename="../../CommandBody.cpp" line="908"/>
      <source>Select Body</source>
      <translation>Выбрать тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="909"/>
      <source>Select a body from the list</source>
      <translation>Выберите тело из списка</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1095"/>
      <source>Move Feature After…</source>
      <translation>Переместить операцию после…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1096"/>
      <source>Select a feature from the list</source>
      <translation>Выбрать операцию из списка</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1183"/>
      <source>Move Tip</source>
      <translation>Переместить точку завершения расчётов</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>Set tip to last feature?</source>
      <translation>Установить точку завершения расчётов к последней операции?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1184"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>Перемещённая операция окажется после текущей точки завершения расчётов.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>Invalid selection</source>
      <translation>Неверный выбор</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="147"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Нет режимов присоединения, которые соответствуют выбранным объектам. Выберите что-нибудь другое.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="160"/>
      <location filename="../../Command.cpp" line="168"/>
      <location filename="../../Command.cpp" line="175"/>
      <source>Error</source>
      <translation>Ошибка</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="809"/>
      <source>Several sub-elements selected</source>
      <translation>Выбрано несколько подэлементов</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="810"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>Выберите одну грань в качестве опорной для эскиза!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="817"/>
      <source>Select a face as support for a sketch!</source>
      <translation>Выберите грань в качестве опорной для эскиза!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="824"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>Требуется плоская грань в качестве опорной для эскиза!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="831"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>Сначала создайте плоскость или выберите грань для размещения эскиза</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="816"/>
      <source>No support face selected</source>
      <translation>Не выбрана грань</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="823"/>
      <source>No planar support</source>
      <translation>Неплоская грань</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="830"/>
      <source>No valid planes in this document</source>
      <translation>В документе нет подходящих плоскостей</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="257"/>
      <location filename="../../ViewProvider.cpp" line="135"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <location filename="../../Command.cpp" line="1138"/>
      <location filename="../../SketchWorkflow.cpp" line="728"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Диалог уже открыт в панели задач</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Невозможно применить данную команду, в связи с отсутствием твердого для вычитания формы.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="995"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Убедитесь в том что тело содержит хоть какую-нибудь форму, перед попыткой применения субтрактивной команды.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1019"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Невозможно использовать выбранный объект. Выбранный объект должен принадлежать активному телу</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>Нет активного тела. Пожалуйста, активируйте тело перед вставкой вспомогательного объекта.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="467"/>
      <source>Sub-shape binder</source>
      <translation>Подобъектная связующая форма</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>No sketch to work on</source>
      <translation>Не найден эскиз для работы с ним</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1052"/>
      <source>No sketch is available in the document</source>
      <translation>В документе отсутствуют эскизы для применения данного действия</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="258"/>
      <location filename="../../ViewProvider.cpp" line="136"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <location filename="../../Command.cpp" line="1139"/>
      <location filename="../../SketchWorkflow.cpp" line="729"/>
      <source>Close this dialog?</source>
      <translation>Закрыть диалоговое окно?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <location filename="../../Command.cpp" line="1856"/>
      <source>Wrong selection</source>
      <translation>Неправильный выбор</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Выберите ребро, грань или тело от одного тела.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1829"/>
      <location filename="../../Command.cpp" line="2191"/>
      <source>Selection is not in the active body</source>
      <translation>Выделение не находится в активном теле</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1857"/>
      <source>Shape of the selected part is empty</source>
      <translation>Форма выбранной детали пуста</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1830"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Выберите ребро, грань или тело от активного тела.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1022"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>Рассмотрите возможность использования связующей формы или базовой операции для привязки внешней геометрии в теле</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1843"/>
      <source>Wrong object type</source>
      <translation>Неверный тип объекта</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1844"/>
      <source>%1 works only on parts.</source>
      <translation>%1 работает только с деталями.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2192"/>
      <source>Please select only one feature in an active body.</source>
      <translation>Пожалуйста, выберите только одну операцию в активном теле.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="71"/>
      <source>Part creation failed</source>
      <translation>Ошибка создания детали</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="72"/>
      <source>Failed to create a part object.</source>
      <translation>Не удалось создать объект Деталь.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="149"/>
      <location filename="../../CommandBody.cpp" line="215"/>
      <source>Bad base feature</source>
      <translation>Неверная базовая операция</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="126"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>Тело не может основываться на операции/функции Проектной Детали.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 уже принадлежит одному телу и не может использоваться в качестве базового элемента для другого тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="150"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>Базовый элемент (%1) принадлежит другой детали.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="177"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит из нескольких твердых тел.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит из нескольких оболочек.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>Выбранная фигура состоит только из оболочки.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="195"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>Выбранная форма состоит из нескольких твердых тел или оболочек.
Это может привести к неожиданным результатам.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="204"/>
      <source>Base feature</source>
      <translation>Базовый элемент</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="216"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Тело может быть основано не более, чем на одном элементе.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="230"/>
      <source>Body</source>
      <translation>Тело</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="421"/>
      <source>Nothing to migrate</source>
      <translation>Ничего нет для миграции</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="692"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>Выберите строго одну операцию/функцию Проектной Детали или тело.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="700"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>Не удалось определить тело для выбранной операции/функции "%s".</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Only features of a single source body can be moved</source>
      <translation>Можно перемещать элементы только одного исходного тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Плоскость эскиза не может быть перенесена</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="422"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>Нет элементов Проектной Детали без тела. Для переноса ничего нет.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="617"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Пожалуйста, отредактируйте '%1' и переопределите его, чтобы использовать базовую или опорную плоскость в качестве плоскости эскиза.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <location filename="../../CommandBody.cpp" line="699"/>
      <location filename="../../CommandBody.cpp" line="711"/>
      <location filename="../../CommandBody.cpp" line="1061"/>
      <location filename="../../CommandBody.cpp" line="1071"/>
      <source>Selection error</source>
      <translation>Ошибка выбора</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="712"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Только операция/функция с твёрдым телом может быть точкой окончания расчётов тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="846"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <location filename="../../CommandBody.cpp" line="894"/>
      <source>Features cannot be moved</source>
      <translation>Элементы не могут быть перемещены</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Некоторые из выбранных элементов имеют зависимости в исходном теле</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>There are no other bodies to move to</source>
      <translation>Нет других тел для перемещения к</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1062"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Невозможно переместить базовые элементы тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1072"/>
      <source>Select one or more features from the same body.</source>
      <translation>Выберите один или несколько элементов одного тела.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1087"/>
      <source>Beginning of the body</source>
      <translation>Начало тела</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1168"/>
      <source>Dependency violation</source>
      <translation>Нарушение зависимостей</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1169"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Более ранние операции/функции не должны зависеть от более поздних.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="307"/>
      <source>No previous feature found</source>
      <translation>Предыдущий элемент не найден</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="308"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Невозможно создать субтрактивный элемент без базового элемента</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Vertical sketch axis</source>
      <translation>Вертикальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Horizontal sketch axis</source>
      <translation>Горизонтальная ось эскиза</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="243"/>
      <source>Construction line %1</source>
      <translation>Вспомогательная линия %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="85"/>
      <source>Face</source>
      <translation>Грань</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>Active Body Required</source>
      <translation>Требуется активное тело</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="148"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>Для использования Проектной Детали в документе необходимо наличие активного тела. Активируйте тело (дважды щёлкните по нему) или создайте новое. 

Для устаревших документов с объектами Проектной Детали, не имеющих тела, используйте функцию миграции в Проектную Деталь, чтобы поместить их в тело.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="207"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>Для создания нового объекта Проектной Детали в документе необходимо иметь активное тело. Активируйте существующее тело (дважды щёлкните по нему) или создайте новое.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="273"/>
      <source>Feature is not in a body</source>
      <translation>Элемент находится вне тела</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="274"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Для использования этой операции/функции, она должна применяться к объекту Тело в документе.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="320"/>
      <source>Feature is not in a part</source>
      <translation>Элемент находится вне детали</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="321"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Для использования этой операции/функции, она должна применяться к объекту Деталь в документе.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="62"/>
      <location filename="../../ViewProviderTransformed.cpp" line="63"/>
      <location filename="../../ViewProvider.cpp" line="92"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="225"/>
      <source>Edit %1</source>
      <translation>Редактировать %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="105"/>
      <source>Set Face Colors</source>
      <translation>Задать цвет граней</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="112"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Plane</source>
      <translation>Плоскость</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="117"/>
      <location filename="../../ViewProviderDatum.cpp" line="207"/>
      <source>Line</source>
      <translation>Линия</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="122"/>
      <location filename="../../ViewProviderDatum.cpp" line="217"/>
      <source>Point</source>
      <translation>Точка</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="127"/>
      <source>Coordinate System</source>
      <translation>Система координат</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="234"/>
      <source>Edit Datum</source>
      <translation>Редактировать опорный элемент</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="91"/>
      <source>Feature error</source>
      <translation>Ошибка элемента</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="92"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1 не имеет базовый элемент. Этот элемент нарушен и не может быть изменён.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="220"/>
      <source>Edit Shape Binder</source>
      <translation>Редактировать связующую форму</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="350"/>
      <source>Synchronize</source>
      <translation>Синхронизировать</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Select Bound Object</source>
      <translation>Выбрать граничащий объект</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="154"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>Документ "%1", который вы редактируете, был создан с помощью старой версии верстака Проектная Деталь.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>Выполнить миграцию, чтобы использовать современные функции верстака Проектная Деталь?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="166"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>Документ "%1" находится либо в процессе миграции из устаревшей версии верстака Проектная Деталь, либо имеет слегка повреждённую структуру.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="173"/>
      <source>Make the migration automatically?</source>
      <translation>Выполнить миграцию автоматически?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="176"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Примечание: Если вы решите выполнить миграцию, вы не сможете редактировать файл в более старой версии FreeCAD. Если вы откажетесь от миграции, вы не сможете использовать новые функции верстака Проектная Деталь, такие как Тело и Деталь. В результате вы также не сможете использовать свои детали в верстаке Сборка.

Однако вы сможете выполнить миграцию в любой момент с помощью команды "Проектная Деталь -&gt; Миграция".</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="189"/>
      <source>Migrate Manually</source>
      <translation>Миграция вручную</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="67"/>
      <source>Edit Boolean</source>
      <translation>Редактировать булеву операцию</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="40"/>
      <source>Edit Chamfer</source>
      <translation>Редактировать фаску</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="41"/>
      <source>Edit Draft</source>
      <translation>Редактировать уклон</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="40"/>
      <source>Edit Fillet</source>
      <translation>Редактировать скругление</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="43"/>
      <source>Edit Groove</source>
      <translation>Редактировать проточку</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="48"/>
      <source>Edit Helix</source>
      <translation>Редактировать спираль</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit Hole</source>
      <translation>Редактировать отверстие</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="38"/>
      <source>Edit Linear Pattern</source>
      <translation>Редактировать линейный шаблон</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="65"/>
      <source>Edit Loft</source>
      <translation>Редактировать профиль по сечениям</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="38"/>
      <source>Edit Mirror</source>
      <translation>Редактировать зеркальное преобразование</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="47"/>
      <source>Edit Multi-Transform</source>
      <translation>Редактировать множественное преобразование</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="43"/>
      <source>Edit Pad</source>
      <translation>Редактировать выдавливание</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="75"/>
      <source>Edit Pipe</source>
      <translation>Редактировать профиль по траектории</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="45"/>
      <source>Edit Pocket</source>
      <translation>Редактировать вырезание</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation>Редактировать круговой шаблон</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="49"/>
      <source>Edit Primitive</source>
      <translation>Редактировать примитив</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="43"/>
      <source>Edit Revolution</source>
      <translation>Редактировать вращение</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="38"/>
      <source>Edit Scale</source>
      <translation>Редактировать масштабирование</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="40"/>
      <source>Edit Thickness</source>
      <translation>Редактировать придать толщину</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>Параметры цепного колеса</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>Число зубьев</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>Стандарт цепного колеса</translation>
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
      <translation>Велосипедное с переключением скоростей</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>Велосипедное без переключения скоростей</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>Шаг цепи</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>Диаметр ролика цепи</translation>
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
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Автоматическое обновление изменений в резьбе
Обратите внимание, что вычисления могут занять некоторое время</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>Глубина резьбы </translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>Настройка межвиткового зазора</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>Отклонение</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>Тип головки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>Тип углубления</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>Диаметр головки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>Глубина головки</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation>Свободное / Сквозное</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation>Под резьбу</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation>Моделировать резьбу</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation>Тип отверстия</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>Обновить отображение резьбы</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>Пользовательское отклонение</translation>
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
      <translation>Свободный/широкий</translation>
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
      <translation>Параметры отверстия</translation>
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
      <translation>Угол сверла</translation>
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
      <translation>&lt;b&gt;Нарезание резьбы&lt;/b&gt;</translation>
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
      <translation>Размер кончика сверла будет учитываться
при определении глубины глухих отверстий</translation>
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
меньше 90: отверстие сужается к основанию
больше 90: отверстия расширяется к основанию</translation>
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
      <location filename="../../Workbench.cpp" line="41"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Эскиз</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Проектная Деталь</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>Datums</source>
      <translation>Опорные элементы</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Additive Features</source>
      <translation>Аддитивные операции</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Subtractive Features</source>
      <translation>Субтрактивные операции</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Dress-Up Features</source>
      <translation>Функции отделки</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Transformation Features</source>
      <translation>Операции преобразования</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Sprocket…</source>
      <translation>Звёздочка…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Involute Gear</source>
      <translation>Эвольвентная шестерня</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Shaft Design Wizard</source>
      <translation>Мастер проектирования вала</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Measure</source>
      <translation>Измерения</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Refresh</source>
      <translation>Обновить</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Toggle 3D</source>
      <translation>Переключить 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Part Design Helper</source>
      <translation>Помощник по Проектной Детали</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Modeling</source>
      <translation>Моделирование Проектной Детали</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Length [mm]</source>
      <translation>Длина [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Diameter [mm]</source>
      <translation>Диаметр [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Inner diameter [mm]</source>
      <translation>Внутренний диаметр [мм]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Constraint type</source>
      <translation>Тип ограничения</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge type</source>
      <translation>Тип начального ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Start edge size</source>
      <translation>Размер начального ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>End edge type</source>
      <translation>Тип конечного ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>Размер конечного ребра</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="67"/>
      <source>Shaft Wizard</source>
      <translation>Мастер создания вала</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="75"/>
      <source>Section 1</source>
      <translation>Сечение 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Section 2</source>
      <translation>Сечение 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="80"/>
      <source>Add column</source>
      <translation>Добавить столбец</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="128"/>
      <source>Section %s</source>
      <translation>Сечение %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="157"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="176"/>
      <source>None</source>
      <translation>Ничего</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="158"/>
      <source>Fixed</source>
      <translation>Неподвижное</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <source>Force</source>
      <translation>Сила</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Bearing</source>
      <translation>Подшипник</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Gear</source>
      <translation>Шестерня</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Pulley</source>
      <translation>Шкив</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="179"/>
      <source>Chamfer</source>
      <translation>Фаска</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="180"/>
      <source>Fillet</source>
      <translation>Скругление</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="58"/>
      <source>All</source>
      <translation>Все</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="118"/>
      <source>Missing Module</source>
      <translation>Модуль отсутствует</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="124"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>Дополнение Plot не установлено. Установите его, чтобы включить эту функцию.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="251"/>
      <source>Shaft design wizard...</source>
      <translation>Мастер проектирования вала...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="254"/>
      <source>Start the shaft design wizard</source>
      <translation>Запускает мастер проектирования вала</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>Связанный объект не является операцией/функцией Проектной Детали</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="412"/>
      <source>Tip shape is empty</source>
      <translation>Точка окончания расчётов - Форма пуста</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="66"/>
      <source>BaseFeature link is not set</source>
      <translation>Ссылка на BaseFeature не установлена</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="72"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>BaseFeature должен быть Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="82"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature имеет пустую форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="75"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Невозможно выполнить булево обрезать без BaseFeature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Невозможно выполнить булеву операцию с чем-либо, кроме Part::Feature и его производных</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="104"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Невозможно выполнить логическую операцию с недопустимой базовой формой</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="192"/>
      <location filename="../../../App/FeatureDraft.cpp" line="332"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="159"/>
      <location filename="../../../App/FeatureFillet.cpp" line="140"/>
      <location filename="../../../App/FeatureHole.cpp" line="2067"/>
      <location filename="../../../App/FeatureLoft.cpp" line="331"/>
      <location filename="../../../App/FeatureLoft.cpp" line="375"/>
      <location filename="../../../App/FeaturePipe.cpp" line="480"/>
      <location filename="../../../App/FeaturePipe.cpp" line="529"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="284"/>
      <location filename="../../../App/FeatureGroove.cpp" line="253"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="773"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="789"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="802"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>Результат содержит несколько твёрдых тел: включите параметр "Разрешить составные объекты" в активном теле.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="114"/>
      <source>Tool shape is null</source>
      <translation>Форма инструмента пустая</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="141"/>
      <source>Unsupported boolean operation</source>
      <translation>Неподдерживаемая булева операция</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="351"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>Невозможно выполнить выдавливание с нулевой общей длиной.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="356"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>Невозможно выполнить вырезание с нулевой общей длиной.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="704"/>
      <source>No extrusion geometry was generated.</source>
      <translation>Никакой геометрии выдавливания не было сгенерировано.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="728"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>Результат операции выдавливания равен нулю (null).</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="368"/>
      <location filename="../../../App/FeaturePipe.cpp" line="521"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="139"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="764"/>
      <source>Resulting shape is not a solid</source>
      <translation>Полученная форма не является твёрдотельной</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="172"/>
      <source>Failed to create chamfer</source>
      <translation>Не удалось создать фаску</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="327"/>
      <location filename="../../../App/FeatureFillet.cpp" line="120"/>
      <source>Resulting shape is null</source>
      <translation>Результирующая форма нулевая (null)</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="141"/>
      <source>No edges specified</source>
      <translation>Ребра не указаны</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="293"/>
      <source>Size must be greater than zero</source>
      <translation>Размер должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="304"/>
      <source>Size2 must be greater than zero</source>
      <translation>Размер2 должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="311"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>Угол должен быть больше 0.0 и меньше 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="95"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>Скругление невозможно на выбранных фигурах</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="103"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Радиус скругления должен быть больше нуля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="157"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>Ошибка операции скругления.
Выбранные рёбра могут содержать геометрические элементы, которые невозможно скруглить вместе.
Попробуйте скруглить рёбра по отдельности или с меньшим радиусом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>Angle of groove too large</source>
      <translation>Слишком большой угол канавки</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="108"/>
      <source>Angle of groove too small</source>
      <translation>Угол канавки слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>Запрошенный элемент не может быть создан. Причина может быть в том, что:
  - активное тело не содержит базовой формы, поэтому нет
  материала для удаления;
  - выбранный эскиз не принадлежит активному телу.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="400"/>
      <source>Failed to obtain profile shape</source>
      <translation>Не удалось получить форму профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="454"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Создание не удалось, поскольку направление ортогонально вектору нормали эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="176"/>
      <location filename="../../../App/FeatureGroove.cpp" line="154"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="477"/>
      <source>Creating a face from sketch failed</source>
      <translation>Не удалось создать грань из эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="115"/>
      <source>Angles of groove nullify each other</source>
      <translation>Углы канавки сводят на нет друг друга</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="193"/>
      <location filename="../../../App/FeatureGroove.cpp" line="171"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>Ось вращения пересекает эскиз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="294"/>
      <location filename="../../../App/FeatureGroove.cpp" line="263"/>
      <source>Could not revolve the sketch!</source>
      <translation>Не удалось провернуть эскиз!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="306"/>
      <location filename="../../../App/FeatureGroove.cpp" line="275"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Не удалось создать грань из эскиза.
Пересекающиеся объекты эскиза в эскизе не допускаются.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="235"/>
      <source>Error: Pitch too small!</source>
      <translation>Ошибка: Шаг слишком мал!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="240"/>
      <location filename="../../../App/FeatureHelix.cpp" line="263"/>
      <source>Error: height too small!</source>
      <translation>Ошибка: высота слишком мала!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="249"/>
      <source>Error: pitch too small!</source>
      <translation>Ошибка: шаг слишком мал!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="254"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="277"/>
      <source>Error: turns too small!</source>
      <translation>Ошибка: число оборотов мало!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="283"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Ошибка: ни высота, ни нарастание не должны быть равны нулю!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: unsupported mode</source>
      <translation>Ошибка: неподдерживаемый режим</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="315"/>
      <source>Error: No valid sketch or face</source>
      <translation>Ошибка: нет допустимого эскиза или грани</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="328"/>
      <source>Error: Face must be planar</source>
      <translation>Ошибка: грань должна быть плоской</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2422"/>
      <location filename="../../../App/FeatureHelix.cpp" line="443"/>
      <location filename="../../../App/FeatureHelix.cpp" line="484"/>
      <source>Error: Result is not a solid</source>
      <translation>Ошибка: Результат не твердотельный</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="413"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Ошибка: Нечего вычитать</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="449"/>
      <location filename="../../../App/FeatureHelix.cpp" line="490"/>
      <source>Error: Result has multiple solids</source>
      <translation>Ошибка: Результат имеет несколько твердых частей</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="434"/>
      <source>Error: Adding the helix failed</source>
      <translation>Ошибка: добавление спирали не удалось</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>Ошибка: пересечение спирали не удалось</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="475"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>Ошибка: не удалось вычесть спираль</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="506"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Ошибка: Не удалось создать грань из эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1224"/>
      <source>Thread type is invalid</source>
      <translation>Недопустимый тип резьбы</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1764"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Ошибка отверстия: неподдерживаемая длина</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1770"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Ошибка отверстия: неверная глубина отверстия</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1796"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Ошибка при создании отверстия: недопустимый угол зенкования</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1820"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Ошибка при создании отверстия: диаметр цековки слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1825"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Ошибка при создании отверстия: глубина цековки должна быть меньше глубины отверстия</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1832"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Ошибка при создании отверстия: глубина цековки должна быть больше либо равна нулю</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1862"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Ошибка при создании отверстия: Недопустимая зенковка</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1898"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Ошибка при создании отверстия: Недопустимый угол сверла</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1915"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Ошибка при создании отверстия: недопустимая режущая часть сверла</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1952"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Ошибка при создании отверстия: Не удалось провернуть эскиз</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1959"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Ошибка при создании отверстия: результирующая форма пуста</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1972"/>
      <source>Error: Adding the thread failed</source>
      <translation>Ошибка: не удалось добавить резьбу</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Finding axis failed</source>
      <translation>Ошибка отверстия: Не удалось найти ось</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2039"/>
      <location filename="../../../App/FeatureHole.cpp" line="2047"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Булева операция не удалась на профиле Ребра</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>Булева операция привела к появлению нетвёрдотельных элементов на профиле Ребра</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="151"/>
      <source>Boolean operation failed</source>
      <translation>Не удалось выполнить булеву операцию</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2080"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Не удалось создать грань из эскиза.
Пересекающиеся элементы эскиза или несколько граней в эскизе не допускаются для создания выреза до грани.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2245"/>
      <source>Thread type out of range</source>
      <translation>Тип резьбы выходит за пределы допустимого диапазона</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2248"/>
      <source>Thread size out of range</source>
      <translation>Размер резьбы выходит за пределы допустимого диапазона</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2396"/>
      <source>Error: Thread could not be built</source>
      <translation>Ошибка: резьба не может быть построена</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="191"/>
      <source>Loft: At least one section is needed</source>
      <translation>Операция по сечениям: Требуется хотя бы одно сечение</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="392"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Операция по сечениям: Создание привело к фатальной ошибке</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="238"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Операция по сечениям: Не удалось создать грань из эскиза</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePipe.cpp" line="444"/>
      <source>Loft: Failed to create shell</source>
      <translation>Операция по сечениям: Не удалось создать оболочку</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="817"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Не удалось создать грань из эскиза.
Эскизы с пересекающимися элементами или множественными контурами не допускаются.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="203"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Профиль по траектории: Не удалось получить форму профиля</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="210"/>
      <source>No spine linked</source>
      <translation>Нет привязки к направляющей</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="225"/>
      <source>No auxiliary spine linked.</source>
      <translation>Нет привязки к вспомогательным направляющим.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="248"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Профиль по траектории: при использовании точки в качестве сечения эскиз должен содержать только одну точку</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="257"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Профиль по траектории: При использовании точки в качестве профиля требуется как минимум одно сечение</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>Профиль по траектории: Все сечения должны быть элементами Деталь</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="283"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Профиль по траектории: не удалось получить форму сечения</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="293"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Профиль по траектории: Вершинами могут быть только профиль и последнее сечение</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="306"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Мультисечения должны иметь такое же количество внутренних кривых каркаса, как и базовое сечение</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="339"/>
      <source>Path must not be a null shape</source>
      <translation>Траектория не должна иметь нулевую форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="379"/>
      <source>Pipe could not be built</source>
      <translation>Профиль по траектории не может быть построен</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>Result is not a solid</source>
      <translation>Результат не является твердым телом</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="475"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Профиль по траектории: Здесь нечего вычитать</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="543"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>Произошла фатальная ошибка при создании профиля по траектории</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="672"/>
      <source>Invalid element in spine.</source>
      <translation>Недопустимый элемент в каркасе.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="677"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Элемент в каркасе не является ни ребром, ни кривой.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Spine is not connected.</source>
      <translation>Каркас не присоединён.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="704"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Каркас не является ни ребром, ни кривой.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="709"/>
      <source>Invalid spine.</source>
      <translation>Неверный каркас.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="101"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Невозможно вычесть примитив без базового элемента</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="353"/>
      <location filename="../../../App/FeaturePipe.cpp" line="505"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Unknown operation type</source>
      <translation>Неизвестный тип операции</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="361"/>
      <location filename="../../../App/FeaturePipe.cpp" line="513"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="131"/>
      <source>Failed to perform boolean operation</source>
      <translation>Не удалось выполнить логическую операцию</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="215"/>
      <source>Length of box too small</source>
      <translation>Длина блока слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="220"/>
      <source>Width of box too small</source>
      <translation>Ширина блока слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="225"/>
      <source>Height of box too small</source>
      <translation>Высота блока слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="273"/>
      <source>Radius of cylinder too small</source>
      <translation>Радиус цилиндра слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="278"/>
      <source>Height of cylinder too small</source>
      <translation>Высота цилиндра слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="283"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Угол разворота цилиндра слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="340"/>
      <source>Radius of sphere too small</source>
      <translation>Радиус сферы слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="392"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="397"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Радиус конуса не может быть отрицательным</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="402"/>
      <source>Height of cone too small</source>
      <translation>Высота конуса слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="482"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="487"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Радиус эллипсоида слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="581"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="586"/>
      <source>Radius of torus too small</source>
      <translation>Радиус торса слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="671"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Многоугольник призмы неправилен, должен иметь 3 или более сторон</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="676"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Радиус описанной окружности многоугольника призмы слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="681"/>
      <source>Height of prism is too small</source>
      <translation>Высота призмы слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="768"/>
      <source>delta x of wedge too small</source>
      <translation>дельта x клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="774"/>
      <source>delta y of wedge too small</source>
      <translation>дельта y клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="780"/>
      <source>delta z of wedge too small</source>
      <translation>дельта z клина слишком мала</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="786"/>
      <source>delta z2 of wedge is negative</source>
      <translation>дельта z2 клина отрицательная</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="792"/>
      <source>delta x2 of wedge is negative</source>
      <translation>дельта x2 клина отрицательная</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="123"/>
      <source>Angle of revolution too large</source>
      <translation>Слишком большой угол проворота</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Angle of revolution too small</source>
      <translation>Угол проворота слишком мал</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Angles of revolution nullify each other</source>
      <translation>Углы вращения сводят на нет друг друга</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="168"/>
      <location filename="../../../App/FeatureGroove.cpp" line="146"/>
      <source>Reference axis is invalid</source>
      <translation>Базовая ось недействительна</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="756"/>
      <source>Fusion with base feature failed</source>
      <translation>Сбой слияния с базовым элементом</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="99"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Функция преобразования Связанный объект не является объектом Деталь</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="106"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>Нет оригиналов, связанных с преобразованным объектом.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="346"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Невозможно преобразовать недопустимую опорную форму</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="397"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>Форма аддитивного/субтрактивного преобразования пустая</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="388"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Только аддитивные и субтрактивные операции/функции могут быть преобразованы</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="107"/>
      <source>Invalid face reference</source>
      <translation>Неверная грань-ориентир</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="60"/>
      <source>Involute Gear</source>
      <translation>Эвольвентная шестерня</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="64"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>Создаёт или редактирует параметры эвольвентного зубчатого колеса</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="63"/>
      <source>Sprocket</source>
      <translation>Звёздочка</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="67"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>Создаёт или редактирует параметры цепного колеса.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>Показать конечный результат</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>Показать предварительное наложение</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="48"/>
      <source>Preview</source>
      <translation>Предварительный просмотр</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="223"/>
      <source>Shaft Design Wizard</source>
      <translation>Мастер проектирования вала</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="226"/>
      <source>Starts the shaft design wizard</source>
      <translation>Запускает мастер проектирования вала</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>Сбой при вычислении предварительного просмотра вырезаемого объёма: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="105"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>Получаемая форма пустая. Это может означать, что материал не будет удалён, или что с моделью возникли проблемы.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2644"/>
      <source>Create Datum</source>
      <translation>Создать опорный элемент</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2645"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Создаёт опорный элемент или локальную систему координат</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2679"/>
      <source>Create Datum</source>
      <translation>Создать опорный элемент</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2680"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Создаёт опорный элемент или локальную систему координат</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>Создаёт аддитивный блок по его ширине, высоте и длине</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>Создаёт аддитивный цилиндр по его радиусу, высоте и углу раскрытия</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>Создаёт аддитивную сферу по её радиусу и различным углам</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Creates an additive cone</source>
      <translation>Создаёт аддитивный конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Creates an additive ellipsoid</source>
      <translation>Создаёт аддитивный эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Creates an additive torus</source>
      <translation>Создаёт аддитивный тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Creates an additive prism</source>
      <translation>Создаёт аддитивную призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Creates an additive wedge</source>
      <translation>Создаёт аддитивный клин</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>Создаёт субтрактивный блок по его ширине, высоте и длине</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>Создаёт субтрактивный цилиндр по его радиусу, высоте и углу раскрытия</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>Создаёт субтрактивную сферу по её радиусу и различным углам</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Creates a subtractive cone</source>
      <translation>Создаёт субтрактивный конус</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>Создаёт субтрактивный эллипсоид</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Creates a subtractive torus</source>
      <translation>Создаёт субтрактивный тор</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Creates a subtractive prism</source>
      <translation>Создаёт субтрактивную призму</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Creates a subtractive wedge</source>
      <translation>Создаёт субтрактивный клин</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1007"/>
      <source>Attachment</source>
      <translation>Присоединение</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="835"/>
      <source>Revolution Parameters</source>
      <translation>Параметры вращения</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="845"/>
      <source>Groove Parameters</source>
      <translation>Параметры проточки</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation>Сообщения Функций преобразования</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="122"/>
      <source>Active Body</source>
      <translation>Активное тело</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="43"/>
      <source>Chamfer Parameters</source>
      <translation>Параметры фаски</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="113"/>
      <source>Datum Plane Parameters</source>
      <translation>Параметры опорной плоскости</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="118"/>
      <source>Datum Line Parameters</source>
      <translation>Параметры опорной линии</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="123"/>
      <source>Datum Point Parameters</source>
      <translation>Параметры опорной точки</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="128"/>
      <source>Local Coordinate System Parameters</source>
      <translation>Параметры локальной системы координат</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="44"/>
      <source>Draft Parameters</source>
      <translation>Параметры уклона</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="43"/>
      <source>Fillet Parameters</source>
      <translation>Параметры скругления</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="40"/>
      <source>Linear Pattern Parameters</source>
      <translation>Параметры линейного шаблона</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="40"/>
      <source>Mirror Parameters</source>
      <translation>Параметры зеркального отражения</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="40"/>
      <source>Multi-Transform Parameters</source>
      <translation>Параметры множественного преобразования</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="40"/>
      <source>Polar Pattern Parameters</source>
      <translation>Параметры кругового шаблона</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="40"/>
      <source>Scale Parameters</source>
      <translation>Параметры масштабирования</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="43"/>
      <source>Thickness Parameters</source>
      <translation>Параметры придания толщины</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="130"/>
      <source>Direction 2</source>
      <translation>Направление 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="246"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>Выберите ориентир для направления (ребро, грань, опорная линия)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="332"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>Неверный выбор. Выберите ребро, плоскую грань или опорную линию.</translation>
    </message>
  </context>
</TS>
