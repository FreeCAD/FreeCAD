<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="pl" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Utwórz złożenie</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Tworzy obiekt złożenia w bieżącym dokumencie
lub w bieżącym aktywnym złożeniu (jeśli istnieje).
Limit jednego głównego złożenia na plik.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>Create a Fixed Joint</source>
      <translation>Utwórz połączenie stałe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="88"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Jeśli złożenie jest aktywne:
stwórz połączenie trwale blokujące dwie części,
zapobiegając wszelkim ruchom lub obrotom.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="94"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Jeśli część jest aktywna:
Definiuje pozycję części podrzędnej poprzez dopasowanie wybranych układów współrzędnych.
Druga część zostanie dopasowana.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="116"/>
      <source>Create Revolute Joint</source>
      <translation>Utwórz połączenie obrotowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="123"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Utwórz połączenie z możliwością obrotu:
Pozwala na obrót wokół pojedynczej osi pomiędzy wybranymi częściami.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="144"/>
      <source>Create Cylindrical Joint</source>
      <translation>Utwórz połączenie cylindryczne</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="151"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Tworzy łącze linearne: Umożliwia obrót wzdłuż jednej osi,
umożliwiając poruszanie się wzdłuż tej samej osi między częściami złożenia.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="170"/>
      <source>Create Slider Joint</source>
      <translation>Utwórz połączenie przesuwne</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="177"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Tworzy łącze przesuwne: Pozwala na ruch liniowy wzdłuż pojedynczej osi,
ale ogranicza rotację pomiędzy wybranymi częściami.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="196"/>
      <source>Create Ball Joint</source>
      <translation>Utwórz przegub kulowy</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="203"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Tworzy przegub kulowy: Łączy części w punkcie,
umożliwiając nieograniczony ruch,
o ile punkty połączenia pozostają w kontakcie.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="222"/>
      <source>Create Distance Joint</source>
      <translation>Utwórz połączenie dystansowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="229"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Tworzy łącze odległości:
Ustal odległość między wybranymi obiektami.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="235"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Tworzy jedno z kilku różnych połączeń zależnie od wyboru.
Na przykład odległość 0 między płaszczyzną a cylindrem tworzy połączenie styczne.
Odległość 0 między płaszczyznami sprawi, że będą one współpłaszczyznowe.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Toggle grounded</source>
      <translation>Włącz / wyłącz zakotwienie</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="513"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Zakotwienie części trwale blokuje jej pozycję w złożeniu,
uniemożliwiając jakikolwiek ruch lub obrót.
Przed rozpoczęciem montażu należy zakotwić co najmniej jedną część.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Eksportuj do pliku ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="51"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Wykona eksport aktywnego złożenia do pliku w formacie ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>Wstaw komponent</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Wstawia komponentu do aktywnego złożenia.
Spowoduje to utworzenie dynamicznych łączy do części, brył, prymitywów i złożeń.
Aby wstawić komponenty zewnętrzne, upewnij się, że plik jest otwarty w bieżącej sesji</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Wstaw elementy, klikając ja lewym przyciskiem myszki na liście.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Usuń elementy, klikając je prawym przyciskiem myszki na liście.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Naciśnij Shift, aby dodać kilka wystąpień komponentu podczas klikania na widok.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Rozwiąż złożenie</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Wykona operację "Rozwiązania" aktywnego złożenia.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Złożenie</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="122"/>
      <source>Active object</source>
      <translation>Aktywny obiekt</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="127"/>
      <source>Turn flexible</source>
      <translation type="unfinished">Turn flexible</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="128"/>
      <source>Your sub-assembly is currently rigid. This will make it flexible instead.</source>
      <translation type="unfinished">Your sub-assembly is currently rigid. This will make it flexible instead.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="132"/>
      <source>Turn rigid</source>
      <translation type="unfinished">Turn rigid</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="133"/>
      <source>Your sub-assembly is currently flexible. This will make it rigid instead.</source>
      <translation type="unfinished">Your sub-assembly is currently flexible. This will make it rigid instead.</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly</source>
      <translation>Złożenie</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation>Połączenia złożeń</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Złożenie</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Stały</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Utwórz połączenie obrotowe</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Połączenie cylindryczne</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Połączenie przesuwne</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Przegub kulowy</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <location filename="../../../JointObject.py" line="1456"/>
      <source>Distance</source>
      <translation>Odległość</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Parallel</source>
      <translation>Równolegle</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>Perpendicular</source>
      <translation>Prostopadle</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <location filename="../../../JointObject.py" line="1458"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>RackPinion</source>
      <translation>Przekładnia zębatkowa</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Screw</source>
      <translation>Śruba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>Gears</source>
      <translation>Koła zębate</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Belt</source>
      <translation>Pas</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1301"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>Musisz wybrać dwa elementy z dwóch oddzielnych części.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1460"/>
      <source>Radius 1</source>
      <translation>Promień 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1462"/>
      <source>Pitch radius</source>
      <translation>Promień nachylenia</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>Zapytaj</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>Zawsze</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>Nigdy</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>Indeks (automatycznie)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>Nazwa (automatycznie)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Opis</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>Nazwa pliku (automatycznie)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>Ilość (automatycznie)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Domyślny</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>Duplikat nazwy</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>Ta nazwa jest już używana.
