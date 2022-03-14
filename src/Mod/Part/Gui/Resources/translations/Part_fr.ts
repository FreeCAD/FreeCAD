<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr" sourcelanguage="en">
  <context>
    <name>AttachmentEditor</name>
    <message>
      <location filename="Commands.py" line="79"/>
      <source>Attachment...</source>
      <translation>Accrochage...</translation>
    </message>
    <message>
      <location filename="Commands.py" line="83"/>
      <source>Edit attachment of selected object.</source>
      <translation>Modifier l'accrochage de l’objet sélectionné.</translation>
    </message>
  </context>
  <context>
    <name>Part_CompoundFilter</name>
    <message>
      <location filename="_CommandCompoundFilter.py" line="57"/>
      <source>Compound Filter</source>
      <translation>Filtre composé</translation>
    </message>
    <message>
      <location filename="_CommandCompoundFilter.py" line="67"/>
      <source>Filter out objects from a selected compound by characteristics like volume,
area, or length, or by choosing specific items.
If a second object is selected, it will be used as reference, for example,
for collision or distance filtering.</source>
      <translation>Filtrer les objets d'un composé sélectionné par des caractéristiques telles que le volume, la surface, la longueur ou en choisissant des éléments spécifiques.
Si un second objet est sélectionné, il sera utilisé comme référence, par exemple
pour le filtrage de collision ou de distance.</translation>
    </message>
  </context>
  <context>
    <name>Part_ExplodeCompound</name>
    <message>
      <location filename="_CommandExplodeCompound.py" line="56"/>
      <source>Explode compound</source>
      <translation>Éclater le composé</translation>
    </message>
    <message>
      <location filename="_CommandExplodeCompound.py" line="62"/>
      <source>Split up a compound of shapes into separate objects.
It will create a 'Compound Filter' for each shape.</source>
      <translation>Diviser un composé de formes en objets séparés.
Cela créera un 'filtre composé' pour chaque forme.</translation>
    </message>
  </context>
  <context>
    <name>Part_JoinConnect</name>
    <message>
      <location filename="JoinFeatures.py" line="197"/>
      <source>Connect objects</source>
      <translation>Connecter des objets</translation>
    </message>
    <message>
      <location filename="JoinFeatures.py" line="202"/>
      <source>Fuses objects, taking care to preserve voids.</source>
      <translation>Fusionne les objets, en prenant soin de préserver les cavités.</translation>
    </message>
  </context>
  <context>
    <name>Part_JoinCutout</name>
    <message>
      <location filename="JoinFeatures.py" line="388"/>
      <source>Cutout for object</source>
      <translation>Découpe de l'objet</translation>
    </message>
    <message>
      <location filename="JoinFeatures.py" line="393"/>
      <source>Makes a cutout in one object to fit another object.</source>
      <translation>Créer une découpe dans un objet pour l'adapter à un autre objet.</translation>
    </message>
  </context>
  <context>
    <name>Part_JoinEmbed</name>
    <message>
      <location filename="JoinFeatures.py" line="293"/>
      <source>Embed object</source>
      <translation>Intégrer l'objet</translation>
    </message>
    <message>
      <location filename="JoinFeatures.py" line="298"/>
      <source>Fuses one object into another, taking care to preserve voids.</source>
      <translation>Fusionne un objet dans un autre en prenant soin de préserver les cavités.</translation>
    </message>
  </context>
  <context>
    <name>Part_SplitFeatures</name>
    <message>
      <location filename="SplitFeatures.py" line="188"/>
      <source>Boolean fragments</source>
      <translation>Fragments booléens</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="197"/>
      <source>Create a 'Boolean Fragments' object from two or more selected objects,
or from the shapes inside a compound.
This is a boolean union which is then sliced at the intersections
of the original shapes.
A 'Compound Filter' can be used to extract the individual slices.</source>
      <translation>Crée un objet 'Fragment booléen' à partir de deux ou plusieurs objets sélectionnés,
ou à partir des formes à l'intérieur d'un composé.
Ceci est une union booléenne qui est ensuite coupée aux intersections
des formes originales.
Un 'Filtre composé' peut être utilisé pour extraire les tranches individuelles.</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="347"/>
      <source>Slice to compound</source>
      <translation>Trancher vers composé</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="354"/>
      <source>Slice a selected object by using other objects as cutting tools.
The resulting pieces will be stored in a compound.
A 'Compound Filter' can be used to extract the individual slices.</source>
      <translation>Couper un objet sélectionné en utilisant d'autres objets comme outils de coupe.
Les pièces résultantes seront stockées dans un composé.
Un 'Filtre composé' peut être utilisé pour extraire les tranches individuelles.</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="379"/>
      <source>Slice apart</source>
      <translation>Séparer et exploser</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="385"/>
      <source>Slice a selected object by other objects, and split it apart.
It will create a 'Compound Filter' for each slice.</source>
      <translation>Coupe un objet sélectionné par d'autres objets et le sépare.
Cela va créer un 'filtre composé' pour chaque morceau.</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="529"/>
      <source>Boolean XOR</source>
      <translation>Booléen XOR</translation>
    </message>
    <message>
      <location filename="SplitFeatures.py" line="537"/>
      <source>Perform an 'exclusive OR' boolean operation with two or more selected objects,
or with the shapes inside a compound.
This means the overlapping volumes of the shapes will be removed.
A 'Compound Filter' can be used to extract the remaining pieces.</source>
      <translation>Effectuez une opération booléenne 'OU exclusif' avec au moins deux objets sélectionnés,
ou avec les formes à l'intérieur d'un composé.
Cela signifie que les volumes de chevauchement des formes seront supprimés.
Un 'Filtre composé' peut être utilisé pour extraire les parties restantes.</translation>
    </message>
  </context>
  <context>
    <name>Part_Tube</name>
    <message>
      <location filename="CommandShapes.py" line="44"/>
      <source>Create tube</source>
      <translation>Créer un tube</translation>
    </message>
    <message>
      <location filename="CommandShapes.py" line="50"/>
      <source>Creates a tube</source>
      <translation>Crée un tube</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="CommandShapes.py" line="52"/>
      <source>Create tube</source>
      <translation>Créer un tube</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="57"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="66"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="75"/>
      <source>Edit %1</source>
      <translation>Modifier %1</translation>
    </message>
    <message>
      <location filename="../../AppPartGui.cpp" line="221"/>
      <location filename="../../AppPartGui.cpp" line="222"/>
      <location filename="../../AppPartGui.cpp" line="223"/>
      <source>Part design</source>
      <translation>Conception de pièces</translation>
    </message>
    <message>
      <location filename="../../AppPartGui.cpp" line="224"/>
      <location filename="../../AppPartGui.cpp" line="225"/>
      <source>Import-Export</source>
      <translation>Importer-Exporter</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="286"/>
      <location filename="../../Command.cpp" line="381"/>
      <location filename="../../Command.cpp" line="481"/>
      <location filename="../../Command.cpp" line="900"/>
      <location filename="../../Command.cpp" line="957"/>
      <location filename="../../Command.cpp" line="2097"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="287"/>
      <location filename="../../Command.cpp" line="958"/>
      <source>Select two shapes please.</source>
      <translation>Veuillez sélectionner deux formes.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="296"/>
      <location filename="../../Command.cpp" line="396"/>
      <location filename="../../Command.cpp" line="496"/>
      <source>Non-solids selected</source>
      <translation>Formes non-pleines sélectionnées</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="297"/>
      <location filename="../../Command.cpp" line="397"/>
      <location filename="../../Command.cpp" line="497"/>
      <source>The use of non-solids for boolean operations may lead to unexpected results.
Do you want to continue?</source>
      <translation>L'utilisation de formes non-pleines pour les opérations booléennes peut entraîner des résultats indésirables.
