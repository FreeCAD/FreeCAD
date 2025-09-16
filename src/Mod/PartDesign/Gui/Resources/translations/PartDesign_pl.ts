<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="pl" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="77"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Punkt środkowy początku helisy, wyprowadzony z osi odniesienia.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="79"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Kierunek helisy, wynikający z jej osi referencyjnej.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="81"/>
      <source>The reference axis of the helix.</source>
      <translation>Oś referencyjna helisy.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="83"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Tryb wprowadzania helisy definiuje, które właściwości są ustawiane przez użytkownika.
Po tym następuje obliczenie właściwości zależnych.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="87"/>
      <source>The axial distance between two turns.</source>
      <translation>Wzdłużna odległość pomiędzy dwoma zwojami.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="89"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Wysokość ścieżki profilu helisy, bez uwzględnienia wysokości profilu.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="91"/>
      <source>The number of turns in the helix.</source>
      <translation>Liczba zwojów helisy.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>Kąt stożka, który tworzy powłokę wokół helisy.
Wartości niezerowe zmieniają helisę w stożkową spiralę.
Wartości dodatnie powodują, że promień rośnie, a ujemne, że maleje.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="99"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Przyrost promienia spirali na obrót.
Wartości niezerowe zmieniają spiralę w spiralę stożkową.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="102"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Ustawia kierunek obrotu na lewoskrętny,
tzn. przeciwny do ruchu wskazówek zegara, poruszając się wzdłuż osi.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="105"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Określa, czy helisa jest skierowana w kierunku przeciwnym do osi.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="107"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Jeśli opcja jest wybrana, wynikiem będzie część wspólna profilu i istniejącej już zawartości.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="109"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Jeśli opcja ta ma wartość "Fałsz", narzędzie zaproponuje wartość początkową nachylenia na podstawie ramki otaczającej profil,
aby uniknąć samoczynnego przecięcia.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="112"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Tolerancja łączenia dla helisy, zwiększ jeśli kształt helisy nie łączy się poprawnie z częścią.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="98"/>
      <source>Number of gear teeth</source>
      <translation>Liczba zębów</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="104"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Kąt przyporu zęba koła zębatego</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="101"/>
      <source>Module of the gear</source>
      <translation>Moduł koła zębatego</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>Wartość Prawda = dwie krzywe z trzema punktami kontrolnymi,
wartość Fałsz = jedna krzywa z czterema punktami kontrolnymi.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>Wartość Prawda = uzębienie zewnętrzne,
wartość Fałsz = uzębienie wewnętrzne</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>Wysokość zęba od okręgu podziałowego do wierzchołka, znormalizowana przez moduł.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>Wysokość zęba od okręgu podziałowego do podstawy, znormalizowana przez moduł.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Promień zaokrąglenia u podstawy zęba, znormalizowany przez moduł.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>Odległość, o jaką profil odniesienia jest przesuwany na zewnątrz, znormalizowana przez moduł.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1498"/>
      <source>PartDesign</source>
      <translation>Projekt części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1499"/>
      <source>Additive Helix</source>
      <translation>Addytywna helisa</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1500"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Wyciąga zaznaczony szkic lub profil wzdłuż helisy i dodaje go do zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1405"/>
      <source>PartDesign</source>
      <translation>Projekt części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1406"/>
      <source>Additive Loft</source>
      <translation>Uzupełnianie wyciągnięciem przez profile</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1407"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>Przeciąga zaznaczony szkic lub profil wzdłuż ścieżki i dodaje go do bryły</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1311"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1312"/>
      <source>Additive Pipe</source>
      <translation>Addytywna rura</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1313"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Wyciąga zaznaczony szkic lub profil wzdłuż ścieżki i dodaje go do zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="88"/>
      <source>New Body</source>
      <translation>Nowa zawartość</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="89"/>
      <source>Creates a new body and activates it</source>
      <translation>Tworzy nową zawartość i aktywuje ją</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2309"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2310"/>
      <source>Boolean Operation</source>
      <translation>Operacja logiczna</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2311"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>Stosuje operacje logiczne z zaznaczonymi obiektami i aktywną Zawartością</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Local Coordinate System</source>
      <translation>Lokalny układ współrzędnych</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>Creates a new local coordinate system</source>
      <translation>Tworzy nowy lokalny układ współrzędnych.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1778"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1779"/>
      <source>Chamfer</source>
      <translation>Sfazowanie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1780"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>Stosuje fazowanie do zaznaczonych krawędzi lub ścian</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>PartDesign</source>
      <translation>Projekt części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="430"/>
      <source>Clone</source>
      <translation>Klonuj</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="431"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>Kopiuje stały obiekt parametryczny jako podstawową cechę nowej Zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1807"/>
      <source>PartDesign</source>
      <translation>Part Design</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1808"/>
      <source>Draft</source>
      <translation>Pochylenie ścian</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1809"/>
      <source>Applies a draft to the selected faces</source>
      <translation>Stosuje pochylenie do zaznaczonych ścian</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="614"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="615"/>
      <source>Duplicate &amp;Object</source>
      <translation>Duplikuj obiekt</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Powiela zaznaczony obiekt i dodaje go do aktywnej zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1750"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1751"/>
      <source>Fillet</source>
      <translation>Zaokrąglenie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1752"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>Stosuje zaokrąglenie do zaznaczonych krawędzi lub ścian</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1248"/>
      <source>PartDesign</source>
      <translation>Part Design</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1249"/>
      <source>Groove</source>
      <translation>Rowek</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1250"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>Obraca szkic lub profil wokół linii lub osi i usuwa go z bryły</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1150"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1151"/>
      <source>Hole</source>
      <translation>Otwór</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1152"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>Tworzy otwory w aktywnej bryle w punktach środkowych kół lub łuków zaznaczonego szkicu lub profilu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>Datum Line</source>
      <translation>Linia odniesienia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Creates a new datum line</source>
      <translation>Utwórz nową linię odniesienia</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2044"/>
      <source>Linear Pattern</source>
      <translation>Wzór liniowy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2045"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>Duplikuje zaznaczone cechy lub aktywną bryłę w układzie liniowym</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="320"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="321"/>
      <source>Migrate</source>
      <translation>Przenieś ze starszej wersji</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="322"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>Migruje dokument do nowoczesnego przepływu pracy środowiska Projektowania Części</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="1992"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1993"/>
      <source>Mirror</source>
      <translation>Odbicie lustrzane</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1994"/>
      <source>Mirrors the selected features or active body</source>
      <translation>Tworzy lustrzane odbicie zaznaczonych cech lub aktywnej bryły</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="674"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="675"/>
      <source>Move Object To…</source>
      <translation>Przenieś obiekt do …</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="676"/>
      <source>Moves the selected object to another body</source>
      <translation>Przenosi zaznaczony obiekt do innej zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="841"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="842"/>
      <source>Move Feature After…</source>
      <translation>Przenieś cechę za …</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="843"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>Przenosi zaznaczoną cechę za inną cechę w tej samej Zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="535"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="536"/>
      <source>Set Tip</source>
      <translation>Ustaw czubek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="537"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>Przenosi czubek zawartości do zaznaczonej cechy</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2194"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2195"/>
      <source>Multi-Transform</source>
      <translation>Przekształcenie wielokrotne</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2196"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>Stosuje wiele przekształceń do zaznaczonych cech lub aktywnej bryły</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="503"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="504"/>
      <source>New Sketch</source>
      <translation>Nowy szkic</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="505"/>
      <source>Creates a new sketch</source>
      <translation>Tworzy nowy szkic</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1092"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1093"/>
      <source>Pad</source>
      <translation>Wyciągnij</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1094"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>Wyciąga zaznaczony szkic lub profil i dodaje go do zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>Datum Plane</source>
      <translation>Płaszczyzna odniesienia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="165"/>
      <source>Creates a new datum plane</source>
      <translation>Tworzy nową płaszczyznę odniesienia</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1121"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1122"/>
      <source>Pocket</source>
      <translation>Kieszeń</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1123"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>Wytłacza zaznaczony szkic lub profil i usuwa go z zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Point</source>
      <translation>Punkt odniesienia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum point</source>
      <translation>Tworzy nowy punkt odniesienia</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2097"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2098"/>
      <source>Polar Pattern</source>
      <translation>Układ promieniowy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2099"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>Duplikuje zaznaczone cechy lub aktywną bryłę w układzie promieniowym</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1193"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1194"/>
      <source>Revolve</source>
      <translation>Obrót</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1195"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>Obraca zaznaczony szkic lub profil wokół linii lub osi i dodaje go do zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2152"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2153"/>
      <source>Scale</source>
      <translation>Skala</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2154"/>
      <source>Scales the selected features or the active body</source>
      <translation>Skaluje zaznaczone cechy lub aktywną bryłę</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>Part Design</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Shape Binder</source>
      <translation>Łącznik kształtu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new shape binder</source>
      <translation>Tworzy nowy łącznik kształtu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Sub-Shape Binder</source>
      <translation>Łącznik kształtów podrzędnych</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="347"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>Tworzy odniesienie do geometrii z jednego lub wielu obiektów, umożliwiając jej użycie wewnątrz lub na zewnątrz zawartości. Śledzi względne położenia, obsługuje różne typy geometrii (bryły, ściany, krawędzie, wierzchołki) i może działać z obiektami w tym samym lub zewnętrznym dokumencie.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1570"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1571"/>
      <source>Subtractive Helix</source>
      <translation>Subtraktywna helisa</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1572"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>Wyciąga zaznaczony szkic lub profil wzdłuż helisy i usuwa go z zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1452"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1453"/>
      <source>Subtractive Loft</source>
      <translation>Subtraktywny loft</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1454"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>Przeciąga zaznaczony szkic lub profil wzdłuż ścieżki i usuwa go z zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1358"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1359"/>
      <source>Subtractive Pipe</source>
      <translation>Subtraktywna rura</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1360"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>Wyciąga zaznaczony szkic lub profil wzdłuż ścieżki i usuwa go z zawartości</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1875"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1876"/>
      <source>Thickness</source>
      <translation>Grubość</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1877"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>Nadaje grubość i usuwa zaznaczone ściany</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <source>Additive Primitive</source>
      <translation>Addytywne bryły pierwotne</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Creates an additive primitive</source>
      <translation>Tworzy addytywne bryły pierwotne</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="197"/>
      <source>Additive Box</source>
      <translation>Addytywny prostopadłościan</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="201"/>
      <source>Additive Cylinder</source>
      <translation>Addytywny walec</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="205"/>
      <source>Additive Sphere</source>
      <translation>Addytywna sfera</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="209"/>
      <source>Additive Cone</source>
      <translation>Addytywny stożek</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Ellipsoid</source>
      <translation>Addytywna elipsoida</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Additive Torus</source>
      <translation>Addytywny torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="221"/>
      <source>Additive Prism</source>
      <translation>Addytywny graniastosłup</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="225"/>
      <source>Additive Wedge</source>
      <translation>Addytywny klin</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="241"/>
      <source>PartDesign</source>
      <translation>Projekt części</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Subtractive Primitive</source>
      <translation>Subtraktywne bryły pierwotne</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>Creates a subtractive primitive</source>
      <translation>Tworzy subtraktywne bryły pierwotne</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="349"/>
      <source>Subtractive Box</source>
      <translation>Subtraktywny prostopadłościan</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="353"/>
      <source>Subtractive Cylinder</source>
      <translation>Subtraktywny walec</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="357"/>
      <source>Subtractive Sphere</source>
      <translation>Subtraktywna sfera</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="361"/>
      <source>Subtractive Cone</source>
      <translation>Subtraktywny stożek</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="365"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Subtraktywna elipsoida</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="369"/>
      <source>Subtractive Torus</source>
      <translation>Subtraktywny torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="373"/>
      <source>Subtractive Prism</source>
      <translation>Subtraktywny graniastosłup</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="377"/>
      <source>Subtractive Wedge</source>
      <translation>Subtraktywny klin</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="300"/>
      <source>Edit Shape Binder</source>
      <translation>Edytuj łącznik kształtu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="309"/>
      <source>Create Shape Binder</source>
      <translation>Utwórz łącznik kształtu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="392"/>
      <source>Create Sub-Shape Binder</source>
      <translation>Utwórz łącznik kształtu podrzędnego</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="447"/>
      <source>Create Clone</source>
      <translation>Utwórz klona</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="993"/>
      <source>Make Copy</source>
      <translation>Utwórz kopię</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2239"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>Konwertuj jako cechę wielokrotnej transformacji</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="258"/>
      <source>Sketch on Face</source>
      <translation>Szkic na powierzchni</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="317"/>
      <source>Make copy</source>
      <translation>Utwórz kopię</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="514"/>
      <location filename="../../SketchWorkflow.cpp" line="742"/>
      <source>New Sketch</source>
      <translation>Nowy szkic</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2327"/>
      <source>Create Boolean</source>
      <translation>Utwórz cechę funkcją logiczną</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="193"/>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <source>Add a Body</source>
      <translation>Dodaj zawartość</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="438"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>Migruj przestarzałe cechy środowiska Projekt Części do obiektu zawartości</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="628"/>
      <source>Duplicate a Part Design object</source>
      <translation>Duplikuj obiekt środowiska Projekt Części</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="911"/>
      <source>Move a feature inside body</source>
      <translation>Przenieś cechę do obiektu zawartości</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="583"/>
      <source>Move tip to selected feature</source>
      <translation>Przenieś czubek do wybranej cechy</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>Move an object</source>
      <translation>Przenieś obiekt</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="264"/>
      <source>Mirror</source>
      <translation>Odbicie lustrzane</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="301"/>
      <source>Linear Pattern</source>
      <translation>Wzór liniowy</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="345"/>
      <source>Polar Pattern</source>
      <translation>Układ promieniowy</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="382"/>
      <source>Scale</source>
      <translation>Skala</translation>
    </message>
  </context>
  <context>
    <name>FeaturePickDialog</name>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="42"/>
      <source>Valid</source>
      <translation>Poprawny</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="43"/>
      <source>Invalid shape</source>
      <translation>Nieodpowiedni kształt</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>Nie znaleziono polilinii w szkicu</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>Szkic jest już używany przez inną cechę</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another body feature</source>
      <translation>Szkic należy do innej cechy obiektu zawartości</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the tip of the body</source>
      <translation>Cecha jest umieszczona za czubkiem</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>Płaszczyzna podstawowa</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation>Narzędzia ścian</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation>Narzędzia do pracy z krawędziami</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation>Narzędzia operacji logicznych</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation>Narzędzia geometrii pomocniczej</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation>Narzędzia do modelowania</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>Utwórz geometrię</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>Parametry ewolwenty</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>Liczba zębów</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>Moduł</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>Kąt przyporu</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>Wysoka precyzja</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Tak</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Nie</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>Zębatka zewnętrzna</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>Współczynnik wysokości głowy zęba</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>Współczynnik wysokości stopy zęba</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>Współczynnik zaokrąglenia u podstawy zęba</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>Współczynnik przesunięcia zarysu</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Wymagana jest aktywna zawartość</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>Aby utworzyć nowy obiekt w środowisku Projekt Części, w dokumencie musi być aktywna zawartość.
