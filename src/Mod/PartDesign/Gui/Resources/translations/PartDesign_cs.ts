<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="cs" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="72"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>Středový bod počátku šroubovice; odvozený ze vztažné osy.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="74"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>Směr šroubovice odvozený ze vztažné osy.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="76"/>
      <source>The reference axis of the helix.</source>
      <translation>Vztažná osa šroubovice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="78"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>Zadávací mód šroubovice specifikuje, které vlastnosti jsou nastaveny uživatelem.
Závislé vlastnosti jsou dopočítané.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="82"/>
      <source>The axial distance between two turns.</source>
      <translation>Osová vzdálenost mezi dvěma otočkami.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="84"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>Výška trasy šroubovice, neuvažuje velikost profilu.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="86"/>
      <source>The number of turns in the helix.</source>
      <translation>Počet závitů šroubovice.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="89"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, nevatige shrink.</source>
      <translation>Úhel kuželu tvořící obálku kolem šroubovice.
Nenulové hodnoty udělají ze šroubovice kónickou spirálu.
Kladné hodnoty způsobí rozšiřování, záporné zužování.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="94"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>Růst poloměru šroubovice na otočce.
Nenulové hodnoty udělají ze šroubovice kónickou spirálu.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>Nastaví směr otáčení doleva,
tj. proti směru hodinových ručiček při pohybu podél osy.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="100"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>Určuje, zda spirála směřuje opačným směrem než osa.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="102"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>Pokud je nastaveno, výsledkem bude průsečík profilu a již existujícího těla.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>Pokud neplatí, nástroj navrhne počáteční hodnotu pro rozteč na základě ohraničujícího kvádru profilu,
aby se zabránilo sebe.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Additive helix</source>
      <translation>Přičíst šroubovici</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1747"/>
      <source>Sweep a selected sketch along a helix</source>
      <translation>Táhnout vybranou skicu podél šroubovice</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1644"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1645"/>
      <source>Additive loft</source>
      <translation>Součtové profilování</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1646"/>
      <source>Loft a selected profile through other profile sections</source>
      <translation>Profilovat skrz vybrané profilové řezy</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1542"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1543"/>
      <source>Additive pipe</source>
      <translation>Aditivní potrubí</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1544"/>
      <source>Sweep a selected sketch along a path or to other profiles</source>
      <translation>Táhnout vybraný náčrt podél trasy nebo podél dalšího profilu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="85"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="86"/>
      <source>Create body</source>
      <translation>Vytvořit těleso</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="87"/>
      <source>Create a new body and make it active</source>
      <translation>Vytvořit nové tělo a aktivovat ho</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2638"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2639"/>
      <source>Boolean operation</source>
      <translation>Booleovské operace</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2640"/>
      <source>Boolean operation with two or more bodies</source>
      <translation>Logická operace se dvěma nebo více objekty</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="245"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="246"/>
      <source>Create a local coordinate system</source>
      <translation>Vytvořit lokální souřadnicový systém</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="247"/>
      <source>Create a new local coordinate system</source>
      <translation>Vytvořit nový lokální souřadnicový systém</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="2038"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2039"/>
      <source>Chamfer</source>
      <translation>Sražení</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2040"/>
      <source>Chamfer the selected edges of a shape</source>
      <translation>Srazí vybrané hrany útvaru</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="427"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="428"/>
      <source>Create a clone</source>
      <translation>Vytvořit kopii</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="429"/>
      <source>Create a new clone</source>
      <translation>Vytvořit nový klon</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2067"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2068"/>
      <source>Draft</source>
      <translation>Ponor</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2069"/>
      <source>Make a draft on a face</source>
      <translation>Provede zkosení stěny</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="602"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="603"/>
      <source>Duplicate selected object</source>
      <translation>Duplikovat vybraný objekt</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="604"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>Duplikuje vybraný objekt a přidá jej k aktivnímu tělu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="2010"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2011"/>
      <source>Fillet</source>
      <translation>Zaoblení</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2012"/>
      <source>Make a fillet on an edge, face or body</source>
      <translation>Vytvoří zaoblení na hraně, ploše nebo těle</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1475"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1476"/>
      <source>Groove</source>
      <translation>Vybrání</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1477"/>
      <source>Groove a selected sketch</source>
      <translation>Vybrání z vybranho náčrtu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1369"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1370"/>
      <source>Hole</source>
      <translation>Otvor</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1371"/>
      <source>Create a hole with the selected sketch</source>
      <translation>Vytvořit otvor pomocí vybraného náčrtu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Create a datum line</source>
      <translation>Vytvořit pomocnou čáru</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Create a new datum line</source>
      <translation>Vytvořit novnou pomocnou čáru</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2336"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2337"/>
      <source>LinearPattern</source>
      <translation>Lineární pole</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2338"/>
      <source>Create a linear pattern feature</source>
      <translation>Vytvořit prvek lineárního pole</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="308"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="309"/>
      <source>Migrate</source>
      <translation>Migrace</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="310"/>
      <source>Migrate document to the modern PartDesign workflow</source>
      <translation>Migrovat dokument na moderní postup PartDesign</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2274"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2275"/>
      <source>Mirrored</source>
      <translation>Zrcadlit</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2276"/>
      <source>Create a mirrored feature</source>
      <translation>Vytvořit zrcadlený prvek</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Move object to other body</source>
      <translation>Přesunout objekt k jinému tělu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the selected object to another body</source>
      <translation>Přesune vybraný objekt k jinému tělu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="825"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="826"/>
      <source>Move object after other object</source>
      <translation>Přesunout objekt za jiný objekt</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="827"/>
      <source>Moves the selected object and insert it after another object</source>
      <translation>Přesune vybraný objekt a umístí ho za jiný objekt</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="524"/>
      <source>Set tip</source>
      <translation>Nastavit vrchol</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="525"/>
      <source>Move the tip of the body</source>
      <translation>Přesunout vrchol těla</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2514"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2515"/>
      <source>Create MultiTransform</source>
      <translation>Vytvořit vícenásobnou transformaci</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2516"/>
      <source>Create a multitransform feature</source>
      <translation>Vytvořit vícenásobně transformovaný prvek</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="485"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="486"/>
      <source>Create sketch</source>
      <translation>Vytvoř náčrt</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="487"/>
      <source>Create a new sketch</source>
      <translation>Vytvořit nový náčrt</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1305"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1306"/>
      <source>Pad</source>
      <translation>Deska</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1307"/>
      <source>Pad a selected sketch</source>
      <translation>Přidat vysunutím vybranou skicou</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="162"/>
      <source>Create a datum plane</source>
      <translation>Vytvořit pomocnou rovinu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <source>Create a new datum plane</source>
      <translation>Vytvořit novnou pomocnou rovinu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1337"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1338"/>
      <source>Pocket</source>
      <translation>Kapsa</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1339"/>
      <source>Create a pocket with the selected sketch</source>
      <translation>Vytvořit kapsu pomocí vybraného náčrtu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="217"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="218"/>
      <source>Create a datum point</source>
      <translation>Vytvořit pomocný bod</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>Create a new datum point</source>
      <translation>Vytvořit nový pomocný bod</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2400"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2401"/>
      <source>PolarPattern</source>
      <translation>Kruhové pole</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>Create a polar pattern feature</source>
      <translation>Vytvořit prvek polárního pole</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1416"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1417"/>
      <source>Revolution</source>
      <translation>Přidat rotací</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1418"/>
      <source>Revolve a selected sketch</source>
      <translation>Vytvoří prvek rotací předvybraného náčrtu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2465"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2466"/>
      <source>Scaled</source>
      <translation>Změna měřítka</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2467"/>
      <source>Create a scaled feature</source>
      <translation>Vytvořit škálovaný prvek</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="277"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="278"/>
      <source>Create a shape binder</source>
      <translation>Vytvořit pořadač tvarů</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>Create a new shape binder</source>
      <translation>Vytvořit nový pořadač tvarů</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="343"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="344"/>
      <location filename="../../Command.cpp" line="345"/>
      <source>Create a sub-object(s) shape binder</source>
      <translation>Vytvořit tvar dílčího(ch) objektu</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1823"/>
      <source>Subtractive helix</source>
      <translation>Odečíst šroubovici</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1824"/>
      <source>Sweep a selected sketch along a helix and remove it from the body</source>
      <translation>Táhnout vybranou skicu podél šroubovice a odstranit je z těla</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1695"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1696"/>
      <source>Subtractive loft</source>
      <translation>Odečtové profilování</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1697"/>
      <source>Loft a selected profile through other profile sections and remove it from the body</source>
      <translation>Profilovat skrz vybrané profilové řezy a odstranit je z těla</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1593"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1594"/>
      <source>Subtractive pipe</source>
      <translation>Odečtové potrubí</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1595"/>
      <source>Sweep a selected sketch along a path or to other profiles and remove it from the body</source>
      <translation>Táhnout vybraný náčrt podél trasy nebo podél dalšího profilu a odstranit je z těla</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2125"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2126"/>
      <source>Thickness</source>
      <translation>Tloušťka</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2127"/>
      <source>Make a thick solid</source>
      <translation>Vytvořit skořepinu</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="68"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="69"/>
      <location filename="../../CommandPrimitive.cpp" line="70"/>
      <source>Create an additive primitive</source>
      <translation>Přidat primitivní těleso</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="199"/>
      <source>Additive Box</source>
      <translation>Přídavný kvádr</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="203"/>
      <source>Additive Cylinder</source>
      <translation>Přídavný válec</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="207"/>
      <source>Additive Sphere</source>
      <translation>Přídavná koule</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="211"/>
      <source>Additive Cone</source>
      <translation>Přídavný kužel</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Ellipsoid</source>
      <translation>Přídavný elipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="219"/>
      <source>Additive Torus</source>
      <translation>Přídavný torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="223"/>
      <source>Additive Prism</source>
      <translation>Přídavný hranol</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="227"/>
      <source>Additive Wedge</source>
      <translation>Přídavný klín</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="243"/>
      <source>PartDesign</source>
      <translation>Tvorba dílu</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <location filename="../../CommandPrimitive.cpp" line="245"/>
      <source>Create a subtractive primitive</source>
      <translation>Odečíst primitivní těleso</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="355"/>
      <source>Subtractive Box</source>
      <translation>Odečítový kvádr</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="359"/>
      <source>Subtractive Cylinder</source>
      <translation>Odečtový válec</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="363"/>
      <source>Subtractive Sphere</source>
      <translation>Odečtová koule</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="367"/>
      <source>Subtractive Cone</source>
      <translation>Odečtový kužel</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="371"/>
      <source>Subtractive Ellipsoid</source>
      <translation>Odečtový elipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="375"/>
      <source>Subtractive Torus</source>
      <translation>Odečtový torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="379"/>
      <source>Subtractive Prism</source>
      <translation>Odečtový hranol</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="383"/>
      <source>Subtractive Wedge</source>
      <translation>Odečtový klín</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="298"/>
      <source>Edit ShapeBinder</source>
      <translation>Upravit pořadač tvarů</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="307"/>
      <source>Create ShapeBinder</source>
      <translation>Vytvořit pořadač tvarů</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="390"/>
      <source>Create SubShapeBinder</source>
      <translation>Vytvořit pořadač dílčích tvarů</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="445"/>
      <source>Create Clone</source>
      <translation>Vytvořit klon</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="642"/>
      <location filename="../../Command.cpp" line="1205"/>
      <source>Make copy</source>
      <translation>Vytvořit kopii</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="666"/>
      <source>Create a Sketch on Face</source>
      <translation>Vytvořit nový náčrt na oříznuté ploše</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="687"/>
      <source>Create a new Sketch</source>
      <translation>Vytvořit nový náčrt</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2564"/>
      <source>Convert to MultiTransform feature</source>
      <translation>Převést na multitransformační prvek</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2656"/>
      <source>Create Boolean</source>
      <translation>Použít booleovské operace</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="104"/>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>Add a Body</source>
      <translation>Přidat tělo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="426"/>
      <source>Migrate legacy part design features to Bodies</source>
      <translation>Převést staré funkce tvorby dílu na Těla</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="571"/>
      <source>Move tip to selected feature</source>
      <translation>Přesunout pracovní pozici na vybraný prvek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Duplicate a PartDesign object</source>
      <translation>Duplikovat objekt PartDesign</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="739"/>
      <source>Move an object</source>
      <translation>Přesunout objekt</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="890"/>
      <source>Move an object inside tree</source>
      <translation>Přesunout objekt dovnitř stromu</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="292"/>
      <source>Mirrored</source>
      <translation>Zrcadlit</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="320"/>
      <source>Make LinearPattern</source>
      <translation>Vytvořit lineární pole</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="358"/>
      <source>PolarPattern</source>
      <translation>Kruhové pole</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
      <source>Scaled</source>
      <translation>Změna měřítka</translation>
    </message>
  </context>
  <context>
    <name>FeaturePickDialog</name>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="42"/>
      <source>Valid</source>
      <translation>Platný</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="43"/>
      <source>Invalid shape</source>
      <translation>Neplatý tvar</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="44"/>
      <source>No wire in sketch</source>
      <translation>V náčrtu není drát</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="45"/>
      <source>Sketch already used by other feature</source>
      <translation>Náčrt je již použitý jiným prvkem</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="46"/>
      <source>Sketch belongs to another Body feature</source>
      <translation>Náčrt patří k prvku jiného těla</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="47"/>
      <source>Base plane</source>
      <translation>Základní rovina</translation>
    </message>
    <message>
      <location filename="../../FeaturePickDialog.cpp" line="48"/>
      <source>Feature is located after the Tip feature</source>
      <translation>Prvek se nachází za vrchním prvkem</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Face tools</source>
      <translation>Nástroje tvorby ploch</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Sketch tools</source>
      <translation>Nástroje tvorby náčrtu</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Create Geometry</source>
      <translation>Vytvoř geometrii</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute parameter</source>
      <translation>Parametr evolventy</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>počet zubů:</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module:</source>
      <translation>Modul:</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle:</source>
      <translation>Úhel záběru:</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision:</source>
      <translation>Vysoká přesnost:</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>Pravda</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>Nepravda</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear:</source>
      <translation>Vnější ozubení:</translation>
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
      <translation>Nelze vytvořit požadovaný prvek. Důvodem může být:
  - aktivní Tělo neobsahuje základní tvar, takže zde
 není materiál k odstranění;
  - vybraný náčrt nepatří k aktivnímu Tělu.</translation>
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
      <translation>Nelze vytvořit požadovaný prvek. Důvodem může být:
  - aktivní Tělo neobsahuje základní tvar, takže zde
 není materiál k odstranění;
  - vybraný náčrt nepatří k aktivnímu Tělu.</translation>
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
      <translation>Nelze vytvořit požadovaný prvek. Důvodem může být:
  - aktivní Tělo neobsahuje základní tvar, takže zde
 není materiál k odstranění;
  - vybraný náčrt nepatří k aktivnímu Tělu.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>Je potřeba aktivní Tělo</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document.