Voulez-vous continuer ?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="382"/>
      <source>Select two shapes or more, please. Or, select one compound containing two or more shapes to compute common between.</source>
      <translation>Sélectionnez deux formes ou plus. Ou sélectionnez un composé contenant plusieurs formes à partir desquelles calculer l'intersection.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="482"/>
      <source>Select two shapes or more, please. Or, select one compound containing two or more shapes to be fused.</source>
      <translation>Sélectionnez deux formes ou plus. Ou sélectionnez un composé contenant plusieurs formes à partir desquelles calculer la fusion.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="901"/>
      <source>Select one shape or more, please.</source>
      <translation>Veuillez sélectionner au moins une forme.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1113"/>
      <source>All CAD Files</source>
      <translation>Tous les fichiers CAO</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1118"/>
      <source>All Files</source>
      <translation>Tous les fichiers</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2098"/>
      <source>You have to select either two edges or two wires.</source>
      <translation>Vous devez sélectionner deux arêtes ou deux fils.</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="128"/>
      <source>Sewing Tolerance</source>
      <translation>Tolérance de couture</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="129"/>
      <source>Enter tolerance for sewing shape:</source>
      <translation>Saisir la tolérance pour coudre la forme :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="73"/>
      <location filename="../../TaskAttacher.cpp" line="109"/>
      <source>No reference selected</source>
      <translation>Aucune référence sélectionnée</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="82"/>
      <location filename="../../TaskCheckGeometry.cpp" line="86"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="85"/>
      <location filename="../../TaskCheckGeometry.cpp" line="88"/>
      <source>Edge</source>
      <translation>Arête</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="88"/>
      <location filename="../../TaskCheckGeometry.cpp" line="89"/>
      <source>Vertex</source>
      <translation>Sommet</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="82"/>
      <source>Compound</source>
      <translation>Composé</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="83"/>
      <source>Compound Solid</source>
      <translation>Composé solide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="84"/>
      <source>Solid</source>
      <translation>Solide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="85"/>
      <source>Shell</source>
      <translation>Coque</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="87"/>
      <source>Wire</source>
      <translation>Fil</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="90"/>
      <source>Shape</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="105"/>
      <source>No Error</source>
      <translation>Aucune erreur</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="106"/>
      <source>Invalid Point On Curve</source>
      <translation>Point sur courbe invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="107"/>
      <source>Invalid Point On Curve On Surface</source>
      <translation>Point sur courbe sur surface invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="108"/>
      <source>Invalid Point On Surface</source>
      <translation>Point sur surface invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="109"/>
      <source>No 3D Curve</source>
      <translation>Aucune courbe 3D</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="110"/>
      <source>Multiple 3D Curve</source>
      <translation>Courbe 3D multiple</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="111"/>
      <source>Invalid 3D Curve</source>
      <translation>Courbe 3D invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="112"/>
      <source>No Curve On Surface</source>
      <translation>Aucune courbe sur surface</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="113"/>
      <source>Invalid Curve On Surface</source>
      <translation>Courbe sur surface invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="114"/>
      <source>Invalid Curve On Closed Surface</source>
      <translation>Courbe sur surface fermée invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="115"/>
      <source>Invalid Same Range Flag</source>
      <translation>L'indicateur de plage invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="116"/>
      <source>Invalid Same Parameter Flag</source>
      <translation>Indicateur de paramètre invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="117"/>
      <source>Invalid Degenerated Flag</source>
      <translation>Indicateur dégénéré invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="118"/>
      <source>Free Edge</source>
      <translation>Arête libre</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="119"/>
      <source>Invalid MultiConnexity</source>
      <translation>Connectivité multiple invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="120"/>
      <source>Invalid Range</source>
      <translation>Plage non valide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="121"/>
      <source>Empty Wire</source>
      <translation>Fil vide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="122"/>
      <source>Redundant Edge</source>
      <translation>Arête redondante</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="123"/>
      <source>Self Intersecting Wire</source>
      <translation>Fil entre-croisé</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="124"/>
      <source>No Surface</source>
      <translation>Aucune surface</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="125"/>
      <source>Invalid Wire</source>
      <translation>Fil invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="126"/>
      <source>Redundant Wire</source>
      <translation>Fil redondant</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="127"/>
      <source>Intersecting Wires</source>
      <translation>Fils croisés</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="128"/>
      <source>Invalid Imbrication Of Wires</source>
      <translation>Imbrication de fils invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="129"/>
      <source>Empty Shell</source>
      <translation>Coque vide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="130"/>
      <source>Redundant Face</source>
      <translation>Surface redondante</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="131"/>
      <source>Unorientable Shape</source>
      <translation>La forme n'est pas orientable</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="132"/>
      <source>Not Closed</source>
      <translation>Non fermé</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="133"/>
      <source>Not Connected</source>
      <translation>Non connecté</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="134"/>
      <source>Sub Shape Not In Shape</source>
      <translation>La sous-forme n'est pas dans une forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="135"/>
      <source>Bad Orientation</source>
      <translation>Mauvaise orientation</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="136"/>
      <source>Bad Orientation Of Sub Shape</source>
      <translation>Mauvaise orientation de la sous-forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="137"/>
      <source>Invalid Tolerance Value</source>
      <translation>Valeur de tolérance invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="138"/>
      <source>Check Failed</source>
      <translation>La vérification a échoué</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="148"/>
      <source>No Result</source>
      <translation>Aucun résultat</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="152"/>
      <source>Out Of Enum Range:</source>
      <translation>Hors de la plage Enum :</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="164"/>
      <source>BOPAlgo CheckUnknown</source>
      <translation>BOPAlgo Vérifier les inconnues</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="165"/>
      <source>BOPAlgo BadType</source>
      <translation>BOPAlgo MauvaisType</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="166"/>
      <source>BOPAlgo SelfIntersect</source>
      <translation>BOPAlgo auto intersection</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="167"/>
      <source>BOPAlgo TooSmallEdge</source>
      <translation>BOPAlgo Arête trop courte</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="168"/>
      <source>BOPAlgo NonRecoverableFace</source>
      <translation>BOPAlgo Face non récupérable</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="169"/>
      <source>BOPAlgo IncompatibilityOfVertex</source>
      <translation>BOPAlgo incompatibilité de sommet</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="170"/>
      <source>BOPAlgo IncompatibilityOfEdge</source>
      <translation>BOPAlgo Incompatibilité d'arête</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="171"/>
      <source>BOPAlgo IncompatibilityOfFace</source>
      <translation>BOPAlgo Incompatibilité de face</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="172"/>
      <source>BOPAlgo OperationAborted</source>
      <translation>BOPAlgo Opération avortée</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="173"/>
      <source>BOPAlgo GeomAbs_C0</source>
      <translation>BOPAlgo GeomAbs_C0</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="174"/>
      <source>BOPAlgo_InvalidCurveOnSurface</source>
      <translation>BOPAlgo_Courbe invalide sur surface</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="175"/>
      <source>BOPAlgo NotValid</source>
      <translation>BOPAlgo Non valide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="464"/>
      <location filename="../../TaskCheckGeometry.cpp" line="758"/>
      <source>Invalid</source>
      <translation>Invalide</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="631"/>
      <location filename="../../TaskDimension.cpp" line="1769"/>
      <source>Selections</source>
      <translation>Sélections</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="638"/>
      <location filename="../../TaskDimension.cpp" line="1776"/>
      <source>Control</source>
      <translation>Contrôle</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="1481"/>
      <source>Reset selection</source>
      <translation>Réinitialiser la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="1485"/>
      <source>Toggle direct dimensions</source>
      <translation>Basculer aux cotes directes</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="1490"/>
      <source>Toggle orthogonal dimensions</source>
      <translation>Basculer aux cotes orthogonales</translation>
    </message>
    <message>
      <location filename="../../TaskDimension.cpp" line="1495"/>
      <source>Clear all dimensions</source>
      <translation>Effacer toutes les cotes</translation>
    </message>
    <message>
      <location filename="../../ViewProviderExt.cpp" line="977"/>
      <source>Set colors...</source>
      <translation>Définir les couleurs...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="79"/>
      <source>Edit mirror plane</source>
      <translation>Éditer le plan du miroir</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="279"/>
      <source>Edit fillet edges</source>
      <translation>Modifier les congés d'arête</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="383"/>
      <source>Edit chamfer edges</source>
      <translation>Modifier les arêtes du chanfrein</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="528"/>
      <source>Edit offset</source>
      <translation>Modifier le décalage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="614"/>
      <source>Edit thickness</source>
      <translation>Modifier l'évidement</translation>
    </message>
    <message>
      <location filename="../../ViewProviderSpline.cpp" line="87"/>
      <location filename="../../ViewProviderSpline.cpp" line="339"/>
      <source>Show control points</source>
      <translation>Afficher les points de contrôle</translation>
    </message>
    <message>
      <location filename="../../ViewProviderAttachExtension.cpp" line="116"/>
      <source>Attachment editor</source>
      <translation>Éditeur de pièces jointes</translation>
    </message>
  </context>
  <context>
    <name>Attacher</name>
    <message>
      <location filename="../../AttacherTexts.cpp" line="311"/>
      <source>Any</source>
      <comment>Attacher reference type</comment>
      <translation>Quelconque</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="312"/>
      <source>Vertex</source>
      <comment>Attacher reference type</comment>
      <translation>Sommet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="313"/>
      <source>Edge</source>
      <comment>Attacher reference type</comment>
      <translation>Arête</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="314"/>
      <source>Face</source>
      <comment>Attacher reference type</comment>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="316"/>
      <source>Line</source>
      <comment>Attacher reference type</comment>
      <translation>Ligne</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="317"/>
      <source>Curve</source>
      <comment>Attacher reference type</comment>
      <translation>Courbe</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="318"/>
      <source>Circle</source>
      <comment>Attacher reference type</comment>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="319"/>
      <source>Conic</source>
      <comment>Attacher reference type</comment>
      <translation>Conique</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="320"/>
      <source>Ellipse</source>
      <comment>Attacher reference type</comment>
      <translation>Ellipse</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="321"/>
      <source>Parabola</source>
      <comment>Attacher reference type</comment>
      <translation>Parabole</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="322"/>
      <source>Hyperbola</source>
      <comment>Attacher reference type</comment>
      <translation>Hyperbole</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="324"/>
      <source>Plane</source>
      <comment>Attacher reference type</comment>
      <translation>Plan</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="325"/>
      <source>Sphere</source>
      <comment>Attacher reference type</comment>
      <translation>Sphère</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="326"/>
      <source>Revolve</source>
      <comment>Attacher reference type</comment>
      <translation>Révolution</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="327"/>
      <source>Cylinder</source>
      <comment>Attacher reference type</comment>
      <translation>Cylindre</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="328"/>
      <source>Torus</source>
      <comment>Attacher reference type</comment>
      <translation>Tore</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="329"/>
      <source>Cone</source>
      <comment>Attacher reference type</comment>
      <translation>Cône</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="331"/>
      <source>Object</source>
      <comment>Attacher reference type</comment>
      <translation>Objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="332"/>
      <source>Solid</source>
      <comment>Attacher reference type</comment>
      <translation>Solide</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="333"/>
      <source>Wire</source>
      <comment>Attacher reference type</comment>
      <translation>Fil</translation>
    </message>
  </context>
  <context>
    <name>Attacher0D</name>
    <message>
      <location filename="../../AttacherTexts.cpp" line="268"/>
      <source>Deactivated</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Désactivé</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="269"/>
      <source>Attachment is disabled. Point can be moved by editing Placement property.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>L'accrochage est désactivé. Le point peut être déplacé en modifiant la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="271"/>
      <source>Object's origin</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Origine de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="272"/>
      <source>Point is put at object's Placement.Position. Works on objects with placements, and ellipse/parabola/hyperbola edges.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Le point est placé au Placement/Position de l'objet. Cela fonctionne pour les objets avec des positionnements et pour les arêtes des ellipses/paraboles/hyperboles.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="274"/>
      <source>Focus1</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Focus1</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="275"/>
      <source>Focus of ellipse, parabola, hyperbola.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Focus de l'ellipse, la parabole, l'hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="277"/>
      <source>Focus2</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Focus2</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="278"/>
      <source>Second focus of ellipse and hyperbola.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Le second foyer de l'ellipse et de l'hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="280"/>
      <source>On edge</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Sur l'arête</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="281"/>
      <source>Point is put on edge, MapPathParameter controls where. Additionally, vertex can be linked in for making a projection.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Le point est placé sur l’arête, la position exacte est contrôlée par MapPathParameter. De plus, un sommet peut être lié pour permettre une projection.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="283"/>
      <source>Center of curvature</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Centre de courbure</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="284"/>
      <source>Center of osculating circle of an edge. Optional vertex link defines where.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Centre du cercle osculateur d'une arête. Un lien facultatif vers un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="286"/>
      <source>Center of mass</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Centre de masse</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="287"/>
      <source>Center of mass of all references (equal densities are assumed).</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Centre de masse de toutes les références (on suppose des densités égales).</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="289"/>
      <source>Intersection</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="290"/>
      <source>Not implemented</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Non implémenté</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="292"/>
      <source>Vertex</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Sommet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="293"/>
      <source>Put Datum point coincident with another vertex.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Faire coïncider le point de référence avec un autre sommet.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="295"/>
      <source>Proximity point 1</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Point de proximité 1</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="296"/>
      <source>Point on first reference that is closest to second reference.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Le point sur la première référence qui est le plus proche de la seconde référence.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="298"/>
      <source>Proximity point 2</source>
      <comment>AttachmentPoint mode caption</comment>
      <translation>Point de proximité 2</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="299"/>
      <source>Point on second reference that is closest to first reference.</source>
      <comment>AttachmentPoint mode tooltip</comment>
      <translation>Le point sur la seconde référence qui est le plus proche de la première référence.</translation>
    </message>
  </context>
  <context>
    <name>Attacher1D</name>
    <message>
      <location filename="../../AttacherTexts.cpp" line="199"/>
      <source>Deactivated</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Désactivé</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="200"/>
      <source>Attachment is disabled. Line can be moved by editing Placement property.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>L'accrochage est désactivé. La ligne peut être déplacée en modifiant la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="202"/>
      <source>Object's X</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>X de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="203"/>
      <location filename="../../AttacherTexts.cpp" line="209"/>
      <source>Line is aligned along local X axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>La ligne est alignée avec l'axe X local de l'objet. Cela fonctionne avec les objets avec des positions et les arêtes des ellipses/paraboles/hyperboles.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="205"/>
      <source>Object's Y</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Y de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="206"/>
      <source>Line is aligned along local Y axis of object. Works on objects with placements, and ellipse/parabola/hyperbola edges.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>La ligne est alignée avec l'axe Y local de l'objet. Cela fonctionne avec les objets avec des positions et les arêtes des ellipses/paraboles/hyperboles.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="208"/>
      <source>Object's Z</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Z de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="211"/>
      <source>Axis of curvature</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Axe de courbure</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="212"/>
      <source>Line that is an axis of osculating circle of curved edge. Optional vertex defines where.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne qui est un axe de cercle osculateur d'une arête incurvée. Un sommet facultatif définit la position.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="214"/>
      <source>Directrix1</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Directrice 1</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="215"/>
      <source>Directrix line for ellipse, parabola, hyperbola.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne directrice pour ellipse, parabole, hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="217"/>
      <source>Directrix2</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Directrice 2</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="218"/>
      <source>Second directrix line for ellipse and hyperbola.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Deuxième ligne directrice pour ellipse et hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="220"/>
      <source>Asymptote1</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Asymptote1</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="221"/>
      <source>Asymptote of a hyperbola.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Asymptote d’une hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="223"/>
      <source>Asymptote2</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Asymptote2</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="224"/>
      <source>Second asymptote of hyperbola.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Deuxième asymptote de l’hyperbole.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="226"/>
      <source>Tangent</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Tangent</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="227"/>
      <source>Line tangent to an edge. Optional vertex link defines where.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne tangente à une arête. Le lien optionnel vers un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="229"/>
      <source>Normal to edge</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Normal à l'arête</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="230"/>
      <source>Align to N vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Aligner au vecteur N du système de coordonnées Frenet-Serret d'une arête courbe. Le lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="232"/>
      <source>Binormal</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Binormal</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="233"/>
      <source>Align to B vector of Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Aligner au vecteur B du système de coordonnées Frenet-Serret d'une arête courbe. Le lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="235"/>
      <source>Tangent to surface (U)</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Tangent à la surface (U)</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="236"/>
      <location filename="../../AttacherTexts.cpp" line="239"/>
      <source>Tangent to surface, along U parameter. Vertex link defines where.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Tangent à la surface, le long du paramètre U. Le lien vers le sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="238"/>
      <source>Tangent to surface (V)</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Tangent à la surface (V)</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="241"/>
      <source>Through two points</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Par deux points</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="242"/>
      <source>Line that passes through two vertices.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne qui passe par deux sommets.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="244"/>
      <source>Intersection</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="245"/>
      <source>Not implemented.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Non implémenté.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="247"/>
      <source>Proximity line</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Ligne de proximité</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="248"/>
      <source>Line that spans the shortest distance between shapes.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne de la distance la plus courte entre des formes.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="250"/>
      <source>1st principal axis</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>1er axe principal</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="251"/>
      <source>Line follows first principal axis of inertia.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>La ligne suit le premier axe principal d’inertie.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="253"/>
      <source>2nd principal axis</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>2ème axe principal</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="254"/>
      <source>Line follows second principal axis of inertia.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>La ligne suit le second axe principal d’inertie.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="256"/>
      <source>3rd principal axis</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>3e axe principal</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="257"/>
      <source>Line follows third principal axis of inertia.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>La ligne suit le troisième axe principal d’inertie.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="259"/>
      <source>Normal to surface</source>
      <comment>AttachmentLine mode caption</comment>
      <translation>Normal à la surface</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="260"/>
      <source>Line perpendicular to surface at point set by vertex.</source>
      <comment>AttachmentLine mode tooltip</comment>
      <translation>Ligne perpendiculaire à la surface au point défini par un vertex.</translation>
    </message>
  </context>
  <context>
    <name>Attacher2D</name>
    <message>
      <location filename="../../AttacherTexts.cpp" line="124"/>
      <source>Deactivated</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Désactivé</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="125"/>
      <source>Attachment is disabled. Object can be moved by editing Placement property.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>L'accrochage est désactivé. L'objet peut être déplacé en modifiant la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="127"/>
      <source>Translate origin</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Translation de l'origine</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="128"/>
      <source>Origin is aligned to match Vertex. Orientation is controlled by Placement property.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>L'origine est alignée sur un sommet. L’orientation est contrôlée par la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="130"/>
      <source>Object's XY</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>XY de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="131"/>
      <source>Plane is aligned to XY local plane of linked object.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est aligné sur le plan local XY de l’objet lié.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="133"/>
      <source>Object's XZ</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>XZ de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="134"/>
      <source>Plane is aligned to XZ local plane of linked object.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est aligné sur le plan local XZ de l’objet lié.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="136"/>
      <source>Object's YZ</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>YZ de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="137"/>
      <source>Plane is aligned to YZ local plane of linked object.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est aligné sur le plan local YZ de l’objet lié.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="139"/>
      <source>Plane face</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Face du plan</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="140"/>
      <source>Plane is aligned to coincide planar face.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est aligné pour coïncider avec la face plane.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="142"/>
      <source>Tangent to surface</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Tangent à la surface</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="143"/>
      <source>Plane is made tangent to surface at vertex.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est fait tangent à la surface au niveau du sommet.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="145"/>
      <source>Normal to edge</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Normal à l'arête</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="146"/>
      <source>Plane is made tangent to edge. Optional vertex link defines where.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est fait tangent à une arête. Le lien optionnel vers un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="148"/>
      <source>Frenet NB</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Plan normal de Frenet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="149"/>
      <location filename="../../AttacherTexts.cpp" line="152"/>
      <location filename="../../AttacherTexts.cpp" line="155"/>
      <source>Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Aligner au repère de Frenet-Serret de l'arête courbe. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="151"/>
      <source>Frenet TN</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Plan osculateur de Frenet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="154"/>
      <source>Frenet TB</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Plan rectifiant de Frenet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="157"/>
      <source>Concentric</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Concentrique</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="158"/>
      <source>Align to plane to osculating circle of an edge. Origin is aligned to point of curvature. Optional vertex link defines where.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Aligner au plan du cercle osculateur d’une arête. L'origine est alignée au point de courbure. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="160"/>
      <source>Revolution Section</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Section de révolution</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="161"/>
      <source>Plane is perpendicular to edge, and Y axis is matched with axis of osculating circle. Optional vertex link defines where.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan est perpendiculaire à l'arête, et l'axe Y correspond à l'axe du cercle osculateur. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="163"/>
      <source>Plane by 3 points</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Plan par 3 points</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="164"/>
      <source>Align plane to pass through three vertices.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Aligner le plan pour passer par trois sommets.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="166"/>
      <source>Normal to 3 points</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Normal à 3 points</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="167"/>
      <source>Plane will pass through first two vertices, and perpendicular to plane that passes through three vertices.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Le plan traversera deux premiers sommets et sera perpendiculaire au plan qui passe par les trois sommets.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="169"/>
      <source>Folding</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Pliage</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="170"/>
      <source>Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. Plane will be aligned to folding the first edge.</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Mode spécialisé pour le pliage des polyèdres. Sélectionnez les 4 arêtes dans l’ordre : arête pliable, pliure, autre ligne de pliage, autre arête pliable. Le plan sera aligné sur le premier pli.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="172"/>
      <source>Inertia 2-3</source>
      <comment>AttachmentPlane mode caption</comment>
      <translation>Inertie 2-3</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="173"/>
      <source>Plane constructed on second and third principal axes of inertia (passes through center of mass).</source>
      <comment>AttachmentPlane mode tooltip</comment>
      <translation>Plan construit sur les deuxième et troisième des axes principaux d’inertie (passe par le centre de masse).</translation>
    </message>
  </context>
  <context>
    <name>Attacher3D</name>
    <message>
      <location filename="../../AttacherTexts.cpp" line="49"/>
      <source>Deactivated</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Désactivé</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="50"/>
      <source>Attachment is disabled. Object can be moved by editing Placement property.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>L'accrochage est désactivé. L'objet peut être déplacé en modifiant la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="52"/>
      <source>Translate origin</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Translation de l'origine</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="53"/>
      <source>Origin is aligned to match Vertex. Orientation is controlled by Placement property.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>L'origine est alignée sur un sommet. L’orientation est contrôlée par la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="55"/>
      <source>Object's X Y Z</source>
      <comment>Attachment3D mode caption</comment>
      <translation>X Y Z de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="56"/>
      <source>Placement is made equal to Placement of linked object.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Le placement est calqué sur le Placement de l’objet lié.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="58"/>
      <source>Object's X Z Y</source>
      <comment>Attachment3D mode caption</comment>
      <translation>X Z Y de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="59"/>
      <source>X', Y', Z' axes are matched with object's local X, Z, -Y, respectively.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Les axes X', Y', Z' sont respectivement mis en correspondances avec les axes locaux Y, Z -Y de l’objet.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="61"/>
      <source>Object's Y Z X</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Y Z X de l’objet</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="62"/>
      <source>X', Y', Z' axes are matched with object's local Y, Z, X, respectively.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Les axes X', Y', Z' sont respectivement mis en correspondances avec les axes locaux Y, Z, X de l’objet.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="64"/>
      <source>XY on plane</source>
      <comment>Attachment3D mode caption</comment>
      <translation>XY sur plan</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="65"/>
      <source>X' Y' plane is aligned to coincide planar face.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Le plan X' Y' est aligné pour coïncider avec la face plane.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="67"/>
      <source>XY tangent to surface</source>
      <comment>Attachment3D mode caption</comment>
      <translation>XY tangent à la surface</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="68"/>
      <source>X' Y' plane is made tangent to surface at vertex.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Le plan X' Y' est rendu tangent à la surface au niveau du sommet.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="70"/>
      <source>Z tangent to edge</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Z tangent à l'arête</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="71"/>
      <source>Z' axis is aligned to be tangent to edge. Optional vertex link defines where.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>L'axe Z est aligné pour être tangent à l'arête. Le lien facultatif vers un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="73"/>
      <source>Frenet NBT</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Frenet NBT</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="74"/>
      <location filename="../../AttacherTexts.cpp" line="77"/>
      <location filename="../../AttacherTexts.cpp" line="80"/>
      <source>Align to Frenet-Serret coordinate system of curved edge. Optional vertex link defines where.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Aligner au repère de Frenet-Serret de l'arête courbe. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="76"/>
      <source>Frenet TNB</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Frenet TNB</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="79"/>
      <source>Frenet TBN</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Frenet TBN</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="82"/>
      <source>Concentric</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Concentrique</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="83"/>
      <source>Align XY plane to osculating circle of an edge. Optional vertex link defines where.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Aligner le plan XY au cercle osculateur d'une arête. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="85"/>
      <source>Revolution Section</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Section de révolution</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="86"/>
      <source>Align Y' axis to match axis of osculating circle of an edge. Optional vertex link defines where.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Aligner l'axe Y au cercle osculateur d'une arête. Un lien facultatif à un sommet définit où.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="88"/>
      <source>XY plane by 3 points</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Plan XY par 3 points</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="89"/>
      <source>Align XY plane to pass through three vertices.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Aligner le plan XY pour passer par trois sommets.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="91"/>
      <source>XZ plane by 3 points</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Plan XZ par 3 points</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="92"/>
      <source>Align XZ plane to pass through 3 points; X axis will pass through two first points.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Aligner le plan XZ pour qu'il passe par 3 points ; l'axe X passera par les deux premiers points.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="94"/>
      <source>Folding</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Pliage</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="95"/>
      <source>Specialty mode for folding polyhedra. Select 4 edges in order: foldable edge, fold line, other fold line, other foldable edge. XY plane will be aligned to folding the first edge.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Mode spécialisé pour le pliage des polyèdres. Sélectionnez les 4 arêtes dans l’ordre : arête pliable, pliure, autre ligne de pliage, autre arête pliable. Le plan XY sera aligné sur le premier pli.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="97"/>
      <source>Inertial CS</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Système de coordonnées inertiel</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="98"/>
      <source>Inertial coordinate system, constructed on principal axes of inertia and center of mass.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Système de coordonnées inertiel, construit sur les axes principaux d’inertie et de centre de masse.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="100"/>
      <source>Align O-Z-X</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-Z-X</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="101"/>
      <source>Match origin with first Vertex. Align Z' and X' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes Z' et X' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="103"/>
      <source>Align O-Z-Y</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-Z-Y</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="104"/>
      <source>Match origin with first Vertex. Align Z' and Y' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes Z' et Y' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="106"/>
      <location filename="../../AttacherTexts.cpp" line="181"/>
      <source>Align O-X-Y</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-X-Y</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="107"/>
      <source>Match origin with first Vertex. Align X' and Y' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes X' et Y' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="109"/>
      <source>Align O-X-Z</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-X-Z</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="110"/>
      <source>Match origin with first Vertex. Align X' and Z' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes X' et Z' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="112"/>
      <source>Align O-Y-Z</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-Y-Z</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="113"/>
      <source>Match origin with first Vertex. Align Y' and Z' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes Y' et Z' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="115"/>
      <location filename="../../AttacherTexts.cpp" line="190"/>
      <source>Align O-Y-X</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-Y-X</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="116"/>
      <source>Match origin with first Vertex. Align Y' and X' axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes Y' et X' avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="175"/>
      <source>Align O-N-X</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-N-X</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="176"/>
      <source>Match origin with first Vertex. Align normal and horizontal plane axis towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner la normale et le plan horizontal avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="178"/>
      <source>Align O-N-Y</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-N-Y</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="179"/>
      <source>Match origin with first Vertex. Align normal and vertical plane axis towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner la normale et le plan vertical avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="182"/>
      <source>Match origin with first Vertex. Align horizontal and vertical plane axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes des plans horizontal et vertical avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="184"/>
      <source>Align O-X-N</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-X-N</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="185"/>
      <source>Match origin with first Vertex. Align horizontal plane axis and normal towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner le plan horizontal et la normale avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="187"/>
      <source>Align O-Y-N</source>
      <comment>Attachment3D mode caption</comment>
      <translation>Aligner O-Y-N</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="188"/>
      <source>Match origin with first Vertex. Align vertical plane axis and normal towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner le plan vertical et la normale avec un sommet ou le long d'une ligne.</translation>
    </message>
    <message>
      <location filename="../../AttacherTexts.cpp" line="191"/>
      <source>Match origin with first Vertex. Align vertical and horizontal plane axes towards vertex/along line.</source>
      <comment>Attachment3D mode tooltip</comment>
      <translation>Faire correspondre l'origine avec le premier sommet. Aligner les axes des plans vertical et horizontal avec un sommet ou le long d'une ligne.</translation>
    </message>
  </context>
  <context>
    <name>BlockDefinition</name>
    <message>
      <location filename="../../DlgBlock.ui" line="14"/>
      <source>Block definition</source>
      <translation>Définition de bloc</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="20"/>
      <source>First limit</source>
      <translation>Première limite</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="40"/>
      <location filename="../../DlgBlock.ui" line="221"/>
      <source>Type:</source>
      <translation>Type :</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="47"/>
      <location filename="../../DlgBlock.ui" line="201"/>
      <source>mm</source>
      <translation>mm</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="57"/>
      <location filename="../../DlgBlock.ui" line="257"/>
      <source>Length:</source>
      <translation>Longueur :</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="65"/>
      <location filename="../../DlgBlock.ui" line="229"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="70"/>
      <location filename="../../DlgBlock.ui" line="234"/>
      <source>Up to next</source>
      <translation>Jusqu'au suivant</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="75"/>
      <location filename="../../DlgBlock.ui" line="239"/>
      <source>Up to last</source>
      <translation>Jusqu'au dernier</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="80"/>
      <location filename="../../DlgBlock.ui" line="244"/>
      <source>Up to plane</source>
      <translation>Jusqu'au plan</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="85"/>
      <location filename="../../DlgBlock.ui" line="249"/>
      <source>Up to face</source>
      <translation>Jusqu'à la face</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="93"/>
      <location filename="../../DlgBlock.ui" line="264"/>
      <source>Limit:</source>
      <translation>Limite :</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="103"/>
      <location filename="../../DlgBlock.ui" line="142"/>
      <location filename="../../DlgBlock.ui" line="214"/>
      <location filename="../../DlgBlock.ui" line="309"/>
      <source>No selection</source>
      <translation>Aucune sélection</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="115"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="135"/>
      <source>Selection:</source>
      <translation>Sélection :</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="162"/>
      <source>Reverse</source>
      <translation>Inverser</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="169"/>
      <source>Both sides</source>
      <translation>Deux directions</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="181"/>
      <source>Second limit</source>
      <translation>Deuxième limite</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="276"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="288"/>
      <source>Perpendicular to sketch</source>
      <translation>Perpendiculaire à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../DlgBlock.ui" line="316"/>
      <source>Reference</source>
      <translation>Référence</translation>
    </message>
  </context>
  <context>
    <name>CmdBoxSelection</name>
    <message>
      <location filename="../../Command.cpp" line="2419"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2420"/>
      <location filename="../../Command.cpp" line="2421"/>
      <location filename="../../Command.cpp" line="2423"/>
      <source>Box selection</source>
      <translation>Sélection par boîte</translation>
    </message>
  </context>
  <context>
    <name>CmdCheckGeometry</name>
    <message>
      <location filename="../../Command.cpp" line="2127"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2128"/>
      <source>Check Geometry</source>
      <translation>Vérifier la géométrie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2129"/>
      <source>Analyzes Geometry For Errors</source>
      <translation>Analyse la géométrie</translation>
    </message>
  </context>
  <context>
    <name>CmdColorPerFace</name>
    <message>
      <location filename="../../Command.cpp" line="2161"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2162"/>
      <source>Color per face</source>
      <translation>Couleur par face</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2163"/>
      <source>Set the color of each individual face of the selected object.</source>
      <translation>Définir la couleur de chaque face individuelle de l’objet sélectionné.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureAngular</name>
    <message>
      <location filename="../../Command.cpp" line="2231"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2232"/>
      <source>Measure Angular</source>
      <translation>Mesure angulaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2233"/>
      <source>Measure the angle between two edges.</source>
      <translation>Mesurer l'angle entre deux arêtes.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureClearAll</name>
    <message>
      <location filename="../../Command.cpp" line="2290"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2291"/>
      <source>Clear All</source>
      <translation>Tout effacer</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2292"/>
      <source>Clear all dimensions from the screen.</source>
      <translation>Effacer toutes les cotes à l’écran.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureLinear</name>
    <message>
      <location filename="../../Command.cpp" line="2200"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2201"/>
      <source>Measure Linear</source>
      <translation>Mesure linéaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2202"/>
      <source>Measure the linear distance between two points;
