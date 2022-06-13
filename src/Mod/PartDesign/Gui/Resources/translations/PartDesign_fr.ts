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
      <translation>Si désactivé, l'outil proposera une valeur initiale pour le pas basée sur la boîte de délimitation du profil, de sorte que l'auto-intersection soit évitée.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Additive helix</source>
      <translation>Hélice additive</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1747"/>
      <source>Sweep a selected sketch along a helix</source>
      <translation>Balayer une esquisse sélectionnée le long d'une hélice</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1644"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1645"/>
      <source>Additive loft</source>
      <translation>Lissage additif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1646"/>
      <source>Loft a selected profile through other profile sections</source>
      <translation>Lisser un profil sélectionné à travers d'autres sections de profil</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1542"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1543"/>
      <source>Additive pipe</source>
      <translation>Balayage additif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1544"/>
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
      <location filename="../../Command.cpp" line="2638"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2639"/>
      <source>Boolean operation</source>
      <translation>Opération booléenne</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2640"/>
      <source>Boolean operation with two or more bodies</source>
      <translation>Opération Booléenne entre deux ou plusieurs corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="245"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>Create a local coordinate system</source>
      <translation>Créer un système de coordonnées local</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Create a new local coordinate system</source>
      <translation>Créer un nouveau système de coordonnées local</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="2038"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2039"/>
      <source>Chamfer</source>
      <translation>Chanfrein</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2040"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>Chanfreiner les arêtes sélectionnées d'une forme</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="427"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="428"/>
      <source>Create a clone</source>
      <translation>Créer un clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>Create a new clone</source>
      <translation>Crée un nouveau clone</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2067"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2068"/>
      <source>Draft</source>
      <translation>Dépouille</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2069"/>
      <source>Make a draft on a face</source>
      <translation>Créer une dépouille sur une face</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="602"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="603"/>
      <source>Duplicate selected object</source>
      <translation>Dupliquer l'objet sélectionné</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="604"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Duplique l’objet sélectionné et l’ajoute au corps actif</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="2010"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2011"/>
      <source>Fillet</source>
      <translation>Congé</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2012"/>
      <source>Make a fillet on an edge, face or body</source>
      <translation>Faire un congé sur une arête, une face ou un corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1475"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1476"/>
      <source>Groove</source>
      <translation>Rainure</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1477"/>
      <source>Groove a selected sketch</source>
      <translation>Faire une rainure à partir de l'esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1369"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1370"/>
      <source>Hole</source>
      <translation>Perçage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1371"/>
      <source>Create a hole with the selected sketch</source>
      <translation>Créer un perçage à partir de l’esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Create a datum line</source>
      <translation>Créer une droite de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Create a new datum line</source>
      <translation>Créer une nouvelle droite de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2336"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2337"/>
      <source>LinearPattern</source>
      <translation>Répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2338"/>
      <source>Create a linear pattern feature</source>
      <translation>Créer une fonction de répétition linéaire</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="308"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="309"/>
      <source>Migrate</source>
      <translation>Migrer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="310"/>
      <source>Migrate document to the modern PartDesign workflow</source>
      <translation>Migrer le document vers la nouvelle méthodologie PartDesign</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2274"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2275"/>
      <source>Mirrored</source>
      <translation>Symétrie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2276"/>
      <source>Create a mirrored feature</source>
      <translation>Créer une fonction de symétrie</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Move object to other body</source>
      <translation>Déplacer vers un autre corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the selected object to another body</source>
      <translation>Déplace l’objet sélectionné vers un autre corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="825"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="826"/>
      <source>Move object after other object</source>
      <translation>Déplacer après un autre objet</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="827"/>
      <source>Moves the selected object and insert it after another object</source>
      <translation>Déplace l’objet sélectionné et l’insère après un autre objet</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="524"/>
      <source>Set tip</source>
      <translation>Désigner comme fonction résultante</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="525"/>
      <source>Move the tip of the body</source>
      <translation>Déplacer la fonction résultante du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2514"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2515"/>
      <source>Create MultiTransform</source>
      <translation>Transformation multiple</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2516"/>
      <source>Create a multitransform feature</source>
      <translation>Créer une fonction de transformation multiple</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="485"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="486"/>
      <source>Create sketch</source>
      <translation>Créer une esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="487"/>
      <source>Create a new sketch</source>
      <translation>Créer une nouvelle esquisse</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1305"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1306"/>
      <source>Pad</source>
      <translation>Protrusion</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1307"/>
      <source>Pad a selected sketch</source>
      <translation>Faire une protrusion à partir de l'esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="162"/>
      <source>Create a datum plane</source>
      <translation>Créer un plan de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>Create a new datum plane</source>
      <translation>Créer un nouveau plan de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1337"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1338"/>
      <source>Pocket</source>
      <translation>Cavité</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1339"/>
      <source>Create a pocket with the selected sketch</source>
      <translation>Créer une cavité à partir de l’esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="217"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Create a datum point</source>
      <translation>Créez un point de référence</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>Create a new datum point</source>
      <translation>Créez un nouveau point de référence</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2400"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2401"/>
      <source>PolarPattern</source>
      <translation>Répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>Create a polar pattern feature</source>
      <translation>Créer une fonction de répétition circulaire</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1416"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1417"/>
      <source>Revolution</source>
      <translation>Révolution</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1418"/>
      <source>Revolve a selected sketch</source>
      <translation>Révolution d'une esquisse sélectionnée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2465"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2466"/>
      <source>Scaled</source>
      <translation>Mise à l'échelle</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2467"/>
      <source>Create a scaled feature</source>
      <translation>Créer une fonction de mise à l'échelle</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="277"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>Create a shape binder</source>
      <translation>Créer une forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>Create a new shape binder</source>
      <translation>Créer une nouvelle forme liée</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="343"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="344"/>
      <location filename="../../Command.cpp" line="345"/>
      <source>Create a sub-object(s) shape binder</source>
      <translation>Créer une forme liée du sous-objet(s)</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1823"/>
      <source>Subtractive helix</source>
      <translation>Hélice soustractive</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1824"/>
      <source>Sweep a selected sketch along a helix and remove it from the body</source>
      <translation>Balayer une esquisse sélectionnée le long d'une hélice et la soustraire au corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1695"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1696"/>
      <source>Subtractive loft</source>
      <translation>Enlèvement de matière par lissage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1697"/>
      <source>Loft a selected profile through other profile sections and remove it from the body</source>
      <translation>Lisser un profil sélectionné à travers d'autres sections de profil et soustraire du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1593"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1594"/>
      <source>Subtractive pipe</source>
      <translation>Enlèvement de matière par balayage</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1595"/>
      <source>Sweep a selected sketch along a path or to other profiles and remove it from the body</source>
      <translation>Balayer une esquisse sélectionnée sur un chemin et à travers d’autres profils et soustraire du corps</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2125"/>
      <source>PartDesign</source>
      <translation>PartDesign</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2126"/>
      <source>Thickness</source>
      <translation>Coque</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2127"/>
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
      <translation>Cale additive</translation>
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
      <translation>Cale soustractive</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="298"/>
      <source>Edit ShapeBinder</source>
      <translation>Modifier la forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Create ShapeBinder</source>
      <translation>Créer une forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="390"/>
      <source>Create SubShapeBinder</source>
      <translation>Créer une sous-forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="445"/>
      <source>Create Clone</source>
      <translation>Créer un clone</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="642"/>
      <location filename="../../Command.cpp" line="1205"/>
      <source>Make copy</source>
      <translation>Faire une copie</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="666"/>
      <source>Create a Sketch on Face</source>
      <translation>Créer une esquisse sur une face</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="687"/>
      <source>Create a new Sketch</source>
      <translation>Créer une nouvelle esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2564"/>
      <source>Convert to MultiTransform feature</source>
      <translation>Convertir en fonction de transformation multiple</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2656"/>
      <source>Create Boolean</source>
      <translation>Créer un booléen</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="104"/>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>Add a Body</source>
      <translation>Ajouter un corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="426"/>
      <source>Migrate legacy part design features to Bodies</source>
      <translation>Migrer les fonctions de conception de pièces existantes vers les corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="571"/>
      <source>Move tip to selected feature</source>
      <translation>Déplacer l'astuce vers l'élément sélectionné</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Duplicate a PartDesign object</source>
      <translation>Dupliquer un objet PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="739"/>
      <source>Move an object</source>
      <translation>Déplacer un objet</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="890"/>
      <source>Move an object inside tree</source>
      <translation>Déplacer un objet dans l'arborescence</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="292"/>
      <source>Mirrored</source>
      <translation>Symétrie</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="320"/>
      <source>Make LinearPattern</source>
      <translation>Répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="358"/>
      <source>PolarPattern</source>
      <translation>Répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
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
      <translation>Paramètres de la développante</translation>
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
  </context>
  <context>
    <name>PartDesign::Groove</name>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>La fonction demandée ne peut pas être créée. La raison est peut-être que :
  \xe2\x80\xa2 le corps actif ne contient pas de forme de base et donc il n’y a aucune matière
  à enlever ;
  \xe2\x80\xa2 l’esquisse sélectionnée n’appartient pas au corps actif.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::Hole</name>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1646"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>La fonction demandée ne peut pas être créée. La raison est peut-être que :
  \xe2\x80\xa2 le corps actif ne contient pas de forme de base et donc il n’y a aucune matière
  à enlever ;
  \xe2\x80\xa2 l’esquisse sélectionnée n’appartient pas au corps actif.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::Pocket</name>
    <message>
      <location filename="../../../App/FeaturePocket.cpp" line="101"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>La fonction demandée ne peut pas être créée. La raison est peut-être que :
  \xe2\x80\xa2 le corps actif ne contient pas de forme de base et donc il n’y a aucune matière
  à enlever ;
  \xe2\x80\xa2 l’esquisse sélectionnée n’appartient pas au corps actif.</translation>
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
      <translation>Pour créer un nouvel objet PartDesign, il doit y avoir un corps actif dans le document.

