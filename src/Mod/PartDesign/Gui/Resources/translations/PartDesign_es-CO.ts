<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es-CO" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="84"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Punto central del inicio de la hélice; se deriva del eje de referencia.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Dirección de la hélice; se deriva del eje de referencia.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="101"/>
      <source>The reference axis of the helix.</source>
      <translation>Eje de referencia de la hélice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="108"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>El modo de entrada de la hélice indica qué propiedades define el usuario.
Las propiedades dependientes se calculan a partir de ellas.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="120"/>
      <source>The axial distance between two turns.</source>
      <translation>Distancia axial entre dos vueltas.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="127"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Altura de la trayectoria de la hélice, sin contar la extensión del perfil.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="137"/>
      <source>The number of turns in the helix.</source>
      <translation>Número de vueltas de la hélice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="145"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>Ángulo del cono que envuelve la hélice.
Los valores distintos de cero convierten la hélice en una espiral cónica.
Los valores positivos hacen crecer el radio; los negativos, lo reducen.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="158"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Crecimiento del radio de la hélice por vuelta.
Los valores distintos de cero convierten la hélice en una espiral cónica.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="169"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Define el sentido de giro a izquierda,
es decir, antihorario al avanzar a lo largo de su eje.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="180"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Determina si la hélice apunta en la dirección opuesta al eje.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="210"/>
      <source>deprecated, do not use</source>
      <translation>obsoleto, no usar</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="190"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Si es falso, la herramienta propondrá un valor inicial del paso según la caja envolvente del perfil,
para evitar autointersecciones.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="202"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>Tolerancia de unión de la hélice; auméntela si la forma helicoidal no se une bien con la pieza.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>Number of gear teeth</source>
      <translation>Número de dientes del engranaje</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="120"/>
      <source>Pressure angle of gear teeth</source>
      <translation>Ángulo de presión de los dientes del engranaje</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="114"/>
      <source>Module of the gear</source>
      <translation>Módulo del engranaje</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="129"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True = 2 curvas de 3 puntos de control cada una; False = 1 curva de 4 puntos de control.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="137"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True = engranaje exterior; False = engranaje interior</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="146"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>Altura del diente desde la circunferencia primitiva hasta la cabeza, normalizada por el módulo.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="155"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>Altura del diente desde la circunferencia primitiva hasta el pie, normalizada por el módulo.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="164"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>Radio del redondeo en el pie del diente, normalizado por el módulo.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="173"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>Distancia que se desplaza hacia afuera el perfil de referencia, normalizada por el módulo.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1715"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1716"/>
      <source>Additive Helix</source>
      <translation>Hélice aditiva</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1717"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>Barre el croquis o perfil seleccionado a lo largo de una hélice y lo agrega al cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1617"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1618"/>
      <source>Additive Loft</source>
      <translation>Recubrimiento aditivo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1619"/>
      <source>Lofts the selected sketch or profile through one or more sections and adds it to the body</source>
      <translation>Crea un recubrimiento del croquis o perfil seleccionado a través de una o más secciones y lo agrega al cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1517"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1518"/>
      <source>Additive Pipe</source>
      <translation>Barrido aditivo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1519"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>Barre el croquis o perfil seleccionado a lo largo de una trayectoria y lo agrega al cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="109"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="110"/>
      <source>New Body</source>
      <translation>Cuerpo nuevo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="111"/>
      <source>Creates a new body and activates it</source>
      <translation>Crea un cuerpo nuevo y lo activa</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2860"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2861"/>
      <source>Boolean Operation</source>
      <translation>Operación booleana</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2862"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>Aplica operaciones booleanas entre los objetos seleccionados y el cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="306"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Local Coordinate System</source>
      <translation>Sistema de coordenadas local</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="308"/>
      <source>Creates a new local coordinate system</source>
      <translation>Crea un sistema de coordenadas local nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="2043"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2044"/>
      <source>Chamfer</source>
      <translation>Chaflán</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2045"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>Aplica un chaflán a las aristas o caras seleccionadas</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="522"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="523"/>
      <source>Clone</source>
      <translation>Clonar</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="524"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>Copia un objeto sólido de forma paramétrica como operación base de un cuerpo nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2160"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2161"/>
      <source>Draft</source>
      <translation>Ángulo de salida</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2162"/>
      <source>Applies a draft to the selected faces</source>
      <translation>Aplica un ángulo de salida (desmoldeo) a las caras seleccionadas</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicate &amp;Object</source>
      <translation>Duplicar &amp;objeto</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="758"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Duplica el objeto seleccionado y lo agrega al cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="2015"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>Fillet</source>
      <translation>Redondeo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>Aplica un redondeo a las aristas o caras seleccionadas</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1447"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1448"/>
      <source>Groove</source>
      <translation>Garganta</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1449"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>Gira el croquis o perfil alrededor de una línea o eje y lo quita del cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1340"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1341"/>
      <source>Hole</source>
      <translation>Agujero</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1343"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>Crea agujeros en el cuerpo activo en los centros de los círculos o arcos del croquis o perfil seleccionado</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Datum Line</source>
      <translation>Línea de referencia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="248"/>
      <source>Creates a new datum line</source>
      <translation>Crea una línea de referencia nueva</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2415"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2416"/>
      <source>Linear Pattern</source>
      <translation>Patrón lineal</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2417"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>Duplica las operaciones seleccionadas o el cuerpo activo en un patrón lineal</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="391"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="392"/>
      <source>Migrate</source>
      <translation>Migrar</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="393"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>Migra el documento al flujo de trabajo moderno de Part Design</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2358"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2359"/>
      <source>Mirror</source>
      <translation>Simetría</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2360"/>
      <source>Mirrors the selected features or active body</source>
      <translation>Aplica simetría a las operaciones seleccionadas o al cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="824"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="825"/>
      <source>Move Object To…</source>
      <translation>Mover objeto a…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="826"/>
      <source>Moves the selected object to another body</source>
      <translation>Mueve el objeto seleccionado a otro cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1021"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1022"/>
      <source>Move Feature After…</source>
      <translation>Mover operación después de…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1023"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>Mueve la operación seleccionada después de otra operación del mismo cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="657"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>Set Tip</source>
      <translation>Establecer punta</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>Mueve la punta del cuerpo a la operación seleccionada</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2729"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2730"/>
      <source>Multi-Transform</source>
      <translation>Transformación múltiple</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2731"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>Aplica varias transformaciones a las operaciones seleccionadas o al cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="612"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="613"/>
      <source>New Sketch</source>
      <translation>Croquis nuevo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="614"/>
      <source>Creates a new sketch</source>
      <translation>Crea un croquis nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1282"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1283"/>
      <source>Pad</source>
      <translation>Protrusión</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>Extruye el croquis o perfil seleccionado y lo agrega al cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="216"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="217"/>
      <source>Datum Plane</source>
      <translation>Plano de referencia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Creates a new datum plane</source>
      <translation>Crea un plano de referencia nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1311"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1312"/>
      <source>Pocket</source>
      <translation>Cajera</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1313"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>Extruye el croquis o perfil seleccionado y lo quita del cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="276"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="277"/>
      <source>Datum Point</source>
      <translation>Punto de referencia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>Creates a new datum point</source>
      <translation>Crea un punto de referencia nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2624"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2625"/>
      <source>Polar Pattern</source>
      <translation>Patrón polar</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2626"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>Duplica las operaciones seleccionadas o el cuerpo activo en un patrón circular</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1385"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1386"/>
      <source>Revolve</source>
      <translation>Revolución</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1387"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>Gira el croquis o perfil seleccionado alrededor de una línea o eje y lo agrega al cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2686"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2687"/>
      <source>Scale</source>
      <translation>Escalar</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2688"/>
      <source>Scales the selected features or the active body</source>
      <translation>Escala las operaciones seleccionadas o el cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="340"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="341"/>
      <source>Shape Binder</source>
      <translation>Vinculador de forma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="342"/>
      <source>Creates a new shape binder</source>
      <translation>Crea un vinculador de forma nuevo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="410"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="411"/>
      <source>Sub-Shape Binder</source>
      <translation>Vinculador de subforma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="412"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>Crea una referencia a la geometría de uno o más objetos, que se puede usar dentro o fuera de un cuerpo. Sigue los posicionamientos relativos, admite varios tipos de geometría (sólidos, caras, aristas, vértices) y puede trabajar con objetos del mismo documento o de documentos externos.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1799"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1800"/>
      <source>Subtractive Helix</source>
      <translation>Hélice sustractiva</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1801"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>Barre el croquis o perfil seleccionado a lo largo de una hélice y lo quita del cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1667"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1668"/>
      <source>Subtractive Loft</source>
      <translation>Recubrimiento sustractivo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1670"/>
      <source>Lofts the selected sketch or profile through one or more sections and removes it from the body</source>
      <translation>Crea un recubrimiento del croquis o perfil seleccionado a través de una o más secciones y lo quita del cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1567"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1568"/>
      <source>Subtractive Pipe</source>
      <translation>Barrido sustractivo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1569"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>Barre el croquis o perfil seleccionado a lo largo de una trayectoria y lo quita del cuerpo</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2230"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2231"/>
      <source>Thickness</source>
      <translation>Espesor</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2232"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>Aplica un espesor y quita las caras seleccionadas</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="77"/>
      <source>Additive Primitive</source>
      <translation>Primitiva aditiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="78"/>
      <source>Creates an additive primitive</source>
      <translation>Crea una primitiva aditiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Box</source>
      <translation>Caja aditiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Additive Cylinder</source>
      <translation>Cilindro aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Additive Sphere</source>
      <translation>Esfera aditiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Additive Cone</source>
      <translation>Cono aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Additive Ellipsoid</source>
      <translation>Elipsoide aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Additive Torus</source>
      <translation>Toroide aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Additive Prism</source>
      <translation>Prisma aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Additive Wedge</source>
      <translation>Cuña aditiva</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="285"/>
      <source>Subtractive Primitive</source>
      <translation>Primitiva sustractiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="286"/>
      <source>Creates a subtractive primitive</source>
      <translation>Crea una primitiva sustractiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Subtractive Box</source>
      <translation>Caja sustractiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Subtractive Cylinder</source>
      <translation>Cilindro sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Subtractive Sphere</source>
      <translation>Esfera sustractiva</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Subtractive Cone</source>
      <translation>Cono sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Elipsoide sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Subtractive Torus</source>
      <translation>Toroide sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Subtractive Prism</source>
      <translation>Prisma sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Subtractive Wedge</source>
      <translation>Cuña sustractiva</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="362"/>
      <source>Edit Shape Binder</source>
      <translation>Editar vinculador de forma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="373"/>
      <source>Create Shape Binder</source>
      <translation>Crear vinculador de forma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="472"/>
      <source>Create Sub-Shape Binder</source>
      <translation>Crear vinculador de subforma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="542"/>
      <source>Create Clone</source>
      <translation>Crear clon</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1166"/>
      <source>Make Copy</source>
      <translation>Crear copia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2784"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>Convertir en operación de transformación múltiple</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="266"/>
      <source>Sketch on Face</source>
      <translation>Croquis sobre cara</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="328"/>
      <source>Make copy</source>
      <translation>Crear copia</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="530"/>
      <location filename="../../SketchWorkflow.cpp" line="798"/>
      <source>New Sketch</source>
      <translation>Croquis nuevo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2881"/>
      <source>Create Boolean</source>
      <translation>Crear booleana</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <location filename="../../CommandBody.cpp" line="225"/>
      <source>Add a Body</source>
      <translation>Agregar un cuerpo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="528"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>Migrar operaciones antiguas de Part Design a cuerpos</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="770"/>
      <source>Duplicate a Part Design object</source>
      <translation>Duplicar un objeto de Part Design</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1115"/>
      <source>Move a feature inside body</source>
      <translation>Mover una operación dentro del cuerpo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="724"/>
      <source>Move tip to selected feature</source>
      <translation>Mover la punta a la operación seleccionada</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="929"/>
      <source>Move an object</source>
      <translation>Mover un objeto</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="306"/>
      <source>Mirror</source>
      <translation>Simetría</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="346"/>
      <source>Linear Pattern</source>
      <translation>Patrón lineal</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="395"/>
      <source>Polar Pattern</source>
      <translation>Patrón polar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="436"/>
      <source>Circular Pattern</source>
      <translation>Patrón circular</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="475"/>
      <source>Path Pattern</source>
      <translation>Patrón por trayectoria</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="499"/>
      <source>Point Pattern</source>
      <translation>Patrón por puntos</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="521"/>
      <source>Scale</source>
      <translation>Escalar</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation>Herramientas de caras</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation>Herramientas de aristas</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation>Herramientas booleanas</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation>Herramientas auxiliares</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation>Herramientas de modelado</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>Crear geometría</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Start Part</source>
      <translation>Iniciar pieza</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>Parámetros de evolvente</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>Número de dientes</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>Módulo</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>Ángulo de presión</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>Alta precisión</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Verdadero</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Falso</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>Engranaje exterior</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>Coeficiente de altura de cabeza</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>Coeficiente de altura de pie</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>Coeficiente del redondeo de pie</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>Coeficiente de desplazamiento del perfil</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Se necesita un cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>Para crear un objeto de Part Design nuevo debe haber un cuerpo activo en el documento.
