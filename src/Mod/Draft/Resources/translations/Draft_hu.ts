<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="hu" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../draftobjects/wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>Drótháló sarkai</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>Ha a drótháló lezárt vagy sem</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>Az alap tárgy a drótháló, 2 tárgyból képzett</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>Az eszköz tárgy a drótháló, 2 tárgyból képzett</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>Ennek a vonalnak a kiinduló pontja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>Ennek a vonalnak a végpontja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>Ennek a vonalnak a hossza</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="52"/>
      <location filename="../../draftobjects/polygon.py" line="60"/>
      <location filename="../../draftobjects/wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>Sarkok lekerekítéséhez használt rádiusz</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="55"/>
      <location filename="../../draftobjects/polygon.py" line="64"/>
      <location filename="../../draftobjects/wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>Sarkokhoz használt letörés mérete</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>Hozzon létre egy felületet, ha ez az tárgy zárt</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>Minden él al-osztályainak száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="67"/>
      <location filename="../../draftobjects/circle.py" line="62"/>
      <location filename="../../draftobjects/polygon.py" line="72"/>
      <location filename="../../draftobjects/bspline.py" line="57"/>
      <location filename="../../draftobjects/bezcurve.py" line="70"/>
      <location filename="../../draftobjects/wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>Ennek a tárgynak a területe</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation>A vezető hegyének helyzete.
Ezt a pontot díszítheti nyíl vagy más szimbólum.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation>Objektum és opcionálisan al-elem, amelynek tulajdonságai megjelennek
mint 'Szöveg', a 'Felirattípustól' függően.

A 'Cél' nem használható, ha a 'Felirattípus' beállítása 'Egyéni'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="109"/>
      <source>The list of points defining the leader line; normally a list of three points.

The first point should be the position of the text, that is, the 'Placement',
and the last point should be the tip of the line, that is, the 'Target Point'.
The middle point is calculated automatically depending on the chosen
'Straight Direction' and the 'Straight Distance' value and sign.

If 'Straight Direction' is set to 'Custom', the 'Points' property
can be set as a list of arbitrary points.</source>
      <translation>A vezetővonal által meghatározott pontok listája; általában három pontból áll.

Az első pontnak a szövegen kell lennie, ez az 'Elhelyezés',
és az utolsó pont legyen a vonal csúcsa, azaz a 'Célpont'.
A középső pont kiszámítása automatikusan történik a kiválasztott
'Egyenes irány' és az 'Egyenes távolság' értékétől és jelétől függően.

Ha az 'Egyenes irány' értéke 'Egyéni', a 'Pontok' tulajdonsága
tetszőleges pontok listájaként állítható be.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation>A vezéregyenes egyenes szakaszának iránya.

Ha az 'Egyéni' beállítást választja, a vezető pontjait egy
a 'Pontok' tulajdonságaihoz hozzárendelt egyéni lista határozza meg.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation>A vezér egyenes szegmensének hossza.

Ez egy összehangolt távolság. Ha negatív, a vonal 
'Szöveg' alatt vagy balra, ellenkező esetben jobbra vagy felette,
az 'Egyenes iránya' tulajdonság értékétől függően.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>A 'Szöveg' elem elhelyezése a 3D térben</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>A szöveg megjelenítése, ha a 'Felirat típusa' beállítása 'Egyéni'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>A címke által megjelenített szöveg.

Ez a tulajdonság írásvédett, mivel a végleges szöveg a 'Felirattípustól' függ,
és a 'Cél' tárgyban meghatározott tárgy.
Az 'Egyéni szöveg" csak akkor jelenik meg, ha a 'Felirattípus' beállítása 'Egyéni'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>A címke által megjelenített információ típusa.

Ha az 'Egyéni' beállítást választja, a program az 'Egyéni szöveg' tartalmát használja.
Más típusok esetében a karakterlánc automatikusan kiszámításra kerül a 'Cél' mezőben definiált tárgyból.
A 'Címke' és az 'Anyag' csak olyan tárgyaknál működik, amelyek rendelkeznek ilyen tulajdonságokkal, például architektúra tárgyakkal.

A 'Pozíció', a 'Hossz1 és a 'Terület' tulajdonságokat a 'Cél' fő tárgyából
vagy a 'VégpontN', 'ÉlN' vagy 'FelületN' al-elemből, ha megadott.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="46"/>
      <source>The base object used by this object</source>
      <translation>A tárgy által használt elsődleges tárgy</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="49"/>
      <source>The PAT file used by this object</source>
      <translation>A tárgy által használt PAT-fájl</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="52"/>
      <source>The pattern name used by this object</source>
      <translation>A tárgy által használt mintanév</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="55"/>
      <source>The pattern scale used by this object</source>
      <translation>A tárgy által használt minta lépték</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="58"/>
      <source>The pattern rotation used by this object</source>
      <translation>A tárgy által használt minta elforgatás</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="61"/>
      <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
      <translation>Ha hamisra állított, a kitöltés a felületekhez hasonlóan, fordítás nélkül kerül alkalmazásra (ez rossz eredményeket adhat a nem XY felületek esetében)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>A kapcsolt objektum</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>Vetítési irány</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>Vonalvastagság ebben az objektumban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>A szöveg vastagsága ebben az objektumban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>A szövegsorok közötti térköz</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>A vetített objektumok színe</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>Alakzatkitöltési stílus</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>Vonalstílus</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>Ha be van jelölve, a forrásobjektumok megjelenik függetlenül attól, hogy látható legyen, a 3D-s modellben</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>Ív kezdőszöge</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="50"/>
      <source>End angle of the arc (for a full circle, 
                give it same value as First Angle)</source>
      <translation>Az ív végszöge (teljes körhöz 
                ugyanazt az értéket adja meg, mint az első szögben)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="54"/>
      <source>Radius of the circle</source>
      <translation>Kör sugara</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="58"/>
      <location filename="../../draftobjects/circle.py" line="58"/>
      <location filename="../../draftobjects/polygon.py" line="68"/>
      <location filename="../../draftobjects/ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>Felület létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="46"/>
      <source>Text string</source>
      <translation>Karakterlánc</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="49"/>
      <source>Font file name</source>
      <translation>Betűtípus fájlnév</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="52"/>
      <source>Height of text</source>
      <translation>Szöveg magassága</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="55"/>
      <source>Inter-character spacing</source>
      <translation>Karakterek közötti távolság</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="58"/>
      <source>Fill letters with faces</source>
      <translation>Betűk kitöltése felületekkel</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="86"/>
      <source>The base object that will be duplicated.</source>
      <translation>A másolandó alap objektum.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="93"/>
      <location filename="../../draftobjects/patharray.py" line="177"/>
      <source>The object along which the copies will be distributed. It must contain 'Edges'.</source>
      <translation>A tárgy, ami körül a másolatokat el osztják. Tartalmaznia kell 'Éleket'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
      <source>Number of copies to create.</source>
      <translation>A létrehozni kívánt másolatok száma.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
      <source>Rotation factor of the twisted array.</source>
      <translation>A csavart tömb forgatási tényezője.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="320"/>
      <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
      <location filename="../../draftobjects/pointarray.py" line="112"/>
      <location filename="../../draftobjects/patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>Az egyes tömbelemek megmutatása (csak csatolási tömbökhöz)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="83"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation>Általános léptéktényező, amely következetesen befolyásolja a jegyzetet
mert skálázza a szöveget, és a vonal dekorációkat, ha rendelkezésre állnak, 
azonos arányban.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="93"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation>Az tárgyra alkalmazható jegyzetstílus.
Mentett stílus használatakor a nézettulajdonságok némelyike írásvédett.
Ezeket csak a stílus módosításával lehet szerkeszteni a 'Jegyzetstílus szerkesztő' eszközzel.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="99"/>
      <source>Force sync pattern placements even when array elements are expanded</source>
      <translation>A mintaelhelyezések szinkronizálását kényszeríti a táblázatelemek kibontása esetén is</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="112"/>
      <source>Show the individual array elements</source>
      <translation>Az egyes elrendezési elemek mutatása</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>Ebben a klónban szereplő tárgyak</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>Ennek a klónnak a méretezési tényezője</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>Ha a klónok több tárgyból alkottak,
Igazra állítva egyesíti vagy hamisra az összetételhez</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>A B-görbe pontjai</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>Ha a B-görbe zárt vagy nyitott</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>Felület létrehozása, ha a görbe zárt</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>Paraméterezés tényező</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="57"/>
      <source>The base object this 2D view must represent</source>
      <translation>Az alap tárgyat ennek a 2D nézetnek kell képviselnie</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="62"/>
      <source>The projection vector of this object</source>
      <translation>Ennek a tárgynak a vetítési vektora</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="68"/>
      <source>The way the viewed object must be projected</source>
      <translation>A megtekintett tárgy vetítésének az útja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="75"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>A mutatott felületek vetítése az Egyéni felület módban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="80"/>
      <source>Show hidden lines</source>
      <translation>Mutassa a rejtett vonalakat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="86"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>Ugyanazon típusú anyagból készült fal és építő elemeket olvaszt egybe</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="91"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>Mozaikos ellipsziseket és B-Görbéket vonalszakaszokba</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="98"/>
      <source>For Cutlines and Cutfaces modes, 
                    this leaves the faces at the cut location</source>
      <translation>Szakaszvonal- és szakaszfelületi üzemmódoknál, 
                  ez a felületeket vágási helyzetben hagyja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="105"/>
      <source>Length of line segments if tessellating Ellipses or B-splines 
                    into line segments</source>
      <translation>A vonalszakaszok hossza ellipszisek vagy B-görbék után 
                vonalszakaszokká alakítható</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="111"/>
      <source>If this is True, this object will include only visible objects</source>
      <translation>Ha ez igaz, akkor ez a tárgy csak látható tárgyelemeket tartalmaz</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="117"/>
      <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
      <translation>A kizárási pontok listája. A pontok bármelyikét érintő élek nem kerülnek kisorsolásra.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="122"/>
      <source>If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</source>
      <translation>Ha ez igaz, csak szilárd geometriát kezelünk. Ez felülírja az alaptárgy Csak szilárd anyagok tulajdonságát</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="127"/>
      <source>If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property</source>
      <translation>Ha igaz, a tartalom adott esetben a szakaszsík határaira vágott. Ez felülírja az alapobjektum vágási tulajdonságát - Clip</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="132"/>
      <source>This object will be recomputed only if this is True.</source>
      <translation>Ez a tárgy csak akkor kerül újraszámításra, ha ez igaz.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="45"/>
      <source>X Location</source>
      <translation>X pozíció</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="48"/>
      <source>Y Location</source>
      <translation>Y pozíció</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="51"/>
      <source>Z Location</source>
      <translation>Z pozíció</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>A téglalap hossza</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>A téglalap magassága</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>Ennek a téglalapnak a vízszintes felbontásai</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>Ennek a téglalapnak a függőleges felosztásai</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>Csatolt felületek</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>Azt jelzi, hogy eltávolítja-e az elválasztó vonalakat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>Az összes felületre alkalmazott lehetséges kihúzási érték</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>Az összes felületre alkalmazható választható eltolási érték</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>Meghatározza, hogy az alakzatok varrtak-e</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>A Felülettároló felület területeinek tartománya</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>Felületek száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>Vezérlő kör sugara</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>Hogyan kell a sokszöget rajzolni a vezérlő körről</translation>
    </message>
    <message>
      <location filename="../../draftobjects/block.py" line="43"/>
      <source>The components of this block</source>
      <translation>Ennek a blokknak az összetevői</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>Ennek a vonalnak a kiinduló pontja.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>Ennek a vonalnak a végpontja.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>Ennek a vonalnak a hossza.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>A sarok lekerekítéséhez használt rádiusz.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>Ennek az objektumnak az elhelyezése</translation>
    </message>
    <message>
      <location filename="../../draftobjects/layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>A réteg részét képezik</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>Méretvonal szövegének alapértelmezett iránya</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>Az ezzel a méretvonallal mért tárgy</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="167"/>
      <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</source>
      <translation>A tárgy és annak konkrét al-elemei,
ami ezt a dimenzió tárgyat méri.

Számos lehetőség van:
- Egy tárgy, és annak egyik éle.
- Egy tárgy, és annak két csúcsa.
- Egy íves tárgy, és annak éle.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="190"/>
      <source>A point through which the dimension line, or an extrapolation of it, will pass.

- For linear dimensions, this property controls how close the dimension line
is to the measured object.
- For radial dimensions, this controls the direction of the dimension line
that displays the measured radius or diameter.
- For angular dimensions, this controls the radius of the dimension arc
that displays the measured angle.</source>
      <translation>Egy pont, amelyen keresztül a méretvonal, vagy annak extrapolációja áthalad.

- Lineáris méreteknél ez a tulajdonság határozza meg, hogy a méretvonal mennyire van
a mért tárgyhoz.
- Radiális méretekhez ez határozza meg a méretvonal irányát, 
amely jelzi a mért sugarat vagy átmérőt.
- Szögméretek, ez szabályozza a méret ív sugarát,
amely megjeleníti a mért szöget.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>A méretvonal kezdőpontja.

Ha sugárméret, akkor az ív középpontja lesz.
Ha átmérő mérete, akkor egy pont lesz, ami az íven fekszik.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>A méretvonal zárópontja.

Ha sugár- vagy átmérőméret
ez lesz az a pont, ami az íven fekszik.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>A méretvonal iránya.
Ha ez '(0,0,0)' marad, az irány automatikusan kerül kiszámításra.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>A mérés értéke.

Ez a tulajdonság írásvédett, mert az érték kiszámítása
a 'Kiinduló' és a 'Végpont' tulajdonságai.

Ha a 'Csatolt Geometria' egy ív vagy kör, ez a 'Távolság'
az 'Átmérő' tulajdonságtól függően a sugár vagy az átmérő.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>A körívek mérésekor meghatározza, hogy a sugár vagy az átmérő értéket jelenítse meg</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>A méretvonal kezdőszöge (körív).
Az ívet az óramutató járásával ellentétes irányban rajzolja.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>A méretvonal zárószöge (körív).
Az ívet az óramutató járásával ellentétes irányban rajzolja.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>A méretvonal középpontja, amely körív.

Ez általában az a pont, ahol két vonalszakasz vagy azok kiterjesztései
metszik egymást, ami a közöttük mért 'Szöget' eredményezi.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>A mérés értéke.

Ez a tulajdonság írásvédett, mert az érték
az "Első szög" és az "Utolsó szög" tulajdonságai.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>Az elliptikus ív indítási szöge</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="49"/>
      <source>End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</source>
      <translation>Az elliptikus ív befejező szöge 

                  (egy teljes körhöz, adja meg a kezdő szög értékét)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="52"/>
      <source>Minor radius of the ellipse</source>
      <translation>Az ellipszis kisebb sugara</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>Az ellipszis fő sugara</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>A tárgy területe</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>Az első egyenes alappontjának elhelyezése</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>Az tárgy által megjelenített szöveg.
Ez egy karakter lista; a lista minden eleme a saját sorában jelenik meg.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="82"/>
      <location filename="../../draftobjects/patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>Az alap objektum, melyet kettőzni kell</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>A 'Görbe tárgy' csatlakoztatott éleinek listája.
Ha léteznek, a másolatok csak ezen al-elemek mentén jönnek létre.
Hagyja üresen ezt a tulajdonságot, hogy másolatot készítsen a teljes 'Görbe tárgy' mentén.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>A létrehozni kívánt másolatok száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>Az egyes példányokra további elmozdulást alkalmaznak.
Ez akkor hasznos, ha módosítja az alakzat középpontja és az alakzat referenciapontja közötti különbséget.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>Igazítási vektor az 'Érintő' módhoz</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>A 'függőleges vektor' helyi Z irányként való használatának kényszerítése az 'Eredeti' vagy a 'Érintős' igazítás használatakor</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>A helyi Z tengely iránya, ha a 'Függőleges kényszerítés' igaz</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>A másolatok útvonal mentén igazításának módszere.
- Eredeti: X görbe érintő, Y normál és Z a kereszt termék.
- Frenet: az objektumot a helyi koordináta-rendszer szerint igazítja az útvonal mentén.
- Érintő: hasonló az 'Eredetihez', de a helyi X tengely előre igazodik a 'Érintő vektor'-hoz.

Az 'Eredeti' vagy a 'Érintő' jobb eredmény eléréséhez előfordulhat, hogy be kell állítania a 'Függőleges kényszerítés' lehetőséget.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>Igazítsa a másolatokat a görbe mentén, az 'Igazítási módtól' függően.
Ellenkező esetben a másolatok tájolása megegyezik az eredeti alap tárgyakkal.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>A Bezier-görbe pontjai</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>A Bezier-függvény mértéke</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>Folytonosság</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>Ha a Bezier-görbét le kell-e zárni vagy sem</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>Hozzon létre egy felületet, ha ez a görbe le van zárva</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>Ennek a tárgynak a hossza</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>A létrehozandó tömb típusa.
- Merőleges: a másolatokat a globális X, Y, Z tengelyek irányába helyezi.
- Poláris: a másolatokat egy kör alakú ív mentén, egy megadott szögig és egy közép- és tengely által meghatározott bizonyos tájolással helyezi el.
- Köralakú: a másolatokat koncentrikus körkörös rétegekbe helyezi az alapobjektum körül.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>Itt adható meg, hogy a másolatokat össze kell-e olvasztani, ha megérintik egymást (lassabb)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>X irányú másolatok száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Y irányú másolatok száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Z irányú másolatok száma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>Távolság és közök tájolása az X irányban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Távolság és közök tájolása az Y irányban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Távolság és közök tájolása az Z irányban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>Az a tengelyirány, amely körül a poláris vagy kör alakú tömb elemei létrejönnek</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>Középpont a poláris és kör alakú tömbök számára.
A 'Tengely' áthalad ezen a ponton.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>Az a tengelyobjektum, amely felülbírálja a 'Tengely' és a 'Közép' értékét, például egy dátum vonal.
A poláris és kör alakú tömbök létrehozásakor az elhelyezést, a pozíciót és az elforgatást használja.
Hagyja üresen ezt a tulajdonságot, hogy manuálisan tudja beállítani a 'Tengely' és a 'Közép' lehetőséget.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>Másolatok száma poláris irányban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>Távolság és közök tájolása a 'Tengely' irányban</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation>Szög a másolattokkal fedéshez</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>Kör alakú rétegek közötti távolság</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>A másolatok közötti távolság ugyanabban a kör alakú rétegben</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>Kör alakú rétegek száma. Az 'Alap' objektum egy rétegnek számít.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes the circular array will have.</source>
      <translation>Olyan paraméter, amely meghatározza, hogy hány szimmetriasík lesz a körkörös tömbben.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>A tömb elemeinek teljes száma.