Veuillez sélectionner un corps à partir du bas, ou créer un nouveau corps.</translation>
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
      <translation>Largeur :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height:</source>
      <translation>Hauteur :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
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
      <translation>Rayon 1 :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2:</source>
      <translation>Rayon 2 :</translation>
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
      <translation>Paramètres V :</translation>
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
      <translation>X min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max:</source>
      <translation>Y min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max:</source>
      <translation>Z min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max:</source>
      <translation>X2 min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max:</source>
      <translation>Z2 min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch:</source>
      <translation>Axe de tangage :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system:</source>
      <translation>Système de coordonnées:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>Main droite</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Main gauche</translation>
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
      <translation>Angle 1:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2:</source>
      <translation>Angle 2:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From three points</source>
      <translation>A partir de trois points</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius:</source>
      <translation>Rayon majeur:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius:</source>
      <translation>Rayon mineur:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X:</source>
      <translation>X :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y:</source>
      <translation>Y :</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z:</source>
      <translation>Z :</translation>
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
      <location filename="../../TaskBooleanParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Enlever</translation>
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
      <location filename="../../TaskPrimitiveParameters.cpp" line="718"/>
      <source>Cone radii are equal</source>
      <translation>Les rayon du cône sont égaux</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="719"/>
      <source>The radii for cones must not be equal!</source>
      <translation>Les rayons des cônes ne doivent pas être égaux !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="794"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="799"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="804"/>
      <source>Invalid wedge parameters</source>
      <translation>Paramètres de la pyramide tronquée non valides</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="795"/>
      <source>X min must not be equal to X max!</source>
      <translation>X min ne doit pas être égal à X max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="800"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y min ne doit pas être égal à Y max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="805"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z min ne doit pas être égal à Z max !</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="843"/>
      <source>Create primitive</source>
      <translation>Créer une primitive</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="22"/>
      <location filename="../../TaskChamferParameters.ui" line="36"/>
      <location filename="../../TaskChamferParameters.cpp" line="170"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="26"/>
      <source>Add</source>
      <translation>Ajouter</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="40"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les chanfreins</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="67"/>
      <source>Type</source>
      <translation>Type</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="75"/>
      <source>Equal distance</source>
      <translation>Cote égale</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="80"/>
      <source>Two distances</source>
      <translation>Deux cotes</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="85"/>
      <source>Distance and angle</source>
      <translation>Cote et angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="98"/>
      <source>Flip direction</source>
      <translation>Inverser la direction</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Size</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="138"/>
      <source>Use All Edges</source>
      <translation>Utiliser tous les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="165"/>
      <source>Size 2</source>
      <translation>Taille 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="198"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="180"/>
      <location filename="../../TaskChamferParameters.cpp" line="182"/>
      <location filename="../../TaskChamferParameters.cpp" line="257"/>
      <location filename="../../TaskChamferParameters.cpp" line="259"/>
      <source>There must be at least one item</source>
      <translation>Il doit y avoir au moins un élément</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="225"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="225"/>
      <source>At least one item must be kept.</source>
      <translation>Au moins un élément doit être conservé.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="352"/>
      <source>Empty body list</source>
      <translation>Liste de corps vide</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="353"/>
      <source>The body list cannot be empty</source>
      <translation>La liste de corps ne peut pas être vide</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="364"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Booléen : Accepter : erreur d'entrée</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="104"/>
      <source>Incompatible reference set</source>
      <translation>Combinaison de références incompatible</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Il n'y a aucun mode d'accrochage qui corresponde à la combinaison de références actuelle. Si vous continuez, la fonction restera là où elle est et ne bougera pas si les références changent. Continuer?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="133"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="331"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="22"/>
      <location filename="../../TaskDraftParameters.ui" line="36"/>
      <location filename="../../TaskDraftParameters.cpp" line="137"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="26"/>
      <source>Add face</source>
      <translation>Ajouter une face</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="40"/>
      <source>Remove face</source>
      <translation>Supprimer la face</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les dépouilles</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="65"/>
      <source>Draft angle</source>
      <translation>Angle de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="98"/>
      <source>Neutral plane</source>
      <translation>Plan neutre</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="115"/>
      <source>Pull direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="130"/>
      <source>Reverse pull direction</source>
      <translation>Inverser la direction de dépouille</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="147"/>
      <location filename="../../TaskDraftParameters.cpp" line="149"/>
      <location filename="../../TaskDraftParameters.cpp" line="273"/>
      <location filename="../../TaskDraftParameters.cpp" line="275"/>
      <source>There must be at least one item</source>
      <translation>Il doit y avoir au moins un élément</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="241"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="241"/>
      <source>At least one item must be kept.</source>
      <translation>Au moins un élément doit être conservé.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="276"/>
      <source>Add all edges</source>
      <translation>Ajouter toutes les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="284"/>
      <source>Adds all edges to the list box (active only when in add selection mode).</source>
      <translation>Ajoute toutes les arêtes à la liste (actif uniquement en mode sélection ajoutée).</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <location filename="../../TaskDressUpParameters.cpp" line="305"/>
      <source>There must be at least one item</source>
      <translation>Il doit y avoir au moins un élément</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="53"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="723"/>
      <source>No face selected</source>
      <translation>Aucune face sélectionnée</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="156"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="737"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="360"/>
      <source>Sketch normal</source>
      <translation>Esquisse normale</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="362"/>
      <source>Face normal</source>
      <translation>Face normale</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="365"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="369"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="371"/>
      <source>Custom direction</source>
      <translation>Direction personnalisée </translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <location filename="../../TaskFilletParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="22"/>
      <location filename="../../TaskFilletParameters.ui" line="36"/>
      <location filename="../../TaskFilletParameters.cpp" line="124"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="26"/>
      <source>Add</source>
      <translation>Ajouter</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="40"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les congés</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="65"/>
      <source>Radius:</source>
      <translation>Rayon :</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="81"/>
      <source>Use All Edges</source>
      <translation>Utiliser tous les arêtes</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="134"/>
      <location filename="../../TaskFilletParameters.cpp" line="136"/>
      <location filename="../../TaskFilletParameters.cpp" line="211"/>
      <location filename="../../TaskFilletParameters.cpp" line="213"/>
      <source>There must be at least one item</source>
      <translation>Il doit y avoir au moins un élément</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="179"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="179"/>
      <source>At least one item must be kept.</source>
      <translation>Au moins un élément doit être conservé.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status:</source>
      <translation>État:</translation>
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
      <translation>Axe de tangage :</translation>
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
      <translation>Réactualiser la vue</translation>
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
      <location filename="../../TaskHelixParameters.cpp" line="333"/>
      <source>Error: unsupported mode</source>
      <translation>Erreur : mode non pris en charge</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="47"/>
      <source>Counterbore</source>
      <translation>Chambrage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="48"/>
      <source>Countersink</source>
      <translation>Fraisage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="49"/>
      <source>Cheesehead (deprecated)</source>
      <translation>Vis à tête cylindrique (dépréciée)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="50"/>
      <source>Countersink socket screw (deprecated)</source>
      <translation>Vis à tête fraisée (dépréciée)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="51"/>
      <source>Cap screw (deprecated)</source>
      <translation>Vis à tête creuse (dépréciée)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Hole parameters</source>
      <translation>Paramètres de perçage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>None</source>
      <translation>Aucun</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="66"/>
      <source>ISO metric regular profile</source>
      <translation>Filetage métrique ISO à pas standard</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>ISO metric fine profile</source>
      <translation>Filetage métrique ISO à pas fin</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>UTS coarse profile</source>
      <translation>Filetage unifié (UTS) grossier</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="69"/>
      <source>UTS fine profile</source>
      <translation>Filetage unifié (UTS) fin</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>UTS extra fine profile</source>
      <translation>Filetage unifié (UTS) extra fin</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLinearPatternParameters</name>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <source>Length</source>
      <translation>Longueur</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="101"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="115"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="124"/>
      <source>Update view</source>
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="105"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="333"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Profil</translation>
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
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft parameters</source>
      <translation>Paramètres de lissage</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="101"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="245"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="73"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="86"/>
      <source>Edit</source>
      <translation>Éditer</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="90"/>
      <source>Delete</source>
      <translation>Supprimer</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="94"/>
      <source>Add mirrored transformation</source>
      <translation>Ajouter une fonction de symétrie</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="98"/>
      <source>Add linear pattern</source>
      <translation>Ajouter une répétition linéaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Add polar pattern</source>
      <translation>Ajouter une répétition circulaire</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="106"/>
      <source>Add scaled transformation</source>
      <translation>Ajouter une transformation de mise à échelle</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="110"/>
      <source>Move up</source>
      <translation>Déplacer vers le haut</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="114"/>
      <source>Move down</source>
      <translation>Déplacer vers le bas</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="141"/>
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
      <translation>Inverse la direction du tampon</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To last</source>
      <translation>Au dernier</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To first</source>
      <translation>Au premier</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to face</source>
      <translation>Jusqu'à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Two dimensions</source>
      <translation>Deux dimensions</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <location filename="../../TaskPadPocketParameters.ui" line="58"/>
      <source>Offset to face</source>
      <translation>Décalage par rapport à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="77"/>
      <source>Direction</source>
      <translation>Direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="85"/>
      <source>Direction/edge:</source>
      <translation>Itinéraire / arête :</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="92"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Définir une direction ou sélectionner une arête