Wybierz zawartość z poniższej listy lub utwórz nową.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>Utwórz nową zawartość</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
      <source>Please select</source>
      <translation>Proszę wybrać</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Pierwotna bryła geometryczna</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Kąt w pierwszym kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Kąt w kierunku drugim</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>Długość</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>Szerokość</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>Wysokość</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>Promień</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>Kąt obrotu</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>Promień 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>Promień 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>Parametr U</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>Parametry V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Promień w lokalnym kierunku osi Z</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>Promień w kierunku lokalnej osi X</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>Promień 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>Promień w kierunku lokalnej osi Y
Jeśli wartość wynosi zero, jest równa wartości promienia 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>Parametr V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>Promień zawarty w lokalnej płaszczyźnie XY</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>Promień zawarty w lokalnej płaszczyźnie XZ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Wielokąt</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>Promień okręgu opisanego</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X min / max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y min / max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z min / max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 min / max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 min / max</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Nachylenie</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>Układ współrzędnych</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Skok</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>Liczba obrotów</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>Kąt 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>Kąt 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>Przez trzy punkty</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>Główny promień</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>Mniejszy promień</translation>
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
      <translation>Prawoskrętny</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Lewoskrętny</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Punkt początkowy</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Punkt końcowy</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Odniesienie</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Wybrano geometrie, które nie są częścią aktywnej zawartości. Proszę określić, jak należy postępować z tymi zaznaczeniami. Jeśli nie chcesz tych odwołań, anuluj polecenie.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Wykonaj niezależną kopię (zalecane)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Utwórz zależną kopię</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Utwórz dowiązanie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="274"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Wybranie tej opcji spowoduje powstanie zależności kołowej.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>Dodaj zawartość</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>Usuń zawartość</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Scal</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Wytnij</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Przecięcie</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="54"/>
      <source>Boolean Parameters</source>
      <translation>Parametry operacji logicznej</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="87"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="51"/>
      <source>Primitive Parameters</source>
      <translation>Parametry bryły pierwotnej</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="920"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="926"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <source>Invalid wedge parameters</source>
      <translation>Nieprawidłowe parametry klina</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="921"/>
      <source>X min must not be equal to X max!</source>
      <translation>Wartość minimalna X nie może być równa wartości maksymalnej X!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="927"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Wartość minimalna Y nie może być równa wartości maksymalnej Y!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Wartość minimalna Z nie może być równa wartości maksymalnej Z!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="971"/>
      <source>Create primitive</source>
      <translation>Utwórz bryłę pierwotną</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Przełącza pomiędzy trybem wyboru i podglądu</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- wybierz element, aby go podświetlić
