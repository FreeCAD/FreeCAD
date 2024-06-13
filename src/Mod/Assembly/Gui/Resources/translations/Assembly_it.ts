<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="it" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Crea Assieme</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Crea un oggetto di assieme nel documento corrente o nell'assieme attivo (se presente). Il limite è di un assieme principale per file.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="76"/>
      <source>Create a Fixed Joint</source>
      <translation>Crea un giunto fisso</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="83"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Se un assieme è attivo: crea un giunto che blocca in modo permanente due parti insieme, impedendo qualsiasi movimento o rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="89"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Se una parte è attiva: posiziona le parti in corrispondenza dei sistemi di coordinate selezionati. La seconda parte selezionata si sposterà.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="111"/>
      <source>Create Revolute Joint</source>
      <translation>Crea giunto di rotazione</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="118"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Crea un giunto di rotazione: permette la rotazione attorno a un singolo asse tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="139"/>
      <source>Create Cylindrical Joint</source>
      <translation>Crea giunto cilindrico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="146"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Crea un giunto cilindrico: abilita la rotazione lungo un asse consentendo il movimento lungo lo stesso asse tra le parti assemblate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="165"/>
      <source>Create Slider Joint</source>
      <translation>Crea giunto di scorrimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="172"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Crea un giunto di scorrimento: permette il movimento lineare lungo un singolo asse ma limita la rotazione tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="191"/>
      <source>Create Ball Joint</source>
      <translation>Crea giunto sferico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="198"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Crea un giunto sferico: connette parti in un punto, consentendo un movimento senza restrizioni finché i punti di connessione rimangono in contatto.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="217"/>
      <source>Create Distance Joint</source>
      <translation>Crea giunto parallelo</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="224"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Crea un giunto parallelo: fissa la distanza tra gli oggetti selezionati.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="230"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Crea uno più vincoli in base alla selezione. Ad esempio, una distanza di 0 tra un piano e un cilindro crea un vincolo di tangenza. Una distanza di 0 tra i piani li renderà complanari.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="496"/>
      <source>Toggle grounded</source>
      <translation>Attiva/disattiva fissaggio</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="503"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Il fissaggio di una parte blocca permanentemente la sua posizione nel montaggio, impedendo qualsiasi movimento o rotazione. Hai bisogno di almeno una parte messa a terra prima di iniziare ad assemblare.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Esporta File ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Esporta l'assieme attivo come file ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert Component</source>
      <translation>Inserisci componente</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Inserisce un componente nell'insieme attivo. Questo creerà collegamenti dinamici alle parti, corpi, primitive e assiemi. Per inserire componenti esterni, assicurati che il file sia &lt;b&gt;aperto nella sessione corrente&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="62"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Inserisci facendo clic con il tasto sinistro degli elementi nella lista.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="66"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Rimuovi facendo clic con il tasto destro sugli elementi nella lista.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="71"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Premere il tasto Maiusc per aggiungere diverse istanze del componente mentre si fa clic sulla vista.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Risolvi l'assieme</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Risolvi l'assieme attivo.</translation>
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
      <location filename="../../../InitGui.py" line="107"/>
      <source>Assembly</source>
      <translation>Assembly</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly Joints</source>
      <translation>Giunti di assieme</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="111"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assieme</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Fissa</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Di rotazione</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Cilindrico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Scorrimento</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Sferico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Distance</source>
      <translation>Distanza</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Parallel</source>
      <translation>Parallelo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>Perpendicular</source>
      <translation>Perpendicolare</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Angle</source>
      <translation>Angolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>RackPinion</source>
      <translation>Cremagliera-Pignone</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Screw</source>
      <translation>Vite</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>Gears</source>
      <translation>Ingranaggio</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Belt</source>
      <translation>Cinghia</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Chiedi</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Sempre</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Mai</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="184"/>
      <source>The type of the joint</source>
      <translation>Il tipo di giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="203"/>
      <source>The first object of the joint</source>
      <translation>Il primo oggetto del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="211"/>
      <source>The first part of the joint</source>
      <translation>La prima parte del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="219"/>
      <source>The selected element of the first object</source>
      <translation>L'elemento selezionato del primo oggetto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="227"/>
      <source>The selected vertex of the first object</source>
      <translation>Il vertice selezionato del primo oggetto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="238"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del primo oggetto che sarà utilizzato per il vincolo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="249"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement1, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="258"/>
      <source>The second object of the joint</source>
      <translation>Il secondo oggetto del vincolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="266"/>
      <source>The second part of the joint</source>
      <translation>La seconda parte del vincolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="274"/>
      <source>The selected element of the second object</source>
      <translation>L'elemento selezionato del secondo oggetto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="282"/>
      <source>The selected vertex of the second object</source>
      <translation>Il vertice selezionato del secondo oggetto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="293"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del secondo oggetto che sarà utilizzato per il vincolo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="304"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement2, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="315"/>
      <source>This is the distance of the joint. It is used only by the distance joint and by RackPinion (pitch radius), Screw and Gears and Belt(radius1)</source>
      <translation>Questa è la distanza del vincolo. È usata solo dal vincolo di distanza e da Cremagliera-Pignone (raggio del passo), Vite e ingranaggi e cinghia (raggio1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="326"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Questa è la seconda distanza del vincolo. È usata solo dal vincolo per memorizzare il secondo raggio.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="337"/>
      <source>This is the rotation of the joint.</source>
      <translation>Questa è la rotazione del vincolo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="348"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Questo è il vettore di offset del vincolo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="359"/>
      <source>This indicates if the joint is active.</source>
      <translation>Questo indica se il vincolo è attivo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="371"/>
      <source>Is this joint using limits.</source>
      <translation type="unfinished">Is this joint using limits.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="383"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite minimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="394"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite massimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="405"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite minimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="416"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite massimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="959"/>
      <source>The object to ground</source>
      <translation>L'oggetto è fissato</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="971"/>
      <source>This is where the part is grounded.</source>
      <translation>Questo è dove la parte è fissata.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="230"/>
      <source>The object moved by the move</source>
      <translation>L'oggetto spostato dal movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="237"/>
      <source>The containing parts of objects moved by the move</source>
      <translation>Le parti contenute negli oggetti spostati dal movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="247"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Questo è lo spostamento del movimento. Il posizionamento finale è il risultato di posizionamento iniziale * questo posizionamento.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
      <source>The type of the move</source>
      <translation>Il tipo di movimento</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Crea Vincolo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Distanza</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Radius 2</source>
      <translation>Raggio 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Offset</source>
      <translation>Offset</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="100"/>
      <source>Rotation</source>
      <translation>Rotazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="128"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Inverte la direzione del vincolo.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="131"/>
      <source>Reverse</source>
      <translation>Inverti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="142"/>
      <source>Limits</source>
      <translation>Limiti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="152"/>
      <source>Length min</source>
      <translation>Lunghezza minima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="172"/>
      <source>Length max</source>
      <translation>Lunghezza massima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="192"/>
      <source>Angle min</source>
      <translation>Angolo minimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="212"/>
      <source>Angle max</source>
      <translation>Angolo massimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="232"/>
      <source>Reverse rotation</source>
      <translation>Inverti rotazione</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Component</source>
      <translation>Inserisci componente</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Cerca parti...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Non trovi la tua parte? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Apri file</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Se selezionato, l'elenco mostrerà solo le parti.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Show only parts</source>
      <translation>Mostra solo le parti</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Generale</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Permette di uscire dalla modalità di modifica quando si preme il tasto Esc</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leave edit mode</source>
      <translation>Esc lascia la modalità di modifica</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Fissa al prima parte:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Quando si inserisce la prima parte nell'assieme, è possibile scegliere di fissare la parte automaticamente.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>Delete associated joints</source>
      <translation>Elimina i vincoli associati</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="160"/>
      <source>The object is associated to one or more joints.</source>
      <translation>L'oggetto è associato a uno o più vincoli.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="162"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Si desidera spostare l'oggetto ed eliminare i vincoli associati?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="762"/>
      <source>Move part</source>
      <translation>Sposta parte</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="331"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Crea un vincolo Cremagliera e Pignone</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="338"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Crea un vincolo Cremagliera e Pignone: collega una parte con un vincolo di scorrimento con una parte con un vincolo di rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Seleziona gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la cremagliera e il pignone.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="362"/>
      <source>Create Screw Joint</source>
      <translation>Crea vincolo di Vite</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="369"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Creare un vincolo di vite: collega una parte con un giunto di scorrimento con una parte con un giunto a rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la vite rotante e la parte scorrevole.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="393"/>
      <source>Create Gears Joint</source>
      <translation>Crea vincolo di ingranaggi</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="400"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Crea un vincolo di ingranaggi: collega due ingranaggi rotanti insieme. Avranno una direzione di rotazione inversa.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="424"/>
      <source>Create Belt Joint</source>
      <translation>Crea vincolo di cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="431"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Crea un vincolo di cinghia: collega due oggetti rotanti insieme. Avranno la stessa direzione di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="456"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Crea vincolo Ingranaggio/Cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="462"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Crea un vincolo di ingranaggi/cinghia: collega due ingranaggi rotanti insieme.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Seleziona gli stessi sistemi di coordinate dei vincoli di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="55"/>
      <source>Create Exploded View</source>
      <translation>Crea Vista Esplosa</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="62"/>
      <source>Create an exploded view of the current assembly.</source>
      <translation>Crea una vista esplosa dell'assieme corrente.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateView</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="14"/>
      <source>Create Exploded View</source>
      <translation>Crea Vista Esplosa</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="20"/>
      <source>If checked, Parts will be selected as a single solid.</source>
      <translation>Se selezionato, le parti saranno selezionate come un solido unico.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="23"/>
      <source>Parts as single solid</source>
      <translation>Parti come solido unico</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="42"/>
      <source>Align dragger</source>
      <translation>Allinea trascinatore</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="49"/>
      <source>Aligning dragger:
Select a feature.
Press ESC to cancel.</source>
      <translation>Allineamento trascinatore:
Seleziona una feature.
Premi ESC per annullare.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateView.ui" line="58"/>
      <source>Explode radially</source>
      <translation>Esplodi radialmente</translation>
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