Seleccione un cuerpo de la lista o cree un cuerpo nuevo.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>Crear cuerpo nuevo</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="53"/>
      <source>Select an active body</source>
      <translation>Seleccione un cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Primitivas geométricas</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Ángulo en la primera dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Ángulo en la segunda dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>Longitud</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>Ancho</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>Alto</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>Radio</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>Ángulo de rotación</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>Radio 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>Radio 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>Parámetro U</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>Parámetros V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Radio en la dirección z local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>Radio en la dirección X local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>Radio 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>Radio en la dirección Y local
Si es cero, es igual a Radius2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>Parámetro V</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>Radio en el plano XY local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>Radio en el plano XZ local</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>Polígono</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>Radio circunscrito</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X mín./máx.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y mín./máx.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z mín./máx.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 mín./máx.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 mín./máx.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>Paso</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>Sistema de coordenadas</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>Crecimiento</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>Número de vueltas</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>Ángulo 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>Ángulo 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>Por 3 puntos</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>Radio mayor</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>Radio menor</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2354"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2365"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2370"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>A derecha</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>A izquierda</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Punto inicial</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Punto final</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Referencia</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Seleccionó geometrías que no forman parte del cuerpo activo. Defina cómo tratar esas selecciones. Si no quiere esas referencias, cancele el comando.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Crear una copia independiente (recomendado)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Crear una copia dependiente</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Crear una referencia cruzada</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="336"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Seleccionar esto causaría una dependencia circular.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>Agregar cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>Quitar cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Unión</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Diferencia</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="97"/>
      <source>Boolean Parameters</source>
      <translation>Parámetros de la booleana</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="128"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="100"/>
      <source>Subtractive Box Parameters</source>
      <translation>Parámetros de la caja sustractiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="101"/>
      <source>Additive Box Parameters</source>
      <translation>Parámetros de la caja aditiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="103"/>
      <source>Subtractive Cylinder Parameters</source>
      <translation>Parámetros del cilindro sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="104"/>
      <source>Additive Cylinder Parameters</source>
      <translation>Parámetros del cilindro aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="106"/>
      <source>Subtractive Sphere Parameters</source>
      <translation>Parámetros de la esfera sustractiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="107"/>
      <source>Additive Sphere Parameters</source>
      <translation>Parámetros de la esfera aditiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="109"/>
      <source>Subtractive Cone Parameters</source>
      <translation>Parámetros del cono sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="110"/>
      <source>Additive Cone Parameters</source>
      <translation>Parámetros del cono aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="112"/>
      <source>Subtractive Ellipsoid Parameters</source>
      <translation>Parámetros del elipsoide sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="113"/>
      <source>Additive Ellipsoid Parameters</source>
      <translation>Parámetros del elipsoide aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="115"/>
      <source>Subtractive Torus Parameters</source>
      <translation>Parámetros del toroide sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="116"/>
      <source>Additive Torus Parameters</source>
      <translation>Parámetros del toroide aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="118"/>
      <source>Subtractive Prism Parameters</source>
      <translation>Parámetros del prisma sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="119"/>
      <source>Additive Prism Parameters</source>
      <translation>Parámetros del prisma aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="121"/>
      <source>Subtractive Wedge Parameters</source>
      <translation>Parámetros de la cuña sustractiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="122"/>
      <source>Additive Wedge Parameters</source>
      <translation>Parámetros de la cuña aditiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="125"/>
      <source>Subtractive Primitive Parameters</source>
      <translation>Parámetros de la primitiva sustractiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="126"/>
      <source>Additive Primitive Parameters</source>
      <translation>Parámetros de la primitiva aditiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1045"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1053"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1061"/>
      <source>Invalid wedge parameters</source>
      <translation>Parámetros de cuña no válidos</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1046"/>
      <source>X min must not be equal to X max!</source>
      <translation>¡X mín. no puede ser igual a X máx.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1054"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>¡Y mín. no puede ser igual a Y máx.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1062"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>¡Z mín. no puede ser igual a Z máx.!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1105"/>
      <source>Create primitive</source>
      <translation>Crear primitiva</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1150"/>
      <source>%1 fine dragging</source>
      <translation>%1 arrastre fino</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1153"/>
      <source>%1 coarse dragging</source>
      <translation>%1 arrastre grueso</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Alterna entre el modo de selección y el de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- seleccione un elemento para resaltarlo
- haga doble clic en un elemento para ver los chaflanes</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>Tipo</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>Distancia igual</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>Dos distancias</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>Distancia y ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>Invierte la dirección</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>Usar todas las aristas</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>Tamaño</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>Tamaño 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="346"/>
      <source>Empty chamfer created!
</source>
      <translation>¡Se creó un chaflán vacío!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="437"/>
      <source>The body list cannot be empty</source>
      <translation>La lista de cuerpos no puede estar vacía</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="437"/>
      <source>Empty Body List</source>
      <translation>Lista de cuerpos vacía</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="458"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Booleana: aceptar: error de entrada</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>Incompatible Reference Set</source>
      <translation>Conjunto de referencias incompatible</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="109"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Ningún modo de anclaje se ajusta al conjunto de referencias actual. Si continúa, la operación se quedará donde está y no se moverá cuando cambien las referencias. ¿Continuar?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Alterna entre el modo de selección y el de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- seleccione un elemento para resaltarlo
- haga doble clic en un elemento para ver los ángulos de salida</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>Ángulo de salida</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>Plano neutro</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>Dirección de desmoldeo</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>Invertir la dirección de desmoldeo</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="304"/>
      <source>Empty draft created!
</source>
      <translation>¡Se creó un ángulo de salida vacío!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="591"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="596"/>
      <source>Confirm Selection</source>
      <translation>Confirmar selección</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="609"/>
      <source>Add All Edges</source>
      <translation>Agregar todas las aristas</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="615"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>Agrega todas las aristas a la lista (solo en el modo de agregar a la selección)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="624"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1480"/>
      <source>No face selected</source>
      <translation>No hay ninguna cara seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="184"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1243"/>
      <source>Face</source>
      <translation>Cara</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="81"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="401"/>
      <source>Preview</source>
      <translation>Vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="405"/>
      <source>Select Faces</source>
      <translation>Seleccionar caras</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="780"/>
      <source>Select reference…</source>
      <translation>Seleccionar referencia…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="667"/>
      <source>No shape selected</source>
      <translation>No hay ninguna forma seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="68"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="226"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1180"/>
      <source>No start reference selected</source>
      <translation>No hay ninguna referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="69"/>
      <source>Offset from the profile or selected start reference</source>
      <translation>Desfase desde el perfil o la referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="773"/>
      <source>Sketch normal</source>
      <translation>Normal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="776"/>
      <source>Face normal</source>
      <translation>Normal de la cara</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="784"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="787"/>
      <source>Custom direction</source>
      <translation>Dirección personalizada</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1174"/>
      <source>Cancel</source>
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1175"/>
      <source>Select face, plane...</source>
      <translation>Seleccione una cara, un plano...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1179"/>
      <source>Pick Reference</source>
      <translation>Elegir referencia</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1190"/>
      <source>Click on a shape in the model</source>
      <translation>Haga clic en una forma del modelo</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1467"/>
      <source>One sided</source>
      <translation>Un lado</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1468"/>
      <source>Two sided</source>
      <translation>Dos lados</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1469"/>
      <source>Symmetric</source>
      <translation>Simétrica</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1475"/>
      <source>Face selection active</source>
      <translation>Selección de caras activa</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Permitir operaciones usadas</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>Permitir operaciones externas</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>De otros cuerpos de la misma pieza</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>De otras piezas u operaciones libres</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Crear una copia independiente (recomendado)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Crear una copia dependiente</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Crear una referencia cruzada</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="68"/>
      <source>Valid</source>
      <translation>Válido</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="70"/>
      <source>Invalid shape</source>
      <translation>Forma no válida</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="72"/>
      <source>No wire in sketch</source>
      <translation>El croquis no tiene alambres</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="74"/>
      <source>Sketch already used by other feature</source>
      <translation>Otra operación ya usa el croquis</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="76"/>
      <source>Belongs to another body</source>
      <translation>Pertenece a otro cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="78"/>
      <source>Belongs to another part</source>
      <translation>Pertenece a otra pieza</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="80"/>
      <source>Doesn't belong to any body</source>
      <translation>No pertenece a ningún cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="82"/>
      <source>Base plane</source>
      <translation>Plano base</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="84"/>
      <source>Feature is located after the tip of the body</source>
      <translation>La operación está después de la punta del cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="96"/>
      <source>Select Attachment</source>
      <translation>Seleccionar anclaje</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Alterna entre el modo de selección y el de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- seleccione un elemento para resaltarlo
