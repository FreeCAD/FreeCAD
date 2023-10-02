<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="72"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Le point central de démarrage de l'hélice ; dérivé de l'axe de référence.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="74"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>La direction de l'hélice ; dérivée de l'axe de référence.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="76"/>
      <source>The reference axis of the helix.</source>
      <translation>L'axe de référence de l'hélice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="78"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Le mode de saisie de l'hélice spécifie les propriétés définies par l'utilisateur.
Les propriétés dépendantes sont ensuite calculées.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="82"/>
      <source>The axial distance between two turns.</source>
      <translation>La distance axiale entre deux tours.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="84"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>La hauteur de la trajectoire de l'hélice, sans tenir compte de l'étendue du profil.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="86"/>
      <source>The number of turns in the helix.</source>
      <translation>Le nombre de tours de l'hélice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="89"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, nevatige shrink.</source>
      <translation>L'angle du cône qui forme une coque autour de l'hélice.
Des valeurs non nulles transforment l'hélice en une spirale conique.
Les valeurs positives font croître le rayon, les valeurs négatives le rétrécissent.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>La croissance du rayon de l'hélice par tour.
Les valeurs non nulles transforment l'hélice en une spirale conique.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Définit le sens de rotation à gauche,
c'est-à-dire dans le sens inverse des aiguilles d'une montre lorsqu'on se déplace le long de son axe.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="100"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Détermine si l'hélice pointe dans la direction opposée de l'axe.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="102"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Si activé, le résultat sera l'intersection du profil et du corps préexistant.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Si désactivé, l'outil proposera une valeur initiale pour le pas basée sur la boîte englobante du profil, de sorte que l'auto-intersection soit évitée.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="98"/>
      <source>Number of gear teeth</source>
      <translation>Nombre de dents de l'engrenage</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="101"/>
      <source>Modules of the gear</source>
      <translation>Modules de l'engrenage</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="104"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Angle de pression des dents de l'engrenage</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>True=2 curves with each 3 control points False=1 curve with 4 control points.</source>
      <translation>True=2, courbes avec chaque 3 points de contrôle
False=1, courbe avec 4 points de contrôle.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="111"/>
      <source>True=external Gear False=internal Gear</source>
      <translation>True=engrenage externe
False=engrenage interne</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="115"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>La hauteur de la dent depuis le cercle primitif jusqu'à sa pointe, normalisée par le module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="119"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>La hauteur de la dent depuis le cercle primitif jusqu'à sa racine, normalisée par le module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="123"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Le rayon du congé à la racine de la dent, normalisé par le module.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>La distance à partir de laquelle le profil de référence est déplacé vers l'extérieur, normalisé par le module.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1430"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1431"/>
      <source>Additive helix</source>
      <translation>Hélice additive</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1432"/>
      <source>Sweep a selected sketch along a helix</source>
      <translation>Balayer une esquisse sélectionnée le long d'une hélice</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1329"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1330"/>
      <source>Additive loft</source>
      <translation>Lissage additif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1331"/>
      <source>Loft a selected profile through other profile sections</source>
      <translation>Lisser un profil sélectionné à travers d'autres sections de profil</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Additive pipe</source>
      <translation>Balayage additif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1229"/>
      <source>Sweep a selected sketch along a path or to other profiles</source>
      <translation>Balayer une esquisse sélectionnée sur un chemin ou vers d’autres profils</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="85"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="86"/>
      <source>Create body</source>
      <translation>Créer un corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>Create a new body and make it active</source>
      <translation>Crée un nouveau corps et l'activer</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2343"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2344"/>
      <source>Boolean operation</source>
      <translation>Opération booléenne</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2345"/>
      <source>Boolean operation with two or more bodies</source>
      <translation>Opération Booléenne entre deux ou plusieurs corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Create a local coordinate system</source>
      <translation>Créer un système de coordonnées local</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Create a new local coordinate system</source>
      <translation>Créer un nouveau système de coordonnées local</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1724"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1725"/>
      <source>Chamfer</source>
      <translation>Chanfrein</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1726"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>Chanfreiner les arêtes sélectionnées d'une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="428"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>Create a clone</source>
      <translation>Créer un clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="430"/>
      <source>Create a new clone</source>
      <translation>Crée un nouveau clone</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="1753"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1754"/>
      <source>Draft</source>
      <translation>Dépouille</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1755"/>
      <source>Make a draft on a face</source>
      <translation>Créer une dépouille sur une face</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="606"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="607"/>
      <source>Duplicate selected object</source>
      <translation>Dupliquer l'objet sélectionné</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="608"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Duplique l’objet sélectionné et l’ajoute au corps actif</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1696"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1697"/>
      <source>Fillet</source>
      <translation>Congé</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1698"/>
      <source>Make a fillet on an edge, face or body</source>
      <translation>Faire un congé sur une arête, une face ou un corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1160"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1161"/>
      <source>Groove</source>
      <translation>Rainure</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1162"/>
      <source>Groove a selected sketch</source>
      <translation>Faire une rainure à partir de l'esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1054"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1055"/>
      <source>Hole</source>
      <translation>Perçage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1056"/>
      <source>Create a hole with the selected sketch</source>
      <translation>Créer un perçage à partir de l’esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Create a datum line</source>
      <translation>Créer une droite de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>Create a new datum line</source>
      <translation>Créer une nouvelle droite de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2041"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2042"/>
      <source>LinearPattern</source>
      <translation>Répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>Create a linear pattern feature</source>
      <translation>Créer une fonction de répétition linéaire</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="312"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="313"/>
      <source>Migrate</source>
      <translation>Migrer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="314"/>
      <source>Migrate document to the modern PartDesign workflow</source>
      <translation>Migrer le document vers la nouvelle méthodologie PartDesign</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="1979"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1980"/>
      <source>Mirrored</source>
      <translation>Symétrie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1981"/>
      <source>Create a mirrored feature</source>
      <translation>Créer une fonction de symétrie</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="662"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="663"/>
      <source>Move object to other body</source>
      <translation>Déplacer vers un autre corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="664"/>
      <source>Moves the selected object to another body</source>
      <translation>Déplace l’objet sélectionné vers un autre corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="829"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="830"/>
      <source>Move object after other object</source>
      <translation>Déplacer après un autre objet</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="831"/>
      <source>Moves the selected object and insert it after another object</source>
      <translation>Déplace l’objet sélectionné et l’insère après un autre objet</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="527"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="528"/>
      <source>Set tip</source>
      <translation>Désigner comme fonction résultante</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="529"/>
      <source>Move the tip of the body</source>
      <translation>Déplacer la fonction résultante du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2219"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2220"/>
      <source>Create MultiTransform</source>
      <translation>Transformation multiple</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2221"/>
      <source>Create a multitransform feature</source>
      <translation>Créer une fonction de transformation multiple</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="502"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="503"/>
      <source>Create sketch</source>
      <translation>Créer une esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="504"/>
      <source>Create a new sketch</source>
      <translation>Créer une nouvelle esquisse</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="990"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="991"/>
      <source>Pad</source>
      <translation>Protrusion</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Pad a selected sketch</source>
      <translation>Faire une protrusion à partir de l'esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="162"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>Create a datum plane</source>
      <translation>Créer un plan de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>Create a new datum plane</source>
      <translation>Créer un nouveau plan de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1022"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1023"/>
      <source>Pocket</source>
      <translation>Cavité</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1024"/>
      <source>Create a pocket with the selected sketch</source>
      <translation>Créer une cavité à partir de l’esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>Create a datum point</source>
      <translation>Créer un point de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Create a new datum point</source>
      <translation>Créer un nouveau point de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2105"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2106"/>
      <source>PolarPattern</source>
      <translation>Répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2107"/>
      <source>Create a polar pattern feature</source>
      <translation>Créer une fonction de répétition circulaire</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1101"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1102"/>
      <source>Revolution</source>
      <translation>Révolution</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1103"/>
      <source>Revolve a selected sketch</source>
      <translation>Révolution d'une esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2170"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2171"/>
      <source>Scaled</source>
      <translation>Mise à l'échelle</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2172"/>
      <source>Create a scaled feature</source>
      <translation>Créer une fonction de mise à l'échelle</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>Create a shape binder</source>
      <translation>Créer une forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Create a new shape binder</source>
      <translation>Créer une nouvelle forme liée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="344"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="345"/>
      <location filename="../../Command.cpp" line="346"/>
      <source>Create a sub-object(s) shape binder</source>
      <translation>Créer une forme liée du sous-objet(s)</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1507"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1508"/>
      <source>Subtractive helix</source>
      <translation>Hélice soustractive</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1509"/>
      <source>Sweep a selected sketch along a helix and remove it from the body</source>
      <translation>Balayer une esquisse sélectionnée le long d'une hélice et la soustraire au corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1380"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1381"/>
      <source>Subtractive loft</source>
      <translation>Enlèvement de matière par lissage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1382"/>
      <source>Loft a selected profile through other profile sections and remove it from the body</source>
      <translation>Lisser un profil sélectionné à travers d'autres sections de profil et soustraire du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1278"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1279"/>
      <source>Subtractive pipe</source>
      <translation>Enlèvement de matière par balayage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1280"/>
      <source>Sweep a selected sketch along a path or to other profiles and remove it from the body</source>
      <translation>Balayer une esquisse sélectionnée sur un chemin et à travers d’autres profils et soustraire du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Thickness</source>
      <translation>Coque</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1823"/>
      <source>Make a thick solid</source>
      <translation>Générer une coque solide</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Create an additive primitive</source>
      <translation>Créer une primitive additive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="199"/>
      <source>Additive Box</source>
      <translation>Cube additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="203"/>
      <source>Additive Cylinder</source>
      <translation>Cylindre additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="207"/>
      <source>Additive Sphere</source>
      <translation>Sphère additive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="211"/>
      <source>Additive Cone</source>
      <translation>Cône additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Ellipsoid</source>
      <translation>Ellipsoïde additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="219"/>
      <source>Additive Torus</source>
      <translation>Tore additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="223"/>
      <source>Additive Prism</source>
      <translation>Prisme additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="227"/>
      <source>Additive Wedge</source>
      <translation>Pyramide tronquée additive</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <location filename="../../CommandPrimitive.cpp" line="245"/>
      <source>Create a subtractive primitive</source>
      <translation>Créer une primitive soustractive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="355"/>
      <source>Subtractive Box</source>
      <translation>Cube soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="359"/>
      <source>Subtractive Cylinder</source>
      <translation>Cylindre soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="363"/>
      <source>Subtractive Sphere</source>
      <translation>Sphère soustractive</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="367"/>
      <source>Subtractive Cone</source>
      <translation>Cône soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="371"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Ellipsoïde soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="375"/>
      <source>Subtractive Torus</source>
      <translation>Tore soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="379"/>
      <source>Subtractive Prism</source>
      <translation>Prisme soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="383"/>
      <source>Subtractive Wedge</source>
      <translation>Pyramide tronquée soustractive</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="299"/>
      <source>Edit ShapeBinder</source>
      <translation>Modifier la forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="308"/>
      <source>Create ShapeBinder</source>
      <translation>Créer une forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="391"/>
      <source>Create SubShapeBinder</source>
      <translation>Créer une sous-forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="446"/>
      <source>Create Clone</source>
      <translation>Créer un clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="890"/>
      <location filename="../../SketchWorkflow.cpp" line="297"/>
      <source>Make copy</source>
      <translation>Faire une copie</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="247"/>
      <source>Create a Sketch on Face</source>
      <translation>Créer une esquisse sur une face</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="487"/>
      <source>Create a new Sketch</source>
      <translation>Créer une nouvelle esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2269"/>
      <source>Convert to MultiTransform feature</source>
      <translation>Convertir en fonction de transformation multiple</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2361"/>
      <source>Create Boolean</source>
      <translation>Créer un booléen</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="189"/>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <source>Add a Body</source>
      <translation>Ajouter un corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="430"/>
      <source>Migrate legacy part design features to Bodies</source>
      <translation>Migrer les fonctions de conception de pièces existantes vers les corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="575"/>
      <source>Move tip to selected feature</source>
      <translation>Déplacer la fonction résultante vers l'élément sélectionné</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="620"/>
      <source>Duplicate a PartDesign object</source>
      <translation>Dupliquer un objet Part Design</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="743"/>
      <source>Move an object</source>
      <translation>Déplacer un objet</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="894"/>
      <source>Move an object inside tree</source>
      <translation>Déplacer un objet dans l'arborescence</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="294"/>
      <source>Mirrored</source>
      <translation>Symétrie</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="322"/>
      <source>Make LinearPattern</source>
      <translation>Répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="360"/>
      <source>PolarPattern</source>
      <translation>Répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="388"/>
      <source>Scaled</source>
      <translation>Mise à l'échelle</translation>
    </message>
  </context>
  <context>
    <name>FeaturePickDialog</name>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="42"/>
      <source>Valid</source>
      <translation>Valide</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="43"/>
      <source>Invalid shape</source>
      <translation>Forme non valide</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>Aucun fil dans l'esquisse</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>Esquisse déjà utilisée par une autre fonction</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another Body feature</source>
      <translation>L 'esquisse appartient à une fonction sous un autre corps</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>Plan de base</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the Tip feature</source>
      <translation>La fonction est située après la fonction sommet</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Face tools</source>
      <translation>Outils de face</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Sketch tools</source>
      <translation>Outils d'esquisse</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Create Geometry</source>
      <translation>Créer géométrie</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute parameter</source>
      <translation>Paramètres de l'engrenage à développante</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>Nombre de dents :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module:</source>
      <translation>Module :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle:</source>
      <translation>Angle de pression :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision:</source>
      <translation>Haute précision :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Vrai</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Faux</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear:</source>
      <translation>Engrenages externes :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum Coefficient</source>
      <translation>Coefficient addendum :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum Coefficient</source>
      <translation>Coefficient dedendum :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root Fillet Coefficient</source>
      <translation>Coefficient du congé à la base :</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile Shift Coefficient</source>
      <translation>Coefficient de décalage du profil :</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Corps actif requis</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document.