if edges or faces are picked, it will measure
between two vertices of them.</source>
      <translation>Mesurer la distance linéaire entre deux points ;
si des arêtes ou des faces sont sélectionnées, ce sera
la distance entre deux sommets d’entre elles.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureRefresh</name>
    <message>
      <location filename="../../Command.cpp" line="2260"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2261"/>
      <source>Refresh</source>
      <translation>Actualiser</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2262"/>
      <source>Recalculate the dimensions
if the measured points have moved.</source>
      <translation>Recalculer les cotes
si les points mesurés ont été déplacés.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureToggle3d</name>
    <message>
      <location filename="../../Command.cpp" line="2356"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2357"/>
      <source>Toggle 3D</source>
      <translation>Basculer en 3D</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2358"/>
      <source>Toggle on and off all direct dimensions,
including angular.</source>
      <translation>Basculer toutes les cotes directes,
y compris angulaire.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureToggleAll</name>
    <message>
      <location filename="../../Command.cpp" line="2319"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2320"/>
      <source>Toggle All</source>
      <translation>Basculer tous</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2321"/>
      <source>Toggle on and off all currently visible dimensions,
direct, orthogonal, and angular.</source>
      <translation>Basculer toutes les cotes actuellement visibles,
directes, orthogonales et angulaires.</translation>
    </message>
  </context>
  <context>
    <name>CmdMeasureToggleDelta</name>
    <message>
      <location filename="../../Command.cpp" line="2387"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2388"/>
      <source>Toggle Delta</source>
      <translation>Basculer Delta</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2389"/>
      <source>Toggle on and off all orthogonal dimensions,
meaning that a direct dimension will be decomposed
into its X, Y, and Z components.</source>
      <translation>Basculer toutes les cotes orthogonales,
