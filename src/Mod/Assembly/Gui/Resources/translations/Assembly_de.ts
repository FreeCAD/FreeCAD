<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Baugruppe erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Erstellt eine Baugruppe (Assembly-Objekt) im aktuellen Dokument oder in der aktuell aktiven Baugruppe (falls vorhanden). Eine Datei kann nur eine Hauptbaugruppe enthalten.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>Create a Fixed Joint</source>
      <translation>Starre Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="88"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ist eine Baugruppe aktiv: Erstellt eine dauerhaft starre Verbindung zwischen zwei Bauteilen, welche jegliche Verschiebung oder Drehung verhindert.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="94"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ist ein Bauteil aktiv: Unterbauteile werden durch Angleichen der ausgewählten Koordinatensysteme zueinander ausgerichtet. Das als zweites ausgewählte Bauteil wird sich bewegen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="116"/>
      <source>Create Revolute Joint</source>
      <translation>Drehverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="123"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Erzeuge eine Drehverbindung: Erlaubt die Drehung um eine Achse zwischen zwei ausgwählten Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="144"/>
      <source>Create Cylindrical Joint</source>
      <translation>Zylindrische Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="151"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Erzeugt eine zylindrische Verbindung: Erlaubt die Drehung um eine Achse ohne lineare Bewegung entlang derselben Achse zwischen ausgewählen Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="170"/>
      <source>Create Slider Joint</source>
      <translation>Gleitverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="177"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Erzeugt eine Gleitverbindung: Erlaubt lineare Bewegungen entlang einer Achse und schränkt die Drehung zwischen den ausgewählen Bauteilen ein.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="196"/>
      <source>Create Ball Joint</source>
      <translation>Kugelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="203"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Erzeugt eine Kugelverbindung: Verbindet Bauteile an einem Punkt und erlaubt uneingeschränkte Bewegung, solange die Punkte in Kontakt bleiben.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="222"/>
      <source>Create Distance Joint</source>
      <translation>Abstandsverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="229"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Erstellt eine Abstandsverbindung: Fixiert die Entfernung zwischen den ausgewählten Objekten.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="235"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Erstellt eine von mehreren unterschiedlichen Verbindungen, basierend auf der Auswahl. Ein Abstand von 0 zwischen einer Ebene und einem Zylinder erstellt eine tangentiale Verbindung. Ein Abstand von 0 zwischen zwei Ebenen macht diese komplanar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Toggle grounded</source>
      <translation>Festsetzen umschalten</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="513"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Das Festsetzen eines Bauteils fixiert seine Position in der Baugruppe und verhindert jegliches Verschieben oder Drehen. Es muss mindestens ein Bauteil in der Baugruppe festgesetzt werden, bevor weitere Bauteilverbindungen erstellt werden können.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>ASMT-Datei exportieren</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="51"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Exportiert die aktuell aktive Baugruppe als ASMT-Datei.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>Komponente einfügen</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Fügt eine Komponente in die aktuell aktive Baugruppe ein. Dies erstellt dynamische Verknüpfungen zu Bauteilen, Grundkörpern, Körpern und Baugruppen. Um externe Komponenten einzufügen, muss die Datei in der &lt;b&gt;aktuellen Sitzung geöffnet&lt;/b&gt; sein</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Einfügen durch Linksklick auf Elemente der Liste.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Entfernen durch Rechtsklick auf Elemente der Liste.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Umschalttaste drücken, um durch Klicken in der Ansicht mehrere Instanzen der Komponente hinzuzufügen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Baugruppe berechnen</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Berechnet die aktuell aktive Baugruppe.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Assembly</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="122"/>
      <source>Active object</source>
      <translation>Aktives Objekt</translation>
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
      <translation>Assembly</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation>Assembly-Verbindungen</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assembly</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>StarrerVerbund</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Drehverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Zylindrische Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Gleitverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Kugelverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <location filename="../../../JointObject.py" line="1456"/>
      <source>Distance</source>
      <translation>Abstand</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Parallel</source>
      <translation>Parallel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>Perpendicular</source>
      <translation>Rechtwinklig</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <location filename="../../../JointObject.py" line="1458"/>
      <source>Angle</source>
      <translation>Winkel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>RackPinion</source>
      <translation>Zahnstange-Ritzel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Screw</source>
      <translation>Spindel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>Gears</source>
      <translation>Zahnräder</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Belt</source>
      <translation>Riemen</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1301"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>Es müssen 2 Elemente von 2 separaten Bauteilen ausgewählt werden.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1460"/>
      <source>Radius 1</source>
      <translation>Radius 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1462"/>
      <source>Pitch radius</source>
      <translation>Steigungsradius</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>Nachfragen</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>Immer</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>Nie</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>Index (automatisch)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>Name (automatisch)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Beschreibung</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>Dateiname (automatisch)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>Menge (automatisch)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>Doppelter Name</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>Dieser Name wird bereits verwendet. Bitte einen anderen Namen auswählen.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>Optionen:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Elemente von Unterbaugruppen: Wenn aktiviert, werden Elemente von Unterbaugruppen automatisch zur Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>Elemente von Part-Objekten: Wenn aktiviert, werden Elemente von Part-Objekten zur Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Nur Part-Objekte: Wenn aktiviert, werden nur Part-Container und Unterbaugruppen zur Stückliste hinzugefügt. Festkörper wie PartDesign-Körper, Verbindungselemente oder Grundkörper des Arbeitsbereichs Part werden ignoriert.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>Spalten:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>Automatische Spalten: (Index, Menge, Name...) werden automatisch ausgefüllt. Jede Änderung wird überschrieben. Diese Spalten können nicht umbenannt werden.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>Benutzerdefinierte Spalten: Die Daten von 'Beschreibung' und weiteren benutzerdefinierten Spalten, die durch Anklicken von 'Spalte hinzufügen' hinzugefügt werden, werden nicht überschrieben. Diese Spalten können mit einem Doppelklick oder durch Drücken von F2 umbenannt werden (Das Umbenennen einer Spalte führt derzeit zum Verlust ihrer Daten).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Jede Spalte (benutzerdefiniert oder nicht) kann durch Drücken der Entf-Taste gelöscht werden.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="165"/>
      <source>The type of the joint</source>
      <translation>Die Art der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <location filename="../../../JointObject.py" line="439"/>
      <source>The first reference of the joint</source>
      <translation>Die erste Referenz der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="200"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem im Objekt der ersten Referenz, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="222"/>
      <location filename="../../../JointObject.py" line="498"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>Dies ist der Befestigungsversatz des ersten Gelenks der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="231"/>
      <location filename="../../../JointObject.py" line="464"/>
      <source>The second reference of the joint</source>
      <translation>Die zweite Referenz der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem im Objekt der zweiten Referenz, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <location filename="../../../JointObject.py" line="508"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>Dies ist der Befestigungsversatz des zweiten Gelenks der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="409"/>
      <source>The first object of the joint</source>
      <translation>Das erste Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="211"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der ersten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="428"/>
      <source>The second object of the joint</source>
      <translation>Das zweite Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="253"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der zweiten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>Dies ist der Abstand der Verbindung. Dieser wird nur von der Abstandsverbindung, der Zahnstange-Ritzel-Verbindung und der Spindelverbindung (als Steigungsradius), sowie der Zahnrad- und der Riemenverbindung (als Radius 1) verwendet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="287"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Dies ist der zweite Abstand der Verbindung. Er wird nur von der Zahnrad- und der Riemenverbindung für den zweiten Radius verwendet.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="298"/>
      <source>This indicates if the joint is active.</source>
      <translation>Dies zeigt an, ob die Verbindung aktiv ist.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="310"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Aktiviere die minimale Längenbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="322"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Aktiviere die maximale Längenbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="334"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Aktiviere die minimale Winkelbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="346"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Aktiviere die maximale Länge der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="358"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="369"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="380"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="391"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="947"/>
      <source>The object to ground</source>
      <translation>Das festzusetzende Objekt</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="959"/>
      <source>This is where the part is grounded.</source>
      <translation>Dies ist die Position, an der das Objekt festgesetzt wird.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="248"/>
      <location filename="../../../CommandCreateView.py" line="282"/>
      <source>The objects moved by the move</source>
      <translation>Die durch die Verschiebung bewegten Objekte</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Dies ist die Verschiebung der Bewegung. Die Endplatzierung ist das Ergebnis aus der Startplatzierung * dieser Platzierung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="267"/>
      <source>The type of the move</source>
      <translation>Die Art der Bewegung</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>Abstandsverbindung</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Radius 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="87"/>
      <source>Attachement offsets</source>
      <translation type="unfinished">Attachement offsets</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="94"/>
      <source>Simple</source>
      <translation>Einfach</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="102"/>
      <source>Offset</source>
      <translation>Versatz</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="126"/>
      <source>Rotation</source>
      <translation>Drehwinkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="149"/>
      <source>Advanced</source>
      <translation>Erweitert</translation>
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
      <translation>Die Richtung der Verbindung umkehren.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Reverse</source>
      <translation>Umkehren</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="224"/>
      <source>Limits</source>
      <translation>Grenzwerte</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="230"/>
      <source>Min length</source>
      <translation>Minimale Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="253"/>
      <source>Max length</source>
      <translation>Maximale Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="276"/>
      <source>Min angle</source>
      <translation>Minimaler Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="305"/>
      <source>Max angle</source>
      <translation>Maximaler Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="337"/>
      <source>Reverse rotation</source>
      <translation>Drehrichtung umkehren</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>Komponente einfügen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Bauteile durchsuchen...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>Bauteil nicht gefunden? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>Datei öffnen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Wenn aktiviert, zeigt die Liste nur Part-Objekte an.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>Nur Part-Objekte anzeigen</translation>
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
      <translation>Allgemein</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Ermöglicht den Bearbeitungsmodus durch Drücken der Esc-Taste zu verlassen</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leaves edit mode</source>
      <translation>Mit Esc den Bearbeitungsmodus verlassen</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>Zeichnet die Ziehschritte des Gleichungslösers auf. Nützlich, wenn man einen Fehler melden möchte.