- haga doble clic en un elemento para ver los redondeos</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>Radio</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>Usar todas las aristas</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="204"/>
      <source>Empty fillet created!</source>
      <translation>¡Se creó un redondeo vacío!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Válido</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="259"/>
      <source>Base X-axis</source>
      <translation>Eje X base</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="260"/>
      <source>Base Y-axis</source>
      <translation>Eje Y base</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="261"/>
      <source>Base Z-axis</source>
      <translation>Eje Z base</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Horizontal sketch axis</source>
      <translation>Eje horizontal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="242"/>
      <source>Vertical sketch axis</source>
      <translation>Eje vertical del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Normal sketch axis</source>
      <translation>Normal al  boceto</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>Estado</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>ejes</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="226"/>
      <source>Select reference…</source>
      <translation>Seleccionar referencia…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>Modo</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Paso-Altura-Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Paso-Vueltas-Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Altura-Vueltas-Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Altura-Vueltas-Crecimiento</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>Paso/Espaciado</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>Altura</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>Vueltas</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>Ángulo del cono</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>Crecimiento radial</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="274"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="285"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="290"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="307"/>
      <source>Recompute on change</source>
      <translation>Recalcular al cambiar</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>A izquierda</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Invertido</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="71"/>
      <source>Subtractive Helix Parameters</source>
      <translation>Parámetros de la hélice sustractiva</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="72"/>
      <source>Additive Helix Parameters</source>
      <translation>Parámetros de la hélice aditiva</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="245"/>
      <source>Construction line %1</source>
      <translation>Línea de construcción %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="313"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>Advertencia: la hélice podría autointersecarse</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="318"/>
      <source>Error: helix touches itself</source>
      <translation>Error: la hélice se toca a sí misma</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="364"/>
      <source>Error: unsupported mode</source>
      <translation>Error: modo no admitido</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Counterbore</source>
      <translation>Avellanado cilíndrico</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="60"/>
      <source>Countersink</source>
      <translation>Avellanado cónico</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Counterdrill</source>
      <translation>Contrataladrado</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>Hole Parameters</source>
      <translation>Parámetros del agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>None</source>
      <translation>Ninguno</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ISO metric regular</source>
      <translation>ISO métrica normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO metric fine</source>
      <translation>ISO métrica fina</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>UTS coarse</source>
      <translation>UTS gruesa</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>UTS fine</source>
      <translation>UTS fina</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>UTS extra fine</source>
      <translation>UTS extrafina</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="81"/>
      <source>ANSI pipes</source>
      <translation>Tubería ANSI</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="82"/>
      <source>ISO/BSP pipes</source>
      <translation>Tubería ISO/BSP</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="83"/>
      <source>BSW whitworth</source>
      <translation>Whitworth BSW</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="84"/>
      <source>BSF whitworth fine</source>
      <translation>Whitworth fina BSF</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="85"/>
      <source>ISO tyre valves</source>
      <translation>Válvulas de neumático ISO</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="203"/>
      <source>Offset from the profile or selected start reference</source>
      <translation>Desfase desde el perfil o la referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="702"/>
      <source>Cancel</source>
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="703"/>
      <source>Select face, plane...</source>
      <translation>Seleccione una cara, un plano...</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="708"/>
      <source>Pick Reference</source>
      <translation>Elegir referencia</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="709"/>
      <location filename="../../TaskHoleParameters.cpp" line="1130"/>
      <source>No start reference selected</source>
      <translation>No hay ninguna referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="772"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Media</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="776"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Fina</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="780"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>Basta</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="786"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="790"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Cerrar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="794"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>Holgada</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="798"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Normal</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="799"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Cerrar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="800"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>Amplia</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Superficie reglada</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Cerrado</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Perfil</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Objeto</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Agregar sección</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Quitar sección</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>La lista se puede reordenar arrastrando</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="115"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="126"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="131"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="148"/>
      <source>Recompute on change</source>
      <translation>Recalcular al cambiar</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="65"/>
      <source>Subtractive Loft Parameters</source>
      <translation>Parámetros del recubrimiento sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="66"/>
      <source>Additive Loft Parameters</source>
      <translation>Parámetros del recubrimiento aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="96"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>Plano</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="186"/>
      <source>Error</source>
      <translation>Error</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>Transformaciones</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>Aceptar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="74"/>
      <source>Edit</source>
      <translation>Editar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Delete</source>
      <translation>Eliminar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="80"/>
      <source>Add Mirror Transformation</source>
      <translation>Agregar transformación de simetría</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="88"/>
      <source>Add Linear Pattern</source>
      <translation>Agregar patrón lineal</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="96"/>
      <source>Add Circular Pattern</source>
      <translation>Agregar patrón circular</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="104"/>
      <source>Add Path Pattern</source>
      <translation>Agregar patrón por trayectoria</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="112"/>
      <source>Add Point Pattern</source>
      <translation>Agregar patrón por puntos</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="120"/>
      <source>Add Polar Pattern</source>
      <translation>Agregar patrón polar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="128"/>
      <source>Add Scale Transformation</source>
      <translation>Agregar transformación de escala</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="131"/>
      <source>Move Up</source>
      <translation>Subir</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="134"/>
      <source>Move Down</source>
      <translation>Bajar</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="166"/>
      <source>Right-click to add a transformation</source>
      <translation>Haga clic derecho para agregar una transformación</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation>Parámetros de la protrusión</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>Desfase de la protrusión respecto a la cara donde termina en el lado 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>Desfase de la protrusión respecto a la cara donde termina en el lado 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>Invierte la dirección de la protrusión</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Dimensiones</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>Hasta la última</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>Hasta la primera</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>Hasta la cara</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>Hasta la forma</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="150"/>
      <location filename="../../TaskPadPocketParameters.ui" line="385"/>
      <source>Type</source>
      <translation>Tipo</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="158"/>
      <source>Dimension</source>
      <translation>Dimensiones</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="166"/>
      <location filename="../../TaskPadPocketParameters.ui" line="395"/>
      <source>Length</source>
      <translation>Longitud</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="183"/>
      <location filename="../../TaskPadPocketParameters.ui" line="412"/>
      <source>Offset to face</source>
      <translation>Desfase hasta la cara</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="274"/>
      <location filename="../../TaskPadPocketParameters.ui" line="503"/>
      <source>Select all faces</source>
      <translation>Seleccionar todas las caras</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="299"/>
      <location filename="../../TaskPadPocketParameters.ui" line="528"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="337"/>
      <location filename="../../TaskPadPocketParameters.ui" line="566"/>
      <source>Select Face</source>
      <translation>Seleccionar cara</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="363"/>
      <source>Side 2</source>
      <translation>Lado 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="104"/>
      <location filename="../../TaskPadPocketParameters.ui" line="594"/>
      <source>Direction</source>
      <translation>Dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="623"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>Defina una dirección o seleccione una arista
del modelo como referencia</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="628"/>
      <source>Sketch normal</source>
      <translation>Normal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="638"/>
      <source>Custom direction</source>
      <translation>Dirección personalizada</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="651"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>Usa un vector personalizado como dirección de la protrusión; si no,
