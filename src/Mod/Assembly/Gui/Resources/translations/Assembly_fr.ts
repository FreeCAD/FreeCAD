<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr" sourcelanguage="en">
  <context>
    <name>Assembly_CreateAssembly</name>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="48"/>
      <source>Create Assembly</source>
      <translation>Créer un assemblage</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateAssembly.py" line="53"/>
      <source>Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.</source>
      <translation>Créer un objet d'assemblage dans le document courant, ou dans l'assemblage actif courant (s'il y en a un). Limiter à un assemblage racine par fichier.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointFixed</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="68"/>
      <source>Create a Fixed Joint</source>
      <translation>Créer une liaison fixe</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="75"/>
      <source>1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.</source>
      <translation>1 - Si un assemblage est actif : créer une liaison bloquant définitivement deux pièces ensemble, empêchant tout mouvement ou rotation.</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="81"/>
      <source>2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.</source>
      <translation>2 - Si une pièce est active : positionner les sous-pièces en faisant correspondre les systèmes de coordonnées sélectionnés. La deuxième pièce sélectionnée se déplacera.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointRevolute</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="103"/>
      <source>Create Revolute Joint</source>
      <translation>Créer une liaison pivot</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="110"/>
      <source>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</source>
      <translation>Créer une liaison pivot : permettre la rotation autour d'un seul axe entre les pièces sélectionnées.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointCylindrical</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="131"/>
      <source>Create Cylindrical Joint</source>
      <translation>Créer une liaison pivot glissant</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="138"/>
      <source>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</source>
      <translation>Créer une liaison pivot glissant : permettre la rotation sur un axe tout en permettant le mouvement le long du même axe entre les parties assemblées.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointSlider</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="157"/>
      <source>Create Slider Joint</source>
      <translation>Créer une liaison glissière</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="164"/>
      <source>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</source>
      <translation>Créer une liaison glissière : permettre un mouvement linéaire le long d'un seul axe, mais limite la rotation entre les pièces sélectionnées.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointBall</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="183"/>
      <source>Create Ball Joint</source>
      <translation>Créer une liaison bille</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="190"/>
      <source>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</source>
      <translation>Créer une liaison bille : relier des pièces en un point, ce qui permet un mouvement illimité tant que les points de connexion restent en contact.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_CreateJointDistance</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="209"/>
      <source>Create Distance Joint</source>
      <translation>Créer une liaison distance</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="216"/>
      <source>Create a Distance Joint: Fix the distance between the selected objects.</source>
      <translation>Créer une liaison distance : fixer la distance entre les objets sélectionnés.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ToggleGrounded</name>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="250"/>
      <source>Toggle grounded</source>
      <translation>Activer/désactiver le blocage</translation>
    </message>
    <message>
      <location filename="../../../CommandCreateJoint.py" line="257"/>
      <source>Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.</source>
      <translation>Le blocage d'une pièce bloque sa position dans l'assemblage de façon permanente, empêchant tout mouvement ou rotation. Vous devez avoir au moins une pièce bloquée avant de commencer à assembler.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_ExportASMT</name>
    <message>
      <location filename="../../../CommandExportASMT.py" line="47"/>
      <source>Export ASMT File</source>
      <translation>Exporter un fichier ASMT</translation>
    </message>
    <message>
      <location filename="../../../CommandExportASMT.py" line="52"/>
      <source>Export currently active assembly as a ASMT file.</source>
      <translation>Exporter l'assemblage en cours dans un fichier ASMT.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_InsertLink</name>
    <message>
      <location filename="../../../CommandInsertLink.py" line="52"/>
      <source>Insert Link</source>
      <translation>Insérer un lien</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="59"/>
      <source>Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is &lt;b&gt;open in the current session&lt;/b&gt;</source>
      <translation>Insérer un lien dans l'assemblage actif. Cela créera des liens dynamiques vers les pièces/corps/primitives/assemblages. Pour insérer des objets externes, assurez-vous que le fichier est &lt;b&gt;ouvert dans la session en cours&lt;/b&gt;.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="61"/>
      <source>Insert by left clicking items in the list.</source>
      <translation>Insérer par un clic gauche des éléments de la liste.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="65"/>
      <source>Remove by right clicking items in the list.</source>
      <translation>Supprimer par un clic droit des éléments de la liste.</translation>
    </message>
    <message>
      <location filename="../../../CommandInsertLink.py" line="70"/>
      <source>Press shift to add several links while clicking on the view.</source>
      <translation>Appuyer sur Maj pour ajouter plusieurs liens en cliquant sur la vue.</translation>
    </message>
  </context>
  <context>
    <name>Assembly_SolveAssembly</name>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="51"/>
      <source>Solve Assembly</source>
      <translation>Résoudre l'assemblage</translation>
    </message>
    <message>
      <location filename="../../../CommandSolveAssembly.py" line="58"/>
      <source>Solve the currently active assembly.</source>
      <translation>Résoudre l'assemblage actif.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../../InitGui.py" line="74"/>
      <source>Assembly</source>
      <translation>Assemblage</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../../InitGui.py" line="98"/>
      <source>Assembly</source>
      <translation>Assemblage</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="99"/>
      <source>Assembly Joints</source>
      <translation>Liaisons d'assemblage</translation>
    </message>
    <message>
      <location filename="../../../InitGui.py" line="102"/>
      <source>&amp;Assembly</source>
      <translation>&amp;Assemblage</translation>
    </message>
  </context>
  <context>
    <name>Assembly</name>
    <message>
      <location filename="../../../JointObject.py" line="48"/>
      <source>Fixed</source>
      <translation>Fixé</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="49"/>
      <source>Revolute</source>
      <translation>Pivot</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="50"/>
      <source>Cylindrical</source>
      <translation>Cylindrique</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="51"/>
      <source>Slider</source>
      <translation>Glissière</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="52"/>
      <source>Ball</source>
      <translation>Bille</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="53"/>
      <source>Distance</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="47"/>
      <source>Ask</source>
      <translation>Demander</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="48"/>
      <source>Always</source>
      <translation>Toujours</translation>
    </message>
    <message>
      <location filename="../../../Preferences.py" line="49"/>
      <source>Never</source>
      <translation>Jamais</translation>
    </message>
  </context>
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../JointObject.py" line="116"/>
      <source>The type of the joint</source>
      <translation>Le type de liaison</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="126"/>
      <source>The first object of the joint</source>
      <translation>Le premier objet de la liaison</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="133"/>
      <source>The first part of the joint</source>
      <translation>La première pièce de la liaison</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="140"/>
      <source>The selected element of the first object</source>
      <translation>L'élément sélectionné du premier objet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="147"/>
      <source>The selected vertex of the first object</source>
      <translation>Le sommet sélectionné du premier objet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="157"/>
      <source>This is the local coordinate system within object1 that will be used for the joint.</source>
      <translation>Il s'agit du système de coordonnées local de l'objet 1 qui sera utilisé pour la liaison.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="167"/>
      <source>This prevents Placement1 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Cela empêche Placement1 de recalculer, ce qui permet un positionnement personnalisé du placement.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="175"/>
      <source>The second object of the joint</source>
      <translation>Le deuxième objet de la liaison</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="182"/>
      <source>The second part of the joint</source>
      <translation>La seconde pièce de la liaison</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="189"/>
      <source>The selected element of the second object</source>
      <translation>L'élément sélectionné du second objet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="196"/>
      <source>The selected vertex of the second object</source>
      <translation>Le sommet sélectionné du second objet</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="206"/>
      <source>This is the local coordinate system within object2 that will be used for the joint.</source>
      <translation>Il s'agit du système de coordonnées local de l'objet 2 qui sera utilisé pour la liaison.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="216"/>
      <source>This prevents Placement2 from recomputing, enabling custom positioning of the placement.</source>
      <translation>Cela empêche Placement2 de recalculer, ce qui permet un positionnement personnalisé du placement.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="226"/>
      <source>This is the distance of the joint. It is used only by the distance joint.</source>
      <translation>Il s'agit de la distance de la liaison. Elle n'est utilisée que par la liaison de distance.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="236"/>
      <source>This is the rotation of the joint.</source>
      <translation>Il s'agit de la rotation de la liaison.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="246"/>
      <source>This is the offset vector of the joint.</source>
      <translation>Il s'agit du vecteur de décalage de la liaison.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="256"/>
      <source>This indicates if the joint is active.</source>
      <translation>Ceci indique si la liaison est active.</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="889"/>
      <source>The object to ground</source>
      <translation>L'objet à bloquer</translation>
    </message>
    <message>
      <location filename="../../../JointObject.py" line="901"/>
      <source>This is where the part is grounded.</source>
      <translation>C'est là que la pièce est bloquée.</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyCreateJoint</name>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="14"/>
      <source>Create Joint</source>
      <translation>Créer une liaison</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="28"/>
      <source>Distance</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="52"/>
      <source>Offset</source>
      <translation>Décaler</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="76"/>
      <source>Rotation</source>
      <translation>Rotation</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="104"/>
      <source>Reverse the direction of the joint.</source>
      <translation>Inverser la direction de la liaison</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyCreateJoint.ui" line="107"/>
      <source>Reverse</source>
      <translation>Inverser</translation>
    </message>
  </context>
  <context>
    <name>TaskAssemblyInsertLink</name>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="14"/>
      <source>Insert Link</source>
      <translation>Insérer un lien</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="20"/>
      <source>Search parts...</source>
      <translation>Rechercher des pièces...</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="32"/>
      <source>Don't find your part? </source>
      <translation>Vous ne trouvez pas votre pièce ?</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="39"/>
      <source>Open file</source>
      <translation>Ouvrir un fichier</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="48"/>
      <source>If checked, the selected object will be inserted inside a Part container, unless it is already a Part.</source>
      <translation>Si cette case est cochée, l'objet sélectionné sera inséré dans un conteneur de Part, sauf s'il s'agit déjà d'un objet de type Part.</translation>
    </message>
    <message>
      <location filename="../panels/TaskAssemblyInsertLink.ui" line="51"/>
      <source>Insert as part</source>
      <translation>Insérer en tant qu'objet de type Part</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::DlgSettingsAssembly</name>
    <message>
      <location filename="../preferences/Assembly.ui" line="14"/>
      <source>General</source>
      <translation>Général</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="20"/>
      <source>Allow to leave edit mode when pressing Esc button</source>
      <translation>Permettre de quitter le mode édition en appuyant sur la touche Échap</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="23"/>
      <source>Esc leave edit mode</source>
      <translation>La touche Échap permet de quitter le mode édition.</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="39"/>
      <source>Ground first part:</source>
      <translation>Bloquer d'abord une pièce :</translation>
    </message>
    <message>
      <location filename="../preferences/Assembly.ui" line="46"/>
      <source>When you insert the first part in the assembly, you can choose to ground the part automatically.</source>
      <translation>Lorsque vous insérez la première pièce de l'assemblage, vous pouvez choisir de la bloquer automatiquement.</translation>
    </message>
  </context>
  <context>
    <name>AssemblyGui::ViewProviderAssembly</name>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="136"/>
      <source>Delete associated joints</source>
      <translation>Supprimer les liaisons associées</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="148"/>
      <source>The object is associated to one or more joints.</source>
      <translation>L'objet est associé à une ou plusieurs liaisons.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="150"/>
      <source>Do you want to move the object and delete associated joints?</source>
      <translation>Voulez-vous déplacer l'objet et supprimer les liaisons associées ?</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAssembly.cpp" line="651"/>
      <source>Move part</source>
      <translation>Déplacer une pièce</translation>
    </message>
  </context>
</TS>