Ez a tulajdonság írásvédett, mivel a szám a tömb paramétereitől függ.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>A megkettőzött alapobjektum</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>Az alaptárgy elosztására használt pontokat tartalmazó tárgy, például vázlat vagy összetett alkatrész.
A vázlatnak vagy kapcsolatnak legalább egy egyértelmű pontot vagy csúcspontot tartalmaznia kell.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>A tömb elemeinek teljes száma.
Ez a tulajdonság csak olvasható, mert a szám a 'Pont tárgy' pontjaitól függ.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="104"/>
      <location filename="../../draftobjects/pointarray.py" line="140"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>Az egyes másolati példányokon elvégzett további elhelyezés, elmozdítás és elforgatás</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="60"/>
      <location filename="../../draftviewproviders/view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>A szöveg mérete</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="69"/>
      <location filename="../../draftviewproviders/view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>A szöveg betütípusa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="78"/>
      <location filename="../../draftviewproviders/view_label.py" line="92"/>
      <location filename="../../draftviewproviders/view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>A szöveg függőleges igazítása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="87"/>
      <location filename="../../draftviewproviders/view_label.py" line="102"/>
      <source>Text color</source>
      <translation>Szöveg szín</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="95"/>
      <location filename="../../draftviewproviders/view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>Egyenes illesztés (relatív a betűméretehez)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>A szöveg doboz soronkénti karaktereinek maximális száma</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>A nyíl mérete</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>Ennek a címkének a nyíl típusa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>Az tárgy szövege körüli keret típusa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>Vezéregyenes mutatása vagy elrejtése</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
      <location filename="../../draftviewproviders/view_label.py" line="177"/>
      <source>Line width</source>
      <translation>Vonalvastagság</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
      <location filename="../../draftviewproviders/view_label.py" line="186"/>
      <source>Line color</source>
      <translation>Vonalszín</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>Betűtípus neve</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>Betűméret</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>A szöveg és a méretvonal közötti távolság</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>A méretszöveg 180 fokos elforgatása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>Szöveg pozíciója.
Hagyja '(0,0,0)' az automatikus pozícióért</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>Szöveg felülbírálása.
Írja be $ $dim", hogy a mérethossz váltsa fel.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>A megjelenítendő tizedesjegyek száma</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>Mértékegység utótag megjelenítése</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="216"/>
      <source>A unit to express the measurement.
Leave blank for system default.
Use 'arch' to force US arch notation</source>
      <translation>Egy egység, amely kifejezi a mérést.
Hagyja üresen a rendszer alapértelmezetthez.
Használja az 'arch' kifejezést amerikai US arch jelölésének kikényszerítésére</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="230"/>
      <source>Arrow size</source>
      <translation>Nyíl méret</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
      <source>Arrow type</source>
      <translation>Nyíl típus</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>A méretvonal nyilainak 180 fokos forgatása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>A méretvonal távolsága meghosszabbodik
a hosszabbító vonalakon túl</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
      <source>Length of the extension lines</source>
      <translation>A méret segédvonalak hossza</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>A hosszabbító vonal hossza
a méretvonalon túl</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
      <source>Shows the dimension line and arrows</source>
      <translation>A dimenzióvonal és a nyilak megjelenítve</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="67"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Ha igaz, a réteg tárgyai öröklik a réteg vonalszínét</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="78"/>
      <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
      <translation>Ha igaz, a réteg tárgyai öröklik a réteg alakzat színét</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="89"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>Ha igaz, a nyomtatási szín akkor lesz használható, amikor a réteg tárgyai MűszakiRajz oldalra kerülnek</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="103"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>A rétegen belüli tárgyak vonalszíne</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="117"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>A rétegen belüli tárgyak alakszíne</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="131"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>A rétegen belüli tárgyak vonalszélessége</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="143"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>A rétegen belüli tárgyak rajzstílusa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="154"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>A rétegen belüli tárgyak átlátszósága</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="165"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>A rétegen belüli tárgyak vonalszíne, ha MűszakiRajz oldalon használják</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="106"/>
      <source>Defines an SVG pattern.</source>
      <translation>SVG mintát határoz meg.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="116"/>
      <source>Defines the size of the SVG pattern.</source>
      <translation>Beállítja az SVG minta méretet.</translation>
    </message>
  </context>
  <context>
    <name>Dialog</name>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="14"/>
      <source>Annotation Styles Editor</source>
      <translation>Jegyzetstílusok szerkesztője</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>Stílusnév</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>Stílusának a neve. Meglévő stílus nevek szerkeszthetőek.</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>Új hozzáadása...</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>Átnevezi a kijelölt stílust</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>Átnevezés</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>Törli a kijelölt stílust</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>Törlés</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>Stílusok importálása json fájlból</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>Stílusok exportálása json fájlba</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>Szöveg</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
      <source>The font to use for texts and dimensions</source>
      <translation>A szövegekhez és méretekhez használt betűtípus</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font name</source>
      <translation>Betűtípus neve</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
      <source>Font size in the system units</source>
      <translation>Rendszer mértékegységek betűmérete</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
      <source>Font size</source>
      <translation>Betűméret</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Line spacing in system units</source>
      <translation>Rendszer mértékegységek vonal térköze</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
      <source>Line spacing</source>
      <translation>Sorköz</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>Egységek</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>Lépték szorzó, mely befojásolja a jelölők és szövegek méretét</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>Lépték szorzó</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>Ha be van jelölve, akkor a dimenzióérték mellett az egység jelenik meg</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
      <source>Show unit</source>
      <translation>Mértékegység megjelenítés</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>Adjon meg egy érvényes hosszegységet, például mm, m, in, ft, az egység méretértékének megjelenítésének kényszerítéséhez</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
      <source>Unit override</source>
      <translation>Mértékegység felülírás</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>A dimenzióértékekhez megjelenítandő tizedesjegyek száma</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
      <source>Decimals</source>
      <translation>Tizedesjegyek</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>Vonal és nyilak</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>Ha engedélyezve van, a méretvonal megjelenik</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Show lines</source>
      <translation>Vonalak megjelenítése</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
      <source>The width of the dimension lines</source>
      <translation>Méret segédvonalak vastagsága</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
      <source>Line width</source>
      <translation>Vonalvastagság</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="376"/>
      <source>px</source>
      <translation>px</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="386"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="396"/>
      <source>The color of dimension lines, arrows and texts</source>
      <translation>Méret segédvonalak, nyilak és szövegek színe</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
      <source>Line / text color</source>
      <translation>Vonal / szöveg szín</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>A méretvonalak végén használni kívánt nyilak vagy jelölők típusa</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
      <source>Arrow type</source>
      <translation>Nyíl típus</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>Dot</source>
      <translation>Pont</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
      <source>Circle</source>
      <translation>Kör</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>Arrow</source>
      <translation>Nyíl</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
      <source>Tick</source>
      <translation>Jelölők</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
      <source>Tick-2</source>
      <translation>Jelölők-2</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>A méretnyilak vagy jelölők mérete a rendszeregységekben</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
      <source>Arrow size</source>
      <translation>Nyíl méret</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>A méretvonal továbbnyújtási hossza</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
      <source>Dimension overshoot</source>
      <translation>Méretvonal túllépése</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The length of the extension lines</source>
      <translation>A hosszabbító vonalak hossza</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
      <source>Extension lines</source>
      <translation>Meghosszabbító vonalak</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>Az a távolság, amellyel a hosszabbító vonalak a méretvonalon túl is meghosszabbítanak</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
      <source>Extension overshoot</source>
      <translation>Meghosszabbítás túllépése</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="../../importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>Dxf könyvtárak letöltése nem sikerült. 
Kérjük, telepítse a dxf könyvtár kiegészítőt
kézzel az Eszközök -&gt; Kiegészítő kezelő menüből</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="133"/>
      <location filename="../../InitGui.py" line="134"/>
      <location filename="../../InitGui.py" line="135"/>
      <location filename="../../InitGui.py" line="136"/>
      <location filename="../../InitGui.py" line="137"/>
      <source>Draft</source>
      <translation>Tervrajz</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="179"/>
      <location filename="../../InitGui.py" line="180"/>
      <location filename="../../InitGui.py" line="181"/>
      <location filename="../../InitGui.py" line="182"/>
      <source>Import-Export</source>
      <translation>Importálás-Exportálás</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
      <source>Toggles Grid On/Off</source>
      <translation>Rács be-/kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
      <source>Object snapping</source>
      <translation>Tárgy illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>A vizuális segédméretek be- és kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
      <source>Toggles Ortho On/Off</source>
      <translation>Merőleges be- és kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation>Be- és kikapcsolja a kényszerítést a munkasíkra</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry: Ugyanazokkal a kezdő/vég ponttokkal lezárt ívet talált. A geometria nincs frissítve.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="244"/>
      <location filename="../../draftobjects/pointarray.py" line="306"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>A pontnak nincs szakasz pontja, elrendezéshez nem használható.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
      <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>Lejtő</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>Klónozás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="49"/>
      <source>You must choose a base object before using this command</source>
      <translation>A parancs használata előtt ki kell választania egy elsődleges tárgyat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>Eredeti tárgyak törlése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>Letörés létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
      <source>Save style</source>
      <translation>Stílus mentése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
      <source>Name of this new style:</source>
      <translation>Az új stílus neve:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
      <source>Warning</source>
      <translation>Riasztás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
      <source>Name exists. Overwrite?</source>
      <translation>A név már létezik. Felülírjuk?</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
      <source>Error: json module not found. Unable to save style</source>
      <translation>Hiba: a json modul nem található. A stílus mentése sikertelen</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="329"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation>Az eltolás iránya nem meghatározott. Először mozgassa az egeret a tárgy egyik oldalára, hogy meghatározza az irányt</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>Igaz</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>Hamis</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
      <source>Scale</source>
      <translation>Méretezés</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
      <source>X factor</source>
      <translation>X lépték</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
      <source>Y factor</source>
      <translation>Y lépték</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
      <source>Z factor</source>
      <translation>Z lépték</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
      <source>Uniform scaling</source>
      <translation>Egyenletes méretezés</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
      <source>Working plane orientation</source>
      <translation>Munka sík igazítás</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
      <source>Copy</source>
      <translation>Másolás</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
      <source>Modify subelements</source>
      <translation>Alelemek módosítása</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
      <source>Pick from/to points</source>
      <translation>Kijelölés pontokból/pontba</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
      <source>Create a clone</source>
      <translation>Létrehoz egy klónt</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>Kamera helyzet írása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>Tárgy megjelenítés/elrejtés állapotának kiírása</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>Körkörös elrendelés</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(helykitöltő a szimbólumhoz)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>Távolság az egyik tárgyrétegtől a következő tárgyrétegig.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
      <source>Radial distance</source>
      <translation>Sugárirányú távolság</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>Távolság az elrendezés egyik gyűrűjében található egyik elemtől a gyűrű következő eleméhez.
Nem lehet nulla.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
      <source>Tangential distance</source>
      <translation>Érintőtávolság</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>A létrehozni kívánt körrétegek vagy gyűrűk száma, beleértve az eredeti tárgyak másolatát is.
Legalább 2-esnek kell lennie.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
      <source>Number of circular layers</source>
      <translation>Körkörös rétegek száma</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>A körkörös elrendezés szimmetriavonalainak száma.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
      <source>Symmetry</source>
      <translation>Szimmetria</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Annak a pontnak a koordinátái, amelyen keresztül a forgástengely áthalad.
Módosítsa a tengely irányát a Tulajdonságok szerkesztőben.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
      <source>Center of rotation</source>
      <translation>Forgatás középpontja</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="163"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="183"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="203"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="225"/>
      <source>Reset the coordinates of the center of rotation.</source>
      <translation>Állítsa alaphelyzetbe az elforgatási középpont koordinátáit.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
      <source>Reset point</source>
      <translation>Pont visszaállítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ha engedélyezve van, az elrendezésben lévő eredményül kapott tárgyak egyesülnek, amikor megérintik egymást.
Ez csak akkor működik, ha a "Elrendezések csatolása" ki van kapcsolva.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
      <source>Fuse</source>
      <translation>Egybeolvaszt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ha engedélyezve van, az eredményül kapott tárgy egy "Elrendezés csatolás" lesz a normál elrendezés helyett.
A elrendezés csatolás hatékonyabb több példány létrehozásakor, de nem egyesíthetők.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
      <source>Link array</source>
      <translation>Elrendezés csatolás</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>Merőleges elrendezés</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(helykitöltő a szimbólumhoz)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>Az elrendezés elemeinek száma a megadott irányban, beleértve az eredeti tárgy másolatát is.
A számnak mindkét irányban legalább 1-nek kell lennie.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
      <source>Number of elements</source>
      <translation>Elemszám</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="63"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="132"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="223"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="314"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="80"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="155"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="243"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="334"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="97"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="175"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="266"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="354"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="119"/>
      <source>Distance between the elements in the X direction.
Normally, only the X value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Az elemek közötti távolság X irányban.
Normális esetben csak a X-értékre van szükség, a másik két érték további elmozdulást tehet lehetővé az adott irányba.
A negatív értékek negatív irányban generált másolatokat eredményeznek.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
      <source>X intervals</source>
      <translation>X közök</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
      <source>Reset the distances.</source>
      <translation>Távolságok visszaállítása.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
      <source>Reset X</source>
      <translation>X alaphelyzetbe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Az elemek közötti távolság Y irányban.
Normális esetben csak a Y-értékre van szükség, a másik két érték további elmozdulást tehet lehetővé az adott irányba.
A negatív értékek negatív irányban generált másolatokat eredményeznek.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
      <source>Y intervals</source>
      <translation>Y közök</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
      <source>Reset Y</source>
      <translation>Y alaphelyzetbe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Az elemek közötti távolság Z irányban.
Normális esetben csak a z-értékre van szükség, a másik két érték további elmozdulást tehet lehetővé az adott irányba.
A negatív értékek negatív irányban generált másolatokat eredményeznek.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
      <source>Z intervals</source>
      <translation>Z közök</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
      <source>Reset Z</source>
      <translation>Z alaphelyzetbe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ha engedélyezve van, az elrendezésben lévő eredményül kapott tárgyak egyesülnek, amikor megérintik egymást.
Ez csak akkor működik, ha a "Elrendezések csatolása" ki van kapcsolva.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
      <source>Fuse</source>
      <translation>Egybeolvaszt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ha engedélyezve van, az eredményül kapott tárgy egy "Elrendezés csatolás" lesz a normál elrendezés helyett.
A elrendezés csatolás hatékonyabb több példány létrehozásakor, de nem egyesíthetők.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
      <source>Link array</source>
      <translation>Elrendezés csatolás</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>Poláris elrendezés</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(helykitöltő a szimbólumhoz)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>A poláris eloszlás elforgatási szöge.
A negatív szög poláris mintázatot hoz létre az ellenkező irányba.
A maximális abszolút érték 360 fok.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
      <source>Polar angle</source>
      <translation>Poláris szög</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>Az elrendezés elemeinek száma, beleértve az eredeti tárgy másolatát is.
Legalább 2-esnek kell lennie.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
      <source>Number of elements</source>
      <translation>Elemszám</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Annak a pontnak a koordinátái, amelyen keresztül a forgástengely áthalad.
Módosítsa a tengely irányát a Tulajdonságok szerkesztőben.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
      <source>Center of rotation</source>
      <translation>Forgatás középpontja</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="125"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="145"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="165"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="187"/>
      <source>Reset the coordinates of the center of rotation.</source>
      <translation>Állítsa alaphelyzetbe az elforgatási középpont koordinátáit.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
      <source>Reset point</source>
      <translation>Pont visszaállítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ha engedélyezve van, az elrendezésben lévő eredményül kapott tárgyak egyesülnek, amikor megérintik egymást.
Ez csak akkor működik, ha a "Elrendezések csatolása" ki van kapcsolva.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
      <source>Fuse</source>
      <translation>Egybeolvaszt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ha engedélyezve van, az eredményül kapott tárgy egy "Elrendezés csatolás" lesz a normál elrendezés helyett.
A elrendezés csatolás hatékonyabb több példány létrehozásakor, de nem egyesíthetők.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
      <source>Link array</source>
      <translation>Elrendezés csatolás</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>AlakzatSzövegből</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="46"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="53"/>
      <location filename="../ui/TaskShapeString.ui" line="70"/>
      <location filename="../ui/TaskShapeString.ui" line="87"/>
      <source>Enter coordinates or select point with mouse.</source>
      <translation>Adjon meg koordinátákat vagy válasszon pontot az egérrel.</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="63"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="80"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="114"/>
      <source>Reset 3d point selection</source>
      <translation>3d pont kiválasztás alaphelyzetbe állítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="120"/>
      <source>Reset Point</source>
      <translation>Pont visszaállítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="131"/>
      <source>String</source>
      <translation>Karakterlánc</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="138"/>
      <source>Text to be made into ShapeString</source>
      <translation>Szöveg szövegalakzattás alakítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="149"/>
      <source>Height</source>
      <translation>Magasság</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="156"/>
      <source>Height of the result</source>
      <translation>Az eredmény magassága</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="176"/>
      <source>Font file</source>
      <translation>Betűtípusfájl</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="309"/>
      <source>Add to Construction group</source>
      <translation>Hozzáadás az építési csoporthoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="312"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>A kijelölt tárgyak hozzáadása az építési csoporthoz
és megváltoztatják megjelenésüket az építési stílusra.
Építési csoport jön létre, ha nem létezik.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddNamedGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="361"/>
      <source>Add a new named group</source>
      <translation>Új elnevezett csoport hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="365"/>
      <source>Add a new group with a given name.</source>
      <translation>Adjon hozzá egy új csoportot egy megadott névvel.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>Pont hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>Pontot ad egy meglévő vonalhoz vagy B-görbéhez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddToGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="73"/>
      <source>Move to group...</source>
      <translation>Áthelyezés csoportba...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="76"/>
      <source>Moves the selected objects to an existing group, or removes them from any group.
Create a group first to use this tool.</source>
      <translation>Áthelyezi a kijelölt tárgyakat egy meglévő csoportba, vagy eltávolítja őket az egyes csoportból.
