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
      <translation>Erstelle eine Baugruppe im aktuellen Dokument oder in der aktuell aktiven Baugruppe (falls vorhanden). Begrenzung auf eine Stamm Baugruppe pro Datei..</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="79"/>
      <source>Create a Fixed Joint</source>
      <translation>Erstelle eine feste Verbindung</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="86"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Wenn eine Baugruppe aktiv ist: Erstell eine Verbindung, die zwei Teile dauerhaft miteinander verriegelt und jede Bewegung oder Drehung verhindert.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="92"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ist ein Bauteil aktiv: Unterbauteile werden durch Angleichen der ausgewählten Koordinatensysteme zueinander ausgerichtet. Das als zweites ausgewählte Bauteil wird sich bewegen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="114"/>
      <source>Create Revolute Joint</source>
      <translation>Drehverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="121"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Erzeuge eine Drehverbindung: Erlaubt die Drehung um eine Achse zwischen zwei ausgwählten Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="142"/>
      <source>Create Cylindrical Joint</source>
      <translation>Zylindrische Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="149"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Erzeugt eine zylindrische Verbindung: Erlaubt die Drehung um eine Achse ohne lineare Bewegung entlang derselben Achse zwischen ausgewählen Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="168"/>
      <source>Create Slider Joint</source>
      <translation>Schiebereglerverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="175"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Erzeugt eine Gleitverbindung: Erlaubt lineare Bewegungen entlang einer Achse und schränkt die Drehung zwischen den ausgewählen Bauteilen ein.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="194"/>
      <source>Create Ball Joint</source>
      <translation>Kugelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="201"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Erzeugt eine Kugelverbindung: Verbindet Bauteile an einem Punkt und erlaubt uneingeschränkte Bewegung, solange die Punkte in Kontakt bleiben.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="220"/>
      <source>Create Distance Joint</source>
      <translation>Abstandsverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="227"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Erstelle eine Abstandsverbindung: Lege den Abstand zwischen den ausgewählten Objekten fest.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="233"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Erstellt eine von mehreren unterschiedlichen Verbindungen, basierend auf der Auswahl. Ein Abstand von 0 zwischen einer Ebene und einem Zylinder erstellt eine tangentiale Verbindung. Ein Abstand von 0 zwischen zwei Ebenen macht diese komplanar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="499"/>
      <source>Toggle grounded</source>
      <translation>Festsetzen umschalten</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
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
      <translation>Umschalttaste drücken, um durch Klicken in der Ansicht mehrere Verknüpfungen hinzuzufügen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Baugruppe lösen</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Löst die aktuell aktive Baugruppe (Neuberechnung).</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Assembly</translation>
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
      <location filename="../../../JointObject.py" line="46"/>
      <source>Fixed</source>
      <translation>Starr</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="47"/>
      <source>Revolute</source>
      <translation>Drehverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Cylindrical</source>
      <translation>Zylindrische Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Slider</source>
      <translation>Gleitverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Ball</source>
      <translation>Kugelverbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="1554"/>
      <source>Distance</source>
      <translation>Abstand</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Parallel</source>
      <translation>Parallel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Perpendicular</source>
      <translation>Rechtwinklig</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <location filename="../../../JointObject.py" line="1556"/>
      <source>Angle</source>
      <translation>Winkel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>RackPinion</source>
      <translation>Zahnstange-Ritzel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Screw</source>
      <translation>Spindel</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Gears</source>
      <translation>Zahnräder</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Belt</source>
      <translation>Riemen</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1409"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>Es müssen 2 Elemente von 2 separaten Bauteilen ausgewählt werden.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1558"/>
      <source>Radius 1</source>
      <translation>Radius 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1560"/>
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
      <translation>Index (Auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>Name (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Beschreibung</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>Dateiname (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>Menge (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>Dupliziere Name</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>Dieser Name wird bereits verwendet. Bitte wähle einen anderen Namen.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="367"/>
      <source>Options:</source>
      <translation>Optionen:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="374"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Untergeordnete Bauteile von Baugruppen: wenn ausgewählt, werden untergeordnete Bauteile von Baugruppen automatisch der Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>Komponenten von Bauteilen: wenn ausgewählt, werden Komponenten von Bauteilen der Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Nur Bauteile: wenn ausgewählt, werden nur Bauteil-Container und Unterbaugruppen der Stückliste hinzugefügt. Volumenkörper wie PartDesign Körper, Befestigungen oder Primitive des Part-Arbeitsbereichs werden ignoriert.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="388"/>
      <source>Columns:</source>
      <translation>Spalten:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="395"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>Automatische Spalten: (Index, Menge, Name...) werden automatisch ausgefüllt. Jede Änderung wird überschrieben. Diese Spalten können nicht umbenannt werden.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>Benutzerdefinierte Spalten: Die Daten von 'Beschreibung' und weiteren Benutzerdefinierten Spalten, die durch 'Spalte hinzufügen' hinzugefügt werden, werden nicht überschrieben. Diese Spalten können durch doppelklicken oder drücken von F2 umbenannt werden (aktuell verliert eine Spalte ihre Daten nach dem Umbenennen).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Jede Spalte (benutzerdefiniert oder nicht) kann durch drücken von Entf gelöscht werden.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="188"/>
      <source>The type of the joint</source>
      <translation>Der Typ der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="211"/>
      <location filename="../../../JointObject.py" line="475"/>
      <source>The first reference of the joint</source>
      <translation>Die erste Referenz der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="222"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem des ersten Referenzobjekts, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="244"/>
      <location filename="../../../JointObject.py" line="518"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation type="unfinished">This is the attachment offset of the first connector of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="253"/>
      <location filename="../../../JointObject.py" line="498"/>
      <source>The second reference of the joint</source>
      <translation>Die zweite Referenz der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem des zweiten Referenzobjekts, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="286"/>
      <location filename="../../../JointObject.py" line="528"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation type="unfinished">This is the attachment offset of the second connector of the joint.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="431"/>
      <source>The first object of the joint</source>
      <translation>Das erste Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="233"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der ersten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="450"/>
      <source>The second object of the joint</source>
      <translation>Das zweite Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="275"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der zweiten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="298"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>Dies ist der Abstand der Verbindung. Dieser wird nur von der Parallelverbindung, Zahnstange-Ritzel-Verbindung (Steigungsradius), Schraubverbindung, Zahnrad- und Riemenverbindung (Radius 1) verwendet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="309"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Dies ist der zweite Abstand der Verbindung. Er wird nur von der Zahnrad- und Riemenverbindung für den zweiten Radius verwendert.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="320"/>
      <source>This indicates if the joint is active.</source>
      <translation>Dies zeigt an, ob die Verbindung aktiv ist.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="332"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Aktiviere die minimale Längenbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="344"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Aktiviere die maximale Längenbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="356"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Aktiviere die minimale Winkelbegrenzung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="368"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Aktiviere die maximale Länge der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="380"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="391"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="402"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="413"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1065"/>
      <source>The object to ground</source>
      <translation>Das festzusetzende Objekt</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1077"/>
      <source>This is where the part is grounded.</source>
      <translation>Dies ist die Position, an der das Objekt festgesetzt wird.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="246"/>
      <location filename="../../../CommandCreateView.py" line="280"/>
      <source>The objects moved by the move</source>
      <translation>Die durch die Verschiebung bewegten Objekte</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="257"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Dies ist die Verschiebung der Bewegung. Die Endplatzierung ist das Ergebnis aus der Startplatzierung * dieser Platzierung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="265"/>
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
      <translation>Parallelverbindung</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Radius 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>Versatz</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>Drehwinkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="141"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Die Richtung der Verbindung umkehren.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>Reverse</source>
      <translation>Umkehren</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="155"/>
      <source>Limits</source>
      <translation>Grenzwerte</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="161"/>
      <source>Min length</source>
      <translation>Minimale Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="184"/>
      <source>Max length</source>
      <translation>Maximale Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Min angle</source>
      <translation>Minimaler Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max angle</source>
      <translation>Maximaler Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="268"/>
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
      <translation>Wenn aktiviert, wird die Liste nur Bauteile anzeigen.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
      <source>Show only parts</source>
      <translation>Zeige nur Bauteile</translation>
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
      <translation>Logge die Ziehschritte des Lösers. Nützlich, wenn du einen Fehler melden möchtest.
Die Dateien heißen "runPreDrag.asmt" und "dragging.log" befinden sich im Standardverzeichnis von std::ofstream (unter Windows ist es der Desktop)</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>Protokolliere Ziehschritte</translation>
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
      <location filename="../../ViewProviderAssembly.cpp" line="180"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Das Objekt gehört zu einer oder mehreren Verbindungen.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="182"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Soll das Objekt bewegt und zugehörige Verbindungen gelöscht werden?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="856"/>
      <source>Move part</source>
      <translation>Bauteil verschieben</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="334"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Zahnstange-Ritzel-Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="341"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Zahnstange-Ritzel-Verbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="346"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Dieselben Koordinatensysteme auswählen wie für die Dreh- und Gleitverbindung. Der Teilkreisradius (pitch radius) definiert das Bewegungsverhältnis zwischen Zahnstange und Ritzel.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="365"/>
      <source>Create Screw Joint</source>
      <translation>Spindelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="372"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Spindelverbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="377"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Dieselben Koordinatensysteme wie die Dreh- ud Gleitverbindung auswählen. Der Steigungsradius definiert das Bewegungsverhältnis zwischen rotierender Schraube und dem gleitenden Bauteil.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="408"/>
      <location filename="../../../CommandCreateJoint.py" line="439"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Dieselben Koordinatensysteme wie für die Drehverbindungen auswählen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="396"/>
      <source>Create Gears Joint</source>
      <translation>Zahnradverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="403"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Erstellt eine Zahnradverbindung: Koppelt die Drehungen zweier Zahnräder, d.h. die Drehrichtungen sind entgegengesetzt.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="427"/>
      <source>Create Belt Joint</source>
      <translation>Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="434"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Erstellt eine Riemenverbindung: Koppelt die Drehungen zweier (Riemen-)Scheiben, d.h. die Drehrichtungen sind gleich.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="459"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Zahnrad-/Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="465"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Erstellt Zahnrad/Riemenverbindung: Verknüpft zwei rotierende Objekte mittels Übersetzung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="470"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Dieselben Koordinatensysteme der Drehverbindungen auswählen.</translation>
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
      <translation>Wenn aktiviert, werden Bauteile als einzelner Festkörper ausgewählt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Bauteile als einzelne Festkörper</translation>
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
      <translation>Radial explodieren</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>Stückliste erstellen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Wenn ausgewählt, werden untergeordnete Bauteile von Baugruppen automatisch der Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>Untergeordnete Bauteile von Baugruppen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>Wenn ausgewählt, werden Komponenten von Bauteilen automatisch der Stückliste hinzugefügt.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Komponenten von Bauteilen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Wenn ausgewählt, werden nur Teile-Container und Unterbaugruppen der Stückliste hinzugefügt. Volumenkörper wie PartDesign Körper, Befestigungen oder Primitive des Part-Arbeitsbereichs werden ignoriert.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>Nur Bauteile</translation>
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
      <location filename="../../../CommandCreateJoint.py" line="252"/>
      <source>Create Parallel Joint</source>
      <translation>Parallele Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="259"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Erstellt eine parallele Verbindung: Die Z-Achsen der ausgewählten Koordinatensysteme werden parallel ausgerichtet.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="280"/>
      <source>Create Perpendicular Joint</source>
      <translation>Rechtwinklige Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="287"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Erstellt eine rechtwinklige Verbindung: Die Z-Achsen der ausgewählten Koordinatensysteme werden rechtwinklig ausgerichtet.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="306"/>
      <source>Create Angle Joint</source>
      <translation>Winkelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="313"/>
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
</TS>