Please select a body from below, or create a new body.</source>
      <translation>Pro vytvoření nového PartDesign objektu musí být v dokumentu aktivní Tělo.

Aktivujte prosím tělo níže nebo vytvořte nové tělo.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="36"/>
      <source>Create new body</source>
      <translation>Vytvořit nové těleso</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="55"/>
      <source>Please select</source>
      <translation>Vyberte, prosím</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>Základní geometrické útvary</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length:</source>
      <translation>Délka:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width:</source>
      <translation>Šířka:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height:</source>
      <translation>Výška:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius:</source>
      <translation>Poloměr:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <source>Angle in first direction:</source>
      <translation>Úhel v prvním směru:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>Úhel v prvním směru</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <source>Angle in second direction:</source>
      <translation>Úhel v druhém směru:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>Úhel v druhém směru</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle:</source>
      <translation>Úhel otáčení:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1:</source>
      <translation>Poloměr 1:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2:</source>
      <translation>Poloměr 2:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle:</source>
      <translation>Úhel:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <source>U parameter:</source>
      <translation>Parametr U:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters:</source>
      <translation>Parametry V:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>Poloměr v lokálním Z směru</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local x-direction</source>
      <translation>Poloměr v lokálním X směru</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3:</source>
      <translation>Poloměr 3:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local y-direction
If zero, it is equal to Radius2</source>
      <translation>Poloměr v lokálním Y směru