se usa el vector normal del plano del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="603"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Si está desmarcada, la longitud se
mide a lo largo de la dirección indicada</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="607"/>
      <source>Length along sketch normal</source>
      <translation>Longitud a lo largo de la normal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="296"/>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Alterna entre el modo de selección y el de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Start</source>
      <translation>Inicio</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Profile plane</source>
      <translation>Plano del perfil</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="35"/>
      <location filename="../../TaskPadPocketParameters.ui" line="87"/>
      <source>Offset</source>
      <translation>Desplazamiento</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="40"/>
      <location filename="../../TaskPadPocketParameters.ui" line="48"/>
      <source>Reference</source>
      <translation>Referencia</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="77"/>
      <source>Pick Reference</source>
      <translation>Elegir referencia</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="587"/>
      <source>Reversed</source>
      <translation>Invertido</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="616"/>
      <source>Direction/edge</source>
      <translation>Dirección/arista</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="633"/>
      <source>Select reference…</source>
      <translation>Seleccionar referencia…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="664"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="671"/>
      <source>X-component of direction vector</source>
      <translation>Componente X del vector de dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="693"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="700"/>
      <source>Y-component of direction vector</source>
      <translation>Componente Y del vector de dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="722"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="729"/>
      <source>Z-component of direction vector</source>
      <translation>Componente Z del vector de dirección</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="200"/>
      <location filename="../../TaskPadPocketParameters.ui" line="429"/>
      <source>Angle to taper the extrusion</source>
      <translation>Ángulo de inclinación de la extrusión</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="128"/>
      <source>Side 1</source>
      <translation>Lado 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="203"/>
      <location filename="../../TaskPadPocketParameters.ui" line="432"/>
      <source>Taper angle</source>
      <translation>Ángulo de inclinación</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="256"/>
      <location filename="../../TaskPadPocketParameters.ui" line="485"/>
      <source>Select Shape</source>
      <translation>Seleccionar forma</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="271"/>
      <location filename="../../TaskPadPocketParameters.ui" line="500"/>
      <source>Selects all faces of the shape</source>
      <translation>Selecciona todas las caras de la forma</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="762"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="773"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="778"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="788"/>
      <source>Recompute on change</source>
      <translation>Recalcular al cambiar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Modo de orientación</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Estándar</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Fijo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Auxiliar</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Binormal</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>Equivalencia curvilínea</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Perfil</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Objeto</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Agregar arista</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Quitar arista</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Define el vector binormal constante con el que se calcula la orientación de los perfiles</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="83"/>
      <source>Subtractive Pipe Section Orientation</source>
      <translation>Orientación de la sección del barrido sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="84"/>
      <source>Additive Pipe Section Orientation</source>
      <translation>Orientación de la sección del barrido aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="666"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>Perfil</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>Objeto</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>Transición en las esquinas</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>Esquina recta</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>Esquina redondeada</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>Trayectoria del barrido</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>Agregar arista</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>Quitar arista</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="158"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="169"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="174"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Transformada</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="77"/>
      <source>Subtractive Pipe Parameters</source>
      <translation>Parámetros del barrido sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="78"/>
      <source>Additive Pipe Parameters</source>
      <translation>Parámetros del barrido aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="113"/>
      <source>Select All</source>
      <translation>Seleccionar todo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="131"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="502"/>
      <location filename="../../TaskPipeParameters.cpp" line="624"/>
      <source>Input Error</source>
      <translation>Error de entrada</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="502"/>
      <source>No active body</source>
      <translation>No hay ningún cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Modo de transformación</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Constante</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Multisección</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Agregar sección</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Quitar sección</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>La lista se puede reordenar arrastrando</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="89"/>
      <source>Subtractive Pipe Section Transformation</source>
      <translation>Transformación de la sección del barrido sustractivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="90"/>
      <source>Additive Pipe Section Transformation</source>
      <translation>Transformación de la sección del barrido aditivo</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="956"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation>Parámetros de la cajera</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>Desfase respecto a la cara seleccionada donde termina la cajera en el lado 1</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>Desfase respecto a la cara seleccionada donde termina la cajera en el lado 2</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="48"/>
      <source>Reverses pocket direction</source>
      <translation>Invierte la dirección de la cajera</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Dimension</source>
      <translation>Dimensiones</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Through all</source>
      <translation>A través de todo</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>To first</source>
      <translation>Hasta la primera</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="80"/>
      <source>Up to face</source>
      <translation>Hasta la cara</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="81"/>
      <source>Up to shape</source>
      <translation>Hasta la forma</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="194"/>
      <location filename="../../TaskRevolutionParameters.ui" line="296"/>
      <source>Type</source>
      <translation>Tipo</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="131"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="313"/>
      <source>Base X-axis</source>
      <translation>Eje X base</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Start</source>
      <translation>Inicio</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <source>Profile plane</source>
      <translation>Plano del perfil</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="35"/>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <source>Offset</source>
      <translation>Desplazamiento</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="40"/>
      <location filename="../../TaskRevolutionParameters.ui" line="48"/>
      <source>Reference</source>
      <translation>Referencia</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="77"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="497"/>
      <source>Pick Reference</source>
      <translation>Elegir referencia</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="113"/>
      <source>Direction</source>
      <translation>Dirección</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="136"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="314"/>
      <source>Base Y-axis</source>
      <translation>Eje Y base</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="141"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="315"/>
      <source>Base Z-axis</source>
      <translation>Eje Z base</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="146"/>
      <source>Horizontal sketch axis</source>
      <translation>Eje horizontal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="151"/>
      <source>Vertical sketch axis</source>
      <translation>Eje vertical del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="178"/>
      <source>Side 1</source>
      <translation>Lado 1</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="247"/>
      <location filename="../../TaskRevolutionParameters.ui" line="343"/>
      <source>Select Face</source>
      <translation>Seleccionar cara</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="280"/>
      <source>Side 2</source>
      <translation>Lado 2</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="367"/>
      <source>Symmetric to plane</source>
      <translation>Simétrico al plano</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="374"/>
      <source>Projects the selected axis onto the profile plane if the profile is planar. When unchecked, uses the axis in its original position and direction.</source>
      <translation>Proyecta el eje seleccionado sobre el plano del perfil si el perfil es plano. Si está desmarcada, usa el eje en su posición y dirección originales.</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="377"/>
      <source>Project selected axis to profile plane</source>
      <translation>Proyectar el eje seleccionado sobre el plano del perfil</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="384"/>
      <source>Reversed</source>
      <translation>Invertido</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="393"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="404"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="409"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="123"/>
      <source>Axis</source>
      <translation>ejes</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="156"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="323"/>
      <source>Select reference…</source>
      <translation>Seleccionar referencia…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="210"/>
      <location filename="../../TaskRevolutionParameters.ui" line="306"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="258"/>
      <source>Angle</source>
      <translation>Ángulo</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="235"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="630"/>
      <source>Face</source>
      <translation>Cara</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="426"/>
      <source>Recompute on change</source>
      <translation>Recalcular al cambiar</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="146"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="498"/>
      <source>No start reference selected</source>
      <translation>No hay ninguna referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="147"/>
      <source>Angular offset from the profile or selected start reference</source>
      <translation>Desfase angular desde el perfil o la referencia inicial seleccionada</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="260"/>
      <source>To last</source>
      <translation>Hasta la última</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="263"/>
      <source>Through all</source>
      <translation>A través de todo</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="265"/>
      <source>To first</source>
      <translation>Hasta la primera</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="266"/>
      <source>Up to face</source>
      <translation>Hasta la cara</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="273"/>
      <source>One sided</source>
      <translation>Un lado</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="274"/>
      <source>Two sided</source>
      <translation>Dos lados</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="275"/>
      <source>Symmetric</source>
      <translation>Simétrica</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="506"/>
      <source>Cancel</source>
      <translation>Cancelar</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="507"/>
      <source>Select face, plane...</source>
      <translation>Seleccione una cara, un plano...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="651"/>
      <source>Face selection active</source>
      <translation>Selección de caras activa</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="246"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="656"/>
      <source>No face selected</source>
      <translation>No hay ninguna cara seleccionada</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>Factor</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>Repeticiones</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Objeto</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Agregar geometría</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Quitar geometría</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation>Parámetros del vinculador de forma</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="139"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="127"/>
      <location filename="../../TaskSketchBasedParameters.cpp" line="242"/>
      <source>Face</source>
      <translation>Cara</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>Alterna entre el modo de selección y el de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- seleccione un elemento para resaltarlo
- haga doble clic en un elemento para ver las operaciones</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>Espesor</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>Modo</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>Superficie</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>Tubo</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>Anverso</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>Tipo de unión</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>Arco</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>Aplicar el espesor hacia adentro</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="216"/>
      <source>Recto verso applies the thickness equally to both sides</source>
      <translation>Recto verso aplica el espesor por igual a ambos lados</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="219"/>
      <source>Total wall thickness; half is applied to each side</source>
      <translation>Espesor total de la pared; se aplica la mitad a cada lado</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="293"/>
      <source>Empty thickness created!
</source>
      <translation>¡Se creó un espesor vacío!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>Quitar</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="398"/>
      <source>Normal sketch axis</source>
      <translation>Normal al croquis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="397"/>
      <source>Vertical sketch axis</source>
      <translation>Eje vertical del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="396"/>
      <source>Horizontal sketch axis</source>
      <translation>Eje horizontal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="400"/>
      <location filename="../../TaskTransformedParameters.cpp" line="436"/>
      <source>Construction line %1</source>
      <translation>Línea de construcción %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="414"/>
      <source>Base X-axis</source>
      <translation>Eje X base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="415"/>
      <source>Base Y-axis</source>
      <translation>Eje Y base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="416"/>
      <source>Base Z-axis</source>
      <translation>Eje Z base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="450"/>
      <source>Base XY-plane</source>
      <translation>Plano XY base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="451"/>
      <source>Base YZ-plane</source>
      <translation>Plano YZ base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="452"/>
      <source>Base XZ-plane</source>
      <translation>Plano XZ base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="424"/>
      <location filename="../../TaskTransformedParameters.cpp" line="460"/>
      <source>Select reference…</source>
      <translation>Seleccionar referencia…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>Transformar el cuerpo</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>Transformar las formas de herramienta</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>Agregar operación</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>Quitar operación</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="116"/>
      <source>Recompute on change</source>
      <translation>Recalcular al cambiar</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="911"/>
      <source>Select Body</source>
      <translation>Seleccionar cuerpo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="912"/>
      <source>Select a body from the list</source>
      <translation>Seleccione un cuerpo de la lista</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1100"/>
      <source>Move Feature After…</source>
      <translation>Mover operación después de…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1101"/>
      <source>Select a feature from the list</source>
      <translation>Seleccione una operación de la lista</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1188"/>
      <source>Move Tip</source>
      <translation>Mover punta</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1194"/>
      <source>Set tip to last feature?</source>
      <translation>¿Poner la punta en la última operación?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>La operación movida queda después de la punta actual.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="174"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Ningún modo de anclaje se ajusta a los objetos seleccionados. Seleccione otra cosa.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="187"/>
      <location filename="../../Command.cpp" line="195"/>
      <location filename="../../Command.cpp" line="202"/>
      <source>Error</source>
      <translation>Error</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="836"/>
      <source>Several sub-elements selected</source>
      <translation>Hay varios subelementos seleccionados</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="837"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>¡Seleccione una sola cara como soporte del croquis!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="844"/>
      <source>Select a face as support for a sketch!</source>
      <translation>¡Seleccione una cara como soporte del croquis!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="851"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>¡Se necesita una cara plana como soporte del croquis!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="858"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>Cree primero un plano o seleccione una cara donde dibujar el croquis</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="843"/>
      <source>No support face selected</source>
      <translation>No hay ninguna cara de soporte seleccionada</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="850"/>
      <source>No planar support</source>
      <translation>No hay soporte plano</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="857"/>
      <source>No valid planes in this document</source>
      <translation>No hay planos válidos en este documento</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1194"/>
      <location filename="../../ViewProviderDatum.cpp" line="259"/>
      <location filename="../../ViewProvider.cpp" line="164"/>
      <location filename="../../SketchWorkflow.cpp" line="763"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="98"/>
      <source>A dialog is already open in the task panel</source>
      <translation>Ya hay un diálogo abierto en el panel de tareas</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1048"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>No se puede usar este comando porque no hay ningún sólido del que restar.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>Asegúrese de que el cuerpo tenga una operación antes de usar un comando sustractivo.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1075"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>No se puede usar el objeto seleccionado. Debe pertenecer al cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="188"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>No hay ningún cuerpo activo. Active un cuerpo antes de insertar una geometría de referencia.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="173"/>
      <source>Invalid Selection</source>
      <translation>Selección no válida</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="500"/>
      <source>Sub-shape binder</source>
      <translation>Vinculador de subforma</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1107"/>
      <source>No sketch to work on</source>
      <translation>No hay ningún croquis con el que trabajar</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1108"/>
      <source>No sketch is available in the document</source>
      <translation>No hay ningún croquis disponible en el documento</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1195"/>
      <location filename="../../ViewProviderDatum.cpp" line="260"/>
      <location filename="../../ViewProvider.cpp" line="165"/>
      <location filename="../../SketchWorkflow.cpp" line="764"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="99"/>
      <source>Close this dialog?</source>
      <translation>¿Cerrar este diálogo?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1876"/>
      <location filename="../../Command.cpp" line="1911"/>
      <source>Wrong selection</source>
      <translation>Selección incorrecta</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1877"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Seleccione una arista, una cara o un cuerpo de un solo cuerpo.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1884"/>
      <location filename="../../Command.cpp" line="2335"/>
      <source>Selection is not in the active body</source>
      <translation>La selección no está en el cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1912"/>
      <source>Shape of the selected part is empty</source>
      <translation>La forma de la pieza seleccionada está vacía</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2085"/>
      <location filename="../../Command.cpp" line="2113"/>
      <source>Wrong Selection</source>
      <translation>Selección incorrecta</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>Select faces from a single body</source>
      <translation>Seleccione caras de un solo cuerpo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2093"/>
      <source>Selection Outside Active Body</source>
      <translation>Selección fuera del cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2094"/>
      <source>Select faces from the active body</source>
      <translation>Seleccione caras del cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2102"/>
      <source>Wrong Object Type</source>
      <translation>Tipo de objeto incorrecto</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2103"/>
      <location filename="../../Command.cpp" line="2114"/>
      <source>Defeaturing works only on faces</source>
      <translation>La supresión de caras solo funciona con caras</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2336"/>
      <source>Select only one feature in an active body.</source>
      <translation>Seleccione solo una operación de un cuerpo activo.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1885"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Seleccione una arista, una cara o un cuerpo de un cuerpo activo.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1078"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>Considere usar un vinculador de forma o una operación base para hacer referencia a geometría externa en un cuerpo</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1898"/>
      <source>Wrong object type</source>
      <translation>Tipo de objeto incorrecto</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1899"/>
      <source>%1 works only on parts.</source>
      <translation>%1 solo funciona con piezas.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>Part creation failed</source>
      <translation>No se pudo crear la pieza</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>Failed to create a part object.</source>
      <translation>No se pudo crear un objeto pieza.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="139"/>
      <location filename="../../CommandBody.cpp" line="147"/>
      <location filename="../../CommandBody.cpp" line="166"/>
      <location filename="../../CommandBody.cpp" line="218"/>
      <source>Bad base feature</source>
      <translation>Operación base incorrecta</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="140"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>Un cuerpo no puede basarse en una operación de Part Design.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="148"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1 ya pertenece a un cuerpo y no se puede usar como operación base de otro cuerpo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="185"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>La forma seleccionada consta de varios sólidos.