- kliknij dwukrotnie na element, aby zobaczyć podgląd sfazowania</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>Wymiary równe</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>Dwa wymiary</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>Wymiar i kąt</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>Odwraca kierunek</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>Użyj wszystkich krawędzi</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>Rozmiar</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>Rozmiar 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="347"/>
      <source>Empty chamfer created!
</source>
      <translation>Fazowanie nie zawiera geometrii!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="388"/>
      <source>Empty body list</source>
      <translation>Lista zawartości jest pusta</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="389"/>
      <source>The body list cannot be empty</source>
      <translation>Lista zawartości nie może być pusta</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="403"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Funkcja logiczna: Akceptuj: Błąd danych wejściowych</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="101"/>
      <source>Incompatible Reference Set</source>
      <translation>Niekompatybilny zestaw odniesień</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="102"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Nie ma trybu dołączania, który pasowałby do obecnego zestawu odniesień. Jeśli zdecydujesz się kontynuować, właściwość pozostanie tam, gdzie jest teraz i nie zostanie przeniesiona, gdy referencje się zmienią. Kontynuować?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="198"/>
      <source>Input error</source>
      <translation>Błąd danych wejściowych</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="407"/>
      <source>Input error</source>
      <translation>Błąd danych wejściowych
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Przełącza pomiędzy trybem wyboru i podglądu</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- wybierz element, aby go podświetlić
- kliknij dwukrotnie na element, aby zobaczyć podgląd pochylenia</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Kąt początkowy</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>Płaszczyzna neutralna</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>Kierunek wyciągania</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Odwróć kierunek wyciągnięcia</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="289"/>
      <source>Empty draft created!
</source>
      <translation>Utworzono pochylenie bez geometrii!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="271"/>
      <source>Select</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="276"/>
      <source>Confirm Selection</source>
      <translation>Potwierdź wybór</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="288"/>
      <source>Add All Edges</source>
      <translation>Dodaj wszystkie krawędzie</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>Dodaje wszystkie krawędzie do listy (tylko w trybie dodawania zaznaczenia)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="301"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>No face selected</source>
      <translation>Nie zaznaczono ściany</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="163"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1140"/>
      <source>Face</source>
      <translation>Powierzchnia</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="351"/>
      <source>Preview</source>
      <translation>Podgląd</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="355"/>
      <source>Select Faces</source>
      <translation>Wybierz ściany</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="690"/>
      <source>Select reference…</source>
      <translation>Wybierz odniesienie …</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="598"/>
      <source>No shape selected</source>
      <translation>Brak wybranych kształtów</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="683"/>
      <source>Sketch normal</source>
      <translation>Wektor normalny szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="686"/>
      <source>Face normal</source>
      <translation>Wektor normalny ściany</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="694"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>Kierunek niestandardowy</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1086"/>
      <source>Click on a shape in the model</source>
      <translation>Kliknij kształt modelu</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1347"/>
      <source>One sided</source>
      <translation>W jednym kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1348"/>
      <source>Two sided</source>
      <translation>W dwóch kierunkach</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1349"/>
      <source>Symmetric</source>
      <translation>Symetrycznie</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1355"/>
      <source>Click on a face in the model</source>
      <translation>Kliknij ścianę modelu</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Zezwalaj na używane cechy</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>Zezwalaj na cechy zewnętrzne</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>Z innych zawartości tej samej bryły</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>Z różnych części lub swobodnych cech</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Wykonaj niezależną kopię (zalecane)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Utwórz zależną kopię</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Utwórz dowiązanie</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Valid</source>
      <translation>Poprawny</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Invalid shape</source>
      <translation>Nieodpowiedni kształt</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>No wire in sketch</source>
      <translation>Nie znaleziono polilinii w szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Sketch already used by other feature</source>
      <translation>Szkic jest już używany przez inną cechę</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another body</source>
      <translation>Należy do innego kontenera</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Belongs to another part</source>
      <translation>Należy do innej części</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Doesn't belong to any body</source>
      <translation>Nie należy do żadnej zawartości</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Base plane</source>
      <translation>Płaszczyzna podstawowa</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="85"/>
      <source>Feature is located after the tip of the body</source>
      <translation>Cecha jest umieszczona za czubkiem</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Select attachment</source>
      <translation>Wybierz dołączenie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Przełącza pomiędzy trybem wyboru i podglądu</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- wybierz element, aby go podświetlić
- kliknij dwukrotnie na element, aby zobaczyć podgląd zaokrąglenia</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>Promień</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>Użyj wszystkich krawędzi</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="205"/>
      <source>Empty fillet created!</source>
      <translation>Utworzono puste zaokrąglenie!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Poprawny</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Base X-axis</source>
      <translation>Bazowa oś X</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="244"/>
      <source>Base Y-axis</source>
      <translation>Bazowa oś Y</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="245"/>
      <source>Base Z-axis</source>
      <translation>Bazowa oś Z</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="227"/>
      <source>Horizontal sketch axis</source>
      <translation>Pozioma oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="226"/>
      <source>Vertical sketch axis</source>
      <translation>Pionowa oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Normal sketch axis</source>
      <translation>Oś normalna do szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>Aktualny status</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>Oś</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="210"/>
      <source>Select reference…</source>
      <translation>Wybierz odniesienie …</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>Tryb</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Skok - wysokość - kąt</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Skok - liczba obrotów - kąt</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Wysokość - liczba obrotów - kąt</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Wysokość - liczba obrotów - przyrost</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>Nachylenie</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>Wysokość</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>Obroty</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>Kąt stożka</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>Przyrost promieniowy</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>Przelicz po zmianie</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Lewostronnie</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Odwrócony</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>Usuń za obrębem profilu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="60"/>
      <source>Helix Parameters</source>
      <translation>Parametry helisy</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="229"/>
      <source>Construction line %1</source>
      <translation>Linia konstrukcyjna %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="295"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Ostrzeżenie: helisa może się przecinać</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="300"/>
      <source>Error: helix touches itself</source>
      <translation>Błąd: helisa dotyka sama siebie</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="349"/>
      <source>Error: unsupported mode</source>
      <translation>Błąd: nieobsługiwany tryb</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Counterbore</source>
      <translation>Pogłębienie walcowe</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="54"/>
      <source>Countersink</source>
      <translation>Pogłębienie stożkowe</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterdrill</source>
      <translation>Nawiercenie stożkowe</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Hole Parameters</source>
      <translation>Parametry otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="69"/>
      <source>None</source>
      <translation>Brak</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>ISO metric regular</source>
      <translation>Gwint ISO metryczny standardowy</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric fine</source>
      <translation>Gwint ISO metryczny drobny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>UTS coarse</source>
      <translation>Gwint UTC zgrubny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS fine</source>
      <translation>Gwint UTS drobny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS extra fine</source>
      <translation>Gwint UTS bardzo drobny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>ANSI pipes</source>
      <translation>Rury ANSI</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ISO/BSP pipes</source>
      <translation>Rury ISO/BSP</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>BSW whitworth</source>
      <translation>Gwint BSW whitworth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSF whitworth fine</source>
      <translation>Gwint drobny BSF whitworth</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>ISO tyre valves</source>
      <translation>Zawory opon według normy ISO</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="673"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Średnie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="674"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Drobno</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="675"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Zgrubnie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Normalny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="679"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Dokładne</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="680"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Pasowanie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="683"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Normalny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="684"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Zamknij</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="685"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Zgrubne</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Powierzchnia prostokreślna</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Zamknięty</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Obiekt</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Dodaj profil sekcji</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Usuń sekcję profilu</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>Lista może zostać uporządkowana poprzez przeciąganie</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation>Przelicz po zmianie</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="51"/>
      <source>Loft Parameters</source>
      <translation>Parametry wyciągnięcia przez profile</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>Płaszczyzna</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="175"/>
      <source>Error</source>
      <translation>Błąd</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>Transformacje</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="70"/>
      <source>Edit</source>
      <translation>Edytuj</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="76"/>
      <source>Delete</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="82"/>
      <source>Add Mirror Transformation</source>
      <translation>Dodaj transformację odbicia lustrzanego</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="88"/>
      <source>Add Linear Pattern</source>
      <translation>Dodaj transformację szyku liniowego</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="94"/>
      <source>Add Polar Pattern</source>
      <translation>Dodaj transformację szyku promieniowego</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="100"/>
      <source>Add Scale Transformation</source>
      <translation>Dodaj transformację zmiany skali</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="106"/>
      <source>Move Up</source>
      <translation>Przesuń w górę</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="109"/>
      <source>Move Down</source>
      <translation>Przesuń w dół</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="143"/>
      <source>Right-click to add a transformation</source>
      <translation>Kliknij prawym przyciskiem myszki, aby dodać transformację</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation>Parametry wyciągnięcia</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>Przesuń wypust od ściany, na której wypust zakończy się po stronie 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>Przesuń wypust od ściany, na której wypust zakończy się po stronie 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>Odwróć kierunek wyciągnięcia</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>Do ostatniego</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>Do pierwszego</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Do powierzchni</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>Do kształtu</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>Długość</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>Przesunięcie do ściany</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>Wybierz wszystkie ściany</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>Wybierz</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation>Wybierz ścianę</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>Strona 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>Kierunek</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="527"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Ustaw kierunek lub wybierz krawędź