Proszę wybrać inną.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>Opcje:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Elementy podrzędne złożeń: Jeśli opcja jest zaznaczona, elementy podrzędne złożeń zostaną dodane do listy materiałów.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>Elementy podrzędne: Jeśli opcja jest zaznaczona, elementy podrzędne zostaną dodane do listy materiałów.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Tylko części: Jeśli opcja jest zaznaczona, do zestawienia materiałów będą dodawane tylko kontenery środowiska Część i podzespoły. Obiekty takie jak Zawartość środowiska Projekt Części, elementy złączne lub elementy pierwotne środowiska pracy Części będą ignorowane.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>Kolumny:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>Kolumny automatycznie: (Indeks, Ilość, Nazwa...) są wypełniane automatycznie.
Wszelkie wprowadzone modyfikacje zostaną zastąpione. Nie można zmienić nazwy tych kolumn.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>Kolumny niestandardowe:
 "Opis" i inne kolumny niestandardowe dodane przez kliknięcie przycisku "Dodaj kolumnę" nie zostaną nadpisane.
Nazwy tych kolumn można zmienić, klikając dwukrotnie lub naciskając klawisz F2 (zmiana nazwy kolumny spowoduje utratę jej danych).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Każdą kolumnę (niestandardową lub nie) można usunąć,
 naciskając klawisz Del.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="165"/>
      <source>The type of the joint</source>
      <translation>Typ połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <location filename="../../../JointObject.py" line="439"/>
      <source>The first reference of the joint</source>
      <translation>Pierwsze odniesienie do połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="200"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Jest to lokalny układ współrzędnych w pierwszym obiekcie Odniesienie 1, który będzie używany dla połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="222"/>
      <location filename="../../../JointObject.py" line="498"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>Jest to odsunięcie mocowania pierwszego złącza połączenia. </translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="231"/>
      <location filename="../../../JointObject.py" line="464"/>
      <source>The second reference of the joint</source>
      <translation>Drugie odniesienie do połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Jest to lokalny układ współrzędnych w pierwszym obiekcie Odniesienie 2, który będzie używany dla połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <location filename="../../../JointObject.py" line="508"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>Jest to odsunięcie mocowania drugiego złącza połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="409"/>
      <source>The first object of the joint</source>
      <translation>Pierwszy obiekt połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="211"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Zapobiega to przeliczaniu pierwszego umiejscowienia,
umożliwiając niestandardowe pozycjonowanie umiejscowienia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="428"/>
      <source>The second object of the joint</source>
      <translation>Drugi obiekt połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="253"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Zapobiega to przeliczaniu drugiego umiejscowienia,
