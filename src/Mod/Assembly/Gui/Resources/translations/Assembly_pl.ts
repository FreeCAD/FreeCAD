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
      <location filename="../../../CommandCreateJoint.py" line="68"/>
      <source>Create a Fixed Joint</source>
      <translation>Utwórz połączenie stałe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="75"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Jeśli złożenie jest aktywne:
stwórz połączenie trwale blokujące dwie części,
zapobiegając wszelkim ruchom lub obrotom.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Jeśli część jest aktywna:
Definiuje pozycję części podrzędnej poprzez dopasowanie wybranych układów współrzędnych.
Druga część zostanie dopasowana.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="103"/>
      <source>Create Revolute Joint</source>
      <translation>Utwórz połączenie obrotowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="110"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Utwórz połączenie z możliwością obrotu:
Pozwala na obrót wokół pojedynczej osi pomiędzy wybranymi częściami.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="131"/>
      <source>Create Cylindrical Joint</source>
      <translation>Utwórz połączenie cylindryczne</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="138"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Tworzy łącze linearne: Umożliwia obrót wzdłuż jednej osi,
umożliwiając poruszanie się wzdłuż tej samej osi między częściami złożenia.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="157"/>
      <source>Create Slider Joint</source>
      <translation>Utwórz połączenie przesuwne</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="164"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Tworzy łącze przesuwne: Pozwala na ruch liniowy wzdłuż pojedynczej osi,
ale ogranicza rotację pomiędzy wybranymi częściami.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="183"/>
      <source>Create Ball Joint</source>
      <translation>Utwórz przegub kulowy</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="190"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Tworzy przegub kulowy: Łączy części w punkcie,
umożliwiając nieograniczony ruch,
o ile punkty połączenia pozostają w kontakcie.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="209"/>
      <source>Create Distance Joint</source>
      <translation>Utwórz połączenie dystansowe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="216"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Tworzy łącze odległości:
Ustal odległość między wybranymi obiektami.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Toggle grounded</source>
      <translation>Włącz / wyłącz zakotwienie</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
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
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Wykona eksport aktywnego złożenia do pliku w formacie ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="52"/>
      <source>Insert Link</source>
      <translation>Wstaw łącze</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="59"/>
      <source>Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Umożliwia wstawienie łącza do aktualnie aktywnego złożenia.
Spowoduje to utworzenie dynamicznych łączy do części, obiektów, elementów pierwotnych, złożeń.
Aby wstawić obiekty zewnętrzne, upewnij się, że plik jest &lt;b&gt;otwarty w bieżącej sesji&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="61"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Wstaw elementy, klikając ja lewym przyciskiem myszki na liście.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="65"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Usuń elementy, klikając je prawym przyciskiem myszki na liście.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="70"/>
      <source>Press shift to add several links while clicking on the view.</source>
      <translation>Naciśnij klawisz Shift,
aby dodać kilka łączy podczas klikania w oknie widoku.</translation>
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
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="98"/>
      <source>Assembly</source>
      <translation>Złożenie</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="99"/>
      <source>Assembly Joints</source>
      <translation>Połączenia złożeń</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="102"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Złożenie</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Nieruchomy</translation>
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
      <source>Distance</source>
      <translation>Odległość</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Zapytaj</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Zawsze</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Nigdy</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="116"/>
      <source>The type of the joint</source>
      <translation>Typ połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="126"/>
      <source>The first object of the joint</source>
      <translation>Pierwszy obiekt połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="133"/>
      <source>The first part of the joint</source>
      <translation>Pierwsza część do połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="140"/>
      <source>The selected element of the first object</source>
      <translation>Wybrany element pierwszego obiektu</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="147"/>
      <source>The selected vertex of the first object</source>
      <translation>Wybrany wierzchołek pierwszego obiektu</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="157"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Jest to lokalny układ współrzędnych w pierwszym obiekcie, który będzie używany dla połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="167"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Zapobiega to przeliczaniu pierwszego umiejscowienia,
umożliwiając niestandardowe pozycjonowanie umiejscowienia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="175"/>
      <source>The second object of the joint</source>
      <translation>Drugi obiekt połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="182"/>
      <source>The second part of the joint</source>
      <translation>Druga część do połączenia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <source>The selected element of the second object</source>
      <translation>Wybrany element drugiego obiektu</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="196"/>
      <source>The selected vertex of the second object</source>
      <translation>Wybrany wierzchołek drugiego obiektu</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Jest to lokalny układ współrzędnych w drugim obiekcie, który będzie używany dla połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Zapobiega to przeliczaniu drugiego umiejscowienia,
umożliwiając niestandardowe pozycjonowanie umiejscowienia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="226"/>
      <source>This is the distance of the joint. It is used only by the distance joint.</source>
      <translation>Jest to odległość połączenia. Jest używana tylko przez połączenie dystansowe.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="236"/>
      <source>This is the rotation of the joint.</source>
      <translation>Jest to obrót połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="246"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Jest to wektor odsunięcia połączenia.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="256"/>
      <source>This indicates if the joint is active.</source>
      <translation>Wskazuje to, czy połączenie jest aktywne.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="889"/>
      <source>The object to ground</source>
      <translation>Obiekt do zakotwienia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="901"/>
      <source>This is where the part is grounded.</source>
      <translation>W tym miejscu część jest zakotwiona.</translation>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Odległość</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Offset</source>
      <translation>Odsunięcie</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Rotation</source>
      <translation>Obrót</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="104"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Odwróć kierunek połączenia.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="107"/>
      <source>Reverse</source>
      <translation>Odwróć</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Link</source>
      <translation>Wstaw łącze</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Wyszukiwanie części ...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Nie możesz znaleźć części? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Otwórz plik</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the selected object will be inserted inside a Part container, unless it is already a Part.</source>
      <translation>Jeśli opcja ta jest zaznaczona, wybrany obiekt
zostanie umieszczony wewnątrz kontenera Część,
chyba że jest już obiektem typu Część.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Insert as part</source>
      <translation>Wstaw jako część</translation>
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
      <source>Esc leave edit mode</source>
      <translation>Naciśnięcie "Esc" opuszcza tryb edycji</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Kotwienie pierwszej części:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Kiedy umieścisz pierwszą część w złożeniu, możesz automatycznie zakotwić tę część.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="136"/>
      <source>Delete associated joints</source>
      <translation>Usuń powiązane połączenia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Obiekt jest powiązany z jednym lub większą liczbą połączeń.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Czy chcesz przenieść obiekt i usunąć powiązane połączenia?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="651"/>
      <source>Move part</source>
      <translation>Przesuń część</translation>
    </message>
  </context>
</TS>