z modelu jako odniesienie</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="532"/>
      <source>Sketch normal</source>
      <translation>Wektor normalny szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="542"/>
      <source>Custom direction</source>
      <translation>Kierunek niestandardowy</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="552"/>
      <source>Show direction</source>
      <translation>Pokaż kierunek</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="562"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Użyj wybranego wektora dla kierunku wyciągnięcia, w przeciwnym razie
zostanie użyty wektor normalny płaszczyzny szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="671"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Jeśli opcja nie jest zaznaczona, długość będzie
mierzona wzdłuż podanego kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="675"/>
      <source>Length along sketch normal</source>
      <translation>Długość wzdłuż wektora normalnego szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Przełącza pomiędzy trybem wyboru i podglądu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>Odwrócony</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="520"/>
      <source>Direction/edge</source>
      <translation>Kierunek / krawędź</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="537"/>
      <source>Select reference…</source>
      <translation>Wybierz odniesienie …</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="575"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X-component of direction vector</source>
      <translation>Składowa X wektora kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="604"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y-component of direction vector</source>
      <translation>Składowa Y wektora kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="633"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z-component of direction vector</source>
      <translation>Składowa Z wektora kierunku</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>Kąt zwężenia wyciągnięcia</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>Tryb</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation>Strona 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>Kąt zwężenia</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>Wybierz kształt</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>Wybiera wszystkie powierzchnie kształtu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="685"/>
      <source>Recompute on change</source>
      <translation>Przelicz po zmianie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Tryb orientacji</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Standardowe</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Stały</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Wektor Freneta</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Pomocniczy</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Binormalna</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Krzywoliniowa równoważność</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Obiekt</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Dodaj krawędź</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Usuń krawędź</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Ustaw stały binormalny wektor używany do obliczenia orientacji profilu</translation>
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
      <translation>Kierunek przekroju</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="608"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Obiekt</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>Przejście narożnika</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>Prawy narożnik</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>Zaokrąglaj narożniki</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>Ścieżka do przeciągnięcia</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Dodaj krawędź</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>Usuń krawędź</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Przekształcony</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe Parameters</source>
      <translation>Parametry rury</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="89"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="448"/>
      <location filename="../../TaskPipeParameters.cpp" line="560"/>
      <source>Input error</source>
      <translation>Błąd danych wejściowych</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="448"/>
      <source>No active body</source>
      <translation>Brak aktywnej zawartości</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Tryb transformacji</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Stały</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Sekcja wielokrotna</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Dodaj profil sekcji</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Usuń sekcję profilu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>Lista może zostać uporządkowana poprzez przeciąganie</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="879"/>
      <source>Section Transformation</source>
      <translation>Przekształcenie przekroju</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="896"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation>Parametry kieszeni</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="42"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>Przesunięcie od wybranej powierzchni, na której kieszeń zakończy się po stronie 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>Przesunięcie od wybranej powierzchni, na której kieszeń zakończy się po stronie 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation>Odwraca kierunek kieszeni</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Through all</source>
      <translation>Przez wszystkie</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>Do pierwszego</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Do powierzchni</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>Do kształtu</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="193"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Base X-axis</source>
      <translation>Bazowa oś X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Base Y-axis</source>
      <translation>Bazowa oś Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="242"/>
      <source>Base Z-axis</source>
      <translation>Bazowa oś Z</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>Pozioma oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>Pionowa oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>Symetrycznie do płaszczyzny</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>Odwrócony</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>Drugi kąt:</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>Oś</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="249"/>
      <source>Select reference…</source>
      <translation>Wybierz odniesienie …</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="157"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="475"/>
      <source>Face</source>
      <translation>Ściana</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation>Przelicz po zmianie</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="195"/>
      <source>To last</source>
      <translation>Do ostatniego</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="198"/>
      <source>Through all</source>
      <translation>Przez wszystkie</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="200"/>
      <source>To first</source>
      <translation>Do pierwszego</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="201"/>
      <source>Up to face</source>
      <translation>Do powierzchni</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Two dimensions</source>
      <translation>Dwa wymiary</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="462"/>
      <source>No face selected</source>
      <translation>Nie zaznaczono ściany</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>Współczynnik</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>Wystąpienia</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Obiekt</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Dodaj geometrię</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Usuń geometrię</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation>Parametry łącznika kształtu</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="131"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="189"/>
      <source>Face</source>
      <translation>Powierzchnia</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Przełącza pomiędzy trybem wyboru i podglądu</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Wybierz</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- wybierz element, aby go podświetlić
- kliknij dwukrotnie na element, aby zobaczyć cechy</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>Grubość</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>Tryb</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>Powłoka</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Rura</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>Obie strony</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>Typ dołączenia</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>Łuk</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>Przecięcie</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>Stwórz grubość do wewnątrz</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="269"/>
      <source>Empty thickness created!
</source>
      <translation>Utworzono pustą cechę grubości !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="105"/>
      <source>Remove</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>Oś normalna do szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>Pionowa oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>Pozioma oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="443"/>
      <source>Construction line %1</source>
      <translation>Linia konstrukcyjna %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation>Bazowa oś X</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation>Bazowa oś Y</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation>Bazowa oś Z</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base XY-plane</source>
      <translation>Bazowa płaszczyzna XY</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base YZ-plane</source>
      <translation>Bazowa płaszczyzna YZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="459"/>
      <source>Base XZ-plane</source>
      <translation>Bazowa płaszczyzna XZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="467"/>
      <source>Select reference…</source>
      <translation>Wybierz odniesienie …</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>Przekształć Zawartość</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>Przekształć kształty narzędzi</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>Dodaj cechę</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>Usuń cechę</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation>Przelicz po zmianie</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>Lista może zostać uporządkowana poprzez przeciąganie</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="744"/>
      <source>Select Body</source>
      <translation>Wybierz zawartość</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="745"/>
      <source>Select a body from the list</source>
      <translation>Wybierz zawartość z listy</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="898"/>
      <source>Move Feature After…</source>
      <translation>Przenieś cechę za …</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="899"/>
      <source>Select a feature from the list</source>
      <translation>Wybierz cechę z listy</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="973"/>
      <source>Move Tip</source>
      <translation>Przenieś czubek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="975"/>
      <source>Set tip to last feature?</source>
      <translation>Ustawić czubek na ostatnią cechę?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="974"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>Przeniesiony element pojawia się za aktualnie ustawionym czubkiem.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>Invalid selection</source>
      <translation>Nieprawidłowy wybór</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="140"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Nie ma trybów dołączania, które pasują do wybranych obiektów. Wybierz coś innego.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <location filename="../../Command.cpp" line="149"/>
      <location filename="../../Command.cpp" line="151"/>
      <source>Error</source>
      <translation>Błąd</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="775"/>
      <source>Several sub-elements selected</source>
      <translation>Wybrano kilka podelementów</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="776"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>Zaznacz jedną ścianę jako podparcie dla szkicu!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="780"/>
      <source>Select a face as support for a sketch!</source>
      <translation>Wybierz ścianę jako podparcie dla szkicu!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="784"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>Do szkicu wymagana jest ściana płaska jako podparcie!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="788"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>Najpierw utwórz płaszczyznę lub wybierz ścianę, na której chcesz szkicować.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="779"/>
      <source>No support face selected</source>
      <translation>Nie wybrano ściany bazowej</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="783"/>
      <source>No planar support</source>
      <translation>Brak płaskiej powierzchni</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="787"/>
      <source>No valid planes in this document</source>
      <translation>Brak prawidłowej płaszczyzny w tym dokumencie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1014"/>
      <location filename="../../SketchWorkflow.cpp" line="702"/>
      <location filename="../../ViewProvider.cpp" line="139"/>
      <location filename="../../ViewProviderDatum.cpp" line="250"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Okno dialogowe jest już otwarte w panelu zadań</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="894"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Nie można użyć tego polecenia, ponieważ nie ma bryły do odjęcia.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="895"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Upewnij się, że zawartość posiada cechę, zanim spróbujesz wykonać polecenie odejmowania.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="916"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Nie można użyć wybranego obiektu. Wybrany obiekt musi należeć do aktywnej zawartości</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>Nie ma aktywnej zawartości. Aktywuj zawartość przed wstawieniem elementu bazowego.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>Sub-shape binder</source>
      <translation>Łącznik kształtów podrzędnych</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="939"/>
      <source>No sketch to work on</source>
      <translation>Brak szkicu do pracy</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="940"/>
      <source>No sketch is available in the document</source>
      <translation>Szkic nie jest dostępny w dokumencie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1015"/>
      <location filename="../../SketchWorkflow.cpp" line="703"/>
      <location filename="../../ViewProvider.cpp" line="140"/>
      <location filename="../../ViewProviderDatum.cpp" line="251"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="97"/>
      <source>Close this dialog?</source>
      <translation>Zamknąć to okno dialogowe?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1633"/>
      <location filename="../../Command.cpp" line="1659"/>
      <source>Wrong selection</source>
      <translation>Nieprawidłowy wybór</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1634"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Wybierz krawędź, ścianę lub zawartość z pojedynczej zawartości.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1638"/>
      <location filename="../../Command.cpp" line="1970"/>
      <source>Selection is not in the active body</source>
      <translation>Wybór nie znajduje się w aktywnej zawartości</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>Shape of the selected part is empty</source>
      <translation>Kształt wybranej części nie został zdefiniowany</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1639"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Wybierz krawędź, ścianę lub zawartość z aktywnej zawartości.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="917"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>Rozważ użycie Łącznika Kształtu lub Cechy Podstawowej do odniesienia zewnętrznej geometrii w zawartości</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1649"/>
      <source>Wrong object type</source>
      <translation>Niewłaściwy typ obiektu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1650"/>
      <source>%1 works only on parts.</source>
      <translation>%1 działa tylko na częściach.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1971"/>
      <source>Please select only one feature in an active body.</source>
      <translation>Wybierz tylko jedną cechę w aktywnej zawartości.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="69"/>
      <source>Part creation failed</source>
      <translation>Utworzenie części nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="70"/>
      <source>Failed to create a part object.</source>
      <translation>Nie udało się utworzyć obiektu części.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="119"/>
      <location filename="../../CommandBody.cpp" line="124"/>
      <location filename="../../CommandBody.cpp" line="137"/>
      <location filename="../../CommandBody.cpp" line="186"/>
      <source>Bad base feature</source>
      <translation>Zła podstawowa funkcja</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="120"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>Zawartość nie może opierać się na cechach środowiska Projekt Części.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 już należy do zawartości, nie można go użyć jak podstawową funkcję dla innej zawartości.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="138"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>Podstawowa funkcja (%1) należy do innej części.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="162"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>Wybrany kształt składa się z wielu brył.