ce qui signifie qu’une cote directe sera décomposée
en ses composantes X, Y et Z.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="1290"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1291"/>
      <source>Boolean...</source>
      <translation>Opération booléenne...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1292"/>
      <source>Run a boolean operation with two shapes selected</source>
      <translation>Exécuter une opération booléenne sur deux formes sélectionnées</translation>
    </message>
  </context>
  <context>
    <name>CmdPartBox</name>
    <message>
      <location filename="../../CommandParametric.cpp" line="87"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="88"/>
      <location filename="../../CommandParametric.cpp" line="99"/>
      <location filename="../../CommandParametric.cpp" line="104"/>
      <source>Cube</source>
      <translation>Cube</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="89"/>
      <source>Create a cube solid</source>
      <translation>Créer un cube plein</translation>
    </message>
  </context>
  <context>
    <name>CmdPartBox2</name>
    <message>
      <location filename="../../Command.cpp" line="136"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="137"/>
      <source>Box fix 1</source>
      <translation>Boîte correctif 1</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>Create a box solid without dialog</source>
      <translation>Créer une boîte solide sans boîte de dialogue</translation>
    </message>
  </context>
  <context>
    <name>CmdPartBox3</name>
    <message>
      <location filename="../../Command.cpp" line="177"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="178"/>
      <source>Box fix 2</source>
      <translation>Boîte correctif 2</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="179"/>
      <source>Create a box solid without dialog</source>
      <translation>Créer une boîte solide sans boîte de dialogue</translation>
    </message>
  </context>
  <context>
    <name>CmdPartBuilder</name>
    <message>
      <location filename="../../Command.cpp" line="1553"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1554"/>
      <source>Shape builder...</source>
      <translation>Générateur de forme...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1555"/>
      <source>Advanced utility to create shapes</source>
      <translation>Utilitaire avancé de création de formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1457"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1458"/>
      <source>Chamfer...</source>
      <translation>Chanfrein...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1459"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>Chanfreiner les arêtes sélectionnées d'une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCommon</name>
    <message>
      <location filename="../../Command.cpp" line="350"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="351"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="352"/>
      <source>Make an intersection of two shapes</source>
      <translation>Exécuter une intersection entre deux formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCompCompoundTools</name>
    <message>
      <location filename="../../Command.cpp" line="779"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="780"/>
      <source>Compound tools</source>
      <translation>Outils composés</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="781"/>
      <source>Compound tools: working with lists of shapes.</source>
      <translation>Outils de composition : travailler avec des listes de formes.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCompJoinFeatures</name>
    <message>
      <location filename="../../Command.cpp" line="551"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="552"/>
      <source>Join objects...</source>
      <translation>Joindre les objets...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="553"/>
      <source>Join walled objects</source>
      <translation>Joindre les objets à paroi</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCompOffset</name>
    <message>
      <location filename="../../Command.cpp" line="1743"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1744"/>
      <source>Offset:</source>
      <translation>Décalage :</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>Tools to offset shapes (construct parallel shapes)</source>
      <translation>Outils pour décaler des formes (construire des formes parallèles)</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCompSplitFeatures</name>
    <message>
      <location filename="../../Command.cpp" line="657"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="658"/>
      <source>Split objects...</source>
      <translation>Éclater des objets...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="659"/>
      <source>Shape splitting tools. Compsolid creation tools. OCC 6.9.0 or later is required.</source>
      <translation>Outils de division de forme. Outils de création de Compsolid. OCC 6.9.0 ou ultérieur est requis.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCompound</name>
    <message>
      <location filename="../../Command.cpp" line="886"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="887"/>
      <source>Make compound</source>
      <translation>Créer un composé</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="888"/>
      <source>Make a compound of several shapes</source>
      <translation>Créer un composé à partir de plusieurs formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCone</name>
    <message>
      <location filename="../../CommandParametric.cpp" line="169"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="170"/>
      <location filename="../../CommandParametric.cpp" line="181"/>
      <location filename="../../CommandParametric.cpp" line="186"/>
      <source>Cone</source>
      <translation>Cône</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="171"/>
      <source>Create a cone solid</source>
      <translation>Créer un solide conique</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCrossSections</name>
    <message>
      <location filename="../../Command.cpp" line="1513"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1514"/>
      <source>Cross-sections...</source>
      <translation>Coupes...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1515"/>
      <source>Cross-sections</source>
      <translation>Coupes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCut</name>
    <message>
      <location filename="../../Command.cpp" line="272"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="273"/>
      <source>Cut</source>
      <translation>Soustraction</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="274"/>
      <source>Make a cut of two shapes</source>
      <translation>Exécuter une soustraction sur deux formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartCylinder</name>
    <message>
      <location filename="../../CommandParametric.cpp" line="46"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="47"/>
      <location filename="../../CommandParametric.cpp" line="58"/>
      <location filename="../../CommandParametric.cpp" line="63"/>
      <source>Cylinder</source>
      <translation>Cylindre</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="48"/>
      <source>Create a Cylinder</source>
      <translation>Créer un cylindre</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDefeaturing</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="416"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="417"/>
      <source>Defeaturing</source>
      <translation>Supprimer la fonctionnalité</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="418"/>
      <source>Remove feature from a shape</source>
      <translation>Supprimer la fonction d’une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartElementCopy</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="328"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="329"/>
      <source>Create shape element copy</source>
      <translation>Créer une copie d’élément forme</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="330"/>
      <source>Create a non-parametric copy of the selected shape element</source>
      <translation>Créer une copie non paramétrique de l'élément forme sélectionné</translation>
    </message>
  </context>
  <context>
    <name>CmdPartExport</name>
    <message>
      <location filename="../../Command.cpp" line="1053"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1054"/>
      <source>Export CAD...</source>
      <translation>Exportation CAO...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1055"/>
      <source>Exports to a CAD file</source>
      <translation>Exporte vers un fichier dans un format de CAO</translation>
    </message>
  </context>
  <context>
    <name>CmdPartExtrude</name>
    <message>
      <location filename="../../Command.cpp" line="1321"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1322"/>
      <source>Extrude...</source>
      <translation>Extrusion...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1323"/>
      <source>Extrude a selected sketch</source>
      <translation>Extrusion d'une esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1429"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1430"/>
      <source>Fillet...</source>
      <translation>Congé...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1431"/>
      <source>Fillet the selected edges of a shape</source>
      <translation>Créer un congé sur les arêtes sélectionnées d'une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartFuse</name>
    <message>
      <location filename="../../Command.cpp" line="450"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="451"/>
      <source>Union</source>
      <translation>Union</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="452"/>
      <source>Make a union of several shapes</source>
      <translation>Exécuter l'union de plusieurs formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartImport</name>
    <message>
      <location filename="../../Command.cpp" line="991"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Import CAD...</source>
      <translation>Importation CAO...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="993"/>
      <source>Imports a CAD file</source>
      <translation>Importe un fichier CAO</translation>
    </message>
  </context>
  <context>
    <name>CmdPartImportCurveNet</name>
    <message>
      <location filename="../../Command.cpp" line="1100"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1101"/>
      <source>Import curve network...</source>
      <translation>Importer réseau de courbes...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1102"/>
      <source>Import a curve network</source>
      <translation>Importer un réseau de courbes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1582"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1583"/>
      <source>Loft...</source>
      <translation>Lissage...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1584"/>
      <source>Utility to loft</source>
      <translation>Lissage d'une série de profils</translation>
    </message>
  </context>
  <context>
    <name>CmdPartMakeFace</name>
    <message>
      <location filename="../../Command.cpp" line="1349"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1350"/>
      <source>Make face from wires</source>
      <translation>Créer une face à partir de fils</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1351"/>
      <source>Make face from set of wires (e.g. from a sketch)</source>
      <translation>Créer une face à partir d'un ensemble de polylignes (par exemple, à partir d’une esquisse)</translation>
    </message>
  </context>
  <context>
    <name>CmdPartMakeSolid</name>
    <message>
      <location filename="../../Command.cpp" line="1148"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1149"/>
      <source>Convert to solid</source>
      <translation>Convertir en solide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1150"/>
      <source>Create solid from a shell or compound</source>
      <translation>Créer un solide à partir d'une coque ou d'un composé</translation>
    </message>
  </context>
  <context>
    <name>CmdPartMirror</name>
    <message>
      <location filename="../../Command.cpp" line="1485"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1486"/>
      <source>Mirroring...</source>
      <translation>Mise en miroir...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1487"/>
      <source>Mirroring a selected shape</source>
      <translation>Mise en miroir de la forme sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartOffset</name>
    <message>
      <location filename="../../Command.cpp" line="1640"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1641"/>
      <source>3D Offset...</source>
      <translation>Décalage 3D...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1642"/>
      <source>Utility to offset in 3D</source>
      <translation>Utilitaire pour décaler en 3D</translation>
    </message>
  </context>
  <context>
    <name>CmdPartOffset2D</name>
    <message>
      <location filename="../../Command.cpp" line="1692"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1693"/>
      <source>2D Offset...</source>
      <translation>Décalage 2D...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1694"/>
      <source>Utility to offset planar shapes</source>
      <translation>Utilitaire pour décaler des formes planaires</translation>
    </message>
  </context>
  <context>
    <name>CmdPartPickCurveNet</name>
    <message>
      <location filename="../../Command.cpp" line="90"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="91"/>
      <source>Pick curve network</source>
      <translation>Choisir un réseau de courbes</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="92"/>
      <source>Pick a curve network</source>
      <translation>Choisir un réseau de courbes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartPointsFromMesh</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="179"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="180"/>
      <source>Create points object from mesh</source>
      <translation>Créer un objet points à partir d'un maillage</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="181"/>
      <source>Create selectable points object from selected mesh object</source>
      <translation>Créer un objet points sélectionnable à partir de l'objet maillage sélectionné</translation>
    </message>
  </context>
  <context>
    <name>CmdPartPrimitives</name>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>Create primitives...</source>
      <translation>Créer des primitives...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Creation of parametrized geometric primitives</source>
      <translation>Création de primitives géométriques paramétrées</translation>
    </message>
  </context>
  <context>
    <name>CmdPartProjectionOnSurface</name>
    <message>
      <location filename="../../Command.cpp" line="2448"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2449"/>
      <source>Create projection on surface...</source>
      <translation>Créer une projection sur la surface...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2450"/>
      <source>Project edges, wires, or faces of one object
onto a face of another object.
The camera view determines the direction
of projection.</source>
      <translation>Projeter des arêtes, des fils ou des faces d’'un objet
sur une face d’un autre objet.
La vue caméra détermine la direction
de la projection.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartRefineShape</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="356"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="357"/>
      <source>Refine shape</source>
      <translation>Affiner la forme</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="358"/>
      <source>Refine the copy of a shape</source>
      <translation>Affiner la copie d'une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartReverseShape</name>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Reverse shapes</source>
      <translation>Inverser les formes</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1229"/>
      <source>Reverse orientation of shapes</source>
      <translation>Inverser l'orientation des formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartRevolve</name>
    <message>
      <location filename="../../Command.cpp" line="1401"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1402"/>
      <source>Revolve...</source>
      <translation>Révolution...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1403"/>
      <source>Revolve a selected shape</source>
      <translation>Révolutionner une forme sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartRuledSurface</name>
    <message>
      <location filename="../../Command.cpp" line="2011"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2012"/>
      <source>Create ruled surface</source>
      <translation>Créer une surface réglée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2013"/>
      <source>Create a ruled surface from either two Edges or two wires</source>
      <translation>Créer une surface réglée depuis deux arêtes ou deux fils</translation>
    </message>
  </context>
  <context>
    <name>CmdPartSection</name>
    <message>
      <location filename="../../Command.cpp" line="943"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="944"/>
      <source>Section</source>
      <translation>Section</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="945"/>
      <source>Make a section of two shapes</source>
      <translation>Faire une section de deux formes</translation>
    </message>
  </context>
  <context>
    <name>CmdPartShapeFromMesh</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="107"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="108"/>
      <source>Create shape from mesh...</source>
      <translation>Créer la forme à partir d'un maillage...</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="109"/>
      <source>Create shape from selected mesh object</source>
      <translation>Créer une forme à partir du maillage sélectionné</translation>
    </message>
  </context>
  <context>
    <name>CmdPartSimpleCopy</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="229"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="230"/>
      <source>Create simple copy</source>
      <translation>Créer une copie simple</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="231"/>
      <source>Create a simple non-parametric copy</source>
      <translation>Créer une copie simple non-paramétrique</translation>
    </message>
  </context>
  <context>
    <name>CmdPartSimpleCylinder</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="57"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="58"/>
      <source>Create Cylinder...</source>
      <translation>Créer un cylindre...</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="59"/>
      <source>Create a Cylinder</source>
      <translation>Créer un cylindre</translation>
    </message>
  </context>
  <context>
    <name>CmdPartSphere</name>
    <message>
      <location filename="../../CommandParametric.cpp" line="128"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="129"/>
      <location filename="../../CommandParametric.cpp" line="140"/>
      <location filename="../../CommandParametric.cpp" line="145"/>
      <source>Sphere</source>
      <translation>Sphère</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="130"/>
      <source>Create a sphere solid</source>
      <translation>Créer un solide sphérique</translation>
    </message>
  </context>
  <context>
    <name>CmdPartSweep</name>
    <message>
      <location filename="../../Command.cpp" line="1611"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>Sweep...</source>
      <translation>Balayage...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Utility to sweep</source>
      <translation>Balayage d'une ou plusieurs section le long d'un chemin</translation>
    </message>
  </context>
  <context>
    <name>CmdPartThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1836"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1837"/>
      <source>Thickness...</source>
      <translation>Évidement...</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1838"/>
      <source>Utility to apply a thickness</source>
      <translation>Évidement d'un solide sélectionné</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1850"/>
      <location filename="../../Command.cpp" line="1870"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1851"/>
      <source>Selected one or more faces of a shape</source>
      <translation>Sélectionner une ou plusieurs faces d'une forme</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1871"/>
      <source>Selected shape is not a solid</source>
      <translation>La forme sélectionnée n'est pas un solide</translation>
    </message>
  </context>
  <context>
    <name>CmdPartTorus</name>
    <message>
      <location filename="../../CommandParametric.cpp" line="210"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="211"/>
      <location filename="../../CommandParametric.cpp" line="222"/>
      <location filename="../../CommandParametric.cpp" line="227"/>
      <source>Torus</source>
      <translation>Tore</translation>
    </message>
    <message>
      <location filename="../../CommandParametric.cpp" line="212"/>
      <source>Create a torus solid</source>
      <translation>Créer un solide toroïdal</translation>
    </message>
  </context>
  <context>
    <name>CmdPartTransformedCopy</name>
    <message>
      <location filename="../../CommandSimple.cpp" line="300"/>
      <source>Part</source>
      <translation>Pièce</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="301"/>
      <source>Create transformed copy</source>
      <translation>Créer une copie transformée</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="302"/>
      <source>Create a non-parametric copy with transformed placement</source>
      <translation>Créer une copie non paramétrique avec placement transformé</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="147"/>
      <location filename="../../Command.cpp" line="188"/>
      <source>Part Box Create</source>
      <translation>Créer une boîte de pièces</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Part Cut</source>
      <translation>Part Soustraction</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="408"/>
      <source>Common</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="508"/>
      <source>Fusion</source>
      <translation>Fusion</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="921"/>
      <source>Compound</source>
      <translation>Composé</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="966"/>
      <source>Section</source>
      <translation>Section</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1017"/>
      <source>Import Part</source>
      <translation>Importer une pièce</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1123"/>
      <source>Part Import Curve Net</source>
      <translation>Part Réseau de courbe d'importation</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1240"/>
      <source>Reverse</source>
      <translation>Inverser</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1363"/>
      <source>Make face</source>
      <translation>Réaliser une face</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1657"/>
      <source>Make Offset</source>
      <translation>Créer un décalage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1709"/>
      <source>Make 2D Offset</source>
      <translation>Créer un décalage 2D</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1877"/>
      <source>Make Thickness</source>
      <translation>Créer une épaisseur</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2102"/>
      <source>Create ruled surface</source>
      <translation>Créer une surface réglée</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="72"/>
      <source>Create Part Cylinder</source>
      <translation>Créer une pièce cylindre</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="137"/>
      <source>Convert mesh</source>
      <translation>Convertir le maillage</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="196"/>
      <source>Points from mesh</source>
      <translation>Points depuis le maillage</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="373"/>
      <source>Refine shape</source>
      <translation>Affiner la forme</translation>
    </message>
    <message>
      <location filename="../../CommandSimple.cpp" line="430"/>
      <source>Defeaturing</source>
      <translation>Supprimer la fonctionnalité</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="1063"/>
      <source>Edit attachment</source>
      <translation>Modifier la pièce jointe</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.cpp" line="432"/>
      <source>Change face colors</source>
      <translation>Changer les couleurs de la face</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="202"/>
      <source>Loft</source>
      <translation>Lissage</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="240"/>
      <source>Edge</source>
      <translation>Arête</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="281"/>
      <source>Wire</source>
      <translation>Fil</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="333"/>
      <location filename="../../TaskShapeBuilder.cpp" line="385"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="448"/>
      <source>Shell</source>
      <translation>Coque</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="500"/>
      <source>Solid</source>
      <translation>Solide</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="342"/>
      <source>Sweep</source>
      <translation>Balayage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirror.cpp" line="199"/>
      <source>Edit Mirror</source>
      <translation>Editer le miroir</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDatumParameters</name>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="20"/>
      <source>Selection accepted</source>
      <translation>Sélection acceptée</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="35"/>
      <source>Reference 1</source>
      <translation>Référence 1</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="52"/>
      <source>Reference 2</source>
      <translation>Référence 2</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="69"/>
      <source>Reference 3</source>
      <translation>Référence 3</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="86"/>
      <source>Reference 4</source>
      <translation>Référence 4</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="101"/>
      <source>Attachment mode:</source>
      <translation>Mode d'accrochage :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="124"/>
      <source>Attachment Offset (in local coordinates):</source>
      <translation>Décalage de l'accrochage (en coordonnées locales) :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="136"/>
      <source>In x-direction:</source>
      <translation>Dans la direction x :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="152"/>
      <source>In y-direction:</source>
      <translation>Dans la direction y :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="171"/>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="203"/>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="261"/>
      <source>Note: The placement is expressed in local space of object being attached.</source>
      <translation>Note : Le placement est exprimé localement par rapport à l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="184"/>
      <source>In z-direction:</source>
      <translation>Dans la direction z :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="216"/>
      <source>Around x-axis:</source>
      <translation>Autour de l'axe x :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="229"/>
      <source>Around y-axis:</source>
      <translation>Autour de l'axe y :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="242"/>
      <source>Around z-axis:</source>
      <translation>Autour de l'axe z :</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="280"/>
      <source>Rotation around the x-axis
Note: The placement is expressed in local space of object being attached.</source>
      <translation>Rotation autour de l'axe x
Note : Le placement est exprimé localement par rapport à l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="312"/>
      <source>Rotation around the y-axis
Note: The placement is expressed in local space of object being attached.</source>
      <translation>Rotation autour de l'axe y
Note : Le placement est exprimé localement par rapport à l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="344"/>
      <source>Rotation around the z-axis
Note: The placement is expressed in local space of object being attached.</source>
      <translation>Rotation autour de l'axe z