du modèle comme référence</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="97"/>
      <source>Sketch normal</source>
      <translation>Esquisse normale</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="102"/>
      <source>Select reference...</source>
      <translation>Sélectionnez une référence...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="107"/>
      <source>Custom direction</source>
      <translation>Direction personnalisée </translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="117"/>
      <source>Show direction</source>
      <translation>Afficher la direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="127"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Utiliser un vecteur personnalisé pour la direction de la protrusion sinon
le vecteur normal du plan de l'esquisse sera utilisé</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="140"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="147"/>
      <source>x-component of direction vector</source>
      <translation>composante x du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="169"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="176"/>
      <source>y-component of direction vector</source>
      <translation>composante y du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="198"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="205"/>
      <source>z-component of direction vector</source>
      <translation>composante z du vecteur de direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="236"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Si non cochée, la longueur sera
mesurée dans la direction spécifiée</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="240"/>
      <source>Length along sketch normal</source>
      <translation>Longueur le long de la normale à l'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="253"/>
      <source>Applies length symmetrically to sketch plane</source>
      <translation>Applique une longueur symétriquement au plan d'esquisse</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="256"/>
      <source>Symmetric to plane</source>
      <translation>Symétrique au plan</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="263"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="272"/>
      <location filename="../../TaskPadPocketParameters.ui" line="317"/>
      <source>Angle to taper the extrusion</source>
      <translation>Angle pour effiler l'extrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="275"/>
      <source>Taper angle</source>
      <translation>Angle de conicité</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="296"/>
      <source>2nd length</source>
      <translation>2ième longueur</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="320"/>
      <source>2nd taper angle</source>
      <translation>2ème angle de conicité</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="341"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="360"/>
      <source>Update view</source>
      <translation>Réactualiser la vue</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Profil</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="541"/>
      <source>Section orientation</source>
      <translation>Orientation de la section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="567"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Profil</translation>
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
      <translation>Chemin le long duquel effectuer le balayage</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe parameters</source>
      <translation>Paramètres de balayage</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="85"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="425"/>
      <location filename="../../TaskPipeParameters.cpp" line="527"/>
      <source>Input error</source>
      <translation>Erreur de saisie</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="425"/>
      <source>No active body</source>
      <translation>Aucun corps actif</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <location filename="../../TaskPipeParameters.cpp" line="809"/>
      <source>Section transformation</source>
      <translation>Transformation de section</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="825"/>
      <source>Remove</source>
      <translation>Enlever</translation>
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
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Dimension</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Through all</source>
      <translation>À travers tout</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>Au premier</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Jusqu'à la face</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Two dimensions</source>
      <translation>Deux dimensions</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPolarPatternParameters</name>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="107"/>
      <source>Occurrences</source>
      <translation>Occurrences</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="121"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="130"/>
      <source>Update view</source>
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="112"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="333"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="893"/>
      <source>Attachment</source>
      <translation>Accrochage</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Axis:</source>
      <translation>Axe :</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="139"/>
      <source>Base X axis</source>
      <translation>Axe X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="35"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="140"/>
      <source>Base Y axis</source>
      <translation>Axe Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="40"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="141"/>
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
      <location filename="../../TaskRevolutionParameters.cpp" line="148"/>
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
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="49"/>
      <source>Revolution parameters</source>
      <translation>Paramètres de la révolution</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Réactualiser la vue</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.cpp" line="92"/>
      <source>Remove</source>
      <translation>Enlever</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Datum shape parameters</source>
      <translation>Paramètres de la forme liée</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="160"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="22"/>
      <location filename="../../TaskThicknessParameters.ui" line="36"/>
      <location filename="../../TaskThicknessParameters.cpp" line="136"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Cliquez sur le bouton pour passer en mode sélection, cliquez à nouveau pour terminer la sélection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="26"/>
      <source>Add face</source>
      <translation>Ajouter une face</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="40"/>
      <source>Remove face</source>
      <translation>Supprimer la face</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- sélectionnez un élément pour le mettre en surbrillance