Może to prowadzić do nieoczekiwanych rezultatów.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="166"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>Wybrany kształt składa się z wielu powłok.
Może to prowadzić do nieoczekiwanych rezultatów.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="170"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>Wybrany kształt składa się tylko z powłoki.
Może to prowadzić do nieoczekiwanych rezultatów.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="174"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>Wybrany kształt składa się z wielu brył lub powłok.
Może to prowadzić do nieoczekiwanych rezultatów.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="179"/>
      <source>Base feature</source>
      <translation>Cecha podstawowa</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="187"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Zawartość nie może być oparta na więcej niż jednej funkcji.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="201"/>
      <source>Body</source>
      <translation>Zawartość</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="351"/>
      <source>Nothing to migrate</source>
      <translation>Nic do zaimportowania</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="564"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>Zaznacz dokładnie jedną cechę środowiska Projekt Części lub jedną zawartość.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="568"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>Nie można określić zawartości dla wybranej funkcji '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="718"/>
      <source>Only features of a single source body can be moved</source>
      <translation>Można przenosić tylko cechy jednej zawartości źródłowej</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="500"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Płaszczyzna szkicu nie może być przeniesiona</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="352"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>Nie znaleziono żadnych cech środowiska Projekt Części poza zawartością. 
Brak elementów do migracji.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="501"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Proszę edytować %1 i przedefiniuj go, aby używać płaszczyzny bazowej lub odniesienia jako płaszczyzny szkicu.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="563"/>
      <location filename="../../CommandBody.cpp" line="567"/>
      <location filename="../../CommandBody.cpp" line="572"/>
      <location filename="../../CommandBody.cpp" line="869"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <source>Selection error</source>
      <translation>Błąd w zaznaczeniu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="573"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Tylko funkcja bryły może być górą zawartości.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="695"/>
      <location filename="../../CommandBody.cpp" line="717"/>
      <location filename="../../CommandBody.cpp" line="732"/>
      <source>Features cannot be moved</source>
      <translation>Cechy nie mogą być przesuwane</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="696"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Niektóre z wybranych cech mają zależności w zawartości źródłowej</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="733"/>
      <source>There are no other bodies to move to</source>
      <translation>Nie istnieją inne zawartości, do których można przenieść cechę</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="870"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Niemożliwe jest przeniesienie podstawowej cechy zawartości.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Select one or more features from the same body.</source>
      <translation>Wybierz jedną lub więcej cech z tej samej zawartości.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="890"/>
      <source>Beginning of the body</source>
      <translation>Początek zawartości</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="959"/>
      <source>Dependency violation</source>
      <translation>Naruszenie warunków zależności</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="960"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Cecha początkowa nie może zależeć od cechy następnej.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="263"/>
      <source>No previous feature found</source>
      <translation>Nie znaleziono poprzedniego elementu</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Nie jest możliwe utworzenie elementu do odjęcia bez dostępnego elementu bazowego</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="226"/>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <source>Vertical sketch axis</source>
      <translation>Pionowa oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="227"/>
      <location filename="../../TaskTransformedParameters.cpp" line="441"/>
      <source>Horizontal sketch axis</source>
      <translation>Pozioma oś szkicu</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="229"/>
      <source>Construction line %1</source>
      <translation>Linia konstrukcyjna %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="80"/>
      <source>Face</source>
      <translation>Powierzchnia</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="187"/>
      <source>Active Body Required</source>
      <translation>Wymagana jest aktywna zawartość</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="140"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>Aby użyć środowiska Projekt Części, w dokumencie wymagany jest aktywna zawartość. Aktywuj zawartość (podwójne kliknięcie) lub utwórz nowe.

W przypadku starszych dokumentów z obiektami nieposiadającymi obiektu zawartości użyj funkcji migracji w środowisku Projekt Części, aby umieścić je w Zawartości.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="188"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>Aby utworzyć nowy obiekt środowiska Projekt Części, w dokumencie wymagana jest aktywna zawartość. 
Aktywuj istniejącą zawartość (podwójne kliknięcie) lub utwórz nową.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="222"/>
      <source>Feature is not in a body</source>
      <translation>Funkcja nie jest w korpusie</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="223"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Ta cecha musi przynależeć do zawartości w danym dokumencie, by można ją było wykorzystać.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="255"/>
      <source>Feature is not in a part</source>
      <translation>Funkcja nie jest w części</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="256"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Ta cecha musi przynależeć do obiektu części w danym dokumencie, by można ją było wykorzystać.</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="96"/>
      <location filename="../../ViewProviderDressUp.cpp" line="65"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="210"/>
      <location filename="../../ViewProviderTransformed.cpp" line="66"/>
      <source>Edit %1</source>
      <translation>Edytuj %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="109"/>
      <source>Set Face Colors</source>
      <translation>Ustaw kolory ścian</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="208"/>
      <source>Plane</source>
      <translation>Płaszczyzna</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="204"/>
      <source>Line</source>
      <translation>Linia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Point</source>
      <translation>Punkt</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>Układ współrzędnych</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="229"/>
      <source>Edit Datum</source>
      <translation>Edytuj odniesienie</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="89"/>
      <source>Feature error</source>
      <translation>Błąd funkcji</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="90"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1 nie ma cechy bazowej.
Ta cecha jest uszkodzona i nie można jej edytować.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="205"/>
      <source>Edit Shape Binder</source>
      <translation>Edytuj łącznik kształtu</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="317"/>
      <source>Synchronize</source>
      <translation>Synchronizuj</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="319"/>
      <source>Select Bound Object</source>
      <translation>Wybierz powiązany obiekt</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="140"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>Edytowany dokument "%1" został zaprojektowany w starszej wersji środowiska Projekt Części.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="143"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>Migrować w celu korzystania z nowoczesnych funkcji Projektowania Części?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="146"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>Dokument „%1” wydaje się być w trakcie procesu migracji z dawnego środowiska Projekt Części lub ma nieco uszkodzoną strukturę.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Make the migration automatically?</source>
      <translation>Czy dokonać migracji automatycznie?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="152"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Uwaga: Jeśli zdecydujesz się na migrację, nie będziesz mógł edytować pliku przy użyciu starszej wersji FreeCAD.