Please select a body from below, or create a new body.</source>
      <translation>Pour créer un nouvel objet Part Design, il doit y avoir un corps actif dans le document.

Sélectionner un corps à partir du bas ou créer un nouveau corps.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="36"/>
      <source>Create new body</source>
      <translation>Créer un nouveau corps</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
      <source>Please select</source>
      <translation>Veuillez sélectionner</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Primitives géométriques</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length:</source>
      <translation>Longueur :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width:</source>
      <translation>Largeur :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <source>Angle in first direction:</source>
      <translation>Angle dans la première direction :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Angle dans la première direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <source>Angle in second direction:</source>
      <translation>Angle dans la seconde direction :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Angle dans la seconde direction</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle:</source>
      <translation>Angle de rotation :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1:</source>
      <translation>Rayon 1 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2:</source>
      <translation>Rayon 2 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle:</source>
      <translation>Angle :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <source>U parameter:</source>
      <translation>Paramètre U :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters:</source>
      <translation>Paramètre V :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Rayon dans la direction z locale</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local x-direction</source>
      <translation>Rayon dans la direction x locale</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3:</source>
      <translation>Rayon 3 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local y-direction
If zero, it is equal to Radius2</source>
      <translation>Rayon dans la direction y locale. Si zéro, il est égal au Rayon2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter:</source>
      <translation>Paramètre V :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local xy-plane</source>
      <translation>Rayon dans le plan xy local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local xz-plane</source>
      <translation>Rayon dans le plan xz local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U Parameter:</source>
      <translation>Paramètre U :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon:</source>
      <translation>Polygone :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius:</source>
      <translation>Rayon circonscrit :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max:</source>
      <translation>Min/max en X :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max:</source>
      <translation>Min/max en Y :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max:</source>
      <translation>Min/max en Z :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max:</source>
      <translation>Min/max en X2 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max:</source>
      <translation>Min/max en Z2 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch:</source>
      <translation>Pas :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system:</source>
      <translation>Système de coordonnées :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>Pas à droite</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Pas à gauche</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth:</source>
      <translation>Croissance :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations:</source>
      <translation>Nombre de rotations :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1:</source>
      <translation>Angle 1 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2:</source>
      <translation>Angle 2 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From three points</source>
      <translation>À partir de trois points</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius:</source>
      <translation>Rayon principal :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius:</source>
      <translation>Rayon secondaire :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z:</source>
      <translation>Z :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Point de départ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Point final</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Référence</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Vous avez sélectionné des éléments géométriques qui ne font pas partie du corps actif. Veuillez définir comment gérer ces sélections. Si vous ne voulez pas de ces références, annulez la commande.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Faire une copie indépendante (recommandée)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Faire une copie dépendante</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Créer une référence croisée</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="270"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Cette sélection créera une dépendance circulaire.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add body</source>
      <translation>Ajouter un corps</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove body</source>
      <translation>Enlever un corps</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Union</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Soustraction</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="52"/>
      <source>Boolean parameters</source>
      <translation>Paramètres Booléens</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="81"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="49"/>
      <source>Primitive parameters</source>
      <translation>Paramètres de la primitive</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="760"/>
      <source>Cone radii are equal</source>
      <translation>Les rayon du cône sont égaux</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="761"/>
      <source>The radii for cones must not be equal!</source>
      <translation>Les rayons des cônes ne doivent pas être égaux !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="836"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="841"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="846"/>
      <source>Invalid wedge parameters</source>
      <translation>Paramètres de la pyramide tronquée non valides</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="837"/>
      <source>X min must not be equal to X max!</source>
      <translation>X min ne doit pas être égal à X max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="842"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y min ne doit pas être égal à Y max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="847"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z min ne doit pas être égal à Z max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="885"/>
      <source>Create primitive</source>
      <translation>Créer une primitive</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="24"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les chanfreins</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="49"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="57"/>
      <source>Equal distance</source>
      <translation>Cote égale</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="62"/>
      <source>Two distances</source>
      <translation>Deux cotes</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="67"/>
      <source>Distance and angle</source>
      <translation>Cote et angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="80"/>
      <source>Flip direction</source>
      <translation>Inverser la direction</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="101"/>
      <source>Size</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="120"/>
      <source>Use All Edges</source>
      <translation>Utiliser tous les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="147"/>
      <source>Size 2</source>
      <translation>Taille 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="180"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="323"/>
      <source>Empty chamfer created !
</source>
      <translation>Création d'un chanfrein vide !</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="347"/>
      <source>Empty body list</source>
      <translation>Liste de corps vide</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="348"/>
      <source>The body list cannot be empty</source>
      <translation>La liste de corps ne peut pas être vide</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="360"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Booléen : Accepter : erreur d'entrée</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="101"/>
      <source>Incompatible reference set</source>
      <translation>Combinaison de références incompatible</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="102"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Il n'y a aucun mode d'accrochage qui corresponde à la combinaison de références actuelle. Si vous continuez, la fonction restera là où elle est et ne bougera pas si les références changent. Continuer?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="130"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="408"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="24"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les dépouilles</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="47"/>
      <source>Draft angle</source>
      <translation>Angle de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="80"/>
      <source>Neutral plane</source>
      <translation>Plan neutre</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="97"/>
      <source>Pull direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="112"/>
      <source>Reverse pull direction</source>
      <translation>Inverser la direction de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="281"/>
      <source>Empty draft created !