- double-cliquez sur un élément pour voir les fonctions</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="65"/>
      <source>Thickness</source>
      <translation>Épaisseur</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="94"/>
      <source>Mode</source>
      <translation>Mode</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="102"/>
      <source>Skin</source>
      <translation>Couche</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="107"/>
      <source>Pipe</source>
      <translation>Tuyau</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="112"/>
      <source>Recto Verso</source>
      <translation>Recto-verso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="120"/>
      <source>Join Type</source>
      <translation>Type de raccordement</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="128"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="133"/>
      <location filename="../../TaskThicknessParameters.ui" line="143"/>
      <source>Intersection</source>
      <translation>Intersection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="150"/>
      <source>Make thickness inwards</source>
      <translation>Générer l'épaisseur vers l'intérieur</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="146"/>
      <location filename="../../TaskThicknessParameters.cpp" line="148"/>
      <location filename="../../TaskThicknessParameters.cpp" line="213"/>
      <location filename="../../TaskThicknessParameters.cpp" line="215"/>
      <source>There must be at least one item</source>
      <translation>Il doit y avoir au moins un élément</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="181"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="181"/>
      <source>At least one item must be kept.</source>
      <translation>Au moins un élément doit être conservé.</translation>
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
      <location filename="../../ViewProviderBody.cpp" line="133"/>
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
      <translation>Paramètres de la ligne de référence</translation>
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
      <location filename="../../ViewProviderLinearPattern.h" line="38"/>
      <source>LinearPattern parameters</source>
      <translation>Paramètres de la répétition linéaire</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="38"/>
      <source>MultiTransform parameters</source>
      <translation>Paramètres de la transformation multiple</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="38"/>
      <source>PolarPattern parameters</source>
      <translation>Paramètres de la répétition circulaire</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="38"/>
      <source>Scaled parameters</source>
      <translation>Paramètres de la mise à l'échelle</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness parameters</source>
      <translation>Paramètres de la coque</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="38"/>
      <source>Mirrored parameters</source>
      <translation>Paramètres de la symétrie</translation>
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
      <translation>Créer une cale additive</translation>
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
      <translation>Créer une cale soustractive</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="57"/>
      <source>Involute gear...</source>
      <translation>Engrenage à développante...</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="61"/>
      <source>Creates or edit the involute gear definition.</source>
      <translation>Crée ou édite la définition de l'engrenage.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="728"/>
      <source>Select body</source>
      <translation>Sélectionner le corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="729"/>
      <source>Select a body from the list</source>
      <translation>Sélectionner un corps dans la liste</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="881"/>
      <source>Select feature</source>
      <translation>Sélectionner une fonction</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="882"/>
      <source>Select a feature from the list</source>
      <translation>Sélectionner une fonction dans la liste</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="953"/>
      <source>Move tip</source>
      <translation>Déplacer l'astuce</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="954"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>La fonction déplacée apparaît après la fonction résultante en cours.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="955"/>
      <source>Do you want the last feature to be the new tip?</source>
      <translation>Voulez-vous que la dernière fonction soit la nouvelle fonction résultante?</translation>
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
      <translation>Crée ou édite la définition du pignon.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>Invalid selection</source>
      <translation>Sélection non valide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Il n’y a aucun mode d'accrochage qui convienne aux objets sélectionnés. Sélectionnez autre chose.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <location filename="../../Command.cpp" line="147"/>
      <location filename="../../Command.cpp" line="149"/>
      <source>Error</source>
      <translation>Erreur</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <source>There is no active body. Please make a body active before inserting a datum entity.</source>
      <translation>Pas de corps actif. Activez un corps avant d'insérer une référence.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="408"/>
      <source>Sub-Shape Binder</source>
      <translation>Sous forme liée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="590"/>
      <source>Several sub-elements selected</source>
      <translation>Plusieurs sous-éléments sélectionnés</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="591"/>
      <source>You have to select a single face as support for a sketch!</source>
      <translation>Vous devez sélectionner une seule face comme support pour une esquisse !</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="601"/>
      <source>No support face selected</source>
      <translation>Aucune face de support sélectionnée</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="602"/>
      <source>You have to select a face as support for a sketch!</source>
      <translation>Vous devez sélectionner un plan ou une face plane comme support de l'esquisse !</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="611"/>
      <source>No planar support</source>
      <translation>Aucun plan de support</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="612"/>
      <source>You need a planar face as support for a sketch!</source>
      <translation>Vous avez besoin d'un plan ou d'une face plane comme support de l'esquisse !</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="797"/>
      <source>No valid planes in this document</source>
      <translation>Pas de plan valide dans le document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="798"/>
      <source>Please create a plane first or select a face to sketch on</source>
      <translation>Veuillez d'abord créer un plan, ou choisissez une face sur laquelle appliquer l'esquisse</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="811"/>
      <location filename="../../Command.cpp" line="1226"/>
      <location filename="../../Command.cpp" line="2225"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="73"/>
      <location filename="../../ViewProviderDatum.cpp" line="246"/>
      <location filename="../../ViewProvider.cpp" line="98"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="98"/>
      <location filename="../../ViewProviderHole.cpp" line="79"/>
      <location filename="../../ViewProviderBoolean.cpp" line="80"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Une boîte de dialogue est déjà ouverte dans le panneau des tâches</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="812"/>
      <location filename="../../Command.cpp" line="1227"/>
      <location filename="../../Command.cpp" line="2226"/>
      <location filename="../../ViewProviderPrimitive.cpp" line="74"/>
      <location filename="../../ViewProviderDatum.cpp" line="247"/>
      <location filename="../../ViewProvider.cpp" line="99"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="99"/>
      <location filename="../../ViewProviderHole.cpp" line="80"/>
      <location filename="../../ViewProviderBoolean.cpp" line="81"/>
      <source>Do you want to close this dialog?</source>
      <translation>Voulez-vous fermer cette boîte de dialogue?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1106"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>Impossible d'utiliser cette commande car il n'y a pas de solide à soustraire.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1107"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Assurez-vous que le corps contient une fonctionnalité avant de tenter une commande soustractive.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1128"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Impossible d'utiliser l'objet sélectionné. L'objet sélectionné doit appartenir au corps actif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1129"/>
      <source>Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</source>
      <translation>Envisagez d'utiliser une forme liée ou une fonction de base pour référencer une géométrie externe dans un corps.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1151"/>
      <source>No sketch to work on</source>
      <translation>Pas d'esquisse de travail</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1152"/>
      <source>No sketch is available in the document</source>
      <translation>Aucune esquisse n’est disponible dans le document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1890"/>
      <location filename="../../Command.cpp" line="1894"/>
      <location filename="../../Command.cpp" line="1920"/>
      <location filename="../../Command.cpp" line="1950"/>
      <source>Wrong selection</source>
      <translation>Sélection invalide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1891"/>
      <source>Select an edge, face, or body.</source>
      <translation>Sélectionnez une arête, une face ou un corps.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1895"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Sélectionnez une arête, une face ou un corps d'un corps unique.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1899"/>
      <location filename="../../Command.cpp" line="2252"/>
      <source>Selection is not in Active Body</source>
      <translation>La sélection n’est pas dans le corps actif</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1900"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Sélectionnez une arête, une face ou un corps d'un corps actif.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1910"/>
      <source>Wrong object type</source>
      <translation>Type d'objet incorrect</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1911"/>
      <source>%1 works only on parts.</source>
      <translation>%1 fonctionne uniquement sur les pièces.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1921"/>
      <source>Shape of the selected Part is empty</source>
      <translation>La forme de la pièce sélectionnée est vide</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1951"/>
      <source> not possible on selected faces/edges.</source>
      <translation> pas possible sur les faces/arêtes sélectionnées.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2243"/>
      <source>No valid features in this document</source>
      <translation>Aucune fonction valide dans ce document</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2244"/>
      <source>Please create a feature first.</source>
      <translation>Veuillez d'abord créer une fonction.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2253"/>
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
      <location filename="../../CommandBody.cpp" line="339"/>
      <source>Nothing to migrate</source>
      <translation>Rien à migrer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="340"/>
      <source>No PartDesign features found that don't belong to a body. Nothing to migrate.</source>
      <translation>Aucune fonction PartDesign trouvée qui n'appartienne pas à un corps. Rien à migrer.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="488"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Le plan d'esquisse ne peut pas être migré</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="489"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Veuillez éditer '%1' et définir son plan selon un plan de base ou de référence.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="551"/>
      <location filename="../../CommandBody.cpp" line="555"/>
      <location filename="../../CommandBody.cpp" line="560"/>
      <location filename="../../CommandBody.cpp" line="853"/>
      <location filename="../../CommandBody.cpp" line="860"/>
      <source>Selection error</source>
      <translation>Erreur de sélection</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="552"/>
      <source>Select exactly one PartDesign feature or a body.</source>
      <translation>Sélectionnez seulement une fonction ou un corps PartDesign.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="556"/>
      <source>Couldn't determine a body for the selected feature '%s'.</source>
      <translation>Impossible de déterminer un corps pour la fonction '%s' sélectionnée.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="561"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Seule une fonction solide peut être la fonction résultante d'un corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="679"/>
      <location filename="../../CommandBody.cpp" line="701"/>
      <location filename="../../CommandBody.cpp" line="716"/>
      <source>Features cannot be moved</source>
      <translation>Impossible de déplacer les fonctions</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="680"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Certaines des fonctions sélectionnées ont des dépendances dans le corps source</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="702"/>
      <source>Only features of a single source Body can be moved</source>
      <translation>Seules les fonctions d'un seul corps source peuvent être déplacées</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="717"/>
      <source>There are no other bodies to move to</source>
      <translation>Il n'y a aucun autre corps vers lequel déplacer</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="854"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Impossible de déplacer la fonction de base d’un corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="861"/>
      <source>Select one or more features from the same body.</source>
      <translation>Sélectionnez une ou plusieurs fonctions dans le même corps.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="874"/>
      <source>Beginning of the body</source>
      <translation>Début du corps</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="939"/>
      <source>Dependency violation</source>
      <translation>Violation de dépendance</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="940"/>
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
      <location filename="../../TaskTransformedParameters.cpp" line="287"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="124"/>
      <source>Vertical sketch axis</source>
      <translation>Axe d'esquisse vertical</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="288"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="125"/>
      <source>Horizontal sketch axis</source>
      <translation>Axe d'esquisse horizontal</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="127"/>
      <source>Construction line %1</source>
      <translation>Ligne de construction %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="76"/>
      <source>Face</source>
      <translation>Face</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="122"/>
      <source>In order to use PartDesign you need an active Body object in the document. Please make one active (double click) or create one.