Jeśli odmówisz migracji, nie będziesz mógł używać nowych funkcji środowiska Projekt Części, takich jak Zawartość i Część. W rezultacie nie będziesz mógł również używać swoich części w środowisku Złożeń.
Migracja będzie możliwa w każdej chwili za pomocą "Projekt części -&gt; Migruj".</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate Manually</source>
      <translation>Migruj na żądanie</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="72"/>
      <source>Edit Boolean</source>
      <translation>Edytuj wynik działania funkcji logicznej</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="41"/>
      <source>Edit Chamfer</source>
      <translation>Edycja funkcji sfazowania</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="42"/>
      <source>Edit Draft</source>
      <translation>Edytuj wynik działania funkcji pochylenia ścian</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="41"/>
      <source>Edit Fillet</source>
      <translation>Edycja funkcji zaokrąglenia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="46"/>
      <source>Edit Groove</source>
      <translation>Edytuj rowek</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="51"/>
      <source>Edit Helix</source>
      <translation>Edytuj helisę</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="66"/>
      <source>Edit Hole</source>
      <translation>Edytuj otwór</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="39"/>
      <source>Edit Linear Pattern</source>
      <translation>Edycja funkcji szyku kołowego</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="66"/>
      <source>Edit Loft</source>
      <translation>Edytuj wyciągnięcie przez profile</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="39"/>
      <source>Edit Mirror</source>
      <translation>Edytuj odbicie lustrzane</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="48"/>
      <source>Edit Multi-Transform</source>
      <translation>Edycja funkcji transformacji wielokrotnej</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="46"/>
      <source>Edit Pad</source>
      <translation>Edytuj wyciągnięcie</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="74"/>
      <source>Edit Pipe</source>
      <translation>Edytuj rurę</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="48"/>
      <source>Edit Pocket</source>
      <translation>Edytuj kieszeń</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation>Edycja funkcji szyku kołowego</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="51"/>
      <source>Edit Primitive</source>
      <translation>Edytuj bryłę pierwotną</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="46"/>
      <source>Edit Revolution</source>
      <translation>Edytuj wyciągnięcie przez obrót</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="39"/>
      <source>Edit Scale</source>
      <translation>Edytuj skalę</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="41"/>
      <source>Edit Thickness</source>
      <translation>Edytuj grubość</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>Parametry koła łańcuchowego</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>Liczba zębów</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>Typ koła łańcuchowego</translation>
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
      <translation>Rowerowa z przerzutkami</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>Rowerowa bez przerzutek</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>Podziałka łańcucha</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>Średnica wałka łańcuchowego</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>Szerokość zębów</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="146"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="151"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="156"/>
      <source>ISO 606 10B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="161"/>
      <source>ISO 606 12B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="166"/>
      <source>ISO 606 16B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="171"/>
      <source>ISO 606 20B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="176"/>
      <source>ISO 606 24B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="181"/>
      <source>Motorcycle 420</source>
      <translation>Motocyklowy 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>Motocyklowy 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>Motocyklowy 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>Motocyklowy 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>Motocyklowy 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>Motocyklowy 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>Motocyklowy 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 w</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Aktualizacja zmian gwintu na żywo
Zauważ, że obliczenia mogą zająć trochę czasu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>Głębokość gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>Dostosuj pasowanie gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>Pasowanie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>Typ główki</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>Typ głębokości</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>Średnica główki</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>Głębokość główki</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation>Luz / przejście swobodne</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation>Wiertło do gwintowania</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation>Gwint modelowany</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation>Typ otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>Aktualizuj widok gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>Odstęp bezpieczeństwa niestandardowy</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Thread clearance value</source>
      <translation>Niestandardowa wartość pasowania gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Direction</source>
      <translation>Kierunek</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>Rozmiar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Pasowanie
