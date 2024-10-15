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
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>Create a Fixed Joint</source>
      <translation>Crea un giunto fisso</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="88"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Se un assieme è attivo: crea un giunto che blocca in modo permanente due parti insieme, impedendo qualsiasi movimento o rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="94"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Se una parte è attiva: posiziona le parti in corrispondenza dei sistemi di coordinate selezionati. La seconda parte selezionata si sposterà.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="116"/>
      <source>Create Revolute Joint</source>
      <translation>Crea giunto di rotazione</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="123"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Crea un giunto di rotazione: permette la rotazione attorno a un singolo asse tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="144"/>
      <source>Create Cylindrical Joint</source>
      <translation>Crea giunto cilindrico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="151"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Crea un giunto cilindrico: abilita la rotazione lungo un asse consentendo il movimento lungo lo stesso asse tra le parti assemblate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="170"/>
      <source>Create Slider Joint</source>
      <translation>Crea giunto di scorrimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="177"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Crea un giunto di scorrimento: permette il movimento lineare lungo un singolo asse ma limita la rotazione tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="196"/>
      <source>Create Ball Joint</source>
      <translation>Crea giunto sferico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="203"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Crea un giunto sferico: connette parti in un punto, consentendo un movimento senza restrizioni finché i punti di connessione rimangono in contatto.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="222"/>
      <source>Create Distance Joint</source>
      <translation>Crea giunto parallelo</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="229"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Crea un giunto parallelo: fissa la distanza tra gli oggetti selezionati.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="235"/>
      <source>Create one of several different joints based on the selection.For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation>Crea uno più vincoli in base alla selezione. Ad esempio, una distanza di 0 tra un piano e un cilindro crea un vincolo di tangenza. Una distanza di 0 tra i piani li renderà complanari.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="506"/>
      <source>Toggle grounded</source>
      <translation>Attiva/disattiva fissaggio</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="513"/>
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
      <location filename="../../../CommandExportASMT.py" line="51"/>
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
      <location filename="../../../InitGui.py" line="108"/>
      <source>Assembly</source>
      <translation>Assembly</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly Joints</source>
      <translation>Giunti di assieme</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="112"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assieme</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="46"/>
      <source>Fixed</source>
      <translation>Fissa</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="47"/>
      <source>Revolute</source>
      <translation>Di rotazione</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Cylindrical</source>
      <translation>Cilindrico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Slider</source>
      <translation>Scorrimento</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Ball</source>
      <translation>Sferico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <location filename="../../../JointObject.py" line="1582"/>
      <source>Distance</source>
      <translation>Distanza</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Parallel</source>
      <translation>Parallelo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Perpendicular</source>
      <translation>Perpendicolare</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <location filename="../../../JointObject.py" line="1584"/>
      <source>Angle</source>
      <translation>Angolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <source>RackPinion</source>
      <translation>Cremagliera-Pignone</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Screw</source>
      <translation>Vite</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Gears</source>
      <translation>Ingranaggio</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <source>Belt</source>
      <translation>Cinghia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1433"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>È necessario selezionare 2 elementi da 2 parti separate.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1586"/>
      <source>Radius 1</source>
      <translation>Raggio 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1588"/>
      <source>Pitch radius</source>
      <translation type="unfinished">Pitch radius</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Ask</source>
      <translation>Chiedi</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="50"/>
      <source>Always</source>
      <translation>Sempre</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="51"/>
      <source>Never</source>
      <translation>Mai</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="46"/>
      <source>Index (auto)</source>
      <translation>Indice (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="47"/>
      <source>Name (auto)</source>
      <translation>Nome (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="48"/>
      <source>Description</source>
      <translation>Descrizione</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="49"/>
      <source>File Name (auto)</source>
      <translation>Nome File (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="50"/>
      <source>Quantity (auto)</source>
      <translation>Quantità (auto)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="197"/>
      <source>Default</source>
      <translation>Predefinito</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="293"/>
      <source>Duplicate Name</source>
      <translation>Nome duplicato</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="294"/>
      <source>This name is already used. Please choose a different name.</source>
      <translation>Questo nome è già in uso. Scegli un nome diverso.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="373"/>
      <source>Options:</source>
      <translation>Opzioni:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="380"/>
      <source>Sub-assemblies children : If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Sottoassiemi figli: Se selezionato, i Sottoassiemi figli saranno aggiunti alla distinta dei materiali.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="386"/>
      <source>Parts children : If checked, Parts children will be added to the bill of materials.</source>
      <translation>Part figli : Se selezionato, i Part figli saranno aggiunti alla distinta dei materiali.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="392"/>
      <source>Only parts : If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Solo parti: se selezionato, solo contenitori e sottoinsiemi di parti saranno aggiunti alla distinta materiali. Solidi come corpi di PartDesign, elementi di fissaggio o parti primitivi saranno ignorati.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="394"/>
      <source>Columns:</source>
      <translation>Colonne:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="401"/>
      <source>Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.</source>
      <translation>Colonne automatiche : (indice, quantità, nome...) sono popolate automaticamente. Qualsiasi modifica apportata verrà sovrascritta. Queste colonne non possono essere rinominate.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="407"/>
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation>Colonne personalizzate: 'Descrizione' ed altre colonne personalizzate che si aggiungono facendo clic su 'Aggiungi colonna' non avranno i loro dati sovrascritti. Queste colonne possono essere rinominate facendo doppio clic o premendo F2 (Rinominando una colonna si perderanno i dati).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Qualsiasi colonna (personalizzata o no) può essere cancellata premendo Del.</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="188"/>
      <source>The type of the joint</source>
      <translation>Il tipo di giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="212"/>
      <location filename="../../../JointObject.py" line="462"/>
      <source>The first reference of the joint</source>
      <translation>Il primo riferimento del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="223"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del primo oggetto che sarà utilizzato per il giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="245"/>
      <location filename="../../../JointObject.py" line="521"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>Questo è lo spostamento del collegamento del primo connettore del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="254"/>
      <location filename="../../../JointObject.py" line="487"/>
      <source>The second reference of the joint</source>
      <translation>Il primo riferimento del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="265"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del secondo oggetto di riferimento che verrà utilizzato per il giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="287"/>
      <location filename="../../../JointObject.py" line="531"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>Questo è lo spostamento del collegamento del secondo connettore del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="432"/>
      <source>The first object of the joint</source>
      <translation>Il primo oggetto del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="234"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement1, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="451"/>
      <source>The second object of the joint</source>
      <translation>Il secondo oggetto del vincolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement2, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="299"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>Questa è la distanza del giunto. È usata solo dal vincolo di distanza e da Cremagliera-Pignone (raggio di passo), Vite e ingranaggi e cinghia (raggio1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="310"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Questa è la seconda distanza del vincolo. È usata solo dal vincolo per memorizzare il secondo raggio.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="321"/>
      <source>This indicates if the joint is active.</source>
      <translation>Questo indica se il vincolo è attivo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="333"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Abilita il limite minimo di lunghezza del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="345"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Abilita il limite massimo di lunghezza del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="357"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Abilita il limite minimo di angolo del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="369"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Abilita la lunghezza minima del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="381"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite minimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="392"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite massimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="403"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite minimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite massimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1088"/>
      <source>The object to ground</source>
      <translation>L'oggetto è fissato</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1100"/>
      <source>This is where the part is grounded.</source>
      <translation>Questo è dove la parte è fissata.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="248"/>
      <location filename="../../../CommandCreateView.py" line="282"/>
      <source>The objects moved by the move</source>
      <translation>Gli oggetti spostati dal movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="259"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Questo è lo spostamento del movimento. Il posizionamento finale è il risultato di posizionamento iniziale * questo posizionamento.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="267"/>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="41"/>
      <source>Distance</source>
      <translation>Distanza</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="65"/>
      <source>Radius 2</source>
      <translation>Raggio 2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="89"/>
      <source>Offset</source>
      <translation>Offset</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="113"/>
      <source>Rotation</source>
      <translation>Rotazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="141"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Inverte la direzione del vincolo.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>Reverse</source>
      <translation>Inverti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="155"/>
      <source>Limits</source>
      <translation>Limiti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="161"/>
      <source>Min length</source>
      <translation>Lunghezza minima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="184"/>
      <source>Max length</source>
      <translation>Lunghezza massima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Min angle</source>
      <translation>Angolo minimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max angle</source>
      <translation>Angolo massimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="268"/>
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
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Don't find your part? </source>
      <translation>Non trovi la tua parte? </translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="46"/>
      <source>Open file</source>
      <translation>Apri file</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="55"/>
      <source>If checked, the list will show only Parts.</source>
      <translation>Se selezionato, l'elenco mostrerà solo le parti.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="58"/>
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
      <source>Esc leaves edit mode</source>
      <translation>Il tasto Esc lascia la modalità di modifica</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Log the dragging steps of the solver. Useful if you want to report a bug.
The files are named "runPreDrag.asmt" and "dragging.log" and are located in the default directory of std::ofstream (on Windows it's the desktop)</source>
      <translation>Registra i passaggi di trascinamento del risolutore. Utile se vuoi segnalare un bug.
I file sono denominati "runPreDrag. asmt" e "dragging.log" e si trovano nella directory predefinita di std::ofstream (su Windows è il desktop)</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="43"/>
      <source>Log dragging steps</source>
      <translation>Registra passi di trascinamento</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="59"/>
      <source>Ground first part:</source>
      <translation>Fissa al prima parte:</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="66"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Quando si inserisce la prima parte nell'assieme, è possibile scegliere di fissare la parte automaticamente.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="180"/>
      <source>The object is associated to one or more joints.</source>
      <translation>L'oggetto è associato a uno o più vincoli.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="182"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Si desidera spostare l'oggetto ed eliminare i vincoli associati?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="871"/>
      <source>Move part</source>
      <translation>Sposta parte</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="336"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Crea un vincolo Cremagliera e Pignone</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="343"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Crea un vincolo Cremagliera e Pignone: collega una parte con un vincolo di scorrimento con una parte con un vincolo di rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="348"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Seleziona gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la cremagliera e il pignone.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="367"/>
      <source>Create Screw Joint</source>
      <translation>Crea vincolo di Vite</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="374"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Creare un vincolo di vite: collega una parte con un giunto di scorrimento con una parte con un giunto a rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="379"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la vite rotante e la parte scorrevole.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="410"/>
      <location filename="../../../CommandCreateJoint.py" line="441"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="398"/>
      <source>Create Gears Joint</source>
      <translation>Crea vincolo di ingranaggi</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="405"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Crea un vincolo di ingranaggi: collega due ingranaggi rotanti insieme. Avranno una direzione di rotazione inversa.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="429"/>
      <source>Create Belt Joint</source>
      <translation>Crea vincolo di cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="436"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Crea un vincolo di cinghia: collega due oggetti rotanti insieme. Avranno la stessa direzione di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="461"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Crea vincolo Ingranaggio/Cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="467"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Crea un vincolo di ingranaggi/cinghia: collega due ingranaggi rotanti insieme.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="472"/>
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
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="14"/>
      <source>Create Bill Of Materials</source>
      <translation>Crea una distinta base</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="20"/>
      <source>If checked, Sub assemblies children will be added to the bill of materials.</source>
      <translation>Se selezionato, i Sottoassiemi figli saranno aggiunti alla distinta dei materiali.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="23"/>
      <source>Sub-assemblies children</source>
      <translation>Sottoassiemi figli</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="39"/>
      <source>If checked, Parts children will be added to the bill of materials.</source>
      <translation>Se selezionato, i Part figli saranno aggiunti alla distinta dei materiali.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="42"/>
      <source>Parts children</source>
      <translation>Part figli</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="58"/>
      <source>If checked, only Part containers and sub-assemblies will be added to the bill of materials. Solids like PartDesign Bodies, fasteners or Part workbench primitives will be ignored.</source>
      <translation>Se selezionato, alla distinta materiali saranno aggiunti solo contenitori e sottoinsiemi parziali. Solidi come corpi PartDesign, elementi di fissaggio o parti primitivi saranno ignorati.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="61"/>
      <source>Only parts</source>
      <translation>Solo parti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="77"/>
      <source>Columns</source>
      <translation>Colonne</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="86"/>
      <source>Add column</source>
      <translation>Aggiungi una colonna</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="96"/>
      <source>Export</source>
      <translation>Esporta</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateBom.ui" line="109"/>
      <source>Help</source>
      <translation>Aiuto</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointParallel</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="254"/>
      <source>Create Parallel Joint</source>
      <translation>Crea Vincolo di Parallelismo</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="261"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Crea un Vincolo di Parallelismo: Rende paralleli gli assi Z dei sistemi di coordinate selezionati.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="282"/>
      <source>Create Perpendicular Joint</source>
      <translation>Crea Vincolo Perpendicolare</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="289"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Crea un Vincolo Perpendicolare: Rende perpendicolari gli assi Z dei sistemi di coordinate selezionati.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="308"/>
      <source>Create Angle Joint</source>
      <translation>Crea Vincolo Angolare</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="315"/>
      <source>Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.</source>
      <translation>Crea un Vincolo Angolare: Fissa l'angolo tra gli assi Z dei sistemi di coordinate selezionati.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateBom</name>
    <message>
      <location filename="../../../CommandCreateBom.py" line="69"/>
      <source>Create Bill of Materials</source>
      <translation>Crea una distinta base</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="76"/>
      <source>Create a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.</source>
      <translation>Crea una distinta materiali dell'assemblaggio corrente. Se un assemblaggio è attivo, sarà un BOM di questa assieme. Altrimenti sarà un BOM dell'intero documento.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="81"/>
      <source>The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.</source>
      <translation>L'oggetto BOM è un oggetto documento che memorizza le impostazioni del proprio BOM. È anche un oggetto foglio di calcolo in modo da poter visualizzare facilmente il BOM. Se non si ha bisogno dell'oggetto BOM per essere salvato come oggetto documento, è possibile semplicemente esportare e annullare l'attività.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="86"/>
      <source>The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.</source>
      <translation>Le colonne 'Indice', 'Nome', 'Nome File' e 'Quantità' vengono generate automaticamente al momento del calcolo. Le colonne 'Descrizione' e quelle personalizzate non vengono sovrascritte.</translation>
    </message>
  </context>
</TS>