</source>
      <translation>Création d'une esquisse vide !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="84"/>
      <source>Preview</source>
      <translation>Aperçu</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="90"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="262"/>
      <source>Add all edges</source>
      <translation>Ajouter toutes les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="269"/>
      <source>Adds all edges to the list box (active only when in add selection mode).</source>
      <translation>Ajoute toutes les arêtes à la liste (actif uniquement en mode sélection ajoutée).</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="277"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="897"/>
      <source>No face selected</source>
      <translation>Aucune face sélectionnée</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="153"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="735"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="357"/>
      <source>Sketch normal</source>
      <translation>Normale à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="359"/>
      <source>Face normal</source>
      <translation>Face normale</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="362"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="366"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="368"/>
      <source>Custom direction</source>
      <translation>Direction personnalisée </translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="892"/>
      <source>Click on a face in the model</source>
      <translation>Cliquer sur une face du modèle</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Autoriser les fonctions utilisées</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow external features</source>
      <translation>Autoriser les fonctions externes</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>À partir d'autres corps de la même pièce</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>De différentes pièces ou de fonctions indépendantes</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Faire une copie indépendante (recommandée)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Faire une copie dépendante</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Créer une référence croisée</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="61"/>
      <source>Valid</source>
      <translation>Valide</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="62"/>
      <source>Invalid shape</source>
      <translation>Forme non valide</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="63"/>
      <source>No wire in sketch</source>
      <translation>Aucun fil dans l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="64"/>
      <source>Sketch already used by other feature</source>
      <translation>Esquisse déjà utilisée par une autre fonction</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="65"/>
      <source>Belongs to another body</source>
      <translation>Appartient à un autre corps</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="66"/>
      <source>Belongs to another part</source>
      <translation>Appartient à une autre pièce</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Doesn't belong to any body</source>
      <translation>N'appartient à aucun corps</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="68"/>
      <source>Base plane</source>
      <translation>Plan de base</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Feature is located after the tip feature</source>
      <translation>La fonction est située après la fonction résultante</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="80"/>
      <source>Select feature</source>
      <translation>Sélectionner une fonction</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="24"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les congés</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="47"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="63"/>
      <source>Use All Edges</source>
      <translation>Utiliser toutes les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="193"/>
      <source>Empty fillet created !
</source>
      <translation>Création d'un congé vide !
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status:</source>
      <translation>État :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Valide</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis:</source>
      <translation>Axe :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="232"/>
      <source>Base X axis</source>
      <translation>Axe X</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="233"/>
      <source>Base Y axis</source>
      <translation>Axe Y</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="234"/>
      <source>Base Z axis</source>
      <translation>Axe Z</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="215"/>
      <source>Horizontal sketch axis</source>
      <translation>Axe d'esquisse horizontal</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="214"/>
      <source>Vertical sketch axis</source>
      <translation>Axe d'esquisse vertical</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="213"/>
      <source>Normal sketch axis</source>
      <translation>Axe normal à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="197"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode:</source>
      <translation>Mode:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Pas-Hauteur-Angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Pas-Tours-Angle</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Hauteur-Tours-Angles</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Hauteur-Tours-Croissance</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch:</source>
      <translation>Pas :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns:</source>
      <translation>Tours :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle:</source>
      <translation>Angle du cône :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth:</source>
      <translation>Croissance radiale :</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Pas à gauche</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>Supprimer l'extérieur du profil</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="51"/>
      <source>Helix parameters</source>
      <translation>Paramètres de l'hélice</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="217"/>
      <source>Construction line %1</source>
      <translation>Ligne de construction %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="281"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Attention : l'hélice peut s'intersecter elle-même</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="286"/>
      <source>Error: helix touches itself</source>
      <translation>Erreur : l'hélice se touche elle-même</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="334"/>
      <source>Error: unsupported mode</source>
      <translation>Erreur : mode non pris en charge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="47"/>
      <source>Counterbore</source>
      <translation>Lamage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="48"/>
      <source>Countersink</source>
      <translation>Fraisage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="49"/>
      <source>Counterdrill</source>
      <translation>Contre-perçage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="53"/>
      <source>Hole parameters</source>
      <translation>Paramètres de perçage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="63"/>
      <source>None</source>
      <translation>Rien</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="64"/>
      <source>ISO metric regular profile</source>
      <translation>Filetage métrique ISO à pas standard</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>ISO metric fine profile</source>
      <translation>Filetage métrique ISO à pas fin</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="66"/>
      <source>UTS coarse profile</source>
      <translation>Filetage unifié (UTS) grossier</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>UTS fine profile</source>
      <translation>Filetage unifié (UTS) fin</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>UTS extra fine profile</source>
      <translation>Filetage unifié (UTS) extra fin</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLinearPatternParameters</name>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Ajouter une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Supprimer une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="56"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="68"/>
      <source>Reverse direction</source>
      <translation>Inverser la direction</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="77"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="85"/>
      <source>Overall Length</source>
      <translation>Longueur globale</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="90"/>
      <location filename="../../TaskLinearPatternParameters.ui" line="153"/>
      <source>Offset</source>
      <translation>Décalage</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="115"/>
      <source>Length</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="178"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="192"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="201"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="108"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="382"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Surface réglée</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Fermé</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Profilé</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Objet</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Ajouter une section</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Supprimer une section</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft parameters</source>
      <translation>Paramètres de lissage</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Ajouter une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Supprimer une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="56"/>
      <source>Plane</source>
      <translation>Plan</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="70"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="79"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="101"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="244"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Ajouter une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Supprimer une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="54"/>
      <source>Transformations</source>
      <translation>Transformations</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="71"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="89"/>
      <source>Edit</source>
      <translation>Éditer</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="93"/>
      <source>Delete</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="97"/>
      <source>Add mirrored transformation</source>
      <translation>Ajouter une fonction de symétrie</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="101"/>
      <source>Add linear pattern</source>
      <translation>Ajouter une répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="105"/>
      <source>Add polar pattern</source>
      <translation>Ajouter une répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="109"/>
      <source>Add scaled transformation</source>
      <translation>Ajouter une transformation de mise à l'échelle</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="113"/>
      <source>Move up</source>
      <translation>Déplacer vers le haut</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="117"/>
      <source>Move down</source>
      <translation>Déplacer vers le bas</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="144"/>
      <source>Right-click to add</source>
      <translation>Faites un clic droit pour ajouter</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad parameters</source>
      <translation>Paramètres de protrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset from face at which pad will end</source>
      <translation>Décalage à partir de la face à laquelle la protrusion se terminera</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Reverses pad direction</source>
      <translation>Inverse la direction de la protrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="70"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>To last</source>
      <translation>Au dernier</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To first</source>
      <translation>Au premier</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Up to face</source>
      <translation>Jusqu'à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Two dimensions</source>
      <translation>Deux dimensions</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="38"/>
      <source>Length</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="75"/>
      <source>Offset to face</source>
      <translation>Décalage par rapport à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="141"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="149"/>
      <source>Direction/edge:</source>
      <translation>Direction/arête :</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="156"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Définir une direction ou sélectionner une arête
du modèle comme référence</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="161"/>
      <source>Sketch normal</source>
      <translation>Normale à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="166"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="171"/>
      <source>Custom direction</source>
      <translation>Direction personnalisée </translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="181"/>
      <source>Show direction</source>
      <translation>Afficher la direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="191"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Utiliser un vecteur personnalisé pour la direction de la protrusion sinon
le vecteur normal du plan de l'esquisse sera utilisé</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="204"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="211"/>
      <source>x-component of direction vector</source>
      <translation>composante x du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="233"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="240"/>
      <source>y-component of direction vector</source>
      <translation>composante y du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="262"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="269"/>
      <source>z-component of direction vector</source>
      <translation>composante z du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="300"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Si non cochée, la longueur sera
mesurée dans la direction spécifiée</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="304"/>
      <source>Length along sketch normal</source>
      <translation>Longueur le long de la normale à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="124"/>
      <source>Applies length symmetrically to sketch plane</source>
      <translation>Applique une longueur symétriquement au plan de l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="127"/>
      <source>Symmetric to plane</source>
      <translation>Symétrique au plan</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="134"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="316"/>
      <location filename="../../TaskPadPocketParameters.ui" line="340"/>
      <source>Angle to taper the extrusion</source>
      <translation>Angle pour effiler l'extrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="319"/>
      <source>Taper angle</source>
      <translation>Angle de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="58"/>
      <source>2nd length</source>
      <translation>2ème longueur</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="343"/>
      <source>2nd taper angle</source>
      <translation>2ème angle de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="102"/>
      <source>Select face</source>
      <translation>Sélectionner une face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="369"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Mode d’orientation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Fixé</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Auxiliaire</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Binormal</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvelinear equivalence</source>
      <translation>Équivalence curviligne</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Profilé</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Objet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Ajouter une arête</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Enlever une arête</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Définir le vecteur binormal constant utilisé pour calculer l'orientation des profils</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="190"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="197"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="204"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="534"/>
      <source>Section orientation</source>
      <translation>Orientation de la section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="560"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Profilé</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Objet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner Transition</source>
      <translation>Transition de coin</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Transformé</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right Corner</source>
      <translation>Coin droit</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round Corner</source>
      <translation>Coin arrondi</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to sweep along</source>
      <translation>Trajectoire à balayer tout le long</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add Edge</source>
      <translation>Ajouter une arête</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove Edge</source>
      <translation>Enlever une arête</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="68"/>
      <source>Pipe parameters</source>
      <translation>Paramètres de balayage</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="84"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="422"/>
      <location filename="../../TaskPipeParameters.cpp" line="520"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="422"/>
      <source>No active body</source>
      <translation>Aucun corps actif</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Mode de transformation</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Constant</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Sections multiples</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Ajouter une section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Supprimer une section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="802"/>
      <source>Section transformation</source>
      <translation>Transformation de section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="818"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket parameters</source>
      <translation>Paramètres de la cavité</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from face at which pocket will end</source>
      <translation>Décalage à partir de la face à laquelle la cavité se terminera</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation>Inverse la direction de la cavité</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="72"/>
      <source>Through all</source>
      <translation>À travers tout</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>Au premier</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>Jusqu'à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Two dimensions</source>
      <translation>Deux dimensions</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPolarPatternParameters</name>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Ajouter une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Supprimer une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation>La liste peut être réordonnée en faisant glisser</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="56"/>
      <source>Axis</source>
      <translation>Axe</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="68"/>
      <source>Reverse direction</source>
      <translation>Inverser la direction</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="77"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="85"/>
      <source>Overall Angle</source>
      <translation>Angle global</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="90"/>
      <source>Offset Angle</source>
      <translation>Angle de décalage</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="115"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="159"/>
      <source>Offset</source>
      <translation>Décalage</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="190"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="204"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="213"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="114"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="377"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="935"/>
      <source>Attachment</source>
      <translation>Accrochage</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Axis:</source>
      <translation>Axe :</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="137"/>
      <source>Base X axis</source>
      <translation>Axe X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="35"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="138"/>
      <source>Base Y axis</source>
      <translation>Axe Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="40"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="139"/>
      <source>Base Z axis</source>
      <translation>Axe Z</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="45"/>
      <source>Horizontal sketch axis</source>
      <translation>Axe d'esquisse horizontal</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <source>Vertical sketch axis</source>
      <translation>Axe d'esquisse vertical</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="146"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="67"/>
      <source>Angle:</source>
      <translation>Angle :</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="101"/>
      <source>Symmetric to plane</source>
      <translation>Symétrique au plan</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="108"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="122"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="50"/>
      <source>Revolution parameters</source>
      <translation>Paramètres de la révolution</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Ajouter une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Supprimer une fonction</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="53"/>
      <source>Factor</source>
      <translation>Facteur</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="67"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="81"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="90"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.cpp" line="92"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Objet</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Ajouter géométrie</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Supprimer géométrie</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="60"/>
      <source>Datum shape parameters</source>
      <translation>Paramètres de la forme liée</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="130"/>
      <source>Remove</source>
      <translation>Supprimer</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="161"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="24"/>
      <source>Select</source>
      <translation>Sélectionner</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="34"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les fonctions</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="47"/>
      <source>Thickness</source>
      <translation>Coque</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="76"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="84"/>
      <source>Skin</source>
      <translation>Couche</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="89"/>
      <source>Pipe</source>
      <translation>Tuyau</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="94"/>
      <source>Recto Verso</source>
      <translation>Recto-verso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="102"/>
      <source>Join Type</source>
      <translation>Type de raccordement</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="110"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="115"/>
      <location filename="../../TaskThicknessParameters.ui" line="125"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="132"/>
      <source>Make thickness inwards</source>
      <translation>Générer l'épaisseur vers l'intérieur</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="248"/>
      <source>Empty thickness created !