Dostępne tylko dla otworów bez gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Standard</source>
      <translation>Standardowy</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="732"/>
      <source>Close</source>
      <translation>Zamknij</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="737"/>
      <source>Wide</source>
      <translation>Zgrubne</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Class</source>
      <translation>Klasa</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="835"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Klasa tolerancji dla otworów gwintowanych w zależności od profilu otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>Średnica</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>Średnica otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>Głębokość</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>Parametry otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>Typy profilu bazowego</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>Okręgi i łuki</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>Punkty, okręgi i łuki</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>Punkty</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>Przez wszystkie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>Niestandardowe wartości głowicy</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Kąt wiercenia</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Uwzględnij w głębokości</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>Zmień kierunek</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="662"/>
      <source>&lt;b&gt;Threading&lt;/b&gt;</source>
      <translation>&lt;b&gt;Gwintowanie&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="783"/>
      <source>Thread</source>
      <translation>Gwint</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="892"/>
      <source>&amp;Right hand</source>
      <translation>Prawoskrętny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="908"/>
      <source>&amp;Left hand</source>
      <translation>Lewoskrętny</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="957"/>
      <source>Thread Depth Type</source>
      <translation>Typ głębokości gwintu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="971"/>
      <source>Hole depth</source>
      <translation>Głębokość otworu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="981"/>
      <source>Tapped (DIN76)</source>
      <translation>Gwintowany (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>Typ nacięcia dla łbów śrub</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Zaznacz opcję, aby nadpisać wartości predefiniowane przez "Typ"</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>W przypadku pogłębienia stożkowego jest to głębokość
wierzchołka śruby poniżej powierzchni</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>Kąt pogłębienia stożkowego</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>Wielkość wierzchołka wiertła będzie brana pod uwagę
dla głębokości otworów nieprzelotowych</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>Stożkowy</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Kąt stożka dla otworu
90°: otwór prosty
poniżej 90°: mniejszy promień otworu u dołu
powyżej 90°: większy promień otworu u dołu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>Odwraca kierunek otworu</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Brak wiadomości</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Szkic</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Projekt Części</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>Odniesienia</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation>Funkcje przyrostowe</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation>Funkcje ubytków</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation>Funkcje wykończeniowe</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation>Funkcje transformacji</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation>Koło łańcuchowe</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation>Koło zębate ewolwentowe</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation>Kreator projektowania wału</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Measure</source>
      <translation>Pomiary</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Refresh</source>
      <translation>Odśwież pomiary</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Toggle 3D</source>
      <translation>Włącz / wyłącz widok pomiarów 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Helper</source>
      <translation>Projekt Części — pomocnik</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Modeling</source>
      <translation>Projekt Części — modelowanie</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Length [mm]</source>
      <translation>Długość [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Diameter [mm]</source>
      <translation>Średnica [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Inner diameter [mm]</source>
      <translation>Średnica wewnętrzna [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Constraint type</source>
      <translation>Typ obciążenia</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Start edge type</source>
      <translation>Typ krawędzi początkowej</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge size</source>
      <translation>Rozmiar krawędzi początkowej</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>End edge type</source>
      <translation>Typ krawędzi końcowej</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>Rozmiar krawędzi końcowej</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="64"/>
      <source>Shaft Wizard</source>
      <translation>Kreator wału</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 1</source>
      <translation>Przekrój 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 2</source>
      <translation>Przekrój 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Add column</source>
      <translation>Dodaj kolumnę</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation>Przekrój %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="150"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="165"/>
      <source>None</source>
      <translation>Brak</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="151"/>
      <source>Fixed</source>
      <translation>Stały</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="152"/>
      <source>Force</source>
      <translation>Siła</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="153"/>
      <source>Bearing</source>
      <translation>Łożysko</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="154"/>
      <source>Gear</source>
      <translation>Zębatka</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="155"/>
      <source>Pulley</source>
      <translation>Koło pasowe</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="166"/>
      <source>Chamfer</source>
      <translation>Sfazowanie</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="167"/>
      <source>Fillet</source>
      <translation>Zaokrąglenie</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="57"/>
      <source>All</source>
      <translation>Wszystkie</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="104"/>
      <source>Missing Module</source>
      <translation>Brakujący moduł</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>Wtyczka Plot nie jest zainstalowana. 
Zainstaluj ją, aby włączyć tę funkcję.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="214"/>
      <source>Shaft design wizard...</source>
      <translation>Kreator projektowania wału ...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="215"/>
      <source>Start the shaft design wizard</source>
      <translation>Uruchom kreatora projektowania wału</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="396"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>Powiązany obiekt nie ma cech środowiska Projekt Części</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Tip shape is empty</source>
      <translation>Kształt czubka nie został zdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="64"/>
      <source>BaseFeature link is not set</source>
      <translation>Odnośnik Cechy Podstawowej nie jest ustawiony</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="69"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>Cecha Podstawowa musi być Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="75"/>
      <source>BaseFeature has an empty shape</source>
      <translation>Kształt Cechy Podstawowej nie został zdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="78"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Nie można przeprowadzić cięcia logicznego bez Cechy Podstawowej</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="121"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Nie można przeprowadzić operacji logicznej z niczym oprócz Part::Feature i jej pochodnych</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="99"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Nie można przeprowadzić operacji logicznej z nieprawidłowym kształtem podstawowym</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="105"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation>Nie można przeprowadzić operacji logicznej na cesze, która nie jest w zawartości</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="131"/>
      <source>Base shape is null</source>
      <translation>Kształt podstawowy nie został zdefiniowany</translation>
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
      <translation>Wynik zawiera wiele brył: włącz opcję "Zezwalaj na złożenia" w aktywnej zawartości.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="112"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="134"/>
      <source>Tool shape is null</source>
      <translation>Kształt narzędzia nie został zdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="150"/>
      <source>Unsupported boolean operation</source>
      <translation>Nieobsługiwana operacja logiczna.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="327"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>Nie można utworzyć wyciągnięcia o całkowitej długości równej zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="332"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>Nie można utworzyć kieszeni o całkowitej długości zero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="514"/>
      <source>No extrusion geometry was generated.</source>
      <translation>Nie wygenerowano geometrii do wyciągnięcia.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="534"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>Wynikowy proces scalania przez wytłaczanie nie zawiera objętości.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="568"/>
      <location filename="../../../App/FeatureLoft.cpp" line="306"/>
      <location filename="../../../App/FeaturePipe.cpp" line="401"/>
      <location filename="../../../App/FeaturePipe.cpp" line="422"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="130"/>
      <source>Resulting shape is not a solid</source>
      <translation>Otrzymany kształt nie jest bryłą</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="157"/>
      <source>Failed to create chamfer</source>
      <translation>Nie udało się utworzyć sfazowania</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="287"/>
      <location filename="../../../App/FeatureFillet.cpp" line="102"/>
      <source>Resulting shape is null</source>
      <translation>Kształt wynikowy jest niezdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="128"/>
      <source>No edges specified</source>
      <translation>Nie określono krawędzi</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="241"/>
      <source>Size must be greater than zero</source>
      <translation>Rozmiar musi być większy niż zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="250"/>
      <source>Size2 must be greater than zero</source>
      <translation>Rozmiar2 musi być większy niż zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="255"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>Kąt musi być większy niż 0 i mniejszy niż 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="85"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>Zaokrąglenie nie jest możliwe do wykonania na wybranych kształtach.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="92"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Promień zaokrąglenia musi być większy niż zero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="89"/>
      <source>Angle of groove too large</source>
      <translation>Kąt rowka zbyt duży</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="93"/>
      <source>Angle of groove too small</source>
      <translation>Kąt rowka zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="112"/>
      <location filename="../../../App/FeatureHole.cpp" line="1904"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>Nie można utworzyć żądanej cechy. Powodem może być:
  - aktywna Zawartość nie zawiera podstawowego kształtu, więc nie ma
  materiału do usunięcia;
  - wybrany szkic nie należy do aktywnej zawartości.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="373"/>
      <source>Failed to obtain profile shape</source>
      <translation>Nie udało się uzyskać kształtu profilu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="425"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Tworzenie nie powiodło się, ponieważ kierunek jest prostopadły do wektora normalnego szkicu.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="447"/>
      <location filename="../../../App/FeatureGroove.cpp" line="129"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Creating a face from sketch failed</source>
      <translation>Tworzenie ściany ze szkicu nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="151"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="155"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>Oś obrotu przecina szkic</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="159"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="240"/>
      <source>Could not revolve the sketch!</source>
      <translation>Nie można obrócić szkicu!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="205"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="252"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Nie można utworzyć ściany ze szkicu.
Przecinające się obiekty w szkicu są niedozwolone.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="137"/>
      <source>Error: Pitch too small!</source>
      <translation>Błąd: zbyt mała podziałka!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="139"/>
      <location filename="../../../App/FeatureHelix.cpp" line="153"/>
      <source>Error: height too small!</source>
      <translation>Błąd: zbyt mała wysokość!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="145"/>
      <source>Error: pitch too small!</source>
      <translation>Błąd: zbyt mały skok!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="147"/>
      <location filename="../../../App/FeatureHelix.cpp" line="155"/>
      <location filename="../../../App/FeatureHelix.cpp" line="161"/>
      <source>Error: turns too small!</source>
      <translation>Błąd: zbyt mało obrotów!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Błąd: ani wysokość, ani wzrost nie mogą być zerowe!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="179"/>
      <source>Error: unsupported mode</source>
      <translation>Błąd: nieobsługiwany tryb</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="191"/>
      <source>Error: No valid sketch or face</source>
      <translation>Błąd: brak prawidłowego szkicu lub ściany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="200"/>
      <source>Error: Face must be planar</source>
      <translation>Błąd: ściana musi być płaska</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="299"/>
      <location filename="../../../App/FeatureHelix.cpp" line="331"/>
      <location filename="../../../App/FeatureHole.cpp" line="2529"/>
      <source>Error: Result is not a solid</source>
      <translation>Błąd: wynik nie jest bryłą</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="275"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Błąd: nie ma nic do odjęcia</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="279"/>
      <location filename="../../../App/FeatureHelix.cpp" line="303"/>
      <location filename="../../../App/FeatureHelix.cpp" line="334"/>
      <source>Error: Result has multiple solids</source>
      <translation>Błąd: otrzymano wynik z wieloma bryłami</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="292"/>
      <source>Error: Adding the helix failed</source>
      <translation>Błąd: dodanie helisy nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="318"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>Błąd: samo przecięcie helisy nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="325"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>Błąd: odjęcie helisy nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="348"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Błąd: Nie można utworzyć ściany ze szkicu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1419"/>
      <source>Thread type is invalid</source>
      <translation>Typ gwintu jest nieprawidłowy</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1944"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Błąd otworu: Nieobsługiwana specyfikacja długości</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1947"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Błąd otworu: Nieprawidłowa głębokość otworu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1970"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Błąd otworu: nieprawidłowy kąt zwężenia</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1991"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Błąd otworu: zbyt mała średnica wycięcia otworu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1995"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Błąd otworu: głębokość wycięcia otworu musi być mniejsza niż głębokość otworu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1999"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Błąd otworu: głębokość wycięcia otworu musi być większa lub równa zeru</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2021"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Błąd otworu: Nieprawidłowe pogłębienie</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Błąd otworu: nieprawidłowy kąt punktu wiercenia</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2064"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Błąd otworu: Nieprawidłowy punkt wiercenia</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2098"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Błąd otworu: Nie można obrócić szkicu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2102"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Błąd otworu: Kształt wynikowy jest niezdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2112"/>
      <source>Error: Adding the thread failed</source>
      <translation>Błąd: Dodanie gwintu nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2163"/>
      <location filename="../../../App/FeatureHole.cpp" line="2168"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Operacja logiczna na profilu krawędzi nie powiodła się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2174"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>Operacja logiczna doprowadziła do powstania elementów nietrwałych (non-solid) na profilu krawędzi</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="156"/>
      <source>Boolean operation failed</source>
      <translation>Operacja logiczna nie powiodła się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2195"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Nie można utworzyć ściany ze szkicu.
Przecinające się elementy szkicu lub wiele ścian w szkicu nie są dozwolone przy tworzeniu kieszeni aż do powierzchni.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2345"/>
      <source>Thread type out of range</source>
      <translation>Typ gwintu poza zakresem</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2348"/>
      <source>Thread size out of range</source>
      <translation>Rozmiar gwintu poza zakresem</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2504"/>
      <source>Error: Thread could not be built</source>
      <translation>Błąd: Nie można zbudować gwintu</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="170"/>
      <source>Loft: At least one section is needed</source>
      <translation>Wyciągnięcie przez profile: Wymagany jest co najmniej jeden przekrój</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="325"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Wyciągnięcie przez profile: Wystąpił krytyczny błąd podczas tworzenia wyciągnięcia przez profile</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="207"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Wyciągnięcie przez profile: Tworzenie ściany ze szkicu nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="256"/>
      <source>Loft: Failed to create shell</source>
      <translation>Wyciągnięcie przez profile: Nie udało się utworzyć powłoki</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="611"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Nie można utworzyć ściany ze szkicu.
Przecinające się elementy szkicu lub wiele ścian w szkicu nie są dozwolone.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="178"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: Nie można uzyskać kształtu profilu</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="183"/>
      <source>No spine linked</source>
      <translation>Brak powiązanych krzywych prowadzących</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="196"/>
      <source>No auxiliary spine linked.</source>
      <translation>Brak powiązanych pomocniczych krzywych prowadzących.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="217"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: tylko jeden odizolowany punkt jest wymagany gdy używa się szkicu z odizolowanymi punktami dla przekroju</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="223"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: co najmniej jeden przekrój jest wymagany gdy używa się pojedynczego punktu dla profilu</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="237"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>Rura: Wszystkie przekroje muszą być cechami środowiska Część</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="243"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: Nie można uzyskać kształtu przekroju</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="252"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: Tylko profil i ostatni przekrój mogą być wierzchołkami</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="261"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Przekroje wielokrotne muszą mieć taką samą liczbę wewnętrznych polilinii jak bazowy przekrój</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="288"/>
      <source>Path must not be a null shape</source>
      <translation>Ścieżka nie może być pustym kształtem</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="323"/>
      <source>Pipe could not be built</source>
      <translation>Wyciągnięcie wzdłuż ścieżki nie może zostać zbudowane</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="368"/>
      <source>Result is not a solid</source>
      <translation>Wynik nie jest bryłą</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="383"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Wyciągnięcie wzdłuż ścieżki: Nie ma od czego odjąć</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="396"/>
      <source>Adding the pipe failed</source>
      <translation>Dodawanie wyciągnięcia wzdłuż ścieżki nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="417"/>
      <source>Subtracting the pipe failed</source>
      <translation>Odejmowanie wyciągnięcia wzdłuż ścieżki nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="442"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>Wystąpił błąd krytyczny podczas tworzenia wyciągnięcia wzdłuż ścieżki</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="565"/>
      <source>Invalid element in spine.</source>
      <translation>Nieprawidłowy element w krzywej prowadzącej.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="568"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Element w krzywej prowadzącej nie jest ani krawędzią ani polilinią.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="581"/>
      <source>Spine is not connected.</source>
      <translation>Krzywa prowadząca nie jest połączona.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="585"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>Krzywa prowadząca nie jest ani krawędzią, ani polilinią.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="589"/>
      <source>Invalid spine.</source>
      <translation>Nieprawidłowa krzywa prowadząca.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="98"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Nie można odjąć cechy prymitywu bez cechy podstawowej</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="295"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="116"/>
      <source>Unknown operation type</source>
      <translation>Nieznany typ operacji</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Failed to perform boolean operation</source>
      <translation>Nie udało się wykonać operacji logicznej</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="204"/>
      <source>Length of box too small</source>
      <translation>Długość prostopadłościanu jest zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="206"/>
      <source>Width of box too small</source>
      <translation>Szerokość prostopadłościanu jest zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="208"/>
      <source>Height of box too small</source>
      <translation>Wysokość prostopadłościanu zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="254"/>
      <source>Radius of cylinder too small</source>
      <translation>Promień walca zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="256"/>
      <source>Height of cylinder too small</source>
      <translation>Wysokość walca zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="258"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Kąt obrotu walca zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="311"/>
      <source>Radius of sphere too small</source>
      <translation>Promień kuli zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="360"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="362"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Promień stożka nie może być ujemny</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="364"/>
      <source>Height of cone too small</source>
      <translation>Wysokość stożka zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="427"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="429"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Promień elipsoidy zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="511"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="513"/>
      <source>Radius of torus too small</source>
      <translation>Promień torusa zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="576"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Wielokąt graniastosłupa jest nieprawidłowa, musi mieć 3 lub więcej boków</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="578"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Promień opisany wielokąta graniastosłupa jest zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="580"/>
      <source>Height of prism is too small</source>
      <translation>Wysokość graniastosłupa jest zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="661"/>
      <source>delta x of wedge too small</source>
      <translation>delta x klina zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="664"/>
      <source>delta y of wedge too small</source>
      <translation>delta y klina zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="667"/>
      <source>delta z of wedge too small</source>
      <translation>delta z klina zbyt mała</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="670"/>
      <source>delta z2 of wedge is negative</source>
      <translation>delta z2 klina jest ujemna</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="673"/>
      <source>delta x2 of wedge is negative</source>
      <translation>delta x2 klina jest ujemna</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="92"/>
      <source>Angle of revolution too large</source>
      <translation>Kąt obrotu zbyt duży</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="98"/>
      <source>Angle of revolution too small</source>
      <translation>Kąt obrotu zbyt mały</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Reference axis is invalid</source>
      <translation>Oś odniesienia jest nieprawidłowa</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="561"/>
      <source>Fusion with base feature failed</source>
      <translation>Scalenie z cechą podstawową nie powiodło się</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="103"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Cecha przekształcenia obiektu powiązanego nie jest obiektem części</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="108"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>Brak oryginałów powiązanych z przekształconą cechą.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="326"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Nie można przekształcić nieprawidłowego kształtu podpory</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="375"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>Kształt cechy addytywnej/subtraktywnej jest niezdefiniowany</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="367"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Tylko cechy addytywne i subtraktywne mogą podlegać przekształceniom</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="103"/>
      <source>Invalid face reference</source>
      <translation>Nieprawidłowe odniesienie ściany</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute Gear</source>
      <translation>Koło zębate ewolwentowe</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>Tworzy lub edytuje przekładnię ewolwentową</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket</source>
      <translation>Koło łańcuchowe</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>Tworzy lub edytuje koło łańcuchowe.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>Pokaż wynik końcowy</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>Pokaż podgląd nakładki</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="50"/>
      <source>Preview</source>
      <translation>Podgląd</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft Design Wizard</source>
      <translation>Kreator projektowania wału</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Starts the shaft design wizard</source>
      <translation>Uruchom kreatora projektowania wału</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>Błąd podczas obliczania podglądu usuniętej objętości: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="103"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>Kształt wynikowy nie zawiera materiału. 
Może to oznaczać, że nie zostanie usunięty żaden materiał lub wystąpił problem z modelem.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2372"/>
      <source>Create Datum</source>
      <translation>Utwórz układ odniesienia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2373"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Tworzy obiekt odniesienia lub system współrzędnych lokalnych</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2407"/>
      <source>Create Datum</source>
      <translation>Utwórz układ odniesienia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2408"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Tworzy obiekt odniesienia lub system współrzędnych lokalnych</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="198"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>Tworzy addytywny prostopadłościan przez podanie szerokości, wysokości i długości</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="202"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>Tworzy addytywny walec przez podanie promienia, wysokości i kąta</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="206"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>Tworzy addytywną sferę poprzez podanie promienia i różnych kątów</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="210"/>
      <source>Creates an additive cone</source>
      <translation>Tworzy addytywny stożek</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="214"/>
      <source>Creates an additive ellipsoid</source>
      <translation>Tworzy addytywną elipsoidę</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="218"/>
      <source>Creates an additive torus</source>
      <translation>Tworzy addytywny torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Creates an additive prism</source>
      <translation>Tworzy addytywny graniastosłup</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive wedge</source>
      <translation>Tworzy addytywny klin</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="350"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>Tworzy subtraktywny prostopadłościan podając jego szerokość, wysokość i długość</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="354"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>Tworzy subtraktywny walec przez podanie promienia, wysokości i kąta</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="358"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>Tworzy subtraktywną sferę poprzez podanie promienia i różnych kątów</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="362"/>
      <source>Creates a subtractive cone</source>
      <translation>Tworzy subtraktywny stożek</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="366"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>Tworzy subtraktywną elipsoidę</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="370"/>
      <source>Creates a subtractive torus</source>
      <translation>Tworzy subtraktywny torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="374"/>
      <source>Creates a subtractive prism</source>
      <translation>Tworzy subtraktywny graniastosłup</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="378"/>
      <source>Creates a subtractive wedge</source>
      <translation>Tworzy subtraktywny klin</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="986"/>
      <source>Attachment</source>
      <translation>Dołączenie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="828"/>
      <source>Revolution Parameters</source>
      <translation>Parametry wyciągnięcia przez obrót</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="836"/>
      <source>Groove Parameters</source>
      <translation>Parametry rowkowania</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation>Komunikaty funkcji transformacji</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="121"/>
      <source>Active Body</source>
      <translation>Aktywna zawartość</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer Parameters</source>
      <translation>Parametry sfazowania</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation>Parametry Płaszczyzny odniesienia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation>Parametry Linii odniesienia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation>Parametry Punktu odniesienia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation>Parametry Lokalnego układu współrzędnych</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft Parameters</source>
      <translation>Parametry funkcji pochylenie ścian</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet Parameters</source>
      <translation>Parametry zaokrąglenia</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>Linear Pattern Parameters</source>
      <translation>Parametry wzorca liniowego</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirror Parameters</source>
      <translation>Parametry odbicia lustrzanego</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>Multi-Transform Parameters</source>
      <translation>Parametry Transformacji wielokrotnej</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>Polar Pattern Parameters</source>
      <translation>Parametry Szyku kołowego</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scale Parameters</source>
      <translation>Parametry skalowania</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness Parameters</source>
      <translation>Parametry funkcji grubość</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="116"/>
      <source>Direction 2</source>
      <translation>Kierunek 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="219"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>Wybierz odniesienie kierunku (krawędź, ścianę, linię odniesienia).</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="299"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>Nieprawidłowy wybór. 
Wybierz krawędź, płaszczyznę lub linię odniesienia.</translation>
    </message>
  </context>
</TS>