Je-li nulový, rovná se Poloměr2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter:</source>
      <translation>Parametr V:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local xy-plane</source>
      <translation>Poloměr v lokální rovině XY</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local xz-plane</source>
      <translation>Poloměr v lokální rovině XZ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U Parameter:</source>
      <translation>Parametr U:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon:</source>
      <translation>Mnohoúhelník:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius:</source>
      <translation>Kruhový rádius:</translation>
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
      <translation>X 2 min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max:</source>
      <translation>Z2 min/max:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch:</source>
      <translation>Rozteč:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system:</source>
      <translation>Systém souřadnic:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>Pravou rukou</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>Levou rukou</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth:</source>
      <translation>Vybrání:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations:</source>
      <translation>Počet otočení:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1:</source>
      <translation>Úhel 1:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2:</source>
      <translation>Úhel 2:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From three points</source>
      <translation>Ze tří bodů</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius:</source>
      <translation>Hlavní poloměr:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius:</source>
      <translation>Vedlejší poloměr:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2005"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2093"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2170"/>
      <source>X:</source>
      <translation>X:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2025"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2113"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2193"/>
      <source>Y:</source>
      <translation>Y:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2045"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2133"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2216"/>
      <source>Z:</source>
      <translation>Z:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>Počáteční bod</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>Koncový bod</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>Odkaz</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>Vybrali jste geometrie, které nejsou součástí aktivního těla. Definujte, jak s těmito výběry nakládat. Pokud nechcete tyto odkazy, zrušte příkaz.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>Vytvořit nezávislou kopii (doporučeno)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>Závislá kopie</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>Vytovřit křížový odkaz</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="270"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>Tento výběr způsobí kruhovou závislost.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add body</source>
      <translation>Přidat tělo</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove body</source>
      <translation>Odstranit tělo</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>Sloučit</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>Řezat</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>Průnik</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="52"/>
      <source>Boolean parameters</source>
      <translation>Booleovské parametry</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="81"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="49"/>
      <source>Primitive parameters</source>
      <translation>Parametry primitivního tělesa</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="718"/>
      <source>Cone radii are equal</source>
      <translation>Poloměr kužele je roven</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="719"/>
      <source>The radii for cones must not be equal!</source>
      <translation>Poloměry kužele se nesmí rovnat!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="794"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="799"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="804"/>
      <source>Invalid wedge parameters</source>
      <translation>Neplatné parametry klínu</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="795"/>
      <source>X min must not be equal to X max!</source>
      <translation>X min se nesmí rovnat X max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="800"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y min se nesmí rovnat Y max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="805"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z min se nesmí rovnat Z max!</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="843"/>
      <source>Create primitive</source>
      <translation>Vytvořit primitivní těleso</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="22"/>
      <location filename="../../TaskChamferParameters.ui" line="36"/>
      <location filename="../../TaskChamferParameters.cpp" line="170"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Klikněte na tlačítko pro vstup do módu výběru,
klikněte znovu pro ukončení výběru</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="26"/>
      <source>Add</source>
      <translation>Přidat</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="40"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- vyberte položku pro zvýraznění
- dvakrát klikněte na položku pro zobrazení zkosení</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="67"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="75"/>
      <source>Equal distance</source>
      <translation>Rovná vzdálenost</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="80"/>
      <source>Two distances</source>
      <translation>Dvě vzdálenosti</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="85"/>
      <source>Distance and angle</source>
      <translation>Vzdálenost a úhel</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="98"/>
      <source>Flip direction</source>
      <translation>Překlopit směr</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Size</source>
      <translation>Velikost</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="138"/>
      <source>Use All Edges</source>
      <translation>Použít všechny hrany</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="165"/>
      <source>Size 2</source>
      <translation>Velikost 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="198"/>
      <source>Angle</source>
      <translation>Úhel</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="180"/>
      <location filename="../../TaskChamferParameters.cpp" line="182"/>
      <location filename="../../TaskChamferParameters.cpp" line="257"/>
      <location filename="../../TaskChamferParameters.cpp" line="259"/>
      <source>There must be at least one item</source>
      <translation>Musí existovat alespoň jedna položka</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="225"/>
      <source>Selection error</source>
      <translation>Chyba výběru</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="225"/>
      <source>At least one item must be kept.</source>
      <translation>Musí být zachována alespoň jedna položka.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="352"/>
      <source>Empty body list</source>
      <translation>Prázdný seznam těla</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="353"/>
      <source>The body list cannot be empty</source>
      <translation>Seznam těla nemůže být prázdný</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="364"/>
      <source>Boolean: Accept: Input error</source>
      <translation>Boolean: Přimutí: Vstupní chyba</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="104"/>
      <source>Incompatible reference set</source>
      <translation>Nekompatibilí referenční množina</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>Neexistuje režim připojení odpovídající uktuální množině odkazů. Budete-li pokračovat, prvek zůstane tam, kde se právě nachází a nepřesune se při změně referencí. Pokračovat?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="133"/>
      <source>Input error</source>
      <translation>Chyba zadání</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="331"/>
      <source>Input error</source>
      <translation>Chyba zadání</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="22"/>
      <location filename="../../TaskDraftParameters.ui" line="36"/>
      <location filename="../../TaskDraftParameters.cpp" line="137"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Klikněte na tlačítko pro vstup do módu výběru,
klikněte znovu pro ukončení výběru</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="26"/>
      <source>Add face</source>
      <translation>Přidat stěnu</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="40"/>
      <source>Remove face</source>
      <translation>Odstranit stěnu</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- vyberte položku pro zvýraznění
- dvakrát klikněte na položku pro zobrazení zkosení</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="65"/>
      <source>Draft angle</source>
      <translation>Úhel zkosení</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="98"/>
      <source>Neutral plane</source>
      <translation>Neutrální rovina</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="115"/>
      <source>Pull direction</source>
      <translation>Směr zkosení</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="130"/>
      <source>Reverse pull direction</source>
      <translation>Obrátit směr zkosení</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="147"/>
      <location filename="../../TaskDraftParameters.cpp" line="149"/>
      <location filename="../../TaskDraftParameters.cpp" line="273"/>
      <location filename="../../TaskDraftParameters.cpp" line="275"/>
      <source>There must be at least one item</source>
      <translation>Musí existovat alespoň jedna položka</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="241"/>
      <source>Selection error</source>
      <translation>Chyba výběru</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="241"/>
      <source>At least one item must be kept.</source>
      <translation>Musí být zachována alespoň jedna položka.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="276"/>
      <source>Add all edges</source>
      <translation>Přidat všechny hrany</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="284"/>
      <source>Adds all edges to the list box (active only when in add selection mode).</source>
      <translation>Přidá všechny hrany do pole seznamu (aktivní pouze v režimu přidávání).</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="293"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <location filename="../../TaskDressUpParameters.cpp" line="305"/>
      <source>There must be at least one item</source>
      <translation>Musí existovat alespoň jedna položka</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="53"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="723"/>
      <source>No face selected</source>
      <translation>Nevybrána žádná stěna</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="156"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="737"/>
      <source>Face</source>
      <translation>Plocha</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="360"/>
      <source>Sketch normal</source>
      <translation>Normála náčrtu</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="362"/>
      <source>Face normal</source>
      <translation>Normála plochy</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="365"/>
      <source>Select reference...</source>
      <translation>Vyber referenci...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="369"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="371"/>
      <source>Custom direction</source>
      <translation>Vlastní směr</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>Povolit použité prvky</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow external features</source>
      <translation>Povolit externí prvky</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>Z jiných těl stejného dílu</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>Z jiných dílů nebo volných prvků</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>Vytvořit nezávislou kopii (doporučeno)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>Závislá kopie</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>Vytovřit křížový odkaz</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="61"/>
      <source>Valid</source>
      <translation>Platný</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="62"/>
      <source>Invalid shape</source>
      <translation>Neplatý tvar</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="63"/>
      <source>No wire in sketch</source>
      <translation>V náčrtu není drát</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="64"/>
      <source>Sketch already used by other feature</source>
      <translation>Náčrt je již použitý jiným prvkem</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="65"/>
      <source>Belongs to another body</source>
      <translation>Patří k jinému tělu</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="66"/>
      <source>Belongs to another part</source>
      <translation>Patří k jinému dílu</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Doesn't belong to any body</source>
      <translation>Nepatří k žádnému tělu</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="68"/>
      <source>Base plane</source>
      <translation>Základní rovina</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Feature is located after the tip feature</source>
      <translation>Prvek se nachází za vrchním prvkem</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="80"/>
      <source>Select feature</source>
      <translation>Vybrat prvek</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="22"/>
      <location filename="../../TaskFilletParameters.ui" line="36"/>
      <location filename="../../TaskFilletParameters.cpp" line="124"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Klikněte na tlačítko pro vstup do módu výběru,
klikněte znovu pro ukončení výběru</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="26"/>
      <source>Add</source>
      <translation>Přidat</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="40"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- vyberte položku pro zvýraznění