Note : Le placement est exprimé localement par rapport à l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../../AttachmentEditor/TaskAttachmentEditor.ui" line="367"/>
      <source>Flip sides</source>
      <translation>Retourner</translation>
    </message>
  </context>
  <context>
    <name>PartGui::CrossSections</name>
    <message>
      <location filename="../../CrossSections.ui" line="14"/>
      <source>Cross sections</source>
      <translation>Coupes</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="20"/>
      <source>Guiding plane</source>
      <translation>Plan guide</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="26"/>
      <source>XY</source>
      <translation>XY</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="36"/>
      <source>XZ</source>
      <translation>XZ</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="43"/>
      <source>YZ</source>
      <translation>YZ</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="52"/>
      <source>Position:</source>
      <translation>Position :</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="71"/>
      <source>Sections</source>
      <translation>Sections</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="83"/>
      <source>On both sides</source>
      <translation>Des deux côtés</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="92"/>
      <source>Count</source>
      <translation>Nombre</translation>
    </message>
    <message>
      <location filename="../../CrossSections.ui" line="109"/>
      <source>Distance:</source>
      <translation>Distance :</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgBooleanOperation</name>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="14"/>
      <source>Boolean Operation</source>
      <translation>Opération booléenne</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="20"/>
      <source>Boolean operation</source>
      <translation>Opération booléenne</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="32"/>
      <source>Section</source>
      <translation>Section</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="39"/>
      <source>Difference</source>
      <translation>Différence</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="46"/>
      <source>Union</source>
      <translation>Union</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="56"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="76"/>
      <source>First shape</source>
      <translation>Première forme</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="81"/>
      <location filename="../../DlgBooleanOperation.ui" line="119"/>
      <source>Solids</source>
      <translation>Solides</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="86"/>
      <location filename="../../DlgBooleanOperation.ui" line="124"/>
      <source>Shells</source>
      <translation>Coques</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="91"/>
      <location filename="../../DlgBooleanOperation.ui" line="129"/>
      <source>Compounds</source>
      <translation>Composés</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="96"/>
      <location filename="../../DlgBooleanOperation.ui" line="134"/>
      <source>Faces</source>
      <translation>Faces</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="114"/>
      <source>Second shape</source>
      <translation>Deuxième forme</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.ui" line="155"/>
      <source>Swap selection</source>
      <translation>Changement de sélection</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="378"/>
      <source>Select a shape on the left side, first</source>
      <translation>Sélectionner d'abord une forme sur le côté gauche</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="383"/>
      <source>Select a shape on the right side, first</source>
      <translation>Sélectionner d'abord une forme sur le côté droit</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="388"/>
      <source>Cannot perform a boolean operation with the same shape</source>
      <translation>Impossible d'effectuer une opération booléenne sur une seule forme</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="398"/>
      <source>No active document available</source>
      <translation>Aucun document actif disponible</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="408"/>
      <source>One of the selected objects doesn't exist anymore</source>
      <translation>Un des objets sélectionnés n'existe plus</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="415"/>
      <source>Performing union on non-solids is not possible</source>
      <translation>L'union de formes non-solides n'est pas possible</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="424"/>
      <source>Performing intersection on non-solids is not possible</source>
      <translation>L'intersection de formes non-solides n'est pas possible</translation>
    </message>
    <message>
      <location filename="../../DlgBooleanOperation.cpp" line="433"/>
      <source>Performing difference on non-solids is not possible</source>
      <translation>La soustraction de formes non-solides n'est pas possible</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgChamferEdges</name>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="1041"/>
      <source>Chamfer Edges</source>
      <translation>Chanfreiner des arêtes</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgExtrusion</name>
    <message>
      <location filename="../../DlgExtrusion.ui" line="14"/>
      <source>Extrude</source>
      <translation>Extruder</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="26"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="32"/>
      <source>If checked, direction of extrusion is reversed.</source>
      <translation>Si coché, la direction d’extrusion est inversée.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="35"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="42"/>
      <source>Specify direction manually using X,Y,Z values.</source>
      <translation>Spécifier la direction manuellement à l’aide des valeurs X, Y, Z.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="45"/>
      <source>Custom direction:</source>
      <translation>Orientation personnalisée :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="52"/>
      <source>Extrude perpendicularly to plane of input shape.</source>
      <translation>Extruder perpendiculairement au plan de la forme d’entrée.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="55"/>
      <source>Along normal</source>
      <translation>Le long de la normale</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="65"/>
      <source>Click to start selecting an edge in 3d view.</source>
      <translation>Cliquez pour commencer à sélectionner une arête dans la vue 3d.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="68"/>
      <location filename="../../DlgExtrusion.cpp" line="208"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="78"/>
      <source>Set direction to match a direction of straight edge. Hint: to account for length of the edge too, set both lengths to zero.</source>
      <translation>Définir l'orientation pour qu'elle corresponde à celle d'une arête droite. Astuce : pour tenir compte de la longueur de l’arête, définir les deux longueurs à zéro.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="81"/>
      <source>Along edge:</source>
      <translation>Le long d'une arête :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="99"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="122"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="145"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="179"/>
      <source>Length</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="185"/>
      <source>Along:</source>
      <translation>Le long de :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="204"/>
      <source>Length to extrude along direction (can be negative). If both lengths are zero, magnitude of direction is used.</source>
      <translation>Longueur à extruder le long d'une direction (peut être négative). Si les deux longueurs sont nulles, la magnitude de la direction est utilisée.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="223"/>
      <source>Against:</source>
      <translation>Contre :</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="242"/>
      <source>Length to extrude against direction (can be negative).</source>
      <translation>Longueur à extruder dans le sens inverse (peut être négative).</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="261"/>
      <source>Distribute extrusion length equally to both sides.</source>
      <translation>Distribuer la longueur d’extrusion également des deux cotés.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="264"/>
      <source>Symmetric</source>
      <translation>Symétrique</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="276"/>
      <source>Taper outward angle</source>
      <translation>Angle de dépouille externe</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="289"/>
      <location filename="../../DlgExtrusion.ui" line="314"/>
      <source>Apply slope (draft) to extrusion side faces.</source>
      <translation>Appliquer un angle de dépouille aux faces extrudées.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="335"/>
      <source>If checked, extruding closed wires will give solids, not shells.</source>
      <translation>Si coché, l'extrusion de contours fermés produira des solides, non des coques.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="338"/>
      <source>Create solid</source>
      <translation>Créer le solide</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.ui" line="368"/>
      <source>Shape</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="182"/>
      <source>Selecting...</source>
      <translation>Sélection de...</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="423"/>
      <source>The document '%1' doesn't exist.</source>
      <translation>Document « %1 » inexistant.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="470"/>
      <location filename="../../DlgExtrusion.cpp" line="475"/>
      <source>Creating Extrusion failed.
%1</source>
      <translation>La création d’extrusion a échoué.
%1</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="542"/>
      <source>Object not found: %1</source>
      <translation>Objet introuvable : %1</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="604"/>
      <source>No shapes selected for extrusion. Select some, first.</source>
      <translation>Aucune forme sélectionnée pour l’extrusion. Sélectionnez-en une d’abord.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="625"/>
      <source>Extrusion direction link is invalid.

%1</source>
      <translation>Le lien de direction d'extrusion est invalide.

%1</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="627"/>
      <source>Direction mode is to use an edge, but no edge is linked.</source>
      <translation>Le mode de direction consiste à utiliser une arête, mais aucune arête n’est liée.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="650"/>
      <source>Can't determine normal vector of shape to be extruded. Please use other mode. 

(%1)</source>
      <translation>Impossible de déterminer le vecteur normal de la forme à extruder. Veuillez utiliser un autre mode. 

(%1)</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="660"/>
      <source>Extrusion direction vector is zero-length. It must be non-zero.</source>
      <translation>La direction du vecteur d’extrusion est de longueur nulle. Elle doit être différente de zéro.</translation>
    </message>
    <message>
      <location filename="../../DlgExtrusion.cpp" line="671"/>
      <source>Total extrusion length is zero (length1 == -length2). It must be nonzero.</source>
      <translation>Longueur totale d’extrusion est à zéro (length1 == - length2). Elle doit être différente de zéro.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgFilletEdges</name>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="14"/>
      <source>Fillet Edges</source>
      <translation>Congés des arêtes</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="20"/>
      <source>Shape</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="32"/>
      <source>Selected shape:</source>
      <translation>Forme sélectionnée :</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="40"/>
      <source>No selection</source>
      <translation>Aucune sélection</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="51"/>
      <source>Fillet Parameter</source>
      <translation>Paramètres de congé</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="57"/>
      <source>Selection</source>
      <translation>Sélection</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="63"/>
      <source>Select edges</source>
      <translation>Sélectionner les arêtes</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="73"/>
      <source>Select faces</source>
      <translation>Sélectionner les faces</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="80"/>
      <source>All</source>
      <translation>Tous</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="87"/>
      <source>None</source>
      <translation>Aucun</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="110"/>
      <source>Fillet type:</source>
      <translation>Type de congé :</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="118"/>
      <source>Constant Radius</source>
      <translation>Rayon constant</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="123"/>
      <source>Variable Radius</source>
      <translation>Rayon variable</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.ui" line="142"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="266"/>
      <source>Length:</source>
      <translation>Longueur :</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="267"/>
      <source>Constant Length</source>
      <translation>Longueur constante</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="268"/>
      <source>Variable Length</source>
      <translation>Longueur variable</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="270"/>
      <source>Edges to chamfer</source>
      <translation>Arêtes à chanfreiner</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="271"/>
      <location filename="../../DlgFilletEdges.cpp" line="838"/>
      <source>Start length</source>
      <translation>Longueur de départ</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="272"/>
      <source>End length</source>
      <translation>Longueur finale</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="275"/>
      <source>Edges to fillet</source>
      <translation>Congé sur arêtes</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="276"/>
      <location filename="../../DlgFilletEdges.cpp" line="840"/>
      <source>Start radius</source>
      <translation>Rayon initial</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="277"/>
      <source>End radius</source>
      <translation>Rayon final</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="687"/>
      <location filename="../../DlgFilletEdges.cpp" line="747"/>
      <source>Edge%1</source>
      <translation>Arête%1</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="830"/>
      <source>Length</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="832"/>
      <source>Radius</source>
      <translation>Rayon</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="886"/>
      <source>No shape selected</source>
      <translation>Aucune forme sélectionnée</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="887"/>
      <source>No valid shape is selected.
Please select a valid shape in the drop-down box first.</source>
      <translation>Aucune forme valide n'est sélectionnée. Veuillez sélectionner d'abord une forme valide dans le menu déroulant.</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="938"/>
      <source>No edge selected</source>
      <translation>Aucune arête sélectionnée</translation>
    </message>
    <message>
      <location filename="../../DlgFilletEdges.cpp" line="939"/>
      <source>No edge entity is checked to fillet.
Please check one or more edge entities first.</source>
      <translation>Aucune arête n'est sélectionnée pour le congé. Veuillez d'abord cocher une ou plusieurs arêtes.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgImportExportIges</name>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="14"/>
      <source>IGES</source>
      <translation>IGES</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="20"/>
      <source>Export</source>
      <translation>Exporter</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="26"/>
      <source>Units for export of IGES</source>
      <translation>Unités pour l'exportation vers IGES</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="47"/>
      <source>Millimeter</source>
      <translation>Millimètre</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="52"/>
      <source>Meter</source>
      <translation>Mètre</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="57"/>
      <source>Inch</source>
      <translation>Pouce</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="65"/>
      <source>Write solids and shells as</source>
      <translation>Exporter les solides et les coques comme</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="71"/>
      <source>Solids and shells will be exported as trimmed surface</source>
      <translation>Les solides et les coques seront exportés en tant que surface coupée</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="74"/>
      <source>Groups of Trimmed Surfaces (type 144)</source>
      <translation>Groupes de surfaces ajustées (type 144)</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="84"/>
      <source>Solids will be exported as manifold solid B-Rep object, shells as shell</source>
      <translation>Les solides seront exportés en tant qu'objets B-Rep solides de type manifold, les coques sous la forme d'une coque</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="87"/>
      <source>Solids (type 186) and Shells (type 514) / B-REP mode</source>
      <translation>Solides (type 186) et coques (type 514) / mode B-REP</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="100"/>
      <source>Import</source>
      <translation>Importation</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="106"/>
      <source>Blank entities will not be imported</source>
      <translation>Les entités vides ne seront pas importées</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="109"/>
      <source>Skip blank entities</source>
      <translation>Ignorer les entités vides</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="119"/>
      <source>If not empty, field contents will be used in the IGES file header</source>
      <translation>Si non vide, le contenu du champ sera utilisé dans l'en-tête du fichier IGES</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="122"/>
      <source>Header</source>
      <translation>En-tête</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="128"/>
      <source>Company</source>
      <translation>Société</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="138"/>
      <source>Product</source>
      <translation>Produit</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportIges.ui" line="151"/>
      <source>Author</source>
      <translation>Auteur</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgImportExportStep</name>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="14"/>
      <source>STEP</source>
      <translation>STEP</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="20"/>
      <source>Export</source>
      <translation>Exporter</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="26"/>
      <source>Scheme</source>
      <translation>Schéma</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="64"/>
      <source>Uncheck this to skip invisible object when exporting, which is useful for CADs that do not support invisibility STEP styling.</source>
      <translation>Décochez cette case pour ignorer l'objet invisible lors de l'exportation, ce qui est utile pour les CAO qui ne prennent pas en charge l'invisibilité du style STEP.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="67"/>
      <source>Export invisible objects</source>
      <translation>Exporter les objets invisibles</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="80"/>
      <source>Units for export of STEP</source>
      <translation>Unités pour l'exportation STEP</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="87"/>
      <source>Write out curves in parametric space of surface</source>
      <translation>Enregistrer les courbes dans l’espace paramétrique de surface</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="95"/>
      <source>Millimeter</source>
      <translation>Millimètre</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="100"/>
      <source>Meter</source>
      <translation>Mètre</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="105"/>
      <source>Inch</source>
      <translation>Pouce</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="126"/>
      <source>Use legacy exporter</source>
      <translation>Utiliser l’ancien exporteur</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="139"/>
      <source>Check this option to keep the placement information when exporting
a single object. Please note that when import back the STEP file, the
placement will be encoded into the shape geometry, instead of keeping
it inside the Placement property.</source>
      <translation>Cochez cette option pour conserver les informations de placement lors de l'exportation d'un seul objet. Veuillez noter que lors de la réimportation du fichier STEP, le placement sera encodé dans la géométrie de la forme, au lieu de le conserver dans la propriété Placement.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="145"/>
      <source>Export single object placement</source>
      <translation>Exporter le placement d'un seul objet</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="174"/>
      <source>If not empty, field contents will be used in the STEP file header.</source>
      <translation>Si non vide, le contenu du champ sera utilisé dans l'en-tête du fichier STEP.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="177"/>
      <source>Header</source>
      <translation>En-tête</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="189"/>
      <source>Author</source>
      <translation>Auteur</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="196"/>
      <source>Product</source>
      <translation>Produit</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="203"/>
      <source>Company</source>
      <translation>Société</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="216"/>
      <source>Import</source>
      <translation>Importation</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="222"/>
      <source>If checked, no Compound merge will be done