If you have a legacy document with PartDesign objects without Body, use the migrate function in PartDesign to put them into a Body.</source>
      <translation>Pour utiliser PartDesign, vous avez besoin d'un corps actif dans le document. Veuillez en activer un (par double-clic) ou en créer un.

Si vous avez un vieux document avec des objets PartDesign sans corps, utilisez la fonction de migration sous PartDesign pour les inclure dans un corps.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="169"/>
      <source>Active Body Required</source>
      <translation>Corps actif requis</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="170"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document. Please make one active (double click) or create a new Body.</source>
      <translation>Pour créer un nouvel objet de PartDesign, il doit y avoir un corps actif dans le document. Veuillez en activer un (par double clic) ou créer un nouveau corps.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="205"/>
      <source>Feature is not in a body</source>
      <translation>La fonction ne fait pas partie d'un corps</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Pour utiliser cette fonction, elle doit appartenir à un corps dans le document.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="238"/>
      <source>Feature is not in a part</source>
      <translation>La fonction ne fait pas partie d'une pièce</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="239"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Afin d'utiliser cette fonction, elle doit appartenir à un objet pièce dans le document.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="50"/>
      <location filename="../../ViewProvider.cpp" line="65"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="204"/>
      <location filename="../../ViewProviderTransformed.cpp" line="65"/>
      <location filename="../../ViewProviderMultiTransform.cpp" line="42"/>
      <source>Edit %1</source>
      <translation>Modifier %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="78"/>
      <source>Set colors...</source>
      <translation>Définir les couleurs...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="64"/>
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
      <location filename="../../ViewProviderDressUp.cpp" line="71"/>
      <source>Feature error</source>
      <translation>Erreur de la fonction</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="72"/>
      <source>%1 misses a base feature.