- dvakrát klikněte na položku pro zobrazení zaoblení</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="65"/>
      <source>Radius:</source>
      <translation>Poloměr:</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="81"/>
      <source>Use All Edges</source>
      <translation>Použít všechny hrany</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="134"/>
      <location filename="../../TaskFilletParameters.cpp" line="136"/>
      <location filename="../../TaskFilletParameters.cpp" line="211"/>
      <location filename="../../TaskFilletParameters.cpp" line="213"/>
      <source>There must be at least one item</source>
      <translation>Musí existovat alespoň jedna položka</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="179"/>
      <source>Selection error</source>
      <translation>Chyba výběru</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="179"/>
      <source>At least one item must be kept.</source>
      <translation>Musí být zachována alespoň jedna položka.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status:</source>
      <translation>Stav:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>Platný</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis:</source>
      <translation>Osa:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="232"/>
      <source>Base X axis</source>
      <translation>Základní osa X</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="233"/>
      <source>Base Y axis</source>
      <translation>Základní osa Y</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="234"/>
      <source>Base Z axis</source>
      <translation>Základní osa Z</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="215"/>
      <source>Horizontal sketch axis</source>
      <translation>Vodorovná skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="214"/>
      <source>Vertical sketch axis</source>
      <translation>Svislá skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="213"/>
      <source>Normal sketch axis</source>
      <translation>Normálová osa roviny náčrtu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="197"/>
      <source>Select reference...</source>
      <translation>Vyber referenci...</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode:</source>
      <translation>Režim:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>Rozteč-Výška-Úhel</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>Rozteč-Otočky-Úhel</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>Výška-Otočky-Úhel</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>Výška-Otočky-Růst</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch:</source>
      <translation>Rozteč:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height:</source>
      <translation>Výška:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns:</source>
      <translation>Otáček:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle:</source>
      <translation>Úhel kužele:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth:</source>
      <translation>Radiální růst:</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>Levotočivý</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>Překlopit</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>Odstraňovat vně profilu</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="51"/>
      <source>Helix parameters</source>
      <translation>Parametry šroubovice</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="217"/>
      <source>Construction line %1</source>
      <translation>Konstrukční čára %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="333"/>
      <source>Error: unsupported mode</source>
      <translation>Chyba: nepodporovaný režim</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="47"/>
      <source>Counterbore</source>
      <translation>Válcové zahloubení</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="48"/>
      <source>Countersink</source>
      <translation>Kuželové zahloubení</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="49"/>
      <source>Cheesehead (deprecated)</source>
      <translation>Válcová hlava (deprecated)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="50"/>
      <source>Countersink socket screw (deprecated)</source>
      <translation type="unfinished">Countersink socket screw (deprecated)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="51"/>
      <source>Cap screw (deprecated)</source>
      <translation type="unfinished">Cap screw (deprecated)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Hole parameters</source>
      <translation>Parametry díry</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="65"/>
      <source>None</source>
      <translation>Žádný</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="66"/>
      <source>ISO metric regular profile</source>
      <translation type="unfinished">ISO metric regular profile</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="67"/>
      <source>ISO metric fine profile</source>
      <translation>ISO metrický jemný profil</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="68"/>
      <source>UTS coarse profile</source>
      <translation>UTS hrubý profil</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="69"/>
      <source>UTS fine profile</source>
      <translation>UTS jemný profil</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>UTS extra fine profile</source>
      <translation>UTS extrémně jemný profil</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLinearPatternParameters</name>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Přidat prvek</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Odstranit prvek</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="56"/>
      <source>Direction</source>
      <translation>Směr</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="68"/>
      <source>Reverse direction</source>
      <translation type="unfinished">Reverse direction</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="77"/>
      <source>Length</source>
      <translation>Délka</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="101"/>
      <source>Occurrences</source>
      <translation>Počet výskytů</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="115"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.ui" line="124"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="105"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskLinearPatternParameters.cpp" line="333"/>
      <source>Error</source>
      <translation>Chyba</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>Přímková plocha</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>Uzavřeno</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>Objekt</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>Přidat výběr</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>Odebrat výběr</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft parameters</source>
      <translation>Parametry profilování</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Přidat prvek</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Odstranit prvek</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="56"/>
      <source>Plane</source>
      <translation>Rovina</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="70"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="79"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="101"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="245"/>
      <source>Error</source>
      <translation>Chyba</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Přidat prvek</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Odstranit prvek</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="54"/>
      <source>Transformations</source>
      <translation>Transformace</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="71"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="73"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="86"/>
      <source>Edit</source>
      <translation>Upravit</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="90"/>
      <source>Delete</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="94"/>
      <source>Add mirrored transformation</source>
      <translation>Přidat zrcadlovou transformaci</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="98"/>
      <source>Add linear pattern</source>
      <translation>Přidat lineární vzor</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Add polar pattern</source>
      <translation>Přidat polární vzor</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="106"/>
      <source>Add scaled transformation</source>
      <translation>Přidat měřítko transformace</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="110"/>
      <source>Move up</source>
      <translation>Posunout nahorů</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="114"/>
      <source>Move down</source>
      <translation>Posunout dolů</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="141"/>
      <source>Right-click to add</source>
      <translation>Přidejte pravým tlačítkem myši</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad parameters</source>
      <translation>Parametry desky</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset from face at which pad will end</source>
      <translation type="unfinished">Offset from face at which pad will end</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Reverses pad direction</source>
      <translation type="unfinished">Reverses pad direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>Dimension</source>
      <translation>Rozměr</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To last</source>
      <translation>K poslední</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To first</source>
      <translation>K další</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to face</source>
      <translation>K ploše</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Two dimensions</source>
      <translation>Dvě kóty</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="30"/>
      <source>Dimension</source>
      <translation>Rozměr</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="38"/>
      <source>Length</source>
      <translation>Délka</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="58"/>
      <source>Offset to face</source>
      <translation>Odsazení od oříznuté plochy</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="77"/>
      <source>Direction</source>
      <translation>Směr</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="85"/>
      <source>Direction/edge:</source>
      <translation>Směr/hrana:</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="92"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation type="unfinished">Set a direction or select an edge
from the model as reference</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="97"/>
      <source>Sketch normal</source>
      <translation>Normála náčrtu</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="102"/>
      <source>Select reference...</source>
      <translation>Vyber referenci...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="107"/>
      <source>Custom direction</source>
      <translation>Vlastní směr</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="117"/>
      <source>Show direction</source>
      <translation type="unfinished">Show direction</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="127"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation type="unfinished">Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="140"/>
      <source>x</source>
      <translation>x</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="147"/>
      <source>x-component of direction vector</source>
      <translation>X složka vektoru směru</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="169"/>
      <source>y</source>
      <translation>y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="176"/>
      <source>y-component of direction vector</source>
      <translation>Y složka vektoru směru</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="198"/>
      <source>z</source>
      <translation>z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="205"/>
      <source>z-component of direction vector</source>
      <translation>Z složka vektoru směru</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="236"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>Pokud není zaškrtnuto, délka bude