Először hozzon létre egy csoportot az eszköz használatára.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
      <source>Annotation styles...</source>
      <translation>Jegyzetstílusok...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
      <source>Manage or create annotation styles</source>
      <translation>Jegyzetstílusok kezelése vagy létrehozása</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>Aktuális stílus alkalmazása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>Az eszköztáron definiált stílust (vonalszélességet és színeket) alkalmazza a kijelölt tárgyakra és csoportokra.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>Ív</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Ívet hoz létre középponttal és sugárral. 
CTRL az illesztéshez, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="606"/>
      <source>Arc tools</source>
      <translation>Íveszközök</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="609"/>
      <source>Create various types of circular arcs.</source>
      <translation>Hozzon létre különböző típusú köríveket.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc_3Points</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="487"/>
      <source>Arc by 3 points</source>
      <translation>Körív 3 pontból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="490"/>
      <source>Creates a circular arc by picking 3 points.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Körkörös ívet hoz létre 3 pont kiválasztásával.
CTRL illesztéshez, a SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Array</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="68"/>
      <source>Array</source>
      <translation>Sorba rendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>Elrendezést hoz létre kijelölt tárgyakból.
Alapértelmezés szerint ez egy 2x2 merőleges elrendezés.
Az elrendezés létrejötte után az elrendezés típusa
poláris vagy körkörös, és tulajdonságaik megváltoztathatóak.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArrayTools</name>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>Elrendezés eszközei</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>Hozzon létre különböző típusú elrendezéseket, beleértve a téglalap alakú, poláris, kör alakú, elérési utat és pontot</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="208"/>
      <source>Autogroup</source>
      <translation>Autocsoport</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="211"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>Jelöljön ki egy csoportot, amelyhez az összes tervrajt- és íves tárgyakat hozzá szeretné adni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-görbe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>Többpontos B-görbét hoz létre. CTRL igazításhoz, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="64"/>
      <source>Bézier curve</source>
      <translation>Bézier-görbe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="67"/>
      <source>Creates an N-degree Bézier curve. The more points you pick, the higher the degree.
CTRL to snap, SHIFT to constrain.</source>
      <translation>N-fokos Bézier görbét hoz létre. Minél több pontot választasz, annál magasabb a fok.
CTRL illesztéshez, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezierTools</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="475"/>
      <source>Bézier tools</source>
      <translation>Bezier-görbe eszközök</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="478"/>
      <source>Create various types of Bézier curves.</source>
      <translation>Hozzon létre különböző típusú Bazier görbéket.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Circle</name>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="80"/>
      <source>Circle</source>
      <translation>Kör</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="84"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>Létrehoz egy kört (teljes körív).
CTRL igazításához, ALT billentyűkombinációval jelölhet ki érintő tárgyakat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CircularArray</name>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
      <source>Circular array</source>
      <translation>Körkörös elrendelés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation>Másolatot készít a kijelölt tárgyról, és a másolatokat sugárirányú mintába helyezi
és különböző körkörös rétegeket hoz létre.

Az elrendezés merőleges vagy poláris elrendezésre alakítható a típus megváltoztatásával.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Clone</name>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="70"/>
      <source>Clone</source>
      <translation>Klónozás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="73"/>
      <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
      <translation>Létrehozza a kijelölt tárgyak klónozását.
Az eredményül kapott klón mindhárom irányban méretezhető.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CubicBezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="242"/>
      <source>Cubic Bézier curve</source>
      <translation>Köbös bezier görbe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="245"/>
      <source>Creates a Bézier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.
After the curve is created you can go back to edit each control point and set the properties of each knot.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Bezier görbét hoz létre a 2. fokon (másodfokú) és 3. fokon (harmadfokú) szakaszokkal. Kattintson és húzza a billentyűt az egyes szakaszok meghatározásához.
A görbe létrehozása után visszamehet, és szerkesztheti az egyes vezérlőpontokat, és beállíthatja az egyes csomópontok tulajdonságait.
CTRL igazításhoz, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_DelPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="89"/>
      <source>Remove point</source>
      <translation>Pont eltávolítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation>Eltávolít egy pontot egy meglévő vonalból vagy B-görbéből.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
      <source>Dimension</source>
      <translation>Dimenzió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="87"/>
      <source>Creates a dimension.

- Pick three points to create a simple linear dimension.
- Select a straight line to create a linear dimension linked to that line.
- Select an arc or circle to create a radius or diameter dimension linked to that arc.
- Select two straight lines to create an angular dimension between them.
CTRL to snap, SHIFT to constrain, ALT to select an edge or arc.

You may select a single line or single circular arc before launching this command
to create the corresponding linked dimension.
You may also select an 'App::MeasureDistance' object before launching this command
to turn it into a 'Draft Dimension' object.</source>
      <translation>Létrehoz egy méretet.

- Válasszon ki három pontot egy egyszerű lineáris dimenzió létrehozásához.
- Válasszon egy egyenes sort egy ehhez a vonalhoz társított lineáris dimenzió létrehozásához.
- Válasszon ki egy ívet vagy kört az adott ívhez kapcsolódó sugár- vagy átmérőméret létrehozásához.
- Válasszon ki két egyenes vonalat egy közöttük lévő méretezett méret létrehozásához.
CTRL igazításhoz, SHIFT kényszerhez, ALT egy él vagy ív kijelöléséhez.

A parancs indítása előtt kijelölhet egy egyenest vagy ívet a megfelelő csatolt méretezés létrehozásához.
A parancs elindítása előtt kijelölhet egy 'App:: DistanceMeasurement' tárgyat is,
'Tervrajz méret' tárggyá alakíthatja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation>Lefokoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation>Egyszerűbb alakzatokká lefokozza a kijelölt tárgyakat.
A művelet eredménye a tárgytípusoktól függ, amelyek egymás után többször is visszaminősíthetőek lehetnek.
Például a kijelölt vonalláncokat egyszerűbb felületekre, egyenesekre és élekre bomlanak. A felületeket ki is vonhatja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>Tervrajzból vázlat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>Kétirányú konvertálás tervrajz tárgyak és vázlatok között.
Sok tervrajz tárgy egyetlen nem kényszerített vázlattá alakul át.
Azonban a leválasztott nyomkövetésű egyedi vázlatok több egyedi vázlat tárggyá alakul át.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>Rajz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation>2D vetítést hoz létre a rajz munkafelületén lévő kijelölt tárgyakról.
Ez a parancs ELAVULT, mert a rajz munkafelület 0,17 óta elavult.
Ehelyett a MűszakiRajz munkafelület segítségével hozzon létre műszaki rajzokat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>Szerkesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation>Az aktív tárgy szerkesztése.
A helyi menü megjelenítéséhez nyomja le az E vagy az ALT+LeftClick billentyűkombinációt
támogatott csomópontokon és támogatott tárgyakon.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>Ellipszis</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>Ellipszis létrehozása. CTRL az illesztéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation>Felülettároló</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>Felületcsoportot profil objektumot hoz létre a kijelölt felületekből.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Fillet</name>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="64"/>
      <source>Fillet</source>
      <translation>Lekerekítés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>Lekerekítése hoz létre két kijelölt vonal vagy él között.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>Méretek megfordítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>Fordítsa meg a kijelölt méretek normál irányát (egyenes, sugár, szög).
Ha más tárgyak vannak kijelölve, a program figyelmen kívül hagyja őket.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Hatch</name>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="38"/>
      <source>Hatch</source>
      <translation>Kitöltés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="42"/>
      <source>Creates hatches on the faces of a selected object</source>
      <translation>Kitöltőkép létrehozása egy kijelölt tárgy felületén</translation>
    </message>
  </context>
  <context>
    <name>Draft_Heal</name>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="58"/>
      <source>Heal</source>
      <translation>Gyógyítani</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>A program egy korábbi verziójával mentett hibás tervrajz tárgyak gyógyítása.
Ha egy tárgy ki van jelölve, akkor megpróbálja meggyógyítani kimondottan a tárgyat,
ellenkező esetben megpróbálja gyógyítani az aktív dokumentum összes tárgyát.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="66"/>
      <source>Join</source>
      <translation>Csatlakoztatás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>A kijelölt vonalakat vagy vonalláncokat egyetlen tárgyba illeszti.
A művelet sikeres végrehajtásához a vonalaknak közös pontot kell megosztaniuk az elején vagy a végén.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>Felirat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="67"/>
      <source>Creates a label, optionally attached to a selected object or subelement.

First select a vertex, an edge, or a face of an object, then call this command,
and then set the position of the leader line and the textual label.
The label will be able to display information about this object, and about the selected subelement,
if any.

If many objects or many subelements are selected, only the first one in each case
will be used to provide information to the label.</source>
      <translation>Létrehoz egy címkét, amely opcionálisan egy kijelölt tárgyhoz vagy al elemhez van csatolva.

Először jelöljön ki egy csúcspontot, egy szegélyt vagy egy tárgy felületét, majd hívja ezt a parancsot,
majd állítsa be a vezetővonal és a szöveges felirat pozícióját.
A címke képes lesz információkat megjeleníteni erről a tárgyról és a kijelölt al elemről,
ha van ilyen.

Ha sok tárgy vagy több al elem van kijelölve, minden esetben csak az első kerül használatra
a feliraton való tájékoztatásra.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>Réteg</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>Réteget ad a dokumentumhoz.
A réteghez hozzáadott tárgyak ugyanazokat a vizuális tulajdonságokat osztják meg, mint a vonalszín, a vonalszélesség és az alakzat színe.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="63"/>
      <source>Line</source>
      <translation>Vonal</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="66"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>Kétpontos vonalat hoz létre. CTRL igazításhoz, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation>Összekapcsolt elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Az elrendezési eszközhöz hasonló 'Összekapcsolt elrendezést' hoz létre.
Az összekapcsolt elrendezés több példány feldolgozásakor hatékonyabb, de az 'Egyesítés' beállítás nem használható.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>Tükrözés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>A kijelölt objektumokat két pont által meghatározott vonal mentén tükrözi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Move</name>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="63"/>
      <source>Move</source>
      <translation>Mozgat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="66"/>
      <source>Moves the selected objects from one base point to another point.
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>A kijelölt tárgyak mozgatása egyik bázispontról a másikra.
Ha a "másolás" beállítás aktív, akkor elmozdult másolatokat hoz létre.
CTRL illesztéshez, SHIFT a kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>Eltolás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>A kijelölt tárgyak eltolása.
Létrehozhat egy eltolási másolatot is az eredeti tárgyról.
CTRL illesztéshez, SHIFT a kényszerítéshez. Tartsa az ALT billentyűt, és kattintson, ha minden kattintással másolatot szeretne létrehozni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_OrthoArray</name>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
      <source>Array</source>
      <translation>Sorba rendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>Másolatot készít a kijelölt tárgyról, és a másolatokat merőleges mintába helyezi.
Ez azt jelenti, hogy a másolatok az X-Y, Z tengelyek megadott irányát követik.

Az elrendezés merőleges vagy poláris elrendezésre alakítható a típus megváltoztatásával.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation>Útvonal elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>A kijelölt tárgyak másolatait hozza létre a kijelölt elérési út mentén.
Először jelölje ki a tárgyat, majd jelölje ki az útvonalat.
Az útvonal lehet vonallánc, B-görbe, Bézier görbe, vagy akár más tárgyak élei.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation>Útvonal összekapcsolt elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Az útvonal elrendezési eszközhöz hasonló 'Összekapcsolt elrendezést' hoz létre.
Az összekapcsolt elrendezés több példány feldolgozásakor hatékonyabb, de az 'Egyesítés' beállítás nem használható.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation>Útvonal csavart elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>A kijelölt tárgyak másolatait hozza létre a kijelölt elérési út mentén és megcsavarja azokat
Először jelölje ki a tárgyat, majd jelölje ki az útvonalat.
Az útvonal lehet vonallánc, B-görbe, Bézier görbe, vagy akár más tárgyak élei.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation>Útvonal csavart elrendezési eszköz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Az útvonal csavart elrendezési eszközhöz hasonló 'Összekapcsolt elrendezést' hoz létre.
Az 'Összekapcsolt elrendezés' több példány feldolgozásakor hatékonyabb, de az 'Egyesítés' beállítás nem használható.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="62"/>
      <source>Point</source>
      <translation>Pont</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation>Pont tárgyat hoz létre. Kattintson bárhová a 3D nézetben.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>Pont elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation>Másolatot készít a kijelölt tárgyról, és a másolatokat a különböző pontok helyére helyezi.

A pontokat az eszköz használata előtt a pont kapcsolatok csoportok alá kell csoportosítani.
A kapcsolat létrehozásához jelöljön ki különböző pontokat, majd használja a Rész kapcsolatok eszközt. Vagy használja a Tervrajzfrissítés eszközt egy 'blokk' létrehozásához, vagy hozzon létre egy vázlatot, és adjon hozzá egyszerű pontokat.

Jelölje ki az alap tárgyat, majd jelölje ki a kapcsolatot vagy a vázlatot a pontelrendezés létrehozásához.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
      <source>PointLinkArray</source>
      <translation>PontCsatolásElrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>Mint a PomtElrendezés eszköz, de létrehoz egy 'Pont csatolás elrendezés' helyett.
A 'Pont csatolás elrendezés' hatékonyabb, ha sok példányt kezel.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PolarArray</name>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="65"/>
      <source>Polar array</source>
      <translation>Poláris elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation>Másolatot készít a kijelölt tárgyról, és a másolatokat poláris mintába helyezi,
melyet egy elforgatási pont és annak szöge határoz meg.

Az elrendezés merőleges vagy poláris elrendezésre alakítható a típus megváltoztatásával.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>Sokszög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>Egyszerű sokszöget (háromszög, négyzet, ötszög, ...) hoz létre az oldalak számának és a körülírt sugárnak a meghatározásával.
CTRL illesztéshez, SHIFT a kényszerítéshez</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>Téglalap</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>Kétpontos téglalapot hoz létre. A CTRL az illesztéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>Forgatás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Elforgatja a kijelölt tárgyakat. Válassza ki az elforgatás közepét, majd a kezdeti szöget, majd a végső szöget.
Ha a "másolás" beállítás aktív, akkor elforgatott másolatokat hoz létre.
CTRL illeszt, SHIFT a kényszerít. Tartsa az ALT billentyűt, és kattintson ide, ha minden kattintással másolatot szeretne létrehozni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>Méretezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>A kijelölt tárgyakat az alappontból méretezi.
CTRL illesztéshez, SHIFT a kényszerítéshez, ALT másoláshoz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="164"/>
      <source>Select group</source>
      <translation>Csoport kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="167"/>
      <source>Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</source>
      <translation>Kijelöli a kijelölt csoportok tartalmát. Kijelölt nem csoportobjektumok esetében a csoport tartalma van kiválasztva.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectPlane</name>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="65"/>
      <source>Select Plane</source>
      <translation type="unfinished">Select Plane</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="67"/>
      <source>Select the face of solid body to create a working plane on which to sketch Draft objects.
You may also select a three vertices or a Working Plane Proxy.</source>
      <translation>Jelölje ki a szilárd test felületét, ha olyan munkasíkot szeretne létrehozni, amelyen vázlatokat készíthet Tervrajz tárgyakról.
Három csúcspontot vagy egy munkasík proxyt is kijelölhet.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
      <source>Set style</source>
      <translation>Stílus beállítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
      <source>Sets default styles</source>
      <translation>Alapértelmezett stílusok beállítása</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
      <source>Shape 2D view</source>
      <translation>Alakzat 2D nézete</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>Létrehoz egy 2D vetületet a kijelölt tárgyakról az XY síkon.
A kezdeti vetítési irány az aktuális aktív nézet irányának negatívja.
Kiválaszthatja az egyes kivetítési felületeket vagy a teljes szilárd testet, és rejtett sorokat is tartalmazhat.
Ezek a vetítések használhatók technikai rajzok létrehozásához a MűszakiRajz munkafelületen.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
      <source>Shape from text</source>
      <translation>Alakzat szövegből</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>Alakzatot hoz létre egy szöveges karakterláncból egy adott betűtípus és elhelyezés kiválasztásával.
A zárt alakzatok kihúzásához és logikai műveletekhez használhatók.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="589"/>
      <source>Show snap toolbar</source>
      <translation>Illesztési eszköztár megjelenítése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="592"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>Az illesztési eszköztár megjelenítése, ha rejtett.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>Lejtés beállítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>A kijelölt vonal lejtését úgy állítja be, hogy megváltoztatja az egyik pont Z értékét.
Ha egy vonalláncot kijelöl, akkor a lejtésátalakítást alkalmazza az egyes szegmensekre.

A lejtés mindig megváltoztatja a Z értéket, ezért ez a parancs csak az XY síkban rajzolt
egyenes piszkozatvonalak működik jól. A program figyelmen kívül hagyja a nem egy vonalból álló kijelölt tárgyakat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Angle</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="344"/>
      <source>Angle</source>
      <translation>Szög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="347"/>
      <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
      <translation>Állítsa be az illesztés pontokat egy körívben, amely 30 és 45 fokos szögek többszöröse.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Center</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="374"/>
      <source>Center</source>
      <translation>Középre</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="377"/>
      <source>Set snapping to the center of a circular arc.</source>
      <translation>Állítsa be az illesztést egy körív közepére.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="526"/>
      <source>Show dimensions</source>
      <translation>Dimenziók megjelenítése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="529"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>Ideiglenes lineáris dimenziók megjelenítése a tárgyak szerkesztésénél és más illesztési módszerek használatakor.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="313"/>
      <source>Endpoint</source>
      <translation>Végpont</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="316"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>Állítsa az illesztést egy él végpontjaira.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="404"/>
      <source>Extension</source>
      <translation>Meghosszabbítás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="407"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>Állítsa az illesztést az él meghosszabbításához.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="223"/>
      <source>Grid</source>
      <translation>Rács</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="226"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>Állítsa az illesztést a rácsvonalak metszéspontjaira.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="253"/>
      <source>Intersection</source>
      <translation>Metszet</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="256"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>Állítsa az illesztést az élek metszéspontjaira.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="133"/>
      <source>Main snapping toggle On/Off</source>
      <translation>Illesztés főkapcsoló be- és kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="136"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>Egyszerre aktiválja vagy inaktiválja az összes illesztési eszközt.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="163"/>
      <source>Midpoint</source>
      <translation>Felezőpont</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="166"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>Állítsa be az illesztést egy él felezőpontjához.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="434"/>
      <source>Nearest</source>
      <translation>Legközelebbi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="437"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>Állítsa be az illesztést az él legközelebbi pontjához.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Ortho</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="465"/>
      <source>Orthogonal</source>
      <translation>Merőleges</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="468"/>
      <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
      <translation>Állítsa az illesztést egy pont 45 fokos többszörösére.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Parallel</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="283"/>
      <source>Parallel</source>
      <translation>Párhuzamos</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="286"/>
      <source>Set snapping to a direction that is parallel to an edge.</source>
      <translation>Állítsa az illesztést egy éllel párhuzamos irányba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="193"/>
      <source>Perpendicular</source>
      <translation>Merőleges</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="196"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>Állítsa az illesztést egy éllel merőleges irányba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="495"/>
      <source>Special</source>
      <translation>Különleges</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="498"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>Állítsa be az illesztést a tárgyon belül definiált speciális pontokhoz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="559"/>
      <source>Working plane</source>
      <translation>Munkasík</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="562"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>Korlátozza az illesztést egy pontra az aktuális munkasíkon.