This feature is broken and can't be edited.</source>
      <translation>%1 est sans fonction de base.
Cette fonction est cassée et ne peut pas être modifiée.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="48"/>
      <source>Edit groove</source>
      <translation>Modifier la rainure</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit hole</source>
      <translation>Modifier le perçage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="70"/>
      <source>Edit loft</source>
      <translation>Modifier le lissage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="48"/>
      <source>Edit pad</source>
      <translation>Modifier la protrusion</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="78"/>
      <source>Edit pipe</source>
      <translation>Modifier le balayage</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="50"/>
      <source>Edit pocket</source>
      <translation>Modifier la cavité</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="59"/>
      <source>Edit primitive</source>
      <translation>Modifier la primitive</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="48"/>
      <source>Edit revolution</source>
      <translation>Modifier la révolution</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="199"/>
      <source>Edit shape binder</source>
      <translation>Modifier la forme liée</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="308"/>
      <source>Synchronize</source>
      <translation>Synchroniser</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="310"/>
      <source>Select bound object</source>
      <translation>Sélectionnez l'objet lié</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="169"/>
      <source>One transformed shape does not intersect support</source>
      <translation>Une des formes transformées n'intersecte pas le support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="171"/>
      <source>%1 transformed shapes do not intersect support</source>
      <translation>%1 des formes transformées n'intersectent pas le support</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="181"/>
      <source>Transformation succeeded</source>
      <translation>Transformation réussie</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="138"/>
      <source>The document "%1" you are editing was designed with an old version of PartDesign workbench.</source>
      <translation>Le document « %1 » que vous modifiez a été conçu dans une vieille version de l'atelier PartDesign.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="141"/>
      <source>Do you want to migrate in order to use modern PartDesign features?</source>
      <translation>Voulez-vous appliquer la migration afin d'utiliser les fonctions modernes PartDesign ?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="144"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy PartDesign or have a slightly broken structure.</source>
      <translation>Le document « %1 » semble être au milieu du processus de migration de PartDesign ou a une structure légèrement cassée.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="148"/>
      <source>Do you want to make the migration automatically?</source>
      <translation>Voulez-vous faire la migration automatiquement ?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Remarque : si vous choisissez de migrer, vous ne serez pas en mesure d'éditer le fichier avec d’anciennes versions de FreeCAD. Si vous refusez de migrer, vous ne serez pas en mesure d’utiliser les nouvelles fonctionnalités de PartDesign comme les corps et les pièces. Pour résultat, vous ne serez pas en mesure d’utiliser vos pièces dans l'atelier assemblage. Il vous sera néanmoins possible de migrer à tout moment, plus tard, via « Part Design -&gt; Migrer ».</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="159"/>
      <source>Migrate manually</source>
      <translation>Migrer manuellement</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="55"/>
      <source>Edit helix</source>
      <translation>Modifier l'hélice</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket parameter</source>
      <translation>Paramètre du pignon</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>Nombre de dents :</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="40"/>
      <source>Sprocket Reference</source>
      <translation>Référence du pignon</translation>
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
      <location filename="../../../FeatureHole/TaskHole.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
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
      <translation>Chambrage</translation>
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
      <translation>Profil</translation>
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
      <translation>Réactualiser la vue</translation>
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
      <translation>Jeu d'ajustement</translation>
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
      <location filename="../../TaskHoleParameters.cpp" line="586"/>
      <source>Standard</source>
      <translation>Standard</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="256"/>
      <location filename="../../TaskHoleParameters.cpp" line="587"/>
      <location filename="../../TaskHoleParameters.cpp" line="598"/>
      <source>Close</source>
      <translation>Fermer</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="261"/>
      <location filename="../../TaskHoleParameters.cpp" line="588"/>
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
      <location filename="../../TaskHoleParameters.ui" line="609"/>
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
      <location filename="../../TaskHoleParameters.ui" line="561"/>
      <source>Countersink angle</source>
      <translation>Angle de fraisage</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="593"/>
      <source>&lt;b&gt;Drill point&lt;/b&gt;</source>
      <translation>&lt;b&gt;Pointe de perçage&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="625"/>
      <source>Flat</source>
      <translation>Plat</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="641"/>
      <source>Angled</source>
      <translation>En angle</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="676"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>La taille de la pointe du foret sera prise en compte