</source>
      <translation>Création d'une coque vide !</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed feature messages</source>
      <translation>Messages de la fonction de transformation</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="250"/>
      <source>Normal sketch axis</source>
      <translation>Axe normal à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="251"/>
      <source>Vertical sketch axis</source>
      <translation>Axe d'esquisse vertical</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="252"/>
      <source>Horizontal sketch axis</source>
      <translation>Axe d'esquisse horizontal</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="254"/>
      <location filename="../../TaskTransformedParameters.cpp" line="290"/>
      <source>Construction line %1</source>
      <translation>Ligne de construction %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="268"/>
      <source>Base X axis</source>
      <translation>Axe X</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="269"/>
      <source>Base Y axis</source>
      <translation>Axe Y</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="270"/>
      <source>Base Z axis</source>
      <translation>Axe Z</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="277"/>
      <location filename="../../TaskTransformedParameters.cpp" line="313"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="304"/>
      <source>Base XY plane</source>
      <translation>Plan XY</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="305"/>
      <source>Base YZ plane</source>
      <translation>Plan YZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="306"/>
      <source>Base XZ plane</source>
      <translation>Plan XZ</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="135"/>
      <source>Toggle active body</source>
      <translation>Activer/désactiver le corps</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer parameters</source>
      <translation>Paramètres du chanfrein</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="111"/>
      <source>Datum Plane parameters</source>
      <translation>Paramètres du plan de référence</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="116"/>
      <source>Datum Line parameters</source>
      <translation>Paramètres de la droite de référence</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="121"/>
      <source>Datum Point parameters</source>
      <translation>Paramètres du point de référence</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="126"/>
      <source>Local Coordinate System parameters</source>
      <translation>Paramètres du système de coordonnées locales</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft parameters</source>
      <translation>Paramètres de la dépouille</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet parameters</source>
      <translation>Paramètres du congé</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="37"/>
      <source>LinearPattern parameters</source>
      <translation>Paramètres de la répétition linéaire</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="37"/>
      <source>MultiTransform parameters</source>
      <translation>Paramètres de la transformation multiple</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="37"/>
      <source>PolarPattern parameters</source>
      <translation>Paramètres de la répétition circulaire</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="37"/>
      <source>Scaled parameters</source>
      <translation>Paramètres de la mise à l'échelle</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness parameters</source>
      <translation>Paramètres d'épaisseur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="37"/>
      <source>Mirrored parameters</source>
      <translation>Paramètres de symétrie</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="200"/>
      <source>Create an additive box by its width, height, and length</source>
      <translation>Créer un cube additif par sa largeur, sa hauteur et sa longueur</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="204"/>
      <source>Create an additive cylinder by its radius, height, and angle</source>
      <translation>Créer un cylindre additif par son rayon, sa hauteur et son angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="208"/>
      <source>Create an additive sphere by its radius and various angles</source>
      <translation>Créer une sphère additive par son rayon et divers angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="212"/>
      <source>Create an additive cone</source>
      <translation>Créer un cône additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="216"/>
      <source>Create an additive ellipsoid</source>
      <translation>Créer un ellipsoïde additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="220"/>
      <source>Create an additive torus</source>
      <translation>Créer un tore additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Create an additive prism</source>
      <translation>Créer un prisme additif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="228"/>
      <source>Create an additive wedge</source>
      <translation>Créer une pyramide tronquée additive</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="356"/>
      <source>Create a subtractive box by its width, height and length</source>
      <translation>Créer un cube soustractif par sa largeur, sa hauteur et sa longueur</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="360"/>
      <source>Create a subtractive cylinder by its radius, height and angle</source>
      <translation>Créer un cylindre soustractif par son rayon, sa hauteur et son angle</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="364"/>
      <source>Create a subtractive sphere by its radius and various angles</source>
      <translation>Créer une sphère soustractive par son rayon et divers angles</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="368"/>
      <source>Create a subtractive cone</source>
      <translation>Créer un cône soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="372"/>
      <source>Create a subtractive ellipsoid</source>
      <translation>Créer un ellipsoïde soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="376"/>
      <source>Create a subtractive torus</source>
      <translation>Créer un tore soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="380"/>
      <source>Create a subtractive prism</source>
      <translation>Créer un prisme soustractif</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="384"/>
      <source>Create a subtractive wedge</source>
      <translation>Créer une pyramide tronquée soustractive</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="732"/>
      <source>Select body</source>
      <translation>Sélectionner le corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="733"/>
      <source>Select a body from the list</source>
      <translation>Sélectionner un corps dans la liste</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="885"/>
      <source>Select feature</source>
      <translation>Sélectionner une fonction</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="886"/>
      <source>Select a feature from the list</source>
      <translation>Sélectionner une fonction dans la liste</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="957"/>
      <source>Move tip</source>
      <translation>Déplacer la fonction résultante</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="958"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>La fonction déplacée apparaît après la fonction résultante en cours.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="959"/>
      <source>Do you want the last feature to be the new tip?</source>
      <translation>Voulez-vous que la dernière fonction soit la nouvelle fonction résultante?</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="139"/>
      <source>Invalid selection</source>
      <translation>Sélection non valide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="139"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Il n’y a aucun mode d'accrochage qui convienne aux objets sélectionnés. Sélectionnez autre chose.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="145"/>
      <location filename="../../Command.cpp" line="148"/>
      <location filename="../../Command.cpp" line="150"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="145"/>
      <source>There is no active body. Please make a body active before inserting a datum entity.</source>
      <translation>Pas de corps actif. Activez un corps avant d'insérer une référence.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="409"/>
      <source>Sub-Shape Binder</source>
      <translation>Sous forme liée</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="650"/>
      <source>Several sub-elements selected</source>
      <translation>Plusieurs sous-éléments sélectionnés</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="651"/>
      <source>You have to select a single face as support for a sketch!</source>
      <translation>Vous devez sélectionner une seule face comme support pour une esquisse !</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="654"/>
      <source>No support face selected</source>
      <translation>Aucune face de support sélectionnée</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="655"/>
      <source>You have to select a face as support for a sketch!</source>
      <translation>Vous devez sélectionner un plan ou une face plane comme support de l'esquisse !</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="658"/>
      <source>No planar support</source>
      <translation>Aucun plan de support</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="659"/>
      <source>You need a planar face as support for a sketch!</source>
      <translation>Vous avez besoin d'un plan ou d'une face plane comme support de l'esquisse !</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="662"/>
      <source>No valid planes in this document</source>
      <translation>Pas de plan valide dans le document</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="663"/>
      <source>Please create a plane first or select a face to sketch on</source>
      <translation>Créer d'abord un plan ou choisir une face sur laquelle appliquer l'esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="911"/>
      <location filename="../../Command.cpp" line="1930"/>
      <location filename="../../SketchWorkflow.cpp" line="591"/>
      <location filename="../../ViewProvider.cpp" line="95"/>
      <location filename="../../ViewProviderBoolean.cpp" line="78"/>
      <location filename="../../ViewProviderDatum.cpp" line="246"/>
      <location filename="../../ViewProviderHole.cpp" line="77"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="68"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Une boîte de dialogue est déjà ouverte dans le panneau des tâches</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="912"/>
      <location filename="../../Command.cpp" line="1931"/>
      <location filename="../../SketchWorkflow.cpp" line="592"/>
      <location filename="../../ViewProvider.cpp" line="96"/>
      <location filename="../../ViewProviderBoolean.cpp" line="79"/>
      <location filename="../../ViewProviderDatum.cpp" line="247"/>
      <location filename="../../ViewProviderHole.cpp" line="78"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="69"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <source>Do you want to close this dialog?</source>
      <translation>Voulez-vous fermer cette boîte de dialogue?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="791"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Impossible d'utiliser cette commande car il n'y a pas de solide à soustraire.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="792"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Assurez-vous que le corps contient une fonctionnalité avant de tenter une commande soustractive.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="813"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Impossible d'utiliser l'objet sélectionné. L'objet sélectionné doit appartenir au corps actif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="814"/>
      <source>Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</source>
      <translation>Envisagez d'utiliser une forme liée ou une fonction de base pour référencer une géométrie externe dans un corps.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="836"/>
      <source>No sketch to work on</source>
      <translation>Pas d'esquisse de travail</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="837"/>
      <source>No sketch is available in the document</source>
      <translation>Aucune esquisse n’est disponible dans le document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1579"/>
      <location filename="../../Command.cpp" line="1605"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1580"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Sélectionnez une arête, une face ou un corps d'un corps unique.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1584"/>
      <location filename="../../Command.cpp" line="1957"/>
      <source>Selection is not in Active Body</source>
      <translation>La sélection n’est pas dans le corps actif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1585"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Sélectionnez une arête, une face ou un corps d'un corps actif.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1595"/>
      <source>Wrong object type</source>
      <translation>Type d'objet incorrect</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1596"/>
      <source>%1 works only on parts.</source>
      <translation>%1 fonctionne uniquement sur les pièces.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1606"/>
      <source>Shape of the selected Part is empty</source>
      <translation>La forme de la pièce sélectionnée est vide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1948"/>
      <source>No valid features in this document</source>
      <translation>Aucune fonction valide dans ce document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1949"/>
      <source>Please create a feature first.</source>
      <translation>Veuillez d'abord créer une fonction.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1958"/>
      <source>Please select only one feature in an active body.</source>
      <translation>Veuillez sélectionner une seule fonction d'un corps actif.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="67"/>
      <source>Part creation failed</source>
      <translation>La création de la pièce a échoué</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="68"/>
      <source>Failed to create a part object.</source>
      <translation>Impossible de créer un objet pièce.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="115"/>
      <location filename="../../CommandBody.cpp" line="120"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="182"/>
      <source>Bad base feature</source>
      <translation>Mauvaise fonction de base</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="116"/>
      <source>Body can't be based on a PartDesign feature.</source>
      <translation>Le corps ne peut pas être basé sur une fonction de PartDesign.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="121"/>
      <source>%1 already belongs to a body, can't use it as base feature for another body.</source>
      <translation>%1 appartient déjà à un corps, ne peut pas être utilisé comme fonction de base pour un autre corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>La fonction de base (%1) appartient à une autre pièce.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="158"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>La forme sélectionnée se compose de plusieurs solides. Cela peut conduire à des résultats inattendus.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="162"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>La forme sélectionnée se compose de plusieurs coques. Cela peut conduire à des résultats inattendus.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="166"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>La forme sélectionnée se compose de seulement une coque. Cela peut conduire à des résultats inattendus.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="170"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>La forme sélectionnée se compose de plusieurs solides ou de coques. Cela peut conduire à des résultats inattendus.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="175"/>
      <source>Base feature</source>
      <translation>Fonction de base</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Le corps ne peut être basé que sur une seule fonction.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="197"/>
      <source>Body</source>
      <translation>Corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="343"/>
      <source>Nothing to migrate</source>
      <translation>Rien à migrer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="344"/>
      <source>No PartDesign features found that don't belong to a body. Nothing to migrate.</source>
      <translation>Aucune fonction de Part Design trouvée qui n'appartienne pas à un corps. Rien à migrer.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="492"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Le plan d'esquisse ne peut pas être migré</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="493"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Veuillez éditer '%1' et définir son plan selon un plan de base ou de référence.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="555"/>
      <location filename="../../CommandBody.cpp" line="559"/>
      <location filename="../../CommandBody.cpp" line="564"/>
      <location filename="../../CommandBody.cpp" line="857"/>
      <location filename="../../CommandBody.cpp" line="864"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="556"/>
      <source>Select exactly one PartDesign feature or a body.</source>
      <translation>Sélectionnez seulement une fonction ou un corps PartDesign.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="560"/>
      <source>Couldn't determine a body for the selected feature '%s'.</source>
      <translation>Impossible de déterminer un corps pour la fonction '%s' sélectionnée.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="565"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Seule une fonction solide peut être la fonction résultante d'un corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="683"/>
      <location filename="../../CommandBody.cpp" line="705"/>
      <location filename="../../CommandBody.cpp" line="720"/>
      <source>Features cannot be moved</source>
      <translation>Impossible de déplacer les fonctions</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="684"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Certaines des fonctions sélectionnées ont des dépendances dans le corps source</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="706"/>
      <source>Only features of a single source Body can be moved</source>
      <translation>Seules les fonctions d'un seul corps source peuvent être déplacées</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="721"/>
      <source>There are no other bodies to move to</source>
      <translation>Il n'y a aucun autre corps vers lequel déplacer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="858"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Impossible de déplacer la fonction de base d’un corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="865"/>
      <source>Select one or more features from the same body.</source>
      <translation>Sélectionnez une ou plusieurs fonctions dans le même corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="878"/>
      <source>Beginning of the body</source>
      <translation>Début du corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="943"/>
      <source>Dependency violation</source>
      <translation>Violation de dépendance</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="944"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>La fonction première ne doit pas dépendre de la fonction suivante.</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="269"/>
      <source>No previous feature found</source>
      <translation>Aucune fonction antérieure trouvée</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="270"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Il n’est pas possible de créer une fonction soustractive sans une fonction de base présente</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="123"/>
      <location filename="../../TaskTransformedParameters.cpp" line="287"/>
      <source>Vertical sketch axis</source>
      <translation>Axe d'esquisse vertical</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="124"/>
      <location filename="../../TaskTransformedParameters.cpp" line="288"/>
      <source>Horizontal sketch axis</source>
      <translation>Axe d'esquisse horizontal</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="126"/>
      <source>Construction line %1</source>
      <translation>Ligne de construction %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="77"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="138"/>
      <source>In order to use PartDesign you need an active Body object in the document. Please make one active (double click) or create one.