Esto puede producir resultados inesperados.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="191"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>La forma seleccionada consta de varias cáscaras.
Esto puede producir resultados inesperados.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="197"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>La forma seleccionada consta solo de una cáscara.
Esto puede producir resultados inesperados.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="203"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>La forma seleccionada consta de varios sólidos o cáscaras.
Esto puede producir resultados inesperados.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="210"/>
      <source>Base feature</source>
      <translation>Operación base</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="219"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Un cuerpo puede basarse como máximo en una operación.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="234"/>
      <source>Body</source>
      <translation>Cuerpo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="427"/>
      <source>Nothing to migrate</source>
      <translation>No hay nada que migrar</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Edite '%1' y redefínalo para que use un plano base o de referencia como plano del croquis.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>Seleccione exactamente una operación de Part Design o un cuerpo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="699"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>No se pudo determinar el cuerpo de la operación seleccionada '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="880"/>
      <source>Only features of a single source body can be moved</source>
      <translation>Solo se pueden mover operaciones de un mismo cuerpo de origen</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="615"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>No se puede migrar el plano del croquis</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="167"/>
      <source>Base feature (%1) has an empty shape.</source>
      <translation>La operación base (%1) tiene una forma vacía.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="428"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>No se encontraron operaciones de Part Design sin cuerpo. No hay nada que migrar.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="690"/>
      <location filename="../../CommandBody.cpp" line="698"/>
      <location filename="../../CommandBody.cpp" line="712"/>
      <location filename="../../CommandBody.cpp" line="1066"/>
      <location filename="../../CommandBody.cpp" line="1076"/>
      <source>Selection error</source>
      <translation>Error de selección</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="713"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Solo una operación sólida puede ser la punta de un cuerpo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="849"/>
      <location filename="../../CommandBody.cpp" line="879"/>
      <location filename="../../CommandBody.cpp" line="897"/>
      <source>Features cannot be moved</source>
      <translation>No se pueden mover las operaciones</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="850"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Algunas de las operaciones seleccionadas tienen dependencias en el cuerpo de origen</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="898"/>
      <source>There are no other bodies to move to</source>
      <translation>No hay otros cuerpos a los que mover</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1067"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>No se puede mover la operación base de un cuerpo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1077"/>
      <source>Select one or more features from the same body.</source>
      <translation>Seleccione una o más operaciones del mismo cuerpo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1092"/>
      <source>Beginning of the body</source>
      <translation>Inicio del cuerpo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1173"/>
      <source>Dependency violation</source>
      <translation>Violación de dependencias</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1174"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>Una operación anterior no puede depender de una posterior.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="309"/>
      <source>No previous feature found</source>
      <translation>No se encontró ninguna operación anterior</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="310"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>No se puede crear una operación sustractiva sin una operación base</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="433"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="299"/>
      <source>Vertical sketch axis</source>
      <translation>Eje vertical del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="434"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="300"/>
      <source>Horizontal sketch axis</source>
      <translation>Eje horizontal del croquis</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="302"/>
      <source>Construction line %1</source>
      <translation>Línea de construcción %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="97"/>
      <source>Face</source>
      <translation>Cara</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="203"/>
      <source>Active Body Required</source>
      <translation>Se necesita un cuerpo activo</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="145"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>Para usar Part Design se necesita un cuerpo activo en el documento. Active un cuerpo (doble clic) o cree uno nuevo.

En documentos antiguos con objetos de Part Design sin cuerpo, use la función Migrar de Part Design para colocarlos en un cuerpo.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="204"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>Para crear un objeto de Part Design nuevo se necesita un cuerpo activo en el documento. Active un cuerpo existente (doble clic) o cree uno nuevo.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="266"/>
      <source>Feature is not in a body</source>
      <translation>La operación no está en un cuerpo</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="267"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Para usar esta operación, debe pertenecer a un objeto cuerpo del documento.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="313"/>
      <source>Feature is not in a part</source>
      <translation>La operación no está en una pieza</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="314"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Para usar esta operación, debe pertenecer a un objeto pieza del documento.</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="121"/>
      <location filename="../../ViewProviderDressUp.cpp" line="71"/>
      <location filename="../../ViewProviderTransformed.cpp" line="68"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="228"/>
      <source>Edit %1</source>
      <translation>Editar %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="134"/>
      <source>Set Face Colors</source>
      <translation>Asignar colores a las caras</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="214"/>
      <source>Plane</source>
      <translation>Plano</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="209"/>
      <source>Line</source>
      <translation>Línea</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="219"/>
      <source>Point</source>
      <translation>Punto</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>Sistema de coordenadas</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="236"/>
      <source>Edit Datum</source>
      <translation>Editar referencia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="100"/>
      <source>Feature error</source>
      <translation>Error de la operación</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="101"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>A %1 le falta una operación base.
La operación está rota y no se puede editar.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="223"/>
      <source>Edit Shape Binder</source>
      <translation>Editar vinculador de forma</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Synchronize</source>
      <translation>Sincronizar</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="354"/>
      <source>Select Bound Object</source>
      <translation>Seleccionar el objeto vinculado</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="156"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>El documento "%1" que está editando se diseñó con una versión antigua del banco de trabajo Part Design.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="163"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>¿Migrar para usar las operaciones modernas de Part Design?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="168"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>El documento "%1" parece estar a mitad de la migración desde el Part Design antiguo o tener una estructura algo dañada.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="175"/>
      <source>Make the migration automatically?</source>
      <translation>¿Migrar automáticamente?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="178"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>Nota: si elige migrar, no podrá editar el archivo con una versión anterior de FreeCAD.
Si no migra, no podrá usar las operaciones nuevas de Part Design, como cuerpos y piezas. Por tanto, tampoco podrá usar sus piezas en el banco de trabajo de ensamble.
De todos modos, podrá migrar en cualquier momento con 'Part Design -&gt; Migrar'.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="191"/>
      <source>Migrate Manually</source>
      <translation>Migrar manualmente</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="351"/>
      <source>Edit Boolean</source>
      <translation>Editar booleana</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="42"/>
      <source>Edit Chamfer</source>
      <translation>Editar chaflán</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="43"/>
      <source>Edit Draft</source>
      <translation>Editar ángulo de salida</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="42"/>
      <source>Edit Fillet</source>
      <translation>Editar redondeo</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="45"/>
      <source>Edit Groove</source>
      <translation>Editar garganta</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="50"/>
      <source>Edit Helix</source>
      <translation>Editar hélice</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="130"/>
      <source>Edit Hole</source>
      <translation>Editar agujero</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="40"/>
      <source>Edit Linear Pattern</source>
      <translation>Editar patrón lineal</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="67"/>
      <source>Edit Loft</source>
      <translation>Editar recubrimiento</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="40"/>
      <source>Edit Mirror</source>
      <translation>Editar simetría</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="49"/>
      <source>Edit Multi-Transform</source>
      <translation>Editar transformación múltiple</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="45"/>
      <source>Edit Pad</source>
      <translation>Editar protrusión</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="77"/>
      <source>Edit Pipe</source>
      <translation>Editar barrido</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="47"/>
      <source>Edit Pocket</source>
      <translation>Editar cajera</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="40"/>
      <source>Edit Polar Pattern</source>
      <translation>Editar patrón polar</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="52"/>
      <source>Edit Primitive</source>
      <translation>Editar primitiva</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="45"/>
      <source>Edit Revolution</source>
      <translation>Editar revolución</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="40"/>
      <source>Edit Scale</source>
      <translation>Editar escala</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="42"/>
      <source>Edit Thickness</source>
      <translation>Editar espesor</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDefeaturing.cpp" line="39"/>
      <source>Edit Defeaturing</source>
      <translation>Editar supresión de caras</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>Parámetros de la rueda dentada</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>Número de dientes</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>Referencia de la rueda dentada</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="66"/>
      <source>ANSI 25</source>
      <translation>ANSI 25</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="71"/>
      <source>ANSI 35</source>
      <translation>ANSI 35</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="76"/>
      <source>ANSI 41</source>
      <translation>ANSI 41</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="81"/>
      <source>ANSI 40</source>
      <translation>ANSI 40</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="86"/>
      <source>ANSI 50</source>
      <translation>ANSI 50</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="91"/>
      <source>ANSI 60</source>
      <translation>ANSI 60</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="96"/>
      <source>ANSI 80</source>
      <translation>ANSI 80</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="101"/>
      <source>ANSI 100</source>
      <translation>ANSI 100</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="106"/>
      <source>ANSI 120</source>
      <translation>ANSI 120</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="111"/>
      <source>ANSI 140</source>
      <translation>ANSI 140</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="116"/>
      <source>ANSI 160</source>
      <translation>ANSI 160</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="121"/>
      <source>ANSI 180</source>
      <translation>ANSI 180</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="126"/>
      <source>ANSI 200</source>
      <translation>ANSI 200</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="131"/>
      <source>ANSI 240</source>
      <translation>ANSI 240</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="136"/>
      <source>Bicycle with derailleur</source>
      <translation>Bicicleta con desviador</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>Bicicleta sin desviador</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>Paso de la cadena</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>Diámetro del rodillo de la cadena</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>Ancho del diente</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="146"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="151"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 08B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="156"/>
      <source>ISO 606 10B</source>
      <translation>ISO 606 10B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="161"/>
      <source>ISO 606 12B</source>
      <translation>ISO 606 12B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="166"/>
      <source>ISO 606 16B</source>
      <translation>ISO 606 16B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="171"/>
      <source>ISO 606 20B</source>
      <translation>ISO 606 20B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="176"/>
      <source>ISO 606 24B</source>
      <translation>ISO 606 24B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="181"/>
      <source>Motorcycle 420</source>
      <translation>Motocicleta 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>Motocicleta 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>Motocicleta 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>Motocicleta 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>Motocicleta 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>Motocicleta 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>Motocicleta 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 pulgadas</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="919"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>Actualización en vivo de los cambios en la rosca