měřena ve stanoveném směru</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="240"/>
      <source>Length along sketch normal</source>
      <translation type="unfinished">Length along sketch normal</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="253"/>
      <source>Applies length symmetrically to sketch plane</source>
      <translation type="unfinished">Applies length symmetrically to sketch plane</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="256"/>
      <source>Symmetric to plane</source>
      <translation>Symetrický k rovině</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="263"/>
      <source>Reversed</source>
      <translation>Překlopit</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="272"/>
      <location filename="../../TaskPadPocketParameters.ui" line="317"/>
      <source>Angle to taper the extrusion</source>
      <translation type="unfinished">Angle to taper the extrusion</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="275"/>
      <source>Taper angle</source>
      <translation type="unfinished">Taper angle</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="296"/>
      <source>2nd length</source>
      <translation>2. délka</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="320"/>
      <source>2nd taper angle</source>
      <translation type="unfinished">2nd taper angle</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="341"/>
      <source>Face</source>
      <translation>Plocha</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="360"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>Mód orientace</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>Standardní</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>Pevné</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>Frenet</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>Pomocný</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>Binormála</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvelinear equivalence</source>
      <translation>Křivočará ekvivalence</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>Objekt</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>Přidat hranu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>Odebrat hranu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>Nastavte konstantní binormálový vektor pro dopočtení orientace profilů</translation>
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
      <translation>Orientace průřezu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="567"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
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
      <translation>Objekt</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner Transition</source>
      <translation>Přechod rohů</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>Transformovaný</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right Corner</source>
      <translation>Pravý roh</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round Corner</source>
      <translation>Zaoblený roh</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to sweep along</source>
      <translation>Trajektorie tažení</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add Edge</source>
      <translation>Přidat hranu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove Edge</source>
      <translation>Odebrat hranu</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe parameters</source>
      <translation>Parametry potrubí</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="85"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="425"/>
      <location filename="../../TaskPipeParameters.cpp" line="527"/>
      <source>Input error</source>
      <translation>Chyba zadání</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="425"/>
      <source>No active body</source>
      <translation>Žádné aktivní tělo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>Mód transformace</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>Konstantní</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>Víceprůřezový</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>Přidat výběr</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>Odebrat výběr</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="809"/>
      <source>Section transformation</source>
      <translation>Transformace průřezů</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="825"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket parameters</source>
      <translation>Parametry kapsy</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from face at which pocket will end</source>
      <translation type="unfinished">Offset from face at which pocket will end</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Reverses pocket direction</source>
      <translation type="unfinished">Reverses pocket direction</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>Rozměr</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="74"/>
      <source>Through all</source>
      <translation>Skrz vše</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>K další</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>K ploše</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Two dimensions</source>
      <translation>Dvě kóty</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPolarPatternParameters</name>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Přidat prvek</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Odstranit prvek</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="44"/>
      <source>List can be reordered by dragging</source>
      <translation type="unfinished">List can be reordered by dragging</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="56"/>
      <source>Axis</source>
      <translation>Osa</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="68"/>
      <source>Reverse direction</source>
      <translation type="unfinished">Reverse direction</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="77"/>
      <source>Angle</source>
      <translation>Úhel</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="107"/>
      <source>Occurrences</source>
      <translation>Počet výskytů</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="121"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.ui" line="130"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="112"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
    <message>
      <location filename="../../TaskPolarPatternParameters.cpp" line="333"/>
      <source>Error</source>
      <translation>Chyba</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="893"/>
      <source>Attachment</source>
      <translation>Připojení</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Axis:</source>
      <translation>Osa:</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="30"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="139"/>
      <source>Base X axis</source>
      <translation>Základní osa X</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="35"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="140"/>
      <source>Base Y axis</source>
      <translation>Základní osa Y</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="40"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="141"/>
      <source>Base Z axis</source>
      <translation>Základní osa Z</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="45"/>
      <source>Horizontal sketch axis</source>
      <translation>Vodorovná skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <source>Vertical sketch axis</source>
      <translation>Svislá skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="148"/>
      <source>Select reference...</source>
      <translation>Vyber referenci...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="67"/>
      <source>Angle:</source>
      <translation>Úhel:</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="101"/>
      <source>Symmetric to plane</source>
      <translation>Symetrický k rovině</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="108"/>
      <source>Reversed</source>
      <translation>Překlopit</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="122"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="49"/>
      <source>Revolution parameters</source>
      <translation>Parametry přidání rotací</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="22"/>
      <source>Add feature</source>
      <translation>Přidat prvek</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="32"/>
      <source>Remove feature</source>
      <translation>Odstranit prvek</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="53"/>
      <source>Factor</source>
      <translation>měřítko</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="67"/>
      <source>Occurrences</source>
      <translation>Počet výskytů</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="81"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="90"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.cpp" line="92"/>
      <source>Remove</source>
      <translation>Odstranit</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>Objekt</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>Přidat geometrii</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>Odstranit geometrii</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Datum shape parameters</source>
      <translation>Parametry pomocného tvaru</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="160"/>
      <source>Face</source>
      <translation>Plocha</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="22"/>
      <location filename="../../TaskThicknessParameters.ui" line="36"/>
      <location filename="../../TaskThicknessParameters.cpp" line="136"/>
      <source>Click button to enter selection mode,
click again to end selection</source>
      <translation>Klikněte na tlačítko pro vstup do módu výběru,
klikněte znovu pro ukončení výběru</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="26"/>
      <source>Add face</source>
      <translation>Přidat stěnu</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="40"/>
      <source>Remove face</source>
      <translation>Odstranit stěnu</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="52"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation type="unfinished">- select an item to highlight it