If you have a legacy document with PartDesign objects without Body, use the migrate function in PartDesign to put them into a Body.</source>
      <translation>Pour utiliser PartDesign, vous avez besoin d'un corps actif dans le document. Veuillez en activer un (par double-clic) ou en créer un.

Si vous avez un vieux document avec des objets PartDesign sans corps, utilisez la fonction de migration sous PartDesign pour les inclure dans un corps.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="186"/>
      <source>Active Body Required</source>
      <translation>Corps actif requis</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="187"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document. Please make one active (double click) or create a new Body.</source>
      <translation>Pour créer un nouvel objet de PartDesign, il doit y avoir un corps actif dans le document. Veuillez en activer un (par double clic) ou créer un nouveau corps.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="222"/>
      <source>Feature is not in a body</source>
      <translation>La fonction ne fait pas partie d'un corps</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="223"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Pour utiliser cette fonction, elle doit appartenir à un corps dans le document.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="255"/>
      <source>Feature is not in a part</source>
      <translation>La fonction ne fait pas partie d'une pièce</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="256"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Afin d'utiliser cette fonction, elle doit appartenir à un objet pièce dans le document.</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="62"/>
      <location filename="../../ViewProviderDressUp.cpp" line="50"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="201"/>
      <location filename="../../ViewProviderTransformed.cpp" line="76"/>
      <source>Edit %1</source>
      <translation>Modifier %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="75"/>
      <source>Set colors...</source>
      <translation>Définir les couleurs...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="62"/>
      <source>Edit boolean</source>
      <translation>Modifier la fonction Booléenne</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="110"/>
      <location filename="../../ViewProviderDatum.cpp" line="204"/>
      <source>Plane</source>
      <translation>Plan</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <location filename="../../ViewProviderDatum.cpp" line="200"/>
      <source>Line</source>
      <translation>Ligne</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <location filename="../../ViewProviderDatum.cpp" line="208"/>
      <source>Point</source>
      <translation>Point</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Coordinate System</source>
      <translation>Système de coordonnées</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="225"/>
      <source>Edit datum</source>
      <translation>Modifier la référence</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="74"/>
      <source>Feature error</source>
      <translation>Erreur de la fonction</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="75"/>
      <source>%1 misses a base feature.
This feature is broken and can't be edited.</source>
      <translation>%1 est sans fonction de base.