Ha a munkasíkon kívül jelöl ki egy pontot, például más illesztési móddal,
az aktuális munkasíkban ennek a pontnak a kivetítésére fog illeszkedni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="56"/>
      <source>Split</source>
      <translation>Feloszt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>A kijelölt vonal vagy vonallánc felosztása két független vonalra
vagy vonalláncok kattintva bárhol az eredeti tárgy mentén.
Ez akkor működik a legjobban, ha egy pontot választ egy egyenes szakaszában, és nem sarok csúcspontot.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>Nyújtás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>Megnyújtja a kijelölt objektumokat.
Jelöljön ki egy tárgyat, majd rajzoljon egy téglalapot a nyújtani kívánt csúcspontok kiválasztásához,
majd rajzoljon egy vonalat a nyújtás távolságának és irányának megadásához.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="61"/>
      <source>Subelement highlight</source>
      <translation>Alelem kiemelése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="64"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>Jelölje ki a kijelölt tárgyak alelemeit, hogy azokat az áthelyezze, elforgassa és méretezési eszközökkel szerkessze.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>Szöveg</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>Többsoros jegyzetet hoz létre. A CTRL az illesztéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>Építési mód váltása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>Az Építési mód váltása.
Ha ez aktív, a következő létrehozott tárgyak szerepelnek az építési csoportban, és a megadott színnel és tulajdonságokkal kerülnek rajzolásra.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>Folyamatos mód váltása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>Folyamatos módra vált.
Ha engedélyezve van, minden befejezett rajzeszköz automatikusan újraindul.
Ezzel egymás után több tárgyat is rajzolhat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation>Normál/drótvázas kijelzés váltása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>A kijelölt tárgyak megjelenítési módját sík vonalakról drótvázra és vissza váltja.
Ez akkor hasznos, ha gyorsan megjeleníti a más tárgyak elől rejtett tárgyakat.
Ez zárt alakzatokhoz és szilárd testekhez készült, és nem befolyásolja a nyitott vonalakat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>Rács kapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>Tervrajz rácsának be- és kikapcsolása.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation>Levág-Bővít (trimex)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="82"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>Levágja vagy bővíti a kijelölt tárgyat, vagy egyetlen felületet bővít.
CTRL illesztéshez, SHIFT-el kényszeríti az aktuális szakaszhoz vagy normális állapothoz, ALT megfordítja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>Frissít</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>A kijelölt tárgyakat összetettebb alakzatokra frissíti.
A művelet eredménye a egymás után többször frissíthető tárgyaktól függ.
Egyesítheti például a kijelölt tárgyakat egyetlen tárgyba, vagy átalakíthatja az egyszerű szegélyeket parametrikus vonalláncokká. A zárt élek kitöltött felületekké és paraméteres sokszögekké alakíthatók. A felületek egyesíthetők is.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="306"/>
      <source>Polyline</source>
      <translation>Vonallánc</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="309"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>Többpontos vonalat (vonalláncot) hoz létre. CTRL illesztéshez, SHIFT kényszerítéshez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
      <source>Wire to B-spline</source>
      <translation>Drótháló B-görbéhez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>A kijelölt vonalláncot B-görbévé vagy egy B-görbét vonallánccá alakítja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>Munkasík proxy létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>Proxy tárgyat hoz létre az aktuális munkasíkból.
