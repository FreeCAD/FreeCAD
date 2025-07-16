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
      <location filename="../../../CommandCreateJoint.py" line="77"/>
      <source>Create Fixed Joint</source>
      <translation type="unfinished">Create Fixed Joint</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="84"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Se un assieme è attivo: crea un giunto che blocca in modo permanente due parti insieme, impedendo qualsiasi movimento o rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="90"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Se una parte è attiva: posiziona le parti in corrispondenza dei sistemi di coordinate selezionati. La seconda parte selezionata si sposterà.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="112"/>
      <source>Create Revolute Joint</source>
      <translation>Crea giunto di rotazione</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="119"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Crea un giunto di rotazione: permette la rotazione attorno a un singolo asse tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="140"/>
      <source>Create Cylindrical Joint</source>
      <translation>Crea giunto cilindrico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="147"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Crea un giunto cilindrico: abilita la rotazione lungo un asse consentendo il movimento lungo lo stesso asse tra le parti assemblate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="166"/>
      <source>Create Slider Joint</source>
      <translation>Crea giunto di scorrimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="173"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Crea un giunto di scorrimento: permette il movimento lineare lungo un singolo asse ma limita la rotazione tra le parti selezionate.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="192"/>
      <source>Create Ball Joint</source>
      <translation>Crea giunto sferico</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="199"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Crea un giunto sferico: connette parti in un punto, consentendo un movimento senza restrizioni finché i punti di connessione rimangono in contatto.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="218"/>
      <source>Create Distance Joint</source>
      <translation>Crea Vincolo di Distanziamento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="225"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Crea un giunto di distanziamento: fissa la distanza tra gli oggetti selezionati.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="231"/>
      <source>Create one of several different joints based on the selection. For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</source>
      <translation type="unfinished">Create one of several different joints based on the selection. For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="502"/>
      <source>Toggle grounded</source>
      <translation>Attiva/disattiva vincolo a terra</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="509"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Il fissaggio di una parte blocca permanentemente la sua posizione nel montaggio, impedendo qualsiasi movimento o rotazione. Hai bisogno di almeno una parte messa fissata prima di iniziare ad assemblare.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="46"/>
      <source>Export ASMT File</source>
      <translation>Esporta File ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="50"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Esporta l'assieme attivo come file ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="89"/>
      <source>Insert Component</source>
      <translation>Inserisci componente</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="51"/>
      <source>Insert a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Inserisce un componente nell'insieme attivo. Questo creerà collegamenti dinamici alle parti, corpi, primitive e assiemi. Per inserire componenti esterni, assicurati che il file sia &lt;b&gt;aperto nella sessione corrente&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="53"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Inserisci facendo clic con il tasto sinistro degli elementi nella lista.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="55"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Rimuovi facendo clic con il tasto destro sugli elementi nella lista.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="60"/>
      <source>Press shift to add several instances of the component while clicking on the view.</source>
      <translation>Premere il tasto Maiusc per aggiungere diverse istanze del componente mentre si fa clic sulla vista.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="50"/>
      <source>Solve Assembly</source>
      <translation>Risolvi l'assieme</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="57"/>
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
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="127"/>
      <source>Active object</source>
      <translation>Oggetto attivo</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="127"/>
      <source>Turn flexible</source>
      <translation>Rendi mobile</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="128"/>
      <source>Your sub-assembly is currently rigid. This will make it flexible instead.</source>
      <translation>Il tuo sotto assieme è attualmente rigido. Questo lo renderà invece flessibile.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="132"/>
      <source>Turn rigid</source>
      <translation>Rendi rigido</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="133"/>
      <source>Your sub-assembly is currently flexible. This will make it rigid instead.</source>
      <translation>Il tuo sotto assieme è attualmente mobile. Questo lo renderà invece rigido.</translation>
    </message>
    <message>
      <location filename="../../../App/BomObject.cpp" line="272"/>
      <source>N/A</source>
      <translation type="unfinished">N/A</translation>
    </message>
    <message>
      <location filename="../../../App/BomObject.cpp" line="296"/>
      <source>Not supported</source>
      <translation type="unfinished">Not supported</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="109"/>
      <source>Assembly</source>
      <translation>Assembly</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="110"/>
      <source>Assembly Joints</source>
      <translation>Giunti di assieme</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="113"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assieme</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Fixed</source>
      <translation>Fisso</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Revolute</source>
      <translation>Di rotazione</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Cylindrical</source>
      <translation>Cilindrico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Slider</source>
      <translation>Scorrimento</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="54"/>
      <source>Ball</source>
      <translation>Sferico</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="55"/>
      <location filename="../../../JointObject.py" line="1516"/>
      <source>Distance</source>
      <translation>Distanza</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="56"/>
      <source>Parallel</source>
      <translation>Parallelo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="57"/>
      <source>Perpendicular</source>
      <translation>Perpendicolare</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="58"/>
      <location filename="../../../JointObject.py" line="1518"/>
      <source>Angle</source>
      <translation>Angolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="59"/>
      <source>RackPinion</source>
      <translation>Cremagliera-Pignone</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="60"/>
      <source>Screw</source>
      <translation>Vite</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="61"/>
      <source>Gears</source>
      <translation>Ingranaggio</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="62"/>
      <source>Belt</source>
      <translation>Cinghia</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="624"/>
      <source>Broken link in: </source>
      <translation type="unfinished">Broken link in: </translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1360"/>
      <source>You need to select 2 elements from 2 separate parts.</source>
      <translation>È necessario selezionare 2 elementi da 2 parti separate.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1520"/>
      <source>Radius 1</source>
      <translation>Raggio 1</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="1522"/>
      <source>Pitch radius</source>
      <translation>Raggio del passo</translation>
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
      <translation>Sottoassiemi figli: Se selezionato, i sottoassiemi figli saranno aggiunti alla distinta dei materiali.</translation>
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
      <source>Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</source>
      <translation type="unfinished">Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (Renaming a column will currently lose its data).</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="413"/>
      <source>Any column (custom or not) can be deleted by pressing Del.</source>
      <translation>Qualsiasi colonna (personalizzata o no) può essere cancellata premendo Del.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="415"/>
      <source>Export:</source>
      <translation>Esportazione:</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateBom.py" line="422"/>
      <source>The exported file format can be customized in the Spreadsheet workbench preferences.</source>
      <translation>Il formato di file esportato può essere personalizzato nelle preferenze dell'ambiente di lavoro Spreadsheet.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="84"/>
      <source>Part name</source>
      <translation>Nome parte</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="89"/>
      <source>Part</source>
      <translation>Parte</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="94"/>
      <source>Create part in new file</source>
      <translation>Crea parte in un nuovo file</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="101"/>
      <source>Joint new part origin</source>
      <translation>Collega nuova origine della parte</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="135"/>
      <source>Save Document</source>
      <translation>Salva il documento</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="137"/>
      <source>Save</source>
      <translation>Salva</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="140"/>
      <source>Don't link</source>
      <translation>Non collegare</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="474"/>
      <source>Enter your formula...</source>
      <translation>Inserisci la tua formula...</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="527"/>
      <source>In capital are variables that you need to replace with actual values. More details about each example in it's tooltip.</source>
      <translation>È necessario sostituire le variabili in maiuscolo con valori reali. Maggiori dettagli su ogni esempio nel suo suggerimento.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="530"/>
      <source> - Linear: C + VEL*time</source>
      <translation> - Lineare: C + VEL*tempo</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="532"/>
      <source> - Quadratic: C + VEL*time + ACC*time^2</source>
      <translation> - Quadratico: C + VEL*tempo + ACC*tempo ^2</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="535"/>
      <source> - Harmonic: C + AMP*sin(VEL*time - PHASE)</source>
      <translation> - Armonica: C + AMP*sin(VEL*tempo - PHASE)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="538"/>
      <source> - Exponential: C*exp(time/TIMEC)</source>
      <translation> - Esponenziale: C*exp(tempo/TIMEC)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="544"/>
      <source> - Smooth Step: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(time - T0)))</source>
      <translation> - Passo uniforme: L1 + (L2 - L1)*((1/2) + (1/pi)*arctan(SLOPE*(tempo - T0)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="551"/>
      <source> - Smooth Square Impulse: (H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</source>
      <translation> - Impulso rettangolare liscio: (H/pi)*(arctan(SLOPE*(time - T1)) - arctan(SLOPE*(time - T2)))</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="558"/>
      <source> - Smooth Ramp Top Impulse: ((1/pi)*(arctan(1000*(time - T1)) - arctan(1000*(time - T2))))*(((H2 - H1)/(T2 - T1))*(time - T1) + H1)</source>
      <translation> - Impulso superiore della rampa regolare: ((1/pi)*(arctan(1000*(tempo - T1)) - arctan(1000*(tempo - T2))))*(((H2 - H1)/(T2 - T1))*(tempo - T1) + H1)</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="568"/>
      <source>C is a constant offset.
VEL is a velocity or slope or gradient of the straight line.</source>
      <translation>C è un offset costante.
VEL è la velocità, la pendenza o il gradiente della linea retta.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="576"/>
      <source>C is a constant offset.
VEL is the velocity or slope or gradient of the straight line.
ACC is the acceleration or coefficient of the second order. The function is a parabola.</source>
      <translation>C è un offset costante.
VEL è la velocità, la pendenza o il gradiente della retta.
ACC è l'accelerazione o il coefficiente del secondo ordine. La funzione è una parabola.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="585"/>
      <source>C is a constant offset.
AMP is the amplitude of the sine wave.
VEL is the angular velocity in radians per second.
PHASE is the phase of the sine wave.</source>
      <translation>C è un offset costante.
AMP è l'ampiezza dell'onda sinusoidale.
VEL è la velocità angolare in radianti al secondo.
PHASE è la fase dell'onda sinusoidale.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="592"/>
      <source>C is a constant.
TIMEC is the time constant of the exponential function.</source>
      <translation>C è una costante.
TIMEC è la costante di tempo della funzione esponenziale.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="600"/>
      <source>L1 is step level before time = T0.
L2 is step level after time = T0.
SLOPE defines the steepness of the transition between L1 and L2 about time = T0. Higher values gives sharper cornered steps. SLOPE = 1000 or greater are suitable.</source>
      <translation>L1 è il livello del gradino prima del tempo = T0.
L2 è il livello del gradino dopo il tempo = T0.
SLOPE definisce la ripidità della transizione tra L1 e L2 rispetto al tempo = T0. Valori più alti danno gradini con angoli più netti. SLOPE = 1000 o superiore sono adatti.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="609"/>
      <source>H is the height of the impulse.
T1 is the start of the impulse.
T2 is the end of the impulse.
SLOPE defines the steepness of the transition between 0 and H about time = T1 and T2. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>H è l'altezza dell'impulso.
T1 è l'inizio dell'impulso.
T2 è la fine dell'impulso.
SLOPE definisce la ripidità della transizione tra 0 e H al tempo = T1 e T2. Valori più alti danno impulsi con angoli più netti. SLOPE = 1000 o superiore sono adatti.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="620"/>
      <source>This is similar to the square impulse but the top has a sloping ramp. It is good for building a smooth piecewise linear function by adding a series of these.
T1 is the start of the impulse.
T2 is the end of the impulse.
H1 is the height at T1 at the beginning of the ramp.
H2 is the height at T2 at the end of the ramp.
SLOPE defines the steepness of the transition between 0 and H1 and H2 to 0 about time = T1 and T2 respectively. Higher values gives sharper cornered impulses. SLOPE = 1000 or greater are suitable.</source>
      <translation>È simile all'impulso quadrato, ma la parte superiore ha una rampa inclinata. È utile per costruire una funzione lineare continua ad intervalli sommando una serie di questi impulsi.
T1 è l'inizio dell'impulso.
T2 è la fine dell'impulso.
H1 è l'altezza a T1 all'inizio della rampa.
H2 è l'altezza a T2 alla fine della rampa.
SLOPE definisce la pendenza della transizione tra 0 e H1 e H2 a 0 al tempo = T1 e T2 rispettivamente. Valori più alti danno impulsi con angoli più netti. SLOPE = 1000 o superiore sono adatti.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="658"/>
      <location filename="../../../CommandCreateSimulation.py" line="675"/>
      <source>Help</source>
      <translation>Aiuto</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="673"/>
      <source>Hide help</source>
      <translation>Nascondi aiuto</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="181"/>
      <source>The type of the joint</source>
      <translation>Il tipo di giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>The first reference of the joint</source>
      <translation>Il primo riferimento del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="218"/>
      <source>This is the local coordinate system within Reference1's object that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del primo oggetto che sarà utilizzato per il giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="242"/>
      <location filename="../../../JointObject.py" line="526"/>
      <source>This is the attachment offset of the first connector of the joint.</source>
      <translation>Questo è lo spostamento del collegamento del primo connettore del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="252"/>
      <source>The second reference of the joint</source>
      <translation>Il primo riferimento del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="264"/>
      <source>This is the local coordinate system within Reference2's object that will be used for the joint.</source>
      <translation>Questo è il sistema di coordinate locali all'interno del secondo oggetto di riferimento che verrà utilizzato per il giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="288"/>
      <location filename="../../../JointObject.py" line="537"/>
      <source>This is the attachment offset of the second connector of the joint.</source>
      <translation>Questo è lo spostamento del collegamento del secondo connettore del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="445"/>
      <source>The first object of the joint</source>
      <translation>Il primo oggetto del giunto</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="230"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement1, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="465"/>
      <source>The second object of the joint</source>
      <translation>Il secondo oggetto del vincolo</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="276"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Questo impedisce il ricalcolo di Placement2, consentendo il posizionamento personalizzato.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="301"/>
      <source>This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)</source>
      <translation>Questa è la distanza del giunto. È usata solo dal vincolo di distanza e da Cremagliera-Pignone (raggio di passo), Vite e ingranaggi e cinghia (raggio1)</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="313"/>
      <source>This is the second distance of the joint. It is used only by the gear joint to store the second radius.</source>
      <translation>Questa è la seconda distanza del vincolo. È usata solo dal vincolo per memorizzare il secondo raggio.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="325"/>
      <source>This indicates if the joint is active.</source>
      <translation>Questo indica se il vincolo è attivo.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="338"/>
      <source>Enable the minimum length limit of the joint.</source>
      <translation>Abilita il limite minimo di lunghezza del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="351"/>
      <source>Enable the maximum length limit of the joint.</source>
      <translation>Abilita il limite massimo di lunghezza del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="364"/>
      <source>Enable the minimum angle limit of the joint.</source>
      <translation>Abilita il limite minimo di angolo del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="377"/>
      <source>Enable the minimum length of the joint.</source>
      <translation>Abilita la lunghezza minima del giunto.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="390"/>
      <source>This is the minimum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite minimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="402"/>
      <source>This is the maximum limit for the length between both coordinate systems (along their Z axis).</source>
      <translation>Questo è il limite massimo per la lunghezza tra i due sistemi di coordinate (lungo il loro asse Z).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="414"/>
      <source>This is the minimum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite minimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="426"/>
      <source>This is the maximum limit for the angle between both coordinate systems (between their X axis).</source>
      <translation>Questo è il limite massimo per l'angolo tra i due sistemi di coordinate (tra i loro assi X).</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="479"/>
      <source>The {order} reference of the joint</source>
      <translation type="unfinished">The {order} reference of the joint</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="993"/>
      <source>The object to ground</source>
      <translation>L'oggetto è fissato</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="254"/>
      <location filename="../../../CommandCreateView.py" line="291"/>
      <source>The objects moved by the move</source>
      <translation>Gli oggetti spostati dal movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="266"/>
      <source>This is the movement of the move. The end placement is the result of the start placement * this placement.</source>
      <translation>Questo è lo spostamento del movimento. Il posizionamento finale è il risultato di posizionamento iniziale * questo posizionamento.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="275"/>
      <source>The type of the move</source>
      <translation>Il tipo di movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="107"/>
      <source>Simulation start time.</source>
      <translation>Tempo di avvio simulazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="119"/>
      <source>Simulation end time.</source>
      <translation>Tempo di fine simulazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="131"/>
      <source>Simulation time step for output.</source>
      <translation>Passo temporale di simulazione per l'output.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="143"/>
      <source>Integration global error tolerance.</source>
      <translation>Tolleranza di errore globale dell'integrazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="155"/>
      <source>Frames Per Second.</source>
      <translation>Frame al secondo.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="207"/>
      <source>The number of decimals to use for calculated texts</source>
      <translation>Il numero di decimali da utilizzare per i testi calcolati</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="304"/>
      <source>The joint that is moved by the motion</source>
      <translation>Il giunto mosso dal movimento</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="316"/>
      <source>This is the formula of the motion. For example '1.0*time'.</source>
      <translation>Questa è la formula del movimento. Per esempio '1.0*time'.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="325"/>
      <source>The type of the motion</source>
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
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="137"/>
      <source>Offset1</source>
      <translation>Offset1</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="158"/>
      <source>Offset2</source>
      <translation>Offset2</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="144"/>
      <source>By clicking this button, you can set the attachment offset of the first marker (coordinate system) of the joint.</source>
      <translation>Facendo clic su questo pulsante, è possibile impostare lo spostamento del primo marcatore (sistema di coordinate) del giunto.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="165"/>
      <source>By clicking this button, you can set the attachment offset of the second marker (coordinate system) of the joint.</source>
      <translation>Facendo clic su questo pulsante, è possibile impostare lo spostamento del secondo marcatore (sistema di coordinate) del giunto.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="177"/>
      <source>Show advanced offsets</source>
      <translation>Mostra offset avanzati</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="193"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Inverte la direzione del vincolo.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="196"/>
      <source>Reverse</source>
      <translation>Inverti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="207"/>
      <source>Limits</source>
      <translation>Limiti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="213"/>
      <source>Min length</source>
      <translation>Lunghezza minima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="236"/>
      <source>Max length</source>
      <translation>Lunghezza massima</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="259"/>
      <source>Min angle</source>
      <translation>Angolo minimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="288"/>
      <source>Max angle</source>
      <translation>Angolo massimo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="320"/>
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
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="74"/>
      <source>Sets whether the inserted sub-assemblies will be rigid or flexible.
Rigid means that the added sub-assembly will be considered as a solid unit within the parent assembly.
Flexible means that the added sub-assembly will allow movement of its individual components' joints within the parent assembly.
You can change this behavior at any time by either right-clicking the sub-assembly on the document tree and toggling the
Turn rigid/Turn flexible command there, or by editing its Rigid property in the Property Editor.</source>
      <translation>Imposta se i sottoinsiemi inseriti saranno rigidi o flessibili.
Rigido significa che il sottoinsieme aggiunto sarà considerato come un'unità solida all'interno dell'insieme capostipite.
Flessibile significa che il sottoinsieme aggiunto permetterà il movimento delle articolazioni dei singoli componenti all'interno dell'insieme capostipite.
È possibile modificare questo comportamento in qualsiasi momento facendo clic con il tasto destro del mouse sul sottoassieme nell'albero dei documenti e attivando il comando Imposta rigida/Imposta flessibile lì, o modificando la sua proprietà Rigida nell'editor di proprietà.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="81"/>
      <source>Rigid sub-assemblies</source>
      <translation>Sotto assiemi rigidi</translation>
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
      <source>Allows leaving edit mode when pressing Esc button</source>
      <translation type="unfinished">Allows leaving edit mode when pressing Esc button</translation>
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
      <translation>Fissa la prima parte:</translation>
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
      <location filename="../../ViewProviderAssembly.cpp" line="196"/>
      <source>The object is associated to one or more joints.</source>
      <translation>L'oggetto è associato a uno o più vincoli.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="198"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Si desidera spostare l'oggetto ed eliminare i vincoli associati?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="891"/>
      <source>Move part</source>
      <translation>Sposta parte</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRackPinion</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="332"/>
      <source>Create Rack and Pinion Joint</source>
      <translation>Crea un vincolo Cremagliera e Pignone</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="339"/>
      <source>Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Crea un vincolo Cremagliera e Pignone: collega una parte con un vincolo di scorrimento con una parte con un vincolo di rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="344"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.</source>
      <translation>Seleziona gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la cremagliera e il pignone.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointScrew</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="363"/>
      <source>Create Screw Joint</source>
      <translation>Crea vincolo di Vite</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="370"/>
      <source>Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.</source>
      <translation>Creare un vincolo di vite: collega una parte con un giunto di scorrimento con una parte con un giunto a rotazione.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="375"/>
      <source>Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione e di scorrimento. Il raggio di passo definisce il rapporto di movimento tra la vite rotante e la parte scorrevole.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="406"/>
      <location filename="../../../CommandCreateJoint.py" line="437"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Selezionare gli stessi sistemi di coordinate dei vincoli di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGears</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="394"/>
      <source>Create Gears Joint</source>
      <translation>Crea vincolo di ingranaggi</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="401"/>
      <source>Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.</source>
      <translation>Crea un vincolo di ingranaggi: collega due ingranaggi rotanti insieme. Avranno una direzione di rotazione inversa.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="425"/>
      <source>Create Belt Joint</source>
      <translation>Crea vincolo di cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="432"/>
      <source>Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.</source>
      <translation>Crea un vincolo di cinghia: collega due oggetti rotanti insieme. Avranno la stessa direzione di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointGearBelt</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="457"/>
      <source>Create Gear/Belt Joint</source>
      <translation>Crea vincolo Ingranaggio/Cinghia</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="463"/>
      <source>Create a Gears/Belt Joint: Links two rotating gears together.</source>
      <translation>Crea un vincolo di ingranaggi/cinghia: collega due ingranaggi rotanti insieme.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="468"/>
      <source>Select the same coordinate systems as the revolute joints.</source>
      <translation>Seleziona gli stessi sistemi di coordinate dei vincoli di rotazione.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateView</name>
    <message>
      <location filename="../../../CommandCreateView.py" line="54"/>
      <source>Create Exploded View</source>
      <translation>Crea Vista Esplosa</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateView.py" line="61"/>
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
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Create Parallel Joint</source>
      <translation>Crea Vincolo di Parallelismo</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.</source>
      <translation>Crea un Vincolo di Parallelismo: Rende paralleli gli assi Z dei sistemi di coordinate selezionati.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointPerpendicular</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="278"/>
      <source>Create Perpendicular Joint</source>
      <translation>Crea Vincolo di Perpendicolarità</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="285"/>
      <source>Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.</source>
      <translation>Crea un Vincolo Perpendicolare: Rende perpendicolari gli assi Z dei sistemi di coordinate selezionati.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointAngle</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="304"/>
      <source>Create Angle Joint</source>
      <translation>Crea Vincolo Angolare</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="311"/>
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
  <context>
    <name>Assembly::AssemblyLink</name>
    <message>
      <location filename="../../../App/AssemblyLink.cpp" line="492"/>
      <source>Joints</source>
      <translation>Giunti</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../ViewProviderAssemblyLink.cpp" line="139"/>
      <source>Toggle Rigid</source>
      <translation>Attiva/Disattiva Rigido</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertNewPart</name>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="54"/>
      <source>Insert New Part</source>
      <translation type="unfinished">Insert New Part</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertNewPart.py" line="61"/>
      <source>Insert a new part into the active assembly. The new part's origin can be positioned in the assembly.</source>
      <translation>Inserisci una nuova parte nell'assemblaggio attivo. L'origine della nuova parte può essere posizionata nell'assemblaggio.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateSimulation</name>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="67"/>
      <source>Create Simulation</source>
      <translation>Crea Simulazione</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateSimulation.py" line="74"/>
      <source>Create a simulation of the current assembly.</source>
      <translation>Crea una simulazione dell'insieme corrente.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_Insert</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="73"/>
      <source>Insert</source>
      <translation>Inserimento</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateSimulation</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="14"/>
      <source>Create Simulation</source>
      <translation>Crea Simulazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="20"/>
      <source>Motions</source>
      <translation>Movimenti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="50"/>
      <source>Add a prescribed motion</source>
      <translation>Aggiungi un movimento prescritto</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="70"/>
      <source>Delete selected motions</source>
      <translation>Elimina i movimenti selezionati</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="89"/>
      <source>Simulation settings</source>
      <translation>Impostazioni simulazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="95"/>
      <source>Start</source>
      <translation>Inizio</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="98"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="105"/>
      <source>Start time of the simulation</source>
      <translation>Tempo di avvio della simulazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="112"/>
      <source>End</source>
      <translation>Fine</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="115"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="122"/>
      <source>End time of the simulation</source>
      <translation>Tempo di fine della simulazione</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="129"/>
      <source>Step</source>
      <translation>Passo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="132"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="139"/>
      <source>Time Step</source>
      <translation>Passo temporale</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="146"/>
      <source>Tolerance</source>
      <translation>Tolleranza</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="149"/>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="156"/>
      <source>Global Error Tolerance</source>
      <translation>Tolleranza globale degli errori</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="166"/>
      <source>Generate</source>
      <translation>Genera</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="173"/>
      <source>Animation player</source>
      <translation>Riproduttore di animazioni</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="181"/>
      <source>Frame</source>
      <translation>Telaio</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="201"/>
      <source>0.00 s</source>
      <translation>0,00 s</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="212"/>
      <source>Frames Per Second</source>
      <translation>Fotogrammi al secondo</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="232"/>
      <source>Step backward</source>
      <translation>Passo indietro</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="252"/>
      <source>Play backward</source>
      <translation>Riproduzione all' incontrario</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="272"/>
      <source>Stop</source>
      <translation>Ferma</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="292"/>
      <source>Play forward</source>
      <translation>Riproduzione in avanti</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateSimulation.ui" line="312"/>
      <source>Step forward</source>
      <translation>Passo avanti</translation>
    </message>
  </context>
</TS>