Die Dateien heißen "runPreDrag.asmt" und "dragging.log" und befinden sich im Standardverzeichnis von std::ofstream (unter Windows ist es der Schreibtisch)</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>Ziehschritte protokollieren</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>Erstes Bauteil festsetzen:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Wenn das erste Bauteil in die Baugruppe eingefügt wird, kann es automatisch festgesetzt werden.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="197"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Das Objekt gehört zu einer oder mehreren Verbindungen.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="199"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Soll das Objekt bewegt und zugehörige Verbindungen gelöscht werden?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="883"/>
      <source>Move part</source>
      <translation>Bauteil verschieben</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="336"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Zahnstange-Ritzel-Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Zahnstange-Ritzel-Verbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="348"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Dieselben Koordinatensysteme auswählen wie für die Dreh- und Gleitverbindung. Der Teilkreisradius (pitch radius) definiert das Bewegungsverhältnis zwischen Zahnstange und Ritzel.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="367"/>
      <source>Create Screw Joint</source>
      <translation>Spindelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Spindelverbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="379"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Dieselben Koordinatensysteme wie für die Dreh- ud Gleitverbindung auswählen. Der Steigungsradius definiert das Bewegungsverhältnis zwischen rotierender Spindel (Schraube) und dem gleitenden Bauteil.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="410"/>
      <location filename="../../../CommandCreateJoint.py" line="441"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Dieselben Koordinatensysteme wie für die Drehverbindungen auswählen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="398"/>
      <source>Create Gears Joint</source>
      <translation>Zahnradverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Erstellt eine Zahnradverbindung: Koppelt die Drehungen zweier Zahnräder, d.h. die Drehrichtungen sind entgegengesetzt.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="429"/>
      <source>Create Belt Joint</source>
      <translation>Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Erstellt eine Riemenverbindung: Koppelt die Drehungen zweier (Riemen-)Scheiben, d.h. die Drehrichtungen sind gleich.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="461"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Zahnrad-/Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Erstellt eine Zahnrad- oder eine Riemenverbindung: Koppelt zwei rotierende (Zahn-) Räder.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="472"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Dieselben Koordinatensysteme wie für die Drehverbindungen auswählen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>Explosionsansicht erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>Erstellt eine Explosionsansicht der aktuellen Baugruppe.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>Explosionsansicht erstellen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>Wenn aktiviert, werden Part-Objekte jeweils wie ein einteiliger Festkörper ausgewählt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Part-Objekte wie einteilige Festkörper</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>Bewegungskreuz ausrichten</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>Bewegungskreuz ausrichten:
Ein Merkmal auswählen.
ESC zum abbrechen.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>Radial sprengen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>Stückliste erstellen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Wenn aktiviert, werden Elemente von Unterbaugruppen automatisch zur Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>Elemente von Unterbaugruppen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>Wenn aktiviert, werden Elemente von Part-Objekten automatisch zur Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Elemente von Part-Objekten</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Wenn aktiviert, werden nur Part-Container und Unterbaugruppen zur Stückliste hinzugefügt. Festkörper wie PartDesign-Körper, Verbindungselemente oder Grundkörper des Arbeitsbereichs Part werden ignoriert.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>Nur Part-Objekte</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>Spalten</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>Spalte hinzufügen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>Exportieren</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>Hilfe</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="254"/>
      <source>Create Parallel Joint</source>
      <translation>Parallele Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="261"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Erstellt eine parallele Verbindung: Die Z-Achsen der ausgewählten Koordinatensysteme werden parallel ausgerichtet.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Perpendicular Joint</source>
      <translation>Rechtwinklige Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Erstellt eine rechtwinklige Verbindung: Die Z-Achsen der ausgewählten Koordinatensysteme werden rechtwinklig ausgerichtet.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="308"/>
      <source>Create Angle Joint</source>
      <translation>Winkelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="315"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>Erstellt eine Winkelverbindung: Der Winkel zwischen den Z-Achsen der ausgewählten Koordinatensysteme wird festgelegt.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>Stückliste erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>Erstelle eine Stückliste der aktuellen Baugruppe. Falls eine Baugruppe aktiv ist, wird die Stückliste für diese Baugruppe erstellt, andernfalls für das gesamte Dokument.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>Das BOM-Objekt ist ein Dokument-Objekt, das die Einstellungen der Stückliste speichert. Es ist auch ein Tabellenobjekt, mit dem die Stückliste einfach dargestellt werden kann. Muss das BOM-Objekt nicht als Dokument-Objekt gespeichert werden, kann es einfach exportiert und der Vorgang abgebrochen werden.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>Die Spalten 'Index', 'Name', 'Dateiname' und 'Menge' werden automatisch beim Neuberechnen generiert. Die 'Beschreibung' und benutzerdefinierte Spalten werden nicht überschrieben.</translation>
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