A tárgy létrehozása után kattintson rá duplán a fa nézetben a kamera helyzetének és a tárgy láthatóságának visszaállításához.
Ezután használhatja különböző kamera helyzetek mentéséhez és a tárgyakat állapotát bármikor, amikor akarja.</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>Munkasík beállítása</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>Jelöljön ki egy felületet vagy munkasík-proxyt vagy 3 csúcspontot.
Vagy válasszon az alábbi lehetőségek közül</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>A munkasíkot az XY síkra állítja (alapsík)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>Fellülnézet (XY)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>A munkasíkot az XZ síkra állítja (első sík)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>Elölnézet (XZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>A munkasíkot az YZ síkra állítja (oldalsík)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>Oldalnézet (YZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>A munkasíkot az aktuális nézet felé állítja</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>Igazítás a nézethez</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>A munkasík minden parancs elindításakor
igazodik az aktuális nézethez</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>Automatikus</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="87"/>
      <source>Offset</source>
      <translation>Eltolás</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="94"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>Egyéni eltolás, amely az alappozíció feletti
munkasíknak adható. Használja ezt a fenti
gombok egyikével együtt</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="106"/>
      <location filename="../ui/TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>Ha ez be van jelölve, a munkasík az aktuális nézet
középre igazodik, amikor megnyomja a fenti
gombok egyikét</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>Sík központ a nézeten</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>Vagy jelöljön ki egyetlen csúcspontot az aktuális munkasík
áthelyezésére a tájolásának megváltoztatása nélkül..
Ezután nyomja meg az alábbi gombot</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>A munkasík mozgatása a tájolásának 
megváltoztatása nélkül. Ha nincs kijelölve pont,
a sík a nézet közepére kerül</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>Munkasík áthelyezése</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="161"/>
      <location filename="../ui/TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>A kisebb rácsvonalak közötti távolság</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>Rács térköze</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="181"/>
      <location filename="../ui/TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>A rács egyes fővonalai közötti négyzetek száma</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>Fő vonal minden</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="198"/>
      <source>Grid extension</source>
      <translation>Rácsmeghosszabbítás</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="205"/>
      <source> lines</source>
      <translation> vonalak</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="218"/>
      <location filename="../ui/TaskSelectPlane.ui" line="230"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>Az a távolság, amivel egy pontot lehet illeszteni, amikor
közeledik az egér. Ezt az értéket az [ és ] kulcsszóval is 
módosíthatja rajzolás közben</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="223"/>
      <source>Snapping radius</source>
      <translation>Illesztő sugár</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>A nézet középre a jelenlegi munkasíkon</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>Nézet közepe</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>Visszaállítja a munkasíkot az előző pozícióba</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>Előző</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>Stílus beállítások</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="22"/>
      <source>Fills the values below with a stored style preset</source>
      <translation>Az alábbi értékeket egy tárolt stíluskészlettel tölti ki</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="26"/>
      <source>Load preset</source>
      <translation>Sablon betöltése</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="40"/>
      <source>Save current style as a preset...</source>
      <translation>Mentse az aktuális stílust előre beállítottként...</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="57"/>
      <source>Lines and faces</source>
      <translation>Egyenesek és felületek</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
      <source>Line color</source>
      <translation>Vonalszín</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
      <source>The color of lines</source>
      <translation>A vonalak színe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
      <source>Line width</source>
      <translation>Vonalvastagság</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
      <source> px</source>
      <translation> px</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
      <source>Draw style</source>
      <translation>Rajzolási stílus</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
      <source>The line style</source>
      <translation>A vonal stílusa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
      <source>Solid</source>
      <translation>Szilárd test</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
      <source>Dashed</source>
      <translation>Szaggatott</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
      <source>Dotted</source>
      <translation>Pontozott</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
      <source>DashDot</source>
      <translation>Pontvonal</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
      <source>Display mode</source>
      <translation>Megjelenítési mód</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
      <source>The display mode for faces</source>
      <translation>A felületek megjelenítési módja</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
      <source>Flat Lines</source>
      <translation>Lapos vonalak</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
      <source>Wireframe</source>
      <translation>Drótváz</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
      <source>Shaded</source>
      <translation>Árnyékolt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
      <source>Points</source>
      <translation>Pontok</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
      <source>Shape color</source>
      <translation>Alakzat színe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
      <source>The color of faces</source>
      <translation>A felületek színe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
      <source>Transparency</source>
      <translation>Áttetszőség</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
      <source>The transparency of faces</source>
      <translation>A felületek átláthatósága</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
      <source>Annotations</source>
      <translation>Magyarázó szövegek</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
      <source>Text font</source>
      <translation>Szöveg betűtípusa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
      <source>The font to use for texts and dimensions</source>
      <translation>A szövegekhez és méretekhez használt betűtípus</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
      <source>Text size</source>
      <translation>Szövegméret</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
      <source>The size of texts and dimension texts</source>
      <translation>A szövegek és dimenziószövegek mérete</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
      <source>Text spacing</source>
      <translation>Szövegtérköz</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
      <source>The space between the text and the dimension line</source>
      <translation>A szöveg- és a méretvonal távolsága</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
      <source>Text color</source>
      <translation>Szöveg szín</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
      <source>The color of texts and dimension texts</source>
      <translation>A szövegek és dimenziószövegek színe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
      <source>Line spacing</source>
      <translation>Sorköz</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
      <source>The spacing between different lines of text</source>
      <translation>A különböző szövegsorok közötti térköz</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
      <source>Arrow style</source>
      <translation>Nyíl stílusa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
      <source>The type of dimension arrows</source>
      <translation>A dimenziónyilak típusa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
      <source>Dot</source>
      <translation>Pont</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
      <source>Circle</source>
      <translation>Kör</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
      <source>Arrow</source>
      <translation>Nyíl</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
      <source>Tick</source>
      <translation>Jelölők</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
      <source>Tick-2</source>
      <translation>Jelölők-2</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
      <source>Arrow size</source>
      <translation>Nyíl méret</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
      <source>The size of dimension arrows</source>
      <translation>A méretnyilak mérete</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
      <source>Show unit</source>
      <translation>Mértékegység megjelenítés</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>A méretszöveg mértékegységeit megjelenítse vagy sem</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
      <source>Unit override</source>
      <translation>Mértékegység felülírás</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>A méretekhez használó egység. Hagyja üresen az aktuális FreeCAD mértékegységhez</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
      <source>Apply above style to selected object(s)</source>
      <translation>A fenti stílus alkalmazása kijelölt tárgy(ak)ra</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
      <source>Selected</source>
      <translation>Kiválasztott</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
      <source>Texts/dims</source>
      <translation>Szöveg / méretek</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="14"/>
      <source>Form</source>
      <translation>Űrlap</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="20"/>
      <source>PAT file:</source>
      <translation>PAT fájl:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="27"/>
      <source>pattern files (*.pat)</source>
      <translation>minta fájlok (*.pat)</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="34"/>
      <source>Pattern:</source>
      <translation>Minta:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="44"/>
      <source>Scale</source>
      <translation>Méretezés</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="64"/>
      <source>Rotation:</source>
      <translation>Elforgatás:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="71"/>
      <source>°</source>
      <translation>°</translation>
    </message>
  </context>
  <context>
    <name>Gui::Dialog::DlgSettingsDraft</name>
    <message>
      <location filename="../ui/preferences-draft.ui" line="14"/>
      <source>General settings</source>
      <translation>Általános beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>Általános Tervrajz beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>Alapértelmezett munkasík</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>Egyik sem</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (Felülről)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (Elölről)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (Oldalról)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>Belső pontossági szint</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>Tizedesek száma a belső koordinátás műveletekhez (pl. 3 = 0.001). A 6 és 8 közötti értékeket általában a FreeCAD használói között a legjobban elterjedtek.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="143"/>
      <source>Tolerance</source>
      <translation>Tűrés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="169"/>
      <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
      <translation>Ez a tűréshatárt használó függvények által használt érték.
A fenti értéknél kisebb különbségű értékeket azonos értékekként fogja kezelni. Ez az érték hamarosan elavulnak, így a pontossági szint mindkettőt felülértékeli.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>Ha ez bejelölt, a rétegek legördülő listájában csoportok is megjelennek, így automatikusan hozzáadhat tárgyakat a csoportokhoz.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>Csoportok megjelenése a réteglistán legördülő gomb</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>Tervrajz eszközök beállításai</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>Vonalak rajzolásakor állítsa a fókuszt a hosszra az X koordináta helyett.
Ez lehetővé teszi kijelölni az irányt és beírni a távolságot.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>Fókusz beállítása a hosszra az X koordináta helyett</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="247"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>Általában a tárgyak másolása után a másolatok kerülnek kijelölésre.
Ha ez a beállítás be van jelölve, a kiindulási tárgyak lesznek kijelölve.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="251"/>
      <source>Select base objects after copying</source>
      <translation>Válassza ki a bázis objektumokat másolás után</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="264"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>Ha be van állítva ez a lehetőség, akkor ha a tervrajz objektumokat egy másik objektum meglévő felülete fölé szeretné létrehozni, akkor a tervrajz objektum "Támogatás " tulajdonsága az alapobjektumra lesz beállítva. Ez volt a FreeCAD 0.19 előtt az alapértelmezett viselkedés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="267"/>
      <source>Set the Support property when possible</source>
      <translation>A Támogatás tulajdonság beállítása, ha lehetséges</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="280"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>Ha ez be van jelölve, az tárgyak alapértelmezés szerint kitöltöttként jelennek meg.
Ellenkező esetben drótvázként jelennek meg</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="284"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>Töltse ki a tárgyakat felületekkel amikor csak lehetséges</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation>Ha ez be van jelölve, a másolási módot tartja parancsként,
ellenkező esetben a parancsok mindig másolás nélküli módban indulnak el</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>Globális másolási üzemmód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation>A tervrajz eszközök kényszerítése rész-alaptestek létrehozására tervrajz tárgyak helyett.
Ne feledje, hogy ez nem teljes mértékben támogatott, és sok tárgy nem szerkeszthető a tervrajz módosítókkal.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>Ha elérhető alaptest alkatrészeket használ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>Klónozás ezzel előtag felirattal:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>Építési geometria</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>Konstrukció-csoport neve</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>Ez az alapértelmezett csoport-név, az építési geometriánál</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>Építési</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>Építési geometria színe</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>Ez az alapértelmezett színe a tárgyaknak, az építési módban.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>Vizuális beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>Vizuális beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>Illesztés szimbólumok stílusa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Klasszikus stílusú tervrajz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Bitekésdarabok stílus</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>Szín</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>Illesztés szimbólum alapértelmezett színe</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>Jelölje be, ha alapértelmezettként szeretné használni a szín/vonalvastagságot az eszköztárból</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>Mentse az aktuális színt és vonalvastagságot erre a munkamenetre</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>Ha be van jelölve, a kisalkalmazás az aktuális munka sík tájolását jelzi és a rajz műveletek során jelenik meg</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>Munka sík követő megjelenítése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>Alapértelmezett rajzlap sablon</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>Az alapértelmezett sablon, ami alapján létrehoz egy új rajzlapot</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="194"/>
      <source>Alternate SVG patterns location</source>
      <translation>Alternatív SVG minták helye</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="220"/>
      <source>Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</source>
      <translation>Itt megadhat egy könyvtárat egyéni SVG fájlokkal, amelyek &lt;pattern&gt; szabványos mintákhoz hozzáadandó definíciókat tartalmaznak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="237"/>
      <source>SVG pattern resolution</source>
      <translation>SVG minta felbontása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="257"/>
      <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
      <translation>A felbontás a minta rajzolásához. Az alapértelmezett érték 128. A magasabb értékek jobb felbontást adnak, az alacsonyabb értékek gyorsabbak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="280"/>
      <source>SVG pattern default size</source>
      <translation>SVG minta alapértelmezett mérete</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="300"/>
      <source>The default size for SVG patterns</source>
      <translation>Az SVG minták alapértelmezett mérete</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="326"/>
      <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
      <translation>Jelölje be, ha meg szeretné őrizni a felületek színeit azok visszaállításánál és frissítésénél (csak splitFaces és makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>Az felületi színek megőrzése visszaállítás/frissítés során</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>Jelölje ezt, ha azt szeretné, hogy a felület nevek az objektum nevéből származzanak, és fordítva, a visszaállítás/frissítés esetén (csak splitFaces és makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>A felület neveinek megőrzése visszaállítás/frissítés során</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>Rajz nézet egyenes meghatározásai</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>Szaggatott egyenes meghatározása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="395"/>
      <location filename="../ui/preferences-draftvisual.ui" line="438"/>
      <location filename="../ui/preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>Egy SVG vonalstílus meghatározása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>Pont-vonal egyenes meghatározása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>Pontozott egyenes meghatározása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>Szövegek és méretek</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>Szöveg beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>Betűkészlet</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>Ez az alapértelmezett betűtípus név minden tervrajz szöveghez és a mérethez. 
Ez lehet egy betűtípus név, mint a "Arial", alapértelmezett stílus, mint a "sans", "serif" 
vagy "mono", vagy egy család, mint például "Arial, Helvetica, sans", vagy egy név stílussal, 
mint a " Arial: Dőlt "</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>Belső betűtípus</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>Betűméret</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>Alapértelmezett szöveg és méretezés magasság</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="535"/>
      <location filename="../ui/preferences-drafttexts.ui" line="92"/>
      <location filename="../ui/preferences-drafttexts.ui" line="211"/>
      <location filename="../ui/preferences-drafttexts.ui" line="247"/>
      <location filename="../ui/preferences-drafttexts.ui" line="283"/>
      <location filename="../ui/preferences-drafttexts.ui" line="365"/>
      <location filename="../ui/preferences-drafttexts.ui" line="432"/>
      <location filename="../ui/preferences-svg.ui" line="209"/>
      <source>mm</source>
      <translation>mm</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="116"/>
      <source>Dimension settings</source>
      <translation>Távolság beálítás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>Megjelenítési mód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>szöveg felette (2D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation> szöveg belül (3D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>Tizedesjegyek száma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>Meghoszabbító vonalak méret</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>A méret segédvonalak alapértelmezett mérete</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>Meghoszabbító vonal túllépése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>A meghosszabbító vonal alapértelmezett mérete a méretsegédvonal felett</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>Méret segédvonal túllépése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>A meghoszabbítás utáni méretsegédvonal meghosszabbításának alapértelmezett hossza</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>Nyilak-stílus</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>Pont</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>Kör</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>Nyíl</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>Jelölők</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>Jelölők-2</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>Nyíl mérete</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>A nyilak alapértelmezett mérete</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>Szöveg tájolása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>Ez a méretezési szöveg irányultsága amikor ezek iránya függőleges. Alapértelmezett a bal, ami az ISO-szabvány.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>Bal (ISO szabvány)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>Jobb</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>Szövegtérköz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>A méret segédvonalai és a méret szövegrész közti távolság</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>A dimenziók mértékegység utótagjainak megjelenítése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>Felülbírálási mértékegység</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>Ha üresen hagyja ezt a mezőt, a méretmérések a FreeCAD-ben meghatározott aktuális egységben jelennek meg. Ha itt megjelöl egy egységet, például m-et vagy cm-t, erőltetheti az új méreteket, hogy megmutatkozzanak a mértékegységben.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>LáncMinta beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>Alapértelmezett a LáncMinta betű fájl</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>Válasszon ki egy betűtípus fájlt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>Stílus importálása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>Az SVG-objektum színének a FreeCAD-be történő importálásához választott módszer</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>Nincs (leggyorsabb)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>Alapértelmezett szín és vonal vastagság</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>Eredeti szín és vonalvastagság</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>Ha be van jelölve, nem történik mértékegység konverzió.
Az SVG fájl egy egységét egy milliméteresre fordítja. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>Tiltsa le a mértékegység léptékezését</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>Export stílus</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>Vázlat exportálásakor írható SVG-fájl stílusa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>Átalakítva (nyomtatáshoz &amp; képernyőre)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>Nyers (CAM-hoz)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>Minden fehér vonal fekete színben jelenik meg az SVG-ben a fehér háttérrel szembeni jobb olvashatóság érdekében</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>Fehér vonal szín feketére váltása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>Max szegmens hossza a mérlegelt ívekhez</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>A 6.8-as verziónál régebbi Open CASCADE verziók nem támogatják az ívvetítést.
Ebben az esetben az íveket kis vonalszakaszokká osztja.
Ez az érték egy szegmens maximális hossza. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>Ellenőrizze ezt, ha a területeket (3D felületek) is importálni kell.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>OCA területek importálása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="35"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>Ez a beállítás párbeszédpanel a DXF-fájlok importálása/exportálásakor jelenik meg</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="38"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>Importálás és Exportálás alatt ezt a párbeszédpanelt mutassa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="51"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>Python importőrt használnak, ellenkező esetben az újabb C++-t használják.
Megjegyzés: A C++ importőr gyorsabb, de még nem olyan jellemző</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="55"/>
      <source>Use legacy python importer</source>
      <translation>Használj örökölt python importálót</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="71"/>
      <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</source>
      <translation>Ha a lehetőséget választja, a Python exportőrt használja, ellenkező esetben egy újabb C++ exportőrt használ.
Megjegyzés: A C++ exportőr gyorsabb, de még nem olyan funkcionális</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="75"/>
      <source>Use legacy python exporter</source>
      <translation>Használj örökölt python exportálót</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="88"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>Automatikus frissítés (csak az örökölt importálása)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="96"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>Engedélyezze a FreeCAD számára a Python átalakító letöltését DXF importáláshoz és exportáláshoz.
Ezt manuálisan is megteheti a "dxf_library" munkafelülettel
a Kiegészítők kezelőjéből.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="101"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>Engedélyezze a FreeCAD-hoz az automatikus DXF-könyvtárak letöltését és frissítését</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="26"/>
      <location filename="../ui/preferences-dxf.ui" line="119"/>
      <location filename="../ui/preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>Importálási beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="140"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>Megjegyzés: Még nem az összes alábbi lehetőséget használja új importőr</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="149"/>
      <source>Import</source>
      <translation>Importálás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="156"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>Ha nincs bejelölve, a szövegeket és az bekezdéseket nem importálja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="159"/>
      <source>texts and dimensions</source>
      <translation>szövegek és méretek</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="172"/>
      <source>If unchecked, points won't be imported</source>
      <translation>Ha nincs bejelölve, a pontokat nem importálja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="175"/>
      <source>points</source>
      <translation>pontok</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="188"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>Ha bejelölt, a papír területek tárgyait is importálja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="191"/>
      <source>layouts</source>
      <translation>elrendezések</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="204"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>Ha importálni szeretné a név nélküli blokkokat (*-al kezdődő) is</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="207"/>
      <source>*blocks</source>
      <translation>*blokkok</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="224"/>
      <source>Create</source>
      <translation>Létrehozás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="231"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>Csak az általános alkatrész tárgyakat hozza létre (leggyorsabb)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="234"/>
      <source>simple Part shapes</source>
      <translation>egyszerű alkatrész alakzatok</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="250"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>Parametrikus tervrajt tárgyak jönnek létre, amikor csak lehetséges</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="253"/>
      <source>Draft objects</source>
      <translation>Tervezet tárgyak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="266"/>
      <source>Sketches will be created whenever possible</source>
      <translation>Vázlatok jönnek létre, amikor csak lehetséges</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="269"/>
      <source>Sketches</source>
      <translation>Vázlatok</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="289"/>
      <source>Scale factor to apply to imported files</source>
      <translation>Importált fájlokra vonatkozó méretezési léptéktényező</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="309"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>Importáláskor a DXF-fájlokra alkalmazandó méretezési tényező.
A tényező a DXF-fájl egysége és a milliméterek közötti konverzió.
Példa: milliméterben lévő fájlokhoz: 1, centiméterben: 10,
                             méterben: 1000, hüvelykben: 25,4, lábban: 304,8</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="338"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>A színek lehetőség szerint a DXF-tárgyakból kerülnek beolvasásra.
Ellenkező esetben az alapértelmezett színeket alkalmazza. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="342"/>
      <source>Get original colors from the DXF file</source>
      <translation>A DXF fájl eredeti színeit vegye</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="359"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>A FreeCAD megpróbálja az egybeeső tárgyakat dróthálóba illeszteni.
Ne feledje, hogy ez egy ideig is tart!</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="363"/>
      <source>Join geometry</source>
      <translation>Geometria csatlakoztatása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="380"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>Az azonos rétegekből származó tárgyak tervrajz blokkokba egyesülnek,
gyorsabban megjeleníthetővé, de kevésbé könnyen szerkeszthetővé válik </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="384"/>
      <source>Group layers into blocks</source>
      <translation>Csoport rétegek tömbökké</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="401"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>Az importált szövegek a szabványos tervrajz szöveg méretet kapják,
a DXF-dokumentumban található méret helyett</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="405"/>
      <source>Use standard font size for texts</source>
      <translation>Használja az alap betűméretet a szövegekhez</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="422"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>Ha ez bejelölt, a DXF-rétegek tervrajz rétegekként kerülnek importálva</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="425"/>
      <source>Use Layers</source>
      <translation>Rétegek használata</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="445"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>A straffozási minták egyszerű vonalakká alakulnak át</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="448"/>
      <source>Import hatch boundaries as wires</source>
      <translation>Kitöltési határok importálása vonalként</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="465"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>Ha a megadott vonalláncnak van szélesség meghatározva, 
akkor összeolvasztja a vonalakat a megfelelő vastagság használatával</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="469"/>
      <source>Render polylines with width</source>
      <translation>Összekapcsolt vonalláncok vastagságának igazítása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="486"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>Ellipszis exportot kevésbé támogatja. Ennek segítségével exportálhatja vonalláncokként.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="489"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>Ellipsziseket és görbe vonalakat vonalláncokként kezelje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="518"/>
      <source>Max Spline Segment:</source>
      <translation>Max görbületi szegmens:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="528"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>Az egyes vonalláncszegmensek maximális hossza.
Ha '0'-ra van állítva, a teljes csíkozást egyenes szakaszként kezeli.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="559"/>
      <location filename="../ui/preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>Exportálási beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="567"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>A felületeket tartalmazó összes objektum 3D többfelületűként exportálja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="570"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>3D objektum exportálása többfelületű hálórajzzá</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="587"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>A rajznézetek blokként exportálja.
Ez sikertelen lehet a DXF R12 utáni sablonokon.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="591"/>
      <source>Export Drawing Views as blocks</source>
      <translation>Rajz nézetek exportálása blokkokként</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="611"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>Az exportált objektumok kivetítve az aktuális nézet irányát tükrözik</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="614"/>
      <source>Project exported objects along current view direction</source>
      <translation>Exportált objektumok kivetítése az aktuális nézet iránya mentén</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>Rács és illesztés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="35"/>
      <source>Snapping</source>
      <translation>Illesztés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="43"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>Ha ez be van jelölve, az illesztéshez nincs szükség az illesztés gomb megnyomására</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="46"/>
      <source>Always snap (disable snap mod)</source>
      <translation>Mindig illeszt (kiiktatja az illesztési módot)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="66"/>
      <source>Constrain mod</source>
      <translation>Kényszerítő mód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="86"/>
      <source>The Constraining modifier key</source>
      <translation>A kényszerítést módosító billentyű</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="96"/>
      <location filename="../ui/preferences-draftsnap.ui" line="151"/>
      <location filename="../ui/preferences-draftsnap.ui" line="206"/>
      <source>Shift</source>
      <translation>Shift</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="101"/>
      <location filename="../ui/preferences-draftsnap.ui" line="156"/>
      <location filename="../ui/preferences-draftsnap.ui" line="211"/>
      <source>Ctrl</source>
      <translation>Ctrl</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="106"/>
      <location filename="../ui/preferences-draftsnap.ui" line="161"/>
      <location filename="../ui/preferences-draftsnap.ui" line="216"/>
      <source>Alt</source>
      <translation>Alt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="118"/>
      <source>Snap mod</source>
      <translation>Illesztési mód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="138"/>
      <source>The snap modifier key</source>
      <translation>Az illesztő módosító billentyű</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="173"/>
      <source>Alt mod</source>
      <translation>Alt mód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="193"/>
      <source>The Alt modifier key</source>
      <translation>Az Alt módosító billentyű</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="228"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>Ha be van jelölve, az illesztési eszköztár jelenik meg ha illeszteni akar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="231"/>
      <source>Show Draft Snap toolbar</source>
      <translation>Illesztési pont eszköztár megjelenítése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="251"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>A használat után a rajz illesztési pont eszközablak elrejtése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="272"/>
      <source>Grid</source>
      <translation>Rács</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="278"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>Ha be van jelölve, egy rács jelenik meg, ha rajzol</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="281"/>
      <source>Use grid</source>
      <translation>Rács használata</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="300"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>Ha kijelölt, a Tervrajz rács mindig látható lesz, ha a Tervrajz munkafelület aktív. Egyébként csak akkor, ha parancsot használ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="303"/>
      <source>Always show the grid</source>
      <translation>Mindig jelenítse meg a rácsot</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="319"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>Ha be van jelölve, egy további szegély jelenik meg a rács körül, amely a bal alsó határ fő négyzet méretét mutatja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="322"/>
      <source>Show grid border</source>
      <translation>Rács szegélyének megmutatása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="338"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If checked, the outline of a human figure is displayed at the bottom left corner of the grid. This option is only effective if the BIM workbench is installed and if &amp;quot;Show grid border&amp;quot; option is enabled.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Ha ez a beállítás bejelölt, az emberi alak sziluettje a rács bal alsó sarkában jelenik meg. Ez a beállítás csak akkor érvényes, ha a BIM munkakörnyezet telepítve van, és ha a &amp;quot;Rácsszegély megjelenítése&amp;quot; beállítás engedélyezve van.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="341"/>
      <source>Show human figure</source>
      <translation>Emberi alak megjelenítése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="357"/>
      <source>If set, the grid will have its two main axes colored in red, green or blue when they match global axes</source>
      <translation>Ha be van állítva, a rács két fő tengelye piros, zöld vagy kék színű lesz, amikor megfelelnek a globális tengelyeknek</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="360"/>
      <source>Use colored axes</source>
      <translation>Színes tengelyek használata</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="381"/>
      <source>Main lines every</source>
      <translation>Minden egyes fővonal</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="404"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>A fő vonalak vastagabb rajzolatúak. Adja meg, mennyi négyzet legyen a fővonalak közt.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="430"/>
      <source>Grid spacing</source>
      <translation>Rács térköze</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="453"/>
      <source>The spacing between each grid line</source>
      <translation>A rács vonalainak egymás közti távolságai</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="485"/>
      <source>Grid size</source>
      <translation>Rácsméret</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="505"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>Vízszintes vagy függőleges rácsvonalak száma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="511"/>
      <source> lines</source>
      <translation> vonalak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="534"/>
      <source>Grid color and transparency</source>
      <translation>Rács színe és átlátszósága</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="554"/>
      <source>The color of the grid</source>
      <translation>A rács színe</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="574"/>
      <source>The overall transparency of the grid</source>
      <translation>A háló általános átláthatósága</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="595"/>
      <source>Draft Edit preferences</source>
      <translation>Tervrajz szerkesztési beállítások</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="598"/>
      <source>Edit</source>
      <translation>Szerkesztés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="621"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>Az egyidejűleg feldolgozott objektumok maximális száma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="644"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Beállítja a tervrajban szerkeszthető tárgyak maximális számát&lt;/p&gt;&lt;p&gt; ami egyszerre szerkeszthető&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="691"/>
      <source>Draft edit pick radius</source>
      <translation>Tervrajz szerkesztés kiválasztó sugara</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="714"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>Szabályozza a szerkesztési csomópontok kiválasztó sugarát</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>DWG átalakítás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="34"/>
      <source>Conversion method:</source>
      <translation>Konverziós módszer:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="41"/>
      <source>This is the method FreeCAD will use to convert DWG files to DXF. If "Automatic" is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the "dwg2dxf" utility if using LibreDWG, "ODAFileConverter" if using the ODA file converter, or the "dwg2dwg" utility if using the pro version of QCAD.</source>
      <translation>Ez az a módszer, amelyet a FreeCAD a DWG fájlok DXF-re konvertálására fog használni. Ha az "Automatikus" opció van kiválasztva, a FreeCAD megpróbálja megtalálni az alábbi konverterek egyikét ugyanabban a sorrendben, amelyben itt láthatók. Ha a FreeCAD nem találja egyiket sem, előfordulhat, hogy ki kell választania egy adott konvertert, és itt meg kell adnia annak elérési útját. Válassza ki a "dwg2dxf" segédprogramot, ha LibreDWG-t, "ODAFileConverter"-t használ, ha ODA fájlkonvertert használ, vagy a "dwg2dwg" segédprogramot, ha a QCAD Pro verzióját használja.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="51"/>
      <source>Automatic</source>
      <translation>Automatikus</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="56"/>
      <source>LibreDWG</source>
      <translation>LibreDWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="61"/>
      <source>ODA Converter</source>
      <translation>ODA Konvertáló</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="66"/>
      <source>QCAD pro</source>
      <translation>QCAD pro</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="78"/>
      <source>Path to file converter</source>
      <translation>Fájlkonverter elérési útja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="85"/>
      <source>The path to your DWG file converter executable</source>
      <translation>A DWG fájl konverter végrehajtható fájl elérési útja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="100"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Note:&lt;/span&gt; DXF options apply to DWG files as well.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Megjegyzés:&lt;/span&gt; DXF lehetőségek a DWG fájlokra is érvényesek.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>Felhasználói felület beállításai</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>Billentyű-parancsok a parancsokban</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="37"/>
      <source>Relative</source>
      <translation>Relatív</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="59"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="81"/>
      <source>Continue</source>
      <translation>Tovább</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="103"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="125"/>
      <source>Close</source>
      <translation>Bezárás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="147"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="169"/>
      <source>Copy</source>
      <translation>Másolás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="191"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="213"/>
      <source>Subelement Mode</source>
      <translation>Al-elem mód</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="235"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="257"/>
      <source>Fill</source>
      <translation>Kitöltés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="279"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="301"/>
      <source>Exit</source>
      <translation>Kilépés</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="323"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="345"/>
      <source>Select Edge</source>
      <translation>Válasszon élt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="367"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="389"/>
      <source>Add Hold</source>
      <translation>Tartás hozzáadása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="411"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="433"/>
      <source>Length</source>
      <translation>Hossz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="455"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="477"/>
      <source>Wipe</source>
      <translation>Radíroz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="499"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="521"/>
      <source>Set WP</source>
      <translation>WP beállítás</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="543"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="565"/>
      <source>Cycle Snap</source>
      <translation>Illesztés váltogatása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="587"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="609"/>
      <source>Global</source>
      <translation>Globális</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="631"/>
      <source>G</source>
      <translation>G</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="653"/>
      <source>Snap</source>
      <translation>Illeszt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="675"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="697"/>
      <source>Increase Radius</source>
      <translation>Sugár növelése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="719"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="741"/>
      <source>Decrease Radius</source>
      <translation>Sugár csökkentése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="763"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="785"/>
      <source>Restrict X</source>
      <translation>X korlátozása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="807"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="829"/>
      <source>Restrict Y</source>
      <translation>Y korlátozása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="851"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="873"/>
      <source>Restrict Z</source>
      <translation>Z korlátozása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="895"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="928"/>
      <source>Enable draft statusbar customization</source>
      <translation>Tervrajz állapotsor testreszabás engedélyezése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="931"/>
      <source>Draft Statusbar</source>
      <translation>Tervrajz állapotsor</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="951"/>
      <source>Enable snap statusbar widget</source>
      <translation>Illesztő modul állapotsor engedélyezése</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="954"/>
      <source>Draft snap widget</source>
      <translation>Tervrajz illesztés modul</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="970"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>Tervező állapotsáv jegyzet skála modul aktiválása</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="973"/>
      <source>Annotation scale widget</source>
      <translation>Jegyzet méretezés modul</translation>
    </message>
  </context>
  <context>
    <name>ImportAirfoilDAT</name>
    <message>
      <location filename="../../importAirfoilDAT.py" line="193"/>
      <source>Did not find enough coordinates</source>
      <translation>Nem találtam elég koordinátát</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="../../importSVG.py" line="1796"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation>Ismeretlen SVG exportálási stílus fordításra vált</translation>
    </message>
    <message>
      <location filename="../../importSVG.py" line="1816"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>Az exportálási lista nem tartalmaz érvényes határolókerettel rendelkező tárgyat</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../InitGui.py" line="104"/>
      <source>Draft creation tools</source>
      <translation>Tervrajzkészítő eszközök</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="107"/>
      <source>Draft annotation tools</source>
      <translation>Tervrajz megjegyzési eszközök</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="110"/>
      <source>Draft modification tools</source>
      <translation>Tervrajz módosítási eszközök</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="113"/>
      <source>Draft utility tools</source>
      <translation>Tervezőeszközök</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="118"/>
      <source>&amp;Drafting</source>
      <translation>Tervrajzkészítés</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="121"/>
      <source>&amp;Annotation</source>
      <translation>Szövegmagyarázat</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="124"/>
      <source>&amp;Modification</source>
      <translation>&amp;Módosítás</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="127"/>
      <source>&amp;Utilities</source>
      <translation>&amp;Eszközök</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="50"/>
      <source>Arc tools</source>
      <translation>Íveszközök</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="58"/>
      <source>Bézier tools</source>
      <translation>Bezier-görbe eszközök</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="89"/>
      <source>Array tools</source>
      <translation>Elrendezés eszközei</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
      <source>Draft Snap</source>
      <translation>Tervrajz illesztése</translation>
    </message>
  </context>
  <context>
    <name>draft</name>
    <message>
      <location filename="../../importDXF.py" line="146"/>
      <source>The DXF import/export libraries needed by FreeCAD to handle
the DXF format were not found on this system.
Please either enable FreeCAD to download these libraries:
  1 - Load Draft workbench
  2 - Menu Edit &gt; Preferences &gt; Import-Export &gt; DXF &gt; Enable downloads
Or download these libraries manually, as explained on
https://github.com/yorikvanhavre/Draft-dxf-importer
To enabled FreeCAD to download these libraries, answer Yes.</source>
      <translation>A DXF import/export könyvtárak szükségesek a FreeCAD részére
a rendszerben nem található DXF formátum kezeléséhez.
Kérem vagy a FreeCAD könyvtárak letöltésének engedélyezését:
  1 - Tervrajz  munkafelület kiválasztása
  2 - Szerkesztés menü &gt; Beállítások &gt; Import-Export &gt; DXF &gt; Letöltések bekapcsolása
Vagy töltse le kézzel ezeket a könyvtárakat, itt leírtak szerint:
https://github.com/yorikvanhavre/Draft-dxf-importer
A FreeCAD letöltésének bekapcsolásához válassza az "Igen"-t.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="57"/>
      <location filename="../../DraftGui.py" line="751"/>
      <source>Relative</source>
      <translation>Relatív</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="61"/>
      <location filename="../../DraftGui.py" line="756"/>
      <source>Global</source>
      <translation>Globális</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="66"/>
      <location filename="../../DraftGui.py" line="774"/>
      <location filename="../../DraftGui.py" line="1126"/>
      <source>Continue</source>
      <translation>Tovább</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="71"/>
      <location filename="../../DraftGui.py" line="790"/>
      <source>Close</source>
      <translation>Bezárás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="76"/>
      <location filename="../../DraftGui.py" line="801"/>
      <location filename="../../draftguitools/gui_move.py" line="207"/>
      <location filename="../../draftguitools/gui_scale.py" line="203"/>
      <location filename="../../draftguitools/gui_scale.py" line="227"/>
      <location filename="../../draftguitools/gui_scale.py" line="356"/>
      <location filename="../../draftguitools/gui_rotate.py" line="283"/>
      <source>Copy</source>
      <translation>Másolás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="81"/>
      <source>Subelement mode</source>
      <translation>Alelem mód</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="86"/>
      <source>Fill</source>
      <translation>Kitöltés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="91"/>
      <source>Exit</source>
      <translation>Kilépés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="96"/>
      <source>Snap On/Off</source>
      <translation>Illesztés be/ki-kapcsolása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="101"/>
      <source>Increase snap radius</source>
      <translation>Illesztési sugár növelése</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="106"/>
      <source>Decrease snap radius</source>
      <translation>Illesztési sugár csökkentése</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="111"/>
      <source>Restrict X</source>
      <translation>X korlátozása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="116"/>
      <source>Restrict Y</source>
      <translation>Y korlátozása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="121"/>
      <source>Restrict Z</source>
      <translation>Z korlátozása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="126"/>
      <location filename="../../DraftGui.py" line="796"/>
      <source>Select edge</source>
      <translation>Válassza ki az élt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="131"/>
      <source>Add custom snap point</source>
      <translation>Egyéni illesztő pont hozzáadása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="136"/>
      <source>Length mode</source>
      <translation>Hosszanti mód</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="141"/>
      <location filename="../../DraftGui.py" line="792"/>
      <source>Wipe</source>
      <translation>Radíroz</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="146"/>
      <source>Set Working Plane</source>
      <translation>Munka sík beállítás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="151"/>
      <source>Cycle snap object</source>
      <translation>Illesztés objektumok váltogatása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="156"/>
      <source>Toggle near snap on/off</source>
      <translation>Közeli illesztés be- és kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="330"/>
      <source>Draft Command Bar</source>
      <translation>Tervezet parancssor</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="659"/>
      <location filename="../../WorkingPlane.py" line="821"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
      <source>Top</source>
      <translation>Felülnézet</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="661"/>
      <location filename="../../WorkingPlane.py" line="832"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
      <source>Front</source>
      <translation>Elölnézet</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="663"/>
      <location filename="../../WorkingPlane.py" line="843"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
      <source>Side</source>
      <translation>Oldal</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="665"/>
      <source>Auto</source>
      <translation>Automatikus</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="694"/>
      <location filename="../../DraftGui.py" line="729"/>
      <location filename="../../DraftGui.py" line="1058"/>
      <location filename="../../DraftGui.py" line="2047"/>
      <location filename="../../DraftGui.py" line="2062"/>
      <location filename="../../draftguitools/gui_groups.py" line="239"/>
      <location filename="../../draftguitools/gui_groups.py" line="244"/>
      <source>None</source>
      <translation>Egyik sem</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="728"/>
      <source>active command:</source>
      <translation>aktív parancs:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="730"/>
      <source>Active Draft command</source>
      <translation>Aktív tervezési parancs</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="731"/>
      <source>X coordinate of next point</source>
      <translation>Következő pont X koordinátája</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="732"/>
      <location filename="../../DraftGui.py" line="1059"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="733"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="734"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="735"/>
      <source>Y coordinate of next point</source>
      <translation>Következő pont Y koordinátája</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="736"/>
      <source>Z coordinate of next point</source>
      <translation>Következő pont Z koordinátája</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="737"/>
      <source>Enter point</source>
      <translation>Pont megadása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="739"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>Adjon meg egy új pontot a megadott koordinátákkal</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="740"/>
      <source>Length</source>
      <translation>Hossz</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="741"/>
      <location filename="../../draftguitools/gui_trimex.py" line="220"/>
      <source>Angle</source>
      <translation>Szög</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="742"/>
      <source>Length of current segment</source>
      <translation>Aktuális szakasz hossza</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="743"/>
      <source>Angle of current segment</source>
      <translation>Aktuális szakasz szöge</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="747"/>
      <source>Check this to lock the current angle</source>
      <translation>Jelölje be az aktuális szög lezárásához</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="748"/>
      <location filename="../../DraftGui.py" line="1108"/>
      <source>Radius</source>
      <translation>Sugár</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="749"/>
      <location filename="../../DraftGui.py" line="1109"/>
      <source>Radius of Circle</source>
      <translation>A kör sugara</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="754"/>
      <source>Coordinates relative to last point or to coordinate system origin
if is the first point to set</source>
      <translation>Koordináták az utolsó ponthoz vagy a rendszer kezdő koordinátáihoz
ha az első beállítási pont</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="759"/>
      <source>Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</source>
      <translation>Koordináták a globális koordináta-rendszerhez viszonyítva.
A munkasík koordináta-rendszerének használatának feloldása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="761"/>
      <source>Filled</source>
      <translation>Kitöltött</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="765"/>
      <source>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</source>
      <translation>Jelölje be, ha a tárgynak kitöltöttként kell megjelennie, különben drótvázként jelenik meg.
Nem érhető el, ha a 'Rész-primitívek használata' beállítás engedélyezve van</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="767"/>
      <source>Finish</source>
      <translation>Befejezés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="769"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>Befejezi az aktuális rajz vagy szerkesztési műveletet</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="772"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>Ha be van jelölve, a parancs nem fejeződik be, amíg újra meg nem nyomja a parancs gombot</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="777"/>
      <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
      <translation>Ha kijelölt, egy OCC-stílusú eltolás kerül végrehajtásra a klasszikus eltolás helyett</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="778"/>
      <source>&amp;OCC-style offset</source>
      <translation>OCC-stílusú eltolás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="788"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>Visszavonás (CTRL + Z)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="789"/>
      <source>Undo the last segment</source>
      <translation>Utolsó szegmens visszavonása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="791"/>
      <source>Finishes and closes the current line</source>
      <translation>A folyamatban lévő vonal befejezése és lezárása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="793"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>Kiradírozza a meglévő szegmenst ebből a vonalból és ismét az utolsó ponttól kezdi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="794"/>
      <source>Set WP</source>
      <translation>WP beállítás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="795"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>A munkasíkot átállítja az utolsó szegmensen</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="797"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>Ezzel a mérettel történő méréshez válasszon ki egy létező élt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="798"/>
      <source>Sides</source>
      <translation>Oldalak</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="799"/>
      <source>Number of sides</source>
      <translation>Oldalak száma</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="802"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>Ha ki van jelölve, az objektumok másolva lesznek mozgatás helyett. A Beállítások -&gt; Vázlat -&gt; Globális másolás mód a későbbi parancsok esetén</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="803"/>
      <source>Modify subelements</source>
      <translation>Alelemek módosítása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="804"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation>Ha be van jelölve, az al-elemek lesznek módosítva a teljes tárgy helyett</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="805"/>
      <source>Text string to draw</source>
      <translation>Szöveges karakterlánc rajzolás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="806"/>
      <source>String</source>
      <translation>Karakterlánc</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="807"/>
      <source>Height of text</source>
      <translation>Szöveg magassága</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="808"/>
      <source>Height</source>
      <translation>Magasság</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="809"/>
      <source>Intercharacter spacing</source>
      <translation>Karakteren belüli távolság</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="810"/>
      <source>Tracking</source>
      <translation>Léptetés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="811"/>
      <source>Full path to font file:</source>
      <translation>Betűtípus fájl teljes elérési útja:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="812"/>
      <source>Open a FileChooser for font file</source>
      <translation>Nyissa meg a FájlKiválasztót a betűtípus fájlhoz</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="813"/>
      <source>Create text</source>
      <translation>Szöveg létrehozása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="814"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>Nyomja meg ezt a gombot a szöveges tárgy létrehozásához, vagy fejezze be a szöveget két üres vonallal</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="836"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
      <source>Current working plane</source>
      <translation>Jelenlegi munka sík</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="837"/>
      <source>Change default style for new objects</source>
      <translation>Új objektumok alapértelmezett stílusának módosítása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="838"/>
      <source>Toggle construction mode</source>
      <translation>Építési mód váltása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="839"/>
      <location filename="../../DraftGui.py" line="2050"/>
      <location filename="../../DraftGui.py" line="2065"/>
      <source>Autogroup off</source>
      <translation>Autócsoport kikapcsolása</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="950"/>
      <source>Line</source>
      <translation>Vonal</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="958"/>
      <source>DWire</source>
      <translation>Terv-vonal</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="976"/>
      <source>Circle</source>
      <translation>Kör</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="981"/>
      <source>Arc</source>
      <translation>Ív</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="986"/>
      <location filename="../../draftguitools/gui_rotate.py" line="286"/>
      <source>Rotate</source>
      <translation>Forgatás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="990"/>
      <source>Point</source>
      <translation>Pont</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1018"/>
      <source>Label</source>
      <translation>Felirat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1036"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
      <location filename="../../draftguitools/gui_offset.py" line="243"/>
      <location filename="../../draftguitools/gui_offset.py" line="260"/>
      <location filename="../../draftguitools/gui_offset.py" line="324"/>
      <source>Offset</source>
      <translation>Eltolás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1042"/>
      <location filename="../../DraftGui.py" line="1100"/>
      <location filename="../../draftguitools/gui_trimex.py" line="215"/>
      <source>Distance</source>
      <translation>Távolság</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1043"/>
      <location filename="../../DraftGui.py" line="1101"/>
      <location filename="../../draftguitools/gui_trimex.py" line="217"/>
      <source>Offset distance</source>
      <translation>Eltolási távolság</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1097"/>
      <source>Trimex</source>
      <translation>Levág-Bővít (trimex)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1197"/>
      <source>Pick Object</source>
      <translation>Objektum kiválasztás</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1203"/>
      <source>Edit</source>
      <translation>Szerkesztés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1253"/>
      <source>Local u0394X</source>
      <translation>Helyi u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1254"/>
      <source>Local u0394Y</source>
      <translation>Helyi u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1255"/>
      <source>Local u0394Z</source>
      <translation>Helyi u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1257"/>
      <source>Local X</source>
      <translation>Helyi X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1258"/>
      <source>Local Y</source>
      <translation>Helyi Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1259"/>
      <source>Local Z</source>
      <translation>Helyi Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1261"/>
      <source>Global u0394X</source>
      <translation>Globális u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1262"/>
      <source>Global u0394Y</source>
      <translation>Globális u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1263"/>
      <source>Global u0394Z</source>
      <translation>Globális u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1265"/>
      <source>Global X</source>
      <translation>Globális X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1266"/>
      <source>Global Y</source>
      <translation>Globális Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1267"/>
      <source>Global Z</source>
      <translation>Globális Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1503"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>Érvénytelen méret érték. Használja 200.0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1511"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>Léptetés értéke érvénytelen. Használja 0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1525"/>
      <source>Please enter a text string.</source>
      <translation>Adjon meg egy szöveges karakterláncot.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1534"/>
      <source>Select a Font file</source>
      <translation>Válasszon ki egy betűtípus fájlt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1567"/>
      <source>Please enter a font file.</source>
      <translation>Kérjük, írja be a betűtípus fájlt.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2058"/>
      <source>Autogroup:</source>
      <translation>Autocsoport:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2394"/>
      <source>Faces</source>
      <translation>Felületek</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2395"/>
      <source>Remove</source>
      <translation>Törlés</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2396"/>
      <source>Add</source>
      <translation>Hozzáad</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2397"/>
      <source>Facebinder elements</source>
      <translation>Felülettároló elemek</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>Tervrajz</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="209"/>
      <location filename="../../importDWG.py" line="281"/>
      <source>LibreDWG error</source>
      <translation>LibreDWG hiba</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="218"/>
      <location filename="../../importDWG.py" line="290"/>
      <source>Converting:</source>
      <translation>Átalakít:</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="223"/>
      <source>Conversion successful</source>
      <translation>Az átalakítás sikeres</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="226"/>
      <source>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</source>
      <translation>Hiba történt a DWG konverzió során. Próbálja meg áthelyezni a DWG fájlt szóközök és nem angol karakterek nélküli könyvtár elérési útra, vagy próbáljon meg alacsonyabb DWG-verzióra menteni.</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="229"/>
      <location filename="../../importDWG.py" line="296"/>
      <source>ODA File Converter not found</source>
      <translation>ODA fájlkonverter nem található</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="242"/>
      <location filename="../../importDWG.py" line="306"/>
      <source>QCAD error</source>
      <translation>QCAD hiba</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="713"/>
      <location filename="../../draftmake/make_sketch.py" line="127"/>
      <location filename="../../draftmake/make_sketch.py" line="139"/>
      <source>All Shapes must be coplanar</source>
      <translation>Minden alakzatnak együtt kell lennie</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="721"/>
      <source>Selected Shapes must define a plane</source>
      <translation>A kijelölt alakzatoknak síkot kell meghatároznia</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="81"/>
      <source>No graphical interface</source>
      <translation>Nincs grafikus interfész</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="161"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>Nem lehet új tárgyat beszúrni egy méretezett alkatrészbe</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="267"/>
      <source>Symbol not implemented. Using a default symbol.</source>
      <translation>A szimbólum nincs megvalósítva. Alapértelmezett szimbólum használata.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="333"/>
      <source>Visibility off; removed from list: </source>
      <translation>Láthatóság kikapcsolva; eltávolítva a listáról: </translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="603"/>
      <source>image is Null</source>
      <translation>a kép üres</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="609"/>
      <source>filename does not exist on the system or in the resource file</source>
      <translation>a fájlnév nem létezik a rendszeren vagy a forrásfájlban</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="668"/>
      <source>unable to load texture</source>
      <translation>anyagminta betöltése sikertelen</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/cut.py" line="57"/>
      <location filename="../../draftmake/make_pointarray.py" line="108"/>
      <location filename="../../draftmake/make_text.py" line="84"/>
      <location filename="../../draftmake/make_text.py" line="172"/>
      <location filename="../../draftmake/make_dimension.py" line="215"/>
      <location filename="../../draftmake/make_dimension.py" line="308"/>
      <location filename="../../draftmake/make_dimension.py" line="438"/>
      <location filename="../../draftmake/make_dimension.py" line="564"/>
      <location filename="../../draftmake/make_array.py" line="85"/>
      <location filename="../../draftmake/make_layer.py" line="58"/>
      <location filename="../../draftmake/make_layer.py" line="149"/>
      <location filename="../../draftmake/make_patharray.py" line="161"/>
      <location filename="../../draftmake/make_patharray.py" line="330"/>
      <location filename="../../draftmake/make_label.py" line="195"/>
      <location filename="../../draftutils/utils.py" line="1014"/>
      <location filename="../../draftutils/groups.py" line="95"/>
      <location filename="../../draftutils/gui_utils.py" line="720"/>
      <source>No active document. Aborting.</source>
      <translation>Nincs aktív dokumentum. Megszakítás.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="117"/>
      <location filename="../../draftmake/make_pointarray.py" line="128"/>
      <location filename="../../draftmake/make_circulararray.py" line="131"/>
      <location filename="../../draftmake/make_polararray.py" line="103"/>
      <location filename="../../draftmake/make_dimension.py" line="322"/>
      <location filename="../../draftmake/make_dimension.py" line="447"/>
      <location filename="../../draftmake/make_patharray.py" line="170"/>
      <location filename="../../draftmake/make_patharray.py" line="181"/>
      <location filename="../../draftmake/make_patharray.py" line="339"/>
      <location filename="../../draftmake/make_patharray.py" line="350"/>
      <location filename="../../draftmake/make_orthoarray.py" line="167"/>
      <location filename="../../draftmake/make_label.py" line="236"/>
      <location filename="../../draftutils/groups.py" line="132"/>
      <location filename="../../draftutils/gui_utils.py" line="729"/>
      <source>Wrong input: object not in document.</source>
      <translation>Helytelen bemenet: az tárgy nincs a dokumentumban.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="738"/>
      <source>Does not have 'ViewObject.RootNode'.</source>
      <translation>Nincs 'ViewObject.RootNode'.</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="51"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="58"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="65"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="154"/>
      <source>custom</source>
      <translation>egyéni</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="140"/>
      <source>Unable to convert input into a  scale factor</source>
      <translation>Nem lehet a bemenetet méretezési léptéktényezővé alakítani</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="155"/>
      <source>Set custom scale</source>
      <translation>Egyedi lépték beállítása</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="157"/>
      <source>Set custom annotation scale in format x:x, x=x</source>
      <translation>Egyéni jegyzetméret beállítása x:x, x=x formátumban</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="214"/>
      <source>Set the scale used by draft annotation tools</source>
      <translation>A jegyzeteszközök által használt méretezés beállítása</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="650"/>
      <source>Solids:</source>
      <translation>Szilárd testek:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="651"/>
      <source>Faces:</source>
      <translation>Felületek:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="652"/>
      <source>Wires:</source>
      <translation>Drótvázak:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="653"/>
      <source>Edges:</source>
      <translation>Élek:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="654"/>
      <source>Vertices:</source>
      <translation>Csúcspontok:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="658"/>
      <source>Face</source>
      <translation>Felület</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="663"/>
      <source>Wire</source>
      <translation>Drótháló</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="695"/>
      <location filename="../../draftutils/utils.py" line="699"/>
      <source>different types</source>
      <translation>különböző típusok</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="709"/>
      <source>Objects have different placements. Distance between the two base points: </source>
      <translation>A tárgyak különböző elhelyezéssel rendelkeznek. Távolság a két alappont között: </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="712"/>
      <source>has a different value</source>
      <translation>más az értéke</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="715"/>
      <source>doesn't exist in one of the objects</source>
      <translation>nem létezik az egyik tárgyban</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="827"/>
      <source>%s shares a base with %d other objects. Please check if you want to modify this.</source>
      <translation>%s megosztja az alappontot %d más tárgyakkal. Ellenőrizze, hogy módosítani szeretné-e ezt.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="833"/>
      <source>%s cannot be modified because its placement is readonly.</source>
      <translation>A(z) %s nem módosítható, mert az elhelyezése csak olvasható.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="977"/>
      <source>Wrong input: unknown document.</source>
      <translation>Helytelen bemenet: ismeretlen dokumentumban.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1055"/>
      <source>This function will be deprecated in </source>
      <translation>Ez a funkció elavult lesz a </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1056"/>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>Please use </source>
      <translation>Kérem használja </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>This function will be deprecated. </source>
      <translation>Ez a funkció ki lesz zárva. </translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="169"/>
      <source>Snap Lock</source>
      <translation>Illesztés zárolás</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="170"/>
      <source>Snap Endpoint</source>
      <translation>Végpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="171"/>
      <source>Snap Midpoint</source>
      <translation>Felezőpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="172"/>
      <source>Snap Center</source>
      <translation>Középpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="173"/>
      <source>Snap Angle</source>
      <translation>Szög illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="174"/>
      <source>Snap Intersection</source>
      <translation>Metszet illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="175"/>
      <source>Snap Perpendicular</source>
      <translation>Merőleges illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="176"/>
      <source>Snap Extension</source>
      <translation>Meghosszabbítás illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="177"/>
      <source>Snap Parallel</source>
      <translation>Párhuzamos illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="178"/>
      <source>Snap Special</source>
      <translation>Speciális illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="179"/>
      <source>Snap Near</source>
      <translation>Közeli illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="180"/>
      <source>Snap Ortho</source>
      <translation>Ortogonális illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="181"/>
      <source>Snap Grid</source>
      <translation>Rácshoz illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="182"/>
      <source>Snap WorkingPlane</source>
      <translation>Munkasíkra illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="183"/>
      <source>Snap Dimensions</source>
      <translation>Dimenzió illesztés</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="187"/>
      <source>Toggle Draft Grid</source>
      <translation>Tervrajz rácsok kapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="69"/>
      <source>ShapeString: string has no wires</source>
      <translation>ShapeString: A karakterláncnak nincsenek dróthálói</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="89"/>
      <location filename="../../draftobjects/draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>nézet tulajdonság hozzáadva 'ScaleMultiplier'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="125"/>
      <location filename="../../draftobjects/draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>áttelepített 'DraftText' típus a 'Szöveg' -re</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>, a tárgy útvonalnak nincsenek 'élei'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="395"/>
      <location filename="../../draftobjects/patharray.py" line="401"/>
      <location filename="../../draftobjects/patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>A 'PathObj' tulajdonságot áttelepíti a 'PathObject' -ba</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>Nem lehet kiszámítani az elérési út érintőt. A másolás nincs egy vonalban.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>A tangens és a normál párhuzamos. A másolás nincs egy vonalban.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>Az elérési út nem számítható ki aktuális értéken, az alapértelmezett értéket használja.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>Az elérési út binormális számítása nem számítható ki. A másolás nincs egy vonalban.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>AlignMode {} nincs megvalósítva</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="145"/>
      <location filename="../../draftobjects/pointarray.py" line="161"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>hozzáadott tulajdonság 'ExtraPlacement'</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>A tárgynak zárt alakzatnak kell lennie</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>Nem szilárd test tárgy</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>A területeket finomítani kell</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="435"/>
      <location filename="../../draftfunctions/downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation>Frissítés: Ismeretlen erőmódszer:</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="453"/>
      <source>Found groups: closing each open object inside</source>
      <translation>Talált csoportok: Bármely megnyitott objektum bezárása</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="459"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation>Hálót találtam: Alkatrészekké alakít</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="467"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation>Egy kialakítható tárgy található: megszilárdítjuk, felületet képzünk</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="472"/>
      <source>Found 2 objects: fusing them</source>
      <translation>Két tárgy található: egybeolvaszt</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="483"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>Több csatlakozott felülettel rendelkező tárgyat talált: finomítja ezeket</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="489"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>Találtunk 1 nem parametrikus tárgyat: tervrajzzá alakít</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="500"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>1 zárt vázlat tárgyat talált: létrehoz belőle egy felületet</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="505"/>
      <source>Found closed wires: creating faces</source>
      <translation>Zárt drótvázat talált: létrehoz felületeket</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="511"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>Több élt vagy szakaszt talált: összeköti őket</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="513"/>
      <location filename="../../draftfunctions/upgrade.py" line="547"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation>Találtam több nem kezelhető tárgyat: kapcsolat létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="518"/>
      <source>trying: closing it</source>
      <translation>próbálja: bezárni</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="520"/>
      <source>Found 1 open wire: closing it</source>
      <translation>1 nyílt szakaszt talált: bezárja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="537"/>
      <source>Found 1 object: draftifying it</source>
      <translation>Talált 1 tárgyat: tervrajzzá alakít</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="542"/>
      <source>Found points: creating compound</source>
      <translation>Pontokat talált: kapcsolat létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="550"/>
      <source>Unable to upgrade these objects.</source>
      <translation>Nem lehet frissíteni ezeket a tárgyakat.</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="90"/>
      <source>No object given</source>
      <translation>Nincs megadva tárgy</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>A két pont egybeesik</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="113"/>
      <source>mirrored</source>
      <translation>tükrözött</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>Talált 1 blokkot: szétrobbantom</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>Több szilárd anyagból készült kapcsolatot talált: szétrobbantom</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>Talált 1 parametrikus tárgyat: függőségeket felosztja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>Két tárgyat talált: kivonja egymásból</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>Több felület talált: felosztja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>Több tárgyat talált: kivonja az elsőből</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>Talált 1 felületet: drótvázzá bontja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation>Csak vonalak találhatók: éleiket kibontja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation>Nem lehet tovább visszaminősíteni</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="164"/>
      <location filename="../../draftmake/make_polararray.py" line="126"/>
      <location filename="../../draftmake/make_dimension.py" line="222"/>
      <location filename="../../draftmake/make_dimension.py" line="229"/>
      <location filename="../../draftmake/make_dimension.py" line="237"/>
      <location filename="../../draftmake/make_dimension.py" line="354"/>
      <location filename="../../draftmake/make_dimension.py" line="371"/>
      <location filename="../../draftmake/make_dimension.py" line="495"/>
      <location filename="../../draftmake/make_dimension.py" line="571"/>
      <location filename="../../draftmake/make_dimension.py" line="599"/>
      <location filename="../../draftmake/make_dimension.py" line="607"/>
      <location filename="../../draftmake/make_patharray.py" line="200"/>
      <location filename="../../draftmake/make_patharray.py" line="254"/>
      <location filename="../../draftmake/make_patharray.py" line="265"/>
      <location filename="../../draftmake/make_label.py" line="204"/>
      <source>Wrong input: must be a vector.</source>
      <translation>Rossz bemenet: vektornak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="147"/>
      <location filename="../../draftmake/make_text.py" line="107"/>
      <location filename="../../draftmake/make_label.py" line="215"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>Helytelen bemenet: elhelyezésnek, vektornak vagy forgatásnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="316"/>
      <location filename="../../draftmake/make_label.py" line="230"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>Helytelen bemenet: a tárgy nem lehet lista.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="213"/>
      <location filename="../../draftmake/make_label.py" line="251"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>Helytelen bemenet: karakter rekord listának vagy egyetlen karakternek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="263"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>Hibás bemenet: az al elem nincs az objektumban.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="272"/>
      <source>Wrong input: label_type must be a string.</source>
      <translation>Helytelen bevitel: label_type karakterláncnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="277"/>
      <source>Wrong input: label_type must be one of the following: </source>
      <translation>Helytelen bevitel: label_type az alábbiak egyikének kell lennie: </translation>
    </message>
    <message>
      <location filename="../../draftmake/make_text.py" line="91"/>
      <location filename="../../draftmake/make_text.py" line="96"/>
      <location filename="../../draftmake/make_label.py" line="286"/>
      <location filename="../../draftmake/make_label.py" line="291"/>
      <source>Wrong input: must be a list of strings or a single string.</source>
      <translation>Helytelen bemenet: karakterláncnak vagy egyetlen karakternek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="300"/>
      <location filename="../../draftmake/make_label.py" line="304"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>Helytelen bemenet: karakterláncnak kell lennie, 'Vízszintes', 'Függőleges' vagy 'Egyéni'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="119"/>
      <location filename="../../draftmake/make_layer.py" line="201"/>
      <location filename="../../draftmake/make_patharray.py" line="191"/>
      <location filename="../../draftmake/make_patharray.py" line="360"/>
      <location filename="../../draftmake/make_orthoarray.py" line="151"/>
      <location filename="../../draftmake/make_label.py" line="313"/>
      <source>Wrong input: must be a number.</source>
      <translation>Rossz bemenet: számnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="320"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>Helytele bemenet: legalább két vektorból kell állnia.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="353"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>Az irány nem 'Egyéni'; a pontokat nem fogja használni.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="380"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>Helytelen bemenet: két elemből kell állnia. Például [tárgy, 'Él1'].</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>Helytelen bemenet: a pont tárgy nem rendelkezik 'Geometriával', 'Hivatkozásokkal' vagy 'Összetevőkkel'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>Rétegek</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="145"/>
      <location filename="../../draftmake/make_layer.py" line="162"/>
      <location filename="../../draftguitools/gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>Réteg</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>Helytelen bemenet: karakterláncnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="167"/>
      <location filename="../../draftmake/make_layer.py" line="171"/>
      <location filename="../../draftmake/make_layer.py" line="184"/>
      <location filename="../../draftmake/make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>Helytelen bemenet: három lebegőpontos számnak kell lennie 0,0 és 1,0 között.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="208"/>
      <location filename="../../draftmake/make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>Helytelen bemenet: 'Szabványos', 'Szaggatott', 'Pontozott' vagy 'Pontozott vonal' kell, hogy legyen.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>Hibás bemenet: 0 és 100 közötti számnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>Ez a funkció elavult. Ne használja ezt a funkciót közvetlenül.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>Használja ezt 'make_linear_dimension', vagy ezt 'make_linear_dimension_obj'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="327"/>
      <location filename="../../draftmake/make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>Helytelen bemenet: a tárgynak nincs mérhető 'Alakzata'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>Helytelen bemenet: a tárgynak nincs legalább egy eleme a 'Csúcspontokban', amit a méréshez használna.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="338"/>
      <location filename="../../draftmake/make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>Rossz bemenet: egész számnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: az 1 -nél kisebb értékek nem megengedettek; 1-re lesz állítva.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="347"/>
      <location filename="../../draftmake/make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>Helytelen bemenet: a csúcspont nincs a tárgyban.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: az 1 -nél kisebb értékek nem megengedettek; a tárgy utolsó csúcspontjára lesz állítva.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>Helytelen bemenet: a tárgynak nincs legalább egy eleme a 'Éleken', amit a méréshez használna.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>index: az 1 -nél kisebb értékek nem megengedettek; 1-re lesz állítva.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>Helytelen bemenet: az index nem felel meg a tárgy egyik élének.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>Helytelen bemenet: az index nem felel meg körben futó élnek.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="483"/>
      <location filename="../../draftmake/make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>Helytelen bemenet: karakterláncnak, 'sugárnak' vagy 'átmérőnek' kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="579"/>
      <location filename="../../draftmake/make_dimension.py" line="586"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>Hibás bemenet: két szögből álló listának kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>Belső merőleges tömb</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>Hibás bemenet: számnak vagy vektornak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="92"/>
      <location filename="../../draftmake/make_orthoarray.py" line="95"/>
      <location filename="../../draftmake/make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>Bemenet: vektorosra bővített egyetlen érték.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="154"/>
      <location filename="../../draftmake/make_polararray.py" line="112"/>
      <location filename="../../draftmake/make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>Helytelen bemenet: egész számnak kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="123"/>
      <location filename="../../draftmake/make_orthoarray.py" line="126"/>
      <location filename="../../draftmake/make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>Bemenet: az elemek számának legalább 1-nek kell lennie. 1-re van állítva.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="275"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Orthogonal array</source>
      <translation>Merőleges elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>Merőleges elrendezés 2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>Négyszögletes elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>Négyszögletes elrendezés 2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="94"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <source>Polar array</source>
      <translation>Poláris elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>Hibás bemenet: 'Eredeti1, 'Frenet' vagy 'Érintő' kell, hogy legyen.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="125"/>
      <location filename="../../draftmake/make_arc_3points.py" line="130"/>
      <source>Points:</source>
      <translation>Pontok:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="126"/>
      <location filename="../../draftmake/make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>Rossz bemenet: pontosan három pont felsorolásának vagy rekordjának kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="138"/>
      <source>Placement:</source>
      <translation>Elhelyezés:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>Helytelen bemenet: helytelen elhelyezés.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>Helytelen bemenet: helytelen pont.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="159"/>
      <source>Cannot generate shape:</source>
      <translation>Az alakzat nem hozható létre:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>Sugár:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>Középpont:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>Alaptest tárgy létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="193"/>
      <location filename="../../draftmake/make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>Végső elhelyezés:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>Felület: Igaz</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>Támogatás:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>Térkép mód:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="104"/>
      <source>No shape found</source>
      <translation>Nem található alakzat</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="111"/>
      <source>All Shapes must be planar</source>
      <translation>Minden alakzatnak síkbelinek kell lennie</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="122"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="290"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <source>Circular array</source>
      <translation>Körkörös elrendelés</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>Hibás bemenet: számnak vagy mennyiségnek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="58"/>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>hossz:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>Két elem szükséges.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>Sugár túl nagy</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>Segment</source>
      <translation>Szegmens</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="165"/>
      <source>Removed original objects.</source>
      <translation>Eredeti objektumok eltávolítása.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="87"/>
      <source>Select an object to scale</source>
      <translation>Objektum kijelölése méretezéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="108"/>
      <source>Pick base point</source>
      <translation>Alap pont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="135"/>
      <source>Pick reference distance from base point</source>
      <translation>Referencia távolság kiválasztása az alap pontból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="206"/>
      <location filename="../../draftguitools/gui_scale.py" line="236"/>
      <location filename="../../draftguitools/gui_scale.py" line="359"/>
      <source>Scale</source>
      <translation>Méretezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="209"/>
      <source>Some subelements could not be scaled.</source>
      <translation>Egyes al-elemeket nem lehetett méretezni.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="339"/>
      <source>Unable to scale object:</source>
      <translation>A tárgy méretezése sikertelen:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="343"/>
      <source>Unable to scale objects:</source>
      <translation>A tárgyak méretezése sikertelen:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="346"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>Ez a tárgy típus nem méretezhető közvetlenül. Kérjük, használja a klónozási módszert.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="407"/>
      <source>Pick new distance from base point</source>
      <translation>Válassza ki az új távolságot az alap pontból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>Vázlatot túl bonyolult szerkeszteni: azt javasoljuk, hogy használja vázlatkészítő alapértelmezett szerkesztőt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="80"/>
      <source>Pick target point</source>
      <translation>Célpont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="157"/>
      <source>Create Label</source>
      <translation>Felirat létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="191"/>
      <location filename="../../draftguitools/gui_labels.py" line="218"/>
      <source>Pick endpoint of leader line</source>
      <translation>Vezérvonal végpontjának kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="201"/>
      <location filename="../../draftguitools/gui_labels.py" line="228"/>
      <source>Pick text position</source>
      <translation>Szöveg helyzetének kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>Stílusváltás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="77"/>
      <source>Pick location point</source>
      <translation>Válasszon ki pozíciót</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="121"/>
      <source>Create Text</source>
      <translation>Szöveg létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>Rács kapcsolása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>Válasszon egy felületet, 3 csúcspontot vagy egy munkasík proxyt a rajzsík meghatározásához</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
      <source>Working plane aligned to global placement of</source>
      <translation>A globális elhelyezéshez igazított munkasík</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
      <source>Dir</source>
      <translation>Irány</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
      <source>Custom</source>
      <translation>Egyéni</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>Felületek kijelölése meglévő tárgyakból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="73"/>
      <source>Select an object to mirror</source>
      <translation>Tárgy kiválasztása tükrözéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="92"/>
      <source>Pick start point of mirror line</source>
      <translation>Tükrözési egyenes kezdőpontjának kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="122"/>
      <source>Mirror</source>
      <translation>Tükrözés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="177"/>
      <location filename="../../draftguitools/gui_mirror.py" line="203"/>
      <source>Pick end point of mirror line</source>
      <translation>Tükrözési egyenes végpontjának kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="78"/>
      <location filename="../../draftguitools/gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>Középpont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="189"/>
      <location filename="../../draftguitools/gui_polygons.py" line="200"/>
      <location filename="../../draftguitools/gui_polygons.py" line="260"/>
      <location filename="../../draftguitools/gui_arcs.py" line="254"/>
      <location filename="../../draftguitools/gui_arcs.py" line="270"/>
      <location filename="../../draftguitools/gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>Sugár kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="277"/>
      <location filename="../../draftguitools/gui_arcs.py" line="278"/>
      <location filename="../../draftguitools/gui_arcs.py" line="446"/>
      <location filename="../../draftguitools/gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>Kezdő szög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="283"/>
      <location filename="../../draftguitools/gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>Kezdő fok kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="285"/>
      <location filename="../../draftguitools/gui_arcs.py" line="286"/>
      <location filename="../../draftguitools/gui_arcs.py" line="454"/>
      <location filename="../../draftguitools/gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation>Nyitási szög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation>Nyílás kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="317"/>
      <source>Create Circle (Part)</source>
      <translation>Kör létrehozása (rész)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>Kör létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>Ív létrehozása (rész)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>Ív létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation>Nyitás szögének kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="509"/>
      <location filename="../../draftguitools/gui_arcs.py" line="551"/>
      <source>Arc by 3 points</source>
      <translation>Körív 3 pontból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
      <location filename="../../draftguitools/gui_lines.py" line="83"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
      <source>Pick first point</source>
      <translation>Első pont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="163"/>
      <source>Create Line</source>
      <translation>Vonal létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="185"/>
      <source>Create Wire</source>
      <translation>Drótváz létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="218"/>
      <location filename="../../draftguitools/gui_lines.py" line="226"/>
      <location filename="../../draftguitools/gui_lines.py" line="233"/>
      <location filename="../../draftguitools/gui_lines.py" line="241"/>
      <location filename="../../draftguitools/gui_lines.py" line="251"/>
      <location filename="../../draftguitools/gui_beziers.py" line="148"/>
      <location filename="../../draftguitools/gui_splines.py" line="140"/>
      <source>Pick next point</source>
      <translation>Következő pont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="330"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>Nem sikerült létrehozni egy dróthálót a kijelölt tárgyból</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="352"/>
      <source>Convert to Wire</source>
      <translation>Dróthálóvá alakítja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
      <source>Pick ShapeString location point</source>
      <translation>Szövegforma pozíciópont kijelölése</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
      <source>Create ShapeString</source>
      <translation>Szövegalakzat létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>Szerkesztéshez tervrajz tárgyat választ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="558"/>
      <source>No edit point found for selected object</source>
      <translation>A kijelölt tárgyhoz nem található szerkesztési pont</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="811"/>
      <source>Too many objects selected, max number set to:</source>
      <translation>Túl sok tárgy van kijelölve, a beállított maximális szám:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="819"/>
      <source>: this object is not editable</source>
      <translation>: ez a tárgy nem szerkeszthető</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>Objektum kijelölése csatlakoztatáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>Egyenesek csatlakoztatása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>Kiválasztás:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>Lejtés módosítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="94"/>
      <source>Select objects to trim or extend</source>
      <translation>Válassza ki a tárgyakat a vágáshoz/nyújtáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="173"/>
      <location filename="../../draftguitools/gui_offset.py" line="143"/>
      <source>Pick distance</source>
      <translation>Távolság kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="222"/>
      <source>Offset angle</source>
      <translation>Eltolási szög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="483"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>Nem lehet vágni a tárgyakat, csak tervrajz vonalak és ívek támogatottak.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="488"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>Nem lehet ezeket a tárgyakat vágni, túl sok drótváz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="505"/>
      <source>These objects don't intersect.</source>
      <translation>Ezek az objektumok nem metszik egymást.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="508"/>
      <source>Too many intersection points.</source>
      <translation>Túl sok metszési pont.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>Jelöljön ki egy tárgyat konvertálásra.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>Alakítsa vázlattá</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>Alakítsa tervrajzzá</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>Alakítsa tervrajzzá/vázlattá</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>A parancs hívása előtt pontosan két tárgyat, az alap tárgyat és a ponttárgyat jelölje ki.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
      <source>Point array</source>
      <translation>Pont elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="108"/>
      <source>Select an object to edit</source>
      <translation>Jelöljön ki egy objektumot a szerkesztéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>Jelöljön ki egy tárgyat a klónozáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>Méretek létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>Méret létrehozása (Sugárirányú)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
      <source>Edge too short!</source>
      <translation>Túl rövid az él!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
      <source>Edges don't intersect!</source>
      <translation>Élek nem metszik egymást!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="75"/>
      <source>Select an object to stretch</source>
      <translation>Jelöljön ki egy tárgyat a nyújtáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="127"/>
      <source>Pick first point of selection rectangle</source>
      <translation>Válassza ki az első pontot a téglalap kijelölésén</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="164"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>Válassza ki a második pontot a téglalap kijelölésén</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="173"/>
      <source>Pick start point of displacement</source>
      <translation>Elmozdulás kezdőpontjának kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="236"/>
      <source>Pick end point of displacement</source>
      <translation>Elmozdulás végpontjának kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="448"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>Egy téglalap átalakítása drótvázzá</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="477"/>
      <source>Stretch</source>
      <translation>Nyújtás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="102"/>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>A parancs hívása előtt jelöljön ki pontosan két tárgyat, az alap tárgyat és az útvonal tárgyat.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation>Útvonal csavart elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="132"/>
      <location filename="../../draftguitools/gui_beziers.py" line="332"/>
      <source>Bézier curve has been closed</source>
      <translation>Bezier-görbe lezárásra került</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="140"/>
      <location filename="../../draftguitools/gui_beziers.py" line="368"/>
      <location filename="../../draftguitools/gui_splines.py" line="131"/>
      <source>Last point has been removed</source>
      <translation>Utolsó pont eltávolítva</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="153"/>
      <location filename="../../draftguitools/gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>Következő pont kiválasztása vagy befejezés (A) vagy zárás (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="211"/>
      <location filename="../../draftguitools/gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>Bézier-görbe létrehozás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>Fogd és vidd a következő csomópont meghatározásához</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>Fogd és vidd a következő csomópont meghatározásához: vagy befejez (A) vagy bezár (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1543"/>
      <source>(ON)</source>
      <translation>(BE)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1546"/>
      <source>(OFF)</source>
      <translation>(KI)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="67"/>
      <location filename="../../draftguitools/gui_upgrade.py" line="67"/>
      <source>Select an object to upgrade</source>
      <translation>Jelöljön ki egy tárgyat a frissítéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation>Lefokoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation>Útvonal elrendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>Görbe lezárva</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>B-görbe létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
      <source>Create Plane</source>
      <translation>Sík létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
      <source>Create Rectangle</source>
      <translation>Téglalap rajzolása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
      <source>Pick opposite point</source>
      <translation>Ellenkező pont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>Lekerekítés sugara</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>Lekerekítés sugara</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="107"/>
      <source>Enter radius.</source>
      <translation>Sugár megadása.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="126"/>
      <source>Delete original objects:</source>
      <translation>Eredeti tárgyak törlése:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="131"/>
      <source>Chamfer mode:</source>
      <translation>Letörés mód:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="148"/>
      <source>Two elements needed.</source>
      <translation>Két elem szükséges.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="155"/>
      <source>Test object</source>
      <translation>Teszt tárgy</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="156"/>
      <source>Test object removed</source>
      <translation>Teszt tárgy eltávolítva</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="158"/>
      <source>Fillet cannot be created</source>
      <translation>Nem hozható létre lekerekítés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="188"/>
      <source>Create fillet</source>
      <translation>Lekerekítés létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="65"/>
      <source>Add to group</source>
      <translation>Hozzáadás a csoporthoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="68"/>
      <source>Ungroup</source>
      <translation>Csoportbontás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="70"/>
      <source>Add new group</source>
      <translation>Új csoport hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>Csoport kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="193"/>
      <source>No new selection. You must select non-empty groups or objects inside groups.</source>
      <translation>Nincs új választás. A csoportokon belül nem üres csoportokat vagy tárgyakat kell kijelölnie.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="203"/>
      <source>Autogroup</source>
      <translation>Autocsoport</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="250"/>
      <source>Add new Layer</source>
      <translation>Új réteg hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="304"/>
      <source>Add to construction group</source>
      <translation>Hozzáadás az építési csoporthoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="355"/>
      <source>Add a new group with a given name</source>
      <translation>Adjon hozzá egy új csoportot egy adott névvel</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="383"/>
      <source>Add group</source>
      <translation>Csoport hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="385"/>
      <source>Group name</source>
      <translation>Csoportnév</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="392"/>
      <source>Group</source>
      <translation>Csoport</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="92"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="569"/>
      <source>This object does not support possible coincident points, please try again.</source>
      <translation>Ez a tárgy nem támogatja a lehetséges véletlen pontokat, próbálkozzon újra.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>Aktív objektumnak kettőnél több pontot/csomópontot kell tartalmaznia</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
      <source>Selection is not a Knot</source>
      <translation>A kiválasztás nem egy csomó</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>A Bezgörbe végpontja nem simítható</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>A rajz munkafelület elavult 0,17 óta, fontolja meg a MűszakiRajz munkafelületet helyette.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="81"/>
      <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
      <source>Select an object to project</source>
      <translation>Objektum kijelölése vetítéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>Frissít</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="126"/>
      <source>Main toggle snap</source>
      <translation>Fő illesztés kapcsoló</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="157"/>
      <source>Midpoint snap</source>
      <translation>Középpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="187"/>
      <source>Perpendicular snap</source>
      <translation>Merőleges illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="217"/>
      <source>Grid snap</source>
      <translation>Rácshoz igazítás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="247"/>
      <source>Intersection snap</source>
      <translation>Metszéspont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="277"/>
      <source>Parallel snap</source>
      <translation>Párhuzamos illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="307"/>
      <source>Endpoint snap</source>
      <translation>Végpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="338"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>Szög illesztés (30 és 45 fok)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="368"/>
      <source>Arc center snap</source>
      <translation>Ív középpont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="398"/>
      <source>Edge extension snap</source>
      <translation>Élbővítmény illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="428"/>
      <source>Near snap</source>
      <translation>Közeli illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="459"/>
      <source>Orthogonal snap</source>
      <translation>Merőleges illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="489"/>
      <source>Special point snap</source>
      <translation>Speciális pont illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="520"/>
      <source>Dimension display</source>
      <translation>Méret megjelenítése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="553"/>
      <source>Working plane snap</source>
      <translation>Munkasík illesztés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="583"/>
      <source>Show snap toolbar</source>
      <translation>Illesztési eszköztár megjelenítése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="81"/>
      <source>Select an object to move</source>
      <translation>Tárgy kijelölése mozgatáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="103"/>
      <source>Pick start point</source>
      <translation>Kezdőpont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="162"/>
      <location filename="../../draftguitools/gui_move.py" line="308"/>
      <source>Pick end point</source>
      <translation>Végpont kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="210"/>
      <source>Move</source>
      <translation>Mozgat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="289"/>
      <source>Some subelements could not be moved.</source>
      <translation>Egyes al elemeket nem lehetett áthelyezni.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
      <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
      <source>Create Ellipse</source>
      <translation>Ellipszis létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>Méretek megfordítása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>Nincs aktív tervrajz eszköztár.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>Építési mód</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>Folytatási mód</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>Megjelenítési mód váltása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>Jegyzetstílus-szerkesztő</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
      <source>Open styles file</source>
      <translation>Stílusfájl megnyitása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
      <source>JSON file (*.json)</source>
      <translation>JSON fájl (*.json)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
      <source>Save styles file</source>
      <translation>Stílusfájl elmentése</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>Gyógyítani</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="134"/>
      <location filename="../../draftguitools/gui_points.py" line="147"/>
      <source>Create Point</source>
      <translation>Pont létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>Sokszög létrehozása (alkatrész)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>Sokszög létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>Egy tárgy kijelölése az eltoláshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>Egyszerre csak egy tárgyon működik az eltolás.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>Ez a tárgy típus nem tolható el</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="123"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>Bézier görbe eltolás jelenleg még nem támogatott</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>Jelöljön ki egy tárgyat elforgatáshoz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="99"/>
      <source>Pick rotation center</source>
      <translation>Válasszon ki elforgatás középpontot</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="193"/>
      <location filename="../../draftguitools/gui_rotate.py" line="396"/>
      <source>Base angle</source>
      <translation>Alapszög</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="194"/>
      <location filename="../../draftguitools/gui_rotate.py" line="397"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>Az alapszög, amelyből a forgatás elkezdődik</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="199"/>
      <location filename="../../draftguitools/gui_rotate.py" line="400"/>
      <source>Pick base angle</source>
      <translation>Alap szög kiválasztása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="205"/>
      <location filename="../../draftguitools/gui_rotate.py" line="409"/>
      <source>Rotation</source>
      <translation>Forgatás</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="206"/>
      <location filename="../../draftguitools/gui_rotate.py" line="410"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>A végrehajtani kívánt forgatási mennyiség.
A végső szög lesz az alapszög plusz ez az összege.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="418"/>
      <source>Pick rotation angle</source>
      <translation>Válasszon ki elforgatási szöget</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
      <source>Create 2D view</source>
      <translation>2D nézet létrehozása</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>Elem kiválasztása elrendezéshez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>Sorba rendezés</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>Kattintson bárhol egy vonalon, annak felosztásához.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>Vonal felosztása</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>Feladat panel:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
      <source>At least one element must be selected.</source>
      <translation>Legalább egy tételt ki kell választani.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
      <source>Number of elements must be at least 1.</source>
      <translation>A tételek számának legalább 1-nek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
      <source>Selection is not suitable for array.</source>
      <translation>A kijelölés nem alkalmas tömbökhöz.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
      <source>Object:</source>
      <translation>Tárgy:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
      <source>Interval X reset:</source>
      <translation>X intervallum alaphelyzetbe állítása:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
      <source>Interval Y reset:</source>
      <translation>Y intervallum alaphelyzetbe állítása:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
      <source>Interval Z reset:</source>
      <translation>Z intervallum alaphelyzetbe állítása:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
      <source>Fuse:</source>
      <translation>Egybeolvaszt:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
      <source>Create Link array:</source>
      <translation>Elrendezés csatolás létrehozása:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
      <source>Number of X elements:</source>
      <translation>X tételek száma:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
      <source>Interval X:</source>
      <translation>X intervallum:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
      <source>Number of Y elements:</source>
      <translation>Y tételek száma:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
      <source>Interval Y:</source>
      <translation>Y intervallum:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
      <source>Number of Z elements:</source>
      <translation>Z tételek száma:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
      <source>Interval Z:</source>
      <translation>Z intervallum:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Aborted:</source>
      <translation>Megszakított:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
      <source>Number of layers must be at least 2.</source>
      <translation>A rétegek számának legalább 2-nek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>A sugárirányú távolság nulla. Előfordulhat, hogy az eredményül kapott elrendezés nem tűnik megfelelőnek.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>A sugárirányú távolság negatív. Pozitívvá alakítva a továbblépéshez.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>Az érintőtávolság nem lehet nulla.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>Az érintő távolság negatív. Pozitívvá alakítva a továbblépéshez.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
      <source>Center reset:</source>
      <translation>Középre visszaállítás:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
      <source>Radial distance:</source>
      <translation>Sugárirányú távolság:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
      <source>Tangential distance:</source>
      <translation>Érintőtávolság:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
      <source>Number of circular layers:</source>
      <translation>Körkörös rétegek száma:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
      <source>Symmetry parameter:</source>
      <translation>Szimmetria paraméter:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
      <source>Center of rotation:</source>
      <translation>Forgatás középpontja:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
      <source>Number of elements must be at least 2.</source>
      <translation>Az tételek számának legalább 2-nek kell lennie.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>A szög meghaladja a 360 fokot. Ez az érték a folytatáshoz van beállítva.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>A szög nem éri el a 360 fokot. Ez az érték a folytatáshoz van beállítva.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
      <source>Number of elements:</source>
      <translation>Tételek száma:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
      <source>Polar angle:</source>
      <translation>Poláris szög:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
      <source>ShapeString</source>
      <translation>AlakzatSzövegből</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
      <source>Default</source>
      <translation>Alapértelmezett</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="361"/>
      <source>Activate this layer</source>
      <translation>A réteg aktiválása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="367"/>
      <source>Select layer contents</source>
      <translation>Réteg tartalom kijelölése</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="405"/>
      <location filename="../../draftviewproviders/view_layer.py" line="421"/>
      <source>Merge layer duplicates</source>
      <translation>Megsokszorozott rétegek egyesítése</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="410"/>
      <location filename="../../draftviewproviders/view_layer.py" line="469"/>
      <source>Add new layer</source>
      <translation>Új réteg hozzáadása</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="454"/>
      <source>Relabeling layer:</source>
      <translation>A réteg újracímkézése:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="458"/>
      <source>Merging layer:</source>
      <translation>Réteg összeolvasztás:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="402"/>
      <source>Please load the Draft Workbench to enable editing this object</source>
      <translation>A tárgy szerkesztésének engedélyezéséhez töltse be a Munkafelület piszkozatát</translation>
    </message>
  </context>
  <context>
    <name>importOCA</name>
    <message>
      <location filename="../../importOCA.py" line="360"/>
      <source>OCA error: couldn't determine character encoding</source>
      <translation>OCA hiba: nem sikerült meghatározni a karakterkódolást</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="445"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: nem talált exportálandó adatot</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="490"/>
      <source>successfully exported</source>
      <translation>sikeresen exportálva</translation>
    </message>
  </context>
</TS>