Cette fonction est cassée et ne peut pas être modifiée.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="46"/>
      <source>Edit groove</source>
      <translation>Modifier la rainure</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="61"/>
      <source>Edit hole</source>
      <translation>Modifier le perçage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="66"/>
      <source>Edit loft</source>
      <translation>Modifier le lissage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="46"/>
      <source>Edit pad</source>
      <translation>Modifier la protrusion</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="74"/>
      <source>Edit pipe</source>
      <translation>Modifier le balayage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="48"/>
      <source>Edit pocket</source>
      <translation>Modifier la cavité</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="54"/>
      <source>Edit primitive</source>
      <translation>Modifier la primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="46"/>
      <source>Edit revolution</source>
      <translation>Modifier la révolution</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="196"/>
      <source>Edit shape binder</source>
      <translation>Modifier la forme liée</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="305"/>
      <source>Synchronize</source>
      <translation>Synchroniser</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="307"/>
      <source>Select bound object</source>
      <translation>Sélectionner l'objet lié</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="179"/>
      <source>One transformed shape does not intersect support</source>
      <translation>Une des formes transformées n'intersecte pas le support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="181"/>
      <source>%1 transformed shapes do not intersect support</source>
      <translation>%1 des formes transformées n'intersectent pas le support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="191"/>
      <source>Transformation succeeded</source>
      <translation>Transformation réussie</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="140"/>
      <source>The document "%1" you are editing was designed with an old version of PartDesign workbench.</source>
      <translation>Le document « %1 » que vous modifiez a été conçu dans une vieille version de l'atelier PartDesign.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="143"/>
      <source>Do you want to migrate in order to use modern PartDesign features?</source>
      <translation>Voulez-vous appliquer la migration afin d'utiliser les fonctions modernes PartDesign ?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="146"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy PartDesign or have a slightly broken structure.</source>
      <translation>Le document « %1 » semble être au milieu du processus de migration de PartDesign ou a une structure légèrement cassée.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Do you want to make the migration automatically?</source>
      <translation>Voulez-vous faire la migration automatiquement ?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="152"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Remarque : si vous choisissez de migrer, vous ne serez pas en mesure d'éditer le fichier avec d’anciennes versions de FreeCAD. Si vous refusez de migrer, vous ne serez pas en mesure d’utiliser les nouvelles fonctionnalités de Part Design comme les corps et les pièces. Pour résultat, vous ne serez pas en mesure d’utiliser vos pièces dans l'atelier assemblage. Cependant, vous pourrez migrer à tout moment par la suite avec "Part Design -&gt; Migrer".</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate manually</source>
      <translation>Migrer manuellement</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="51"/>
      <source>Edit helix</source>
      <translation>Modifier l'hélice</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="41"/>
      <source>Edit chamfer</source>
      <translation>Modifier le chanfrein</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="42"/>
      <source>Edit draft</source>
      <translation>Modifier l'esquisse</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="41"/>
      <source>Edit fillet</source>
      <translation>Modifier le congé</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="39"/>
      <source>Edit linear pattern</source>
      <translation>Modifier la répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="39"/>
      <source>Edit mirrored</source>
      <translation>Éditer en miroir</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="48"/>
      <source>Edit multi-transform</source>
      <translation>Modifier la transformation multiple</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit polar pattern</source>
      <translation>Modifier la répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="39"/>
      <source>Edit scaled</source>
      <translation>Modifier l'échelle</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="41"/>
      <source>Edit thickness</source>
      <translation>Modifier l'évidement</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket parameter</source>
      <translation>Paramètres du pignon</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>Nombre de dents :</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="40"/>
      <source>Sprocket Reference</source>
      <translation>Référence du pignon :</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="48"/>
      <source>ANSI 25</source>
      <translation>ANSI 25</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="53"/>
      <source>ANSI 35</source>
      <translation>ANSI 35</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="58"/>
      <source>ANSI 41</source>
      <translation>ANSI 41</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="63"/>
      <source>ANSI 40</source>
      <translation>ANSI 40</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="68"/>
      <source>ANSI 50</source>
      <translation>ANSI 50</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="73"/>
      <source>ANSI 60</source>
      <translation>ANSI 60</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="78"/>
      <source>ANSI 80</source>
      <translation>ANSI 80</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="83"/>
      <source>ANSI 100</source>
      <translation>ANSI 100</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="88"/>
      <source>ANSI 120</source>
      <translation>ANSI 120</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="93"/>
      <source>ANSI 140</source>
      <translation>ANSI 140</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="98"/>
      <source>ANSI 160</source>
      <translation>ANSI 160</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="103"/>
      <source>ANSI 180</source>
      <translation>ANSI 180</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="108"/>
      <source>ANSI 200</source>
      <translation>ANSI 200</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="113"/>
      <source>ANSI 240</source>
      <translation>ANSI 240</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="118"/>
      <source>Bicycle with Derailleur</source>
      <translation>Vélo avec Derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="123"/>
      <source>Bicycle without Derailleur</source>
      <translation>Vélo sans Derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="128"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="133"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 08B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="138"/>
      <source>ISO 606 10B</source>
      <translation>ISO 606 10B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="143"/>
      <source>ISO 606 12B</source>
      <translation>ISO 606 12B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="148"/>
      <source>ISO 606 16B</source>
      <translation>ISO 606 16B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="153"/>
      <source>ISO 606 20B</source>
      <translation>ISO 606 20B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="158"/>
      <source>ISO 606 24B</source>
      <translation>ISO 606 24B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="163"/>
      <source>Motorcycle 420</source>
      <translation>Moto 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="168"/>
      <source>Motorcycle 425</source>
      <translation>Moto 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="173"/>
      <source>Motorcycle 428</source>
      <translation>Moto 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="178"/>
      <source>Motorcycle 520</source>
      <translation>Moto 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="183"/>
      <source>Motorcycle 525</source>
      <translation>Moto 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="188"/>
      <source>Motorcycle 530</source>
      <translation>Moto 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="193"/>
      <source>Motorcycle 630</source>
      <translation>Moto 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Chain Pitch:</source>
      <translation>Pas de la chaîne :</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="220"/>
      <source>0 in</source>
      <translation>0 dans</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="248"/>
      <source>Roller Diameter:</source>
      <translation>Diamètre du cylindre :</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="292"/>
      <source>Thickness:</source>
      <translation>Épaisseur :</translation>
    </message>
  </context>
  <context>
    <name>TaskHole</name>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="24"/>
      <source>Position</source>
      <translation>Position</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="35"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="49"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="87"/>
      <source>Edge</source>
      <translation>Arête</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="63"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="101"/>
      <source>Distance</source>
      <translation>Distance</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="137"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="145"/>
      <source>Through</source>
      <translation>A travers</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="152"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="492"/>
      <source>Depth</source>
      <translation>Profondeur</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="161"/>
      <source>Threaded</source>
      <translation>Fileté</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="168"/>
      <source>Countersink</source>
      <translation>Fraisage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="175"/>
      <source>Counterbore</source>
      <translation>Lamage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="196"/>
      <source>Hole norm</source>
      <translation>Norme de perçage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="202"/>
      <source>Custom dimensions</source>
      <translation>Dimensions personnalisées</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="218"/>
      <source>Tolerance</source>
      <translation>Tolérance</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="249"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="368"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="474"/>
      <source>Diameter</source>
      <translation>Diamètre</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="280"/>
      <source>Bolt/Washer</source>
      <translation>Boulon/Rondelle</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="329"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="337"/>
      <source>Thread norm</source>
      <translation>Norme de filetage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="399"/>
      <source> Custom thread length</source>
      <translation> Longueur de filetage personnalisée</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="423"/>
      <source>Finish depth</source>
      <translation>Profondeur finale</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="466"/>
      <source>Data</source>
      <translation>Données</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="510"/>
      <source>Counterbore/sink dia</source>
      <translation>Diamètre de lamage/fraisage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="528"/>
      <source>Counterbore depth</source>
      <translation>Profondeur de lamage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="546"/>
      <source>Countersink angle</source>
      <translation>Angle de fraisage</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="564"/>
      <source>Thread length</source>
      <translation>Longueur de filetage</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Task Hole Parameters</source>
      <translation>Paramètres du perçage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="26"/>
      <source>&lt;b&gt;Threading and size&lt;/b&gt;</source>
      <translation>&lt;b&gt;Filetage et taille&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="39"/>
      <source>Profile</source>
      <translation>Profilé</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="62"/>
      <source>Whether the hole gets a thread</source>
      <translation>Si le trou reçoit un filetage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Threaded</source>
      <translation>Fileté</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="78"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>Si le trou reçoit un filetage modélisé</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="81"/>
      <source>Model Thread</source>
      <translation>Filetage représenté</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="91"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Mise à jour en direct des modifications apportées au taraudage
Notez que le calcul peut prendre un certain temps</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Update view</source>
      <translation>Mise à jour de la vue</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="108"/>
      <source>Customize thread clearance</source>
      <translation>Spécifier le dégagement du filetage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="111"/>
      <source>Custom Thread</source>
      <translation>Filetage personnalisé</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="124"/>
      <location filename="../../TaskHoleParameters.ui" line="233"/>
      <source>Clearance</source>
      <translation>Jeu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="137"/>
      <source>Custom Thread clearance value</source>
      <translation>Valeur du dégagement du filetage avec jeu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="159"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="178"/>
      <source>Right hand</source>
      <translation>Pas à droite</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="194"/>
      <source>Left hand</source>
      <translation>Pas à gauche</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="210"/>
      <source>Size</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="246"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Dégagement du trou. Uniquement disponible pour les trous sans filetage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="251"/>
      <location filename="../../TaskHoleParameters.cpp" line="618"/>
      <source>Standard</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="256"/>
      <location filename="../../TaskHoleParameters.cpp" line="619"/>
      <location filename="../../TaskHoleParameters.cpp" line="630"/>
      <source>Close</source>
      <translation>Faible</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="261"/>
      <location filename="../../TaskHoleParameters.cpp" line="620"/>
      <source>Wide</source>
      <translation>Large</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="275"/>
      <source>Class</source>
      <translation>Classe</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="288"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Tolérance d'ajustement pour les trous filetés selon le profil du trou</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="301"/>
      <location filename="../../TaskHoleParameters.ui" line="488"/>
      <source>Diameter</source>
      <translation>Diamètre</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="314"/>
      <source>Hole diameter</source>
      <translation>Diamètre du trou</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="336"/>
      <location filename="../../TaskHoleParameters.ui" line="526"/>
      <source>Depth</source>
      <translation>Profondeur</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="350"/>
      <location filename="../../TaskHoleParameters.ui" line="404"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="355"/>
      <source>Through all</source>
      <translation>À travers tout</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="385"/>
      <source>Thread Depth</source>
      <translation>Longueur de filetage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>Hole depth</source>
      <translation>Profondeur de la cavité</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="409"/>
      <source>Tapped (DIN76)</source>
      <translation>DIN76</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="433"/>
      <source>&lt;b&gt;Hole cut&lt;/b&gt;</source>
      <translation>&lt;b&gt;Découpe du trou&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="446"/>
      <location filename="../../TaskHoleParameters.ui" line="613"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="459"/>
      <source>Cut type for screw heads</source>
      <translation>Type de coupe pour les têtes de vis</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="472"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Cochez cette case pour remplacer les valeurs prédéfinies par le 'Type'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="475"/>
      <source>Custom values</source>
      <translation>Valeurs spécifiques</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="539"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>Pour les fraisages, il s'agit de la profondeur de la partie supérieure de la vis sous la surface.</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="565"/>
      <source>Countersink angle</source>
      <translation>Angle de fraisage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="597"/>
      <source>&lt;b&gt;Drill point&lt;/b&gt;</source>
      <translation>&lt;b&gt;Pointe de perçage&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="629"/>
      <source>Flat</source>
      <translation>Plat</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="645"/>
      <source>Angled</source>
      <translation>En angle</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="680"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>La taille de la pointe du foret sera prise en compte
compte pour la profondeur des trous borgnes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="684"/>
      <source>Take into account for depth</source>
      <translation>Tenir compte de la profondeur</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="691"/>
      <source>&lt;b&gt;Misc&lt;/b&gt;</source>
      <translation>&lt;b&gt;Divers&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="704"/>
      <source>Tapered</source>
      <translation>Conique</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="717"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Angle de conicité pour le trou
