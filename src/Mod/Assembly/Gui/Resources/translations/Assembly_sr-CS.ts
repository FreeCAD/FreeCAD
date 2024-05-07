<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="sr-CS" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Napravi sklop</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Napravi glavni sklop u korenu stabla dokumenta, ili podsklop u trenutno aktivnom sklopu. Može postojati samo jedan glavni sklop.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="68"/>
      <source>Create a Fixed Joint</source>
      <translation>Napravi fiksni zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="75"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Ako je aktivan sklop: Napravi zglob koji zajedno spaja dva dela, sprečavajući bilo kakvo kretanje ili rotaciju.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Ako je aktivan deo: Pozicionira delove tako što će napraviti podudarnim izabrane koordinatne sisteme. Drugi izabrani deo će se pomeriti.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="103"/>
      <source>Create Revolute Joint</source>
      <translation>Napravi rotacioni zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="110"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Napravi rotacioni zglob (Kinematski par V klase) između izabranih delova: Dozvoljava rotaciju oko jedne ose.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="131"/>
      <source>Create Cylindrical Joint</source>
      <translation>Napravi cilindrični zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="138"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Napravi cilindrični zglob (Kinematski par IV klase) izmeđi izabranih delova: Dozvoljava rotaciju oko jedne ose i translaciju po istoj osi.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="157"/>
      <source>Create Slider Joint</source>
      <translation>Napravi translatorni zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="164"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Napravi translatorni zglob (Kinematski par V klase) između izabranih delova: Dozvoljava jednu translaciju.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="183"/>
      <source>Create Ball Joint</source>
      <translation>Napravi kuglasti zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="190"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Napravi kuglasti zglob (Kinematski par III klase) između izabranih delova: Dozvoljava rotaciju oko sve tri ose.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="209"/>
      <source>Create Distance Joint</source>
      <translation>Napravi ravanski zglob</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="216"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Napravi ravanski zglob (kinematski par III klase) između izabanih delova: Dozvoljava rotaciju oko jedne ose i translaciju po dve ose.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Toggle grounded</source>
      <translation>Učvrsti</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Učvršćavanje trajno fiksira poziciju dela u sklopu, sprečavajući bilo kakvu translaciju ili rotaciju. Potreban je najmanje jedan učvršćen deo pre nego što se počne formirati sklop.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Izvezi ASMT datoteku</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Izvezi trenutno aktivan sklop kao ASMT datoteku.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="52"/>
      <source>Insert Link</source>
      <translation>Umetni vezu</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="59"/>
      <source>Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Umetnite vezu u trenutno aktivni sklop. Ovo će stvoriti dinamičke veze ka delovima/telima/primitivima/sklopovima. Da biste umetnuli spoljne objekte, uverite se da je datoteka &lt;b&gt;trenutno otvorena&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="61"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Umetnuti levim klikom na stavke u listi.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="65"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Ukloniti desnim klikom na stavke u listi.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="70"/>
      <source>Press shift to add several links while clicking on the view.</source>
      <translation type="unfinished">Press shift to add several links while clicking on the view.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Reši sklop</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Reši trenutno aktivni sklop.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation type="unfinished">Assembly</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="98"/>
      <source>Assembly</source>
      <translation type="unfinished">Assembly</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="99"/>
      <source>Assembly Joints</source>
      <translation>Zglobovi sklopa</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="102"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Sklop</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Učvršćen</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Rotacioni</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Cilindrični</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Translacija</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Kuglasti</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Distance</source>
      <translation>Rastojanje</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Pitaj</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Uvek</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Nikada</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="116"/>
      <source>The type of the joint</source>
      <translation>Vrsta zgloba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="126"/>
      <source>The first object of the joint</source>
      <translation>Prvi objekat zgloba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="133"/>
      <source>The first part of the joint</source>
      <translation>Prvi deo zgloba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="140"/>
      <source>The selected element of the first object</source>
      <translation>Izabrani element prvog objekta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="147"/>
      <source>The selected vertex of the first object</source>
      <translation>Izabrano teme prvog objekta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="157"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Ovo je lokalni koordinatni sistem unutar object1 koji će se koristiti u zglobu.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="167"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ovo sprečava da se ponovo izračunava Placement1 omogućavajući sopstveno pozicioniranje.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="175"/>
      <source>The second object of the joint</source>
      <translation>Drugi objekat zgloba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="182"/>
      <source>The second part of the joint</source>
      <translation>Drugi deo zgloba</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <source>The selected element of the second object</source>
      <translation>Izabrani element drugog objekta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="196"/>
      <source>The selected vertex of the second object</source>
      <translation>Izabrano teme drugog objekta</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Ovo je lokalni koordinatni sistem unutar object2 koji će se koristiti u zglobu.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Ovo sprečava da se ponovo izračunava Placement2 omogućavajući sopstveno pozicioniranje.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="226"/>
      <source>This is the distance of the joint. It is used only by the distance joint.</source>
      <translation>Rastojanje između izabranih ravni delova koji čine kinematski par.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="236"/>
      <source>This is the rotation of the joint.</source>
      <translation>Rotacija zgloba.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="246"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Vektor odmaka zgloba,.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="256"/>
      <source>This indicates if the joint is active.</source>
      <translation>Ovo pokazuje da li je zglob aktivan.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="889"/>
      <source>The object to ground</source>
      <translation>Objekat koji treba učvrstiti</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="901"/>
      <source>This is where the part is grounded.</source>
      <translation>Ovde je mesto gde je deo učvršćen.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Napravi zglob</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Rastojanje</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Offset</source>
      <translation>Odmak</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Rotation</source>
      <translation>Rotacija</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="104"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Obrni smer zgloba.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="107"/>
      <source>Reverse</source>
      <translation>Obrnuto</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Link</source>
      <translation>Umetni vezu</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Pretraga delova...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation type="unfinished">Don't find your part? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Otvori datoteku</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the selected object will be inserted inside a Part container, unless it is already a Part.</source>
      <translation>Ako je označeno, izabrani objekat će biti umetnut unutar kontejnera Part, osim ako već nije Part.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Insert as part</source>
      <translation>Umetni kao deo</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Opšte</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Napušti režim uređivanja pritiskom na tipku Esc</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leave edit mode</source>
      <translation>Esc napusti režim uređivanja</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Učvrsti prvi deo:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Da li da prvi deo koji se umeće u sklop treba da bude učvršćen.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="136"/>
      <source>Delete associated joints</source>
      <translation>Obriši pridružene zglobove</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>The object is associated to one or more joints.</source>
      <translation>Objektu su pridruženi jedan ili više zglobova.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Da li želiš pomeriti objekat i obrisati pridružene zglobove?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="651"/>
      <source>Move part</source>
      <translation>Pomeri deo</translation>
    </message>
  </context>
</TS>