during file reading (slower but higher details).</source>
      <translation>Si cette case est cochée, aucune fusion composée ne sera effectuée pendant la lecture du fichier (plus lent mais plus détaillé).</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="226"/>
      <source>Enable STEP Compound merge</source>
      <translation>Activer la fusion de combiné STEP</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="242"/>
      <source>Select this to use App::LinkGroup as group container, or else use App::Part.</source>
      <translation>Sélectionnez cette option pour utiliser App::LinkGroup comme conteneur de groupe, ou bien utilisez App::Part.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="245"/>
      <source>Use LinkGroup</source>
      <translation>Utiliser le groupe de liens</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="258"/>
      <source>Select this to not import any invisible objects.</source>
      <translation>Sélectionnez cette option pour ne pas importer d'objets invisibles.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="261"/>
      <source>Import invisible objects</source>
      <translation>Importer les objets invisibles</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="274"/>
      <source>Reduce number of objects using Link array</source>
      <translation>Réduire le nombre d'objets en utilisant le tableau de liaisons</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="277"/>
      <source>Reduce number of objects</source>
      <translation>Réduire le nombre d'objets</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="290"/>
      <source>Expand compound shape with multiple solids</source>
      <translation>Développer la forme composée avec plusieurs solides</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="293"/>
      <source>Expand compound shape</source>
      <translation>Agrandir la forme composée</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="306"/>
      <location filename="../../DlgImportExportStep.ui" line="309"/>
      <source>Show progress bar when importing</source>
      <translation>Afficher la barre de progression lors de l'importation</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="322"/>
      <source>Do not use instance name. Useful for some legacy STEP file with non-meaningful auto generated instance names.</source>
      <translation>N'utilisez pas le nom de l'instance. Utilisé pour d'anciens fichiers STEP nommés avec des noms d'instance auto-générés non significatifs.</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="325"/>
      <source>Ignore instance names</source>
      <translation>Ignorer les noms d'instance</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="340"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="360"/>
      <source>Single document</source>
      <translation>Document unique</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="365"/>
      <source>Assembly per document</source>
      <translation>Assemblage par document</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="370"/>
      <source>Assembly per document in sub-directory</source>
      <translation>Assemblage par document dans le sous-répertoire</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="375"/>
      <source>Object per document</source>
      <translation>Objet par document</translation>
    </message>
    <message>
      <location filename="../../DlgImportExportStep.ui" line="380"/>
      <source>Object per document in sub-directory</source>
      <translation>Objet par document dans le sous-répertoire</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.cpp" line="207"/>
      <source>This parameter indicates whether parametric curves (curves in parametric space of surface)