- double-click on an item to see the features</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="65"/>
      <source>Thickness</source>
      <translation>Tloušťka</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="94"/>
      <source>Mode</source>
      <translation>Způsob</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="102"/>
      <source>Skin</source>
      <translation>Skin</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="107"/>
      <source>Pipe</source>
      <translation>Potrubí</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="112"/>
      <source>Recto Verso</source>
      <translation>Rub-líc</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="120"/>
      <source>Join Type</source>
      <translation>Typ spojení</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="128"/>
      <source>Arc</source>
      <translation>oblouk</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="133"/>
      <location filename="../../TaskThicknessParameters.ui" line="143"/>
      <source>Intersection</source>
      <translation>Průnik</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="150"/>
      <source>Make thickness inwards</source>
      <translation>Vytvořit tloušťku dovnitř</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="146"/>
      <location filename="../../TaskThicknessParameters.cpp" line="148"/>
      <location filename="../../TaskThicknessParameters.cpp" line="213"/>
      <location filename="../../TaskThicknessParameters.cpp" line="215"/>
      <source>There must be at least one item</source>
      <translation>Musí existovat alespoň jedna položka</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="181"/>
      <source>Selection error</source>
      <translation>Chyba výběru</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="181"/>
      <source>At least one item must be kept.</source>
      <translation>Musí být zachována alespoň jedna položka.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed feature messages</source>
      <translation>Zprávy transformovaného prvku</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="250"/>
      <source>Normal sketch axis</source>
      <translation>Normálová osa roviny náčrtu</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="251"/>
      <source>Vertical sketch axis</source>
      <translation>Svislá skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="252"/>
      <source>Horizontal sketch axis</source>
      <translation>Vodorovná skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="254"/>
      <location filename="../../TaskTransformedParameters.cpp" line="290"/>
      <source>Construction line %1</source>
      <translation>Konstrukční čára %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="268"/>
      <source>Base X axis</source>
      <translation>Základní osa X</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="269"/>
      <source>Base Y axis</source>
      <translation>Základní osa Y</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="270"/>
      <source>Base Z axis</source>
      <translation>Základní osa Z</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="277"/>
      <location filename="../../TaskTransformedParameters.cpp" line="313"/>
      <source>Select reference...</source>
      <translation>Vyber referenci...</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="304"/>
      <source>Base XY plane</source>
      <translation>Základní rovina XY</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="305"/>
      <source>Base YZ plane</source>
      <translation>Základní rovina YZ</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="306"/>
      <source>Base XZ plane</source>
      <translation>Základní rovina XZ</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="133"/>
      <source>Toggle active body</source>
      <translation>Přepnout aktivní tělo</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="41"/>
      <source>Chamfer parameters</source>
      <translation type="unfinished">Chamfer parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="111"/>
      <source>Datum Plane parameters</source>
      <translation type="unfinished">Datum Plane parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="116"/>
      <source>Datum Line parameters</source>
      <translation type="unfinished">Datum Line parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="121"/>
      <source>Datum Point parameters</source>
      <translation type="unfinished">Datum Point parameters</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="126"/>
      <source>Local Coordinate System parameters</source>
      <translation type="unfinished">Local Coordinate System parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="42"/>
      <source>Draft parameters</source>
      <translation type="unfinished">Draft parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="41"/>
      <source>Fillet parameters</source>
      <translation type="unfinished">Fillet parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="38"/>
      <source>LinearPattern parameters</source>
      <translation type="unfinished">LinearPattern parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="38"/>
      <source>MultiTransform parameters</source>
      <translation type="unfinished">MultiTransform parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="38"/>
      <source>PolarPattern parameters</source>
      <translation type="unfinished">PolarPattern parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="38"/>
      <source>Scaled parameters</source>
      <translation type="unfinished">Scaled parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="41"/>
      <source>Thickness parameters</source>
      <translation>Parametry tloušťky</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="38"/>
      <source>Mirrored parameters</source>
      <translation type="unfinished">Mirrored parameters</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="200"/>
      <source>Create an additive box by its width, height, and length</source>
      <translation>Vytvořit přidaný kvádr pomocí jeho šířky, výšky a délky</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="204"/>
      <source>Create an additive cylinder by its radius, height, and angle</source>
      <translation>Vytvořit přídaný válec pomocí jeho poloměru, výšky a úhlu</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="208"/>
      <source>Create an additive sphere by its radius and various angles</source>
      <translation>Vytvořit přídavnou kouli pomocí jejího poloměru a různých úhlů</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="212"/>
      <source>Create an additive cone</source>
      <translation>Vytvořit součtový kužel</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="216"/>
      <source>Create an additive ellipsoid</source>
      <translation>Vytvořit součtový elipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="220"/>
      <source>Create an additive torus</source>
      <translation>Vytvořit součtový torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Create an additive prism</source>
      <translation>Vytvořit součtový hranol</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="228"/>
      <source>Create an additive wedge</source>
      <translation>Vytvořit součtový klín</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="356"/>
      <source>Create a subtractive box by its width, height and length</source>
      <translation>Vytvořit odečtový kvádr pomocí jeho šířky, výšky a délky</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="360"/>
      <source>Create a subtractive cylinder by its radius, height and angle</source>
      <translation>Vytvořit odečtový válec pomocí jeho poloměru, výšky a úhlu</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="364"/>
      <source>Create a subtractive sphere by its radius and various angles</source>
      <translation>Vytvořit odečtovou kouli pomocí jejího poloměru a různých úhlů</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="368"/>
      <source>Create a subtractive cone</source>
      <translation>Vytvořit odečtový kužel</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="372"/>
      <source>Create a subtractive ellipsoid</source>
      <translation>Vytvořit odečtový elipsoid</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="376"/>
      <source>Create a subtractive torus</source>
      <translation>Vytvořit odečtový torus</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="380"/>
      <source>Create a subtractive prism</source>
      <translation>Vytvořit odečtový hranol</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="384"/>
      <source>Create a subtractive wedge</source>
      <translation>Vytvořit odečtový klín</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="57"/>
      <source>Involute gear...</source>
      <translation>Evolventní ozubené kolo...</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="61"/>
      <source>Creates or edit the involute gear definition.</source>
      <translation>Vytvoří nebo upraví definici "evolventního" ozubeného kola.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="728"/>
      <source>Select body</source>
      <translation>Vyberte tělo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="729"/>
      <source>Select a body from the list</source>
      <translation>Vyberte tělo ze seznamu</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="881"/>
      <source>Select feature</source>
      <translation>Vybrat prvek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="882"/>
      <source>Select a feature from the list</source>
      <translation>Vyberte prvek ze seznamu</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="953"/>
      <source>Move tip</source>
      <translation>Přesunout tip</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="954"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation type="unfinished">The moved feature appears after the currently set tip.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="955"/>
      <source>Do you want the last feature to be the new tip?</source>
      <translation type="unfinished">Do you want the last feature to be the new tip?</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="62"/>
      <source>Sprocket...</source>
      <translation>Řetězové kolo...</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="66"/>
      <source>Creates or edit the sprocket definition.</source>
      <translation>Vytvoří nebo upraví definici pastorku.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>Invalid selection</source>
      <translation>Neplatný výběr</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="138"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>Vybraným objektům neodpovídají žádné módy připojení. Vyberte něco jiného.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <location filename="../../Command.cpp" line="147"/>
      <location filename="../../Command.cpp" line="149"/>
      <source>Error</source>
      <translation>Chyba</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <source>There is no active body. Please make a body active before inserting a datum entity.</source>
      <translation>Tělo není aktivní. Prosím aktivujte tělo před výběrem pomocné entity.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="408"/>
      <source>Sub-Shape Binder</source>
      <translation type="unfinished">Sub-Shape Binder</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="590"/>
      <source>Several sub-elements selected</source>
      <translation>několik pod elementů vybráno</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="591"/>
      <source>You have to select a single face as support for a sketch!</source>
      <translation>Máte vybranou jednoduchou plochu jako podklad pro náčrt!</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="601"/>
      <source>No support face selected</source>
      <translation>Není vybrána žádná podporovaná stěna</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="602"/>
      <source>You have to select a face as support for a sketch!</source>
      <translation>Musíte vybrat stěnu jako plochu pro náčrt!</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="611"/>
      <source>No planar support</source>
      <translation>Není k dispozici podporovaná rovina</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="612"/>
      <source>You need a planar face as support for a sketch!</source>
      <translation>Potřebujete stěnu jako plochu pro náčrt!</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="797"/>
      <source>No valid planes in this document</source>
      <translation>V tomto dokumento nejsou platné roviny</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="798"/>
      <source>Please create a plane first or select a face to sketch on</source>
      <translation>Prosím vytvořte nejdříve rovinu nebo vyberte plochu pro náčrt</translation>
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
      <translation>Dialog je opravdu otevřen v panelu úloh</translation>
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
      <translation>Chcete zavřít tento dialog?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1106"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation type="unfinished">Cannot use this command as there is no solid to subtract from.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1107"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation type="unfinished">Ensure that the body contains a feature before attempting a subtractive command.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1128"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>Vybraný objekt nelze použít. Vybraný objekt musí patřit k aktivnímu tělu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1129"/>
      <source>Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</source>
      <translation type="unfinished">Consider using a ShapeBinder or a BaseFeature to reference external geometry in a body.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1151"/>
      <source>No sketch to work on</source>
      <translation>Chybí náčrt pro práci</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1152"/>
      <source>No sketch is available in the document</source>
      <translation>V dokumentu není k dispozici náčrt</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1890"/>
      <location filename="../../Command.cpp" line="1894"/>
      <location filename="../../Command.cpp" line="1920"/>
      <location filename="../../Command.cpp" line="1950"/>
      <source>Wrong selection</source>
      <translation>Neplatný výběr</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1891"/>
      <source>Select an edge, face, or body.</source>
      <translation>Vyberte hranu, plochu nebo tělo.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1895"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>Vyberte hranu, plochu nebo tělo ze samostatného těla.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1899"/>
      <location filename="../../Command.cpp" line="2252"/>
      <source>Selection is not in Active Body</source>
      <translation>Výběr není v aktivním těle</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1900"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>Vyberte hranu, plochu nebo tělo z aktivního těla.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1910"/>
      <source>Wrong object type</source>
      <translation>Špatný typ objektu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1911"/>
      <source>%1 works only on parts.</source>
      <translation>%1 funguje jen na dílech.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1921"/>
      <source>Shape of the selected Part is empty</source>
      <translation>Tvar vybraného dílu je prázdný</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1951"/>
      <source> not possible on selected faces/edges.</source>
      <translation> není možné na vybraných plochách/hranách.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2243"/>
      <source>No valid features in this document</source>
      <translation>Neplatný prvek v tomto dokumentu</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2244"/>
      <source>Please create a feature first.</source>
      <translation>Nejprve prosím vytvořte prvek.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2253"/>
      <source>Please select only one feature in an active body.</source>
      <translation type="unfinished">Please select only one feature in an active body.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="67"/>
      <source>Part creation failed</source>
      <translation>Vytvoření dílu selhalo</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="68"/>
      <source>Failed to create a part object.</source>
      <translation>Vytvoření objektu dílu selhalo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="115"/>
      <location filename="../../CommandBody.cpp" line="120"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="182"/>
      <source>Bad base feature</source>
      <translation>Špatná základní prvek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="116"/>
      <source>Body can't be based on a PartDesign feature.</source>
      <translation>Tělo nemůže být založeno na PartDesign prvku.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="121"/>
      <source>%1 already belongs to a body, can't use it as base feature for another body.</source>
      <translation>%1 již náleží k tělu, takže nelze použít jako základní prvek pro jiné tělo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>Základní prvek(%1) patří k jinému dílu.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="158"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>Vybraný tvar se skládá z několika těles.
