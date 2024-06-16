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
      <translation>Erzeugt ein Baugruppen-Objekt im aktuellen Dokument oder in der aktuell aktiven Baugruppe (falls vorhanden). Das Limit ist eine Über-Baugruppe pro Datei.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="76"/>
      <source>Create a Fixed Joint</source>
      <translation>Starre Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="83"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ist eine Baugruppe aktiv: Erstellt eine dauerhaft starre Verbindung zwischen zwei Bauteilen, welche jegliche Verschiebung oder Drehung verhindert.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="89"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ist ein Bauteil aktiv: Unterbauteile werden durch Angleichen der ausgewählten Koordinatensysteme zueinander ausgerichtet. Das als zweites ausgewählte Bauteil wird sich bewegen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="111"/>
      <source>Create Revolute Joint</source>
      <translation>Drehverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="118"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Erzeuge eine Drehverbindung: Erlaubt die Drehung um eine Achse zwischen zwei ausgwählten Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="139"/>
      <source>Create Cylindrical Joint</source>
      <translation>Zylindrische Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="146"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Erzeugt eine zylindrische Verbindung: Erlaubt die Drehung um eine Achse ohne lineare Bewegung entlang derselben Achse zwischen ausgewählen Bauteilen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="165"/>
      <source>Create Slider Joint</source>
      <translation>Gleitverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="172"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Erzeugt eine Gleitverbindung: Erlaubt lineare Bewegungen entlang einer Achse und schränkt die Drehung zwischen den ausgewählen Bauteilen ein.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="191"/>
      <source>Create Ball Joint</source>
      <translation>Kugelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="198"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Erzeugt eine Kugelverbindung: Verbindet Bauteile an einem Punkt und erlaubt uneingeschränkte Bewegung, solange die Punkte in Kontakt bleiben.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="217"/>
      <source>Create Distance Joint</source>
      <translation>Parallelverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="224"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Erzeuge eine Parallelverbindung: Fixiert die Entfernung zwischen den ausgewählten Objekten.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="230"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Einen von mehreren Verbindungen aus der Auswahl erstellen. Ein Abstand von 0 zwischen einer Ebene und einem Zylinder erstellt eine tangentiale Verbindung. Ein Abstand von 0 zwischen zwei Ebenen macht diese koplanar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="496"/>
      <source>Toggle grounded</source>
      <translation>Festsetzen umschalten</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="503"/>
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
      <location filename="../../../CommandExportASMT.py" line="52"/>
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
      <translation>Baugruppe</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="107"/>
      <source>Assembly</source>
      <translation>Baugruppe</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly Joints</source>
      <translation>Assembly-Verbindungen</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="111"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assembly</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Starr</translation>
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
      <translation>Senkrecht</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
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
      <translation>Schraube</translation>
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
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Nachfragen</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Immer</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Nie</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="184"/>
      <source>The type of the joint</source>
      <translation>Der Typ der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="203"/>
      <source>The first object of the joint</source>
      <translation>Das erste Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="211"/>
      <source>The first part of the joint</source>
      <translation>Das erste Bauteil der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="219"/>
      <source>The selected element of the first object</source>
      <translation>Das ausgewählte Element des ersten Objekts</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="227"/>
      <source>The selected vertex of the first object</source>
      <translation>Der ausgewählte Punkt des ersten Objekts</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="238"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem des ersten Objekts, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="249"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der ersten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="258"/>
      <source>The second object of the joint</source>
      <translation>Das zweite Objekt der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="266"/>
      <source>The second part of the joint</source>
      <translation>Das zweite Bauteil der Verbindung</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="274"/>
      <source>The selected element of the second object</source>
      <translation>Das ausgewählte Element des zweiten Objekts</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="282"/>
      <source>The selected vertex of the second object</source>
      <translation>Der ausgewählte Punkt des zweiten Objekts</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="293"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Dies ist das lokale Koordinatensystem des zweiten Objekts, welches für die Verbindung verwendet wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="304"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Dies verhindert die Neuberechnung der zweiten Platzierung, wodurch eine benuzerdefinierte Platzierung ermöglicht wird.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="315"/>
      <source>This is the distance of the joint. It is used only by the distance joint and by RackPinion (pitch radius), Screw and Gears and Belt(radius1)</source>
      <translation>Dies ist der Abstand der Verbindung. Dieser wird nur von der Parallelverbindung, Zahnstange-Ritzel-Verbindung (Steigungsradius), Schraubverbindung, Zahnrad- und Riemenverbindung (Radius 1) verwendet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="326"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Dies ist der zweite Abstand der Verbindung. Er wird nur von der Zahnrad- und Riemenverbindung für den zweiten Radius verwendert.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="337"/>
      <source>This is the rotation of the joint.</source>
      <translation>Dies ist die Drehung der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="348"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Dies ist der Versatz-Vektor der Verbindung.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="359"/>
      <source>This indicates if the joint is active.</source>
      <translation>Dies zeigt an, ob die Verbindung aktiv ist.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="371"/>
      <source>Is this joint using limits.</source>
      <translation>Nutzt diese Verbindung Grenzwerte.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="383"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="394"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Abstand zwischen beiden Koordinatensystemen (entlang ihrer Z-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="405"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der minimale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="416"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Dies ist der maximale Grenzwert für den Winkel zwischen beiden Koordinatensystemen (zwischen ihrer X-Achse).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="959"/>
      <source>The object to ground</source>
      <translation>Das festzusetzende Objekt</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="971"/>
      <source>This is where the part is grounded.</source>
      <translation>Dies ist die Position, an der das Objekt festgesetzt wird.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="230"/>
      <source>The object moved by the move</source>
      <translation>Das Objekt wurde durch die Verschiebung bewegt</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="237"/>
      <source>The containing parts of objects moved by the move</source>
      <translation>Die enthaltenen Bauteile von Objekten wurden durch die Verschiebung bewegt</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="247"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Dies ist die Verschiebung der Bewegung. Die Endplatzierung ist das Ergebnis aus der Startplatzierung * dieser Platzierung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Parallelverbindung</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Radius 2</source>
      <translation>Radius 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Offset</source>
      <translation>Versetzen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="100"/>
      <source>Rotation</source>
      <translation>Drehverbindung</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="128"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Die Richtung der Verbindung umkehren.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="131"/>
      <source>Reverse</source>
      <translation>Umkehren</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="142"/>
      <source>Limits</source>
      <translation>Grenzwerte</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="152"/>
      <source>Length min</source>
      <translation>Min. Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="172"/>
      <source>Length max</source>
      <translation>Max. Länge</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="192"/>
      <source>Angle min</source>
      <translation>Min. Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="212"/>
      <source>Angle max</source>
      <translation>Max. Winkel</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="232"/>
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
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Bauteil nicht gefunden? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Datei öffnen</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Wenn aktiviert, wird die Liste nur Bauteile anzeigen.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
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
      <source>Esc leave edit mode</source>
      <translation>Mit Esc den Bearbeitungsmodus verlassen</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Erstes Bauteil festsetzen:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Wenn das erste Bauteil in die Baugruppe eingefügt wird, kann es automatisch festgesetzt werden.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>Delete associated joints</source>
      <translation>Zugehörige Verbindungen löschen</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="160"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Das Objekt gehört zu einer oder mehreren Verbindungen.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="162"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Soll das Objekt bewegt und zugehörige Verbindungen gelöscht werden?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="762"/>
      <source>Move part</source>
      <translation>Bauteil verschieben</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="331"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Zahnstange-Ritzel-Verbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="338"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Zahnstange-Ritzel-Verbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Dieselben Koordinatensysteme auswählen wie für die Dreh- und Gleitverbindung. Der Teilkreisradius (pitch radius) definiert das Bewegungsverhältnis zwischen Zahnstange und Ritzel.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="362"/>
      <source>Create Screw Joint</source>
      <translation>Schraubverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="369"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Erstellt eine Schraubverbindung: Koppelt ein Bauteil mit Gleitverbindung mit einem Bauteil mit Drehverbindung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Dieselben Koordinatensysteme wie die Dreh- ud Gleitverbindung auswählen. Der Steigungsradius definiert das Bewegungsverhältnis zwischen rotierender Schraube und dem gleitenden Bauteil.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Dieselben Koordinatensysteme wie für die Drehverbindungen auswählen.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="393"/>
      <source>Create Gears Joint</source>
      <translation>Zahnradverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="400"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Erstellt eine Zahnradverbindung: Koppelt die Drehungen zweier Zahnräder, d.h. die Drehrichtungen sind entgegengesetzt.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="424"/>
      <source>Create Belt Joint</source>
      <translation>Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="431"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Erstellt eine Riemenverbindung: Koppelt die Drehungen zweier (Riemen-)Scheiben, d.h. die Drehrichtungen sind gleich.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="456"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Zahnrad-/Riemenverbindung erstellen</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="462"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Eine Zahnrad/Riemen Verbindung erstellen: Verknüpft zwei rotierende Objekte mittels Übersetzung.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
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
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="249"/>
      <source>Create Parallel Joint</source>
      <translation type="unfinished">Create Parallel Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="256"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation type="unfinished">Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="277"/>
      <source>Create Perpendicular Joint</source>
      <translation type="unfinished">Create Perpendicular Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="284"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation type="unfinished">Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="303"/>
      <source>Create Angle Joint</source>
      <translation type="unfinished">Create Angle Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="310"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation type="unfinished">Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</translation>
    </message>
  </context>
</TS>