90 degrés : trou droit
moins de 90 : rayon du trou plus petit au fond
plus de 90 : rayon du trou plus grand à la base</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="742"/>
      <source>Reverses the hole direction</source>
      <translation>Inverse la direction du trou</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="745"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="629"/>
      <source>Normal</source>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="631"/>
      <source>Loose</source>
      <translation>Détachés</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Aucun message</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Esquisse</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Part Design</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Create a datum</source>
      <translation>Créer une référence</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Create an additive feature</source>
      <translation>Créer une fonction additive</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Create a subtractive feature</source>
      <translation>Créer une fonction soustractive</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Apply a pattern</source>
      <translation>Appliquer une transformation</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Apply a dress-up feature</source>
      <translation>Appliquer une fonction d'habillage</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket...</source>
      <translation>Pignon...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute gear...</source>
      <translation>Engrenage à développante...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Shaft design wizard</source>
      <translation>Conception d'arbre</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Measure</source>
      <translation>Mesure</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Refresh</source>
      <translation>Rafraîchir</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Toggle 3D</source>
      <translation>Basculer vers les mesures dans la 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Part Design Helper</source>
      <translation>Assistant de Part Design</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Part Design Modeling</source>
      <translation>Modélisation de Part Design</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="58"/>
      <source>Involute gear...</source>
      <translation>Engrenage à développante...</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Creates or edit the involute gear definition.</source>
      <translation>Créer ou modifier la définition de l'engrenage à développante.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket...</source>
      <translation>Pignon...</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edit the sprocket definition.</source>
      <translation>Créer ou modifier la définition du pignon.</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Length [mm]</source>
      <translation>Longueur [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Diameter [mm]</source>
      <translation>Diamètre [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Inner diameter [mm]</source>
      <translation>Diamètre intérieur [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Constraint type</source>
      <translation>Type de contrainte</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Start edge type</source>
      <translation>Type d'arête au départ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge size</source>
      <translation>Taille de l'arête au départ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>End edge type</source>
      <translation>Type d'arête de fin</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>Taille de l'arête de fin</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="64"/>
      <source>Shaft wizard</source>
      <translation>Assistant de l'arbre</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 1</source>
      <translation>Section 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="72"/>
      <source>Section 2</source>
      <translation>Section 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Add column</source>
      <translation>Ajouter une colonne</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="122"/>
      <source>Section %s</source>
      <translation>Section %s</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="57"/>
      <source>All</source>
      <translation>Tout</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="104"/>
      <source>Missing module</source>
      <translation>Module manquant</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="105"/>
      <source>You may have to install the Plot add-on</source>
      <translation>Vous devrez peut-être installer l'extension Plot</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="189"/>
      <source>Shaft design wizard...</source>
      <translation>Conception d'arbre...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="190"/>
      <source>Start the shaft design wizard</source>
      <translation>Démarrer la conception d'arbre</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="214"/>
      <source>Shaft design wizard...</source>
      <translation>Conception d'arbre...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="215"/>
      <source>Start the shaft design wizard</source>
      <translation>Démarrer la conception d'arbre</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="401"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>L'objet lié n'est pas une fonctionnalité PartDesign</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="408"/>
      <source>Tip shape is empty</source>
      <translation>La forme de la fonction résultante est vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="62"/>
      <source>BaseFeature link is not set</source>
      <translation>Le lien BaseFeature n'est pas défini</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="65"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>BaseFeature doit être une Part::Feature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="69"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature a une forme vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="78"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>Impossible de faire une coupe booléenne sans BaseFeature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="112"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Impossible de faire un booléen avec autre chose qu'un Part::Feature et ses dérivés</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="99"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>Impossible de faire une opération booléenne avec une forme de base invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="105"/>
      <source>Cannot do boolean on feature which is not in a body</source>
      <translation>Impossible de faire un booléen sur une fonctionnalité qui n'est pas dans un corps</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="119"/>
      <source>Base shape is null</source>
      <translation>La forme de base est nulle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="122"/>
      <source>Tool shape is null</source>
      <translation>La forme de l'outil est nulle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="127"/>
      <source>Fusion of tools failed</source>
      <translation>La fusion des outils a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="132"/>
      <location filename="../../../App/FeatureGroove.cpp" line="161"/>
      <location filename="../../../App/FeatureHole.cpp" line="1900"/>
      <location filename="../../../App/FeatureLoft.cpp" line="293"/>
      <location filename="../../../App/FeatureLoft.cpp" line="311"/>
      <location filename="../../../App/FeaturePad.cpp" line="217"/>
      <location filename="../../../App/FeaturePipe.cpp" line="394"/>
      <location filename="../../../App/FeaturePipe.cpp" line="414"/>
      <location filename="../../../App/FeaturePocket.cpp" line="222"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="103"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Resulting shape is not a solid</source>
      <translation>La forme résultante n'est pas un solide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="136"/>
      <source>Cut out failed</source>
      <translation>La découpe a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="141"/>
      <source>Common operation failed</source>
      <translation>L'opération commune a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="152"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="223"/>
      <location filename="../../../App/FeatureDraft.cpp" line="323"/>
      <location filename="../../../App/FeatureFillet.cpp" line="137"/>
      <location filename="../../../App/FeatureGroove.cpp" line="168"/>
      <location filename="../../../App/FeatureHole.cpp" line="1908"/>
      <location filename="../../../App/FeatureLoft.cpp" line="296"/>
      <location filename="../../../App/FeatureLoft.cpp" line="314"/>
      <location filename="../../../App/FeaturePad.cpp" line="221"/>
      <location filename="../../../App/FeaturePad.cpp" line="230"/>
      <location filename="../../../App/FeaturePipe.cpp" line="398"/>
      <location filename="../../../App/FeaturePipe.cpp" line="418"/>
      <location filename="../../../App/FeaturePocket.cpp" line="191"/>
      <location filename="../../../App/FeaturePocket.cpp" line="226"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="107"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="127"/>
      <source>Result has multiple solids: that is not currently supported.</source>
      <translation>Le résultat a plusieurs solides : ce n'est pas supporté actuellement.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="203"/>
      <source>Failed to create chamfer</source>
      <translation>Impossible de créer le chanfrein</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="207"/>
      <location filename="../../../App/FeatureDraft.cpp" line="319"/>
      <location filename="../../../App/FeatureFillet.cpp" line="120"/>
      <source>Resulting shape is null</source>
      <translation>La forme résultante est nulle</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="218"/>
      <location filename="../../../App/FeatureFillet.cpp" line="131"/>
      <source>Resulting shape is invalid</source>
      <translation>La forme résultante est invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="289"/>
      <source>Size must be greater than zero</source>
      <translation>La taille doit être plus grande que zéro</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="298"/>
      <source>Size2 must be greater than zero</source>
      <translation>La taille 2 doit être plus grande que zéro</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="303"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>L'angle doit être compris entre 0 et 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="315"/>
      <source>Failed to create draft</source>
      <translation>Échec de la création de l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="93"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>Le rayon du congé doit être plus grand que zéro</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="116"/>
      <source>Failed to create fillet</source>
      <translation>Échec de la création de congé</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="78"/>
      <source>Angle of groove too large</source>
      <translation>Angle de rainure trop grand</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="82"/>
      <source>Angle of groove too small</source>
      <translation>Angle de rainure trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <location filename="../../../App/FeatureHole.cpp" line="1669"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>La fonction demandée ne peut pas être créée. La raison est peut-être que :
  \xe2\x80\xa2 le corps actif ne contient pas de forme de base et donc il n’y a aucune matière
  à enlever ;
  \xe2\x80\xa2 l’esquisse sélectionnée n’appartient pas au corps actif.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="118"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="118"/>
      <source>Creating a face from sketch failed</source>
      <translation>La création d'une face à partir de l'esquisse a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="140"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="140"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>L'axe de révolution coupe l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="156"/>
      <source>Cut out of base feature failed</source>
      <translation>La découpe de l'élément de base a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="173"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="165"/>
      <source>Could not revolve the sketch!</source>
      <translation>Impossible de tourner l'esquisse !</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="180"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="172"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>Impossible de créer une face à partir de l'esquisse.
Les entités d'intersection d'esquisse dans une esquisse ne sont pas autorisées.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="127"/>
      <source>Error: Pitch too small</source>
      <translation>Erreur : pas trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="129"/>
      <location filename="../../../App/FeatureHelix.cpp" line="143"/>
      <source>Error: height too small!</source>
      <translation>Erreur : hauteur trop petite !</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="135"/>
      <source>Error: pitch too small!</source>
      <translation>Erreur: Pas trop petit!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="137"/>
      <location filename="../../../App/FeatureHelix.cpp" line="145"/>
      <location filename="../../../App/FeatureHelix.cpp" line="151"/>
      <source>Error: turns too small!</source>
      <translation>Erreur : pas assez de tours !</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="155"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Erreur : la hauteur ou la croissance ne doit pas être nulle !</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="169"/>
      <source>Error: unsupported mode</source>
      <translation>Erreur : mode non pris en charge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="181"/>
      <source>Error: No valid sketch or face</source>
      <translation>Erreur : pas d'esquisse ou de face valide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="190"/>
      <source>Error: Face must be planar</source>
      <translation>Erreur : la face doit être plane</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="256"/>
      <source>Error: Could not build</source>
      <translation>Erreur : impossible de construire</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="296"/>
      <location filename="../../../App/FeatureHelix.cpp" line="330"/>
      <location filename="../../../App/FeatureHelix.cpp" line="360"/>
      <location filename="../../../App/FeatureHole.cpp" line="2145"/>
      <source>Error: Result is not a solid</source>
      <translation>Erreur : le résultat n'est pas un solide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="310"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Erreur : il n'y a rien à soustraire</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="314"/>
      <location filename="../../../App/FeatureHelix.cpp" line="334"/>
      <location filename="../../../App/FeatureHelix.cpp" line="364"/>
      <source>Error: Result has multiple solids</source>
      <translation>Erreur : le résultat a plusieurs solides</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="324"/>
      <source>Error: Adding the helix failed</source>
      <translation>Erreur : l'ajout de l'hélice a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="347"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>Erreur : l'intersection de l'hélice a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="354"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>Erreur : la soustraction de l'hélice a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="376"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Erreur: Impossible de créer une face à partir de l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1687"/>
      <source>Hole error: Creating a face from sketch failed</source>
      <translation>Erreur de trou: La création d'une face à partir de l'esquisse a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1712"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Erreur de trou: Spécification de longueur non prise en charge</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1715"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Erreur de trou: profondeur de trou invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1738"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Erreur de trou: angle de cône non valide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1759"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Erreur de trou: diamètre de coupe de trou trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1763"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Erreur de trou: La profondeur de coupe du trou doit être inférieure à la profondeur du trou</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1767"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Erreur de trou: La profondeur de coupe du trou doit être supérieure ou égale à zéro</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1789"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Erreur de trou : fraisage non valide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1822"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Erreur de trou: angle de point de perçage invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1832"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Erreur de trou: point de perçage invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1866"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Erreur de trou: Impossible de faire tourner l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1870"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Erreur de trou: La forme résultante est vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1880"/>
      <source>Error: Adding the thread failed</source>
      <translation>Erreur : L'ajout du fil de suivi a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1892"/>
      <location filename="../../../App/FeatureTransformed.cpp" line="283"/>
      <location filename="../../../App/FeatureTransformed.cpp" line="298"/>
      <source>Boolean operation failed</source>
      <translation>L'opération booléenne a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1919"/>
      <location filename="../../../App/FeaturePocket.cpp" line="242"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>Impossible de créer une face à partir d'une esquisse.
L'intersection d'entités d'esquisse ou de plusieurs faces dans une esquisse n'est pas autorisée pour créer une poche jusqu'à une face.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2025"/>
      <source>Thread type out of range</source>
      <translation>Type de filetage hors plage</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2028"/>
      <source>Thread size out of range</source>
      <translation>Taille de filetage hors plage</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2120"/>
      <source>Error: Thread could not be built</source>
      <translation>Erreur: le filetage n'a pas pu être construit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="135"/>
      <source>Loft: At least one section is needed</source>
      <translation>Lissage : Au moins une section est nécessaire</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="140"/>
      <source>Loft: Could not obtain profile shape</source>
      <translation>Lissage: Impossible d'obtenir la forme du profil</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="153"/>
      <source>Loft: When using points for profile/sections, the sketch should have a single point</source>
      <translation>Lissage : Lors de l'utilisation de points pour le profil/sections, l'esquisse devrait avoir un seul point</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="161"/>
      <source>Loft: All sections need to be part features</source>
      <translation>Lissage: toutes les sections doivent faire partie des fonctionnalités</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="166"/>
      <source>Loft: Could not obtain section shape</source>
      <translation>Lissage: Impossible d'obtenir la forme de la section</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="182"/>
      <source>Loft: A section doesn't contain any wires nor is a single vertex</source>
      <translation>Lissage: Une section ne contient aucun fil ni aucun sommet</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="184"/>
      <source>Loft: Only the profile and the last section can be vertices</source>
      <translation>lissage : Seul le profil et la dernière section peuvent être des sommets</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="186"/>
      <source>Loft: For closed lofts only the profile can be a vertex</source>
      <translation>Lissage : Pour les lissages fermés, seul le profil peut être un sommet</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="193"/>
      <source>Loft: all loft sections need to have the same amount of inner wires</source>
      <translation>Lissage: toutes les sections de lissages doivent avoir le même nombre de fils internes</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="232"/>
      <source>Loft could not be built</source>
      <translation>Le Lissage n'a pas pu être construit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="265"/>
      <source>Loft: Result is not a solid</source>
      <translation>Lissage : Le résultat n'est pas un solide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="278"/>
      <source>Loft: There is nothing to subtract from</source>
      <translation>Lissage: Il n'y a rien à soustraire de</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="288"/>
      <source>Loft: Adding the loft failed</source>
      <translation>Lissage: L'ajout du lissage a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="306"/>
      <source>Loft: Subtracting the loft failed</source>
      <translation>Lissage : Échec de la soustraction du lissage</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="330"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Lissage : Une erreur fatale s'est produite lors de la création du lissage</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePad.cpp" line="125"/>
      <source>Pad: Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Protrusion : La création a échoué car la direction est orthogonale du vecteur normal de l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePad.cpp" line="136"/>
      <source>Pad: Creating a face from sketch failed</source>
      <translation>Protrusion : La création d'une face à partir de l'esquisse a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePad.cpp" line="200"/>
      <source>Pad: Resulting shape is empty</source>
      <translation>Protrusion : La forme résultante est vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePad.cpp" line="211"/>
      <source>Pad: Fusion with base feature failed</source>
      <translation>Protrusion : Fusion avec la fonction de base échouée</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePad.cpp" line="243"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>Impossible de créer une face à partir de l'esquisse.
Les entités d'intersection d'esquisse dans une esquisse ne sont pas autorisées.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="172"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Tuyau: impossible d'obtenir la forme du profil</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="177"/>
      <source>No spine linked</source>
      <translation>Pas d'arête liée</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="190"/>
      <source>No auxiliary spine linked.</source>
      <translation>Pas d'arête auxiliaire liée.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="211"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Tuyau : un seul point isolé est nécessaire si vous utilisez une esquisse avec des points isolés pour la section</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="217"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Tuyau : au moins une section est nécessaire lorsque vous utilisez un seul point pour le profil</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="231"/>
      <source>Pipe: All sections need to be part features</source>
      <translation>Tuyau : toutes les sections doivent être des part features</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="237"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Tuyau : impossible d'obtenir la forme du profil</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="246"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Tuyau : seul le profil et la dernière section peuvent être des sommets</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="255"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Les multisections doivent avoir le même nombre de polylignes internes que la section de base</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="282"/>
      <source>Path must not be a null shape</source>
      <translation>Le chemin ne doit pas être une forme nulle</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="317"/>
      <source>Pipe could not be built</source>
      <translation>Le tuyau n'a pas pu être construit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="363"/>
      <source>Result is not a solid</source>
      <translation>La forme résultante n'est pas un solide</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="378"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Tuyau : il n'y a rien à soustraire</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="389"/>
      <source>Adding the pipe failed</source>
      <translation>L'ajout du tuyau a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="409"/>
      <source>Subtracting the pipe failed</source>
      <translation>La soustraction du tuyau a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="433"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>Une erreur fatale s'est produite lors de la création du tuyau</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="556"/>
      <source>Invalid element in spine.</source>
      <translation>Élément non valide dans une arête.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="559"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>L'élément dans l'arête n'est ni un bord ni une polyligne.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="572"/>
      <source>Spine is not connected.</source>
      <translation>L'arête n'est pas connectée.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="576"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>L'arête n'est ni un bord ni une polyligne.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="580"/>
      <source>Invalid spine.</source>
      <translation>Arête non valide.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="132"/>
      <source>Pocket: Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>Cavité : la création a échoué car la direction est orthogonale au vecteur normal de l'esquisse</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="143"/>
      <source>Pocket: Creating a face from sketch failed</source>
      <translation>Cavité : la création d'une face à partir de l'esquisse a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="149"/>
      <source>Pocket: Extruding up to a face is only possible if the sketch is located on a face</source>
      <translation>Cavité : l'extrusion jusqu'à une face n'est possible que si l'esquisse est située sur une face</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="184"/>
      <source>Pocket: Up to face: Could not get SubShape!</source>
      <translation>Cavité : jusqu'à une face : impossible d'obtenir une sous-forme !</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="208"/>
      <source>Pocket: Resulting shape is empty</source>
      <translation>Cavité : la forme résultante est vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="217"/>
      <source>Pocket: Cut out of base feature failed</source>
      <translation>Cavité : la découpe de l'élément de base a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="89"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>Impossible de soustraire la fonction primitive sans la fonction de base</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="98"/>
      <source>Adding the primitive failed</source>
      <translation>L'ajout de la primitive a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="118"/>
      <source>Subtracting the primitive failed</source>
      <translation>La soustraction de la primitive a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="198"/>
      <source>Length of box too small</source>
      <translation>Longueur de la boîte trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="200"/>
      <source>Width of box too small</source>
      <translation>La largeur de la boîte est trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="202"/>
      <source>Height of box too small</source>
      <translation>La hauteur de la boîte est trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="248"/>
      <source>Radius of cylinder too small</source>
      <translation>Le rayon du cylindre est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="250"/>
      <source>Height of cylinder too small</source>
      <translation>La hauteur du cylindre est trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="252"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>L'angle de rotation du cylindre est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="305"/>
      <source>Radius of sphere too small</source>
      <translation>Le rayon de la sphère est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="354"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="356"/>
      <source>Radius of cone cannot be negative</source>
      <translation>Le rayon du cône ne peut pas être négatif</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="358"/>
      <source>The radii for cones must not be equal</source>
      <translation>Les rayons des cônes ne doivent pas être égaux</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="360"/>
      <source>Height of cone too small</source>
      <translation>La hauteur du cône est trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="417"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="419"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Le rayon de l'ellipsoïde est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="501"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="503"/>
      <source>Radius of torus too small</source>
      <translation>Le rayon du tore est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="566"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>Le polygone du prisme est invalide, il doit avoir au moins 3 côtés</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="568"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>Le rayon du cercle circonscrit du polygone, du prisme, est trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="570"/>
      <source>Height of prism is too small</source>
      <translation>La hauteur du prisme est trop petite</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="651"/>
      <source>delta x of wedge too small</source>
      <translation>delta x de la pyramide tronquée trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="654"/>
      <source>delta y of wedge too small</source>
      <translation>delta y de la pyramide tronquée trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="657"/>
      <source>delta z of wedge too small</source>
      <translation>delta z de la pyramide tronquée trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="660"/>
      <source>delta z2 of wedge is negative</source>
      <translation>delta z2 de la pyramide tronquée est négatif</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="663"/>
      <source>delta x2 of wedge is negative</source>
      <translation>delta x2 de la pyramide tronquée est négatif</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="77"/>
      <source>Angle of revolution too large</source>
      <translation>Angle de révolution trop grand</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="81"/>
      <source>Angle of revolution too small</source>
      <translation>Angle de révolution trop petit</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="157"/>
      <source>Fusion with base feature failed</source>
      <translation>La fusion avec la fonction de base échouée</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="94"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>La fonctionnalité de transformation de l'objet lié n'est pas un objet Part</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="97"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>Aucun original lié à la fonctionnalité transformée.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="204"/>
      <source>Cannot transform invalid support shape</source>
      <translation>Impossible de transformer une forme de support invalide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="233"/>
      <source>Transformation failed</source>
      <translation>La transformation a échoué</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="261"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>La forme de la fonction additive/soustractive est vide</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="269"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Seules les fonctionnalités additive et de soustractive peuvent être transformées</translation>
    </message>
  </context>
</TS>