should be written into the STEP file. This parameter can be set to off in order to minimize
the size of the resulting STEP file.</source>
      <translation>Ce paramètre indique si les courbes paramétriques (courbes dans l'espace paramétrique de surface)
doivent être écrites dans le fichier STEP. Ce paramètre peut être désactivé afin de minimiser
la taille du fichier STEP résultant.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartBox</name>
    <message>
      <location filename="../../DlgPartBox.ui" line="14"/>
      <source>Box definition</source>
      <translation>Définition de boîte</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="20"/>
      <source>Position:</source>
      <translation>Position :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="78"/>
      <source>Direction:</source>
      <translation>Direction :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="85"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="92"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="99"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="109"/>
      <source>Size:</source>
      <translation>Taille :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="160"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="167"/>
      <source>Width:</source>
      <translation>Largeur :</translation>
    </message>
    <message>
      <location filename="../../DlgPartBox.ui" line="174"/>
      <source>Length:</source>
      <translation>Longueur :</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartCylinder</name>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="14"/>
      <source>Cylinder definition</source>
      <translation>Définition du cylindre</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="20"/>
      <source>Position:</source>
      <translation>Position :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="39"/>
      <source>Direction:</source>
      <translation>Direction :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="46"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="53"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="60"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="109"/>
      <source>Parameter</source>
      <translation>Paramètre</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="121"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../DlgPartCylinder.ui" line="128"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartImportIges</name>
    <message>
      <location filename="../../DlgPartImportIges.ui" line="14"/>
      <source>IGES input file</source>
      <translation>Fichier d'entrée IGES</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportIges.ui" line="20"/>
      <source>File Name</source>
      <translation>Nom du fichier</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportIges.ui" line="54"/>
      <source>...</source>
      <translation>...</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartImportIgesImp</name>
    <message>
      <location filename="../../DlgPartImportIgesImp.cpp" line="73"/>
      <source>IGES</source>
      <translation>IGES</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportIgesImp.cpp" line="74"/>
      <source>All Files</source>
      <translation>Tous les fichiers</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartImportStep</name>
    <message>
      <location filename="../../DlgPartImportStep.ui" line="14"/>
      <source>Step input file</source>
      <translation>Fichier d'entrée STEP</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportStep.ui" line="20"/>
      <source>File Name</source>
      <translation>Nom du fichier</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportStep.ui" line="54"/>
      <source>...</source>
      <translation>...</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPartImportStepImp</name>
    <message>
      <location filename="../../DlgPartImportStepImp.cpp" line="72"/>
      <source>STEP</source>
      <translation>STEP</translation>
    </message>
    <message>
      <location filename="../../DlgPartImportStepImp.cpp" line="73"/>
      <source>All Files</source>
      <translation>Tous les fichiers</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgPrimitives</name>
    <message>
      <location filename="../../DlgPrimitives.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Primitives géométriques</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="33"/>
      <location filename="../../DlgPrimitives.cpp" line="726"/>
      <source>Plane</source>
      <translation>Plan</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="42"/>
      <location filename="../../DlgPrimitives.cpp" line="743"/>
      <source>Box</source>
      <translation>Boîte</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="51"/>
      <location filename="../../DlgPrimitives.cpp" line="764"/>
      <source>Cylinder</source>
      <translation>Cylindre</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="60"/>
      <location filename="../../DlgPrimitives.cpp" line="783"/>
      <source>Cone</source>
      <translation>Cône</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="69"/>
      <location filename="../../DlgPrimitives.cpp" line="802"/>
      <source>Sphere</source>
      <translation>Sphère</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="78"/>
      <location filename="../../DlgPrimitives.cpp" line="825"/>
      <source>Ellipsoid</source>
      <translation>Ellipsoïde</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="87"/>
      <location filename="../../DlgPrimitives.cpp" line="846"/>
      <source>Torus</source>
      <translation>Tore</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="96"/>
      <location filename="../../DlgPrimitives.cpp" line="867"/>
      <source>Prism</source>
      <translation>Prisme</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="105"/>
      <location filename="../../DlgPrimitives.cpp" line="898"/>
      <source>Wedge</source>
      <translation>Prisme</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="114"/>
      <location filename="../../DlgPrimitives.cpp" line="920"/>
      <source>Helix</source>
      <translation>Hélice</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="123"/>
      <location filename="../../DlgPrimitives.cpp" line="937"/>
      <source>Spiral</source>
      <translation>Spirale</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="132"/>
      <location filename="../../DlgPrimitives.cpp" line="954"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="141"/>
      <location filename="../../DlgPrimitives.cpp" line="973"/>
      <source>Ellipse</source>
      <translation>Ellipse</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="150"/>
      <source>Point</source>
      <translation>Point</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="159"/>
      <location filename="../../DlgPrimitives.cpp" line="1013"/>
      <source>Line</source>
      <translation>Ligne</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="168"/>
      <location filename="../../DlgPrimitives.cpp" line="1028"/>
      <source>Regular polygon</source>
      <translation>Polygone régulier</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="180"/>
      <source>Parameter</source>
      <translation>Paramètre</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="253"/>
      <location filename="../../DlgPrimitives.ui" line="387"/>
      <source>Width:</source>
      <translation>Largeur :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="260"/>
      <location filename="../../DlgPrimitives.ui" line="380"/>
      <source>Length:</source>
      <translation>Longueur :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="373"/>
      <location filename="../../DlgPrimitives.ui" line="520"/>
      <location filename="../../DlgPrimitives.ui" line="731"/>
      <location filename="../../DlgPrimitives.ui" line="1419"/>
      <location filename="../../DlgPrimitives.ui" line="1752"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="448"/>
      <source>Rotation angle:</source>
      <translation>Angle de rotation :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="513"/>
      <location filename="../../DlgPrimitives.ui" line="917"/>
      <location filename="../../DlgPrimitives.ui" line="1738"/>
      <location filename="../../DlgPrimitives.ui" line="1887"/>
      <location filename="../../DlgPrimitives.ui" line="1970"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="553"/>
      <location filename="../../DlgPrimitives.ui" line="1439"/>
      <source>Angle in first direction:</source>
      <translation>Angle dans la première direction :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="560"/>
      <location filename="../../DlgPrimitives.ui" line="1446"/>
      <source>Angle in first direction</source>
      <translation>Angle dans la première direction</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="579"/>
      <location filename="../../DlgPrimitives.ui" line="1465"/>
      <source>Angle in second direction:</source>
      <translation>Angle dans la seconde direction :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="586"/>
      <location filename="../../DlgPrimitives.ui" line="1472"/>
      <source>Angle in second direction</source>
      <translation>Angle dans la seconde direction</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="640"/>
      <location filename="../../DlgPrimitives.ui" line="1759"/>
      <source>Angle:</source>
      <translation>Angle :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="738"/>
      <location filename="../../DlgPrimitives.ui" line="998"/>
      <location filename="../../DlgPrimitives.ui" line="1318"/>
      <source>Radius 1:</source>
      <translation>Rayon 1 :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="745"/>
      <location filename="../../DlgPrimitives.ui" line="1005"/>
      <location filename="../../DlgPrimitives.ui" line="1311"/>
      <source>Radius 2:</source>
      <translation>Rayon 2 :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="829"/>
      <location filename="../../DlgPrimitives.ui" line="1077"/>
      <source>U parameter:</source>
      <translation>Paramètre U :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="836"/>
      <source>V parameters:</source>
      <translation>Paramètres V :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1012"/>
      <source>Radius 3:</source>
      <translation>Rayon 3 :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1100"/>
      <location filename="../../DlgPrimitives.ui" line="1217"/>
      <source>V parameter:</source>
      <translation>Paramètre V :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1194"/>
      <source>U Parameter:</source>
      <translation>Paramètre U :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1389"/>
      <location filename="../../DlgPrimitives.ui" line="2418"/>
      <source>Polygon:</source>
      <translation>Polygone :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1412"/>
      <location filename="../../DlgPrimitives.ui" line="2441"/>
      <source>Circumradius:</source>
      <translation>Rayon circonscrit :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1518"/>
      <source>X min/max:</source>
      <translation>X min/max:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1525"/>
      <source>Y min/max:</source>
      <translation>Y min/max:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1532"/>
      <source>Z min/max:</source>
      <translation>Z min/max:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1539"/>
      <source>X2 min/max:</source>
      <translation>X2 min/max:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1546"/>
      <source>Z2 min/max:</source>
      <translation>Z2 min/max:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1745"/>
      <source>Pitch:</source>
      <translation>Axe de tangage :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1766"/>
      <source>Coordinate system:</source>
      <translation>Système de coordonnées:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1774"/>
      <source>Right-handed</source>
      <translation>Main droite</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1779"/>
      <source>Left-handed</source>
      <translation>Main gauche</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1894"/>
      <source>Growth:</source>
      <translation>Croissance :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1901"/>
      <source>Number of rotations:</source>
      <translation>Nombre de rotations :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1977"/>
      <location filename="../../DlgPrimitives.ui" line="2086"/>
      <source>Angle 1:</source>
      <translation>Angle 1:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="1984"/>
      <location filename="../../DlgPrimitives.ui" line="2093"/>
      <source>Angle 2:</source>
      <translation>Angle 2:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2044"/>
      <source>From three points</source>
      <translation>A partir de trois points</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2072"/>
      <source>Major radius:</source>
      <translation>Rayon majeur:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2079"/>
      <source>Minor radius:</source>
      <translation>Rayon mineur:</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2170"/>
      <location filename="../../DlgPrimitives.ui" line="2268"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2180"/>
      <location filename="../../DlgPrimitives.ui" line="2301"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2190"/>
      <location filename="../../DlgPrimitives.ui" line="2334"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2251"/>
      <source>Start point</source>
      <translation>Point de départ</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.ui" line="2258"/>
      <source>End point</source>
      <translation>Point final</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.cpp" line="990"/>
      <source>Vertex</source>
      <translation>Sommet</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.cpp" line="1037"/>
      <location filename="../../DlgPrimitives.cpp" line="1107"/>
      <location filename="../../DlgPrimitives.cpp" line="1115"/>
      <source>Create %1</source>
      <translation>Créer %1</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.cpp" line="1038"/>
      <source>No active document</source>
      <translation>Aucun document actif</translation>
    </message>
    <message>
      <location filename="../../DlgPrimitives.cpp" line="2036"/>
      <source>&amp;Create</source>
      <translation>&amp;Créer</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgProjectionOnSurface</name>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="14"/>
      <source>Projection on surface</source>
      <translation>Projection sur surface</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="20"/>
      <source>Select projection surface</source>
      <translation>Sélectionner la surface de projection</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="31"/>
      <source>Add face</source>
      <translation>Ajouter une face</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="38"/>
      <source>Add wire</source>
      <translation>Ajouter un fil</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="45"/>
      <source>Add edge</source>
      <translation>Ajouter une arête</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="56"/>
      <source>Show all</source>
      <translation>Tout afficher</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="66"/>
      <source>Show faces</source>
      <translation>Afficher les faces</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="76"/>
      <source>Show Edges</source>
      <translation>Afficher les arêtes</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="92"/>
      <source>Extrude height</source>
      <translation>Hauteur de l’extrusion</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="116"/>
      <source>Solid depth</source>
      <translation>Profondeur du solide</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="141"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="147"/>
      <source>Get current camera direction</source>
      <translation>Obtenir la direction actuelle de la caméra</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="156"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="186"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.ui" line="213"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.cpp" line="134"/>
      <source>Projection Object</source>
      <translation>Objet de projection</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.cpp" line="162"/>
      <source>Have no active document!!!</source>
      <translation>N'a pas de document actif!!!</translation>
    </message>
    <message>
      <location filename="../../DlgProjectionOnSurface.cpp" line="169"/>
      <source>Can not create a projection object!!!</source>
      <translation>Impossible de créer un objet de projection !!!</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgRevolution</name>
    <message>
      <location filename="../../DlgRevolution.ui" line="20"/>
      <source>Revolve</source>
      <translation>Révolution</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="32"/>
      <source>If checked, revolving wires will produce solids. If not, revolving a wire yields a shell.</source>
      <translation>Si coché, la rotation des fils produira des solides. Sinon, la rotation d'un fil produit une coque.</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="35"/>
      <source>Create Solid</source>
      <translation>Créer solide</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="52"/>
      <source>Shape</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="71"/>
      <source>Angle:</source>
      <translation>Angle :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="111"/>
      <source>Revolution axis</source>
      <translation>Axe de révolution</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="119"/>
      <source>Center X:</source>
      <translation>Centre X :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="139"/>
      <source>Center Y:</source>
      <translation>Centre Y :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="159"/>
      <source>Center Z:</source>
      <translation>Centre Z :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="201"/>
      <location filename="../../DlgRevolution.ui" line="242"/>
      <source>Click to set this as axis</source>
      <translation>Cliquez pour définir ceci comme axe</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="204"/>
      <source>Dir. X:</source>
      <translation>Dir. X :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="245"/>
      <source>Dir. Y:</source>
      <translation>Dir. Y :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="283"/>
      <source>Dir. Z:</source>
      <translation>Dir. Z :</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="305"/>
      <location filename="../../DlgRevolution.cpp" line="447"/>
      <source>Select reference</source>
      <translation>Sélectionner une référence</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="325"/>
      <source>If checked, revolution will extend forwards and backwards by half the angle.</source>
      <translation>Si coché, la révolution s’étendra de part et d'autre de la moitié de l’angle.</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.ui" line="328"/>
      <source>Symmetric angle</source>
      <translation>Angle symétrique</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="172"/>
      <source>Object not found: %1</source>
      <translation>Objet introuvable : %1</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="253"/>
      <source>Select a shape for revolution, first.</source>
      <translation>Sélectionnez d'abord une forme à révolutionner.</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="269"/>
      <location filename="../../DlgRevolution.cpp" line="274"/>
      <location filename="../../DlgRevolution.cpp" line="279"/>
      <source>Revolution axis link is invalid.

%1</source>
      <translation>Le lien à la direction d'extrusion est invalide. 

%1</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="288"/>
      <source>Revolution axis direction is zero-length. It must be non-zero.</source>
      <translation>La direction d’axe de révolution est de longueur nulle. Elle ne doit pas être nulle.</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="298"/>
      <source>Revolution angle span is zero. It must be non-zero.</source>
      <translation>L'angle de révolution est nul. Il doit être différent de zéro.</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="427"/>
      <location filename="../../DlgRevolution.cpp" line="431"/>
      <source>Creating Revolve failed.

%1</source>
      <translation>La création de la révolution a échoué.

%1</translation>
    </message>
    <message>
      <location filename="../../DlgRevolution.cpp" line="443"/>
      <source>Selecting... (line or arc)</source>
      <translation>Sélection de... (ligne ou arc)</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgSettings3DViewPart</name>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="14"/>
      <source>Shape view</source>
      <translation>Vue de la forme</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="39"/>
      <source>Tessellation</source>
      <translation>Tessellation</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="59"/>
      <source> %</source>
      <translation>%</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="87"/>
      <source>Defines the deviation of tessellation to the actual surface</source>
      <translation>Définit la déviation de tessellation à la surface réelle</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="90"/>
      <source>&lt;html&gt;&lt;head&gt;&lt;meta name="qrichtext" content="1" /&gt;&lt;/head&gt;&lt;body style=" white-space: pre-wrap; font-family:MS Shell Dlg 2; font-size:7.8pt; font-weight:400; font-style:normal; text-decoration:none;"&gt;&lt;p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"&gt;&lt;span style=" font-weight:600;"&gt;Tessellation&lt;/span&gt;&lt;/p&gt;&lt;p style="-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;"&gt;&lt;/p&gt;&lt;p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;"&gt;&lt;span style=" font-weight:400;"&gt;Defines the maximum deviation of the tessellated mesh to the surface. The smaller the value is the slower the render speed which results in increased detail/resolution.&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head&gt;&lt;meta name="qrichtext" content="1" /&gt;&lt;/head&gt;&lt;body style=" white-space: pre-wrap; font-family:MS Shell Dlg 2; font-size:7.8pt; font-weight:400; font-style:normal; text-decoration:none;"&gt;&lt;p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"&gt;&lt;span style=" font-weight:600;"&gt;Tessellation&lt;/span&gt;&lt;/p&gt;&lt;p style="-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;"&gt;&lt;/p&gt;&lt;p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-weight:600;"&gt;&lt;span style=" font-weight:400;"&gt;Définit la déviation maximale de la maille tesselée à la surface. Plus la valeur est lente la vitesse de rendu qui résulte en détail/résolution accrue.&lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="93"/>
      <source>Maximum deviation depending on the model bounding box</source>
      <translation>L'écart maximal suivant la  boîte englobant le modèle</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="100"/>
      <source>Maximum angular deflection</source>
      <translation>Déviation angulaire maximale</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPart.ui" line="107"/>
      <source> °</source>
      <translation> °</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPartImp.cpp" line="69"/>
      <source>Deviation</source>
      <translation>Déviation</translation>
    </message>
    <message>
      <location filename="../../DlgSettings3DViewPartImp.cpp" line="70"/>
      <source>Setting a too small deviation causes the tessellation to take longerand thus freezes or slows down the GUI.</source>
      <translation>Définir une déviation de tessellation trop faible peut ralentir ou geler l'interface.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgSettingsGeneral</name>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="14"/>
      <source>General</source>
      <translation>Général</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="20"/>
      <source>Model settings</source>
      <translation>Paramètres de modèle</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="26"/>
      <source>Automatically check model after boolean operation</source>
      <translation>Vérifier les modèles automatiquement après une opération booléenne</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="39"/>
      <source>Automatically refine model after boolean operation</source>
      <translation>Affiner les modèles automatiquement après une opération booléenne</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="52"/>
      <source>Automatically refine model after sketch-based operation</source>
      <translation>Affiner automatiquement le modèle après une opération d'esquisse</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="68"/>
      <source>Object naming</source>
      <translation>Nom d'objet</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsGeneral.ui" line="77"/>
      <source>Add name of base object</source>
      <translation>Ajouter un nom à l'objet de base</translation>
    </message>
  </context>
  <context>
    <name>PartGui::DlgSettingsObjectColor</name>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="14"/>
      <source>Shape appearance</source>
      <translation>Aspect de la forme</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="20"/>
      <source>Default Shape view properties</source>
      <translation>Propriétés de la vue forme par défaut</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="34"/>
      <source>Shape color</source>
      <translation>Couleur de la forme</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="41"/>
      <source>The default color for new shapes</source>
      <translation>La couleur par défaut des nouvelles formes</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="61"/>
      <source>Use random color instead</source>
      <translation>Utiliser une couleur aléatoire à la place</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="64"/>
      <source>Random</source>
      <translation>Aléatoire "Random"</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="83"/>
      <source>Line color</source>
      <translation>Couleur de ligne</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="90"/>
      <source>The default line color for new shapes</source>
      <translation>La couleur de ligne par défaut des nouvelles formes</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="116"/>
      <source>Line width</source>
      <translation>Largeur de ligne</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="123"/>
      <source>The default line thickness for new shapes</source>
      <translation>L'épaisseur de ligne par défaut des nouvelles formes</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="126"/>
      <location filename="../../DlgSettingsObjectColor.ui" line="194"/>
      <source>px</source>
      <translation>px</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="151"/>
      <source>Vertex color</source>
      <translation>Couleur des points</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="158"/>
      <source>The default color for new vertices</source>
      <translation>La couleur par défaut pour les nouveaux sommets</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="184"/>
      <source>Vertex size</source>
      <translation>Taille du sommet</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="191"/>
      <source>The default size for new vertices</source>
      <translation>La taille par défaut pour les nouveaux sommets</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="219"/>
      <source>Bounding box color</source>
      <translation>Couleur de la boîte englobante</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="226"/>
      <source>The color of bounding boxes in the 3D view</source>
      <translation>Couleur de la boîte englobante dans la vue 3D</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="252"/>
      <source>Bounding box font size</source>
      <translation>Taille de police de la boîte englobante</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="259"/>
      <source>The font size of bounding boxes in the 3D view</source>
      <translation>La taille de police de la boite englobantes dans la vue 3D</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="293"/>
      <source>Bottom side of surface will be rendered the same way than top.
If not checked, it depends on the option "Backlight color"
(preferences section Display -&gt; 3D View); either the backlight color
will be used or black.</source>
      <translation>Un rendu du côté inférieur de la surface sera généré de la même manière que le côté supérieur.
Si non cochée, cela dépend de l'option "Couleur de rétroéclairage"
(section Préférences Affichage -&gt; Vue 3D). Soit la couleur de rétroéclairage
sera utilisée soit du noir.</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="299"/>
      <source>Two-side rendering</source>
      <translation>Rendu biface</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="333"/>
      <source>Default Annotation color</source>
      <translation>Couleur d'annotation par défaut</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="347"/>
      <source>Text color</source>
      <translation>Couleur du texte</translation>
    </message>
    <message>
      <location filename="../../DlgSettingsObjectColor.ui" line="354"/>
      <source>Text color for document annotations</source>
      <translation>Couleur du texte pour les annotations de document</translation>
    </message>
  </context>
  <context>
    <name>PartGui::Location</name>
    <message>
      <location filename="../../Location.ui" line="14"/>
      <source>Location</source>
      <translation>Emplacement</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="29"/>
      <source>Position</source>
      <translation>Position</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="37"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="54"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="71"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="90"/>
      <source>3D view</source>
      <translation>3D</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="106"/>
      <source>Use custom vector for pad direction otherwise
the sketch plane's normal vector will be used</source>
      <translation>Utiliser un vecteur personnalisé pour la direction de la protrusion sinon
le vecteur normal du plan de l'esquisse sera utilisé</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="110"/>
      <source>Rotation axis</source>
      <translation>Axe de rotation</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="118"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="125"/>
      <source>x-component of direction vector</source>
      <translation>composante x du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="147"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="154"/>
      <source>y-component of direction vector</source>
      <translation>composante y du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="176"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="183"/>
      <source>z-component of direction vector</source>
      <translation>composante z du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../Location.ui" line="208"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
  </context>
  <context>
    <name>PartGui::LoftWidget</name>
    <message>
      <location filename="../../TaskLoft.cpp" line="80"/>
      <source>Available profiles</source>
      <translation>Profils disponibles</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="81"/>
      <source>Selected profiles</source>
      <translation>Profils sélectionnés</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="180"/>
      <source>Too few elements</source>
      <translation>Trop peu d'éléments</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="180"/>
      <source>At least two vertices, edges, wires or faces are required.</source>
      <translation>Au moins deux sommets, arêtes, fils ou faces sont requis.</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="214"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="243"/>
      <source>Vertex/Edge/Wire/Face</source>
      <translation>Sommet/Arête/Fil/Face</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.cpp" line="244"/>
      <source>Loft</source>
      <translation>Lissage</translation>
    </message>
  </context>
  <context>
    <name>PartGui::Mirroring</name>
    <message>
      <location filename="../../Mirroring.ui" line="14"/>
      <source>Mirroring</source>
      <translation>Mise en miroir</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="33"/>
      <source>Shapes</source>
      <translation>Formes</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="41"/>
      <source>Mirror plane:</source>
      <translation>Plan miroir :</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="49"/>
      <source>XY plane</source>
      <translation>Plan XY</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="54"/>
      <source>XZ plane</source>
      <translation>Plan XZ</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="59"/>
      <source>YZ plane</source>
      <translation>Plan YZ</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="67"/>
      <source>Base point</source>
      <translation>Point de base</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="73"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="96"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../Mirroring.ui" line="119"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../Mirroring.cpp" line="122"/>
      <source>Select a shape for mirroring, first.</source>
      <translation>Sélectionner d'abord une forme à mettre en miroir.</translation>
    </message>
    <message>
      <location filename="../../Mirroring.cpp" line="129"/>
      <source>No such document '%1'.</source>
      <translation>Aucun document '%1'.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::OffsetWidget</name>
    <message>
      <location filename="../../TaskOffset.cpp" line="198"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>PartGui::ResultModel</name>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="348"/>
      <source>Name</source>
      <translation>Nom</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="350"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="352"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartGui::ShapeBuilderWidget</name>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="208"/>
      <location filename="../../TaskShapeBuilder.cpp" line="227"/>
      <location filename="../../TaskShapeBuilder.cpp" line="255"/>
      <location filename="../../TaskShapeBuilder.cpp" line="296"/>
      <location filename="../../TaskShapeBuilder.cpp" line="348"/>
      <location filename="../../TaskShapeBuilder.cpp" line="400"/>
      <location filename="../../TaskShapeBuilder.cpp" line="463"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="208"/>
      <location filename="../../TaskShapeBuilder.cpp" line="227"/>
      <source>Select two vertices</source>
      <translation>Sélectionnez deux sommets</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="255"/>
      <location filename="../../TaskShapeBuilder.cpp" line="348"/>
      <source>Select one or more edges</source>
      <translation>Sélectionnez une ou plusieurs arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="296"/>
      <source>Select three or more vertices</source>
      <translation>Sélectionnez au moins trois sommets</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="400"/>
      <source>Select two or more faces</source>
      <translation>Sélectionnez au moins deux faces</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="463"/>
      <source>Select only one part object</source>
      <translation>Sélectionnez seulement un objet pièce</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="515"/>
      <source>Select two vertices to create an edge</source>
      <translation>Sélectionner deux sommets pour créer une arête</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="522"/>
      <source>Select adjacent edges</source>
      <translation>Sélectionnez des bords adjacents</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="529"/>
      <source>Select a list of vertices</source>
      <translation>Sélectionnez une liste de sommets</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="536"/>
      <source>Select a closed set of edges</source>
      <translation>Sélectionner un ensemble fermé d'arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="543"/>
      <source>Select adjacent faces</source>
      <translation>Sélectionner les faces adjacentes</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.cpp" line="550"/>
      <source>All shape types can be selected</source>
      <translation>Tous les types de forme peuvent être sélectionnés</translation>
    </message>
  </context>
  <context>
    <name>PartGui::SweepWidget</name>
    <message>
      <location filename="../../TaskSweep.cpp" line="134"/>
      <source>Available profiles</source>
      <translation>Profils disponibles</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="135"/>
      <source>Selected profiles</source>
      <translation>Profils sélectionnés</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="281"/>
      <location filename="../../TaskSweep.cpp" line="411"/>
      <location filename="../../TaskSweep.cpp" line="419"/>
      <source>Sweep path</source>
      <translation>Chemin de balayage</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="281"/>
      <source>Select one or more connected edges you want to sweep along.</source>
      <translation>Sélectionnez une ou plusieurs arêtes connectées le long desquelles passe le balayage.</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="309"/>
      <source>Too few elements</source>
      <translation>Trop peu d'éléments</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="309"/>
      <source>At least one edge or wire is required.</source>
      <translation>Au moins une arête ou un fil est requis.</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="316"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="316"/>
      <source>'%1' cannot be used as profile and path.</source>
      <translation>'%1' ne peut pas servir de profil et de chemin.</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="354"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="387"/>
      <source>Done</source>
      <translation>Fait</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="389"/>
      <source>Select one or more connected edges in the 3d view and press 'Done'</source>
      <translation>Sélectionnez une ou plusieurs arêtes connectées dans la vue 3D et appuyez sur « Fait »</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="411"/>
      <location filename="../../TaskSweep.cpp" line="419"/>
      <source>The selected sweep path is invalid.</source>
      <translation>Le chemin de balayage sélectionné n'est pas valide.</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="431"/>
      <source>Vertex/Wire</source>
      <translation>Sommet/Fil</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="432"/>
      <source>Sweep</source>
      <translation>Balayage</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskAttacher</name>
    <message>
      <location filename="../../TaskAttacher.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="20"/>
      <source>Selection accepted</source>
      <translation>Sélection acceptée</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="35"/>
      <source>Reference 1</source>
      <translation>Référence 1</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="52"/>
      <source>Reference 2</source>
      <translation>Référence 2</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="69"/>
      <source>Reference 3</source>
      <translation>Référence 3</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="86"/>
      <source>Reference 4</source>
      <translation>Référence 4</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="101"/>
      <source>Attachment mode:</source>
      <translation>Mode d'accrochage :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="124"/>
      <location filename="../../TaskAttacher.cpp" line="335"/>
      <source>Attachment Offset (in local coordinates):</source>
      <translation>Décalage de l'accrochage (en coordonnées locales) :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="136"/>
      <source>In x-direction:</source>
      <translation>Dans la direction x :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="152"/>
      <source>In y-direction:</source>
      <translation>Dans la direction y :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="171"/>
      <location filename="../../TaskAttacher.ui" line="207"/>
      <location filename="../../TaskAttacher.ui" line="269"/>
      <source>Note: The placement is expressed in local coordinate system
of object being attached.</source>
      <translation>Note: Le placement est exprimé dans le système de coordonnées local
de l'objet en cours d'attachement.</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="188"/>
      <source>In z-direction:</source>
      <translation>Dans la direction z :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="224"/>
      <source>Around x-axis:</source>
      <translation>Autour de l'axe x :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="237"/>
      <source>Around y-axis:</source>
      <translation>Autour de l'axe y :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="250"/>
      <source>Around z-axis:</source>
      <translation>Autour de l'axe z :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="286"/>
      <source>Rotation around the x-axis
Note: The placement is expressed in local coordinate system
of object being attached.</source>
      <translation>Rotation autour de l'axe x
Note : Le placement est exprimé dans le système de coordonnées local de l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="313"/>
      <source>Rotation around the y-axis
Note: The placement is expressed in local coordinate system
of object being attached.</source>
      <translation>Rotation autour de l'axe x
Note : Le placement est exprimé dans le système de coordonnées local de l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="340"/>
      <source>Rotation around the z-axis
Note: The placement is expressed in local coordinate system
of object being attached.</source>
      <translation>Rotation autour de l'axe z
Note : Le placement est exprimé dans le système de coordonnées local de l'objet auquel il est attaché.</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="364"/>
      <source>Flip side of attachment and offset</source>
      <translation>Retourner le côté de l'accrochage et décalage</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.ui" line="367"/>
      <source>Flip sides</source>
      <translation>Retourner</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="318"/>
      <source>OCC error: %1</source>
      <translation>Erreur OCC : %1</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="320"/>
      <source>unknown error</source>
      <translation>erreur inconnue</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="323"/>
      <source>Attachment mode failed: %1</source>
      <translation>Le mode d'accrochage a échoué : %1</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="327"/>
      <source>Not attached</source>
      <translation>Non accroché</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="331"/>
      <source>Attached with mode %1</source>
      <translation>Accroché avec mode %1</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="335"/>
      <source>Attachment Offset (inactive - not attached):</source>
      <translation>Compensation d'accrochage (inactif - non accroché) :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="627"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="632"/>
      <source>Edge</source>
      <translation>Arête</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="637"/>
      <source>Vertex</source>
      <translation>Sommet</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="699"/>
      <source>Selecting...</source>
      <translation>Sélection de...</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="703"/>
      <source>Reference%1</source>
      <translation>Référence%1</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="750"/>
      <source>Not editable because rotation of AttachmentOffset is bound by expressions.</source>
      <translation>Non modifiable parce que la rotation de AttachmentOffset est définie par des expressions.</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="816"/>
      <source>Reference combinations:</source>
      <translation>Combinaisons de référence :</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="833"/>
      <source>%1 (add %2)</source>
      <translation>%1 (ajouter %2)</translation>
    </message>
    <message>
      <location filename="../../TaskAttacher.cpp" line="838"/>
      <source>%1 (add more references)</source>
      <translation>%1 (ajouter plus de références)</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskCheckGeometryDialog</name>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1068"/>
      <source>Shape Content</source>
      <translation>Contenu de la forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1076"/>
      <location filename="../../TaskCheckGeometry.cpp" line="1270"/>
      <source>Settings</source>
      <translation>Réglages</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1080"/>
      <source>Skip settings page</source>
      <translation>Passer la page de paramètres</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1081"/>
      <source>Skip this settings page and run the geometry check automatically.
Default: false</source>
      <translation>Ignorer cette page de paramètres et exécuter la vérification de la géométrie automatiquement.
Valeur par défaut : false</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1090"/>
      <source>Run BOP check</source>
      <translation>Éxecuter la vérification BOP</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1091"/>
      <source>Extra boolean operations check that can sometimes find errors that
the standard BRep geometry check misses. These errors do not always 
mean the checked object is unusable.  Default: false</source>
      <translation>Les opérations booléennes supplémentaires peuvent parfois trouver des erreurs que
la vérification de géométrie standard BRep rate. Ces erreurs ne signifient pas toujours 
que l'objet vérifié est inutilisable. Valeur par dar défaut : false</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1101"/>
      <source>Single-threaded</source>
      <translation>Fil unique</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1102"/>
      <source>Run the geometry check in a single thread.  This is slower,
but more stable.  Default: false</source>
      <translation>Exécuter la vérification de la géométrie dans un seul thread. Ceci est plus lent,
mais plus stable. Valeur par défaut : false</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1111"/>
      <source>Log errors</source>
      <translation>Journal des erreurs</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1112"/>
      <source>Log errors to report view.  Default: true</source>
      <translation>Journal des erreurs vers la vue rapport. Par défaut: vrai</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1119"/>
      <source>Expand shape content</source>
      <translation>Développer le contenu de la forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1120"/>
      <source>Expand shape content.  Changes will take effect next time you use 
the check geometry tool.  Default: false</source>
      <translation>Développer le contenu de la forme. Les modifications prendront effet la prochaine fois que vous utiliserez 
l'outil de vérification de la géométrie. valeur par défaut : false</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1129"/>
      <source>Advanced shape content</source>
      <translation>Contenu avancé de la forme</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1130"/>
      <source>Show advanced shape content.  Changes will take effect next time you use 
the check geometry tool.  Default: false</source>
      <translation>Afficher le contenu avancé de la forme. Les modifications prendront effet la prochaine fois que vous utiliserez 
l'outil de vérification de la géométrie. Valeur par défaut : false</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1138"/>
      <source>
Individual BOP Checks:</source>
      <translation>
Vérifications individuelles du BOP :</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1141"/>
      <source>  Bad type</source>
      <translation>  Mauvais type</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1142"/>
      <source>Check for bad argument types.  Default: true</source>
      <translation>Vérifier les mauvais types d'arguments. Valeur par défaut : true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1149"/>
      <source>  Self-intersect</source>
      <translation>  Auto-intersection</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1150"/>
      <source>Check for self-intersections.  Default: true</source>
      <translation>Rechercher la présence d'auto-intersections. Valeur par défaut : true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1157"/>
      <source>  Too small edge</source>
      <translation>  Arête trop petite</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1158"/>
      <source>Check for edges that are too small.  Default: true</source>
      <translation>Rechercher la présence d'arêtes trop petites. Valeur par défaut: true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1165"/>
      <source>  Nonrecoverable face</source>
      <translation>  Face non-récupérable</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1166"/>
      <source>Check for nonrecoverable faces.  Default: true</source>
      <translation>Rechercher la présence de faces non-récupérables. Valeur par défaut : true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1173"/>
      <source>  Continuity</source>
      <translation>  Continuité</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1174"/>
      <source>Check for continuity.  Default: true</source>
      <translation>Vérifier la continuité. Par défaut: vrai</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1181"/>
      <source>  Incompatibility of face</source>
      <translation>  Incompatibilité de la face</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1182"/>
      <source>Check for incompatible faces.  Default: true</source>
      <translation>Vérifier s'il y a des faces incompatibles. Par défaut: vrai</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1189"/>
      <source>  Incompatibility of vertex</source>
      <translation>  Incompatibilité de sommets</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1190"/>
      <source>Check for incompatible vertices.  Default: true</source>
      <translation>Vérifier s'il y a des faces incompatibles. Valeur par défaut: vrai</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1197"/>
      <source>  Incompatibility of edge</source>
      <translation>  Incompatibilité de l'arête</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1198"/>
      <source>Check for incompatible edges.  Default: true</source>
      <translation>Rechercher la présende d'arêtes incompatibles. Valeur par défaut : true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1205"/>
      <source>  Invalid curve on surface</source>
      <translation>  Courbe de surface invalide</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1206"/>
      <source>Check for invalid curves on surfaces.  Default: true</source>
      <translation>Rechercher la présence de courbes de surfaces invalides. Valeur par défaut : true</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1269"/>
      <source>Run check</source>
      <translation>Exécuter la vérification</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="1275"/>
      <source>Results</source>
      <translation>Résultats</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskCheckGeometryResults</name>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="383"/>
      <source>Check Geometry Results</source>
      <translation>Vérifier les résultats de la géométrie</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="402"/>
      <source>Check is running...</source>
      <translation>Vérification en cours...</translation>
    </message>
    <message>
      <location filename="../../TaskCheckGeometry.cpp" line="426"/>
      <location filename="../../TaskCheckGeometry.cpp" line="432"/>
      <source>Check geometry</source>
      <translation>Vérifier la géométrie</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskDlgAttacher</name>
    <message>
      <location filename="../../TaskAttacher.cpp" line="1104"/>
      <source>Datum dialog: Input error</source>
      <translation>Boîte de dialogue de dimension : erreur d’entrée</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskFaceColors</name>
    <message>
      <location filename="../../TaskFaceColors.ui" line="14"/>
      <source>Set color per face</source>
      <translation>Définir la couleur des faces</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="20"/>
      <source>Click on the faces in the 3D view to select them</source>
      <translation>Cliquez sur les faces dans la vue 3D pour les sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="39"/>
      <source>Faces:</source>
      <translation>Faces :</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="87"/>
      <source>Resets color for all faces of the part</source>
      <translation>Réinitialiser la couleur pour toutes les faces de la pièce</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="90"/>
      <source>Set to default</source>
      <translation>Définir par défaut</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="97"/>
      <source>When checked, the you can select multiple faces
by dragging a selection rectangle in the 3D view</source>
      <translation>Quand coché, vous pouvez sélectionner plusieurs faces
en traçant un rectangle de sélection dans la vue 3D</translation>
    </message>
    <message>
      <location filename="../../TaskFaceColors.ui" line="101"/>
      <source>Box selection</source>
      <translation>Sélection par boîte</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskLoft</name>
    <message>
      <location filename="../../TaskLoft.ui" line="14"/>
      <source>Loft</source>
      <translation>Lissage</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.ui" line="23"/>
      <source>Create solid</source>
      <translation>Créer le solide</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.ui" line="30"/>
      <source>Ruled surface</source>
      <translation>Surface réglée</translation>
    </message>
    <message>
      <location filename="../../TaskLoft.ui" line="50"/>
      <source>Closed</source>
      <translation>Fermé</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskOffset</name>
    <message>
      <location filename="../../TaskOffset.ui" line="14"/>
      <location filename="../../TaskOffset.ui" line="20"/>
      <source>Offset</source>
      <translation>Décalage</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="34"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="42"/>
      <source>Skin</source>
      <translation>Couche</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="47"/>
      <source>Pipe</source>
      <translation>Tuyau</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="52"/>
      <source>RectoVerso</source>
      <translation>Recto/verso</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="60"/>
      <source>Join type</source>
      <translation>Type de raccordement</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="68"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="73"/>
      <source>Tangent</source>
      <translation>Tangent</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="78"/>
      <location filename="../../TaskOffset.ui" line="86"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="93"/>
      <source>Self-intersection</source>
      <translation>Auto-intersection</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="100"/>
      <source>Fill offset</source>
      <translation>Remplir le décalage</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="114"/>
      <source>Faces</source>
      <translation>Faces</translation>
    </message>
    <message>
      <location filename="../../TaskOffset.ui" line="144"/>
      <source>Update view</source>
      <translation>Réactualiser la vue</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskShapeBuilder</name>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="14"/>
      <location filename="../../TaskShapeBuilder.ui" line="20"/>
      <source>Create shape</source>
      <translation>Créer la forme</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="26"/>
      <source>Face from vertices</source>
      <translation>Face depuis des sommets</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="33"/>
      <source>Shell from faces</source>
      <translation>Coque à partir de faces</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="40"/>
      <source>Edge from vertices</source>
      <translation>Arête à partir de sommets</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="47"/>
      <source>Face from edges</source>
      <translation>Face à partir d'arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="54"/>
      <source>Solid from shell</source>
      <translation>Solide depuis une coque</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="68"/>
      <source>Planar</source>
      <translation>Planaire</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="75"/>
      <source>Refine shape</source>
      <translation>Affiner la forme</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="85"/>
      <source>All faces</source>
      <translation>Toutes les faces</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="107"/>
      <source>Create</source>
      <translation>Créer</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBuilder.ui" line="116"/>
      <source>Wire from edges</source>
      <translation>Fil à partir d'arêtes</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskSweep</name>
    <message>
      <location filename="../../TaskSweep.ui" line="14"/>
      <source>Sweep</source>
      <translation>Balayage</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.ui" line="23"/>
      <source>Sweep Path</source>
      <translation>Chemin de balayage</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.ui" line="53"/>
      <source>Create solid</source>
      <translation>Créer le solide</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.ui" line="60"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskSweep.cpp" line="461"/>
      <source>Select one or more profiles and select an edge or wire
in the 3D view for the sweep path.</source>
      <translation>Sélectionnez au moins un profil dans la liste, puis une arête ou un fil dans la vue 3D comme chemin.</translation>
    </message>
  </context>
  <context>
    <name>PartGui::TaskTube</name>
    <message>
      <location filename="../../../BasicShapes/TaskTube.ui" line="14"/>
      <source>Tube</source>
      <translation>Tube</translation>
    </message>
    <message>
      <location filename="../../../BasicShapes/TaskTube.ui" line="23"/>
      <source>Parameter</source>
      <translation>Paramètre</translation>
    </message>
    <message>
      <location filename="../../../BasicShapes/TaskTube.ui" line="78"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../../BasicShapes/TaskTube.ui" line="85"/>
      <source>Outer radius</source>
      <translation>Rayon extérieur</translation>
    </message>
    <message>
      <location filename="../../../BasicShapes/TaskTube.ui" line="92"/>
      <source>Inner radius</source>
      <translation>Rayon interne</translation>
    </message>
  </context>
  <context>
    <name>PartGui::ThicknessWidget</name>
    <message>
      <location filename="../../TaskThickness.cpp" line="99"/>
      <location filename="../../TaskThickness.cpp" line="279"/>
      <location filename="../../TaskThickness.cpp" line="289"/>
      <source>Thickness</source>
      <translation>Épaisseur</translation>
    </message>
    <message>
      <location filename="../../TaskThickness.cpp" line="177"/>
      <source>Select faces of the source object and press 'Done'</source>
      <translation>Sélectionnez des faces de l'objet source et cliquez sur 'Terminé'</translation>
    </message>
    <message>
      <location filename="../../TaskThickness.cpp" line="180"/>
      <source>Done</source>
      <translation>Fait</translation>
    </message>
    <message>
      <location filename="../../TaskThickness.cpp" line="244"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>Part_FaceMaker</name>
    <message>
      <location filename="../../../App/FaceMaker.cpp" line="172"/>
      <source>Simple</source>
      <translation>Simple</translation>
    </message>
    <message>
      <location filename="../../../App/FaceMaker.cpp" line="177"/>
      <source>Makes separate plane face from every wire independently. No support for holes; wires can be on different planes.</source>
      <translation>Construit des faces planes séparées à partir de chaque fil indépendamment. Les trous ne sont pas pris en charge ; les fils peuvent être sur différents plans.</translation>
    </message>
    <message>
      <location filename="../../../App/FaceMakerBullseye.cpp" line="72"/>
      <source>Bull's-eye facemaker</source>
      <translation>Générateur de faces cible</translation>
    </message>
    <message>
      <location filename="../../../App/FaceMakerBullseye.cpp" line="77"/>
      <source>Supports making planar faces with holes with islands.</source>
      <translation>Supporte la création de faces planaires avec des îlots.</translation>
    </message>
    <message>
      <location filename="../../../App/FaceMakerCheese.cpp" line="249"/>
      <source>Cheese facemaker</source>
      <translation>Générateur de faces gruyère</translation>
    </message>
    <message>
      <location filename="../../../App/FaceMakerCheese.cpp" line="254"/>
      <source>Supports making planar faces with holes, but no islands inside holes.</source>
      <translation>Supporte la création de faces planaires avec des trous, mais pas d'îlots à l'intérieur des trous.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrusion.cpp" line="504"/>
      <source>Part Extrude facemaker</source>
      <translation>Générateur de faces extrusion</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrusion.cpp" line="509"/>
      <source>Supports making faces with holes, does not support nesting.</source>
      <translation>Supporte la création de faces trouées, ne supporte pas l'imbrication.</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="37"/>
      <source>&amp;Part</source>
      <translation>&amp;Pièce</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="38"/>
      <source>&amp;Simple</source>
      <translation>&amp;Simple</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="39"/>
      <source>&amp;Parametric</source>
      <translation>&amp;Paramétrique</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="40"/>
      <source>Solids</source>
      <translation>Solides</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="41"/>
      <source>Part tools</source>
      <translation>Outils de pièces</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="42"/>
      <source>Boolean</source>
      <translation>Opération booléenne</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>Primitives</source>
      <translation>Primitives</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>Join</source>
      <translation>Joindre</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Split</source>
      <translation>Scinder</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Compound</source>
      <translation>Composé</translation>
    </message>
  </context>
</TS>