umożliwiając niestandardowe pozycjonowanie umiejscowienia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>To jest odległość połączenia. Jest używana tylko przez połączenie dystansowe, przekładni zębatkowej (promień skoku), śrubowej, zębatej oraz pasowej (promień 1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="287"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Jest to druga odległość połączenia.
Jest on używany tylko przez połączenie zębate do przechowywania drugiego promienia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="298"/>
      <source>This indicates if the joint is active.</source>
      <translation>Wskazuje to, czy połączenie jest aktywne.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="310"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Włącz limit minimalnej długości połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="322"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Włącz limit maksymalnej długości połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="334"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Włącz limit minimalny kąta połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="346"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Włącz minimalną długość połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="358"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Jest to minimalny limit długości między oboma układami współrzędnych (wzdłuż ich osi Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="369"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Jest to maksymalny limit długości między oboma układami współrzędnych (wzdłuż ich osi Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="380"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Jest to minimalny limit kąta między oboma układami współrzędnych (między ich osiami X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="391"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Jest to maksymalny limit kąta między oboma układami współrzędnych (między ich osiami X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="947"/>
      <source>The object to ground</source>
      <translation>Obiekt do zakotwienia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="959"/>
      <source>This is where the part is grounded.</source>
      <translation>W tym miejscu część jest zakotwiona.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="248"/>
      <location filename="../../../CommandCreateView.py" line="282"/>
      <source>The objects moved by the move</source>
      <translation>Obiekty przeniesione przez ruch</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Jest to ruch przesunięcia. Pozycja końcowa jest wynikiem pozycji początkowej * tej pozycji.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="267"/>
      <source>The type of the move</source>
      <translation>Typ ruchu</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Utwórz połączenie</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>Odległość</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Promień 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="87"/>
      <source>Attachement offsets</source>
      <translation type="unfinished">Attachement offsets</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="94"/>
      <source>Simple</source>
      <translation>Proste</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="102"/>
      <source>Offset</source>
      <translation>Odsunięcie</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="126"/>
      <source>Rotation</source>
      <translation>Obrót</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="149"/>
      <source>Advanced</source>
      <translation>Zaawansowane</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="157"/>
      <source>Offset1</source>
      <translation type="unfinished">Offset1</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="164"/>
      <source>By clicking this button, you can set the attachement offset of the first marker (coordinate system) of the joint.</source>
      <translation type="unfinished">By clicking this button, you can set the attachement offset of the first marker (coordinate system) of the joint.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="178"/>
      <source>Offset2</source>
      <translation type="unfinished">Offset2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="185"/>
      <source>By clicking this button, you can set the attachement offset of the second marker (coordinate system) of the joint.</source>
      <translation type="unfinished">By clicking this button, you can set the attachement offset of the second marker (coordinate system) of the joint.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="210"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Odwróć kierunek połączenia.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Reverse</source>
      <translation>Odwróć</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="224"/>
      <source>Limits</source>
      <translation>Limity</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="230"/>
      <source>Min length</source>
      <translation>Minimalna długość</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="253"/>
      <source>Max length</source>
      <translation>Maksymalna długość</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="276"/>
      <source>Min angle</source>
      <translation>Minimalny kąt</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="305"/>
      <source>Max angle</source>
      <translation>Maksymalny kąt</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="337"/>
      <source>Reverse rotation</source>
      <translation>Obroty w przeciwnym kierunku</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>Wstaw komponent</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Wyszukiwanie części ...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>Nie możesz znaleźć części? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>Otwórz plik</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Jeśli opcja ta jest zaznaczona, na liście wyświetlane będą tylko części.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>Pokaż tylko części</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="74"/>
      <source>If checked, the inserted sub-assemblies will not be flexible.
Rigid means that the sub-assembly will be considered as a solid.
Flexible means that the sub-assembly joints will be taken into account in the main assembly.
You can change this property of sub-assemblies at any time by right clicking them.</source>
      <translation type="unfinished">If checked, the inserted sub-assemblies will not be flexible.
Rigid means that the sub-assembly will be considered as a solid.
Flexible means that the sub-assembly joints will be taken into account in the main assembly.
You can change this property of sub-assemblies at any time by right clicking them.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="80"/>
      <source>Rigid sub-assemblies</source>
      <translation type="unfinished">Rigid sub-assemblies</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Ogólne</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Pozwala opuścić tryb edycji szkicu po naciśnięciu klawisza Esc.</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Naciśnięcie "Esc" opuszcza tryb edycji</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>Rejestruje kroki przeciągania solvera. Przydatne, jeśli chcesz zgłosić błąd.
Pliki noszą nazwy "runPreDrag.asmt" i "dragging.log" i znajdują się w domyślnym katalogu std::ofstream (w systemie Windows jest to pulpit).</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>Zapisuj kroki przeciągania</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>Kotwienie pierwszej części:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Kiedy umieścisz pierwszą część w złożeniu, możesz automatycznie zakotwić tę część.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="197"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Obiekt jest powiązany z jednym lub większą liczbą połączeń.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="199"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Czy chcesz przenieść obiekt i usunąć powiązane połączenia?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="883"/>
      <source>Move part</source>
      <translation>Przesuń część</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="336"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Utwórz połączenie zębatki i koła zębatego</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Utwórz połączenie zębatkowe: łączy część ze złączem przesuwnym z częścią ze złączem obrotowym.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="348"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Wybierz te same układy współrzędnych, co połączenia obrotowe i przesuwne. Promień skoku określa stosunek ruchu między zębatką a zębnikiem.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="367"/>
      <source>Create Screw Joint</source>
      <translation>Utwórz połączenie śrubowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Utwórz połączenie śrubowe: Łączy część ze złączem ślizgowym z częścią ze złączem obrotowym.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="379"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Wybierz te same układy współrzędnych, co dla połączeń obrotowych i przesuwnych. Promień skoku określa stosunek ruchu między obracającą się śrubą a częścią ślizgową.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="410"/>
      <location filename="../../../CommandCreateJoint.py" line="441"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Wybierz te same układy współrzędnych co połączenia obrotowe.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="398"/>
      <source>Create Gears Joint</source>
      <translation>Utwórz połączenie kół zębatych</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Utwórz połączenie kół zębatych:
Łączy ze sobą dwa obracające się koła zębate.
Będą one miały odwrotny kierunek obrotu.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="429"/>
      <source>Create Belt Joint</source>
      <translation>Utwórz połączenie pasowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Utwórz połączenie pasowe:
Łączy ze sobą dwa obracające się obiekty.
Będą one miały ten sam kierunek obrotu.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="461"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Utwórz połączenie przekładni zębatej / pasowej</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Utwórz połączenie przekładni / pasa:
Łączy ze sobą dwa obracające się koła zębate.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="472"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Wybierz te same układy współrzędnych co połączenia obrotowe.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>Utwórz widok rozłożenia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>Tworzy widok rozstrzelony bieżącego złożenia.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>Utwórz widok rozłożenia</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>Jeśli opcja ta jest zaznaczona, części będą wybierane jako pojedyncza bryła.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Części jako pojedyncza bryła</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>Wyrównanie przeciągania</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>Wyrównanie przeciągania:
Wybierz funkcję.
Naciśnij ESC, aby anulować.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>Rozłóż promieniowo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>Utwórz bilans materiałów</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Jeśli opcja ta jest zaznaczona, elementy podrzędne złożeń zostaną dodane do listy materiałów.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>Elementy podrzędne złożeń zagnieżdżonych</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>Jeśli opcja ta jest zaznaczona, do listy materiałów zostaną dodane elementy podrzędne obiektów środowiska Część.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Elementy podrzędne obiektów środowiska Część</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>W przypadku zaznaczenia tej opcji do zestawienia materiałów dodawane będą tylko kontenery części i ich elementy podrzędne. Obiekty, takie jak Zawartość środowiska Projekt Części, elementy złączne lub elementy pierwotne środowiska Część zostaną zignorowane.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>Tylko części</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>Kolumny</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>Dodaj kolumnę</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>Eksport</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>Pomoc</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="254"/>
      <source>Create Parallel Joint</source>
      <translation>Utwórz połączenie równoległe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="261"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Tworzy połączenie równoległe: Utwórz oś Z wybranych układów współrzędnych równolegle.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Perpendicular Joint</source>
      <translation>Utwórz połączenie prostopadłe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Tworzy połączenie prostopadłe: Utwórz oś Z wybranych układów współrzędnych prostopadle.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="308"/>
      <source>Create Angle Joint</source>
      <translation>Utwórz połączenie kątowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="315"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>Tworzy połączenie kątowe: Ustala kąt między osiami Z wybranych układów współrzędnych.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>Utwórz Zestawienie materiałów</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>Tworzy zestawienie materiałów bieżącego złożenia.
Jeśli zespół jest aktywny, będzie to lista materiałowa tego zespołu.
W przeciwnym razie będzie to zestawienie całego dokumentu.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>Obiekt BOM jest obiektem dokumentu, który przechowuje ustawienia BOM.
Jest to również obiekt arkusza kalkulacyjnego, dzięki czemu można łatwo wizualizować BOM.
Jeśli obiekt BOM nie ma być zapisany jako obiekt dokumentu, można go po prostu wyeksportować i anulować zadanie.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>Kolumny "Indeks", "Nazwa", "Nazwa pliku" i "Ilość" są generowane automatycznie po ponownym obliczeniu. Kolumny " Opis " i niestandardowe nie są nadpisywane.</translation>
    </message>
  </context>
  <context>
    <name>Assembly::AssemblyLink</name>
    <message>
      <location filename="../../../App/AssemblyLink.cpp" line="512"/>
      <source>Joints</source>
      <translation type="unfinished">Joints</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="139"/>
      <source>Toggle Rigid</source>
      <translation type="unfinished">Toggle Rigid</translation>
    </message>
  </context>
</TS>