To může vést k neočekávaným výsledkům.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="162"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>Vybraný tvar se skládá z několika skořepin.
To může vést k neočekávaným výsledkům.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="166"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>Vybraný tvar se skládá jen ze skořepiny.
To může vést k neočekávaným výsledkům.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="170"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>Vybraný tvar se skládá z několika těles nebo skořepin.
To může vést k neočekávaným výsledkům.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="175"/>
      <source>Base feature</source>
      <translation>Základní prvek</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>Tělo nemůže být založeno na více než jednom prvku.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="339"/>
      <source>Nothing to migrate</source>
      <translation>Nic k migraci</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="340"/>
      <source>No PartDesign features found that don't belong to a body. Nothing to migrate.</source>
      <translation type="unfinished">No PartDesign features found that don't belong to a body. Nothing to migrate.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="488"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>Rovina náčrtu nemůže migrovat</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="489"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>Prosím upravte '%1' a předefinujte ho za použití základny nebo pomocné roviny jako roviny náčrtu.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="551"/>
      <location filename="../../CommandBody.cpp" line="555"/>
      <location filename="../../CommandBody.cpp" line="560"/>
      <location filename="../../CommandBody.cpp" line="853"/>
      <location filename="../../CommandBody.cpp" line="860"/>
      <source>Selection error</source>
      <translation>Chyba výběru</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="552"/>
      <source>Select exactly one PartDesign feature or a body.</source>
      <translation>Vyberte právě jeden PartDesign prvek nebo tělo.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="556"/>
      <source>Couldn't determine a body for the selected feature '%s'.</source>
      <translation>Nelze určit tělo pro vybraný prvek '%s'.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="561"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>Jen prvek tělesa může být vrcholem těla.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="679"/>
      <location filename="../../CommandBody.cpp" line="701"/>
      <location filename="../../CommandBody.cpp" line="716"/>
      <source>Features cannot be moved</source>
      <translation>Prvky nemohou být přesunuty</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="680"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>Některé z vybraných prvků mají závislosti ve zdrojovém těle</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="702"/>
      <source>Only features of a single source Body can be moved</source>
      <translation>Přesunuty mohou být pouze prvky ze jednoho výchozího těla</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="717"/>
      <source>There are no other bodies to move to</source>
      <translation>Nejsou další těla k přesunutí</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="854"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>Nelze přesunout základní prvek těla.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="861"/>
      <source>Select one or more features from the same body.</source>
      <translation>Vyberte jeden nebo více prvků ze stejného těla.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="874"/>
      <source>Beginning of the body</source>
      <translation>Začátek těla</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="939"/>
      <source>Dependency violation</source>
      <translation>Porušení závislosti</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="940"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation type="unfinished">Early feature must not depend on later feature.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="269"/>
      <source>No previous feature found</source>
      <translation>Nebyl nalezen předchozí prvek</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="270"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>Není možné vytvořit odečtový prvek bez základního prvku</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="287"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="124"/>
      <source>Vertical sketch axis</source>
      <translation>Svislá skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="288"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="125"/>
      <source>Horizontal sketch axis</source>
      <translation>Vodorovná skicovací osa</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="127"/>
      <source>Construction line %1</source>
      <translation>Konstrukční čára %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="76"/>
      <source>Face</source>
      <translation>Plocha</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="122"/>
      <source>In order to use PartDesign you need an active Body object in the document. Please make one active (double click) or create one.

If you have a legacy document with PartDesign objects without Body, use the migrate function in PartDesign to put them into a Body.</source>
      <translation>Pro použití PartDesignu je potřený v dokumentu aktivní objekt Tělo. Aktivujte prosím jeden (dvojklikem) nebo ho vytvořte.

Pokud máte starší dokument s objekty PartDesignu bez Těla, použijte funkci pro migraci, která je umístí do Těla.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="169"/>
      <source>Active Body Required</source>
      <translation>Je potřeba aktivní Tělo</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="170"/>
      <source>To create a new PartDesign object, there must be an active Body object in the document. Please make one active (double click) or create a new Body.</source>
      <translation>Pro vytvoření nového PartDesign objektu musí být v dokumentu aktivní objekt Tělo. Aktivujte prosím jeden (dvojklikem) nebo vytvořte nové Tělo.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="205"/>
      <source>Feature is not in a body</source>
      <translation>Prvek není tělo</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>Pro použití tohoto prvku je potřebné, aby patřil k objektu těla v dokumentu.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="238"/>
      <source>Feature is not in a part</source>
      <translation>Prvek není díl</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="239"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>Pro použití tohoto prvku je potřebné, aby patřil k objektu díl v dokumentu.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="50"/>
      <location filename="../../ViewProvider.cpp" line="65"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="204"/>
      <location filename="../../ViewProviderTransformed.cpp" line="65"/>
      <location filename="../../ViewProviderMultiTransform.cpp" line="42"/>
      <source>Edit %1</source>
      <translation>Upravit %1</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="78"/>
      <source>Set colors...</source>
      <translation>Nastavení barev...</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="64"/>
      <source>Edit boolean</source>
      <translation>Upravit boolean</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="110"/>
      <location filename="../../ViewProviderDatum.cpp" line="204"/>
      <source>Plane</source>
      <translation>Rovina</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <location filename="../../ViewProviderDatum.cpp" line="200"/>
      <source>Line</source>
      <translation>Čára</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <location filename="../../ViewProviderDatum.cpp" line="208"/>
      <source>Point</source>
      <translation>Bod</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Coordinate System</source>
      <translation>Souřadnicový systém</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="225"/>
      <source>Edit datum</source>
      <translation>Upravit pomocné</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="71"/>
      <source>Feature error</source>
      <translation>Chyba prvku</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="72"/>
      <source>%1 misses a base feature.
This feature is broken and can't be edited.</source>
      <translation>%1 nemá základní prvek.
Tento prvek je rozbitý a nelze upravovat.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="48"/>
      <source>Edit groove</source>
      <translation>Upravit vybrání</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit hole</source>
      <translation>Upravit díru</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="70"/>
      <source>Edit loft</source>
      <translation>Upravit profilování</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="48"/>
      <source>Edit pad</source>
      <translation>Upravit desku</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="78"/>
      <source>Edit pipe</source>
      <translation>Upravit potrubí</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="50"/>
      <source>Edit pocket</source>
      <translation>Upravit kapsu</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="59"/>
      <source>Edit primitive</source>
      <translation>Upravit primitivní těleso</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="48"/>
      <source>Edit revolution</source>
      <translation>Upravit rotaci</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="199"/>
      <source>Edit shape binder</source>
      <translation>Upravit pořadač tvarů</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="308"/>
      <source>Synchronize</source>
      <translation>Synchronizovat</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="310"/>
      <source>Select bound object</source>
      <translation>Vybrat vázaný objekt</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="169"/>
      <source>One transformed shape does not intersect support</source>
      <translation>Jeden transformovaný tvar neprotíná základnu</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="171"/>
      <source>%1 transformed shapes do not intersect support</source>
      <translation>%1 transformované/ých tvary/ů neprotínají/á základnu</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="181"/>
      <source>Transformation succeeded</source>
      <translation>Transformace byla úspěšná</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="138"/>
      <source>The document "%1" you are editing was designed with an old version of PartDesign workbench.</source>
      <translation>Dokument "%1", který upravujete, byl vytvořen ve starém pracovním prostředí PartDesign.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="141"/>
      <source>Do you want to migrate in order to use modern PartDesign features?</source>
      <translation>Migrovat, aby bylo možno použít moderní PartDesign prvky?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="144"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy PartDesign or have a slightly broken structure.</source>
      <translation>Zdá se, že dokument "%1" je buď v procesu migrace ze staršího PartDesignu nebo má mírně rozbitou strukturu.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="148"/>
      <source>Do you want to make the migration automatically?</source>
      <translation>Chcete provést migraci automaticky?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="150"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation type="unfinished">Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="159"/>
      <source>Migrate manually</source>
      <translation>Migrovat manuálně</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="55"/>
      <source>Edit helix</source>
      <translation>Upravit šroubovici</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket parameter</source>
      <translation>Parametr řetězového kola</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="20"/>
      <source>Number of teeth:</source>
      <translation>počet zubů:</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="40"/>
      <source>Sprocket Reference</source>
      <translation>Reference řetězového kola</translation>
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
      <translation type="unfinished">Bicycle with Derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="123"/>
      <source>Bicycle without Derailleur</source>
      <translation type="unfinished">Bicycle without Derailleur</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="128"/>
      <source>ISO 606 06B</source>
      <translation>ISO 606 06B</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="133"/>
      <source>ISO 606 08B</source>
      <translation>ISO 606 06B</translation>
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
      <translation type="unfinished">Motorcycle 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="168"/>
      <source>Motorcycle 425</source>
      <translation type="unfinished">Motorcycle 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="173"/>
      <source>Motorcycle 428</source>
      <translation type="unfinished">Motorcycle 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="178"/>
      <source>Motorcycle 520</source>
      <translation type="unfinished">Motorcycle 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="183"/>
      <source>Motorcycle 525</source>
      <translation type="unfinished">Motorcycle 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="188"/>
      <source>Motorcycle 530</source>
      <translation type="unfinished">Motorcycle 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="193"/>
      <source>Motorcycle 630</source>
      <translation type="unfinished">Motorcycle 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Chain Pitch:</source>
      <translation>Rozteč řetězu:</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="220"/>
      <source>0 in</source>
      <translation type="unfinished">0 in</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="248"/>
      <source>Roller Diameter:</source>
      <translation>Průměr válečku:</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="292"/>
      <source>Thickness:</source>
      <translation>Tloušťka:</translation>
    </message>
  </context>
  <context>
    <name>TaskHole</name>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="24"/>
      <source>Position</source>
      <translation type="unfinished">Position</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="35"/>
      <source>Face</source>
      <translation>Plocha</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="49"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="87"/>
      <source>Edge</source>
      <translation>Hrana</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="63"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="101"/>
      <source>Distance</source>
      <translation type="unfinished">Distance</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="137"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="145"/>
      <source>Through</source>
      <translation>Přes</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="152"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="492"/>
      <source>Depth</source>
      <translation>Hloubka</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="161"/>
      <source>Threaded</source>
      <translation>Se závitem</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="168"/>
      <source>Countersink</source>
      <translation>Kuželové zahloubení</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="175"/>
      <source>Counterbore</source>
      <translation>Válcové zahloubení</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="196"/>
      <source>Hole norm</source>
      <translation>Norma díry</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="202"/>
      <source>Custom dimensions</source>
      <translation>Vlastní rozměry</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="218"/>
      <source>Tolerance</source>
      <translation>Odchylka</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="249"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="368"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="474"/>
      <source>Diameter</source>
      <translation>Průměr</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="280"/>
      <source>Bolt/Washer</source>
      <translation>Šroub/podložka</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="329"/>
      <location filename="../../../FeatureHole/TaskHole.ui" line="337"/>
      <source>Thread norm</source>
      <translation>Norma závitu</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="399"/>
      <source> Custom thread length</source>
      <translation> Vlastní délka závitu</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="423"/>
      <source>Finish depth</source>
      <translation>Dokončovací hloubka</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="466"/>
      <source>Data</source>
      <translation>Údaje</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="510"/>
      <source>Counterbore/sink dia</source>
      <translation>Průměr zahloubení</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="528"/>
      <source>Counterbore depth</source>
      <translation>Hloubka válcového zahloubení</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="546"/>
      <source>Countersink angle</source>
      <translation>Úhel kuželového zahloubení</translation>
    </message>
    <message>
      <location filename="../../../FeatureHole/TaskHole.ui" line="564"/>
      <source>Thread length</source>
      <translation>Délka závitu</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Task Hole Parameters</source>
      <translation>Parametry díry</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="26"/>
      <source>&lt;b&gt;Threading and size&lt;/b&gt;</source>
      <translation>&lt;b&gt;Závit a velikost&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="39"/>
      <source>Profile</source>
      <translation>Profil</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="62"/>
      <source>Whether the hole gets a thread</source>
      <translation>Zda díra dostane závit</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Threaded</source>
      <translation>Se závitem</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="78"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation>Zda díra dostane modelovaný závit</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="81"/>
      <source>Model Thread</source>
      <translation>Model závitu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="91"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation type="unfinished">Live update of changes to the thread