compte pour la profondeur des trous borgnes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="680"/>
      <source>Take into account for depth</source>
      <translation>Tenir compte de la profondeur</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="687"/>
      <source>&lt;b&gt;Misc&lt;/b&gt;</source>
      <translation>&lt;b&gt;Divers&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="700"/>
      <source>Tapered</source>
      <translation>Conique</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="713"/>
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
      <location filename="../../TaskHoleParameters.ui" line="738"/>
      <source>Reverses the hole direction</source>
      <translation>Inverse la direction du trou</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="741"/>
      <source>Reversed</source>
      <translation>Inversé</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="597"/>
      <source>Normal</source>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="599"/>
      <source>Loose</source>
      <translation>Détachés</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="14"/>
      <source>Form</source>
      <translation>Forme</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Aucun message</translation>
    </message>
  </context>
  <context>
    <name>WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="186"/>
      <location filename="../../../WizardShaft/WizardShaft.py" line="211"/>
      <source>Shaft design wizard...</source>
      <translation>Assistant conception d'arbre...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="187"/>
      <location filename="../../../WizardShaft/WizardShaft.py" line="212"/>
      <source>Start the shaft design wizard</source>
      <translation>Démarrer l'assistant de conception d'arbre</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="43"/>
      <source>Length [mm]</source>
      <translation>Longueur [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="44"/>
      <source>Diameter [mm]</source>
      <translation>Diamètre [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Inner diameter [mm]</source>
      <translation>Diamètre intérieur [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Constraint type</source>
      <translation>Type de contrainte</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Start edge type</source>
      <translation>Type d'arête de départ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Start edge size</source>
      <translation>Taille d'arête de départ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>End edge type</source>
      <translation>Type d'arête de fin</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>Taille d'arête de fin</translation>
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
      <translation>Assistant de conception d'arbre</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Measure</source>
      <translation>Mesure</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Refresh</source>
      <translation>Actualiser</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Toggle 3D</source>
      <translation>Basculer en 3D</translation>
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
</TS>