Tenga en cuenta que el cálculo puede tardar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1114"/>
      <source>Thread Depth</source>
      <translation>Profundidad de la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1167"/>
      <source>Customize thread clearance</source>
      <translation>Personalizar la holgura de la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="794"/>
      <source>Clearance</source>
      <translation>Holgura</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="219"/>
      <source>Head type</source>
      <translation>Tipo de cabeza</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="248"/>
      <source>Depth type</source>
      <translation>Tipo de profundidad</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="340"/>
      <source>Head diameter</source>
      <translation>Diámetro de la cabeza</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="388"/>
      <source>Head depth</source>
      <translation>Profundidad de la cabeza</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="763"/>
      <source>Clearance / Passthrough</source>
      <translation>Holgura / pasante</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="781"/>
      <source>Hole type</source>
      <translation>Tipo de agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="923"/>
      <source>Update thread view</source>
      <translation>Actualizar la vista de la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1173"/>
      <source>Custom Clearance</source>
      <translation>Holgura personalizada</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1186"/>
      <source>Custom Thread clearance value</source>
      <translation>Valor personalizado de la holgura de la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Direction</source>
      <translation>Dirección</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="193"/>
      <source>Size</source>
      <translation>Tamaño</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="807"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>Holgura del agujero
Solo disponible para agujeros sin rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="812"/>
      <source>Standard</source>
      <translation>Estándar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="768"/>
      <source>Tap drill</source>
      <translation>Broca para roscar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="773"/>
      <source>Threaded</source>
      <translation>Roscado</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Close</source>
      <translation>Cerrar</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="822"/>
      <source>Wide</source>
      <translation>Amplia</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="900"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>Si el agujero tiene una rosca modelada</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="903"/>
      <source>Model Thread</source>
      <translation>Modelar la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="956"/>
      <source>Class</source>
      <translation>Clase</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="943"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>Clase de tolerancia de los agujeros roscados según el perfil del agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="647"/>
      <source>Diameter</source>
      <translation>Diámetro</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="669"/>
      <source>Hole diameter</source>
      <translation>Diámetro del agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="602"/>
      <source>Depth</source>
      <translation>Profundidad</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>Parámetros del agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="38"/>
      <source>Profile and Placement</source>
      <translation>Perfil y posicionamiento</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="52"/>
      <source>Base profile types</source>
      <translation>Tipos de perfil base</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="60"/>
      <source>Circles and arcs</source>
      <translation>Círculos y arcos</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Points, circles and arcs</source>
      <translation>Puntos, círculos y arcos</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="70"/>
      <source>Points</source>
      <translation>Puntos</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="78"/>
      <source>Start</source>
      <translation>Inicio</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="86"/>
      <source>Profile plane</source>
      <translation>Plano del perfil</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="91"/>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Offset</source>
      <translation>Desplazamiento</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="96"/>
      <location filename="../../TaskHoleParameters.ui" line="104"/>
      <source>Reference</source>
      <translation>Referencia</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="121"/>
      <source>Pick Reference</source>
      <translation>Elegir referencia</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Hole</source>
      <translation>Agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="265"/>
      <location filename="../../TaskHoleParameters.ui" line="1084"/>
      <source>Dimension</source>
      <translation>Dimensiones</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="270"/>
      <source>Through all</source>
      <translation>A través de todo</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="292"/>
      <source>Custom head values</source>
      <translation>Valores de cabeza personalizados</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Ángulo de la punta de la broca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="498"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>Incluir en la profundidad</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="705"/>
      <source>Switch direction</source>
      <translation>Cambiar la dirección</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1210"/>
      <source>Operation</source>
      <translation>Operación</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1221"/>
      <source>Subtraction</source>
      <translation>Sustracción</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1226"/>
      <source>Common</source>
      <translation>Intersección</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Thread</source>
      <translation>Rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1000"/>
      <source>&amp;Right hand</source>
      <translation>A &amp;derecha</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1016"/>
      <source>&amp;Left hand</source>
      <translation>A &amp;izquierda</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Thread Depth Type</source>
      <translation>Tipo de profundidad de la rosca</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1079"/>
      <source>Hole depth</source>
      <translation>Profundidad del agujero</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1089"/>
      <source>Tapped (DIN76)</source>
      <translation>Roscado (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="232"/>
      <source>Cut type for screw heads</source>
      <translation>Tipo de corte para cabezas de tornillo</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="286"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>Márquela para reemplazar los valores predefinidos por el 'Tipo'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="401"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>En los avellanados cónicos es la profundidad
de la parte superior del tornillo bajo la superficie</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="557"/>
      <source>Countersink angle</source>
      <translation>Ángulo del avellanado</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="494"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>El tamaño de la punta de la broca se tendrá
en cuenta en la profundidad de los agujeros ciegos</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="721"/>
      <source>Tapered</source>
      <translation>Cónico</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="734"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>Ángulo de conicidad del agujero
90 grados: agujero recto
menos de 90: radio menor en el fondo
más de 90: radio mayor en el fondo</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="702"/>
      <source>Reverses the hole direction</source>
      <translation>Invierte la dirección del agujero</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Sin mensajes</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Croquis</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>&amp;Diseño de Pieza</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>Cotas</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation>Operaciones aditivas</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation>Operaciones sustractivas</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation>Operaciones de acabado</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation>Operaciones de transformación</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation>Rueda dentada…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation>Engranaje de evolvente</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation>Asistente de diseño de ejes</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Measure</source>
      <translation>Medición</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Refresh</source>
      <translation>Actualizar</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Toggle 3D</source>
      <translation>Alternar 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Helper</source>
      <translation>Ayudas de Part Design</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="67"/>
      <source>Part Design Modeling</source>
      <translation>Modelado de Part Design</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Length [mm]</source>
      <translation>Longitud [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Diameter [mm]</source>
      <translation>Diámetro [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Inner diameter [mm]</source>
      <translation>Diámetro interior [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Constraint type</source>
      <translation>Tipo de restricción</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>Start edge type</source>
      <translation>Tipo de arista inicial</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>Start edge size</source>
      <translation>Tamaño de la arista inicial</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="54"/>
      <source>End edge type</source>
      <translation>Tipo de arista final</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>Tamaño de la arista final</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="69"/>
      <source>Shaft Wizard</source>
      <translation>Asistente de ejes</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="77"/>
      <source>Section 1</source>
      <translation>Sección 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="78"/>
      <source>Section 2</source>
      <translation>Sección 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="82"/>
      <source>Add column</source>
      <translation>Agregar columna</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="130"/>
      <source>Section %s</source>
      <translation>Sección %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="178"/>
      <source>None</source>
      <translation>Ninguno</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Fixed</source>
      <translation>Fijo</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Force</source>
      <translation>Fuerza</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Bearing</source>
      <translation>Rodamiento</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="163"/>
      <source>Gear</source>
      <translation>Engranaje</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="164"/>
      <source>Pulley</source>
      <translation>Polea</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="181"/>
      <source>Chamfer</source>
      <translation>Chaflán</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="182"/>
      <source>Fillet</source>
      <translation>Redondeo</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="60"/>
      <source>All</source>
      <translation>Todo</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="120"/>
      <source>Missing Module</source>
      <translation>Falta un módulo</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="126"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>El complemento Plot no está instalado. Instálelo para activar esta función.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="253"/>
      <source>Shaft design wizard...</source>
      <translation>Asistente de diseño de ejes...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="256"/>
      <source>Start the shaft design wizard</source>
      <translation>Inicia el asistente de diseño de ejes</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="408"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>El objeto enlazado no es una operación de PartDesign</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="417"/>
      <source>Tip shape is empty</source>
      <translation>La forma de la punta está vacía</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="80"/>
      <source>BaseFeature link is not set</source>
      <translation>El enlace BaseFeature no está definido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="100"/>
      <location filename="../../../App/FeatureBase.cpp" line="125"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeature tiene una forma vacía</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="172"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>No se puede hacer una diferencia booleana sin BaseFeature</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="194"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Solo se pueden hacer booleanas con Part::Feature y sus derivados</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="211"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>No se puede hacer una operación booleana con una forma base no válida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="266"/>
      <location filename="../../../App/FeaturePipe.cpp" line="514"/>
      <location filename="../../../App/FeaturePipe.cpp" line="539"/>
      <location filename="../../../App/FeaturePipe.cpp" line="557"/>
      <location filename="../../../App/FeatureFillet.cpp" line="142"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="196"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="535"/>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="435"/>
      <location filename="../../../App/FeatureHelix.cpp" line="453"/>
      <location filename="../../../App/FeatureDraft.cpp" line="335"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="836"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="852"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="865"/>
      <location filename="../../../App/FeatureHole.cpp" line="2144"/>
      <location filename="../../../App/FeatureLoft.cpp" line="387"/>
      <location filename="../../../App/FeatureLoft.cpp" line="422"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>El resultado tiene varios sólidos: active 'Allow Compound' en el cuerpo activo.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="221"/>
      <source>Tool shape is null</source>
      <translation>La forma de herramienta es nula</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="248"/>
      <source>Unsupported boolean operation</source>
      <translation>Operación booleana no admitida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="402"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>No se puede crear una protrusión con longitud total cero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="407"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>No se puede crear una cajera con longitud total cero.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="770"/>
      <source>No extrusion geometry was generated.</source>
      <translation>No se generó ninguna geometría de extrusión.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="794"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>La extrusión unida resultante es nula.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="549"/>
      <location filename="../../../App/FeatureHelix.cpp" line="445"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="132"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="827"/>
      <location filename="../../../App/FeatureLoft.cpp" line="415"/>
      <source>Resulting shape is not a solid</source>
      <translation>La forma resultante no es un sólido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Failed to create chamfer</source>
      <translation>No se pudo crear el chaflán</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="122"/>
      <location filename="../../../App/FeatureDraft.cpp" line="330"/>
      <source>Resulting shape is null</source>
      <translation>La forma resultante es nula</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="144"/>
      <source>No edges specified</source>
      <translation>No se especificaron aristas</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="211"/>
      <source>Chamfer failed: OCC kernel error in chamfer computation</source>
      <translation>Falló el chaflán: error del núcleo OCC al calcular el chaflán</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="302"/>
      <source>Size must be greater than zero</source>
      <translation>El tamaño debe ser mayor que cero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="313"/>
      <source>Size2 must be greater than zero</source>
      <translation>Size2 debe ser mayor que cero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="320"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>El ángulo debe ser mayor que 0 y menor que 180</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="97"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>No es posible el redondeo en las formas seleccionadas</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="105"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>El radio del redondeo debe ser mayor que cero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="159"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>Falló el redondeo. Las aristas seleccionadas pueden tener geometría que no se puede redondear en conjunto. Pruebe a redondear las aristas por separado o con un radio menor.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1785"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>No se puede crear la operación solicitada. Puede deberse a que:
  - el cuerpo activo no tiene una forma base, por lo que no hay
  material que quitar;
  - el croquis seleccionado no pertenece al cuerpo activo.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="451"/>
      <source>Failed to obtain profile shape</source>
      <translation>No se pudo obtener la forma del perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="505"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>La creación falló porque la dirección es ortogonal al vector normal del croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="298"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="528"/>
      <source>Creating a face from sketch failed</source>
      <translation>No se pudo crear una cara a partir del croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="333"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>El eje de revolución corta el croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="494"/>
      <source>No revolution geometry was generated</source>
      <translation>No se generó ninguna geometría de revolución</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="520"/>
      <source>Could not revolve the sketch!</source>
      <translation>¡No se pudo hacer la revolución del croquis!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="849"/>
      <source>Cannot project the axis because the profile is not planar</source>
      <translation>No se puede proyectar el eje porque el perfil no es plano</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="860"/>
      <source>Cannot project an axis perpendicular to the profile plane</source>
      <translation>No se puede proyectar un eje perpendicular al plano del perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="208"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>No se pudo crear una cara a partir del croquis.
No se permiten entidades que se intersequen en un croquis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="236"/>
      <source>Error: Pitch too small!</source>
      <translation>Error: ¡paso demasiado pequeño!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="241"/>
      <location filename="../../../App/FeatureHelix.cpp" line="264"/>
      <source>Error: height too small!</source>
      <translation>Error: ¡altura demasiado pequeña!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="250"/>
      <source>Error: pitch too small!</source>
      <translation>Error: ¡paso demasiado pequeño!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="255"/>
      <location filename="../../../App/FeatureHelix.cpp" line="269"/>
      <location filename="../../../App/FeatureHelix.cpp" line="278"/>
      <source>Error: turns too small!</source>
      <translation>Error: ¡muy pocas vueltas!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="284"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>Error: ¡la altura o el crecimiento no pueden ser cero!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="302"/>
      <source>Error: unsupported mode</source>
      <translation>Error: modo no admitido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="316"/>
      <source>Error: No valid sketch or face</source>
      <translation>Error: no hay un croquis o una cara válidos</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="329"/>
      <source>Error: Face must be planar</source>
      <translation>Error: la cara debe ser plana</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2507"/>
      <source>Error: Result is not a solid</source>
      <translation>Error: el resultado no es un sólido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="414"/>
      <source>Error: There is nothing to subtract</source>
      <translation>Error: no hay nada que restar</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Could not create face from sketch</source>
      <translation>Error: no se pudo crear una cara a partir del croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1270"/>
      <source>Thread type is invalid</source>
      <translation>El tipo de rosca no es válido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1798"/>
      <source>Hole error: Diameter too small</source>
      <translation>Error del agujero: diámetro demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1849"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>Error del agujero: especificación de longitud no admitida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1855"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>Error del agujero: profundidad no válida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1881"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>Error del agujero: ángulo de conicidad no válido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1905"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>Error del agujero: diámetro del corte de cabeza demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1910"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>Error del agujero: la profundidad del corte de cabeza debe ser menor que la del agujero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1917"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>Error del agujero: la profundidad del corte de cabeza debe ser mayor o igual a cero</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1947"/>
      <source>Hole error: Invalid countersink</source>
      <translation>Error del agujero: avellanado cónico no válido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>Error del agujero: ángulo de la punta de la broca no válido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2000"/>
      <source>Hole error: Invalid drill point</source>
      <translation>Error del agujero: punta de la broca no válida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2037"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>Error del agujero: no se pudo hacer la revolución del croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2044"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>Error del agujero: la forma resultante está vacía</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2067"/>
      <source>Hole error: Finding axis failed</source>
      <translation>Error del agujero: no se encontró el eje</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2116"/>
      <location filename="../../../App/FeatureHole.cpp" line="2124"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>Falló la operación booleana en la arista del perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2131"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>La operación booleana produjo un no sólido en la arista del perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="258"/>
      <source>Boolean operation failed</source>
      <translation>Falló la operación booleana</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2157"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>No se pudo crear una cara a partir del croquis.
Para hacer una cajera hasta una cara no se permiten entidades que se intersequen ni varias caras en el croquis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2330"/>
      <source>Thread type out of range</source>
      <translation>Tipo de rosca fuera de rango</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2333"/>
      <source>Thread size out of range</source>
      <translation>Tamaño de rosca fuera de rango</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2481"/>
      <source>Error: Thread could not be built</source>
      <translation>Error: no se pudo construir la rosca</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="246"/>
      <source>Loft: At least one section is needed</source>
      <translation>Recubrimiento: se necesita al menos una sección</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="439"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>Recubrimiento: ocurrió un error grave al crear el recubrimiento</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="293"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>Recubrimiento: no se pudo crear una cara a partir del croquis</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="477"/>
      <location filename="../../../App/FeatureLoft.cpp" line="357"/>
      <source>Loft: Failed to create shell</source>
      <translation>Recubrimiento: no se pudo crear la cáscara</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="880"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>No se pudo crear una cara a partir del croquis.
No se permiten entidades que se intersequen ni varias caras en un croquis.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="187"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>Barrido: no se pudo obtener la forma del perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="194"/>
      <source>No spine linked</source>
      <translation>No hay ninguna trayectoria enlazada</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="209"/>
      <source>No auxiliary spine linked.</source>
      <translation>No hay ninguna trayectoria auxiliar enlazada.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="231"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>Barrido: si se usa como sección un croquis con puntos aislados, solo se necesita un punto aislado</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="240"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>Barrido: se necesita al menos una sección cuando se usa un solo punto como perfil</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="258"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>Barrido: todas las secciones deben ser operaciones de Part</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="267"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>Barrido: no se pudo obtener la forma de la sección</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>Barrido: solo el perfil y la última sección pueden ser vértices</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="288"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>Las multisecciones deben tener la misma cantidad de alambres interiores que la sección base</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="321"/>
      <source>Path must not be a null shape</source>
      <translation>La trayectoria no puede ser una forma nula</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="361"/>
      <source>Pipe could not be built</source>
      <translation>No se pudo construir el barrido</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="509"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>Barrido: no hay nada de lo que restar</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="571"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>Ocurrió un error grave al crear el barrido</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Invalid element in spine.</source>
      <translation>Elemento no válido en la trayectoria.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="703"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>Un elemento de la trayectoria no es ni una arista ni un alambre.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="716"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>La trayectoria no es ni una arista ni un alambre.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="721"/>
      <source>Invalid spine.</source>
      <translation>Trayectoria no válida.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="103"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>No se puede restar una primitiva sin operación base</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="124"/>
      <location filename="../../../App/FeatureLoft.cpp" line="408"/>
      <source>Failed to perform boolean operation</source>
      <translation>No se pudo realizar la operación booleana</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="208"/>
      <source>Length of box too small</source>
      <translation>Longitud de la caja demasiado pequeña</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="213"/>
      <source>Width of box too small</source>
      <translation>Ancho de la caja demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="218"/>
      <source>Height of box too small</source>
      <translation>Altura de la caja demasiado pequeña</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="266"/>
      <source>Radius of cylinder too small</source>
      <translation>Radio del cilindro demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="271"/>
      <source>Height of cylinder too small</source>
      <translation>Altura del cilindro demasiado pequeña</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="276"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>Ángulo de rotación del cilindro demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="333"/>
      <source>Radius of sphere too small</source>
      <translation>Radio de la esfera demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="385"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="390"/>
      <source>Radius of cone cannot be negative</source>
      <translation>El radio del cono no puede ser negativo</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="395"/>
      <source>Height of cone too small</source>
      <translation>Altura del cono demasiado pequeña</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="475"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="480"/>
      <source>Radius of ellipsoid too small</source>
      <translation>Radio del elipsoide demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="574"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="579"/>
      <source>Radius of torus too small</source>
      <translation>Radio del toroide demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="664"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>El polígono del prisma no es válido; debe tener 3 lados o más</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="669"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>El radio circunscrito del polígono del prisma es demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="674"/>
      <source>Height of prism is too small</source>
      <translation>Altura del prisma demasiado pequeña</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="761"/>
      <source>delta x of wedge too small</source>
      <translation>delta x de la cuña demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="767"/>
      <source>delta y of wedge too small</source>
      <translation>delta y de la cuña demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="773"/>
      <source>delta z of wedge too small</source>
      <translation>delta z de la cuña demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="779"/>
      <source>delta z2 of wedge is negative</source>
      <translation>delta z2 de la cuña es negativo</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="785"/>
      <source>delta x2 of wedge is negative</source>
      <translation>delta x2 de la cuña es negativo</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="246"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="261"/>
      <source>Angle of revolution too large</source>
      <translation>Ángulo de revolución demasiado grande</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="254"/>
      <source>Angle of revolution too small</source>
      <translation>Ángulo de revolución demasiado pequeño</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="292"/>
      <source>Reference axis is invalid</source>
      <translation>El eje de referencia no es válido</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="819"/>
      <source>Fusion with base feature failed</source>
      <translation>Falló la unión con la operación base</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>Operación de transformación: el objeto enlazado no es un objeto de Part</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="120"/>
      <source>No features selected to be mirrored.</source>
      <translation>No hay operaciones seleccionadas para aplicar simetría.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="127"/>
      <source>No features selected to be patterned.</source>
      <translation>No hay operaciones seleccionadas para el patrón.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="130"/>
      <source>No features selected to be transformed.</source>
      <translation>No hay operaciones seleccionadas para transformar.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="447"/>
      <source>Cannot transform invalid support shape</source>
      <translation>No se puede transformar una forma de soporte no válida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="561"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>La forma de la operación aditiva/sustractiva está vacía</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="552"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>Solo se pueden transformar operaciones aditivas y sustractivas</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="181"/>
      <source>Invalid face reference</source>
      <translation>Referencia de cara no válida</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDefeaturing.cpp" line="75"/>
      <source>Defeaturing failed: result is null</source>
      <translation>Falló la supresión de caras: el resultado es nulo</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDefeaturing.cpp" line="84"/>
      <source>Defeaturing did not produce a single solid</source>
      <translation>La supresión de caras no produjo un solo sólido</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Involute Gear</source>
      <translation>Engranaje de evolvente</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="66"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>Crea o edita la definición del engranaje de evolvente</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="65"/>
      <source>Sprocket</source>
      <translation>Rueda dentada</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="69"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>Crea o edita la definición de la rueda dentada.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>Mostrar el resultado final</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>Mostrar la superposición de vista previa</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="52"/>
      <source>Preview</source>
      <translation>Vista previa</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="225"/>
      <source>Shaft Design Wizard</source>
      <translation>Asistente de diseño de ejes</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="228"/>
      <source>Starts the shaft design wizard</source>
      <translation>Inicia el asistente de diseño de ejes</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="141"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>Error al calcular la vista previa del volumen quitado: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="180"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>La forma resultante está vacía. Puede indicar que no se quitará material o que hay un problema en el modelo.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2928"/>
      <source>Create Datum</source>
      <translation>Crear referencia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2929"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Crea un objeto de referencia o un sistema de coordenadas local</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2963"/>
      <source>Create Datum</source>
      <translation>Crear referencia</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2964"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>Crea un objeto de referencia o un sistema de coordenadas local</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>Crea una caja aditiva a partir de su ancho, altura y longitud</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>Crea un cilindro aditivo a partir de su radio, altura y ángulo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="235"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>Crea una esfera aditiva a partir de su radio y varios ángulos</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <source>Creates an additive cone</source>
      <translation>Crea un cono aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="250"/>
      <source>Creates an additive ellipsoid</source>
      <translation>Crea un elipsoide aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="256"/>
      <source>Creates an additive torus</source>
      <translation>Crea un toroide aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="262"/>
      <source>Creates an additive prism</source>
      <translation>Crea un prisma aditivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="268"/>
      <source>Creates an additive wedge</source>
      <translation>Crea una cuña aditiva</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="402"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>Crea una caja sustractiva a partir de su ancho, altura y longitud</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="411"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>Crea un cilindro sustractivo a partir de su radio, altura y ángulo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="420"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>Crea una esfera sustractiva a partir de su radio y varios ángulos</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="429"/>
      <source>Creates a subtractive cone</source>
      <translation>Crea un cono sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="435"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>Crea un elipsoide sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="441"/>
      <source>Creates a subtractive torus</source>
      <translation>Crea un toroide sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="447"/>
      <source>Creates a subtractive prism</source>
      <translation>Crea un prisma sustractivo</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="453"/>
      <source>Creates a subtractive wedge</source>
      <translation>Crea una cuña sustractiva</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1202"/>
      <source>Attachment</source>
      <translation>Anclaje</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="1116"/>
      <source>Revolution Parameters</source>
      <translation>Parámetros de la revolución</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="1126"/>
      <source>Groove Parameters</source>
      <translation>Parámetros de la garganta</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="39"/>
      <source>Transformed Feature Messages</source>
      <translation>Mensajes de la operación transformada</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="220"/>
      <source>Active Body</source>
      <translation>Cuerpo activo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="44"/>
      <source>Chamfer Parameters</source>
      <translation>Parámetros del chaflán</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation>Parámetros del plano de referencia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation>Parámetros de la línea de referencia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation>Parámetros del punto de referencia</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation>Parámetros del sistema de coordenadas local</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="45"/>
      <source>Draft Parameters</source>
      <translation>Parámetros del ángulo de salida</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="44"/>
      <source>Fillet Parameters</source>
      <translation>Parámetros del redondeo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="41"/>
      <source>Linear Pattern Parameters</source>
      <translation>Parámetros del patrón lineal</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="41"/>
      <source>Mirror Parameters</source>
      <translation>Parámetros de la simetría</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="41"/>
      <source>Multi-Transform Parameters</source>
      <translation>Parámetros de la transformación múltiple</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="41"/>
      <source>Polar Pattern Parameters</source>
      <translation>Parámetros del patrón polar</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="41"/>
      <source>Scale Parameters</source>
      <translation>Parámetros de escala</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="44"/>
      <source>Thickness Parameters</source>
      <translation>Parámetros del espesor</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="415"/>
      <source>Select a sketch or shape containing the pattern points</source>
      <translation>Seleccione un croquis o una forma que contenga los puntos del patrón</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="421"/>
      <source>Select a sketch, Sub-Shape Binder, or path edge</source>
      <translation>Seleccione un croquis, un vinculador de subforma o una arista de trayectoria</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="430"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>Seleccione una referencia de dirección (arista, cara, línea de referencia)</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="494"/>
      <source>Invalid selection. Select a sketch or shape containing points.</source>
      <translation>Selección no válida. Seleccione un croquis o una forma que contenga puntos.</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="497"/>
      <source>Invalid selection. Select a sketch, Sub-Shape Binder, or path edge.</source>
      <translation>Selección no válida. Seleccione un croquis, un vinculador de subforma o una arista de trayectoria.</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="499"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>Selección no válida. Seleccione una arista, una cara plana o una línea de referencia.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDefeaturingParameters</name>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="20"/>
      <source>Toggles selection mode</source>
      <translation>Alterna el modo de selección</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="23"/>
      <source>Select</source>
      <translation>Seleccionar</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.ui" line="33"/>
      <source>Select an item in the 3D view to highlight it</source>
      <translation>Seleccione un elemento en la vista 3D para resaltarlo</translation>
    </message>
    <message>
      <location filename="../../TaskDefeaturingParameters.cpp" line="118"/>
      <source>Empty defeaturing created</source>
      <translation>Se creó una supresión de caras vacía</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDefeaturing</name>
    <message>
      <location filename="../../Command.cpp" line="2132"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2133"/>
      <source>Defeaturing</source>
      <translation>Supresión de caras</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2134"/>
      <source>Removes selected faces from a solid</source>
      <translation>Quita las caras seleccionadas de un sólido</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCircularPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2489"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2490"/>
      <source>Circular Pattern</source>
      <translation>Patrón circular</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2491"/>
      <source>Duplicates the selected features or the active body in concentric circular patterns</source>
      <translation>Duplica las operaciones seleccionadas o el cuerpo activo en patrones circulares concéntricos</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPathPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2546"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2547"/>
      <source>Path Pattern</source>
      <translation>Patrón por trayectoria</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2548"/>
      <source>Duplicates the selected features or the active body along a path</source>
      <translation>Duplica las operaciones seleccionadas o el cuerpo activo a lo largo de una trayectoria</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPointPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2584"/>
      <source>PartDesign</source>
      <translation>Diseño de parte</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2585"/>
      <source>Point Pattern</source>
      <translation>Patrón por puntos</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2586"/>
      <source>Duplicates the selected features or the active body at points from a shape</source>
      <translation>Duplica las operaciones seleccionadas o el cuerpo activo en los puntos de una forma</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDefeaturing</name>
    <message>
      <location filename="../../ViewProviderDefeaturing.h" line="40"/>
      <source>Defeaturing Parameters</source>
      <translation>Parámetros de la supresión de caras</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPathPattern</name>
    <message>
      <location filename="../../ViewProviderPathPattern.h" line="18"/>
      <source>Path Pattern Parameters</source>
      <translation>Parámetros del patrón por trayectoria</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPathPattern.cpp" line="11"/>
      <source>Edit Path Pattern</source>
      <translation>Editar patrón por trayectoria</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="138"/>
      <source>%1 fine dragging</source>
      <translation>%1 arrastre fino</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="141"/>
      <source>%1 coarse dragging</source>
      <translation>%1 arrastre grueso</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="264"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Adjust the parameters and try again.</source>
      <translation>No se pudo crear la operación con los parámetros indicados.
La geometría puede no ser válida o los parámetros pueden ser incompatibles.
Ajuste los parámetros e inténtelo de nuevo.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="443"/>
      <source>Input Error</source>
      <translation>Error de entrada</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderCircularPattern</name>
    <message>
      <location filename="../../ViewProviderCircularPattern.h" line="18"/>
      <source>Circular Pattern Parameters</source>
      <translation>Parámetros del patrón circular</translation>
    </message>
    <message>
      <location filename="../../ViewProviderCircularPattern.cpp" line="11"/>
      <source>Edit Circular Pattern</source>
      <translation>Editar patrón circular</translation>
    </message>
  </context>
  <context>
    <name>BooleanToolSelection</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="83"/>
      <source>The result Body cannot be used as a Boolean tool.</source>
      <translation>El cuerpo resultante no se puede usar como herramienta booleana.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPointPattern</name>
    <message>
      <location filename="../../ViewProviderPointPattern.h" line="18"/>
      <source>Point Pattern Parameters</source>
      <translation>Parámetros del patrón por puntos</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPointPattern.cpp" line="11"/>
      <source>Edit Point Pattern</source>
      <translation>Editar patrón por puntos</translation>
    </message>
  </context>
</TS>