Note that the calculation can take some time</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Update view</source>
      <translation>Aktualizovat zobrazení</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="108"/>
      <source>Customize thread clearance</source>
      <translation type="unfinished">Customize thread clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="111"/>
      <source>Custom Thread</source>
      <translation type="unfinished">Custom Thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="124"/>
      <location filename="../../TaskHoleParameters.ui" line="233"/>
      <source>Clearance</source>
      <translation type="unfinished">Clearance</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="137"/>
      <source>Custom Thread clearance value</source>
      <translation type="unfinished">Custom Thread clearance value</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="159"/>
      <source>Direction</source>
      <translation>Směr</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="178"/>
      <source>Right hand</source>
      <translation>Pravý</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="194"/>
      <source>Left hand</source>
      <translation>Levý</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="210"/>
      <source>Size</source>
      <translation>Velikost</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="246"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation type="unfinished">Hole clearance
Only available for holes without thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="251"/>
      <location filename="../../TaskHoleParameters.cpp" line="586"/>
      <source>Standard</source>
      <translation>Standardní</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="256"/>
      <location filename="../../TaskHoleParameters.cpp" line="587"/>
      <location filename="../../TaskHoleParameters.cpp" line="598"/>
      <source>Close</source>
      <translation>Zavřít</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="261"/>
      <location filename="../../TaskHoleParameters.cpp" line="588"/>
      <source>Wide</source>
      <translation>Šířka</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="275"/>
      <source>Class</source>
      <translation>Třída</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="288"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation type="unfinished">Tolerance class for threaded holes according to hole profile</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="301"/>
      <location filename="../../TaskHoleParameters.ui" line="488"/>
      <source>Diameter</source>
      <translation>Průměr</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="314"/>
      <source>Hole diameter</source>
      <translation>Průměr otvoru</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="336"/>
      <location filename="../../TaskHoleParameters.ui" line="526"/>
      <source>Depth</source>
      <translation>Hloubka</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="350"/>
      <location filename="../../TaskHoleParameters.ui" line="404"/>
      <source>Dimension</source>
      <translation>Rozměr</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="355"/>
      <source>Through all</source>
      <translation>Skrz vše</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="385"/>
      <source>Thread Depth</source>
      <translation>Hloubka závitu</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>Hole depth</source>
      <translation>Hloubka otvoru</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="409"/>
      <source>Tapped (DIN76)</source>
      <translation type="unfinished">Tapped (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="433"/>
      <source>&lt;b&gt;Hole cut&lt;/b&gt;</source>
      <translation>&lt;b&gt;Řez díry&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="446"/>
      <location filename="../../TaskHoleParameters.ui" line="609"/>
      <source>Type</source>
      <translation>Typ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="459"/>
      <source>Cut type for screw heads</source>
      <translation type="unfinished">Cut type for screw heads</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="472"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation type="unfinished">Check to override the values predefined by the 'Type'</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="475"/>
      <source>Custom values</source>
      <translation>Vlastní hodnoty</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="561"/>
      <source>Countersink angle</source>
      <translation>Úhel kuželového zahloubení</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="593"/>
      <source>&lt;b&gt;Drill point&lt;/b&gt;</source>
      <translation>&lt;b&gt;Koncový bod&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="625"/>
      <source>Flat</source>
      <translation>Rovné</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="641"/>
      <source>Angled</source>
      <translation>Zkosené</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="676"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation type="unfinished">The size of the drill point will be taken into
account for the depth of blind holes</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="680"/>
      <source>Take into account for depth</source>
      <translation>Zohlednit hloubku</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="687"/>
      <source>&lt;b&gt;Misc&lt;/b&gt;</source>
      <translation>&lt;b&gt;Různé&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="700"/>
      <source>Tapered</source>
      <translation>Kuželový</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="713"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation type="unfinished">Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="738"/>
      <source>Reverses the hole direction</source>
      <translation type="unfinished">Reverses the hole direction</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="741"/>
      <source>Reversed</source>
      <translation>Překlopit</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="597"/>
      <source>Normal</source>
      <translation>Normála</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="599"/>
      <source>Loose</source>
      <translation type="unfinished">Loose</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="14"/>
      <source>Form</source>
      <translation>Návrh</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>Žádná zpráva</translation>
    </message>
  </context>
  <context>
    <name>WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="186"/>
      <location filename="../../../WizardShaft/WizardShaft.py" line="211"/>
      <source>Shaft design wizard...</source>
      <translation>Průvodce konstrukcí hřídele...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="187"/>
      <location filename="../../../WizardShaft/WizardShaft.py" line="212"/>
      <source>Start the shaft design wizard</source>
      <translation>Spusťte průvodce konstrukcí hřídele</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="43"/>
      <source>Length [mm]</source>
      <translation>Délka [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="44"/>
      <source>Diameter [mm]</source>
      <translation>Průměr [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="45"/>
      <source>Inner diameter [mm]</source>
      <translation>Vnitřní průměr [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Constraint type</source>
      <translation>Typ omezení</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Start edge type</source>
      <translation>Typ počáteční hrany</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Start edge size</source>
      <translation>Velikost počáteční hrany</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>End edge type</source>
      <translation>Typ koncové hrany</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>Velikost koncové hrany</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>&amp;Skica</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation type="unfinished">&amp;Part Design</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Create a datum</source>
      <translation type="unfinished">Create a datum</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Create an additive feature</source>
      <translation type="unfinished">Create an additive feature</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Create a subtractive feature</source>
      <translation type="unfinished">Create a subtractive feature</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Apply a pattern</source>
      <translation>Použít vzor</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Apply a dress-up feature</source>
      <translation type="unfinished">Apply a dress-up feature</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket...</source>
      <translation>Řetězové kolo...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute gear...</source>
      <translation>Evolventní ozubené kolo...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Shaft design wizard</source>
      <translation>Průvodce konstrukcí hřídele</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Measure</source>
      <translation>Měření</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Refresh</source>
      <translation>Aktualizovat</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Toggle 3D</source>
      <translation type="unfinished">Toggle 3D</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Part Design Helper</source>
      <translation type="unfinished">Part Design Helper</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Part Design Modeling</source>
      <translation type="unfinished">Part Design Modeling</translation>
    </message>
  </context>
</TS>
